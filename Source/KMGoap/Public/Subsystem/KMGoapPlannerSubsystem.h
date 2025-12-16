#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "GameplayTagContainer.h"
#include "Interface/KMGoapPlannerInterface.h"
#include "KMGoapPlannerSubsystem.generated.h"

struct FKMGoapCondition;
class UKMGoapAgentGoal;
class UKMGoapAgentAction;
class UKMGoapAgentComponent;

/**
 * UGoapPlannerSubsystem
 *
 * Stateless GOAP planner service.
 * Computes a plan for an agent given a set of candidate goals.
 */
UCLASS(Category="KMGoap")
class KMGOAP_API UKMGoapPlannerSubsystem : public UGameInstanceSubsystem, public IKMGoapPlannerInterface
{
	GENERATED_BODY()

public:
	virtual bool Plan_Implementation(
		UKMGoapAgentComponent* Agent,
		const TArray<UKMGoapAgentGoal*>& GoalsToCheck,
		UKMGoapAgentGoal* MostRecentGoal,
		FKMGoapActionPlan& OutPlan) override;

private:
	struct FGoapNode
	{
		TSharedPtr<FGoapNode> Parent;
		TObjectPtr<UKMGoapAgentAction> Action = nullptr;
		TSet<FKMGoapCondition> Required;      // tags that must become true
		float Cost = 0.f;
		TArray<TSharedPtr<FGoapNode>> Leaves;

		bool IsLeafDead() const { return Leaves.Num() == 0 && Action == nullptr; }
	};

	// Core recursive search (Unity FindPath port, but without mutating parent state)
	bool FindPath(
		UKMGoapAgentComponent* Agent,
		const TSharedPtr<FGoapNode>& Parent,
		const TArray<UKMGoapAgentAction*>& AvailableActions);

	// Helpers
	bool IsConditionSatisfied(UKMGoapAgentComponent* Agent, const FKMGoapCondition& Condition) const;
	void RemoveSatisfied(UKMGoapAgentComponent* Agent, TSet<FKMGoapCondition>& InOutRequired) const;

	// Ordering
	static void SortGoals(UKMGoapAgentComponent* Agent, TArray<UKMGoapAgentGoal*>& InOutGoals, UKMGoapAgentGoal* MostRecentGoal);
	static void SortActionsByCost(const UKMGoapAgentComponent* Agent, TArray<UKMGoapAgentAction*>& InOutActions);
};
