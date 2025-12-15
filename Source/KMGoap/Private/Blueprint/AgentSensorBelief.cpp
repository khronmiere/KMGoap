// Fill out your copyright notice in the Description page of Project Settings.


#include "Blueprint/AgentSensorBelief.h"

#include "Blueprint/Component/AgentComponent.h"
#include "Interface/SensorInterface.h"

UActorComponent* UAgentSensorBelief::GetCachedSensor() const
{
	return ResolveSensor();
}

UActorComponent* UAgentSensorBelief::ResolveSensor() const
{
	if (CachedSensor.IsValid())
	{
		return CachedSensor.Get();
	}

	if (!SensorTag.IsValid())
	{
		return nullptr;
	}
	
	UAgentComponent* AgentComp = GetTypedOuter<UAgentComponent>();
	if (!AgentComp)
	{
		if (const AActor* OwnerActor = GetTypedOuter<AActor>())
		{
			AgentComp = OwnerActor->FindComponentByClass<UAgentComponent>();
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

	if (!Sensor->GetClass()->ImplementsInterface(USensorInterface::StaticClass()))
	{
		return nullptr;
	}

	// Cache it
	const_cast<UAgentSensorBelief*>(this)->CachedSensor = Sensor;
	return Sensor;
}

FVector UAgentSensorBelief::SensorTargetPosition() const
{
	if (UActorComponent* Sensor = ResolveSensor())
	{
		return ISensorInterface::Execute_GetTargetPosition(Sensor);
	}
	return FVector::ZeroVector;
}

bool UAgentSensorBelief::Native_Condition() const
{
	if (UActorComponent* Sensor = ResolveSensor())
	{
		if (ISensorInterface::Execute_HasTarget(Sensor))
		{
			return Super::Native_Condition();
		}
	}
	return false;
}

bool UAgentSensorBelief::Condition_Implementation() const
{
	return true;
}

FVector UAgentSensorBelief::Native_ObservedLocation() const
{
	if (bUseRawTargetLocation)
	{
		return SensorTargetPosition();
	}
	
	return Super::Native_ObservedLocation();
}
