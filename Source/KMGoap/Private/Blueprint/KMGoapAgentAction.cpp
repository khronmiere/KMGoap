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
	if (Status != EKMGoapActionStatus::Running || !CanPerform(Agent))
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
		ApplyFacts(Agent);
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

TSet<FKMGoapCondition> UKMGoapAgentAction::GetPostConditions() const
{
	TSet<FKMGoapCondition> PostConditions = Effects;
	PostConditions.Append(Facts);
	return PostConditions;
}

bool UKMGoapAgentAction::CanPerform_Implementation(UKMGoapAgentComponent* Agent) const
{
	return false;
}

EKMGoapActionStatus UKMGoapAgentAction::OnTick_Implementation(UKMGoapAgentComponent* Agent, float DeltaTime)
{
	return EKMGoapActionStatus::Succeeded;
}

void UKMGoapAgentAction::ApplyFacts(UKMGoapAgentComponent* Agent) const
{
	if (!Agent) return;
	for (const FKMGoapCondition& Fact : Facts)
	{
		Agent->SetFact(Fact.Tag, Fact.bValue);
	}
}
