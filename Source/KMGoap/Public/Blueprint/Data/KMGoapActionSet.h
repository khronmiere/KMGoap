// All rights reserved by Khrönmière Entertainment.
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "KMGoapActionSet.generated.h"

/**
 * Data asset containing the GOAP action classes available to an agent.
 *
 * Action sets are referenced by GOAP agents and knowledge modules to define
 * which action types can be instantiated and considered during planning.
 */
UCLASS(Category = "KMGoap|Data", BlueprintType, Blueprintable)
class KMGOAP_API UKMGoapActionSet : public UDataAsset
{
	GENERATED_BODY()
	
public:
	/**
	 * Soft class references for all GOAP actions included in this set.
	 *
	 * Each class should derive from UKMGoapAgentAction and represents one
	 * executable step that may be used when building an action plan.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TArray<TSoftClassPtr<class UKMGoapAgentAction>> Actions;
};
