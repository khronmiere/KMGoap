// All rights reserved by Khrönmière Entertainment.
#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "KMGoapAgentStateMachineInterface.generated.h"

class UKMGoapAgentComponent;

/**
 * Unreal reflection wrapper for GOAP agent state machine implementations.
 *
 * This interface type enables Blueprint and C++ classes to be recognized by Unreal
 * as valid implementations of the GOAP agent state machine contract.
 */
UINTERFACE()
class KMGOAP_API UKMGoapAgentStateMachineInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * Defines the runtime contract for an object that controls a GOAP agent execution state.
 *
 * Implementations are responsible for starting, stopping, ticking, resetting, and reacting
 * to sensor-driven state changes for a UKMGoapAgentComponent.
 */
class KMGOAP_API IKMGoapAgentStateMachineInterface
{
	GENERATED_BODY()

public:
	/**
	 * Starts the state machine for the supplied GOAP agent.
	 *
	 * @param NewAgent The GOAP agent component that this state machine should control.
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="GOAP|StateMachine")
	void Start(UKMGoapAgentComponent* NewAgent);

	/**
	 * Stops the state machine and releases or clears any active execution state.
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="GOAP|StateMachine")
	void Stop();

	/**
	 * Advances the state machine by one frame.
	 *
	 * @param DeltaTime The elapsed time, in seconds, since the previous tick.
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="GOAP|StateMachine")
	void Tick(float DeltaTime);

	/**
	 * Resets the current execution state without necessarily stopping the state machine.
	 *
	 * Implementations should clear active actions, goals, plans, or other transient
	 * execution data so planning can restart from a clean state.
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="GOAP|StateMachine")
	void Reset();

	/**
	 * Notifies the state machine that sensor-derived world state has changed.
	 *
	 * Implementations should use this callback to invalidate or recompute plans that may
	 * depend on updated sensor data.
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="GOAP|StateMachine")
	void OnSensorStateUpdate();

	/**
	 * Get a State Snapshot for debugging purposes.
	 * 
	 * @param Agent The current agent component
	 * @return a snapshot of the current state
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="GOAP|StateMachine")
	FKMGoapDebugSnapshot GetDebugSnapshot() const;
};
