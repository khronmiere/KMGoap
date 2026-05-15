// All rights reserved by Khrönmière Entertainment.
#pragma once

#include "CoreMinimal.h"
#include "Data/KMGoapActionPlan.h"
#include "Interface/KMGoapAgentStateMachineInterface.h"
#include "UObject/Object.h"
#include "KMGoapDefaultStateMachine.generated.h"

/**
 * Default GOAP state machine implementation used by an agent component.
 *
 * This state machine owns the runtime execution state for the currently selected
 * goal, generated action plan, and active action. It reacts to sensor updates,
 * recalculates plans when required, and advances action execution during ticks.
 */
UCLASS()
class KMGOAP_API UKMGoapDefaultStateMachine : public UObject, public IKMGoapAgentStateMachineInterface
{
	GENERATED_BODY()

public:
	/**
	 * Starts the state machine for the supplied GOAP agent.
	 *
	 * @param NewAgent Agent component that this state machine should control.
	 */
	virtual void Start_Implementation(UKMGoapAgentComponent* NewAgent) override;
	
	/**
	 * Stops the state machine and releases runtime execution state.
	 */
	virtual void Stop_Implementation() override;
	
	/**
	 * Advances the state machine by one frame.
	 *
	 * @param DeltaTime Time elapsed since the previous tick, in seconds.
	 */
	virtual void Tick_Implementation(float DeltaTime) override;
	
	/**
	 * Resets current execution state and forces future planning/execution to restart.
	 */
	virtual void Reset_Implementation() override;
	
	/**
	 * Handles notification that one or more sensors changed state.
	 *
	 * Sensor changes may invalidate the current plan, goal, or action.
	 */
	virtual void OnSensorStateUpdate_Implementation() override;

protected:
	/** Agent component currently controlled by this state machine. */
	UPROPERTY(Transient)
	TObjectPtr<UKMGoapAgentComponent> Agent;
	
	/** Action currently being executed from the active plan. */
	UPROPERTY(BlueprintReadOnly, Category="GOAP|Runtime")
	TObjectPtr<UKMGoapAgentAction> CurrentAction = nullptr;
	
	/** Goal currently selected for execution. */
	UPROPERTY(BlueprintReadOnly, Category="GOAP|Runtime")
	TObjectPtr<UKMGoapAgentGoal> CurrentGoal = nullptr;

	/** Previously selected goal, used to support goal continuity during replanning. */
	UPROPERTY(BlueprintReadOnly, Category="GOAP|Runtime")
	TObjectPtr<UKMGoapAgentGoal> LastGoal = nullptr;

	/** Current generated action plan for the selected goal. */
	UPROPERTY(BlueprintReadOnly, Category="GOAP|Runtime")
	FKMGoapActionPlan CurrentPlan;

	/** Should include the current goal in replanning when it is still valid. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="GOAP|Planning")
	bool bIncludeCurrentGoalWhenReplanning = true;

	/** Allow switching to goals with equal priority, within a margin. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="GOAP|Planning")
	bool bAllowEqualPriorityGoalSwitching = true;

	/** Margin for goal priority comparison when allowing equal priority switching. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="GOAP|Planning")
	float GoalSwitchPriorityMargin = 0.0f;
	
	/**
	 * Calculates a new action plan for the agent's available goals.
	 */
	void CalculatePlan();

	/**
	 * Updates action execution state, including action transitions and completion handling.
	 */
	void UpdateExecutionState();
	
	/**
	 * Completes lifecycle cleanup for an action that reached a terminal state naturally.
	 *
	 * This calls StopAction for action cleanup and then Release so the action instance can be reused.
	 */
	void FinishCurrentAction();

	/**
	 * Interrupts a currently running action because execution state is being reset.
	 *
	 * Running actions receive StopAction before Release so Blueprint/C++ implementations can clean up
	 * animations, timers, movement requests, latent work, or other resources.
	 */
	void InterruptCurrentAction();

	/**
	 * Releases a selected action that was never started.
	 *
	 * This is used when precondition validation fails after an action is selected from a plan but before
	 * StartAction is called. StopAction is intentionally not called for a NotStarted action.
	 */
	void ReleaseSelectedAction();

	/**
	 * Clears current action, goal, and plan runtime state.
	 * 
	 * @param bInterruptActiveAction If true, a running action will be interrupted before resetting state.
	 */
	void ResetExecutionState(bool bInterruptActiveAction);
};
