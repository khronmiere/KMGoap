// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "SensorInterface.generated.h"

// This class does not need to be modified.
UINTERFACE(Category = "KMGoap|Interface", BlueprintType)
class USensorInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class KMGOAP_API ISensorInterface
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="KMGoap|Sensor")
	FGameplayTag GetTag() const;
	
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="KMGoap|Sensor")
	void RegisterTargetChangedListener(UObject* Listener, FName FunctionName);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="KMGoap|Sensor")
	void UnregisterTargetChangedListener(UObject* Listener, FName FunctionName);
	
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="KMGoap|Sensor")
	bool HasTarget() const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="KMGoap|Sensor")
	FVector GetTargetPosition() const;
};
