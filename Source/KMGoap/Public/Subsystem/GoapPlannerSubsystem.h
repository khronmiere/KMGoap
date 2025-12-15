#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "GameplayTagContainer.h"
#include "GoapPlannerSubsystem.generated.h"

class UAgentGoal;
class UAgentAction;
class UAgentComponent;

// Forward-declare your plan struct (defined in AgentComponent.h)
struct FActionPlan;

/**
 * UGoapPlannerSubsystem
 *
 * Stateless GOAP planner service.
 * Computes a plan for an agent given a set of candidate goals.
 */
UCLASS(Category="KMGoap")
class KMGOAP_API UGoapPlannerSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category="KMGoap")
	bool Plan(
		UAgentComponent* Agent,
		const TArray<UAgentGoal*>& GoalsToCheck,
		UAgentGoal* MostRecentGoal,
		FActionPlan& OutPlan);

private:
	struct FGoapNode
	{
		TSharedPtr<FGoapNode> Parent;
		TObjectPtr<UAgentAction> Action = nullptr;
		FGameplayTagContainer Required;      // tags that must become true
		float Cost = 0.f;
		TArray<TSharedPtr<FGoapNode>> Leaves;

		bool IsLeafDead() const { return Leaves.Num() == 0 && Action == nullptr; }
	};

	// Core recursive search (Unity FindPath port, but without mutating parent state)
	bool FindPath(
		UAgentComponent* Agent,
		const TSharedPtr<FGoapNode>& Parent,
		const TArray<UAgentAction*>& AvailableActions);

	// Helpers
	bool IsTagSatisfied(UAgentComponent* Agent, const FGameplayTag& Tag) const;
	void RemoveSatisfied(UAgentComponent* Agent, FGameplayTagContainer& InOutRequired) const;

	// Ordering
	static void SortGoals(UAgentComponent* Agent, TArray<UAgentGoal*>& InOutGoals, UAgentGoal* MostRecentGoal);
	static void SortActionsByCost(TArray<UAgentAction*>& InOutActions);
};
