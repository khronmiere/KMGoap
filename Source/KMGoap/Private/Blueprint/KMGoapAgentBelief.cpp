// Fill out your copyright notice in the Description page of Project Settings.


#include "Blueprint/KMGoapAgentBelief.h"

bool UKMGoapAgentBelief::Native_Condition(const UKMGoapAgentComponent* Agent) const
{
	return Condition(Agent);
}

bool UKMGoapAgentBelief::Condition_Implementation(const UKMGoapAgentComponent* Agent) const
{
	return false;
}

FVector UKMGoapAgentBelief::Native_ObservedLocation(const UKMGoapAgentComponent* Agent) const
{
	return ObservedLocation(Agent);
}

FVector UKMGoapAgentBelief::ObservedLocation_Implementation(const UKMGoapAgentComponent* Agent) const
{
	return FVector::ZeroVector;
}
