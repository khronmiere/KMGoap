// All rights reserved by Khrönmière Entertainment.
#pragma once

#include "CoreMinimal.h"
#include "Data/KMGoapPlannerConfig.h"
#include "Engine/DeveloperSettings.h"
#include "KMGoapSettings.generated.h"

/**
 * Project-wide developer settings for the KMGoap plugin.
 *
 * These settings are stored in the game configuration and expose default GOAP
 * configuration assets used by runtime systems such as the planner subsystem.
 */
UCLASS(Config=Game, DefaultConfig, meta=(DisplayName="KMGoap"))
class KMGOAP_API UKMGoapSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	/**
	 * Soft reference to the planner configuration asset used by the async GOAP planner.
	 *
	 * The configuration supplies runtime search limits such as node budget, maximum
	 * depth, and worker-side time budget.
	 */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category="Planning",
		meta=(AllowedClasses="/Script/KMGoap.KMGoapPlannerConfig"))
	TSoftObjectPtr<UKMGoapPlannerConfig> PlannerConfig;
};
