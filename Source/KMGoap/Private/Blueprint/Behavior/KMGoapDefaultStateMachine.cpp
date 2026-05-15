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
