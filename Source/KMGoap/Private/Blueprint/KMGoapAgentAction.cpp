// All rights reserved by Khrönmière Entertainment.
#include "Blueprint/KMGoapAgentAction.h"

#include "Blueprint/Component/KMGoapAgentComponent.h"

void UKMGoapAgentAction::StartAction(UKMGoapAgentComponent* Agent)
{
	// Runtime execution begins here rather than inside OnStart so native code owns the action lifecycle,
	// while Blueprint subclasses only need to implement their startup behavior.
	Status = EKMGoapActionStatus::Running;
	OnStart(Agent);
}

EKMGoapActionStatus UKMGoapAgentAction::TickAction(UKMGoapAgentComponent* Agent, float DeltaTime)
{
	// Keep completed, failed, or otherwise unavailable actions stable until the state machine releases them.
	if (Status != EKMGoapActionStatus::Running || !CanPerform(Agent))
	{
		return Status;
	}
	
	const EKMGoapActionStatus NewStatus = OnTick(Agent, DeltaTime);
	if (NewStatus == EKMGoapActionStatus::Running || NewStatus == EKMGoapActionStatus::NotStarted)
	{
		return Status;
	}

	// Only terminal results are committed. Successful actions materialize their promised facts into the
	// agent's world state, allowing downstream plan steps and replanning to observe the result.
	Status = NewStatus;
	if (Status == EKMGoapActionStatus::Succeeded)
	{
		ApplyFacts(Agent);
	}

	return Status;
}

void UKMGoapAgentAction::StopAction(UKMGoapAgentComponent* Agent)
{
	// A stopped action that has not already succeeded is treated as failed so interruption cannot be
	// mistaken for completion by the planner or external observers.
	if (Status != EKMGoapActionStatus::Succeeded)
	{
		Status = EKMGoapActionStatus::Failed;
	}
	
	OnStop(Agent);
}

void UKMGoapAgentAction::Release(UKMGoapAgentComponent* Agent)
{
	// Release returns the object to its reusable state. Action instances may be retained by the agent
	// and selected again by a later plan.
	Status = EKMGoapActionStatus::NotStarted;
	OnRelease(Agent);
}

TSet<FKMGoapCondition> UKMGoapAgentAction::GetPostConditions() const
{
	// Planning considers both declarative effects and facts that will be written on success. This allows
	// actions to contribute to plan satisfaction even when their runtime fact writes are separate from cost/effect data.
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

	// Facts are authoritative state changes caused by successful execution, not speculative planner effects.
	for (const FKMGoapCondition& Fact : Facts)
	{
		Agent->SetFact(Fact.Tag, Fact.bValue);
	}
}
