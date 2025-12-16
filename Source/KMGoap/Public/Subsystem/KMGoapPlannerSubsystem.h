#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "GameplayTagContainer.h"
#include "Interface/KMGoapPlannerInterface.h"
#include "KMGoapPlannerSubsystem.generated.h"

struct FKMGoapCondition;
class UKMGoapAgentGoal;
class UKMGoapAgentAction;
class UKMGoapAgentComponent;

/**
 * UGoapPlannerSubsystem
 *
 * Stateless GOAP planner service.
 * Computes a plan for an agent given a set of candidate goals.
 */
UCLASS(Category="KMGoap")
class KMGOAP_API UKMGoapPlannerSubsystem : public UGameInstanceSubsystem, public IKMGoapPlannerInterface
{
	GENERATED_BODY()

public:
	
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	
	virtual bool Plan_Implementation(
		UKMGoapAgentComponent* Agent,
		const TArray<UKMGoapAgentGoal*>& GoalsToCheck,
		UKMGoapAgentGoal* MostRecentGoal,
		FKMGoapActionPlan& OutPlan) override;

private:
	UPROPERTY(Transient)
	TObjectPtr<UKMGoapPlanSearchBase> SearchAlgorithm = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UKMGoapPlannerConfig> LoadedConfig = nullptr;

	void CreateAlgorithmFromConfig();
};
