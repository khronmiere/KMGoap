// All rights reserved by Khrönmière Entertainment.
#include "Subsystem/Behavior/Concretions/KMGoapPlanSearch_Dijkstra.h"

#include "Blueprint/Component/KMGoapAgentComponent.h"
#include "Blueprint/KMGoapAgentAction.h"
#include "Blueprint/KMGoapAgentGoal.h"
#include "Data/KMGoapCondition.h"

#include "Algo/Reverse.h"
#include "Containers/Map.h"
#include "Containers/Set.h"
#include "Data/KMGoapActionPlan.h"
#include "HAL/PlatformTime.h"
#include "Subsystem/Data/KMGoapPlanningTypes.h"

DEFINE_LOG_CATEGORY_STATIC(LogGoapDijkstra, Log, All);

namespace
{
	/**
	 * Small priority adjustment used to prefer the most recently selected goal when
	 * competing goals are otherwise close in priority.
	 *
	 * This reduces visual/behavioral thrashing when two goals have nearly identical
	 * scores, while still allowing clearly higher-priority goals to take over.
	 */
	constexpr float RecentGoalBias = 0.01f;

	/**
	 * Orders candidate goals by runtime priority, with a slight bias toward goal continuity.
	 */
	void SortGoals(UKMGoapAgentComponent* Agent, TArray<UKMGoapAgentGoal*>& InOutGoals, UKMGoapAgentGoal* MostRecentGoal)
	{
		InOutGoals.Sort([Agent, MostRecentGoal](const UKMGoapAgentGoal& A, const UKMGoapAgentGoal& B)
		{
			const float PA = A.GetPriority(Agent) - ((MostRecentGoal == &A) ? RecentGoalBias : 0.f);
			const float PB = B.GetPriority(Agent) - ((MostRecentGoal == &B) ? RecentGoalBias : 0.f);
			return PA > PB;
		});
	}

	/**
	 * Gives cheaper actions earlier consideration during graph expansion.
	 *
	 * Dijkstra still guarantees least-cost discovery through the heap ordering; this
	 * pre-sort only improves the shape of exploration when multiple actions are valid
	 * from the same state.
	 */
	void SortActionsByCost(const UKMGoapAgentComponent* Agent, TArray<TObjectPtr<UKMGoapAgentAction>>& InOutActions)
	{
		InOutActions.Sort([Agent](const UKMGoapAgentAction& A, const UKMGoapAgentAction& B)
		{
			return A.GetDynamicCost(Agent) < B.GetDynamicCost(Agent);
		});
	}

	/**
	 * Returns true when a condition refers to an agent fact instead of a runtime belief.
	 *
	 * Facts are stored directly on the agent and do not have a corresponding belief
	 * object. Beliefs, by contrast, are evaluated and cached before planning begins.
	 */
	bool IsFact(const UKMGoapAgentComponent* Agent, const FKMGoapCondition& Condition)
	{
		if (!Agent || !Condition.Tag.IsValid())
		{
			return false;
		}

		UKMGoapAgentBelief* Belief = Agent->GetBeliefByTag(Condition.Tag);
		return Belief == nullptr;
	}

	/**
	 * Checks whether a fact is known and already matches the requested condition.
	 *
	 * Unknown facts are treated as not satisfied so that the planner can still search
	 * for actions capable of establishing them.
	 */
	bool IsFactKnownSatisfied(const UKMGoapAgentComponent* Agent, const FKMGoapCondition& Condition)
	{
		if (!Agent || !Condition.Tag.IsValid())
		{
			return false;
		}

		const EKMGoapBeliefState FactState = Agent->GetFact(Condition.Tag);
		const EKMGoapBeliefState Expected = Condition.bValue ? EKMGoapBeliefState::Positive : EKMGoapBeliefState::Negative;

		return FactState != EKMGoapBeliefState::Unknown && FactState == Expected;
	}
}

bool UKMGoapPlanSearch_Dijkstra::BuildPlan_Implementation(
	UKMGoapAgentComponent* Agent,
	const TArray<UKMGoapAgentGoal*>& GoalsToCheck,
	UKMGoapAgentGoal* MostRecentGoal,
	FKMGoapActionPlan& OutPlan)
{
	OutPlan.Reset();

	if (!Agent)
	{
		UE_LOG(LogGoapDijkstra, Warning, TEXT("BuildPlan: Agent is null."));
		return false;
	}

	TArray<UKMGoapAgentGoal*> GoalsSorted;
	FKMGoapPlanningContext Ctx;

	if (!BuildContext(Agent, GoalsToCheck, MostRecentGoal, GoalsSorted, Ctx))
	{
		return false;
	}

	// Goals are attempted in priority order and the first solvable goal is accepted.
	// This keeps high-level behavior deterministic: the planner does not compare
	// total action cost across different goals, only within a single goal search.
	for (UKMGoapAgentGoal* Goal : GoalsSorted)
	{
		if (!Goal)
		{
			continue;
		}

		FKMGoapActionPlan Trial;
		Trial.Reset();

		if (SolveGoalDijkstra(Ctx, Goal, Trial) && Trial.IsValid())
		{
			OutPlan = MoveTemp(Trial);
			return true;
		}
	}

	return false;
}

bool UKMGoapPlanSearch_Dijkstra::BuildContext(
	UKMGoapAgentComponent* Agent,
	const TArray<UKMGoapAgentGoal*>& GoalsToCheck,
	UKMGoapAgentGoal* MostRecentGoal,
	TArray<UKMGoapAgentGoal*>& OutGoalsSorted,
	FKMGoapPlanningContext& OutCtx) const
{
	if (!Agent)
	{
		return false;
	}

	// Planning operates on a stable snapshot. Beliefs may query world state, so they
	// are evaluated once up front instead of being queried repeatedly while expanding
	// simulated action chains.
	Agent->UpdateBeliefEvaluationCache();

	OutCtx = FKMGoapPlanningContext{};
	OutCtx.Agent = Agent;

	// Build the initial simulated state from both explicit facts and cached beliefs.
	//
	// Facts represent authored or externally controlled agent state.
	// Beliefs represent sensor-derived state at the moment planning begins.
	//
	// Once copied into InitialState, both are treated uniformly by the planner.
	{
		auto FactsTags = Agent->GetFactsTags();
		for (const FGameplayTag& Tag : FactsTags)
		{
			EKMGoapBeliefState FactState = Agent->GetFact(Tag);
			if (FactState != EKMGoapBeliefState::Unknown)
			{
				OutCtx.InitialState.Set(Tag, FactState == EKMGoapBeliefState::Positive);
			}
		}

		TArray<FGameplayTag> BeliefsTags;
		Agent->BeliefsByTag.GetKeys(BeliefsTags);
		for (const FGameplayTag& BeliefsTag : BeliefsTags)
		{
			EKMGoapBeliefState Result = Agent->EvaluateBeliefByTag(BeliefsTag);
			if (Result != EKMGoapBeliefState::Unknown)
			{
				OutCtx.InitialState.Set(BeliefsTag, Result == EKMGoapBeliefState::Positive);
			}
		}
	}

	// Only unsatisfied goals enter the search. A goal that already matches the
	// initial snapshot would produce an empty plan, which is not useful for action
	// execution.
	OutGoalsSorted.Reset();
	OutGoalsSorted.Reserve(GoalsToCheck.Num());
	for (UKMGoapAgentGoal* Goal : GoalsToCheck)
	{
		if (!Goal)
		{
			continue;
		}

		bool bAnyUnsatisfied = false;
		for (const FKMGoapCondition& Condition : Goal->DesiredEffects)
		{
			if (IsFact(Agent, Condition))
			{
				if (IsFactKnownSatisfied(Agent, Condition))
				{
					continue;
				}

				bAnyUnsatisfied = true;
				break;
			}

			bool bValue = false;
			if (!OutCtx.InitialState.TryGet(Condition.Tag, bValue))
			{
				// Unknown belief-backed conditions are not satisfied. This matches
				// SatisfiesAll(), which requires every condition to be explicitly present
				// in the simulated state before it can be considered satisfied.
				bAnyUnsatisfied = true;
				break;
			}

			if (bValue != Condition.bValue)
			{
				bAnyUnsatisfied = true;
				break;
			}
		}

		if (bAnyUnsatisfied)
		{
			OutGoalsSorted.Add(Goal);
		}
	}

	if (OutGoalsSorted.Num() == 0)
	{
		return false;
	}

	SortGoals(Agent, OutGoalsSorted, MostRecentGoal);

	// Action filtering is intentionally performed once before search. The search
	// loop should only reason about simulated state transitions, not about whether
	// actions exist or can be loaded.
	OutCtx.Actions.Reset();
	OutCtx.Actions.Reserve(Agent->ActionsByTag.Num());

	for (const TTuple<FGameplayTag, TObjectPtr<UKMGoapAgentAction>>& Pair : Agent->ActionsByTag)
	{
		UKMGoapAgentAction* Action = Pair.Value;
		if (!Action)
		{
			continue;
		}

		OutCtx.Actions.Add(Action);
	}

	if (OutCtx.Actions.Num() == 0)
	{
		return false;
	}

	SortActionsByCost(Agent, OutCtx.Actions);
	return true;
}

bool UKMGoapPlanSearch_Dijkstra::SolveGoalDijkstra(
	const FKMGoapPlanningContext& Context,
	UKMGoapAgentGoal* Goal,
	FKMGoapActionPlan& OutPlan) const
{
	OutPlan.Reset();

	UKMGoapAgentComponent* Agent = Context.Agent.Get();
	if (!Agent || !Goal)
	{
		return false;
	}

	const double StartSeconds = FPlatformTime::Seconds();
	int32 ExpandedNodes = 0;

	// The root state is a copy of the planning snapshot. Every explored node owns
	// its own simulated state so planner postconditions can be applied without mutating
	// the real agent.
	FKMGoapSimState RootState = Context.InitialState;

	if (SatisfiesAll(RootState, Goal->DesiredEffects))
	{
		return false;
	}

	struct FNode
	{
		float Cost = 0.f;
		int32 ParentIndex = INDEX_NONE;
		TObjectPtr<UKMGoapAgentAction> Action = nullptr;
		FKMGoapSimState State;
		uint32 StateHash = 0;
		int32 Depth = 0;
	};

	struct FHeapItem
	{
		float Cost = 0.f;
		int32 NodeIndex = INDEX_NONE;
	};

	struct FHeapLess
	{
		bool operator()(const FHeapItem& A, const FHeapItem& B) const
		{
			// Unreal's heap helpers use this predicate shape to produce a min-cost queue.
			return A.Cost > B.Cost;
		}
	};

	TArray<FNode> Nodes;
	Nodes.Reserve(256);

	TArray<FHeapItem> Open;
	Open.Heapify(FHeapLess{});

	// Tracks the cheapest discovered path to each simulated state.
	//
	// This is the core Dijkstra pruning rule: if a state has already been reached
	// at a lower or equal cost, expanding the more expensive path cannot improve the
	// final plan.
	//
	// StateHash is compact and fast, but not collision-proof. If planning correctness
	// ever depends on distinguishing extremely similar large states, replace this
	// with a canonical state key.
	TMap<uint32, float> BestCostByState;
	BestCostByState.Reserve(512);

	auto PushNode = [&](FNode&& NewNode)
	{
		const int32 Idx = Nodes.Add(MoveTemp(NewNode));
		Open.HeapPush(FHeapItem{ Nodes[Idx].Cost, Idx }, FHeapLess{});
		return Idx;
	};

	{
		FNode Root;
		Root.Cost = 0.f;
		Root.ParentIndex = INDEX_NONE;
		Root.Action = nullptr;
		Root.State = MoveTemp(RootState);
		Root.StateHash = HashState(Root.State);
		Root.Depth = 0;

		BestCostByState.Add(Root.StateHash, 0.f);
		PushNode(MoveTemp(Root));
	}

	int32 SolutionIndex = INDEX_NONE;

	while (Open.Num() > 0)
	{
		if (IsBudgetExceeded(StartSeconds, ExpandedNodes))
		{
			break;
		}

		FHeapItem Item;
		Open.HeapPop(Item, FHeapLess{}, EAllowShrinking::Yes);

		if (!Nodes.IsValidIndex(Item.NodeIndex))
		{
			continue;
		}

		FNode& Current = Nodes[Item.NodeIndex];

		// Heap entries are not updated in place. When a cheaper route to the same
		// state is discovered, the older queued entry becomes stale and is ignored
		// when eventually popped.
		if (const float* Best = BestCostByState.Find(Current.StateHash))
		{
			if (Current.Cost > *Best + KINDA_SMALL_NUMBER)
			{
				continue;
			}
		}

		ExpandedNodes++;

		if (SatisfiesAll(Current.State, Goal->DesiredEffects))
		{
			SolutionIndex = Item.NodeIndex;
			break;
		}

		if (Current.Depth >= MaxDepth)
		{
			continue;
		}

		// Expand from the current simulated state by applying every action whose
		// preconditions already hold. Preconditions are not re-evaluated against the
		// world; only the simulated state matters from this point forward.
		for (UKMGoapAgentAction* Action : Context.Actions)
		{
			if (!Action)
			{
				continue;
			}

			if (!SatisfiesAll(Current.State, Action->Preconditions))
			{
				continue;
			}

			FKMGoapSimState NextState = Current.State;
			ApplyPostconditions(NextState, Action->GetPostConditions());

			const float NextCost = Current.Cost + Action->GetDynamicCost(Agent);
			const uint32 NextHash = HashState(NextState);

			if (const float* Existing = BestCostByState.Find(NextHash))
			{
				if (NextCost >= *Existing - KINDA_SMALL_NUMBER)
				{
					continue;
				}
			}

			BestCostByState.Add(NextHash, NextCost);

			FNode Next;
			Next.Cost = NextCost;
			Next.ParentIndex = Item.NodeIndex;
			Next.Action = Action;
			Next.State = MoveTemp(NextState);
			Next.StateHash = NextHash;
			Next.Depth = Current.Depth + 1;

			PushNode(MoveTemp(Next));
		}
	}

	if (SolutionIndex == INDEX_NONE)
	{
		return false;
	}

	// Nodes store parent links instead of full paths to keep expansion cheaper.
	// Once a solution is found, walk backward from the solution node and reverse the
	// collected actions into execution order.
	TArray<TObjectPtr<UKMGoapAgentAction>> ReverseActions;
	ReverseActions.Reserve(16);

	int32 Cursor = SolutionIndex;
	while (Cursor != INDEX_NONE)
	{
		const FNode& N = Nodes[Cursor];
		if (N.Action)
		{
			ReverseActions.Add(N.Action);
		}
		Cursor = N.ParentIndex;
	}

	Algo::Reverse(ReverseActions);

	if (ReverseActions.Num() == 0)
	{
		return false;
	}

	OutPlan.Goal = Goal;
	OutPlan.Actions = MoveTemp(ReverseActions);
	OutPlan.TotalCost = Nodes[SolutionIndex].Cost;

	return true;
}

bool UKMGoapPlanSearch_Dijkstra::SatisfiesAll(const FKMGoapSimState& State, const TSet<FKMGoapCondition>& Conditions)
{
	for (const FKMGoapCondition& Condition : Conditions)
	{
		if (!Condition.Tag.IsValid())
		{
			return false;
		}

		bool Value = false;
		if (!State.TryGet(Condition.Tag, Value))
		{
			// Unknown simulated values are not assumed to be false; they are simply
			// unavailable. Requiring explicit state prevents plans from relying on
			// information that no action or sensor has established.
			return false;
		}

		if (Value != Condition.bValue)
		{
			return false;
		}
	}

	return true;
}

uint32 UKMGoapPlanSearch_Dijkstra::HashState(const FKMGoapSimState& State)
{
	// TMap iteration order is not stable, so hashing directly over map pairs would
	// produce different hashes for equivalent states. Sorting keys first gives each
	// logical state a deterministic hash input order.
	TArray<FGameplayTag> Keys;
	Keys.Reserve(State.Values.Num());

	for (const TTuple<FGameplayTag, bool>& Pair : State.Values)
	{
		Keys.Add(Pair.Key);
	}

	Keys.Sort([](const FGameplayTag& A, const FGameplayTag& B)
	{
		return A.ToString() < B.ToString();
	});

	uint32 Hash = 0;
	for (const FGameplayTag& K : Keys)
	{
		const bool* V = State.Values.Find(K);
		Hash = HashCombineFast(Hash, GetTypeHash(K));
		Hash = HashCombineFast(Hash, GetTypeHash(V ? *V : false));
	}

	Hash = HashCombineFast(Hash, GetTypeHash(State.Values.Num()));
	return Hash;
}

void UKMGoapPlanSearch_Dijkstra::ApplyPostconditions(FKMGoapSimState& State, const TSet<FKMGoapCondition>& Post) const
{
	for (const FKMGoapCondition& Condition : Post)
	{
		State.Set(Condition.Tag, Condition.bValue);
	}
}
