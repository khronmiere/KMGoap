// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "KMGoapPlannerInterface.generated.h"

// This class does not need to be modified.
UINTERFACE()
class UKMGoapPlannerInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class KMGOAP_API IKMGoapPlannerInterface
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "GOAP")
	bool Plan(
		class UKMGoapAgentComponent* Agent,
		const TArray<class UKMGoapAgentGoal*>& GoalsToCheck,
		UKMGoapAgentGoal* MostRecentGoal,
		struct FKMGoapActionPlan& OutPlan);
};
