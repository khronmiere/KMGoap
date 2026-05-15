// All rights reserved by Khrönmière Entertainment.
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "KMGoapKnowledgeProviderComponent.generated.h"

class UKMGoapAgentComponent;
class UKMGoapKnowledgeModule;

/**
 * Actor component that provides GOAP knowledge modules to agents.
 *
 * Attach this component to actors that can teach nearby or sensed GOAP agents
 * additional knowledge at runtime. Each agent is taught only once by this
 * provider instance.
 */
UCLASS(ClassGroup=(KMGoap), BlueprintType, Blueprintable, Category = "KMGoap|ActorComponents", meta=(BlueprintSpawnableComponent))
class KMGOAP_API UKMGoapKnowledgeProviderComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	/**
	 * Initializes this knowledge provider component with default property values.
	 */
	UKMGoapKnowledgeProviderComponent();
	
	/**
	 * Teaches this provider's configured knowledge modules to the supplied agent.
	 *
	 * @param Agent Agent component that should receive the configured knowledge modules.
	 */
	void Teach(UKMGoapAgentComponent* Agent);

protected:
	/**
	 * Knowledge modules that this provider can add to an agent at runtime.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TArray<TObjectPtr<UKMGoapKnowledgeModule>> ModulesToProvide;
	
	/**
	 * Runtime set of agents that have already learned from this provider.
	 *
	 * Used to prevent the same provider from teaching duplicate knowledge to the
	 * same agent more than once.
	 */
	UPROPERTY(Transient)
	TSet<UKMGoapAgentComponent*> AgentsThatLearned;
	
	/**
	 * Initializes provider runtime state when gameplay begins.
	 */
	virtual void BeginPlay() override;
};
