// All rights reserved by Khrönmière Entertainment.
#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "UObject/Object.h"
#include "KMGoapAgentGoal.generated.h"

struct FKMGoapCondition;
class UKMGoapAgentComponent;

/**
 * Represents a GOAP goal definition that an agent can evaluate and plan toward.
 *
 * Goals describe desired world or agent states through their desired effects and expose
 * a priority calculation used by the GOAP planner/state machine to choose which goal
 * should be pursued at runtime.
 */
UCLASS(Abstract, BlueprintType, Blueprintable, EditInlineNew, DefaultToInstanced, Category="KMGoap")
class KMGOAP_API UKMGoapAgentGoal : public UObject
{
	GENERATED_BODY()
	
public:
	/**
	 * Gameplay tag that uniquely identifies this goal within an agent's goal collection.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Goal")
	FGameplayTag GoalTag;
	
	/**
	 * Set of conditions that should become true when this goal is satisfied.
	 *
	 * The planner uses these desired effects as the target state when searching for
	 * a valid sequence of actions.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Goal")
	TSet<FKMGoapCondition> DesiredEffects;
	
	/**
	 * Returns the runtime priority of this goal for the specified agent.
	 *
	 * @param Agent Agent component evaluating this goal.
	 * @return Priority value used to compare this goal against other available goals.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Goal")
	float GetPriority(const UKMGoapAgentComponent* Agent) const { return Native_GetPriority(Agent); }

protected:
	/**
	 * Default priority value used by the native priority implementation.
	 *
	 * Higher values make the goal more likely to be selected over competing goals.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Goal", meta=(ClampMin="0.0"))
	float BasePriority = 1.f;
	
	/**
	 * Native priority calculation entry point.
	 *
	 * Override this in C++ subclasses to provide custom goal priority logic.
	 *
	 * @param Agent Agent component evaluating this goal.
	 * @return Runtime priority for this goal.
	 */
	virtual float Native_GetPriority(const UKMGoapAgentComponent* Agent) const;

	/**
	 * Blueprint-overridable priority calculation.
	 *
	 * @param Agent Agent component evaluating this goal.
	 * @return Runtime priority for this goal.
	 */
	UFUNCTION(BlueprintNativeEvent, Category="Goal", meta=(BlueprintProtected="true"))
	float Priority(const UKMGoapAgentComponent* Agent) const;

	/**
	 * Default implementation of the Blueprint priority event.
	 *
	 * @param Agent Agent component evaluating this goal.
	 * @return Runtime priority for this goal.
	 */
	virtual float Priority_Implementation(const UKMGoapAgentComponent* Agent) const;
};
