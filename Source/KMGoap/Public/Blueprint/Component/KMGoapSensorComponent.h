// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Interface/KMGoapSensorInterface.h"
#include "UObject/Object.h"
#include "KMGoapSensorComponent.generated.h"

/**
 * 
 */
UCLASS(ClassGroup=(KMGoap), Category = "KMGoap|ActorComponents", Abstract, BlueprintType, Blueprintable, meta=(BlueprintSpawnableComponent))
class KMGOAP_API UKMGoapSensorComponent : public UActorComponent, public IKMGoapSensorInterface
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Sensor")
	FGameplayTag SensorTag;

	// Similar to OnTargetChanged
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTargetChanged, FGameplayTag, SensorTag);

	UPROPERTY(BlueprintAssignable, Category="Sensor")
	FOnTargetChanged OnTargetChanged;

	// Default interface behavior (derived classes set these)
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Sensor")
	AActor* GetTargetActor() const { return TargetActor.Get(); }

protected:
	UPROPERTY(Transient)
	TWeakObjectPtr<AActor> TargetActor;

	UPROPERTY(Transient)
	FVector LastKnownPosition = FVector::ZeroVector;

#if WITH_EDITORONLY_DATA
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Sensor|Debug", meta=(ClampMin="0.01"))
	float DebugDrawInterval = 0.1f;
	FTimerHandle DebugDrawTimerHandle;
#endif

	UFUNCTION(BlueprintNativeEvent, Category="Sensor|Debug", meta=(BlueprintProtected="true"))
	void DebugDraw();
	virtual void DebugDraw_Implementation() {}
	
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	
	// Helper for derived sensors to update state and broadcast once
	void SetTarget(AActor* NewTarget);
	
	virtual FGameplayTag GetTag_Implementation() const override;
	virtual void RegisterTargetChangedListener_Implementation(UObject* Listener, FName FunctionName) override;
	virtual void UnregisterTargetChangedListener_Implementation(UObject* Listener, FName FunctionName) override;
	virtual bool HasTarget_Implementation() const override;
	virtual FVector GetTargetPosition_Implementation() const override;
};
