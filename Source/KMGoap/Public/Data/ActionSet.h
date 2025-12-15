// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ActionSet.generated.h"

/**
 * 
 */
UCLASS(Category = "KMGoap|Data", BlueprintType)
class KMGOAP_API UActionSet : public UDataAsset
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TArray<TSoftClassPtr<class UAgentAction>> Actions;
};
