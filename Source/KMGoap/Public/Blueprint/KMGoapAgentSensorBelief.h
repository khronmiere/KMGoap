// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "KMGoapAgentBelief.h"
#include "KMGoapAgentSensorBelief.generated.h"

/**
 * 
 */
UCLASS(Abstract, Category="KMGoap", Blueprintable)
class KMGOAP_API UKMGoapAgentSensorBelief : public UKMGoapAgentBelief
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Belief")
	FGameplayTag SensorTag;
	
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="KMGoap")
	UActorComponent* GetCachedSensor() const;

protected:
	// Cached resolved sensor (runtime)
	UPROPERTY(Transient)
	TWeakObjectPtr<UActorComponent> CachedSensor;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Belief")
	bool bUseRawTargetLocation;
	
	UActorComponent* ResolveSensor() const;
	
	FVector SensorTargetPosition() const;
	
	virtual bool Native_Condition() const override;
	virtual bool Condition_Implementation() const override;
	virtual FVector Native_ObservedLocation() const override;
};
