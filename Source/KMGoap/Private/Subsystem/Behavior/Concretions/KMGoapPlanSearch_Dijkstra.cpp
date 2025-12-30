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
	constexpr float RecentGoalBias = 0.01f;
	
	void SortGoals(UKMGoapAgentComponent* Agent, TArray<UKMGoapAgentGoal*>& InOutGoals, UKMGoapAgentGoal* MostRecentGoal)
	{
		InOutGoals.Sort([Agent, MostRecentGoal](const UKMGoapAgentGoal& A, const UKMGoapAgentGoal& B)
		{
			const float PA = A.GetPriority(Agent) - ((MostRecentGoal == &A) ? RecentGoalBias : 0.f);
			const float PB = B.GetPriority(Agent) - ((MostRecentGoal == &B) ? RecentGoalBias : 0.f);
			return PA > PB;
		});
	}

	void SortActionsByCost(const UKMGoapAgentComponent* Agent, TArray<TObjectPtr<UKMGoapAgentAction>>& InOutActions)
	{
		InOutActions.Sort([Agent](const UKMGoapAgentAction& A, const UKMGoapAgentAction& B)
		{
			return A.GetDynamicCost(Agent) < B.GetDynamicCost(Agent);
		});
	}

	bool IsFactKnownSatisfied(const UKMGoapAgentComponent* Agent, const FKMGoapCondition& Condition)
	{
		if (!Agent || !Condition.Tag.IsValid())
		{
			return false;
		}

		const EKMGoapBeliefState FactState = Agent->GetFact(Condition.Tag);
		const EKMGoapBeliefState Expected = Condition.bValue ? EKMGoapBeliefState::Positive : EKMGoapBeliefState::Negative;

		return FactState == Expected;
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

	// Try goals in priority order. First solvable goal wins (matches prior behavior).
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

	// Ensure cache is fresh at planning time. (Beliefs are read-only during the search.)
	Agent->UpdateBeliefEvaluationCache();

	OutCtx = FKMGoapPlanningContext{};
	OutCtx.Agent = Agent;

	// 1) Build initial simulated state snapshot:
	//    - Facts: from agent
	//    - Beliefs: from cached belief results, treated as snapshot "sensor facts"
	//
	// This is the key to avoid calling agent queries during action chaining.
	{
		// Facts Snapshot
		auto FactsTags = Agent->GetFactsTags();
		for (const FGameplayTag& Tag : FactsTags)
		{
			EKMGoapBeliefState FactState = Agent->GetFact(Tag);
			OutCtx.InitialState.Set(Tag, FactState == EKMGoapBeliefState::Positive);
		}
		
		// Beliefs snapshot
		TArray<FGameplayTag> BeliefsTags;
		Agent->BeliefsByTag.GetKeys(BeliefsTags);
		for (const FGameplayTag& BeliefsTag : BeliefsTags)
		{
			EKMGoapBeliefState Result = Agent->EvaluateBeliefByTag(BeliefsTag);
			OutCtx.InitialState.Set(BeliefsTag, Result == EKMGoapBeliefState::Positive);
		}
	}

	// 2) Filter goals (only those not already satisfied by the INITIAL SNAPSHOT)
	OutGoalsSorted.Reset();
	OutGoalsSorted.Reserve(GoalsToCheck.Num());
	for (UKMGoapAgentGoal* Goal : GoalsToCheck)
	{
		if (!Goal)
		{
			continue;
		}

		// If the goal is already satisfied right now, skip it.
		// Use initial snapshot rules:
		// - if state has a value, use it
		// - else fallback to known facts from agent, then belief cache
		bool bAnyUnsatisfied = false;
		for (const FKMGoapCondition& Condition : Goal->DesiredEffects)
		{
			bool bValue = false;
			if (IsFactKnownSatisfied(Agent, Condition))
			{
				continue;
			}
			
			if (OutCtx.InitialState.TryGet(Condition.Tag, bValue))
			{
				if (bValue != Condition.bValue)
				{
					bAnyUnsatisfied = true;
					break;
				}
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

	// 3) Collect usable actions once (filtering should stay outside the search loop)
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

	// Root simulated state is the snapshot.
	FKMGoapSimState RootState = Context.InitialState;

	// Early out if already satisfied by snapshot.
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
			// min-heap
			return A.Cost > B.Cost;
		}
	};

	TArray<FNode> Nodes;
	Nodes.Reserve(256);

	TArray<FHeapItem> Open;
	Open.Heapify(FHeapLess{});

	// Best known cost per state hash (pruning). Hash collisions are possible but rare;
	// if you want collision-proof, use a canonical key (sorted pairs) instead of uint32.
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

		// Skip stale heap entries (a cheaper cost for same state was found after this node was queued).
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

		// Expand: try all actions whose preconditions are satisfied in this simulated state.
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

			// Next state = current state + postconditions (simulate action completion)
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

	// Reconstruct action chain
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
			// Unknown in simulation => not satisfied.
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
	// Canonical hashing:
	// 1) Copy keys
	// 2) Sort by tag name (stable)
	// 3) Hash pairs (tag + bool)
	TArray<FGameplayTag> Keys;
	Keys.Reserve(State.Facts.Num());

	for (const TTuple<FGameplayTag, bool>& Pair : State.Facts)
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
		const bool* V = State.Facts.Find(K);
		Hash = HashCombineFast(Hash, GetTypeHash(K));
		Hash = HashCombineFast(Hash, GetTypeHash(V ? *V : false));
	}

	Hash = HashCombineFast(Hash, GetTypeHash(State.Facts.Num()));
	return Hash;
}

void UKMGoapPlanSearch_Dijkstra::ApplyPostconditions(FKMGoapSimState& State, const TSet<FKMGoapCondition>& Post) const
{
	for (const FKMGoapCondition& Condition : Post)
	{
		State.Set(Condition.Tag, Condition.bValue);
	}
}
