// All rights reserved by Khrönmière Entertainment.
#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "KMGoapKnowledgeRuntime.generated.h"

struct FGameplayTag;
class UKMGoapAgentComponent;
class UKMGoapKnowledgeModule;

/**
 * Stores the gameplay tags of runtime-created GOAP objects that were instantiated from a knowledge module.
 *
 * This structure is used to track which beliefs, actions, and goals belong to a specific module so they can
 * be removed cleanly when that module is deactivated.
 */
USTRUCT(BlueprintType)
struct KMGOAP_API FKMGoapInstancedModuleTags
{
	GENERATED_BODY()

	/**
	 * Tags of belief instances created from the owning knowledge module.
	 */
	TArray<FGameplayTag> BeliefTags;

	/**
	 * Tags of action instances created from the owning knowledge module.
	 */
	TArray<FGameplayTag> ActionTags;

	/**
	 * Tags of goal instances created from the owning knowledge module.
	 */
	TArray<FGameplayTag> GoalTags;
};

/**
 * Runtime container responsible for activating, tracking, ticking, and deactivating GOAP knowledge modules.
 *
 * A knowledge runtime owns the active knowledge set for an agent. When a module is added, it instantiates
 * the module's beliefs, actions, and goals into the target agent. During ticking, it evaluates module
 * deactivation rules and removes module-provided runtime objects when the rules are satisfied.
 */
UCLASS(BlueprintType, Blueprintable)
class KMGOAP_API UKMGoapKnowledgeRuntime : public UObject
{
	GENERATED_BODY()

public:
	/**
	 * Adds a new knowledge module to the agent at runtime.
	 *
	 * The module is ignored if it is null or if another active module already uses the same knowledge tag.
	 * When successfully added, the module's beliefs, actions, and goals are instantiated into the agent and
	 * the agent execution state is reset.
	 *
	 * @param Agent Agent that will receive the module-provided runtime objects.
	 * @param NewModule Knowledge module asset to activate.
	 * @return True if the module was added successfully; false otherwise.
	 */
	UFUNCTION(BlueprintCallable)
	bool AddKnowledge(UKMGoapAgentComponent* Agent, UKMGoapKnowledgeModule* NewModule);

	/**
	 * Updates the runtime knowledge state for the supplied agent.
	 *
	 * This evaluates active module deactivation rules and removes any modules whose rules are fully satisfied.
	 *
	 * @param Agent Agent that owns this runtime knowledge state.
	 */
	UFUNCTION(BlueprintCallable)
	void Tick(UKMGoapAgentComponent* Agent);

protected:
	/**
	 * Active knowledge modules indexed by their unique knowledge tag.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TMap<FGameplayTag, UKMGoapKnowledgeModule*> KnowledgeSet;

	/**
	 * Evaluates all active module deactivation rules and fills the output array with modules that should be removed.
	 *
	 * A module is marked for removal only when it has at least one deactivation rule and all rules match the
	 * agent's current belief state.
	 *
	 * @param Agent Agent used to evaluate belief states.
	 * @param ToRemove Output array receiving knowledge tags for modules that should be deactivated.
	 */
	void EvaluateKnowledgeModulesDeactivationRules(const UKMGoapAgentComponent* Agent, TArray<FGameplayTag>& ToRemove) const;

	/**
	 * Deactivates all active knowledge modules matching the provided tags.
	 *
	 * This removes the module-provided beliefs, actions, and goals from the agent, updates the belief cache,
	 * and resets the agent execution state.
	 *
	 * @param Agent Agent that owns the runtime objects to remove.
	 * @param Tags Knowledge tags identifying modules to deactivate.
	 */
	void DeactivateKnowledgesWithTags(UKMGoapAgentComponent* Agent, const TArray<FGameplayTag>& Tags);

	/**
	 * Instantiates and registers the runtime objects provided by a knowledge module.
	 *
	 * The created belief, action, and goal tags are stored so they can be removed later if the module deactivates.
	 *
	 * @param Agent Agent that will own the instantiated runtime objects.
	 * @param AddedModule Module whose configured object sets should be instantiated.
	 */
	void InitializeModule(UKMGoapAgentComponent* Agent, UKMGoapKnowledgeModule* AddedModule);

private:
	/**
	 * Tracks the runtime-created belief, action, and goal tags associated with each active module.
	 */
	UPROPERTY(Transient)
	TMap<UKMGoapKnowledgeModule*, FKMGoapInstancedModuleTags> TagGroupPerModule;

	/**
	 * Loads, instantiates, registers, and returns tags for objects declared in a soft-class set.
	 *
	 * Each soft class is loaded synchronously, instantiated with the supplied outer, added to the target map
	 * by tag, and returned in the resulting tag list.
	 *
	 * @tparam TObject Object type to instantiate.
	 * @tparam TTag Tag/key type used to index the object.
	 * @tparam TGetTag Callable type used to extract a tag from an instantiated object.
	 * @param Outer Outer object used when creating runtime instances.
	 * @param SoftClasses Soft class references to load and instantiate.
	 * @param TargetMap Map that receives instantiated objects keyed by their tag.
	 * @param GetTag Callable that returns the tag for each instantiated object.
	 * @return Tags for all successfully created and registered instances.
	 */
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

	/**
	 * Removes registered runtime object instances from a tag-indexed map.
	 *
	 * @tparam TObject Object type stored in the target map.
	 * @tparam TTag Tag/key type used to index the object.
	 * @param TagsToRemove Tags identifying entries to remove from the target map.
	 * @param TargetMap Map to remove entries from.
	 */
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
