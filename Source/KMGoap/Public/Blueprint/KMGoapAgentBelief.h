// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "UObject/Object.h"
#include "KMGoapAgentBelief.generated.h"

/**
 * UAgentBelief
 *
 * A GOAP “belief” is a queryable fact about the world as perceived by an agent.
 * Beliefs can be implemented in C++ (by overriding the native implementations),
 * or in Blueprint (by overriding the events).
 *
 * Important:
 * - Gameplay code should call Evaluate() / GetLocation().
 * - Condition() / ObservedLocation() exist for override points only and are not
 *   meant to be called directly from Blueprints.
 */
UCLASS(Abstract, Category="KMGoap", Blueprintable)
class KMGOAP_API UKMGoapAgentBelief : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Belief")
	FGameplayTag BeliefTag;
	
	UFUNCTION(BlueprintCallable, Category="Belief")
	FVector GetLocation() const { return Native_ObservedLocation(); }
	
	UFUNCTION(BlueprintCallable, Category="Belief")
	bool Evaluate() const { return Native_Condition(); }
	
protected:
	virtual bool Native_Condition() const;
	UFUNCTION(BlueprintNativeEvent, Category="Belief")
	bool Condition() const;
	virtual bool Condition_Implementation() const;
	
	virtual FVector Native_ObservedLocation() const;
	UFUNCTION(BlueprintNativeEvent, Category="Belief")
	FVector ObservedLocation() const;
	virtual FVector ObservedLocation_Implementation() const;
};
