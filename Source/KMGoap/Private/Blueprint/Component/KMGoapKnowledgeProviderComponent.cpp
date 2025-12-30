// Fill out your copyright notice in the Description page of Project Settings.


#include "Blueprint/Component/KMGoapKnowledgeProviderComponent.h"

#include "Blueprint/Component/KMGoapAgentComponent.h"

DEFINE_LOG_CATEGORY_STATIC(LogGoapKnowledgeProvider, Log, All);

// Sets default values for this component's properties
UKMGoapKnowledgeProviderComponent::UKMGoapKnowledgeProviderComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = false;

	// ...
}

void UKMGoapKnowledgeProviderComponent::Teach(UKMGoapAgentComponent* Agent)
{
	if (!Agent)
	{
		UE_LOG(LogGoapKnowledgeProvider, Error, TEXT("No Agent provided to teach"))
		return;
	}
	
	if (AgentsThatLearned.Contains(Agent))
	{
		return;
	}
	
	for (const TObjectPtr<UKMGoapKnowledgeModule>& KnowledgeModule : ModulesToProvide)
	{
		Agent->AddNewKnowledgeModule(KnowledgeModule);
	}
	
	AgentsThatLearned.Add(Agent);
}

void UKMGoapKnowledgeProviderComponent::BeginPlay()
{
	Super::BeginPlay();
	AgentsThatLearned.Reset();
}
