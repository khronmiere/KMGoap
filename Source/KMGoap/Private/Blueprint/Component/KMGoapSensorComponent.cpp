// All rights reserved by Khrönmière Entertainment.
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

	// Debug drawing is editor-only and timer-driven so sensors can visualize state
	// without requiring the component itself to tick every frame.
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

	// Clear the editor debug timer explicitly so it cannot call back into a component
	// that is being destroyed or removed from play.
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

	// Consumers care about meaningful sensor-state changes, not every assignment.
	// Position is included so listeners can react when the same target is perceived
	// at a new location.
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

	// Sensors expose change notifications through the interface so agent components
	// do not need to know the concrete sensor implementation.
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
	if (!TargetActor.IsValid())
	{
		return nullptr;
	}
	
	return TargetActor.Get();
}

FVector UKMGoapSensorComponent::GetTargetPosition_Implementation() const
{
	return TargetActor.IsValid() ? TargetActor->GetActorLocation() : FVector::ZeroVector;
}
