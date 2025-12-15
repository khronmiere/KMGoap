// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "UObject/Object.h"
#include "KMGoapAgentGoal.generated.h"

class UKMGoapAgentComponent;

/**
 * 
 */
UCLASS(Abstract, BlueprintType, Blueprintable, EditInlineNew, DefaultToInstanced, Category="KMGoap")
class KMGOAP_API UKMGoapAgentGoal : public UObject
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Goal")
	FGameplayTag GoalTag;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Goal", meta=(ClampMin="0.0"))
	float BasePriority = 1.f;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Goal")
	FGameplayTagContainer DesiredEffects;
	
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Goal")
	float GetPriority(UKMGoapAgentComponent* Agent) const { return Native_GetPriority(Agent); }

protected:
	virtual float Native_GetPriority(UKMGoapAgentComponent* Agent) const;
	UFUNCTION(BlueprintNativeEvent, Category="Goal", meta=(BlueprintProtected="true"))
	float Priority(UKMGoapAgentComponent* Agent) const;
	virtual float Priority_Implementation(UKMGoapAgentComponent* Agent) const;
};
