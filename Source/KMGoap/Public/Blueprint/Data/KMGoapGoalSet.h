// All rights reserved by Khrönmière Entertainment.
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "KMGoapGoalSet.generated.h"

/**
 * Data asset containing the GOAP goal classes available to an agent.
 *
 * Goal sets define the objectives that an agent may attempt to satisfy.
 * Runtime planning evaluates these goals by priority and selects an action plan
 * capable of reaching the desired goal state.
 */
UCLASS(Category = "KMGoap|Data", BlueprintType, Blueprintable)
class KMGOAP_API UKMGoapGoalSet : public UDataAsset
{
	GENERATED_BODY()
	
public:
	/**
	 * Soft class references for all GOAP goals included in this set.
	 *
	 * Each class should derive from UKMGoapAgentGoal and provides priority and
	 * desired-state data used by the planner.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TArray<TSoftClassPtr<class UKMGoapAgentGoal>> Goals;
};
