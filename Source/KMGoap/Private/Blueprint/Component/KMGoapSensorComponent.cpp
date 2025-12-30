// Fill out your copyright notice in the Description page of Project Settings.


#include "Blueprint/Component/KMGoapSensorComponent.h"

#include "Blueprint/Component/KMGoapAgentComponent.h"

void UKMGoapSensorComponent::BeginPlay()
{
	Super::BeginPlay();

#if WITH_EDITORONLY_DATA
	UWorld* World = GetWorld();
	if (!World || !World->IsGameWorld())
	{
		return;
	}
	FTimerManager& TimerManager = World->GetTimerManager();
	TimerManager.SetTimer(
		DebugDrawTimerHandle, 
		this, 
		&UKMGoapSensorComponent::DebugDraw, 
		DebugDrawInterval, 
		true);
#endif
}

void UKMGoapSensorComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
#if WITH_EDITORONLY_DATA
	if (!DebugDrawTimerHandle.IsValid())
	{
		return;
	}
	
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}
	FTimerManager& TimerManager = World->GetTimerManager();
	TimerManager.ClearTimer(DebugDrawTimerHandle);
#endif
	Super::EndPlay(EndPlayReason);
}

void UKMGoapSensorComponent::SetTarget(AActor* NewTarget)
{
	const FVector NewPos = NewTarget ? NewTarget->GetActorLocation() : FVector::ZeroVector;

	const bool bChanged =
		(TargetActor.Get() != NewTarget) ||
		(LastKnownPosition != NewPos);

	TargetActor = NewTarget;
	LastKnownPosition = NewPos;

	if (bChanged)
	{
		OnTargetChanged.Broadcast(SensorTag);
	}
}

FGameplayTag UKMGoapSensorComponent::GetTag_Implementation() const
{
	return SensorTag;
}

void UKMGoapSensorComponent::RegisterTargetChangedListener_Implementation(UObject* Listener, FName FunctionName)
{
	if (!Listener) return;

	FScriptDelegate Delegate;
	Delegate.BindUFunction(Listener, FunctionName);
	OnTargetChanged.AddUnique(Delegate);
}

void UKMGoapSensorComponent::UnregisterTargetChangedListener_Implementation(UObject* Listener, FName FunctionName)
{
	if (!Listener) return;

	FScriptDelegate Delegate;
	Delegate.BindUFunction(Listener, FunctionName);
	OnTargetChanged.Remove(Delegate);
}

bool UKMGoapSensorComponent::HasTarget_Implementation() const
{
	return TargetActor.IsValid();
}

AActor* UKMGoapSensorComponent::GetTarget_Implementation() const
{
	if (!HasTarget())
	{
		return nullptr;
	}
	
	return TargetActor.Get();
}

FVector UKMGoapSensorComponent::GetTargetPosition_Implementation() const
{
	return TargetActor.IsValid() ? TargetActor->GetActorLocation() : FVector::ZeroVector;
}
