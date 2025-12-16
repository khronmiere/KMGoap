// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Components/ActorComponent.h"
#include "Data/KMGoapActionPlan.h"
#include "Data/KMGoapCondition.h"
#include "KMGoapAgentComponent.generated.h"

struct FKMGoapCondition;
class UKMGoapAgentAction;
class UKMGoapBeliefSet;
class UKMGoapAgentGoal;
class UKMGoapAgentBelief;
class UKMGoapGoalSet;
class UKMGoapActionSet;
class UKMGoapSensorComponent;

USTRUCT(BlueprintType)
struct KMGOAP_API FKMGoapBeliefCacheEntry
{
	GENERATED_BODY()
	
	FGameplayTag BeliefTag;
	bool bValue = false;
};

UCLASS(ClassGroup=(KMGoap), meta=(BlueprintSpawnableComponent))
class KMGOAP_API UKMGoapAgentComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UKMGoapAgentComponent();
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="GOAP")
	TObjectPtr<UKMGoapBeliefSet> BeliefSet;
	UPROPERTY(Transient, BlueprintReadOnly, Category="GOAP")
	TMap<FGameplayTag, TObjectPtr<UKMGoapAgentBelief>> BeliefsByTag;
	
	UPROPERTY(Transient, BlueprintReadOnly, Category="GOAP")
	TMap<FGameplayTag, TObjectPtr<UActorComponent>> SensorsByTag;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="GOAP|Sets")
	TObjectPtr<UKMGoapActionSet> ActionSet;
	UPROPERTY(Transient, BlueprintReadOnly, Category="GOAP")
	TMap<FGameplayTag, TObjectPtr<UKMGoapAgentAction>> ActionsByTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="GOAP|Sets")
	TObjectPtr<UKMGoapGoalSet> GoalSet;
	UPROPERTY(Transient, BlueprintReadOnly, Category="GOAP")
	TMap<FGameplayTag, TObjectPtr<UKMGoapAgentGoal>> GoalsByTag;
	
	UPROPERTY(BlueprintReadOnly, Category="GOAP|Runtime")
	TObjectPtr<UKMGoapAgentGoal> CurrentGoal = nullptr;

	UPROPERTY(BlueprintReadOnly, Category="GOAP|Runtime")
	TObjectPtr<UKMGoapAgentGoal> LastGoal = nullptr;

	UPROPERTY(BlueprintReadOnly, Category="GOAP|Runtime")
	FKMGoapActionPlan CurrentPlan;

	UPROPERTY(BlueprintReadOnly, Category="GOAP|Runtime")
	TObjectPtr<UKMGoapAgentAction> CurrentAction = nullptr;
	
	UFUNCTION(BlueprintCallable, Category="GOAP|Beliefs")
	UKMGoapAgentBelief* GetBeliefByTag(FGameplayTag Tag) const;

	UFUNCTION(BlueprintCallable, Category="GOAP|Sensors")
	UActorComponent* GetSensorByTag(FGameplayTag Tag) const;
	
	UFUNCTION(BlueprintCallable, Category="GOAP|Sensors")
	UKMGoapAgentGoal* GetGoalByTag(FGameplayTag Tag) const;
	
	UFUNCTION(BlueprintCallable, Category="GOAP|Sensors")
	UKMGoapAgentAction* GetActionByTag(FGameplayTag Tag) const;
	
	UFUNCTION(BlueprintCallable, Category="GOAP|Beliefs")
	bool EvaluateBeliefByTag(FGameplayTag Tag) const;

	UFUNCTION(BlueprintCallable, Category="GOAP|Beliefs")
	FVector GetBeliefLocationByTag(FGameplayTag Tag) const;
	
	UFUNCTION(BlueprintCallable, Category="GOAP|Facts")
	void SetFact(FGameplayTag FactTag, bool bAdd = true);

	UFUNCTION(BlueprintCallable, Category="GOAP|Facts")
	EKMGoapFactState GetFact(FGameplayTag Tag) const;
	
	TArray<FGameplayTag> GetFactsTags() const;
	
	bool ValidateActionPreconditions(const UKMGoapAgentAction* Action) const;
	void UpdateBeliefEvaluationCache();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION(BlueprintNativeEvent, Category="GOAP|Runtime")
	void OnSensorTargetChanged(FGameplayTag SensorTag);
	virtual void OnSensorTargetChanged_Implementation(FGameplayTag SensorTag){}

private:
	UPROPERTY(Transient)
	TMap<FGameplayTag, bool> Facts;
	
	UPROPERTY(Transient)
	TMap<FGameplayTag, FKMGoapBeliefCacheEntry> BeliefCache;
	
	FTimerHandle BeliefEvaluateTimerHandle;
	UPROPERTY(EditAnywhere, Category="GOAP|Runtime")
	float EvaluateBeliefTimeStep = 0.03f;
	
	void BuildBeliefs();
	void ClearBeliefs();
	void BuildActions();
	void BuildGoals();
	void CacheSensors();
	void ClearSensors();

	void BindSensorEvents(UActorComponent* Sensor);
	void UnbindSensorEvents(UActorComponent* Sensor);
	
	void FilterFactSatisfiedPreconditions(TSet<FKMGoapCondition>& Preconditions) const;
	void FilterBeliefSatisfiedPreconditions(TSet<FKMGoapCondition>& Preconditions) const;

	UFUNCTION()
	void HandleSensorTargetChanged(FGameplayTag SourceTag);
	UFUNCTION()
	void EvaluateBeliefs();
	
	void CalculatePlan();
	void UpdateExecutionState();
	void ResetExecutionState();
	bool ComputePlanForGoals(const TArray<UKMGoapAgentGoal*>& GoalsToCheck, FKMGoapActionPlan& OutPlan);
	
	template <typename TObjectType, typename TTagGetter>
	static void BuildTaggedObjects(
		UObject* Outer,
		const TArray<TSoftClassPtr<TObjectType>>& Classes,
		TMap<FGameplayTag, TObjectPtr<TObjectType>>& OutMap,
		TTagGetter&& GetTag)
	{
		OutMap.Reset();

		for (const TSoftClassPtr<TObjectType>& SoftClass : Classes)
		{
			if (SoftClass.IsNull())
			{
				continue;
			}

			UClass* LoadedClass = SoftClass.LoadSynchronous();
			if (!LoadedClass)
			{
				continue;
			}

			TObjectType* Obj = NewObject<TObjectType>(Outer, LoadedClass);
			if (!Obj)
			{
				continue;
			}

			const FGameplayTag Tag = GetTag(Obj);
			if (!Tag.IsValid())
			{
				continue;
			}

			// Optional: detect duplicates
			if (OutMap.Contains(Tag))
			{
				UE_LOG(LogTemp, Warning, TEXT("GOAP Agent: Duplicate tag [%s] for %s. Skipping."),
					*Tag.ToString(), *GetNameSafe(LoadedClass));
				continue;
			}

			OutMap.Add(Tag, Obj);
		}
	}
};
