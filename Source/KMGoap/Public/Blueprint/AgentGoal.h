// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "UObject/Object.h"
#include "AgentGoal.generated.h"

class UAgentComponent;

/**
 * 
 */
UCLASS(Abstract, BlueprintType, Blueprintable, EditInlineNew, DefaultToInstanced, Category="KMGoap")
class KMGOAP_API UAgentGoal : public UObject
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
	float GetPriority(UAgentComponent* Agent) const { return Native_GetPriority(Agent); }

protected:
	virtual float Native_GetPriority(UAgentComponent* Agent) const;
	UFUNCTION(BlueprintNativeEvent, Category="Goal", meta=(BlueprintProtected="true"))
	float Priority(UAgentComponent* Agent) const;
	virtual float Priority_Implementation(UAgentComponent* Agent) const;
};
