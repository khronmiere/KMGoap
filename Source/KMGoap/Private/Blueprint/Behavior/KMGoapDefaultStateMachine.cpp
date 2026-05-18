// All rights reserved by Khrönmière Entertainment.
#include "Blueprint/Behavior/KMGoapDefaultStateMachine.h"
#include "Blueprint/KMGoapAgentAction.h"
#include "Blueprint/KMGoapAgentGoal.h"
#include "Blueprint/Component/KMGoapAgentComponent.h"

DEFINE_LOG_CATEGORY_STATIC(LogGoapDefaultStateMachine, Log, All);

void UKMGoapDefaultStateMachine::Start_Implementation(UKMGoapAgentComponent* NewAgent)
{
	Agent = NewAgent;

	// Start from a clean runtime state so reused state machine instances never inherit a stale plan.
	ResetExecutionState(true);

	UE_LOG(LogGoapDefaultStateMachine, Log, TEXT("State Machine Started"));
}

void UKMGoapDefaultStateMachine::Stop_Implementation()
{
	// Stop does not clear the assigned agent. The object may be restarted for the same agent later,
	// but any in-flight action/plan must be discarded.
	ResetExecutionState(true);

	UE_LOG(LogGoapDefaultStateMachine, Log, TEXT("State Machine Stopped"));
}

void UKMGoapDefaultStateMachine::Tick_Implementation(float DeltaTime)
{
	if (!CurrentAction)
	{
		// Planning is requested only when execution needs a new action. Async requests
		// are issued once and completed through planner callbacks, so the game thread
		// never blocks while the search runs.
		if (!CurrentPlan.IsValid())
		{
			if (!bIsWaitingForPlan)
			{
				UE_LOG(LogGoapDefaultStateMachine, Log, TEXT("No Plan, Calculating a new one"));
				CalculatePlan();
			}

			return;
		}

		if (CurrentPlan.IsValid())
		{
			UpdateExecutionState();

			// Preconditions are checked immediately before execution because beliefs/facts may have
			// changed since the planner produced this action sequence.
			if (Agent->ValidateActionPreconditions(CurrentAction))
			{
				CurrentAction->StartAction(Agent);
			}
			else
			{
				UE_LOG(LogGoapDefaultStateMachine, Log, TEXT("Preconditions not met. Clearing current action/goal."));
				ReleaseSelectedAction();
				ResetExecutionState(false);
			}
		}
		
		// Prevent running initialization and ticking in the same frame
		return;
	}

	if (CurrentAction)
	{
		CurrentAction->TickAction(Agent, DeltaTime);

		if (CurrentAction->IsComplete())
		{
			FinishCurrentAction();

			// An exhausted plan means the selected goal was fully serviced. Preserve it as LastGoal
			// so the planner can use continuity/anti-thrashing rules on the next search.
			if (!CurrentPlan.IsValid())
			{
				LastGoal = CurrentGoal;
				ResetExecutionState(false);
			}
		}
	}
}

void UKMGoapDefaultStateMachine::FinishCurrentAction()
{
	if (!CurrentAction)
	{
		return;
	}

	CurrentAction->StopAction(Agent);
	CurrentAction->Release(Agent);
	CurrentAction = nullptr;
}

void UKMGoapDefaultStateMachine::InterruptCurrentAction()
{
	if (!CurrentAction)
	{
		return;
	}

	if (CurrentAction->GetStatus() == EKMGoapActionStatus::Running)
	{
		CurrentAction->StopAction(Agent);
	}

	CurrentAction->Release(Agent);
	CurrentAction = nullptr;
}

void UKMGoapDefaultStateMachine::ReleaseSelectedAction()
{
	if (!CurrentAction)
	{
		return;
	}

	CurrentAction->Release(Agent);
	CurrentAction = nullptr;
}

void UKMGoapDefaultStateMachine::ResetExecutionState(bool bInterruptActiveAction)
{
	CancelPendingPlanRequest();

	if (bInterruptActiveAction)
	{
		InterruptCurrentAction();
	}
	else
	{
		ReleaseSelectedAction();
	}

	CurrentGoal = nullptr;
	CurrentPlan.Reset();
}

void UKMGoapDefaultStateMachine::Reset_Implementation()
{
	ResetExecutionState(true);
	UE_LOG(LogGoapDefaultStateMachine, Log, TEXT("Execution Plan Reset executed"));
}

void UKMGoapDefaultStateMachine::OnSensorStateUpdate_Implementation()
{
	// Sensor changes can invalidate both the selected goal and any action preconditions.
	// Replanning from scratch is safer than trying to patch the existing sequence.
	ResetExecutionState(true);

	UE_LOG(LogGoapDefaultStateMachine, Log, TEXT("Sensor Update received"));
}

void UKMGoapDefaultStateMachine::CalculatePlan()
{
	if (!Agent)
	{
		UE_LOG(LogGoapDefaultStateMachine, Warning, TEXT("CalculatePlan failed: Agent is null."));
		return;
	}

	if (bIsWaitingForPlan)
	{
		return;
	}

	float CurrentPriority = 0.f;
	if (CurrentGoal)
	{
		CurrentPriority = CurrentGoal->GetPriority(Agent);
	}

	const TMap<FGameplayTag, TObjectPtr<UKMGoapAgentGoal>>& GoalsByTag = Agent->GoalsByTag;
	TArray<UKMGoapAgentGoal*> GoalsToCheck;
	GoalsToCheck.Reserve(GoalsByTag.Num());

	for (const TTuple<FGameplayTag, TObjectPtr<UKMGoapAgentGoal>>& Pair : GoalsByTag)
	{
		UKMGoapAgentGoal* Goal = Pair.Value;
		if (!Goal)
		{
			continue;
		}

		// If there is no active goal, let the planner evaluate every valid goal.
		// The planner will sort/filter by priority and satisfaction state.
		if (!CurrentGoal)
		{
			GoalsToCheck.Add(Goal);
			continue;
		}

		const float GoalPriority = Goal->GetPriority(Agent);

		if (Goal == CurrentGoal)
		{
			if (bIncludeCurrentGoalWhenReplanning)
			{
				GoalsToCheck.Add(Goal);
			}

			continue;
		}

		const float PriorityDelta = GoalPriority - CurrentPriority;
		if (PriorityDelta > GoalSwitchPriorityMargin ||
			(bAllowEqualPriorityGoalSwitching && FMath::IsNearlyZero(PriorityDelta)))
		{
			GoalsToCheck.Add(Goal);
		}
	}

	if (GoalsToCheck.IsEmpty())
	{
		UE_LOG(LogGoapDefaultStateMachine, Verbose, TEXT("CalculatePlan: no candidate goals to evaluate."));
		return;
	}

	FKMGoapOnPlanAcquired OnPlanAcquired;
	OnPlanAcquired.BindUObject(this, &UKMGoapDefaultStateMachine::HandlePlanAcquired);

	FKMGoapOnPlanFailed OnPlanFailed;
	OnPlanFailed.BindUObject(this, &UKMGoapDefaultStateMachine::HandlePlanFailed);

	PendingPlanHandle = Agent->RequestPlanForGoalsAsync(
		GoalsToCheck,
		LastGoal,
		MoveTemp(OnPlanAcquired),
		MoveTemp(OnPlanFailed));

	bIsWaitingForPlan = PendingPlanHandle.IsValid();

	if (!bIsWaitingForPlan)
	{
		UE_LOG(LogGoapDefaultStateMachine, Verbose, TEXT("CalculatePlan: async request could not be queued."));
	}
}

void UKMGoapDefaultStateMachine::HandlePlanAcquired(
	const FKMGoapPlanningRequestHandle& Handle,
	FKMGoapActionPlan&& Plan)
{
	if (!IsCurrentPlanRequest(Handle))
	{
		return;
	}

	bIsWaitingForPlan = false;
	PendingPlanHandle.Reset();

	if (!Plan.IsValid())
	{
		UE_LOG(LogGoapDefaultStateMachine, Verbose, TEXT("HandlePlanAcquired: received invalid plan."));
		return;
	}

	CurrentPlan = MoveTemp(Plan);
}

void UKMGoapDefaultStateMachine::HandlePlanFailed(const FKMGoapPlanningRequestHandle& Handle)
{
	if (!IsCurrentPlanRequest(Handle))
	{
		return;
	}

	bIsWaitingForPlan = false;
	PendingPlanHandle.Reset();

	UE_LOG(LogGoapDefaultStateMachine, Verbose, TEXT("CalculatePlan: no valid plan found."));
}

void UKMGoapDefaultStateMachine::CancelPendingPlanRequest()
{
	if (!PendingPlanHandle.IsValid())
	{
		bIsWaitingForPlan = false;
		return;
	}

	if (Agent)
	{
		Agent->CancelPlanRequest(PendingPlanHandle);
	}

	PendingPlanHandle.Reset();
	bIsWaitingForPlan = false;
}

bool UKMGoapDefaultStateMachine::IsCurrentPlanRequest(const FKMGoapPlanningRequestHandle& Handle) const
{
	return bIsWaitingForPlan &&
		PendingPlanHandle.IsValid() &&
		Handle.IsValid() &&
		PendingPlanHandle == Handle;
}

void UKMGoapDefaultStateMachine::UpdateExecutionState()
{
	if (!CurrentPlan.IsValid())
	{
		UE_LOG(LogGoapDefaultStateMachine, Log, TEXT("Attempt to update execution state with invalid plan"));
		return;
	}

	CurrentGoal = CurrentPlan.Goal;

	// Plans are consumed from the front. The state machine owns only the active action at any time;
	// the remaining actions stay queued in CurrentPlan.
	CurrentAction = CurrentPlan.Actions[0];
	CurrentPlan.Actions.RemoveAt(0);
	UE_LOG(LogGoapDefaultStateMachine, Log, TEXT("Execution State Updated. Next Action: %s"), *CurrentAction->GetName());
}
