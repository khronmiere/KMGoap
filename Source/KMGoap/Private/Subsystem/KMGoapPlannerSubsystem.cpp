// All rights reserved by Khrönmière Entertainment.
#include "Subsystem/KMGoapPlannerSubsystem.h"
#include "Settings/KMGoapSettings.h"
#include "Settings/Data/KMGoapPlannerConfig.h"
#include "Subsystem/Behavior/KMGoapPlanSearchBase.h"


DEFINE_LOG_CATEGORY_STATIC(LogGoapPlanner, Log, All);

void UKMGoapPlannerSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	// The planner subsystem owns the configured search algorithm for the lifetime
	// of the game instance. Creating it during subsystem initialization keeps GOAP
	// agents independent from project settings and avoids each agent loading its own
	// planner configuration.
	CreateAlgorithmFromConfig();
}

void UKMGoapPlannerSubsystem::Deinitialize()
{
	// Release references before the subsystem shuts down so transient planner state
	// does not survive into the next world/game-instance lifecycle.
	SearchAlgorithm = nullptr;
	LoadedConfig = nullptr;

	Super::Deinitialize();
}

void UKMGoapPlannerSubsystem::CreateAlgorithmFromConfig()
{
	const UKMGoapSettings* Settings = GetDefault<UKMGoapSettings>();
	if (!Settings)
	{
		return;
	}

	// Planner configuration is a project-level soft reference so teams can swap
	// search strategies and limits from assets/settings without changing code.
	// Loading it here centralizes the synchronous load to subsystem startup instead
	// of paying that cost during an agent planning request.
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

	// The algorithm is a UObject instance rather than a static utility so individual
	// search implementations can expose tunable properties and keep any per-run helper
	// state behind the common UKMGoapPlanSearchBase interface.
	SearchAlgorithm = NewObject<UKMGoapPlanSearchBase>(this, AlgoClass);
	if (SearchAlgorithm)
	{
		// Copy limits from the asset into the runtime algorithm instance. This keeps
		// planner behavior deterministic for the lifetime of this subsystem, even if
		// the config asset is edited later in the editor.
		SearchAlgorithm->MaxExpandedNodes = LoadedConfig->MaxExpandedNodes;
		SearchAlgorithm->MaxDepth = LoadedConfig->MaxDepth;
		SearchAlgorithm->TimeBudgetMs = LoadedConfig->TimeBudgetMs;
	}
}
