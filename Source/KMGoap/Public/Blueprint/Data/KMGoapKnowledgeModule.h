// All rights reserved by Khrönmière Entertainment.
#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Engine/DataAsset.h"
#include "KMGoapKnowledgeModule.generated.h"

class UKMGoapGoalSet;
class UKMGoapActionSet;
class UKMGoapBeliefSet;

/**
 * Data asset that defines a modular package of GOAP knowledge.
 *
 * A knowledge module groups belief, action, and goal sets under a single gameplay tag. Modules can be added
 * to an agent at runtime and optionally removed when their deactivation rules are satisfied.
 */
UCLASS(Category = "KMGoap|Data", BlueprintType, Blueprintable)
class KMGOAP_API UKMGoapKnowledgeModule : public UDataAsset
{
	GENERATED_BODY()

public:
	/**
	 * Unique gameplay tag identifying this knowledge module.
	 *
	 * The runtime uses this tag as the key for detecting duplicate modules and for removing active modules.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FGameplayTag KnowledgeTag;

	/**
	 * Set of belief classes that this module contributes to an agent.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UKMGoapBeliefSet> BeliefSet;

	/**
	 * Set of action classes that this module contributes to an agent.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UKMGoapActionSet> ActionSet;

	/**
	 * Set of goal classes that this module contributes to an agent.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UKMGoapGoalSet> GoalSet;

	/**
	 * Belief-state conditions that determine when this module should deactivate.
	 *
	 * Each entry maps a belief tag to the expected boolean state. The module deactivates only when all rules
	 * are satisfied. An empty map means the module never deactivates automatically after being added.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly,
		meta=(Tooltip="Empty map means this Module will never deactivate once added"))
	TMap<FGameplayTag, bool> DeactivationRules;
};
