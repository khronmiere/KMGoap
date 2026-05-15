// All rights reserved by Khrönmière Entertainment.
#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "KMGoapActionPlan.generated.h"

class UKMGoapAgentGoal;
class UKMGoapAgentAction;

/**
 * Runtime GOAP plan produced by the planner for a selected goal.
 *
 * A plan contains the goal being pursued, an ordered list of actions that should
 * be executed to satisfy that goal, and the total calculated cost of the plan.
 */
USTRUCT(BlueprintType)
struct FKMGoapActionPlan
{
	GENERATED_BODY()

	/**
	 * Goal that this plan is intended to satisfy.
	 */
	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<UKMGoapAgentGoal> Goal = nullptr;

	/**
	 * Ordered list of actions to execute for this plan.
	 *
	 * The state machine consumes actions from the beginning of this array.
	 */
	UPROPERTY(BlueprintReadOnly)
	TArray<TObjectPtr<UKMGoapAgentAction>> Actions;
	
	/**
	 * Total cost calculated for all actions in this plan.
	 *
	 * Lower-cost plans are generally preferred by search algorithms when
	 * multiple plans can satisfy the same goal.
	 */
	UPROPERTY(BlueprintReadOnly)
	float TotalCost = 0.f;
	
	/**
	 * Checks whether this plan contains enough data to be executed.
	 *
	 * @return True when the plan has a goal and at least one action.
	 */
	bool IsValid() const { return Goal != nullptr && Actions.Num() > 0; }

	/**
	 * Clears this plan and restores it to an empty state.
	 */
	void Reset() { Goal = nullptr; Actions.Reset(); TotalCost = 0.f; }
};
