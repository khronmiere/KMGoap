#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "GameplayTagContainer.h"
#include "KMGoapPlannerSubsystem.generated.h"

class UKMGoapAgentGoal;
class UKMGoapAgentAction;
class UKMGoapAgentComponent;

// Forward-declare your plan struct (defined in AgentComponent.h)
struct FKMGoapActionPlan;

/**
 * UGoapPlannerSubsystem
 *
 * Stateless GOAP planner service.
 * Computes a plan for an agent given a set of candidate goals.
 */
UCLASS(Category="KMGoap")
class KMGOAP_API UKMGoapPlannerSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category="KMGoap")
	bool Plan(
		UKMGoapAgentComponent* Agent,
		const TArray<UKMGoapAgentGoal*>& GoalsToCheck,
		UKMGoapAgentGoal* MostRecentGoal,
		FKMGoapActionPlan& OutPlan);

private:
	struct FGoapNode
	{
		TSharedPtr<FGoapNode> Parent;
		TObjectPtr<UKMGoapAgentAction> Action = nullptr;
		FGameplayTagContainer Required;      // tags that must become true
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
	bool IsTagSatisfied(UKMGoapAgentComponent* Agent, const FGameplayTag& Tag) const;
	void RemoveSatisfied(UKMGoapAgentComponent* Agent, FGameplayTagContainer& InOutRequired) const;

	// Ordering
	static void SortGoals(UKMGoapAgentComponent* Agent, TArray<UKMGoapAgentGoal*>& InOutGoals, UKMGoapAgentGoal* MostRecentGoal);
	static void SortActionsByCost(TArray<UKMGoapAgentAction*>& InOutActions);
};
