// All rights reserved by Khrönmière Entertainment.
#pragma once

#include "CoreMinimal.h"
#include "Data/KMGoapActionPlan.h"
#include "KMGoapPlanningRequest.generated.h"

class UKMGoapAgentComponent;
class UKMGoapAgentGoal;

/**
 * Lightweight handle returned when an asynchronous GOAP planning request is queued.
 *
 * Handles are value objects and can be stored by state machines to detect stale
 * results, cancel pending work, or correlate callbacks with a specific request.
 */
USTRUCT(BlueprintType)
struct KMGOAP_API FKMGoapPlanningRequestHandle
{
	GENERATED_BODY()

	/** Unique identifier for the planner request. */
	UPROPERTY(BlueprintReadOnly)
	FGuid RequestId;

	/**
	 * Checks whether this handle represents a queued or previously queued request.
	 *
	 * @return True when the handle contains a valid request id.
	 */
	bool IsValid() const
	{
		return RequestId.IsValid();
	}

	/** Resets this handle to an invalid state. */
	void Reset()
	{
		RequestId.Invalidate();
	}

	bool operator==(const FKMGoapPlanningRequestHandle& Other) const
	{
		return RequestId == Other.RequestId;
	}
};

/**
 * Callback executed on the game thread when an async plan is acquired.
 */
DECLARE_DELEGATE_TwoParams(FKMGoapOnPlanAcquired, const FKMGoapPlanningRequestHandle& /*Handle*/, FKMGoapActionPlan&& /*Plan*/);

/**
 * Callback executed on the game thread when async planning fails or is cancelled.
 */
DECLARE_DELEGATE_OneParam(FKMGoapOnPlanFailed, const FKMGoapPlanningRequestHandle& /*Handle*/);

/**
 * Game-thread request data supplied to the planner subsystem.
 *
 * The subsystem converts this UObject-facing request into an immutable value snapshot
 * before sending work to a background thread.
 */
struct KMGOAP_API FKMGoapPlanningRequest
{
	/** Agent requesting the plan. Stored weakly so destroyed agents do not stay alive. */
	TWeakObjectPtr<UKMGoapAgentComponent> Agent;

	/** Candidate goals to evaluate. Read only on the game thread while building the snapshot. */
	TArray<TWeakObjectPtr<UKMGoapAgentGoal>> GoalsToCheck;

	/** Most recently completed/selected goal, used for goal-continuity ordering. */
	TWeakObjectPtr<UKMGoapAgentGoal> LastGoal;

	/** Success callback invoked on the game thread. */
	FKMGoapOnPlanAcquired OnPlanAcquired;

	/** Failure callback invoked on the game thread. */
	FKMGoapOnPlanFailed OnPlanFailed;
};
