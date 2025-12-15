// Fill out your copyright notice in the Description page of Project Settings.


#include "Blueprint/KMGoapAgentBelief.h"

bool UKMGoapAgentBelief::Native_Condition() const
{
	return Condition();
}

bool UKMGoapAgentBelief::Condition_Implementation() const
{
	return false;
}

FVector UKMGoapAgentBelief::Native_ObservedLocation() const
{
	return ObservedLocation();
}

FVector UKMGoapAgentBelief::ObservedLocation_Implementation() const
{
	return FVector::ZeroVector;
}
