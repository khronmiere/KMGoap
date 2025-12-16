#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Data/KMGoapCondition.h"
#include "KMGoapPlanningTypes.generated.h"

USTRUCT()
struct FKMGoapSimState
{
	GENERATED_BODY()
	
	TMap<FGameplayTag, bool> Facts;

	bool TryGet(const FGameplayTag& Tag, bool& Out) const
	{
		if (const bool* V = Facts.Find(Tag))
		{
			Out = *V;
			return true;
		}
		return false;
	}

	void Set(const FGameplayTag& Tag, bool bValue)
	{
		Facts.Add(Tag, bValue);
	}

	bool Satisfies(const FKMGoapCondition& Condition) const
	{
		bool V = false;
		return TryGet(Condition.Tag, V) && (V == Condition.bValue);
	}
};

struct FKMGoapPlanningContext
{
	// Initial snapshot only (copied from agent once)
	FKMGoapSimState InitialState;

	// Optional: to score goal priority
	TWeakObjectPtr<class UKMGoapAgentComponent> Agent;

	// All usable actions already filtered
	TArray<TObjectPtr<class UKMGoapAgentAction>> Actions;
};
