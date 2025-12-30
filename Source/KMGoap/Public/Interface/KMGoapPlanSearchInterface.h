// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "KMGoapPlanSearchInterface.generated.h"

class UKMGoapAgentComponent;
class UKMGoapAgentGoal;
class UKMGoapAgentAction;
struct FKMGoapActionPlan;

UINTERFACE(BlueprintType)
class KMGOAP_API UKMGoapPlanSearchInterface : public UInterface
{
	GENERATED_BODY()
};

class KMGOAP_API IKMGoapPlanSearchInterface
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="KMGoap|Planning")
	bool BuildPlan(
		UKMGoapAgentComponent* Agent,
		const TArray<UKMGoapAgentGoal*>& GoalsToCheck,
		UKMGoapAgentGoal* MostRecentGoal,
		FKMGoapActionPlan& OutPlan);
};
