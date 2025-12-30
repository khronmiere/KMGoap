// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "KMGoapKnowledgeRuntime.generated.h"

struct FGameplayTag;
class UKMGoapAgentComponent;
class UKMGoapKnowledgeModule;

USTRUCT(BlueprintType)
struct KMGOAP_API FKMGoapInstancedModuleTags
{
	GENERATED_BODY()
	
	TArray<FGameplayTag> BeliefTags;
	TArray<FGameplayTag> ActionTags;
	TArray<FGameplayTag> GoalTags;
};

/**
 * 
 */
UCLASS(BlueprintType, Blueprintable)
class KMGOAP_API UKMGoapKnowledgeRuntime : public UObject
{
	GENERATED_BODY()
	
public:
	UFUNCTION(BlueprintCallable)
	bool AddKnowledge(UKMGoapAgentComponent* Agent, UKMGoapKnowledgeModule* NewModule);

	UFUNCTION(BlueprintCallable)
	void Tick(UKMGoapAgentComponent* Agent);
	
protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TMap<FGameplayTag, UKMGoapKnowledgeModule*> KnowledgeSet;
	
	void EvaluateKnowledgeModulesDeactivationRules(const UKMGoapAgentComponent* Agent, TArray<FGameplayTag>& ToRemove) const;
	void DeactivateKnowledgesWithTags(UKMGoapAgentComponent* Agent, const TArray<FGameplayTag>& Tags);
	
	void InitializeModule(UKMGoapAgentComponent* Agent, UKMGoapKnowledgeModule* AddedModule);
	
private:
	UPROPERTY(Transient)
	TMap<UKMGoapKnowledgeModule*, FKMGoapInstancedModuleTags> TagGroupPerModule;
	
	template<typename TObject, typename TTag, typename TGetTag>
	static TArray<TTag> AddInstancesFromSet(
		UObject* Outer,
		const TArray<TSoftClassPtr<TObject>>& SoftClasses,
		TMap<TTag, TObjectPtr<TObject>>& TargetMap,
		TGetTag&& GetTag
	)
	{
		TArray<TTag> Result;
		for (const TSoftClassPtr<TObject>& SoftClass : SoftClasses)
		{
			UClass* LoadedClass = SoftClass.LoadSynchronous();
			if (!LoadedClass)
			{
				continue;
			}

			// Use TObject explicitly (no NewObject<auto> in C++17)
			TObject* Instance = NewObject<TObject>(Outer, LoadedClass);
			if (!Instance)
			{
				continue;
			}

			TTag Tag = GetTag(Instance);
			TargetMap.Add(Tag, Instance);
			Result.Add(Tag);
		}
		return Result;
	}
	
	template<typename TObject, typename TTag>
	static void RemoveInstancesByTag(
		const TArray<TTag>& TagsToRemove,
		TMap<TTag, TObjectPtr<TObject>>& TargetMap
	)
	{
		for (const TTag& Tag : TagsToRemove)
		{
			TargetMap.Remove(Tag);
		}
	}

};
