// All rights reserved by Khrönmière Entertainment.
#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "KMGoapPlanSearchInterface.generated.h"

class UKMGoapAgentComponent;
class UKMGoapAgentGoal;
class UKMGoapAgentAction;
struct FKMGoapActionPlan;

/**
 * Unreal reflection wrapper for GOAP plan search algorithm implementations.
 *
 * This interface type allows Blueprint and C++ objects to be recognized by Unreal
 * as valid implementations of the GOAP planning search contract.
 */
UINTERFACE(BlueprintType)
class KMGOAP_API UKMGoapPlanSearchInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * Defines the contract for objects that can build an action plan for a GOAP agent.
 *
 * Implementations evaluate candidate goals for the supplied agent and attempt to
 * produce an ordered action plan that satisfies the selected goal.
 */
class KMGOAP_API IKMGoapPlanSearchInterface
{
	GENERATED_BODY()

public:
	/**
	 * Attempts to build a valid GOAP action plan for one of the supplied goals.
	 *
	 * @param Agent The GOAP agent component requesting a plan.
	 * @param GoalsToCheck Candidate goals that should be evaluated by the search algorithm.
	 * @param MostRecentGoal The last goal completed or selected by the agent, used to avoid undesirable immediate repeats.
	 * @param OutPlan The resulting action plan when planning succeeds.
	 * @return True if a valid plan was found and written to OutPlan; otherwise false.
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="KMGoap|Planning")
	bool BuildPlan(
		UKMGoapAgentComponent* Agent,
		const TArray<UKMGoapAgentGoal*>& GoalsToCheck,
		UKMGoapAgentGoal* MostRecentGoal,
		FKMGoapActionPlan& OutPlan);
};
