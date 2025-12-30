// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Subsystem/Behavior/Concretions/KMGoapPlanSearch_Dijkstra.h"
#include "KMGoapPlannerConfig.generated.h"

/**
 * 
 */
UCLASS(BlueprintType, Blueprintable)
class KMGOAP_API UKMGoapPlannerConfig : public UDataAsset
{
	GENERATED_BODY()
	
public:
	UKMGoapPlannerConfig()
	{
		SearchAlgorithmClass = UKMGoapPlanSearch_Dijkstra::StaticClass();
	}
	
	// Which algorithm to instantiate at runtime.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="KMGoap|Planning")
	TSubclassOf<UKMGoapPlanSearchBase> SearchAlgorithmClass;

	// Optional: default instance tuning (applied after creation).
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="KMGoap|Planning")
	int32 MaxExpandedNodes = 5000;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="KMGoap|Planning")
	int32 MaxDepth = 64;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="KMGoap|Planning")
	float TimeBudgetMs = 2.0f;
};
