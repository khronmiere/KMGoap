// Fill out your copyright notice in the Description page of Project Settings.


#include "Blueprint/KMGoapAgentAction.h"

#include "Blueprint/Component/KMGoapAgentComponent.h"

void UKMGoapAgentAction::StartAction(UKMGoapAgentComponent* Agent)
{
	Status = EKMGoapActionStatus::Running;
	OnStart(Agent);
}

EKMGoapActionStatus UKMGoapAgentAction::TickAction(UKMGoapAgentComponent* Agent, float DeltaTime)
{
	if (Status != EKMGoapActionStatus::Running || !CanPerform())
	{
		return Status;
	}
	
	const EKMGoapActionStatus NewStatus = OnTick(Agent, DeltaTime);
	if (NewStatus == EKMGoapActionStatus::Running || NewStatus == EKMGoapActionStatus::NotStarted)
	{
		return Status;
	}

	Status = NewStatus;
	if (Status == EKMGoapActionStatus::Succeeded)
	{
		ApplyEffects(Agent);
	}

	return Status;
}

void UKMGoapAgentAction::StopAction(UKMGoapAgentComponent* Agent)
{
	OnStop(Agent);
	if (Status == EKMGoapActionStatus::Running)
	{
		Status = EKMGoapActionStatus::Failed;
	}
}

bool UKMGoapAgentAction::CanPerform_Implementation() const
{
	return true;
}

EKMGoapActionStatus UKMGoapAgentAction::OnTick_Implementation(UKMGoapAgentComponent* Agent, float DeltaTime)
{
	return EKMGoapActionStatus::Succeeded;
}

void UKMGoapAgentAction::ApplyEffects(UKMGoapAgentComponent* Agent) const
{
	if (!Agent) return;

	// For each effect tag, tell the agent to "apply" it.
	// You need to define what "apply" means in your system.
	// Typical: mark a world state key, or force a belief to become true, etc.
	for (const FGameplayTag& Tag : Effects)
	{
		Agent->ApplyEffectByTag(Tag);
	}
}
