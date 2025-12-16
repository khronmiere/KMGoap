// Fill out your copyright notice in the Description page of Project Settings.


#include "Subsystem/Behavior/Concretions/KMGoapPlanSearch_Dijkstra.h"

bool UKMGoapPlanSearch_Dijkstra::BuildPlan_Implementation(UKMGoapAgentComponent* Agent,
	const TArray<UKMGoapAgentGoal*>& GoalsToCheck, UKMGoapAgentGoal* MostRecentGoal, FKMGoapActionPlan& OutPlan)
{
	return Super::BuildPlan_Implementation(Agent, GoalsToCheck, MostRecentGoal, OutPlan);
}

bool UKMGoapPlanSearch_Dijkstra::IsConditionSatisfied(UKMGoapAgentComponent* Agent,
                                                      const FKMGoapCondition& Condition) const
{
	return false;
}

void UKMGoapPlanSearch_Dijkstra::RemoveSatisfied(UKMGoapAgentComponent* Agent,
	TSet<FKMGoapCondition>& InOutRequired) const
{
}

uint32 UKMGoapPlanSearch_Dijkstra::HashRequired(const TSet<FKMGoapCondition>& Required)
{
	return 0;
}
