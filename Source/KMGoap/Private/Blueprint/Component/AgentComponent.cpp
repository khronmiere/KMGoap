// Fill out your copyright notice in the Description page of Project Settings.


#include "Blueprint/Component/AgentComponent.h"

#include "GameplayTagContainer.h"
#include "Blueprint/AgentAction.h"
#include "Blueprint/AgentBelief.h"
#include "Blueprint/AgentGoal.h"
#include "Data/ActionSet.h"
#include "Data/BeliefSet.h"
#include "Data/GoalSet.h"
#include "Interface/SensorInterface.h"
#include "Subsystem/GoapPlannerSubsystem.h"

DEFINE_LOG_CATEGORY_STATIC(LogGoapAgent, Log, All);

// Sets default values for this component's properties
UAgentComponent::UAgentComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}

UAgentBelief* UAgentComponent::GetBeliefByTag(FGameplayTag Tag) const
{
	if (BeliefsByTag.Contains(Tag))
	{
		return BeliefsByTag[Tag];
	}
	return nullptr;
}

UActorComponent* UAgentComponent::GetSensorByTag(FGameplayTag Tag) const
{
	if (SensorsByTag.Contains(Tag))
	{
		return SensorsByTag[Tag];
	}
	return nullptr;
}

UAgentGoal* UAgentComponent::GetGoalByTag(FGameplayTag Tag) const
{
	if (GoalsByTag.Contains(Tag))
	{
		return GoalsByTag[Tag];
	}
	return nullptr;
}

UAgentAction* UAgentComponent::GetActionByTag(FGameplayTag Tag) const
{
	if (ActionsByTag.Contains(Tag))
	{
		return ActionsByTag[Tag];
	}
	return nullptr;
}

bool UAgentComponent::EvaluateBeliefByTag(FGameplayTag Tag) const
{
	if (UAgentBelief* Belief = GetBeliefByTag(Tag))
	{
		return Belief->Evaluate();
	}
	return false;
}

FVector UAgentComponent::GetBeliefLocationByTag(FGameplayTag Tag) const
{
	if (UAgentBelief* Belief = GetBeliefByTag(Tag))
	{
		return Belief->GetLocation();
	}
	return FVector::ZeroVector;
}

void UAgentComponent::ApplyEffectByTag(FGameplayTag Tag, bool bValue)
{
	if (!Tag.IsValid()) return;
	if (bValue)
	{
		Facts.AddTag(Tag);
		return;
	}
	Facts.RemoveTag(Tag);
}

bool UAgentComponent::GetFact(FGameplayTag Tag) const
{
	return Facts.HasTagExact(Tag);
}

void UAgentComponent::BeginPlay()
{
	Super::BeginPlay();

	CacheSensors();
	BuildBeliefs();
	BuildActions();
	BuildGoals();
	
	ResetExecutionState();
}

void UAgentComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ClearSensors();
	ResetExecutionState();
	Super::EndPlay(EndPlayReason);
}


void UAgentComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	
	if (!CurrentAction)
	{
		CalculatePlan();

		if (CurrentPlan.IsValid())
		{
			CurrentGoal = CurrentPlan.Goal;
			
			CurrentAction = CurrentPlan.Actions[0];
			CurrentPlan.Actions.RemoveAt(0);

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
		const EActionStatus Status = CurrentAction->TickAction(this, DeltaTime);

		if (Status == EActionStatus::Succeeded || Status == EActionStatus::Failed)
		{
			CurrentAction->StopAction(this);
			CurrentAction = nullptr;

			if (CurrentPlan.Actions.Num() == 0)
			{
				LastGoal = CurrentGoal;
				CurrentGoal = nullptr;
				CurrentPlan.Reset();
			}
		}
	}
}

void UAgentComponent::HandleSensorTargetChanged(FGameplayTag SourceTag)
{
	UE_LOG(LogGoapAgent, Display, TEXT("Target changed, clearing current action and goal."));
	ResetExecutionState();
	OnSensorTargetChanged(SourceTag);
}

void UAgentComponent::ResetExecutionState()
{
	CurrentAction = nullptr;
	CurrentGoal = nullptr;
	CurrentPlan.Reset();
}

void UAgentComponent::CalculatePlan()
{
	float CurrentPriority = 0.f;
	if (CurrentGoal)
	{
		CurrentPriority = CurrentGoal->GetPriority(this);
	}

	TArray<UAgentGoal*> GoalsToCheck;
	GoalsToCheck.Reserve(GoalsByTag.Num());

	for (auto& Pair : GoalsByTag)
	{
		UAgentGoal* Goal = Pair.Value;
		if (!Goal) continue;

		const float GoalPriority = Goal->GetPriority(this);

		if (!CurrentGoal || GoalPriority > CurrentPriority)
		{
			GoalsToCheck.Add(Goal);
		}
	}

	FActionPlan NewPlan;
	if (ComputePlanForGoals(GoalsToCheck, NewPlan) && NewPlan.IsValid())
	{
		CurrentPlan = MoveTemp(NewPlan);
	}
}

bool UAgentComponent::ValidateActionPreconditions(const UAgentAction* Action) const
{
	if (!Action)
	{
		return false;
	}

	const FGameplayTagContainer& Preconditions = Action->Preconditions;
	
	if (Facts.HasAllExact(Preconditions))
	{
		return true;
	}
	
	FGameplayTagContainer Missing = Preconditions;
	Missing.RemoveTags(Facts);
	for (const FGameplayTag& Tag : Missing)
	{
		if (!EvaluateBeliefByTag(Tag))
		{
			return false;
		}
	}

	return true;
}

bool UAgentComponent::ComputePlanForGoals(const TArray<UAgentGoal*>& GoalsToCheck, FActionPlan& OutPlan)
{
	if (UWorld* World = GetWorld())
	{
		if (UGameInstance* GI = World->GetGameInstance())
		{
			if (UGoapPlannerSubsystem* Planner = GI->GetSubsystem<UGoapPlannerSubsystem>())
			{
				return Planner->Plan(this, GoalsToCheck, LastGoal, OutPlan);
			}
		}
	}
	return false;
}

void UAgentComponent::BuildBeliefs()
{
	UE_LOG(LogGoapAgent, Display, TEXT("Building Beliefs Cache"));
	BeliefsByTag.Reset();
	if (!BeliefSet) return;
	BuildTaggedObjects<UAgentBelief>(
		this,
		BeliefSet->Beliefs,
		BeliefsByTag,
		[](const UAgentBelief* Belief) { return Belief->BeliefTag; }
	);
}

void UAgentComponent::BuildActions()
{
	UE_LOG(LogGoapAgent, Display, TEXT("Building Actions Cache"));
	ActionsByTag.Reset();
	if (!ActionSet) return;
	BuildTaggedObjects<UAgentAction>(
		this,
		ActionSet->Actions,
		ActionsByTag,
		[](const UAgentAction* Action) { return Action->ActionTag; }
	);
}

void UAgentComponent::BuildGoals()
{
	UE_LOG(LogGoapAgent, Display, TEXT("Building Goals Cache"));
	GoalsByTag.Reset();
	if (!GoalSet) return;
	BuildTaggedObjects<UAgentGoal>(
		this,
		GoalSet->Goals,
		GoalsByTag,
		[](const UAgentGoal* Action) { return Action->GoalTag; }
	);
}

void UAgentComponent::CacheSensors()
{
	UE_LOG(LogGoapAgent, Display, TEXT("Building Sensors Cache"));
	AActor* Owner = GetOwner();
	if (!Owner) return;
	SensorsByTag.Reset();
	
	TArray<UActorComponent*> SensorComponents =	GetOwner()->GetComponentsByInterface(
		USensorInterface::StaticClass()
	);
	
	for (UActorComponent* Sensor : SensorComponents)
	{
		FGameplayTag Tag = ISensorInterface::Execute_GetTag(Sensor);
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

void UAgentComponent::ClearSensors()
{
	for (auto& Pair : SensorsByTag)
	{
		TObjectPtr<UActorComponent> Sensor = Pair.Value;
		UnbindSensorEvents(Sensor);
	}
	SensorsByTag.Reset();
}

void UAgentComponent::BindSensorEvents(UActorComponent* Sensor)
{
	ISensorInterface::Execute_RegisterTargetChangedListener(
			Sensor,
			this,
			GET_FUNCTION_NAME_CHECKED(UAgentComponent, HandleSensorTargetChanged));
}

void UAgentComponent::UnbindSensorEvents(UActorComponent* Sensor)
{
	ISensorInterface::Execute_UnregisterTargetChangedListener(
			Sensor,
			this,
			GET_FUNCTION_NAME_CHECKED(UAgentComponent, HandleSensorTargetChanged));
}

