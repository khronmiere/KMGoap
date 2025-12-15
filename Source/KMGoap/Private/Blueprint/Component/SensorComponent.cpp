// Fill out your copyright notice in the Description page of Project Settings.


#include "Blueprint/Component/SensorComponent.h"

#include "Blueprint/Component/AgentComponent.h"

void USensorComponent::BeginPlay()
{
	Super::BeginPlay();

#if WITH_EDITOR
	UWorld* World = GetWorld();
	if (!World || !World->IsGameWorld())
	{
		return;
	}
	FTimerManager& TimerManager = World->GetTimerManager();
	TimerManager.SetTimer(
		DebugDrawTimerHandle, 
		this, 
		&USensorComponent::DebugDraw, 
		DebugDrawInterval, 
		true);
#endif
}

void USensorComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
#if WITH_EDITOR
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

void USensorComponent::SetTarget(AActor* NewTarget)
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

FGameplayTag USensorComponent::GetTag_Implementation() const
{
	return SensorTag;
}

void USensorComponent::RegisterTargetChangedListener_Implementation(UObject* Listener, FName FunctionName)
{
	if (!Listener) return;

	FScriptDelegate Delegate;
	Delegate.BindUFunction(Listener, FunctionName);
	OnTargetChanged.AddUnique(Delegate);
}

void USensorComponent::UnregisterTargetChangedListener_Implementation(UObject* Listener, FName FunctionName)
{
	if (!Listener) return;

	FScriptDelegate Delegate;
	Delegate.BindUFunction(Listener, FunctionName);
	OnTargetChanged.Remove(Delegate);
}

bool USensorComponent::HasTarget_Implementation() const
{
	return TargetActor.IsValid();
}

FVector USensorComponent::GetTargetPosition_Implementation() const
{
	return TargetActor.IsValid() ? TargetActor->GetActorLocation() : FVector::ZeroVector;
}
