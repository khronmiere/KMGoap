// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "KMGoapGoalSet.generated.h"

/**
 * 
 */
UCLASS(Category = "KMGoap|Data", BlueprintType, Blueprintable)
class KMGOAP_API UKMGoapGoalSet : public UDataAsset
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TArray<TSoftClassPtr<class UKMGoapAgentGoal>> Goals;
};
