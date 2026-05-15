// All rights reserved by Khrönmière Entertainment.
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "KMGoapPlannerSubsystem.generated.h"

class UKMGoapPlannerConfig;
class UKMGoapPlanSearchBase;

/**
 * Game-instance subsystem that owns the runtime GOAP planner search algorithm.
 *
 * The subsystem loads the project planner configuration, creates the configured
 * search algorithm object, and exposes it to GOAP agents that need to compute
 * action plans.
 */
UCLASS(Category="KMGoap")
class KMGOAP_API UKMGoapPlannerSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	/**
	 * Initializes the planner subsystem for the owning game instance.
	 *
	 * Loads the planner configuration from project settings and creates the
	 * configured search algorithm instance.
	 *
	 * @param Collection Subsystem dependency collection provided by Unreal Engine.
	 */
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	/**
	 * Deinitializes the planner subsystem.
	 *
	 * Releases references to the created search algorithm and loaded planner
	 * configuration before allowing the base subsystem to shut down.
	 */
	virtual void Deinitialize() override;

	/**
	 * Returns the currently configured GOAP plan-search algorithm instance.
	 *
	 * @return Runtime search algorithm object, or nullptr if configuration loading
	 * failed or no algorithm class was configured.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="KMGoap|Planner")
	UKMGoapPlanSearchBase* GetSearchAlgorithm() { return SearchAlgorithm; }

private:
	/**
	 * Runtime instance of the configured GOAP search algorithm.
	 *
	 * This object is created from the planner configuration during subsystem
	 * initialization and reused by agents when requesting plans.
	 */
	UPROPERTY(Transient)
	TObjectPtr<UKMGoapPlanSearchBase> SearchAlgorithm = nullptr;

	/**
	 * Planner configuration asset loaded from KMGoap project settings.
	 *
	 * Kept referenced for the lifetime of the subsystem so its configured search
	 * class and runtime limits remain available after synchronous loading.
	 */
	UPROPERTY(Transient)
	TObjectPtr<UKMGoapPlannerConfig> LoadedConfig = nullptr;

	/**
	 * Creates and configures the runtime search algorithm from project settings.
	 *
	 * Loads the configured planner asset, validates the selected algorithm class,
	 * instantiates the algorithm, and copies runtime planning limits onto it.
	 */
	void CreateAlgorithmFromConfig();
};
