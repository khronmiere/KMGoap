// All rights reserved by Khrönmière Entertainment.
#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "UObject/Object.h"
#include "KMGoapCondition.generated.h"

/**
 * Represents a single boolean GOAP condition identified by a gameplay tag.
 *
 * Conditions are used for action preconditions, action effects, desired goal
 * states, facts, and belief comparisons.
 */
USTRUCT(BlueprintType)
struct KMGOAP_API FKMGoapCondition
{
	GENERATED_BODY()
	
	/**
	 * Gameplay tag identifying the fact or belief represented by this condition.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGameplayTag Tag;

	/**
	 * Expected boolean value for the tagged condition.
	 *
	 * True represents a positive condition, while false represents a negative
	 * condition.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bValue = true;
	
	/**
	 * Compares this condition against another condition.
	 *
	 * @param Other Condition to compare with this condition.
	 * @return True when both the tag and expected value are identical.
	 */
	bool operator==(const FKMGoapCondition& Other) const
	{
		return Tag == Other.Tag && bValue == Other.bValue;
	}
};

/**
 * Calculates the hash value for a GOAP condition.
 *
 * Enables FKMGoapCondition to be used in hash-based Unreal containers such as
 * TSet and TMap.
 *
 * @param Condition Condition to hash.
 * @return Combined hash of the condition tag and expected value.
 */
FORCEINLINE uint32 GetTypeHash(const FKMGoapCondition& Condition)
{
	return HashCombine(GetTypeHash(Condition.Tag), GetTypeHash(Condition.bValue));
}

/**
 * Tri-state result of evaluating a GOAP belief or fact.
 */
UENUM(BlueprintType)
enum class EKMGoapBeliefState : uint8
{
	/**
	 * The belief or fact is known and evaluates to true.
	 */
	Positive,

	/**
	 * The belief or fact is known and evaluates to false.
	 */
	Negative,

	/**
	 * The belief or fact is not known or could not be evaluated.
	 */
	Unknown
};
