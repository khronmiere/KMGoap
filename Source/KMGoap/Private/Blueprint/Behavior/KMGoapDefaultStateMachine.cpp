// Fill out your copyright notice in the Description page of Project Settings.


#include "Blueprint/Behavior/KMGoapDefaultStateMachine.h"

#include "Blueprint/KMGoapAgentAction.h"
#include "Blueprint/KMGoapAgentGoal.h"
#include "Blueprint/Component/KMGoapAgentComponent.h"

DEFINE_LOG_CATEGORY_STATIC(LogGoapDefaultStateMachine, Log, All);

void UKMGoapDefaultStateMachine::Start_Implementation(UKMGoapAgentComponent* NewAgent)
{
	Agent = NewAgent;
	ResetExecutionState();
	UE_LOG(LogGoapDefaultStateMachine, Log, TEXT("State Machine Started"));
}

void UKMGoapDefaultStateMachine::Stop_Implementation()
{
	ResetExecutionState();
	UE_LOG(LogGoapDefaultStateMachine, Log, TEXT("State Machine Stopped"));
}

void UKMGoapDefaultStateMachine::Tick_Implementation(float DeltaTime)
{
	if (!CurrentAction)
	{
		UE_LOG(LogGoapDefaultStateMachine, Log, TEXT("No Action, Calculating a new Plan"));
		CalculatePlan();

		if (CurrentPlan.IsValid())
		{
			UpdateExecutionState();
			if (Agent->ValidateActionPreconditions(CurrentAction))
			{
				CurrentAction->StartAction(Agent);
			}
			else
			{
				UE_LOG(LogGoapDefaultStateMachine, Log, TEXT("Preconditions not met. Clearing current action/goal."));
				ResetExecutionState();
			}
		}
	}
	
	if (CurrentAction)
	{
		UE_LOG(LogGoapDefaultStateMachine, Log, TEXT("Tick %s"), *CurrentAction->GetName());
		if (!Agent->ValidateActionPreconditions(CurrentAction))
		{
			ResetExecutionState();
			return;
		}
		
		CurrentAction->TickAction(Agent, DeltaTime);
		if (CurrentAction->IsComplete())
		{
			CurrentAction->StopAction(Agent);
			CurrentAction = nullptr;
			if (!CurrentPlan.IsValid())
			{
				LastGoal = CurrentGoal;
				ResetExecutionState();
			}
		}
	}
}

void UKMGoapDefaultStateMachine::OnSensorStateUpdate_Implementation()
{
	ResetExecutionState();
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

		if (!CurrentGoal || GoalPriority > CurrentPriority)
		{
			GoalsToCheck.Add(Goal);
		}
	}

	FKMGoapActionPlan NewPlan;
	if (Agent->ComputePlanForGoals(GoalsToCheck, LastGoal, NewPlan) && NewPlan.IsValid())
	{
		CurrentPlan = MoveTemp(NewPlan);
	}
}

void UKMGoapDefaultStateMachine::UpdateExecutionState()
{
	CurrentGoal = CurrentPlan.Goal;
	CurrentAction = CurrentPlan.Actions[0];
	CurrentPlan.Actions.RemoveAt(0);
}

void UKMGoapDefaultStateMachine::ResetExecutionState()
{
	CurrentAction = nullptr;
	CurrentGoal = nullptr;
	CurrentPlan.Reset();
}
