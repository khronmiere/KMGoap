// All rights reserved by Khrönmière Entertainment.
#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Components/ActorComponent.h"
#include "Data/KMGoapActionPlan.h"
#include "Data/KMGoapCondition.h"
#include "KMGoapAgentComponent.generated.h"

struct FKMGoapCondition;
class UKMGoapKnowledgeModule;
class UKMGoapKnowledgeRuntime;
class UKMGoapAgentAction;
class UKMGoapBeliefSet;
class UKMGoapAgentGoal;
class UKMGoapAgentBelief;
class UKMGoapGoalSet;
class UKMGoapActionSet;
class UKMGoapSensorComponent;

/**
 * Cached result of a single GOAP belief evaluation.
 *
 * Stores the gameplay tag that identifies the belief and the most recently
 * evaluated boolean value for that belief. This cache is used by the agent to
 * avoid repeatedly evaluating belief objects during planning and precondition
 * checks.
 */
USTRUCT(BlueprintType)
struct KMGOAP_API FKMGoapBeliefCacheEntry
{
	GENERATED_BODY()

	/** Gameplay tag identifying the cached belief. */
	FGameplayTag BeliefTag;

	/** Cached evaluated value of the belief. */
	bool bValue = false;
};

/**
 * Actor component that owns and coordinates an agent's GOAP runtime state.
 *
 * The GOAP agent component is responsible for:
 * - Building runtime instances of configured beliefs, actions, and goals.
 * - Caching sensors by gameplay tag and reacting to sensor target changes.
 * - Evaluating beliefs and exposing cached belief/fact state to planners.
 * - Managing dynamically learned knowledge modules.
 * - Delegating plan computation to the planner subsystem.
 * - Driving the configured state machine runner.
 *
 * Attach this component to an actor that should behave as a GOAP-controlled
 * agent. Configure the belief, action, and goal sets in the editor, and add
 * sensor components implementing the GOAP sensor interface to the same actor.
 */
UCLASS(ClassGroup=(KMGoap), BlueprintType, Blueprintable, Category = "KMGoap|ActorComponents", meta=(BlueprintSpawnableComponent))
class KMGOAP_API UKMGoapAgentComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	/** Creates the component and configures ticking/state-machine defaults. */
	UKMGoapAgentComponent();

	/** Initial set of belief classes available to this agent. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="GOAP|Sets")
	TObjectPtr<UKMGoapBeliefSet> BeliefSet;
	/** Runtime belief instances indexed by their belief gameplay tag. */
	UPROPERTY(Transient, BlueprintReadOnly, Category="GOAP")
	TMap<FGameplayTag, TObjectPtr<UKMGoapAgentBelief>> BeliefsByTag;

	/** Runtime sensor components indexed by their sensor gameplay tag. */
	UPROPERTY(Transient, BlueprintReadOnly, Category="GOAP")
	TMap<FGameplayTag, TObjectPtr<UActorComponent>> SensorsByTag;

	/** Initial set of action classes available to this agent. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="GOAP|Sets")
	TObjectPtr<UKMGoapActionSet> ActionSet;
	/** Runtime action instances indexed by their action gameplay tag. */
	UPROPERTY(Transient, BlueprintReadOnly, Category="GOAP")
	TMap<FGameplayTag, TObjectPtr<UKMGoapAgentAction>> ActionsByTag;

	/** Initial set of goal classes available to this agent. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="GOAP|Sets")
	TObjectPtr<UKMGoapGoalSet> GoalSet;
	/** Runtime goal instances indexed by their goal gameplay tag. */
	UPROPERTY(Transient, BlueprintReadOnly, Category="GOAP")
	TMap<FGameplayTag, TObjectPtr<UKMGoapAgentGoal>> GoalsByTag;
	
	/**
	 * Finds a runtime belief instance by gameplay tag.
	 *
	 * @param Tag Tag identifying the belief.
	 * @return The matching belief instance, or nullptr if no belief is registered for the tag.
	 */
	UFUNCTION(BlueprintCallable, Category="GOAP|Beliefs")
	UKMGoapAgentBelief* GetBeliefByTag(FGameplayTag Tag) const;

	/**
	 * Finds a cached sensor component by gameplay tag.
	 *
	 * @param Tag Tag identifying the sensor.
	 * @return The matching sensor component, or nullptr if no sensor is registered for the tag.
	 */
	UFUNCTION(BlueprintCallable, Category="GOAP|Sensors")
	UActorComponent* GetSensorByTag(FGameplayTag Tag) const;

	/**
	 * Finds a runtime goal instance by gameplay tag.
	 *
	 * @param Tag Tag identifying the goal.
	 * @return The matching goal instance, or nullptr if no goal is registered for the tag.
	 */
	UFUNCTION(BlueprintCallable, Category="GOAP|Sensors")
	UKMGoapAgentGoal* GetGoalByTag(FGameplayTag Tag) const;

	/**
	 * Finds a runtime action instance by gameplay tag.
	 *
	 * @param Tag Tag identifying the action.
	 * @return The matching action instance, or nullptr if no action is registered for the tag.
	 */
	UFUNCTION(BlueprintCallable, Category="GOAP|Sensors")
	UKMGoapAgentAction* GetActionByTag(FGameplayTag Tag) const;

	/**
	 * Reads the latest cached state for a belief.
	 *
	 * @param Tag Tag identifying the belief.
	 * @return Positive or Negative when the belief exists in the cache; Unknown otherwise.
	 */
	UFUNCTION(BlueprintCallable, Category="GOAP|Beliefs")
	EKMGoapBeliefState EvaluateBeliefByTag(FGameplayTag Tag) const;

	/**
	 * Gets the world-space location associated with a belief.
	 *
	 * @param Tag Tag identifying the belief.
	 * @return The belief-provided location, or FVector::ZeroVector if the belief is unavailable.
	 */
	UFUNCTION(BlueprintCallable, Category="GOAP|Beliefs")
	FVector GetBeliefLocationByTag(FGameplayTag Tag) const;

	/**
	 * Sets or updates a simple boolean fact on the agent.
	 *
	 * Facts are explicit state values used alongside evaluated beliefs when
	 * validating action preconditions.
	 *
	 * @param FactTag Tag identifying the fact.
	 * @param bAdd New fact value to store.
	 */
	UFUNCTION(BlueprintCallable, Category="GOAP|Facts")
	void SetFact(FGameplayTag FactTag, bool bAdd = true);

	/**
	 * Gets the current value of a stored fact.
	 *
	 * @param Tag Tag identifying the fact.
	 * @return Positive or Negative when the fact exists; Unknown otherwise.
	 */
	UFUNCTION(BlueprintCallable, Category="GOAP|Facts")
	EKMGoapBeliefState GetFact(FGameplayTag Tag) const;

	/**
	 * Returns all currently known fact tags.
	 *
	 * @return Array containing the gameplay tags of all stored facts.
	 */
	TArray<FGameplayTag> GetFactsTags() const;

	/**
	 * Adds a knowledge module to this agent at runtime.
	 *
	 * The module may contribute additional beliefs, actions, and goals. Adding
	 * knowledge resets the current execution state if the module is accepted.
	 *
	 * @param NewModule Knowledge module to add.
	 * @return true if the module was added; false if invalid, duplicate, or runtime is unavailable.
	 */
	UFUNCTION(BlueprintCallable, Category="GOAP|Knowledge")
	bool AddNewKnowledgeModule(UKMGoapKnowledgeModule* NewModule);

	/**
	 * Checks whether all preconditions for an action are currently satisfied.
	 *
	 * Preconditions may be satisfied by explicit facts or by cached belief values.
	 *
	 * @param Action Action whose preconditions should be validated.
	 * @return true if the action is valid to execute in the current state.
	 */
	bool ValidateActionPreconditions(const UKMGoapAgentAction* Action) const;

	/** Re-evaluates all runtime beliefs and refreshes the belief cache immediately. */
	void UpdateBeliefEvaluationCache();

	/**
	 * Requests a GOAP plan for the provided goals.
	 *
	 * Delegates planning to the configured planner subsystem and search algorithm.
	 *
	 * @param GoalsToCheck Candidate goals to plan for.
	 * @param LastGoal Previously selected goal, used by planners that account for goal continuity.
	 * @param OutPlan Receives the generated action plan on success.
	 * @return true if a valid plan was produced.
	 */
	bool ComputePlanForGoals(
		const TArray<UKMGoapAgentGoal*>& GoalsToCheck,
		UKMGoapAgentGoal* LastGoal,
		FKMGoapActionPlan& OutPlan);

	/** Resets the active state-machine runner execution state, if one exists. */
	void ResetExecutionState() const;
	
	/** Initializes sensors, runtime GOAP objects, state machine, and knowledge runtime. */
	virtual void BeginPlay() override;

	/** Cleans up sensors, beliefs, and state-machine runtime before the component ends play. */
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** Ticks the state-machine runner and knowledge runtime. */
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
	/**
	 * Attempts to learn knowledge from the current target of a sensor.
	 *
	 * @param SourceTag Tag of the sensor whose target may provide knowledge.
	 */
	void TryLearnKnowledge(FGameplayTag SourceTag);

	/**
	 * Blueprint extension point called when a sensor target changes.
	 *
	 * @param SensorTag Tag of the sensor that reported the target change.
	 */
	UFUNCTION(BlueprintNativeEvent, Category="GOAP|Runtime")
	void OnSensorTargetChanged(FGameplayTag SensorTag);
	virtual void OnSensorTargetChanged_Implementation(FGameplayTag SensorTag){}

private:
	/** Explicit boolean facts currently known by this agent. */
	UPROPERTY(Transient)
	TMap<FGameplayTag, bool> Facts;

	/** Cached belief evaluation results indexed by belief tag. */
	UPROPERTY(Transient)
	TMap<FGameplayTag, FKMGoapBeliefCacheEntry> BeliefCache;

	/** Runtime manager for dynamically added knowledge modules. */
	UPROPERTY(Transient)
	TObjectPtr<UKMGoapKnowledgeRuntime> KnowledgeRuntime;

	/** Timer handle used for periodic belief evaluation. */
	FTimerHandle BeliefEvaluateTimerHandle;

	/** Interval, in seconds, between automatic belief cache refreshes. */
	UPROPERTY(EditAnywhere, Category="GOAP|Runtime")
	float EvaluateBeliefTimeStep = 0.03f;

	/** UObject class that implements the GOAP agent state-machine interface. */
	UPROPERTY(EditAnywhere, Category="GOAP|Runtime", meta=(MustImplement="/Script/KMGoap.KMGoapAgentStateMachineInterface"))
	TSubclassOf<UObject> StateMachineRunnerClass;

	/** Runtime state-machine runner instance. */
	UPROPERTY(Transient)
	TObjectPtr<UObject> StateMachineRunner = nullptr;

	/** Builds runtime belief instances from the configured belief set. */
	void BuildBeliefs();

	/** Stops belief evaluation and clears belief-related runtime state. */
	void ClearBeliefs();

	/** Builds runtime action instances from the configured action set. */
	void BuildActions();

	/** Builds runtime goal instances from the configured goal set. */
	void BuildGoals();

	/** Finds owner sensor components, indexes them by tag, and binds sensor events. */
	void CacheSensors();

	/** Unbinds sensor events and clears the sensor cache. */
	void ClearSensors();

	/** Creates and starts the configured state-machine runner. */
	void InitializeStateMachineRunner();

	/** Stops and releases the current state-machine runner. */
	void StopStateMachineRunner();

	/** Creates the runtime object responsible for dynamic knowledge modules. */
	void InitializeKnowledgeRuntime();

	/**
	 * Binds this agent to receive target-change events from a sensor.
	 *
	 * @param Sensor Sensor component implementing the GOAP sensor interface.
	 */
	void BindSensorEvents(UActorComponent* Sensor);

	/**
	 * Removes this agent's target-change binding from a sensor.
	 *
	 * @param Sensor Sensor component implementing the GOAP sensor interface.
	 */
	void UnbindSensorEvents(UActorComponent* Sensor);

	/**
	 * Removes preconditions already satisfied by explicit facts.
	 *
	 * @param Preconditions Mutable set of preconditions to filter.
	 */
	void FilterFactSatisfiedPreconditions(TSet<FKMGoapCondition>& Preconditions) const;

	/**
	 * Removes preconditions already satisfied by cached belief evaluations.
	 *
	 * @param Preconditions Mutable set of preconditions to filter.
	 */
	void FilterBeliefSatisfiedPreconditions(TSet<FKMGoapCondition>& Preconditions) const;

	/**
	 * Handles sensor target-change broadcasts.
	 *
	 * Attempts knowledge learning, resets/updates the state machine, and invokes
	 * the Blueprint extension event.
	 *
	 * @param SourceTag Tag of the sensor that changed.
	 */
	UFUNCTION()
	void HandleSensorTargetChanged(FGameplayTag SourceTag);

	/** Evaluates every runtime belief and rebuilds the belief cache. */
	UFUNCTION()
	void EvaluateBeliefs();

	/**
	 * Instantiates UObject-derived classes from soft class references and stores
	 * them in a tag-indexed map.
	 *
	 * Invalid classes, invalid tags, and duplicate tags are skipped.
	 *
	 * @tparam TObjectType Type of object to instantiate.
	 * @tparam TTagGetter Callable type used to extract the gameplay tag from each object.
	 * @param Outer UObject outer for created instances.
	 * @param Classes Soft class references to load and instantiate.
	 * @param OutMap Destination map indexed by object tag.
	 * @param GetTag Callable that returns the object's identifying gameplay tag.
	 */
	template <typename TObjectType, typename TTagGetter>
	static void BuildTaggedObjects(
		UObject* Outer,
		const TArray<TSoftClassPtr<TObjectType>>& Classes,
		TMap<FGameplayTag, TObjectPtr<TObjectType>>& OutMap,
		TTagGetter&& GetTag)
	{
		OutMap.Reset();

		for (const TSoftClassPtr<TObjectType>& SoftClass : Classes)
		{
			if (SoftClass.IsNull())
			{
				continue;
			}

			UClass* LoadedClass = SoftClass.LoadSynchronous();
			if (!LoadedClass)
			{
				continue;
			}

			TObjectType* Obj = NewObject<TObjectType>(Outer, LoadedClass);
			if (!Obj)
			{
				continue;
			}

			const FGameplayTag Tag = GetTag(Obj);
			if (!Tag.IsValid())
			{
				continue;
			}

			// Optional: detect duplicates
			if (OutMap.Contains(Tag))
			{
				UE_LOG(LogTemp, Warning, TEXT("GOAP Agent: Duplicate tag [%s] for %s. Skipping."),
					*Tag.ToString(), *GetNameSafe(LoadedClass));
				continue;
			}

			OutMap.Add(Tag, Obj);
		}
	}
};