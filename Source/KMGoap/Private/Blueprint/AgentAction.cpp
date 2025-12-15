// Fill out your copyright notice in the Description page of Project Settings.


#include "Blueprint/AgentAction.h"

#include "Blueprint/Component/AgentComponent.h"

void UAgentAction::StartAction(UAgentComponent* Agent)
{
	Status = EActionStatus::Running;
	OnStart(Agent);
}

EActionStatus UAgentAction::TickAction(UAgentComponent* Agent, float DeltaTime)
{
	if (Status != EActionStatus::Running || !CanPerform())
	{
		return Status;
	}
	
	const EActionStatus NewStatus = OnTick(Agent, DeltaTime);
	if (NewStatus == EActionStatus::Running || NewStatus == EActionStatus::NotStarted)
	{
		return Status;
	}

	Status = NewStatus;
	if (Status == EActionStatus::Succeeded)
	{
		ApplyEffects(Agent);
	}

	return Status;
}

void UAgentAction::StopAction(UAgentComponent* Agent)
{
	OnStop(Agent);
	if (Status == EActionStatus::Running)
	{
		Status = EActionStatus::Failed;
	}
}

bool UAgentAction::CanPerform_Implementation() const
{
	return true;
}

EActionStatus UAgentAction::OnTick_Implementation(UAgentComponent* Agent, float DeltaTime)
{
	return EActionStatus::Succeeded;
}

void UAgentAction::ApplyEffects(UAgentComponent* Agent) const
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
