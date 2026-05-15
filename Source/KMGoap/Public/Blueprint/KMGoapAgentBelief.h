// All rights reserved by Khrönmière Entertainment.
#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "UObject/Object.h"
#include "KMGoapAgentBelief.generated.h"

class UKMGoapAgentComponent;

/**
 * Base class for a GOAP belief evaluated by an agent.
 *
 * A belief represents a queryable fact about the world from the agent's point of
 * view. Gameplay code should use Evaluate() and GetLocation(); Condition() and
 * ObservedLocation() are override points for Blueprint or C++ implementations.
 */
UCLASS(Abstract, Category="KMGoap", Blueprintable)
class KMGOAP_API UKMGoapAgentBelief : public UObject
{
	GENERATED_BODY()

public:
	/**
	 * Gameplay tag that uniquely identifies this belief in an agent's belief registry.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Belief")
	FGameplayTag BeliefTag;
	
	/**
	 * Gets the world-space location associated with this belief.
	 *
	 * @param Agent Agent component evaluating the belief location.
	 * @return Observed world location for this belief.
	 */
	UFUNCTION(BlueprintCallable, Category="Belief")
	FVector GetLocation(const UKMGoapAgentComponent* Agent) const { return Native_ObservedLocation(Agent); }
	
	/**
	 * Evaluates whether this belief is currently true for the supplied agent.
	 *
	 * @param Agent Agent component evaluating the belief.
	 * @return True when the belief condition is satisfied, otherwise false.
	 */
	UFUNCTION(BlueprintCallable, Category="Belief")
	bool Evaluate(const UKMGoapAgentComponent* Agent) const { return Native_Condition(Agent); }
	
protected:
	/**
	 * Native C++ condition evaluation entry point.
	 *
	 * @param Agent Agent component evaluating the belief.
	 * @return True when the belief condition is satisfied, otherwise false.
	 */
	virtual bool Native_Condition(const UKMGoapAgentComponent* Agent) const;

	/**
	 * Blueprint/C++ override point for belief condition evaluation.
	 *
	 * @param Agent Agent component evaluating the belief.
	 * @return True when the belief condition is satisfied, otherwise false.
	 */
	UFUNCTION(BlueprintNativeEvent, Category="Belief")
	bool Condition(const UKMGoapAgentComponent* Agent) const;

	/**
	 * Default native implementation for Condition().
	 *
	 * @param Agent Agent component evaluating the belief.
	 * @return True when the belief condition is satisfied, otherwise false.
	 */
	virtual bool Condition_Implementation(const UKMGoapAgentComponent* Agent) const;
	
	/**
	 * Native C++ observed-location entry point.
	 *
	 * @param Agent Agent component requesting the observed location.
	 * @return Observed world location for this belief.
	 */
	virtual FVector Native_ObservedLocation(const UKMGoapAgentComponent* Agent) const;

	/**
	 * Blueprint/C++ override point for returning the belief's observed location.
	 *
	 * @param Agent Agent component requesting the observed location.
	 * @return Observed world location for this belief.
	 */
	UFUNCTION(BlueprintNativeEvent, Category="Belief")
	FVector ObservedLocation(const UKMGoapAgentComponent* Agent) const;

	/**
	 * Default native implementation for ObservedLocation().
	 *
	 * @param Agent Agent component requesting the observed location.
	 * @return Observed world location for this belief.
	 */
	virtual FVector ObservedLocation_Implementation(const UKMGoapAgentComponent* Agent) const;
};
