// All rights reserved by Khrönmière Entertainment.
#pragma once

#include "CoreMinimal.h"
#include "Subsystem/Data/KMGoapPlanningTypes.h"

/**
 * Stateless, thread-safe GOAP search utility for planning snapshots.
 *
 * This helper contains no UObject access and may run on UE worker threads. The
 * planner subsystem owns request lifecycle and delegates the CPU-heavy search here.
 */
class KMGOAP_API FKMGoapPlanSearchSnapshot
{
public:
	/**
	 * Builds a plan from an immutable planning snapshot.
	 *
	 * @param Snapshot Value-only planning input captured on the game thread.
	 * @param OutResult Result containing runtime goal/action indices on success.
	 * @return True when a valid plan was found.
	 */
	static bool BuildPlan(const FKMGoapPlanningSnapshot& Snapshot, FKMGoapPlanningSnapshotResult& OutResult);

private:
	/** Checks whether a simulated state satisfies every condition. */
	static bool SatisfiesAll(const FKMGoapSimState& State, const TSet<FKMGoapCondition>& Conditions);

	/** Computes a deterministic hash for a simulated state. */
	static uint32 HashState(const FKMGoapSimState& State);

	/** Applies postconditions to a simulated state. */
	static void ApplyPostconditions(FKMGoapSimState& State, const TSet<FKMGoapCondition>& Postconditions);

	/** Checks worker-side search limits. */
	static bool IsBudgetExceeded(double StartSeconds, int32 ExpandedNodes, const FKMGoapPlanningSnapshot& Snapshot);

	/** Attempts to solve one goal using Dijkstra search. */
	static bool SolveGoal(
		const FKMGoapPlanningSnapshot& Snapshot,
		const FKMGoapGoalSnapshot& Goal,
		FKMGoapPlanningSnapshotResult& OutResult);
};
