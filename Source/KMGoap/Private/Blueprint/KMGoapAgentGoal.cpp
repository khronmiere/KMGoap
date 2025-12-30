// Fill out your copyright notice in the Description page of Project Settings.


#include "Blueprint/KMGoapAgentGoal.h"

float UKMGoapAgentGoal::Native_GetPriority(UKMGoapAgentComponent* Agent) const
{
	return Priority(Agent);
}

float UKMGoapAgentGoal::Priority_Implementation(UKMGoapAgentComponent* Agent) const
{
	return BasePriority;
}
