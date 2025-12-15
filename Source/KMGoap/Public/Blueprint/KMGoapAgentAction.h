// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "UObject/Object.h"
#include "KMGoapAgentAction.generated.h"

class UKMGoapAgentComponent;

UENUM(BlueprintType)
enum class EKMGoapActionStatus : uint8
{
	NotStarted,
	Running,
	Succeeded,
	Failed
};

/**
 * UAgentAction
 *
 * GOAP action definition + execution hook.
 * Authored as BP assets (one action per BP), instantiated at runtime by the agent.
 *
 * Public contract:
 * - Call StartAction / TickAction / StopAction from the agent executor.
 * - Preconditions/Effects are tags; agent resolves them to beliefs at runtime.
 */
UCLASS(Abstract, BlueprintType, Blueprintable, EditInlineNew, DefaultToInstanced, Category="KMGoap")
class KMGOAP_API UKMGoapAgentAction : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Action")
	FGameplayTag ActionTag;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Action", meta=(ClampMin="0.0"))
	float Cost = 1.f;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Action")
	FGameplayTagContainer Preconditions;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Action")
	FGameplayTagContainer Effects;
	
	UFUNCTION(BlueprintCallable, Category="Action")
	void StartAction(UKMGoapAgentComponent* Agent);

	UFUNCTION(BlueprintCallable, Category="Action")
	EKMGoapActionStatus TickAction(UKMGoapAgentComponent* Agent, float DeltaTime);

	UFUNCTION(BlueprintCallable, Category="Action")
	void StopAction(UKMGoapAgentComponent* Agent);

	UFUNCTION(BlueprintCallable, Category="Action")
	bool IsComplete() const { return Status == EKMGoapActionStatus::Succeeded || Status == EKMGoapActionStatus::Failed; }

	UFUNCTION(BlueprintCallable, Category="Action")
	EKMGoapActionStatus GetStatus() const { return Status; }
	
	UFUNCTION(BlueprintNativeEvent, Category="Action", meta=(BlueprintProtected="true"))
	bool CanPerform() const;
	virtual bool CanPerform_Implementation() const;

protected:
	UPROPERTY(Transient)
	EKMGoapActionStatus Status = EKMGoapActionStatus::NotStarted;
	
	UFUNCTION(BlueprintNativeEvent, Category="Action", meta=(BlueprintProtected="true"))
	void OnStart(UKMGoapAgentComponent* Agent);
	virtual void OnStart_Implementation(UKMGoapAgentComponent* Agent) {}

	UFUNCTION(BlueprintNativeEvent, Category="Action", meta=(BlueprintProtected="true"))
	EKMGoapActionStatus OnTick(UKMGoapAgentComponent* Agent, float DeltaTime);
	virtual EKMGoapActionStatus OnTick_Implementation(UKMGoapAgentComponent* Agent, float DeltaTime);

	UFUNCTION(BlueprintNativeEvent, Category="Action", meta=(BlueprintProtected="true"))
	void OnStop(UKMGoapAgentComponent* Agent);
	virtual void OnStop_Implementation(UKMGoapAgentComponent* Agent) {}
	
	void ApplyEffects(UKMGoapAgentComponent* Agent) const;
};
