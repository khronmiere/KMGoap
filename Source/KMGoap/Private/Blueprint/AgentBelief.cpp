// Fill out your copyright notice in the Description page of Project Settings.


#include "Blueprint/AgentBelief.h"

bool UAgentBelief::Native_Condition() const
{
	return Condition();
}

bool UAgentBelief::Condition_Implementation() const
{
	return false;
}

FVector UAgentBelief::Native_ObservedLocation() const
{
	return ObservedLocation();
}

FVector UAgentBelief::ObservedLocation_Implementation() const
{
	return FVector::ZeroVector;
}
