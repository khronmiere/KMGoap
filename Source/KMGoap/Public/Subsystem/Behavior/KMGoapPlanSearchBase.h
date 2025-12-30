// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Interface/KMGoapPlanSearchInterface.h"
#include "UObject/Object.h"
#include "KMGoapPlanSearchBase.generated.h"

/**
 * 
 */
UCLASS(Abstract, Blueprintable, EditInlineNew, DefaultToInstanced)
class KMGOAP_API UKMGoapPlanSearchBase : public UObject, public IKMGoapPlanSearchInterface
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="KMGoap|Planning")
	int32 MaxExpandedNodes = 5000;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="KMGoap|Planning")
	int32 MaxDepth = 64;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="KMGoap|Planning")
	float TimeBudgetMs = 2.0f;

protected:
	UFUNCTION(Blueprintable, BlueprintPure, Category="KMGoap|Planning")
	bool IsBudgetExceeded(const double StartSeconds, int32 ExpandedNodes) const
	{
		if (ExpandedNodes >= MaxExpandedNodes) return true;
		const double ElapsedMs = (FPlatformTime::Seconds() - StartSeconds) * 1000.0;
		return ElapsedMs >= TimeBudgetMs;
	}
};
