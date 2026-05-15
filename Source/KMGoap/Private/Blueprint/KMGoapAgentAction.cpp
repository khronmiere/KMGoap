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
	if (Status != EKMGoapActionStatus::Running)
	{
		return Status;
	}
	
	// If the action cannot be performed, fail immediately
	if (!CanPerform(Agent))
	{
		Status = EKMGoapActionStatus::Failed;
		return Status;
	}
	
	const EKMGoapActionStatus NewStatus = OnTick(Agent, DeltaTime);
	if (NewStatus == EKMGoapActionStatus::Running)
	{
		return Status;
	}
	
	// Guarantee we are not trying to set an invalid status
	check(NewStatus != EKMGoapActionStatus::NotStarted,
		TEXT("Only Release process can set an Action status to NotStarted"));

	// Only terminal results are committed. Successful actions materialize their runtime facts into the
	// agent's local state, allowing downstream plan steps and replanning to observe the result.
	Status = NewStatus;
	if (Status == EKMGoapActionStatus::Succeeded)
	{
		ApplyRuntimeFacts(Agent);
	}

	return Status;
}

void UKMGoapAgentAction::StopAction(UKMGoapAgentComponent* Agent)
{
	// No need to stop if action is not started
	if (Status == EKMGoapActionStatus::NotStarted)
	{
		return;
	}
	
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
	// Planning considers both planner-only predicted effects and runtime fact writes.
	// Runtime execution only applies RuntimeFacts; PredictedEffects must become true through gameplay,
	// sensors, or beliefs.
	TSet<FKMGoapCondition> PostConditions = PredictedEffects;
	PostConditions.Append(RuntimeFacts);
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

void UKMGoapAgentAction::ApplyRuntimeFacts(UKMGoapAgentComponent* Agent) const
{
	if (!Agent) return;

	// RuntimeFacts are authoritative agent-local state changes caused by successful execution.
	// PredictedEffects are not written here because they represent planner-only expected outcomes.
	for (const FKMGoapCondition& RuntimeFact : RuntimeFacts)
	{
		Agent->SetFact(RuntimeFact.Tag, RuntimeFact.bValue);
	}
}
