// All rights reserved by Khrönmière Entertainment.
#include "Subsystem/KMGoapPlannerSubsystem.h"
#include "Settings/KMGoapSettings.h"
#include "Settings/Data/KMGoapPlannerConfig.h"
#include "Subsystem/Behavior/KMGoapPlanSearchSnapshot.h"

#include "Async/Async.h"
#include "Blueprint/KMGoapAgentAction.h"
#include "Blueprint/KMGoapAgentGoal.h"
#include "Blueprint/Component/KMGoapAgentComponent.h"
#include "Misc/ScopeExit.h"


DEFINE_LOG_CATEGORY_STATIC(LogGoapPlanner, Log, All);

void UKMGoapPlannerSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	// The subsystem owns async request lifecycle and reads planner limits from
	// project settings once at startup.
	LoadPlannerConfig();
}

void UKMGoapPlannerSubsystem::Deinitialize()
{
	// Dropping pending and queued requests prevents callbacks from firing after subsystem shutdown.
	PendingRequests.Reset();
	QueuedPlanWork.Reset();
	ActiveAsyncPlanCount = 0;
	LoadedConfig = nullptr;

	Super::Deinitialize();
}

void UKMGoapPlannerSubsystem::LoadPlannerConfig()
{
	const UKMGoapSettings* Settings = GetDefault<UKMGoapSettings>();
	if (!Settings)
	{
		return;
	}

	// Planner configuration is a project-level soft reference. Loading it here keeps
	// request handling deterministic and avoids per-agent configuration loads.
	LoadedConfig = Settings->PlannerConfig.LoadSynchronous();
	if (!LoadedConfig)
	{
		UE_LOG(LogGoapPlanner, Warning,
			TEXT("KMGoap: PlannerConfig not set or failed to load. Async planner will use default limits."));
		return;
	}

	MaxConcurrentAsyncPlans = FMath::Max(1, LoadedConfig->MaxConcurrentAsyncPlans);
	MaxQueuedAsyncPlans = FMath::Max(0, LoadedConfig->MaxQueuedAsyncPlans);
}

void UKMGoapPlannerSubsystem::ApplyPlanningLimits(FKMGoapPlanningSnapshot& OutSnapshot) const
{
	if (!LoadedConfig)
	{
		return;
	}

	OutSnapshot.MaxExpandedNodes = LoadedConfig->MaxExpandedNodes;
	OutSnapshot.MaxDepth = LoadedConfig->MaxDepth;
	OutSnapshot.TimeBudgetMs = LoadedConfig->TimeBudgetMs;
}

FKMGoapPlanningRequestHandle UKMGoapPlannerSubsystem::RequestPlanAsync(FKMGoapPlanningRequest&& Request)
{
	FKMGoapPlanningRequestHandle Handle;
	Handle.RequestId = FGuid::NewGuid();

	FKMGoapPlanningSnapshot Snapshot;
	FPendingPlanRequest PendingRequest;

	if (!BuildPlanningSnapshot(Request, Snapshot, PendingRequest))
	{
		if (Request.OnPlanFailed.IsBound())
		{
			Request.OnPlanFailed.Execute(Handle);
		}

		return Handle;
	}

	if (MaxQueuedAsyncPlans > 0 && QueuedPlanWork.Num() >= MaxQueuedAsyncPlans)
	{
		UE_LOG(LogGoapPlanner, Warning,
			TEXT("RequestPlanAsync failed: async GOAP planning queue is full. MaxQueuedAsyncPlans=%d"),
			MaxQueuedAsyncPlans);

		if (Request.OnPlanFailed.IsBound())
		{
			Request.OnPlanFailed.Execute(Handle);
		}

		return Handle;
	}

	PendingRequests.Add(Handle.RequestId, MoveTemp(PendingRequest));

	FQueuedPlanWork Work;
	Work.Handle = Handle;
	Work.Snapshot = MoveTemp(Snapshot);
	QueuedPlanWork.Add(MoveTemp(Work));

	TryDispatchQueuedPlanWork();

	return Handle;
}

void UKMGoapPlannerSubsystem::CancelPlanRequest(const FKMGoapPlanningRequestHandle& Handle)
{
	if (!Handle.IsValid())
	{
		return;
	}

	PendingRequests.Remove(Handle.RequestId);

	QueuedPlanWork.RemoveAllSwap(
		[&Handle](const FQueuedPlanWork& Work)
		{
			return Work.Handle == Handle;
		},
		EAllowShrinking::No);
}

void UKMGoapPlannerSubsystem::TryDispatchQueuedPlanWork()
{
	while (ActiveAsyncPlanCount < MaxConcurrentAsyncPlans && !QueuedPlanWork.IsEmpty())
	{
		FQueuedPlanWork Work = MoveTemp(QueuedPlanWork[0]);
		QueuedPlanWork.RemoveAt(0, 1, EAllowShrinking::No);

		if (!Work.Handle.IsValid() || !PendingRequests.Contains(Work.Handle.RequestId))
		{
			continue;
		}

		DispatchPlanWork(MoveTemp(Work));
	}
}

void UKMGoapPlannerSubsystem::DispatchPlanWork(FQueuedPlanWork&& Work)
{
	ActiveAsyncPlanCount++;

	TWeakObjectPtr<UKMGoapPlannerSubsystem> WeakThis(this);

	Async(EAsyncExecution::ThreadPool, [WeakThis, Work = MoveTemp(Work)]() mutable
	{
		FKMGoapPlanningSnapshotResult Result;
		FKMGoapPlanSearchSnapshot::BuildPlan(Work.Snapshot, Result);

		AsyncTask(ENamedThreads::GameThread, [WeakThis, Handle = Work.Handle, Result = MoveTemp(Result)]() mutable
		{
			if (UKMGoapPlannerSubsystem* Planner = WeakThis.Get())
			{
				Planner->CompletePlanRequest(Handle, MoveTemp(Result));
			}
		});
	});
}

bool UKMGoapPlannerSubsystem::BuildPlanningSnapshot(
	const FKMGoapPlanningRequest& Request,
	FKMGoapPlanningSnapshot& OutSnapshot,
	FPendingPlanRequest& OutPendingRequest) const
{
	OutSnapshot = FKMGoapPlanningSnapshot{};
	OutPendingRequest = FPendingPlanRequest{};

	UKMGoapAgentComponent* Agent = Request.Agent.Get();
	if (!Agent)
	{
		UE_LOG(LogGoapPlanner, Warning, TEXT("RequestPlanAsync failed: Agent is invalid."));
		return false;
	}

	Agent->UpdateBeliefEvaluationCache();

	OutPendingRequest.Agent = Agent;
	OutPendingRequest.OnPlanAcquired = Request.OnPlanAcquired;
	OutPendingRequest.OnPlanFailed = Request.OnPlanFailed;

	ApplyPlanningLimits(OutSnapshot);

	for (const FGameplayTag& FactTag : Agent->GetFactsTags())
	{
		const EKMGoapBeliefState FactState = Agent->GetFact(FactTag);
		if (FactState != EKMGoapBeliefState::Unknown)
		{
			OutSnapshot.InitialState.Set(FactTag, FactState == EKMGoapBeliefState::Positive);
		}
	}

	TArray<FGameplayTag> BeliefTags;
	Agent->BeliefsByTag.GetKeys(BeliefTags);
	for (const FGameplayTag& BeliefTag : BeliefTags)
	{
		const EKMGoapBeliefState BeliefState = Agent->EvaluateBeliefByTag(BeliefTag);
		if (BeliefState != EKMGoapBeliefState::Unknown)
		{
			OutSnapshot.InitialState.Set(BeliefTag, BeliefState == EKMGoapBeliefState::Positive);
		}
	}

	OutPendingRequest.RuntimeGoals.Reserve(Request.GoalsToCheck.Num());
	OutSnapshot.Goals.Reserve(Request.GoalsToCheck.Num());

	for (const TWeakObjectPtr<UKMGoapAgentGoal>& GoalPtr : Request.GoalsToCheck)
	{
		UKMGoapAgentGoal* Goal = GoalPtr.Get();
		if (!Goal)
		{
			continue;
		}

		const int32 RuntimeGoalIndex = OutPendingRequest.RuntimeGoals.Add(Goal);

		FKMGoapGoalSnapshot GoalSnapshot;
		GoalSnapshot.RuntimeGoalIndex = RuntimeGoalIndex;
		GoalSnapshot.Priority = Goal->GetPriority(Agent);
		GoalSnapshot.bIsMostRecentGoal = Goal == Request.LastGoal.Get();
		GoalSnapshot.DesiredEffects = Goal->DesiredEffects;

		bool bAnyUnsatisfied = false;
		for (const FKMGoapCondition& Condition : GoalSnapshot.DesiredEffects)
		{
			bool bValue = false;
			if (!OutSnapshot.InitialState.TryGet(Condition.Tag, bValue) || bValue != Condition.bValue)
			{
				bAnyUnsatisfied = true;
				break;
			}
		}

		if (bAnyUnsatisfied)
		{
			OutSnapshot.Goals.Add(MoveTemp(GoalSnapshot));
		}
	}

	OutPendingRequest.RuntimeActions.Reserve(Agent->ActionsByTag.Num());
	OutSnapshot.Actions.Reserve(Agent->ActionsByTag.Num());

	for (const TTuple<FGameplayTag, TObjectPtr<UKMGoapAgentAction>>& Pair : Agent->ActionsByTag)
	{
		UKMGoapAgentAction* Action = Pair.Value;
		if (!Action)
		{
			continue;
		}

		const int32 RuntimeActionIndex = OutPendingRequest.RuntimeActions.Add(Action);

		FKMGoapActionSnapshot ActionSnapshot;
		ActionSnapshot.RuntimeActionIndex = RuntimeActionIndex;
		ActionSnapshot.Cost = Action->GetDynamicCost(Agent);
		ActionSnapshot.Preconditions = Action->Preconditions;
		ActionSnapshot.Postconditions = Action->GetPostConditions();

		OutSnapshot.Actions.Add(MoveTemp(ActionSnapshot));
	}

	OutSnapshot.Actions.Sort([](const FKMGoapActionSnapshot& A, const FKMGoapActionSnapshot& B)
	{
		return A.Cost < B.Cost;
	});

	return !OutSnapshot.Goals.IsEmpty() && !OutSnapshot.Actions.IsEmpty();
}

void UKMGoapPlannerSubsystem::CompletePlanRequest(
	FKMGoapPlanningRequestHandle Handle,
	FKMGoapPlanningSnapshotResult Result)
{
	ActiveAsyncPlanCount = FMath::Max(0, ActiveAsyncPlanCount - 1);

	ON_SCOPE_EXIT
	{
		TryDispatchQueuedPlanWork();
	};

	if (!Handle.IsValid())
	{
		return;
	}

	FPendingPlanRequest PendingRequest;
	if (!PendingRequests.RemoveAndCopyValue(Handle.RequestId, PendingRequest))
	{
		return;
	}

	if (!Result.bSuccess)
	{
		if (PendingRequest.OnPlanFailed.IsBound())
		{
			PendingRequest.OnPlanFailed.Execute(Handle);
		}

		return;
	}

	// ... existing code ...
}
