// All rights reserved by Khrönmière Entertainment.
#include "Blueprint/Component/KMGoapKnowledgeProviderComponent.h"

#include "Blueprint/Component/KMGoapAgentComponent.h"

DEFINE_LOG_CATEGORY_STATIC(LogGoapKnowledgeProvider, Log, All);

// Sets default values for this component's properties
UKMGoapKnowledgeProviderComponent::UKMGoapKnowledgeProviderComponent()
{
	// Knowledge providers are passive. They only react when an agent explicitly
	// requests knowledge, so ticking would add cost without changing behavior.
	PrimaryComponentTick.bCanEverTick = false;
}

void UKMGoapKnowledgeProviderComponent::Teach(UKMGoapAgentComponent* Agent)
{
	if (!Agent)
	{
		UE_LOG(LogGoapKnowledgeProvider, Error, TEXT("No Agent provided to teach"))
		return;
	}

	// Each provider teaches a given agent only once. The agent's knowledge runtime
	// handles duplicate modules by tag, but this avoids repeated work and keeps the
	// provider's interaction semantics explicit.
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

	// Learned-agent state is runtime-only. Reset it on BeginPlay so editor reruns,
	// respawns, or reinstanced components do not retain stale teaching history.
	AgentsThatLearned.Reset();
}
