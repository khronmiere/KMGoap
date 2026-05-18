// All rights reserved by Khrönmière Entertainment.
#include "Subsystem/Behavior/KMGoapPlanSearchSnapshot.h"

#include "Algo/Reverse.h"
#include "HAL/PlatformTime.h"

namespace
{
	constexpr float RecentGoalBias = 0.01f;
}

bool FKMGoapPlanSearchSnapshot::BuildPlan(
	const FKMGoapPlanningSnapshot& Snapshot,
	FKMGoapPlanningSnapshotResult& OutResult)
{
	OutResult = FKMGoapPlanningSnapshotResult{};

	if (Snapshot.Actions.IsEmpty() || Snapshot.Goals.IsEmpty())
	{
		return false;
	}

	TArray<FKMGoapGoalSnapshot> GoalsSorted = Snapshot.Goals;
	GoalsSorted.Sort([](const FKMGoapGoalSnapshot& A, const FKMGoapGoalSnapshot& B)
	{
		const float PA = A.Priority - (A.bIsMostRecentGoal ? RecentGoalBias : 0.f);
		const float PB = B.Priority - (B.bIsMostRecentGoal ? RecentGoalBias : 0.f);
		return PA > PB;
	});

	for (const FKMGoapGoalSnapshot& Goal : GoalsSorted)
	{
		FKMGoapPlanningSnapshotResult Trial;
		if (SolveGoal(Snapshot, Goal, Trial))
		{
			OutResult = MoveTemp(Trial);
			return true;
		}
	}

	return false;
}

bool FKMGoapPlanSearchSnapshot::SolveGoal(
	const FKMGoapPlanningSnapshot& Snapshot,
	const FKMGoapGoalSnapshot& Goal,
	FKMGoapPlanningSnapshotResult& OutResult)
{
	if (Snapshot.Actions.IsEmpty() || Snapshot.Goals.IsEmpty())
	{
		OutResult.FailureReason = EKMGoapPlanningFailureReason::InvalidSnapshot;
		return false;
	}
	
	OutResult = FKMGoapPlanningSnapshotResult{};

	if (Goal.RuntimeGoalIndex == INDEX_NONE || SatisfiesAll(Snapshot.InitialState, Goal.DesiredEffects))
	{
		OutResult.FailureReason = EKMGoapPlanningFailureReason::GoalSatisfied;
		return false;
	}
	
	struct FNode
	{
		float Cost = 0.f;
		int32 ParentIndex = INDEX_NONE;
		int32 RuntimeActionIndex = INDEX_NONE;
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
			return A.Cost > B.Cost;
		}
	};

	TArray<FNode> Nodes;
	Nodes.Reserve(256);

	TArray<FHeapItem> Open;
	Open.Heapify(FHeapLess{});

	TMap<uint32, float> BestCostByState;
	BestCostByState.Reserve(512);

	auto PushNode = [&Nodes, &Open](FNode&& NewNode)
	{
		const int32 Idx = Nodes.Add(MoveTemp(NewNode));
		Open.HeapPush(FHeapItem{ Nodes[Idx].Cost, Idx }, FHeapLess{});
		return Idx;
	};

	FNode Root;
	Root.Cost = 0.f;
	Root.ParentIndex = INDEX_NONE;
	Root.RuntimeActionIndex = INDEX_NONE;
	Root.State = Snapshot.InitialState;
	Root.StateHash = HashState(Root.State);
	Root.Depth = 0;

	BestCostByState.Add(Root.StateHash, Root.Cost);
	PushNode(MoveTemp(Root));

	const double StartSeconds = FPlatformTime::Seconds();
	int32 SolutionIndex = INDEX_NONE;
	int32 ExpandedNodes = 0;
	
	while (Open.Num() > 0)
	{
		if (IsBudgetExceeded(StartSeconds, ExpandedNodes, Snapshot))
		{
			if (SolutionIndex == INDEX_NONE)
			{
				OutResult.FailureReason = EKMGoapPlanningFailureReason::BudgetExceeded;
				return false;
			}
			
			break;
		}

		FHeapItem Item;
		Open.HeapPop(Item, FHeapLess{}, EAllowShrinking::Yes);

		if (!Nodes.IsValidIndex(Item.NodeIndex))
		{
			continue;
		}

		const FNode& Current = Nodes[Item.NodeIndex];

		if (const float* Best = BestCostByState.Find(Current.StateHash))
		{
			if (Current.Cost > *Best + KINDA_SMALL_NUMBER)
			{
				continue;
			}
		}

		ExpandedNodes++;

		if (SatisfiesAll(Current.State, Goal.DesiredEffects))
		{
			SolutionIndex = Item.NodeIndex;
			break;
		}

		if (Current.Depth >= Snapshot.MaxDepth)
		{
			continue;
		}

		for (const FKMGoapActionSnapshot& Action : Snapshot.Actions)
		{
			if (Action.RuntimeActionIndex == INDEX_NONE)
			{
				continue;
			}

			if (!SatisfiesAll(Current.State, Action.Preconditions))
			{
				continue;
			}

			FKMGoapSimState NextState = Current.State;
			ApplyPostconditions(NextState, Action.Postconditions);

			const float NextCost = Current.Cost + Action.Cost;
			const uint32 NextHash = HashState(NextState);
			
			if (const float* Existing = BestCostByState.Find(NextHash))
			{
				if (NextCost >= *Existing)
				{
					continue;
				}
			}
			
			BestCostByState.Add(NextHash, NextCost);

			FNode Next;
			Next.Cost = NextCost;
			Next.ParentIndex = Item.NodeIndex;
			Next.RuntimeActionIndex = Action.RuntimeActionIndex;
			Next.State = MoveTemp(NextState);
			Next.StateHash = NextHash;
			Next.Depth = Current.Depth + 1;

			PushNode(MoveTemp(Next));
		}
	}

	if (SolutionIndex == INDEX_NONE)
	{
		OutResult.FailureReason = EKMGoapPlanningFailureReason::NoPlanFound;
		return false;
	}

	TArray<int32> ReverseActionIndices;
	ReverseActionIndices.Reserve(16);

	int32 Cursor = SolutionIndex;
	while (Cursor != INDEX_NONE)
	{
		const FNode& Node = Nodes[Cursor];
		if (Node.RuntimeActionIndex != INDEX_NONE)
		{
			ReverseActionIndices.Add(Node.RuntimeActionIndex);
		}

		Cursor = Node.ParentIndex;
	}

	Algo::Reverse(ReverseActionIndices);

	if (ReverseActionIndices.IsEmpty())
	{
		OutResult.FailureReason = EKMGoapPlanningFailureReason::EmptySolution;
		return false;
	}

	OutResult.bSuccess = true;
	OutResult.RuntimeGoalIndex = Goal.RuntimeGoalIndex;
	OutResult.RuntimeActionIndices = MoveTemp(ReverseActionIndices);
	OutResult.TotalCost = Nodes[SolutionIndex].Cost;

	return true;
}

bool FKMGoapPlanSearchSnapshot::SatisfiesAll(const FKMGoapSimState& State, const TSet<FKMGoapCondition>& Conditions)
{
	for (const FKMGoapCondition& Condition : Conditions)
	{
		if (!Condition.Tag.IsValid())
		{
			return false;
		}

		bool Value = false;
		State.TryGet(Condition.Tag, Value);
		if (Value != Condition.bValue)
		{
			return false;
		}
	}

	return true;
}

uint32 FKMGoapPlanSearchSnapshot::HashState(const FKMGoapSimState& State)
{
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

	uint32 Hash = GetTypeHash(State.Values.Num());

	for (const FGameplayTag& Key : Keys)
	{
		const bool bValue = State.Values.FindRef(Key);

		const uint32 KeyHash = GetTypeHash(Key);

		// Use distinct constants so true/false always affect the hash differently.
		const uint32 ValueHash = bValue
			? 0x9E3779B9u
			: 0x85EBCA6Bu;

		const uint32 PairHash = HashCombineFast(KeyHash, ValueHash);
		Hash = HashCombineFast(Hash, PairHash);
	}

	return Hash;
}

void FKMGoapPlanSearchSnapshot::ApplyPostconditions(
	FKMGoapSimState& State,
	const TSet<FKMGoapCondition>& Postconditions)
{
	for (const FKMGoapCondition& Condition : Postconditions)
	{
		State.Set(Condition.Tag, Condition.bValue);
	}
}

bool FKMGoapPlanSearchSnapshot::IsBudgetExceeded(
	const double StartSeconds,
	const int32 ExpandedNodes,
	const FKMGoapPlanningSnapshot& Snapshot)
{
	if (ExpandedNodes >= Snapshot.MaxExpandedNodes)
	{
		return true;
	}

	const double ElapsedMs = (FPlatformTime::Seconds() - StartSeconds) * 1000.0;
	return ElapsedMs >= Snapshot.TimeBudgetMs;
}
