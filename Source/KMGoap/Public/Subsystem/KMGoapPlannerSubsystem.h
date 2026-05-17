// All rights reserved by Khrönmière Entertainment.
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Subsystem/Data/KMGoapPlanningRequest.h"
#include "Subsystem/Data/KMGoapPlanningTypes.h"
#include "KMGoapPlannerSubsystem.generated.h"

class UKMGoapAgentComponent;
class UKMGoapAgentGoal;
class UKMGoapAgentAction;
class UKMGoapPlannerConfig;

/**
 * Game-instance subsystem that owns asynchronous GOAP planning requests.
 *
 * The subsystem loads planner limits from project settings, snapshots UObject-facing
 * agent data on the game thread, runs value-only search work on UE worker threads,
 * and dispatches completion callbacks back on the game thread.
 */
UCLASS(Category="KMGoap")
class KMGOAP_API UKMGoapPlannerSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	/**
	 * Initializes the planner subsystem for the owning game instance.
	 *
	 * Loads planner configuration from project settings.
	 *
	 * @param Collection Subsystem dependency collection provided by Unreal Engine.
	 */
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	/**
	 * Deinitializes the planner subsystem.
	 *
	 * Cancels pending request bookkeeping and releases loaded configuration references.
	 */
	virtual void Deinitialize() override;

	/**
	 * Queues an asynchronous GOAP planning request.
	 *
	 * Request data is snapshotted on the game thread, searched on a worker thread,
	 * and completed back on the game thread through the supplied callbacks.
	 *
	 * @param Request UObject-facing request data and callbacks.
	 * @return Handle that can be stored by the caller for tracking or cancellation.
	 */
	FKMGoapPlanningRequestHandle RequestPlanAsync(FKMGoapPlanningRequest&& Request);

	/**
	 * Cancels a pending async planning request.
	 *
	 * Cancellation prevents callbacks from firing. Already-running worker work may
	 * still finish, but its result will be ignored on the game thread.
	 *
	 * @param Handle Request handle returned by RequestPlanAsync.
	 */
	void CancelPlanRequest(const FKMGoapPlanningRequestHandle& Handle);

private:
	/**
	 * Planner configuration asset loaded from KMGoap project settings.
	 *
	 * Kept referenced for the lifetime of the subsystem so its configured search
	 * class and runtime limits remain available after synchronous loading.
	 */
	UPROPERTY(Transient)
	TObjectPtr<UKMGoapPlannerConfig> LoadedConfig = nullptr;

	/**
	 * Runtime data retained while an async planning request is pending.
	 *
	 * UObject references are weak so pending planner work does not keep gameplay
	 * objects alive past their intended lifetime.
	 */
	struct FPendingPlanRequest
	{
		TWeakObjectPtr<UKMGoapAgentComponent> Agent;
		TArray<TWeakObjectPtr<UKMGoapAgentGoal>> RuntimeGoals;
		TArray<TWeakObjectPtr<UKMGoapAgentAction>> RuntimeActions;
		FKMGoapOnPlanAcquired OnPlanAcquired;
		FKMGoapOnPlanFailed OnPlanFailed;
	};

	/** Pending async planning requests indexed by handle id. */
	TMap<FGuid, FPendingPlanRequest> PendingRequests;

	/**
	 * Loads planner configuration from project settings.
	 */
	void LoadPlannerConfig();

	/**
	 * Applies configured search limits to a planning snapshot.
	 *
	 * @param OutSnapshot Snapshot that receives async planner limits.
	 */
	void ApplyPlanningLimits(FKMGoapPlanningSnapshot& OutSnapshot) const;

	/**
	 * Builds a thread-safe snapshot from a game-thread planning request.
	 *
	 * This is the only step that may read UObject planner data for async planning.
	 */
	bool BuildPlanningSnapshot(
		const FKMGoapPlanningRequest& Request,
		FKMGoapPlanningSnapshot& OutSnapshot,
		FPendingPlanRequest& OutPendingRequest) const;

	/**
	 * Completes an async planning request on the game thread.
	 */
	void CompletePlanRequest(
		FKMGoapPlanningRequestHandle Handle,
		FKMGoapPlanningSnapshotResult Result);
};
