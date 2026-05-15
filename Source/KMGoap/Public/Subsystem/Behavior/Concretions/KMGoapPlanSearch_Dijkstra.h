// All rights reserved by Khrönmière Entertainment.
#pragma once

#include "CoreMinimal.h"
#include "Subsystem/Behavior/KMGoapPlanSearchBase.h"
#include "KMGoapPlanSearch_Dijkstra.generated.h"

struct FKMGoapPlanningContext;
struct FKMGoapCondition;
struct FKMGoapSimState;

/**
 * GOAP plan-search implementation that uses Dijkstra's algorithm to find the lowest-cost valid action plan.
 *
 * The search starts from the agent's current simulated world state and expands available actions by cumulative
 * action cost until a selected goal's desired conditions are satisfied, or until planner limits prevent further
 * exploration.
 */
UCLASS()
class KMGOAP_API UKMGoapPlanSearch_Dijkstra : public UKMGoapPlanSearchBase
{
	GENERATED_BODY()

public:
	/**
	 * Builds an executable GOAP action plan for the highest-priority reachable goal.
	 *
	 * This method prepares a planning context from the agent, evaluates candidate goals, and attempts to solve
	 * each goal using Dijkstra search until a valid plan is found.
	 *
	 * @param Agent Agent component that owns beliefs, actions, goals, facts, and planner-relevant runtime data.
	 * @param GoalsToCheck Candidate goals that should be considered for planning.
	 * @param MostRecentGoal Goal most recently completed or selected, used to support goal ordering/tie-breaking.
	 * @param OutPlan Generated action plan when planning succeeds.
	 * @return True when a valid plan was generated; false otherwise.
	 */
	virtual bool BuildPlan_Implementation(
		UKMGoapAgentComponent* Agent,
		const TArray<UKMGoapAgentGoal*>& GoalsToCheck,
		UKMGoapAgentGoal* MostRecentGoal,
		FKMGoapActionPlan& OutPlan) override;

private:
	/**
	 * Builds the immutable data required to run a planning search.
	 *
	 * The context includes the agent's current known state, available actions, configured planner limits,
	 * and sorted goal candidates.
	 *
	 * @param Agent Agent component used as the source of planning data.
	 * @param GoalsToCheck Candidate goals supplied by the state machine.
	 * @param MostRecentGoal Goal that was most recently active, used when sorting candidate goals.
	 * @param OutGoalsSorted Candidate goals sorted into the order they should be attempted.
	 * @param OutCtx Planning context populated for the Dijkstra solver.
	 * @return True when the context was built successfully; false when required data is missing or invalid.
	 */
	bool BuildContext(
		UKMGoapAgentComponent* Agent,
		const TArray<UKMGoapAgentGoal*>& GoalsToCheck,
		UKMGoapAgentGoal* MostRecentGoal,
		TArray<UKMGoapAgentGoal*>& OutGoalsSorted,
		FKMGoapPlanningContext& OutCtx) const;

	/**
	 * Attempts to solve a single goal using Dijkstra's shortest-path search.
	 *
	 * The solver expands simulated states by applying action postconditions and tracks cumulative cost until
	 * the goal's desired conditions are satisfied.
	 *
	 * @param Context Prepared planning context containing world state, actions, and planner limits.
	 * @param Goal Goal to satisfy.
	 * @param OutPlan Generated action sequence for the goal when the search succeeds.
	 * @return True when the goal can be reached with a valid action sequence; false otherwise.
	 */
	bool SolveGoalDijkstra(
		const FKMGoapPlanningContext& Context,
		UKMGoapAgentGoal* Goal,
		FKMGoapActionPlan& OutPlan) const;

	/**
	 * Checks whether a simulated state satisfies every required condition.
	 *
	 * @param State Simulated world state to test.
	 * @param Conditions Conditions that must all match the simulated state.
	 * @return True when every condition is satisfied; false when at least one condition is missing or mismatched.
	 */
	static bool SatisfiesAll(const FKMGoapSimState& State, const TSet<FKMGoapCondition>& Conditions);

	/**
	 * Computes a deterministic hash for a simulated world state.
	 *
	 * The hash is used by the search to identify equivalent states and avoid re-processing more expensive paths.
	 *
	 * @param State Simulated world state to hash.
	 * @return Hash value representing the state's current condition values.
	 */
	static uint32 HashState(const FKMGoapSimState& State);

	/**
	 * Applies action postconditions to a simulated world state.
	 *
	 * Existing condition values are overwritten by matching postconditions, and missing condition values are added.
	 *
	 * @param State Simulated world state to mutate.
	 * @param Post Postconditions produced by an action.
	 */
	void ApplyPostconditions(FKMGoapSimState& State, const TSet<FKMGoapCondition>& Post) const;
};
