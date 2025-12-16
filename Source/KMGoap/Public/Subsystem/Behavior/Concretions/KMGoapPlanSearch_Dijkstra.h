// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystem/Behavior/KMGoapPlanSearchBase.h"
#include "KMGoapPlanSearch_Dijkstra.generated.h"

struct FKMGoapCondition;

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
	// Helpers that mirror your current subsystem ones.
	bool IsConditionSatisfied(UKMGoapAgentComponent* Agent, const FKMGoapCondition& Condition) const;
	void RemoveSatisfied(UKMGoapAgentComponent* Agent, TSet<FKMGoapCondition>& InOutRequired) const;

	static uint32 HashRequired(const TSet<FKMGoapCondition>& Required);
};
