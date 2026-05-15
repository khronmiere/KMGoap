// All rights reserved by Khrönmière Entertainment.
#pragma once

#include "CoreMinimal.h"
#include "Interface/KMGoapPlanSearchInterface.h"
#include "UObject/Object.h"
#include "KMGoapPlanSearchBase.generated.h"

/**
 * Base class for GOAP plan-search implementations.
 *
 * This class provides shared runtime limits used by concrete planning algorithms,
 * such as maximum expanded node count, maximum plan depth, and per-search time
 * budget. Derived classes implement the actual plan-building behavior through
 * the KMGoap plan-search interface.
 */
UCLASS(Abstract, Blueprintable, EditInlineNew, DefaultToInstanced)
class KMGOAP_API UKMGoapPlanSearchBase : public UObject, public IKMGoapPlanSearchInterface
{
	GENERATED_BODY()

public:
	/**
	 * Maximum number of planner nodes that may be expanded during a single search.
	 *
	 * Search implementations should stop planning once this limit is reached to
	 * prevent expensive or unbounded searches.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="KMGoap|Planning")
	int32 MaxExpandedNodes = 5000;

	/**
	 * Maximum depth allowed for a generated action plan.
	 *
	 * This prevents the planner from exploring action chains that are too long to
	 * be practical for runtime execution.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="KMGoap|Planning")
	int32 MaxDepth = 64;

	/**
	 * Maximum amount of wall-clock time, in milliseconds, allowed for a search.
	 *
	 * This budget helps keep planning responsive when running during gameplay.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="KMGoap|Planning")
	float TimeBudgetMs = 2.0f;

protected:
	/**
	 * Checks whether the active plan search has exceeded its configured limits.
	 *
	 * The budget is considered exceeded when either the expanded node count reaches
	 * MaxExpandedNodes or the elapsed time since StartSeconds reaches TimeBudgetMs.
	 *
	 * @param StartSeconds Time value captured from FPlatformTime::Seconds() when the search started.
	 * @param ExpandedNodes Number of planner nodes expanded so far by the active search.
	 * @return True if the search should stop because a node or time budget was reached; otherwise false.
	 */
	UFUNCTION(Blueprintable, BlueprintPure, Category="KMGoap|Planning")
	bool IsBudgetExceeded(const double StartSeconds, int32 ExpandedNodes) const
	{
		if (ExpandedNodes >= MaxExpandedNodes) return true;
		const double ElapsedMs = (FPlatformTime::Seconds() - StartSeconds) * 1000.0;
		return ElapsedMs >= TimeBudgetMs;
	}
};
