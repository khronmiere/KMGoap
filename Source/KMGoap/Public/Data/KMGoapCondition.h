// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "UObject/Object.h"
#include "KMGoapCondition.generated.h"

/**
 * 
 */
USTRUCT(BlueprintType)
struct KMGOAP_API FKMGoapCondition
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGameplayTag Tag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bValue = true;
	
	bool operator==(const FKMGoapCondition& Other) const
	{
		return Tag == Other.Tag && bValue == Other.bValue;
	}
};

FORCEINLINE uint32 GetTypeHash(const FKMGoapCondition& Condition)
{
	return GetTypeHash(Condition.Tag);
}
