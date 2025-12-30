// Fill out your copyright notice in the Description page of Project Settings.


#include "Blueprint/KMGoapAgentSensorBelief.h"

#include "Blueprint/Component/KMGoapAgentComponent.h"
#include "Interface/KMGoapSensorInterface.h"

UActorComponent* UKMGoapAgentSensorBelief::GetCachedSensor(const UKMGoapAgentComponent* Agent) const
{
	return ResolveSensor(Agent);
}

UActorComponent* UKMGoapAgentSensorBelief::ResolveSensor(const UKMGoapAgentComponent* Agent) const
{
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

	if (!Sensor->GetClass()->ImplementsInterface(UKMGoapSensorInterface::StaticClass()))
	{
		return nullptr;
	}

	// Cache it
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
	return true;
}

FVector UKMGoapAgentSensorBelief::Native_ObservedLocation(const UKMGoapAgentComponent* Agent) const
{
	if (bUseRawTargetLocation)
	{
		return SensorTargetPosition(Agent);
	}
	
	return Super::Native_ObservedLocation(Agent);
}
