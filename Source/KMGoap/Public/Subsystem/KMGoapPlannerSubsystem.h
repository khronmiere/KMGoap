#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "KMGoapPlannerSubsystem.generated.h"

class UKMGoapPlannerConfig;
class UKMGoapPlanSearchBase;

/**
 * UGoapPlannerSubsystem
 *
 * Stateless GOAP planner service.
 * Computes a plan for an agent given a set of candidate goals.
 */
UCLASS(Category="KMGoap")
class KMGOAP_API UKMGoapPlannerSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="KMGoap|Planner")
	UKMGoapPlanSearchBase* GetSearchAlgorithm() {return SearchAlgorithm;}

private:
	UPROPERTY(Transient)
	TObjectPtr<UKMGoapPlanSearchBase> SearchAlgorithm = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UKMGoapPlannerConfig> LoadedConfig = nullptr;

	void CreateAlgorithmFromConfig();
};
