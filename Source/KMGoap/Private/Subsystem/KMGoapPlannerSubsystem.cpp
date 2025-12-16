#include "Subsystem/KMGoapPlannerSubsystem.h"
#include "Blueprint/KMGoapAgentGoal.h"
#include "Settings/KMGoapSettings.h"
#include "Settings/Data/KMGoapPlannerConfig.h"
#include "Subsystem/Behavior/KMGoapPlanSearchBase.h"


DEFINE_LOG_CATEGORY_STATIC(LogGoapPlanner, Log, All);

void UKMGoapPlannerSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	CreateAlgorithmFromConfig();
}

void UKMGoapPlannerSubsystem::Deinitialize()
{
	SearchAlgorithm = nullptr;
	LoadedConfig = nullptr;
	Super::Deinitialize();
}

bool UKMGoapPlannerSubsystem::Plan_Implementation(
	UKMGoapAgentComponent* Agent,
	const TArray<UKMGoapAgentGoal*>& GoalsToCheck,
	UKMGoapAgentGoal* MostRecentGoal,
	FKMGoapActionPlan& OutPlan)
{
	if (!SearchAlgorithm)
	{
		return false;
	}
	
	return IKMGoapPlanSearchInterface::Execute_BuildPlan(
		SearchAlgorithm,
		Agent,
		GoalsToCheck,
		MostRecentGoal,
		OutPlan);
}

void UKMGoapPlannerSubsystem::CreateAlgorithmFromConfig()
{
	const UKMGoapSettings* Settings = GetDefault<UKMGoapSettings>();
	if (!Settings)
	{
		return;
	}

	LoadedConfig = Settings->PlannerConfig.LoadSynchronous();
	if (!LoadedConfig)
	{
		UE_LOG(LogGoapPlanner, Warning, TEXT("KMGoap: PlannerConfig not set or failed to load."));
		return;
	}

	TSubclassOf<UKMGoapPlanSearchBase> AlgoClass = LoadedConfig->SearchAlgorithmClass;
	if (!AlgoClass)
	{
		UE_LOG(LogGoapPlanner, Warning, TEXT("KMGoap: SearchAlgorithmClass not set in PlannerConfig."));
		return;
	}

	SearchAlgorithm = NewObject<UKMGoapPlanSearchBase>(this, AlgoClass);
	if (SearchAlgorithm)
	{
		SearchAlgorithm->MaxExpandedNodes = LoadedConfig->MaxExpandedNodes;
		SearchAlgorithm->MaxDepth = LoadedConfig->MaxDepth;
		SearchAlgorithm->TimeBudgetMs = LoadedConfig->TimeBudgetMs;
	}
}
