// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystem/Behavior/KMGoapPlanSearchBase.h"
#include "KMGoapPlanSearch_Dijkstra.generated.h"

struct FKMGoapPlanningContext;
struct FKMGoapCondition;
struct FKMGoapSimState;

/**
 * 
 */
UCLASS()
class KMGOAP_API UKMGoapPlanSearch_Dijkstra : public UKMGoapPlanSearchBase
{
	GENERATED_BODY()
	
public:
	virtual bool BuildPlan_Implementation(
		UKMGoapAgentComponent* Agent,
		const TArray<UKMGoapAgentGoal*>& GoalsToCheck,
		UKMGoapAgentGoal* MostRecentGoal,
		FKMGoapActionPlan& OutPlan) override;

private:
	bool BuildContext(
		UKMGoapAgentComponent* Agent,
		const TArray<UKMGoapAgentGoal*>& GoalsToCheck,
		UKMGoapAgentGoal* MostRecentGoal,
		TArray<UKMGoapAgentGoal*>& OutGoalsSorted,
		FKMGoapPlanningContext& OutCtx) const;
	
	bool SolveGoalDijkstra(
		const FKMGoapPlanningContext& Context,
		UKMGoapAgentGoal* Goal,
		FKMGoapActionPlan& OutPlan) const;
	
	static bool SatisfiesAll(const FKMGoapSimState& State, const TSet<FKMGoapCondition>& Conditions);
	static uint32 HashState(const FKMGoapSimState& State);
	void ApplyPostconditions(FKMGoapSimState& State, const TSet<FKMGoapCondition>& Post) const;
};
