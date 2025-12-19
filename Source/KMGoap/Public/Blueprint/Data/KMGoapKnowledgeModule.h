// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Engine/DataAsset.h"
#include "KMGoapKnowledgeModule.generated.h"

class UKMGoapGoalSet;
class UKMGoapActionSet;
class UKMGoapBeliefSet;
/**
 * 
 */
UCLASS(Category = "KMGoap|Data", BlueprintType, Blueprintable)
class KMGOAP_API UKMGoapKnowledgeModule : public UDataAsset
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FGameplayTag KnowledgeTag;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UKMGoapBeliefSet> BeliefSet;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UKMGoapActionSet> ActionSet;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UKMGoapGoalSet> GoalSet;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TMap<FGameplayTag, bool> DeactivationRules;
};
