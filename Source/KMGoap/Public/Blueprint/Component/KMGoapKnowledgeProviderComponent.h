// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "KMGoapKnowledgeProviderComponent.generated.h"

class UKMGoapAgentComponent;
class UKMGoapKnowledgeModule;

UCLASS(ClassGroup=(KMGoap), BlueprintType, Blueprintable, Category = "KMGoap|ActorComponents", meta=(BlueprintSpawnableComponent))
class KMGOAP_API UKMGoapKnowledgeProviderComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	UKMGoapKnowledgeProviderComponent();
	
	void Teach(UKMGoapAgentComponent* Agent);

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TArray<TObjectPtr<UKMGoapKnowledgeModule>> ModulesToProvide;
	
	UPROPERTY(Transient)
	TSet<UKMGoapAgentComponent*> AgentsThatLearned;
	
	virtual void BeginPlay() override;
};
