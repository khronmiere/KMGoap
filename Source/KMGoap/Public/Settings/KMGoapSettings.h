// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Data/KMGoapPlannerConfig.h"
#include "Engine/DeveloperSettings.h"
#include "KMGoapSettings.generated.h"

UCLASS(Config=Game, DefaultConfig, meta=(DisplayName="KMGoap"))
class KMGOAP_API UKMGoapSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	// Path to a planner config asset.
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category="Planning",
		meta=(AllowedClasses="/Script/KMGoap.KMGoapPlannerConfig"))
	TSoftObjectPtr<UKMGoapPlannerConfig> PlannerConfig;
};
