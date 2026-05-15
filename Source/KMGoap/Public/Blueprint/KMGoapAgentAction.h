// All rights reserved by Khrönmière Entertainment.
#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "UObject/Object.h"
#include "Data/KMGoapCondition.h"
#include "KMGoapAgentAction.generated.h"

struct FKMGoapCondition;
struct FKMGoapActionPlan;
class UKMGoapAgentComponent;

/**
 * Describes the runtime execution state of a GOAP action.
 */
UENUM(BlueprintType)
enum class EKMGoapActionStatus : uint8
{
	/** The action has not been started yet. */
	NotStarted,

	/** The action is currently executing. */
	Running,

	/** The action finished successfully and its effects may be considered achieved. */
	Succeeded,

	/** The action finished unsuccessfully and should not be treated as completed work. */
	Failed
};

/**
 * Defines a single GOAP action that an agent can evaluate, plan with, and execute.
 *
 * A GOAP action represents an executable step in an agent plan. It contains
 * preconditions that must be satisfied before execution, predicted effects used by
 * the planner, and optional runtime facts that are written to the owning agent when
 * the action succeeds.
 *
 * PredictedEffects are simulation-only and are not directly written to the agent.
 * RuntimeFacts are agent-local state changes and are written on successful execution.
 *
 * Actions are authored as Blueprintable UObject assets and are instanced by
 * an agent at runtime. The agent executor drives the action lifecycle by calling
 * StartAction, TickAction, and StopAction.
 */
UCLASS(Abstract, BlueprintType, Blueprintable, EditInlineNew, DefaultToInstanced, Category="KMGoap")
class KMGOAP_API UKMGoapAgentAction : public UObject
{
	GENERATED_BODY()

public:
	/**
	 * Unique gameplay tag that identifies this action in the agent action registry.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Action")
	FGameplayTag ActionTag;

	/**
	 * Base planning cost used when evaluating this action before dynamic modifiers are applied.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Action", meta=(ClampMin="0.0"))
	float BaseCost = 1.f;

	/**
	 * Conditions that must be satisfied before this action can be selected or executed.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Action")
	TSet<FKMGoapCondition> Preconditions;

	/**
	 * Planner-only conditions this action is expected to make true or false after successful completion.
	 *
	 * These conditions are used when simulating plans, but they are not directly written to the agent's
	 * runtime fact map. Use these for outcomes that should be observed through gameplay, sensors, or beliefs.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Action|Planning",
		meta=(ToolTip="Planner-only expected world/belief outcomes. These are not written to the agent at runtime; they should become true through gameplay, sensors, or beliefs."))
	TSet<FKMGoapCondition> PredictedEffects;

	/**
	 * Agent-local fact values written when this action succeeds.
	 *
	 * Runtime facts are also included as planner postconditions so future simulated actions can depend on them.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Action|Runtime",
		meta=(ToolTip="Agent-local fact changes written to the agent when this action succeeds. These also participate in planning as postconditions."))
	TSet<FKMGoapCondition> RuntimeFacts;

	/**
	 * Starts this action for the supplied GOAP agent.
	 *
	 * @param Agent Agent component that owns and executes this action.
	 */
	UFUNCTION(BlueprintCallable, Category="Action")
	void StartAction(UKMGoapAgentComponent* Agent);

	/**
	 * Advances this action by one frame or simulation step.
	 *
	 * @param Agent Agent component that owns and executes this action.
	 * @param DeltaTime Time elapsed since the previous tick.
	 * @return Current execution status after ticking the action.
	 */
	UFUNCTION(BlueprintCallable, Category="Action")
	EKMGoapActionStatus TickAction(UKMGoapAgentComponent* Agent, float DeltaTime);

	/**
	 * Stops this action and performs any required cleanup.
	 *
	 * @param Agent Agent component that owns and executes this action.
	 */
	UFUNCTION(BlueprintCallable, Category="Action")
	void StopAction(UKMGoapAgentComponent* Agent);

	/**
	 * Checks whether this action has reached a terminal state.
	 *
	 * @return True when the action has either succeeded or failed.
	 */
	UFUNCTION(BlueprintCallable, Category="Action")
	bool IsComplete() const { return Status == EKMGoapActionStatus::Succeeded || Status == EKMGoapActionStatus::Failed; }

	/**
	 * Gets the current runtime status of this action.
	 *
	 * @return Current action execution status.
	 */
	UFUNCTION(BlueprintCallable, Category="Action")
	EKMGoapActionStatus GetStatus() const { return Status; }
	
	/**
	 * Releases any resources held by this action and marks it as released.
	 * 
	 * This should be called when the action is no longer needed, such as when it is removed from an agent's action queue.
	 *
	 * @param Agent Agent component that owns and executes this action.
	 */
	UFUNCTION(BlueprintCallable, Category="Action")
	void Release(UKMGoapAgentComponent* Agent);
	
	/**
	 * Gets every condition that should be considered true in planner simulation after this action completes.
	 *
	 * This includes both PredictedEffects and RuntimeFacts:
	 * - PredictedEffects describe expected world/belief outcomes for planning only.
	 * - RuntimeFacts describe agent-local fact writes that are applied on successful execution.
	 *
	 * @return Set of planner postconditions produced by this action.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Action")
	TSet<FKMGoapCondition> GetPostConditions() const;

	/**
	 * Determines whether this action is currently allowed to execute for the supplied agent.
	 *
	 * Override this in Blueprint or C++ to add contextual checks beyond normal GOAP
	 * precondition validation.
	 *
	 * @param Agent Agent component requesting permission to perform the action.
	 * @return True when the action can be performed.
	 */
	UFUNCTION(BlueprintNativeEvent, Category="Action", meta=(BlueprintProtected="true"))
	bool CanPerform(UKMGoapAgentComponent* Agent) const;
	virtual bool CanPerform_Implementation(UKMGoapAgentComponent* Agent) const;

	/**
	 * Calculates the runtime planning cost for this action.
	 *
	 * Override this to adjust action cost based on agent state, distance, resources,
	 * danger, or any other runtime context.
	 *
	 * @param Agent Agent component evaluating this action.
	 * @return Runtime cost used by the planner.
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "Action")
	float GetDynamicCost(const UKMGoapAgentComponent* Agent) const;
	float GetDynamicCost_Implementation(const UKMGoapAgentComponent* Agent) const { return BaseCost; }

protected:
	/**
	 * Current runtime execution status of this action instance.
	 */
	UPROPERTY(Transient)
	EKMGoapActionStatus Status = EKMGoapActionStatus::NotStarted;

	/**
	 * Blueprint/C++ extension point called when the action starts.
	 *
	 * @param Agent Agent component that owns and executes this action.
	 */
	UFUNCTION(BlueprintNativeEvent, Category="Action", meta=(BlueprintProtected="true"))
	void OnStart(UKMGoapAgentComponent* Agent);
	virtual void OnStart_Implementation(UKMGoapAgentComponent* Agent) {}

	/**
	 * Blueprint/C++ extension point called every tick while the action is running.
	 *
	 * @param Agent Agent component that owns and executes this action.
	 * @param DeltaTime Time elapsed since the previous tick.
	 * @return Updated action execution status.
	 */
	UFUNCTION(BlueprintNativeEvent, Category="Action", meta=(BlueprintProtected="true"))
	EKMGoapActionStatus OnTick(UKMGoapAgentComponent* Agent, float DeltaTime);
	virtual EKMGoapActionStatus OnTick_Implementation(UKMGoapAgentComponent* Agent, float DeltaTime);

	/**
	 * Blueprint/C++ extension point called when the action stops.
	 *
	 * @param Agent Agent component that owns and executes this action.
	 */
	UFUNCTION(BlueprintNativeEvent, Category="Action", meta=(BlueprintProtected="true"))
	void OnStop(UKMGoapAgentComponent* Agent);
	virtual void OnStop_Implementation(UKMGoapAgentComponent* Agent) {}
	
	/**
	 * Blueprint/C++ extension point called when the action is released.
	 *
	 * @param Agent Agent component that owns and executes this action.
	 */
	UFUNCTION(BlueprintNativeEvent, Category="Action", meta=(BlueprintProtected="true"))
	void OnRelease(UKMGoapAgentComponent* Agent);
	virtual void OnRelease_Implementation(UKMGoapAgentComponent* Agent) {}

	/**
	 * Applies this action's fact changes to the supplied agent.
	 * Applies this action's runtime fact changes to the supplied agent.
	 *
	 * PredictedEffects are intentionally not written here. They are planner-only expectations that should
	 * become true through gameplay systems, sensors, or evaluated beliefs.
	 *
	 * @param Agent Agent component that receives the runtime fact updates.
	 */
	void ApplyRuntimeFacts(UKMGoapAgentComponent* Agent) const;
};
