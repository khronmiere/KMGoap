// Fill out your copyright notice in the Description page of Project Settings.


#include "Blueprint/Behavior/KMGoapKnowledgeRuntime.h"

#include "Blueprint/Component/KMGoapAgentComponent.h"
#include "Blueprint/Data/KMGoapKnowledgeModule.h"

DEFINE_LOG_CATEGORY_STATIC(LogGoapKnowledgeRuntime, Log, All);

bool UKMGoapKnowledgeRuntime::AddKnowledge(const UKMGoapAgentComponent* Agent, UKMGoapKnowledgeModule* NewModule)
{
	if (!NewModule)
	{
		return false;
	}
	
	if (KnowledgeSet.Contains(NewModule->KnowledgeTag))
	{
		return false;
	}
	KnowledgeSet.Add(NewModule->KnowledgeTag, NewModule);
	Agent->ResetExecutionState();
	UE_LOG(LogGoapKnowledgeRuntime, Log, TEXT("Added new Module to Runtime. Module name: %s"), *NewModule->GetName());
	return true;
}

void UKMGoapKnowledgeRuntime::DeactivateKnowledgesWithTags(const UKMGoapAgentComponent* Agent, const TArray<FGameplayTag>& Tags)
{
	for (const FGameplayTag& Tag : Tags)
	{
		UE_LOG(LogGoapKnowledgeRuntime, Log, TEXT("Deactivating a Module from Runtime. Module name: %s"), *KnowledgeSet[Tag]->GetName());
		KnowledgeSet.Remove(Tag);
		Agent->ResetExecutionState();
	}
}

void UKMGoapKnowledgeRuntime::EvaluateKnowledgeModulesDeactivationRules(
	const UKMGoapAgentComponent* Agent,
	TArray<FGameplayTag>& ToRemove) const
{
	ToRemove.Reset();
	for (const TTuple<FGameplayTag, UKMGoapKnowledgeModule*>& Set : KnowledgeSet)
	{
		UKMGoapKnowledgeModule* Module = Set.Value;
		bool bShouldRemove = true;
		for (const TTuple<FGameplayTag, bool>& DeactivationRule : Module->DeactivationRules)
		{
			bool bCurrentValue = Agent->EvaluateBeliefByTag(DeactivationRule.Key);
			if (bCurrentValue != DeactivationRule.Value)
			{
				bShouldRemove = false;
				break;
			}
		}
		if (bShouldRemove)
		{
			ToRemove.Add(Set.Key);
		}
	}
}

void UKMGoapKnowledgeRuntime::Tick(UKMGoapAgentComponent* Agent)
{
	TArray<FGameplayTag> ToRemove;
	EvaluateKnowledgeModulesDeactivationRules(Agent, ToRemove);
	DeactivateKnowledgesWithTags(Agent, ToRemove);
}
