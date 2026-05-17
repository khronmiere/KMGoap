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
 * Thread-safe value snapshot of an action used by asynchronous planning.
 *
 * This struct intentionally stores only plain data copied on the game thread.
 * Worker threads must not read UObject state, call Blueprint events, or query the
 * world while planning.
 */
struct FKMGoapActionSnapshot
{
	/** Index of the runtime action inside the request's action pointer table. */
	int32 RuntimeActionIndex = INDEX_NONE;

	/** Planning cost captured on the game thread. */
	float Cost = 1.f;

	/** Conditions required before the action can be applied in simulated state. */
	TSet<FKMGoapCondition> Preconditions;

	/** Conditions produced by the action in simulated state. */
	TSet<FKMGoapCondition> Postconditions;
};

/**
 * Thread-safe value snapshot of a goal used by asynchronous planning.
 */
struct FKMGoapGoalSnapshot
{
	/** Index of the runtime goal inside the request's goal pointer table. */
	int32 RuntimeGoalIndex = INDEX_NONE;

	/** Runtime priority captured on the game thread. */
	float Priority = 0.f;

	/** Whether this goal is the request's most recently selected/completed goal. */
	bool bIsMostRecentGoal = false;

	/** Desired conditions that must be satisfied by a generated plan. */
	TSet<FKMGoapCondition> DesiredEffects;
};

/**
 * Immutable value snapshot consumed by async GOAP search workers.
 *
 * The snapshot is created on the game thread and then treated as read-only by worker
 * threads. It contains no UObject pointers.
 */
struct FKMGoapPlanningSnapshot
{
	/** Initial simulated facts/beliefs captured from the agent. */
	FKMGoapSimState InitialState;

	/** Available actions copied into thread-safe value form. */
	TArray<FKMGoapActionSnapshot> Actions;

	/** Candidate goals copied into thread-safe value form. */
	TArray<FKMGoapGoalSnapshot> Goals;

	/** Search limit copied from the active planner configuration. */
	int32 MaxExpandedNodes = 5000;

	/** Maximum allowed action depth for a generated plan. */
	int32 MaxDepth = 64;

	/** Worker-side time budget in milliseconds. */
	float TimeBudgetMs = 2.0f;
};

/**
 * Thread-safe result produced by async GOAP planning.
 *
 * Runtime pointer reconstruction is done later on the game thread by using the
 * stored goal/action indices.
 */
struct FKMGoapPlanningSnapshotResult
{
	/** True when a valid plan was found. */
	bool bSuccess = false;

	/** Index of the selected runtime goal in the request's goal pointer table. */
	int32 RuntimeGoalIndex = INDEX_NONE;

	/** Ordered runtime action indices selected by the search. */
	TArray<int32> RuntimeActionIndices;

	/** Total calculated cost of the selected plan. */
	float TotalCost = 0.f;
};
