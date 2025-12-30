// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Data/KMGoapActionPlan.h"
#include "Interface/KMGoapAgentStateMachineInterface.h"
#include "UObject/Object.h"
#include "KMGoapDefaultStateMachine.generated.h"

/**
 * 
 */
UCLASS()
class KMGOAP_API UKMGoapDefaultStateMachine : public UObject, public IKMGoapAgentStateMachineInterface
{
	GENERATED_BODY()
	
public:
	virtual void Start_Implementation(UKMGoapAgentComponent* NewAgent) override;
	
	virtual void Stop_Implementation() override;
	
	virtual void Tick_Implementation(float DeltaTime) override;
	
	virtual void Reset_Implementation() override;
	
	virtual void OnSensorStateUpdate_Implementation() override;

protected:
	UPROPERTY(Transient)
	TObjectPtr<UKMGoapAgentComponent> Agent;
	
	UPROPERTY(BlueprintReadOnly, Category="GOAP|Runtime")
	TObjectPtr<UKMGoapAgentAction> CurrentAction = nullptr;
	
	UPROPERTY(BlueprintReadOnly, Category="GOAP|Runtime")
	TObjectPtr<UKMGoapAgentGoal> CurrentGoal = nullptr;

	UPROPERTY(BlueprintReadOnly, Category="GOAP|Runtime")
	TObjectPtr<UKMGoapAgentGoal> LastGoal = nullptr;

	UPROPERTY(BlueprintReadOnly, Category="GOAP|Runtime")
	FKMGoapActionPlan CurrentPlan;
	
	void CalculatePlan();
	void UpdateExecutionState();
	void ResetExecutionState();
};
