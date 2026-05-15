// All rights reserved by Khrönmière Entertainment.
#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Interface/KMGoapSensorInterface.h"
#include "UObject/Object.h"
#include "KMGoapSensorComponent.generated.h"

/**
 * Base actor component for GOAP sensors.
 *
 * A sensor observes world state and exposes a current target through the GOAP
 * sensor interface. Derived sensors are responsible for deciding how targets are
 * detected and should call SetTarget when their observed target changes.
 */
UCLASS(ClassGroup=(KMGoap), Category = "KMGoap|ActorComponents", Abstract, BlueprintType, Blueprintable, meta=(BlueprintSpawnableComponent))
class KMGOAP_API UKMGoapSensorComponent : public UActorComponent, public IKMGoapSensorInterface
{
	GENERATED_BODY()
	
public:
	/** Gameplay tag used to identify this sensor to GOAP agents and beliefs. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Sensor")
	FGameplayTag SensorTag;

	/**
	 * Delegate broadcast when this sensor's target or last known target position changes.
	 *
	 * @param SensorTag Tag identifying the sensor that changed.
	 */
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTargetChanged, FGameplayTag, SensorTag);

	/** Event fired when this sensor updates its target state. */
	UPROPERTY(BlueprintAssignable, Category="Sensor")
	FOnTargetChanged OnTargetChanged;

	/**
	 * Returns the currently tracked target actor.
	 *
	 * @return Current target actor, or nullptr when the sensor has no valid target.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Sensor")
	AActor* GetTargetActor() const { return TargetActor.Get(); }

protected:
	/** Weak reference to the actor currently tracked by this sensor. */
	UPROPERTY(Transient)
	TWeakObjectPtr<AActor> TargetActor;

	/** Last known world-space position of the tracked target. */
	UPROPERTY(Transient)
	FVector LastKnownPosition = FVector::ZeroVector;

#if WITH_EDITORONLY_DATA
	/** Interval, in seconds, between editor-only debug draw updates. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Sensor|Debug", meta=(ClampMin="0.01"))
	float DebugDrawInterval = 0.1f;

	/** Timer handle used to schedule repeated debug drawing. */
	FTimerHandle DebugDrawTimerHandle;
#endif

	/**
	 * Blueprint extension point for sensor debug visualization.
	 */
	UFUNCTION(BlueprintNativeEvent, Category="Sensor|Debug", meta=(BlueprintProtected="true"))
	void DebugDraw();

	/**
	 * Default debug draw implementation.
	 *
	 * Derived classes may override this to draw sensor-specific visualization.
	 */
	virtual void DebugDraw_Implementation() {}
	
	/**
	 * Initializes runtime sensor behavior and starts editor-only debug drawing when enabled.
	 */
	virtual void BeginPlay() override;

	/**
	 * Cleans up runtime sensor behavior and stops editor-only debug drawing.
	 *
	 * @param EndPlayReason Reason this component is ending play.
	 */
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	
	/**
	 * Updates the current target and broadcasts OnTargetChanged when state changes.
	 *
	 * @param NewTarget New actor target, or nullptr to clear the current target.
	 */
	UFUNCTION(BlueprintCallable, Category="Sensor", meta=(BlueprintProtected="true"))
	void SetTarget(AActor* NewTarget);
	
	/**
	 * Gets the gameplay tag identifying this sensor.
	 *
	 * @return This sensor's gameplay tag.
	 */
	virtual FGameplayTag GetTag_Implementation() const override;

	/**
	 * Registers an object function as a listener for target-change events.
	 *
	 * @param Listener Object that owns the function to call.
	 * @param FunctionName Name of the UFUNCTION to bind.
	 */
	virtual void RegisterTargetChangedListener_Implementation(UObject* Listener, FName FunctionName) override;

	/**
	 * Unregisters an object function from target-change events.
	 *
	 * @param Listener Object that owns the function to remove.
	 * @param FunctionName Name of the UFUNCTION to unbind.
	 */
	virtual void UnregisterTargetChangedListener_Implementation(UObject* Listener, FName FunctionName) override;

	/**
	 * Checks whether this sensor currently has a valid target.
	 *
	 * @return true when TargetActor is valid; otherwise false.
	 */
	virtual bool HasTarget_Implementation() const override;

	/**
	 * Gets the current target actor.
	 *
	 * @return Current target actor, or nullptr if no valid target exists.
	 */
	virtual AActor* GetTarget_Implementation() const override;

	/**
	 * Gets the current target's world-space position.
	 *
	 * @return Current target location, or FVector::ZeroVector if no valid target exists.
	 */
	virtual FVector GetTargetPosition_Implementation() const override;
};
