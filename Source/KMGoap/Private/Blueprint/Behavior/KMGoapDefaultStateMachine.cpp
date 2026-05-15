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
	ResetExecutionState();

	UE_LOG(LogGoapDefaultStateMachine, Log, TEXT("State Machine Started"));
}

void UKMGoapDefaultStateMachine::Stop_Implementation()
{
	// Stop does not clear the assigned agent. The object may be restarted for the same agent later,
	// but any in-flight action/plan must be discarded.
	ResetExecutionState();

	UE_LOG(LogGoapDefaultStateMachine, Log, TEXT("State Machine Stopped"));
}

void UKMGoapDefaultStateMachine::Tick_Implementation(float DeltaTime)
{
	if (!CurrentAction)
	{
		UE_LOG(LogGoapDefaultStateMachine, Log, TEXT("No Action, Calculating a new Plan"));

		// Planning is deferred until an action is needed. This keeps sensor invalidations cheap:
		// they only clear state, and the next tick decides whether a new plan is still required.
		if (!CurrentPlan.IsValid())
		{
			CalculatePlan();
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
				// Stop the action before clearing it
				CurrentAction->StopAction(Agent);
				ResetExecutionState();
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
			CurrentAction->StopAction(Agent);
			CurrentAction = nullptr;

			// An exhausted plan means the selected goal was fully serviced. Preserve it as LastGoal
			// so the planner can use continuity/anti-thrashing rules on the next search.
			if (!CurrentPlan.IsValid())
			{
				LastGoal = CurrentGoal;
				ResetExecutionState();
			}
		}
	}
}

void UKMGoapDefaultStateMachine::Reset_Implementation()
{
	ResetExecutionState();
	UE_LOG(LogGoapDefaultStateMachine, Log, TEXT("Execution Plan Reset executed"));
}

void UKMGoapDefaultStateMachine::OnSensorStateUpdate_Implementation()
{
	// Sensor changes can invalidate both the selected goal and any action preconditions.
	// Replanning from scratch is safer than trying to patch the existing sequence.
	ResetExecutionState();

	UE_LOG(LogGoapDefaultStateMachine, Log, TEXT("Sensor Update received"));
}

void UKMGoapDefaultStateMachine::CalculatePlan()
{
	float CurrentPriority = 0.f;
	if (CurrentGoal)
	{
		CurrentPriority = CurrentGoal->GetPriority(Agent);
	}

	const TMap<FGameplayTag, TObjectPtr<UKMGoapAgentGoal>>& GoalsByTag = Agent->GoalsByTag;
	TArray<UKMGoapAgentGoal*> GoalsToCheck;
	GoalsToCheck.Reserve(GoalsByTag.Num());

	for (auto& Pair : GoalsByTag)
	{
		UKMGoapAgentGoal* Goal = Pair.Value;
		if (!Goal || Goal == CurrentGoal) continue;

		const float GoalPriority = Goal->GetPriority(Agent);

		// Keep the current goal unless another goal has strictly higher priority.
		// This avoids unnecessary goal churn when priorities are equal.
		if (!CurrentGoal || GoalPriority > CurrentPriority)
		{
			GoalsToCheck.Add(Goal);
		}
	}

	FKMGoapActionPlan NewPlan;

	// The planner receives LastGoal as context so search implementations can bias against
	// oscillating between recently completed goals if they choose to.
	if (Agent->ComputePlanForGoals(GoalsToCheck, LastGoal, NewPlan) && NewPlan.IsValid())
	{
		CurrentPlan = MoveTemp(NewPlan);
	}
}

void UKMGoapDefaultStateMachine::UpdateExecutionState()
{
	CurrentGoal = CurrentPlan.Goal;

	// Plans are consumed from the front. The state machine owns only the active action at any time;
	// the remaining actions stay queued in CurrentPlan.
	CurrentAction = CurrentPlan.Actions[0];
	CurrentPlan.Actions.RemoveAt(0);
}

void UKMGoapDefaultStateMachine::ResetExecutionState()
{
	CurrentAction->Release(Agent);
	CurrentAction = nullptr;
	CurrentGoal = nullptr;
	CurrentPlan.Reset();
}
