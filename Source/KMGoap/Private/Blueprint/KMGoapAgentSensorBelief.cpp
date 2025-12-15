// Fill out your copyright notice in the Description page of Project Settings.


#include "Blueprint/KMGoapAgentSensorBelief.h"

#include "Blueprint/Component/KMGoapAgentComponent.h"
#include "Interface/KMGoapSensorInterface.h"

UActorComponent* UKMGoapAgentSensorBelief::GetCachedSensor() const
{
	return ResolveSensor();
}

UActorComponent* UKMGoapAgentSensorBelief::ResolveSensor() const
{
	if (CachedSensor.IsValid())
	{
		return CachedSensor.Get();
	}

	if (!SensorTag.IsValid())
	{
		return nullptr;
	}
	
	UKMGoapAgentComponent* AgentComp = GetTypedOuter<UKMGoapAgentComponent>();
	if (!AgentComp)
	{
		if (const AActor* OwnerActor = GetTypedOuter<AActor>())
		{
			AgentComp = OwnerActor->FindComponentByClass<UKMGoapAgentComponent>();
		}
	}

	if (!AgentComp)
	{
		return nullptr;
	}

	UActorComponent* Sensor = AgentComp->GetSensorByTag(SensorTag);
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

FVector UKMGoapAgentSensorBelief::SensorTargetPosition() const
{
	if (UActorComponent* Sensor = ResolveSensor())
	{
		return IKMGoapSensorInterface::Execute_GetTargetPosition(Sensor);
	}
	return FVector::ZeroVector;
}

bool UKMGoapAgentSensorBelief::Native_Condition() const
{
	if (UActorComponent* Sensor = ResolveSensor())
	{
		if (IKMGoapSensorInterface::Execute_HasTarget(Sensor))
		{
			return Super::Native_Condition();
		}
	}
	return false;
}

bool UKMGoapAgentSensorBelief::Condition_Implementation() const
{
	return true;
}

FVector UKMGoapAgentSensorBelief::Native_ObservedLocation() const
{
	if (bUseRawTargetLocation)
	{
		return SensorTargetPosition();
	}
	
	return Super::Native_ObservedLocation();
}
