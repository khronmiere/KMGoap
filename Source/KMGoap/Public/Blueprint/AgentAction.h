// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "UObject/Object.h"
#include "AgentAction.generated.h"

class UAgentComponent;

UENUM(BlueprintType)
enum class EActionStatus : uint8
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
class KMGOAP_API UAgentAction : public UObject
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
	void StartAction(UAgentComponent* Agent);

	UFUNCTION(BlueprintCallable, Category="Action")
	EActionStatus TickAction(UAgentComponent* Agent, float DeltaTime);

	UFUNCTION(BlueprintCallable, Category="Action")
	void StopAction(UAgentComponent* Agent);

	UFUNCTION(BlueprintCallable, Category="Action")
	bool IsComplete() const { return Status == EActionStatus::Succeeded || Status == EActionStatus::Failed; }

	UFUNCTION(BlueprintCallable, Category="Action")
	EActionStatus GetStatus() const { return Status; }
	
	UFUNCTION(BlueprintNativeEvent, Category="Action", meta=(BlueprintProtected="true"))
	bool CanPerform() const;
	virtual bool CanPerform_Implementation() const;

protected:
	UPROPERTY(Transient)
	EActionStatus Status = EActionStatus::NotStarted;
	
	UFUNCTION(BlueprintNativeEvent, Category="Action", meta=(BlueprintProtected="true"))
	void OnStart(UAgentComponent* Agent);
	virtual void OnStart_Implementation(UAgentComponent* Agent) {}

	UFUNCTION(BlueprintNativeEvent, Category="Action", meta=(BlueprintProtected="true"))
	EActionStatus OnTick(UAgentComponent* Agent, float DeltaTime);
	virtual EActionStatus OnTick_Implementation(UAgentComponent* Agent, float DeltaTime);

	UFUNCTION(BlueprintNativeEvent, Category="Action", meta=(BlueprintProtected="true"))
	void OnStop(UAgentComponent* Agent);
	virtual void OnStop_Implementation(UAgentComponent* Agent) {}
	
	void ApplyEffects(UAgentComponent* Agent) const;
};
