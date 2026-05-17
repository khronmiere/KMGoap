// All rights reserved by Khrönmière Entertainment.
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "KMGoapPlannerConfig.generated.h"

/**
 * Data asset that defines runtime limits used by the asynchronous GOAP planner.
 *
 * The async planner snapshots UObject data on the game thread, runs the expensive
 * search over value-only data on a worker thread, and applies the result back on
 * the game thread through request callbacks.
 */
UCLASS(BlueprintType, Blueprintable)
class KMGOAP_API UKMGoapPlannerConfig : public UDataAsset
{
	GENERATED_BODY()

public:
	/**
	 * Maximum number of GOAP planning searches allowed to run concurrently.
	 *
	 * Requests above this limit remain queued on the game thread until a worker slot
	 * becomes available. This prevents large AI populations from flooding UE's global
	 * thread pool when many agents replan in the same frame.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="KMGoap|Planning", meta=(ClampMin="1"))
	int32 MaxConcurrentAsyncPlans = 2;

	/**
	 * Maximum number of pending GOAP planning requests allowed in the subsystem queue.
	 *
	 * A value of 0 means the queue is unbounded. Keeping this configurable allows large
	 * projects to apply back-pressure if global events can invalidate many agents at once.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="KMGoap|Planning", meta=(ClampMin="0"))
	int32 MaxQueuedAsyncPlans = 0;

	/**
	 * Maximum number of planner nodes that may be expanded during a single plan search.
	 *
	 * Higher values allow more exhaustive searches but may increase worker-thread cost.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="KMGoap|Planning")
	int32 MaxExpandedNodes = 5000;

	/**
	 * Maximum number of actions allowed in a generated plan.
	 *
	 * This prevents the planner from exploring excessively long action chains.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="KMGoap|Planning")
	int32 MaxDepth = 64;

	/**
	 * Maximum amount of time, in milliseconds, that the async planner should spend searching.
	 *
	 * This budget is evaluated on the worker thread and prevents expensive searches from
	 * monopolizing thread-pool resources.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="KMGoap|Planning")
	float TimeBudgetMs = 2.0f;
};
