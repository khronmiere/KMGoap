// All rights reserved by Khrönmière Entertainment.
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Subsystem/Behavior/Concretions/KMGoapPlanSearch_Dijkstra.h"
#include "KMGoapPlannerConfig.generated.h"

/**
 * Data asset that defines the default configuration used by the GOAP planner.
 *
 * This asset controls which plan-search algorithm is instantiated at runtime
 * and provides shared tuning values for limiting planning cost, depth, and
 * execution time budget.
 */
UCLASS(BlueprintType, Blueprintable)
class KMGOAP_API UKMGoapPlannerConfig : public UDataAsset
{
	GENERATED_BODY()

public:
	/**
	 * Creates the planner configuration and assigns the default GOAP search algorithm.
	 */
	UKMGoapPlannerConfig()
	{
		SearchAlgorithmClass = UKMGoapPlanSearch_Dijkstra::StaticClass();
	}

	/**
	 * Search algorithm class instantiated by the planner subsystem at runtime.
	 *
	 * This should point to a class derived from UKMGoapPlanSearchBase.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="KMGoap|Planning")
	TSubclassOf<UKMGoapPlanSearchBase> SearchAlgorithmClass;

	/**
	 * Maximum number of planner nodes that may be expanded during a single plan search.
	 *
	 * Higher values allow more exhaustive searches but may increase planning cost.
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
	 * Maximum amount of time, in milliseconds, that the planner should spend searching.
	 *
	 * This budget is used to keep runtime planning responsive.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="KMGoap|Planning")
	float TimeBudgetMs = 2.0f;
};
