// All rights reserved by Khrönmière Entertainment.
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "KMGoapBeliefSet.generated.h"

/**
 * Data asset containing the GOAP belief classes available to an agent.
 *
 * Belief sets define the world-state queries that an agent can evaluate and
 * use as action preconditions, action effects, goal requirements, and knowledge
 * deactivation rules.
 */
UCLASS(Category = "KMGoap|Data", BlueprintType, Blueprintable)
class KMGOAP_API UKMGoapBeliefSet : public UDataAsset
{
	GENERATED_BODY()

public:
	/**
	 * Soft class references for all GOAP beliefs included in this set.
	 *
	 * Each class should derive from UKMGoapAgentBelief and is instantiated by
	 * the agent when building its belief cache.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TArray<TSoftClassPtr<class UKMGoapAgentBelief>> Beliefs;
};

