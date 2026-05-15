// All rights reserved by Khrönmière Entertainment.
#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "KMGoapSensorInterface.generated.h"

/**
 * Unreal reflection wrapper for GOAP sensor interface implementations.
 *
 * This interface type allows Blueprint and C++ actor components to be recognized
 * by Unreal as valid GOAP sensors. Sensor implementations expose information
 * about a perceived target and notify listeners when that target changes.
 */
UINTERFACE(Category = "KMGoap|Interface", BlueprintType)
class KMGOAP_API UKMGoapSensorInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * Defines the contract for components that provide target-sensing data to GOAP agents.
 *
 * Implementations identify themselves with a gameplay tag, expose whether they
 * currently have a valid target, provide target actor/location data, and allow
 * listeners to subscribe to target-change notifications.
 */
class KMGOAP_API IKMGoapSensorInterface
{
	GENERATED_BODY()

public:
	/**
	 * Gets the gameplay tag that uniquely identifies this sensor for a GOAP agent.
	 *
	 * @return The gameplay tag used by agents to look up and reference this sensor.
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="KMGoap|Sensor")
	FGameplayTag GetTag() const;

	/**
	 * Registers an object function to be called when this sensor's target changes.
	 *
	 * @param Listener The object that owns the callback function.
	 * @param FunctionName The name of the function to invoke on the listener.
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="KMGoap|Sensor")
	void RegisterTargetChangedListener(UObject* Listener, FName FunctionName);

	/**
	 * Unregisters a previously registered target-changed callback.
	 *
	 * @param Listener The object that owns the callback function.
	 * @param FunctionName The name of the function that should no longer be invoked.
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="KMGoap|Sensor")
	void UnregisterTargetChangedListener(UObject* Listener, FName FunctionName);

	/**
	 * Checks whether this sensor currently has a valid target.
	 *
	 * @return True if a target is currently available; otherwise false.
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="KMGoap|Sensor")
	bool HasTarget() const;

	/**
	 * Gets the actor currently tracked by this sensor.
	 *
	 * @return The current target actor, or nullptr when no target is available.
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="KMGoap|Sensor")
	AActor* GetTarget() const;

	/**
	 * Gets the current world-space position of the sensed target.
	 *
	 * @return The target position when available, or an implementation-defined fallback position.
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="KMGoap|Sensor")
	FVector GetTargetPosition() const;
};
