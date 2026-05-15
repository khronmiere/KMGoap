// All rights reserved by Khrönmière Entertainment.
#include "Blueprint/Behavior/KMGoapKnowledgeRuntime.h"

#include "Blueprint/KMGoapAgentAction.h"
#include "Blueprint/KMGoapAgentBelief.h"
#include "Blueprint/KMGoapAgentGoal.h"
#include "Blueprint/Component/KMGoapAgentComponent.h"
#include "Blueprint/Component/KMGoapKnowledgeProviderComponent.h"
#include "Blueprint/Data/KMGoapActionSet.h"
#include "Blueprint/Data/KMGoapBeliefSet.h"
#include "Blueprint/Data/KMGoapGoalSet.h"
#include "Blueprint/Data/KMGoapKnowledgeModule.h"
#include "Interface/KMGoapSensorInterface.h"

DEFINE_LOG_CATEGORY_STATIC(LogGoapKnowledgeRuntime, Log, All);

bool UKMGoapKnowledgeRuntime::AddKnowledge(UKMGoapAgentComponent* Agent, UKMGoapKnowledgeModule* NewModule)
{
	if (!Agent || !NewModule)
	{
		return false;
	}
	
	if (KnowledgeSet.Contains(NewModule->KnowledgeTag))
	{
		return false;
	}

	UKMGoapKnowledgeModule* AddedModule = KnowledgeSet.Add(NewModule->KnowledgeTag, NewModule);
	InitializeModule(Agent, AddedModule);

	// New beliefs/actions/goals may invalidate the currently selected goal or action plan.
	Agent->ResetExecutionState();

	UE_LOG(LogGoapKnowledgeRuntime, Log, TEXT("Added new Module to Runtime. Module name: %s"), *NewModule->GetName());
	return true;
}

void UKMGoapKnowledgeRuntime::DeactivateKnowledgesWithTags(UKMGoapAgentComponent* Agent, const TArray<FGameplayTag>& Tags)
{
	if (Tags.IsEmpty())
	{
		return;
	}
	
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

	// Removing knowledge can invalidate cached belief results and any plan built from
	// module-provided actions or goals.
	Agent->UpdateBeliefEvaluationCache();
	Agent->ResetExecutionState();
}

void UKMGoapKnowledgeRuntime::InitializeModule(UKMGoapAgentComponent* Agent, UKMGoapKnowledgeModule* AddedModule)
{
	if (!Agent || !AddedModule)
	{
		UE_LOG(LogGoapKnowledgeRuntime, Warning,
			TEXT("InitializeModule failed due to invalid Agent or Module."));
		return;
	}

	TArray<FGameplayTag> BeliefTags;
	TArray<FGameplayTag> ActionTags;
	TArray<FGameplayTag> GoalsTags;

	if (AddedModule->BeliefSet)
	{
		BeliefTags = AddInstancesFromSet<UKMGoapAgentBelief>(
			Agent,
			AddedModule->BeliefSet->Beliefs,
			Agent->BeliefsByTag,
			[](const UKMGoapAgentBelief* B) { return B->BeliefTag; }
			);
	}
	else
	{
		UE_LOG(LogGoapKnowledgeRuntime, Warning,
			TEXT("Knowledge Module [%s] has no BeliefSet assigned."),
			*GetNameSafe(AddedModule));
	}

	if (AddedModule->ActionSet)
	{
		ActionTags = AddInstancesFromSet<UKMGoapAgentAction>(
			Agent,
			AddedModule->ActionSet->Actions,
			Agent->ActionsByTag,
			[](const UKMGoapAgentAction* A) { return A->ActionTag; }
			);
	}
	else
	{
		UE_LOG(LogGoapKnowledgeRuntime, Warning,
			TEXT("Knowledge Module [%s] has no ActionSet assigned."),
			*GetNameSafe(AddedModule));
	}

	if (AddedModule->GoalSet)
	{
		GoalsTags = AddInstancesFromSet<UKMGoapAgentGoal>(
			Agent,
			AddedModule->GoalSet->Goals,
			Agent->GoalsByTag,
			[](const UKMGoapAgentGoal* G) { return G->GoalTag; }
			);
	}
	else
	{
		UE_LOG(LogGoapKnowledgeRuntime, Warning,
			TEXT("Knowledge Module [%s] has no GoalSet assigned."),
			*GetNameSafe(AddedModule));
	}
	
	auto TagsGroup = FKMGoapInstancedModuleTags{BeliefTags, ActionTags, GoalsTags};
	TagGroupPerModule.Add(AddedModule, TagsGroup);

	// Evaluate immediately so deactivation checks and future planning use a coherent view
	// of the newly added knowledge.
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
			// A module without deactivation rules is considered persistent.
			continue;
		}

		// Deactivation rules are conjunctive: every configured belief state must match
		// before the module is removed from the agent.
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

void UKMGoapAgentComponent::TryLearnKnowledge(FGameplayTag SourceTag)
{
	UActorComponent* Sensor = GetSensorByTag(SourceTag);
	if (!Sensor)
	{
		UE_LOG(LogGoapKnowledgeRuntime, Warning,
			TEXT("TryLearnKnowledge failed: no sensor found for tag [%s]."),
			*SourceTag.ToString());
		return;
	}

	if (!Sensor->GetClass()->ImplementsInterface(UKMGoapSensorInterface::StaticClass()))
	{
		UE_LOG(LogGoapKnowledgeRuntime, Warning,
			TEXT("TryLearnKnowledge failed: component [%s] for tag [%s] does not implement KMGoapSensorInterface."),
			*GetNameSafe(Sensor),
			*SourceTag.ToString());
		return;
	}

	AActor* Target = IKMGoapSensorInterface::Execute_GetTarget(Sensor);
	if (!Target)
	{
		return;
	}

	// Knowledge discovery is sensor-driven: perceived actors can provide new beliefs,
	// actions, or goals to the agent at runtime.
	if (auto KnowledgeProvider = Target->GetComponentByClass<UKMGoapKnowledgeProviderComponent>())
	{
		KnowledgeProvider->Teach(this);
	}
}
