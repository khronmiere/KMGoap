// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "KMGoapKnowledgeRuntime.generated.h"

struct FGameplayTag;
class UKMGoapAgentComponent;
class UKMGoapKnowledgeModule;
/**
 * 
 */
UCLASS(BlueprintType, Blueprintable)
class KMGOAP_API UKMGoapKnowledgeRuntime : public UObject
{
	GENERATED_BODY()
	
public:
	UFUNCTION(BlueprintCallable)
	bool AddKnowledge(const UKMGoapAgentComponent* Agent, UKMGoapKnowledgeModule* NewModule);

	UFUNCTION(BlueprintCallable)
	void Tick(UKMGoapAgentComponent* Agent);
	
protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TMap<FGameplayTag, UKMGoapKnowledgeModule*> KnowledgeSet;
	
	void EvaluateKnowledgeModulesDeactivationRules(const UKMGoapAgentComponent* Agent, TArray<FGameplayTag>& ToRemove) const;
	void DeactivateKnowledgesWithTags(const UKMGoapAgentComponent* Agent, const TArray<FGameplayTag>& Tags);
};
