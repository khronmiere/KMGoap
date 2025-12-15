// Fill out your copyright notice in the Description page of Project Settings.


#include "Blueprint/AgentGoal.h"

float UAgentGoal::Native_GetPriority(UAgentComponent* Agent) const
{
	return Priority(Agent);
}

float UAgentGoal::Priority_Implementation(UAgentComponent* Agent) const
{
	return BasePriority;
}
