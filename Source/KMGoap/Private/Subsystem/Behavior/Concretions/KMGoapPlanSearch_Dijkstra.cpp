#include "Subsystem/Behavior/Concretions/KMGoapPlanSearch_Dijkstra.h"

#include "Blueprint/Component/KMGoapAgentComponent.h"
#include "Blueprint/KMGoapAgentAction.h"
#include "Blueprint/KMGoapAgentGoal.h"
#include "Data/KMGoapCondition.h"

#include "Algo/Reverse.h"
#include "Containers/Map.h"
#include "Containers/Set.h"
#include "Data/KMGoapActionPlan.h"
#include "HAL/PlatformTime.h"
#include "Subsystem/Data/KMGoapPlanningTypes.h"

DEFINE_LOG_CATEGORY_STATIC(LogGoapDijkstra, Log, All);

// Same bias you had in the subsystem (slight preference for most recent goal).
static constexpr float RecentGoalBias = 0.01f;

bool UKMGoapPlanSearch_Dijkstra::BuildPlan_Implementation(
	UKMGoapAgentComponent* Agent,
	const TArray<UKMGoapAgentGoal*>& GoalsToCheck,
	UKMGoapAgentGoal* MostRecentGoal,
	FKMGoapActionPlan& OutPlan)
{
	return false;
}

bool UKMGoapPlanSearch_Dijkstra::BuildContext(UKMGoapAgentComponent* Agent,
	const TArray<UKMGoapAgentGoal*>& GoalsToCheck, UKMGoapAgentGoal* MostRecentGoal,
	TArray<UKMGoapAgentGoal*>& OutGoalsSorted, FKMGoapPlanningContext& OutCtx) const
{
	return false;
}

bool UKMGoapPlanSearch_Dijkstra::SolveGoalDijkstra(const FKMGoapPlanningContext& Ctx, UKMGoapAgentGoal* Goal,
	FKMGoapActionPlan& OutPlan) const
{
	return false;
}

bool UKMGoapPlanSearch_Dijkstra::SatisfiesAll(const FKMGoapSimState& State, const TSet<FKMGoapCondition>& Conditions)
{
	return false;
}

uint32 UKMGoapPlanSearch_Dijkstra::HashState(const FKMGoapSimState& State)
{
	return 0;
}

void UKMGoapPlanSearch_Dijkstra::ApplyPostconditions(FKMGoapSimState& State, const TSet<FKMGoapCondition>& Post)
{
	for (const FKMGoapCondition& C : Post)
	{
		State.Set(C.Tag, C.bValue);
	}
}
