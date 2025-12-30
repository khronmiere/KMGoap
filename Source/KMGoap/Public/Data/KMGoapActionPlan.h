// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "KMGoapActionPlan.generated.h"

class UKMGoapAgentGoal;
class UKMGoapAgentAction;

/**
 * 
 */
USTRUCT(BlueprintType)
struct FKMGoapActionPlan
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<UKMGoapAgentGoal> Goal = nullptr;

	UPROPERTY(BlueprintReadOnly)
	TArray<TObjectPtr<UKMGoapAgentAction>> Actions;
	
	UPROPERTY(BlueprintReadOnly)
	float TotalCost = 0.f;
	
	bool IsValid() const { return Goal != nullptr && Actions.Num() > 0; }
	void Reset() { Goal = nullptr; Actions.Reset(); TotalCost = 0.f; }
};
