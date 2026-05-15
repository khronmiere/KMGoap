// All rights reserved by Khrönmière Entertainment.
#pragma once

#include "CoreMinimal.h"
#include "KMGoapAgentBelief.h"
#include "KMGoapAgentSensorBelief.generated.h"

/**
 * Belief implementation backed by a GOAP sensor component.
 *
 * This belief resolves a cached sensor by gameplay tag and uses the sensor state
 * to determine whether a target exists and where that target is located.
 */
UCLASS(Abstract, Category="KMGoap", Blueprintable)
class KMGOAP_API UKMGoapAgentSensorBelief : public UKMGoapAgentBelief
{
	GENERATED_BODY()

public:
	/**
	 * Gameplay tag identifying the sensor component that supplies this belief's data.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Belief")
	FGameplayTag SensorTag;
	
	/**
	 * Gets the cached sensor component for the supplied agent, resolving it if needed.
	 *
	 * @param Agent Agent component that owns the sensor cache.
	 * @return Sensor component matching SensorTag, or nullptr if no sensor is available.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="KMGoap")
	UActorComponent* GetCachedSensor(const UKMGoapAgentComponent* Agent) const;

protected:
	/**
	 * Runtime weak reference to the resolved sensor component.
	 */
	UPROPERTY(Transient)
	TWeakObjectPtr<UActorComponent> CachedSensor;
	
	/**
	 * When true, the belief location is read directly from the sensor target position.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Belief")
	bool bUseRawTargetLocation;
	
	/**
	 * Resolves the sensor component for the supplied agent.
	 *
	 * @param Agent Agent component that owns the sensor cache.
	 * @return Sensor component matching SensorTag, or nullptr if no sensor is available.
	 */
	UActorComponent* ResolveSensor(const UKMGoapAgentComponent* Agent) const;
	
	/**
	 * Gets the current target position reported by the resolved sensor.
	 *
	 * @param Agent Agent component that owns the sensor cache.
	 * @return Sensor target position, or FVector::ZeroVector when unavailable.
	 */
	FVector SensorTargetPosition(const UKMGoapAgentComponent* Agent) const;
	
	/**
	 * Evaluates whether the associated sensor currently satisfies this belief.
	 *
	 * @param Agent Agent component evaluating this belief.
	 * @return True when the sensor state satisfies the belief, otherwise false.
	 */
	virtual bool Native_Condition(const UKMGoapAgentComponent* Agent) const override;

	/**
	 * Default condition implementation for sensor-backed beliefs.
	 *
	 * @param Agent Agent component evaluating this belief.
	 * @return True when the sensor state satisfies the belief, otherwise false.
	 */
	virtual bool Condition_Implementation(const UKMGoapAgentComponent* Agent) const override;

	/**
	 * Gets the observed location supplied by the associated sensor.
	 *
	 * @param Agent Agent component requesting the observed location.
	 * @return Observed target location, or FVector::ZeroVector when unavailable.
	 */
	virtual FVector Native_ObservedLocation(const UKMGoapAgentComponent* Agent) const override;
};
