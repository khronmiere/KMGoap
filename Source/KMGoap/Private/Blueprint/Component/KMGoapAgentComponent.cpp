// Fill out your copyright notice in the Description page of Project Settings.


#include "Blueprint/Component/KMGoapAgentComponent.h"

#include "GameplayTagContainer.h"
#include "Blueprint/KMGoapAgentAction.h"
#include "Blueprint/KMGoapAgentBelief.h"
#include "Blueprint/KMGoapAgentGoal.h"
#include "Blueprint/Data/KMGoapActionSet.h"
#include "Blueprint/Data/KMGoapBeliefSet.h"
#include "Blueprint/Data/KMGoapGoalSet.h"
#include "Data/KMGoapCondition.h"
#include "Interface/KMGoapSensorInterface.h"
#include "Subsystem/KMGoapPlannerSubsystem.h"

DEFINE_LOG_CATEGORY_STATIC(LogGoapAgent, Log, All);

// Sets default values for this component's properties
UKMGoapAgentComponent::UKMGoapAgentComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}

UKMGoapAgentBelief* UKMGoapAgentComponent::GetBeliefByTag(FGameplayTag Tag) const
{
	if (BeliefsByTag.Contains(Tag))
	{
		return BeliefsByTag[Tag];
	}
	return nullptr;
}

UActorComponent* UKMGoapAgentComponent::GetSensorByTag(FGameplayTag Tag) const
{
	if (SensorsByTag.Contains(Tag))
	{
		return SensorsByTag[Tag];
	}
	return nullptr;
}

UKMGoapAgentGoal* UKMGoapAgentComponent::GetGoalByTag(FGameplayTag Tag) const
{
	if (GoalsByTag.Contains(Tag))
	{
		return GoalsByTag[Tag];
	}
	return nullptr;
}

UKMGoapAgentAction* UKMGoapAgentComponent::GetActionByTag(FGameplayTag Tag) const
{
	if (ActionsByTag.Contains(Tag))
	{
		return ActionsByTag[Tag];
	}
	return nullptr;
}

bool UKMGoapAgentComponent::EvaluateBeliefByTag(FGameplayTag Tag) const
{
	if (UKMGoapAgentBelief* Belief = GetBeliefByTag(Tag))
	{
		return Belief->Evaluate();
	}
	return false;
}

FVector UKMGoapAgentComponent::GetBeliefLocationByTag(FGameplayTag Tag) const
{
	if (UKMGoapAgentBelief* Belief = GetBeliefByTag(Tag))
	{
		return Belief->GetLocation();
	}
	return FVector::ZeroVector;
}

void UKMGoapAgentComponent::SetFact(FGameplayTag FactTag, bool bValue)
{
	if (!FactTag.IsValid()) return;
	if (Facts.Contains(FactTag))
	{
		Facts[FactTag] = bValue;
	}
	Facts.Add(FactTag, bValue);
}

bool UKMGoapAgentComponent::GetFact(FGameplayTag Tag) const
{
	return Facts.Contains(Tag) ? Facts[Tag] : false;
}

void UKMGoapAgentComponent::BeginPlay()
{
	Super::BeginPlay();

	CacheSensors();
	BuildBeliefs();
	BuildActions();
	BuildGoals();
	
	ResetExecutionState();
}

void UKMGoapAgentComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ClearSensors();
	ResetExecutionState();
	Super::EndPlay(EndPlayReason);
}


void UKMGoapAgentComponent::UpdateExecutionState()
{
	CurrentGoal = CurrentPlan.Goal;
			
	CurrentAction = CurrentPlan.Actions[0];
	CurrentPlan.Actions.RemoveAt(0);
}

void UKMGoapAgentComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	
	if (!CurrentAction)
	{
		CalculatePlan();

		if (CurrentPlan.IsValid())
		{
			UpdateExecutionState();
			if (ValidateActionPreconditions(CurrentAction))
			{
				CurrentAction->StartAction(this);
			}
			else
			{
				UE_LOG(LogGoapAgent, Verbose, TEXT("Preconditions not met. Clearing current action/goal."));
				ResetExecutionState();
			}
		}
	}
	
	if (CurrentAction)
	{
		const EKMGoapActionStatus Status = CurrentAction->TickAction(this, DeltaTime);
		if (Status == EKMGoapActionStatus::Succeeded || Status == EKMGoapActionStatus::Failed)
		{
			CurrentAction->StopAction(this);
			CurrentAction = nullptr;

			if (CurrentPlan.Actions.Num() == 0)
			{
				LastGoal = CurrentGoal;
				ResetExecutionState();
			}
		}
	}
}

void UKMGoapAgentComponent::HandleSensorTargetChanged(FGameplayTag SourceTag)
{
	UE_LOG(LogGoapAgent, Display, TEXT("Target changed, clearing current action and goal."));
	ResetExecutionState();
	OnSensorTargetChanged(SourceTag);
}

void UKMGoapAgentComponent::ResetExecutionState()
{
	CurrentAction = nullptr;
	CurrentGoal = nullptr;
	CurrentPlan.Reset();
}

void UKMGoapAgentComponent::CalculatePlan()
{
	float CurrentPriority = 0.f;
	if (CurrentGoal)
	{
		CurrentPriority = CurrentGoal->GetPriority(this);
	}

	TArray<UKMGoapAgentGoal*> GoalsToCheck;
	GoalsToCheck.Reserve(GoalsByTag.Num());

	for (auto& Pair : GoalsByTag)
	{
		UKMGoapAgentGoal* Goal = Pair.Value;
		if (!Goal) continue;

		const float GoalPriority = Goal->GetPriority(this);

		if (!CurrentGoal || GoalPriority > CurrentPriority)
		{
			GoalsToCheck.Add(Goal);
		}
	}

	FKMGoapActionPlan NewPlan;
	if (ComputePlanForGoals(GoalsToCheck, NewPlan) && NewPlan.IsValid())
	{
		CurrentPlan = MoveTemp(NewPlan);
	}
}

bool UKMGoapAgentComponent::ValidateActionPreconditions(const UKMGoapAgentAction* Action) const
{
	if (!Action)
	{
		return false;
	}

	const TSet<FKMGoapCondition>& Preconditions = Action->Preconditions;
	// We need to evaluate if the 
	bool bContainAllInRequiredState = true;
	for (const FKMGoapCondition& Precondition : Preconditions)
	{
		if (!Facts.Contains(Precondition.Tag))
		{
			bContainAllInRequiredState = false;
			break;
		}
		
		if (Precondition.bValue != Facts[Precondition.Tag])
		{
			bContainAllInRequiredState = false;
			break;
		}
	}
	
	if (bContainAllInRequiredState)
	{
		return true;
	}
	
	TSet<FKMGoapCondition> Missing = Preconditions;
	for (const TTuple<FGameplayTag, bool>& Fact : Facts)
	{
		if (FKMGoapCondition* Item = Missing.Find(FKMGoapCondition{Fact.Key, Fact.Value}))
		{
			if (Item->bValue != Fact.Value)
			{
				Missing.Remove(*Item);
			}
		}
	}
	
	for (const FKMGoapCondition& Condition : Missing)
	{
		if (EvaluateBeliefByTag(Condition.Tag) != Condition.bValue)
		{
			return false;
		}
	}

	return true;
}

bool UKMGoapAgentComponent::ComputePlanForGoals(const TArray<UKMGoapAgentGoal*>& GoalsToCheck, FKMGoapActionPlan& OutPlan)
{
	if (UWorld* World = GetWorld())
	{
		if (UGameInstance* GI = World->GetGameInstance())
		{
			if (UKMGoapPlannerSubsystem* Planner = GI->GetSubsystem<UKMGoapPlannerSubsystem>())
			{
				return IKMGoapPlannerInterface::Execute_Plan(Planner, this, GoalsToCheck, LastGoal, OutPlan);
			}
		}
	}
	return false;
}

void UKMGoapAgentComponent::BuildBeliefs()
{
	UE_LOG(LogGoapAgent, Display, TEXT("Building Beliefs Cache"));
	BeliefsByTag.Reset();
	if (!BeliefSet) return;
	BuildTaggedObjects<UKMGoapAgentBelief>(
		this,
		BeliefSet->Beliefs,
		BeliefsByTag,
		[](const UKMGoapAgentBelief* Belief) { return Belief->BeliefTag; }
	);
}

void UKMGoapAgentComponent::BuildActions()
{
	UE_LOG(LogGoapAgent, Display, TEXT("Building Actions Cache"));
	ActionsByTag.Reset();
	if (!ActionSet) return;
	BuildTaggedObjects<UKMGoapAgentAction>(
		this,
		ActionSet->Actions,
		ActionsByTag,
		[](const UKMGoapAgentAction* Action) { return Action->ActionTag; }
	);
}

void UKMGoapAgentComponent::BuildGoals()
{
	UE_LOG(LogGoapAgent, Display, TEXT("Building Goals Cache"));
	GoalsByTag.Reset();
	if (!GoalSet) return;
	BuildTaggedObjects<UKMGoapAgentGoal>(
		this,
		GoalSet->Goals,
		GoalsByTag,
		[](const UKMGoapAgentGoal* Action) { return Action->GoalTag; }
	);
}

void UKMGoapAgentComponent::CacheSensors()
{
	UE_LOG(LogGoapAgent, Display, TEXT("Building Sensors Cache"));
	AActor* Owner = GetOwner();
	if (!Owner) return;
	SensorsByTag.Reset();
	
	TArray<UActorComponent*> SensorComponents =	GetOwner()->GetComponentsByInterface(
		UKMGoapSensorInterface::StaticClass()
	);
	
	for (UActorComponent* Sensor : SensorComponents)
	{
		FGameplayTag Tag = IKMGoapSensorInterface::Execute_GetTag(Sensor);
		if (!Tag.IsValid())
		{
			UE_LOG(LogGoapAgent, Warning, TEXT("Found sensor component with invalid tag: %s"),
				*GetNameSafe(Sensor));
			continue;
		}
		
		if (SensorsByTag.Contains(Tag))
		{
			UE_LOG(LogGoapAgent, Warning, TEXT("Duplicate sensor tag [%s]. Ignoring %s"),
				*Tag.ToString(), *GetNameSafe(Sensor));
			continue;
		}
		
		BindSensorEvents(Sensor);
		SensorsByTag.Add(Tag, Sensor);
	}
}

void UKMGoapAgentComponent::ClearSensors()
{
	for (auto& Pair : SensorsByTag)
	{
		TObjectPtr<UActorComponent> Sensor = Pair.Value;
		UnbindSensorEvents(Sensor);
	}
	SensorsByTag.Reset();
}

void UKMGoapAgentComponent::BindSensorEvents(UActorComponent* Sensor)
{
	IKMGoapSensorInterface::Execute_RegisterTargetChangedListener(
			Sensor,
			this,
			GET_FUNCTION_NAME_CHECKED(UKMGoapAgentComponent, HandleSensorTargetChanged));
}

void UKMGoapAgentComponent::UnbindSensorEvents(UActorComponent* Sensor)
{
	IKMGoapSensorInterface::Execute_UnregisterTargetChangedListener(
			Sensor,
			this,
			GET_FUNCTION_NAME_CHECKED(UKMGoapAgentComponent, HandleSensorTargetChanged));
}

