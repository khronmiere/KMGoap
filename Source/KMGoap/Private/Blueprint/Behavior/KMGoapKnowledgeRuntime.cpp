// Fill out your copyright notice in the Description page of Project Settings.


#include "Blueprint/Behavior/KMGoapKnowledgeRuntime.h"

#include "Blueprint/KMGoapAgentAction.h"
#include "Blueprint/KMGoapAgentBelief.h"
#include "Blueprint/KMGoapAgentGoal.h"
#include "Blueprint/Component/KMGoapAgentComponent.h"
#include "Blueprint/Data/KMGoapActionSet.h"
#include "Blueprint/Data/KMGoapBeliefSet.h"
#include "Blueprint/Data/KMGoapGoalSet.h"
#include "Blueprint/Data/KMGoapKnowledgeModule.h"

DEFINE_LOG_CATEGORY_STATIC(LogGoapKnowledgeRuntime, Log, All);

bool UKMGoapKnowledgeRuntime::AddKnowledge(UKMGoapAgentComponent* Agent, UKMGoapKnowledgeModule* NewModule)
{
	if (!NewModule)
	{
		return false;
	}
	
	if (KnowledgeSet.Contains(NewModule->KnowledgeTag))
	{
		return false;
	}
	UKMGoapKnowledgeModule* AddedModule = KnowledgeSet.Add(NewModule->KnowledgeTag, NewModule);
	InitializeModule(Agent, AddedModule);
	Agent->ResetExecutionState();
	UE_LOG(LogGoapKnowledgeRuntime, Log, TEXT("Added new Module to Runtime. Module name: %s"), *NewModule->GetName());
	return true;
}

void UKMGoapKnowledgeRuntime::DeactivateKnowledgesWithTags(UKMGoapAgentComponent* Agent, const TArray<FGameplayTag>& Tags)
{
	for (const FGameplayTag& Tag : Tags)
	{
		UKMGoapKnowledgeModule* Module = KnowledgeSet[Tag];
		UE_LOG(LogGoapKnowledgeRuntime, Log,
			TEXT("Deactivating a Module from Runtime. Module name: %s"),
			*Module->GetName());
		
		{
			const FKMGoapInstancedModuleTags& TagGroup = TagGroupPerModule[Module];
			RemoveInstancesByTag(TagGroup.BeliefTags, Agent->BeliefsByTag);
			RemoveInstancesByTag(TagGroup.ActionTags, Agent->ActionsByTag);
			RemoveInstancesByTag(TagGroup.GoalTags, Agent->GoalsByTag);
		}
		
		KnowledgeSet.Remove(Tag);
	}
	Agent->UpdateBeliefEvaluationCache();
	Agent->ResetExecutionState();
}

void UKMGoapKnowledgeRuntime::InitializeModule(UKMGoapAgentComponent* Agent, UKMGoapKnowledgeModule* AddedModule)
{
	auto BeliefTags = AddInstancesFromSet<UKMGoapAgentBelief>(Agent,
		AddedModule->BeliefSet->Beliefs,
		Agent->BeliefsByTag,
		[](const UKMGoapAgentBelief* B) { return B->BeliefTag; }
		);

	auto ActionTags = AddInstancesFromSet<UKMGoapAgentAction>(
		Agent,
		AddedModule->ActionSet->Actions,
		Agent->ActionsByTag,
		[](const UKMGoapAgentAction* A) { return A->ActionTag; }
		);

	auto GoalsTags = AddInstancesFromSet<UKMGoapAgentGoal>(
		Agent,
		AddedModule->GoalSet->Goals,
		Agent->GoalsByTag,
		[](const UKMGoapAgentGoal* G) { return G->GoalTag; }
		);
	
	auto TagsGroup = FKMGoapInstancedModuleTags{BeliefTags, ActionTags, GoalsTags};
	TagGroupPerModule.Add(AddedModule, TagsGroup);
	Agent->UpdateBeliefEvaluationCache();
	Agent->ResetExecutionState();
}

void UKMGoapKnowledgeRuntime::EvaluateKnowledgeModulesDeactivationRules(
	const UKMGoapAgentComponent* Agent,
	TArray<FGameplayTag>& ToRemove) const
{
	ToRemove.Reset();
	for (const TTuple<FGameplayTag, UKMGoapKnowledgeModule*>& Set : KnowledgeSet)
	{
		UKMGoapKnowledgeModule* Module = Set.Value;
		const auto& DeactivationRules = Module->DeactivationRules;
		if (DeactivationRules.IsEmpty())
		{
			// we do not evaluate deactivation of a module that has no rules
			// no rules = never deactivate
			continue;
		}
		
		bool bShouldRemove = true;
		for (const TTuple<FGameplayTag, bool>& DeactivationRule : DeactivationRules)
		{
			EKMGoapBeliefState CurrentValue = Agent->EvaluateBeliefByTag(DeactivationRule.Key);
			EKMGoapBeliefState ExpectedState = DeactivationRule.Value ? EKMGoapBeliefState::Positive : EKMGoapBeliefState::Negative;
			if (CurrentValue != ExpectedState)
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
