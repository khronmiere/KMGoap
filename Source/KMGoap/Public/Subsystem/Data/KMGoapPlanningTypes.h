// All rights reserved by Khrönmière Entertainment.
#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Data/KMGoapCondition.h"
#include "KMGoapPlanningTypes.generated.h"

/**
 * Lightweight simulated world-state snapshot used by the GOAP planner.
 *
 * The planner mutates and queries this structure while evaluating potential
 * action chains without changing the agent's real runtime facts or beliefs.
 */
USTRUCT()
struct FKMGoapSimState
{
	GENERATED_BODY()

	/**
	 * Boolean simulated condition values indexed by gameplay tag.
	 *
	 * The planner stores both copied runtime facts and cached belief values here.
	 * This structure represents simulated planning state only; mutating it does not
	 * write to the agent's real runtime facts or beliefs.
	 */
	TMap<FGameplayTag, bool> Values;

	/**
	 * Attempts to retrieve the simulated value for a tagged condition.
	 *
	 * @param Tag Gameplay tag identifying the condition to look up.
	 * @param Out Receives the stored value when the condition exists.
	 * @return True if the condition exists in the simulated state; otherwise false.
	 */
	bool TryGet(const FGameplayTag& Tag, bool& Out) const
	{
		if (const bool* V = Values.Find(Tag))
		{
			Out = *V;
			return true;
		}
		return false;
	}

	/**
	 * Sets or overwrites the simulated value for a tagged condition.
	 *
	 * @param Tag Gameplay tag identifying the condition to update.
	 * @param bValue New boolean value to store for the condition.
	 */
	void Set(const FGameplayTag& Tag, bool bValue)
	{
		Values.Add(Tag, bValue);
	}

	/**
	 * Checks whether this simulated state satisfies a GOAP condition.
	 *
	 * A condition is satisfied only when the condition tag exists in the state
	 * and its stored value matches the condition's expected value.
	 *
	 * @param Condition Condition to evaluate against this simulated state.
	 * @return True if the state contains the condition tag with the expected value; otherwise false.
	 */
	bool Satisfies(const FKMGoapCondition& Condition) const
	{
		bool V = false;
		return TryGet(Condition.Tag, V) && (V == Condition.bValue);
	}
};

/**
 * Planning input context passed to GOAP search algorithms.
 *
 * This structure groups together the immutable initial state, the requesting
 * agent, and the filtered action list available for a single planning request.
 */
struct FKMGoapPlanningContext
{
	/**
	 * Initial simulated state copied from the agent at the start of planning.
	 *
	 * Search algorithms should use this as the root state for plan expansion.
	 */
	FKMGoapSimState InitialState;

	/**
	 * Weak reference to the agent requesting the plan.
	 *
	 * This may be used by planning algorithms to evaluate goal priority or
	 * access agent-specific context without owning the component.
	 */
	TWeakObjectPtr<class UKMGoapAgentComponent> Agent;

	/**
	 * Collection of actions available to the planner for the current request.
	 *
	 * The actions are expected to already be filtered to those usable by the
	 * requesting agent before planning begins.
	 */
	TArray<TObjectPtr<class UKMGoapAgentAction>> Actions;
};
