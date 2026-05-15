// All rights reserved by Khrönmière Entertainment.
#include "Blueprint/KMGoapAgentSensorBelief.h"

#include "Blueprint/Component/KMGoapAgentComponent.h"
#include "Interface/KMGoapSensorInterface.h"

UActorComponent* UKMGoapAgentSensorBelief::GetCachedSensor(const UKMGoapAgentComponent* Agent) const
{
	return ResolveSensor(Agent);
}

UActorComponent* UKMGoapAgentSensorBelief::ResolveSensor(const UKMGoapAgentComponent* Agent) const
{
	// Sensor lookup is cached because beliefs may be evaluated frequently by planning and runtime checks.
	// The cache is weak so destroyed components do not remain pinned by the belief.
	if (CachedSensor.IsValid())
	{
		return CachedSensor.Get();
	}

	if (!SensorTag.IsValid() || !Agent)
	{
		return nullptr;
	}
	
	UActorComponent* Sensor = Agent->GetSensorByTag(SensorTag);
	if (!Sensor)
	{
		return nullptr;
	}

	// The tag finds a component candidate; the interface check verifies it exposes the sensor contract
	// this belief depends on.
	if (!Sensor->GetClass()->ImplementsInterface(UKMGoapSensorInterface::StaticClass()))
	{
		return nullptr;
	}

	const_cast<UKMGoapAgentSensorBelief*>(this)->CachedSensor = Sensor;
	return Sensor;
}

FVector UKMGoapAgentSensorBelief::SensorTargetPosition(const UKMGoapAgentComponent* Agent) const
{
	if (UActorComponent* Sensor = ResolveSensor(Agent))
	{
		return IKMGoapSensorInterface::Execute_GetTargetPosition(Sensor);
	}
	return FVector::ZeroVector;
}

bool UKMGoapAgentSensorBelief::Native_Condition(const UKMGoapAgentComponent* Agent) const
{
	// Sensor-backed beliefs are only true candidates when the sensor currently has a target. The base
	// condition then applies any additional custom filtering supplied by subclasses or Blueprints.
	if (UActorComponent* Sensor = ResolveSensor(Agent))
	{
		if (IKMGoapSensorInterface::Execute_HasTarget(Sensor))
		{
			return Super::Native_Condition(Agent);
		}
	}
	return false;
}

bool UKMGoapAgentSensorBelief::Condition_Implementation(const UKMGoapAgentComponent* Agent) const
{
	// Once a valid sensor target exists, the default sensor belief is satisfied. Override this to add
	// range checks, team checks, line-of-sight rules, or other domain-specific constraints.
	return true;
}

FVector UKMGoapAgentSensorBelief::Native_ObservedLocation(const UKMGoapAgentComponent* Agent) const
{
	// Some consumers want the sensor's raw target position, while others want the belief's overridden
	// observed location. This flag allows the asset to choose without changing calling code.
	if (bUseRawTargetLocation)
	{
		return SensorTargetPosition(Agent);
	}
	
	return Super::Native_ObservedLocation(Agent);
}
