#include "Subsystem/KMGoapPlannerSubsystem.h"

#include "Blueprint/Component/KMGoapAgentComponent.h"
#include "Blueprint/KMGoapAgentGoal.h"
#include "Blueprint/KMGoapAgentAction.h"

DEFINE_LOG_CATEGORY_STATIC(LogGoapPlanner, Log, All);

static constexpr float RecentGoalBias = 0.01f;

bool UKMGoapPlannerSubsystem::Plan(
	UKMGoapAgentComponent* Agent,
	const TArray<UKMGoapAgentGoal*>& GoalsToCheck,
	UKMGoapAgentGoal* MostRecentGoal,
	FKMGoapActionPlan& OutPlan)
{
	OutPlan.Reset();

	if (!Agent)
	{
		UE_LOG(LogGoapPlanner, Warning, TEXT("Plan: Agent is null."));
		return false;
	}

	// Collect candidate goals (filter: only goals that are not already satisfied)
	TArray<UKMGoapAgentGoal*> CandidateGoals;
	CandidateGoals.Reserve(GoalsToCheck.Num());

	for (UKMGoapAgentGoal* Goal : GoalsToCheck)
	{
		if (!Goal) continue;

		// Only consider goals that have at least one desired effect not satisfied
		bool bAnyUnsatisfied = false;
		for (const FGameplayTag& DesiredTag : Goal->DesiredEffects)
		{
			if (!IsTagSatisfied(Agent, DesiredTag))
			{
				bAnyUnsatisfied = true;
				break;
			}
		}

		if (bAnyUnsatisfied)
		{
			CandidateGoals.Add(Goal);
		}
	}

	if (CandidateGoals.Num() == 0)
	{
		UE_LOG(LogGoapPlanner, Verbose, TEXT("Plan: No candidate goals (all satisfied or none provided)."));
		return false;
	}

	SortGoals(Agent, CandidateGoals, MostRecentGoal);

	// Build list of actions available to this agent
	TArray<UKMGoapAgentAction*> Actions;
	Actions.Reserve(Agent->ActionsByTag.Num());

	for (const auto& Pair : Agent->ActionsByTag)
	{
		if (UKMGoapAgentAction* Action = Pair.Value)
		{
			// Optional: let actions gate themselves
			// If you don't have CanPerform yet, delete this block or return true by default in Action.
			if (!Action->CanPerform())
			{
				continue;
			}
			Actions.Add(Action);
		}
	}

	if (Actions.Num() == 0)
	{
		UE_LOG(LogGoapPlanner, Warning, TEXT("Plan: Agent has no usable actions."));
		return false;
	}

	SortActionsByCost(Actions);

	// Try goals in priority order
	for (UKMGoapAgentGoal* Goal : CandidateGoals)
	{
		if (!Goal) continue;

		// Root node: required effects = goal desired effects
		TSharedPtr<FGoapNode> Root = MakeShared<FGoapNode>();
		Root->Parent = nullptr;
		Root->Action = nullptr;
		Root->Required = Goal->DesiredEffects;
		Root->Cost = 0.f;

		// Remove already satisfied requirements
		RemoveSatisfied(Agent, Root->Required);

		// If already satisfied after removing, skip (should be filtered out above, but safe)
		if (Root->Required.Num() == 0)
		{
			continue;
		}

		if (!FindPath(Agent, Root, Actions))
		{
			continue;
		}

		// If solved but leaf-dead (Unity’s “goalNode.IsLeafDead”), skip
		if (Root->IsLeafDead())
		{
			continue;
		}

		// Extract plan by walking down cheapest leaves repeatedly (Unity behavior)
		TArray<TObjectPtr<UKMGoapAgentAction>> ForwardActions;
		TSharedPtr<FGoapNode> Cursor = Root;

		while (Cursor.IsValid() && Cursor->Leaves.Num() > 0)
		{
			// Pick cheapest leaf at this level
			TSharedPtr<FGoapNode> Cheapest = Cursor->Leaves[0];
			for (const TSharedPtr<FGoapNode>& Leaf : Cursor->Leaves)
			{
				if (Leaf.IsValid() && Leaf->Cost < Cheapest->Cost)
				{
					Cheapest = Leaf;
				}
			}

			Cursor = Cheapest;
			if (Cursor.IsValid() && Cursor->Action)
			{
				ForwardActions.Add(Cursor->Action);
			}
		}

		if (ForwardActions.Num() == 0)
		{
			// No executable actions found, try another goal
			continue;
		}

		OutPlan.Goal = Goal;
		OutPlan.Actions = MoveTemp(ForwardActions);

		UE_LOG(LogGoapPlanner, Verbose, TEXT("Plan: Found plan for goal [%s] with %d actions."),
			*GetNameSafe(Goal), OutPlan.Actions.Num());

		return true;
	}

	UE_LOG(LogGoapPlanner, Verbose, TEXT("Plan: No plan found."));
	return false;
}

bool UKMGoapPlannerSubsystem::FindPath(
	UKMGoapAgentComponent* Agent,
	const TSharedPtr<FGoapNode>& Parent,
	const TArray<UKMGoapAgentAction*>& AvailableActions)
{
	if (!Agent || !Parent.IsValid())
	{
		return false;
	}
	
	if (Parent->Required.Num() == 0)
	{
		return true;
	}
	
	TArray<UKMGoapAgentAction*> OrderedActions = AvailableActions;
	SortActionsByCost(OrderedActions);

	for (UKMGoapAgentAction* Action : OrderedActions)
	{
		if (!Action) continue;
		
		FGameplayTagContainer Required = Parent->Required;
		RemoveSatisfied(Agent, Required);

		if (Required.Num() == 0)
		{
			return true;
		}
		
		const bool bProvidesAny = Action->Effects.HasAnyExact(Required);
		if (!bProvidesAny)
		{
			continue;
		}
		
		FGameplayTagContainer NewRequired = Required;
		NewRequired.RemoveTags(Action->Effects);
		NewRequired.AppendTags(Action->Preconditions);
		
		TArray<UKMGoapAgentAction*> NewAvailable = OrderedActions;
		NewAvailable.Remove(Action);

		TSharedPtr<FGoapNode> Child = MakeShared<FGoapNode>();
		Child->Parent = Parent;
		Child->Action = Action;
		Child->Required = NewRequired;
		Child->Cost = Parent->Cost + Action->Cost;
		
		if (FindPath(Agent, Child, NewAvailable))
		{
			Parent->Leaves.Add(Child);
		}
	}
	
	return Parent->Leaves.Num() > 0;
}

bool UKMGoapPlannerSubsystem::IsTagSatisfied(UKMGoapAgentComponent* Agent, const FGameplayTag& Tag) const
{
	if (!Agent || !Tag.IsValid())
	{
		return false;
	}
	
	if (Agent->GetFact(Tag))
	{
		return true;
	}
	
	return Agent->EvaluateBeliefByTag(Tag);
}

void UKMGoapPlannerSubsystem::RemoveSatisfied(UKMGoapAgentComponent* Agent, FGameplayTagContainer& InOutRequired) const
{
	if (!Agent)
	{
		return;
	}

	TArray<FGameplayTag> Tags;
	InOutRequired.GetGameplayTagArray(Tags);

	for (const FGameplayTag& Tag : Tags)
	{
		if (IsTagSatisfied(Agent, Tag))
		{
			InOutRequired.RemoveTag(Tag);
		}
	}
}

void UKMGoapPlannerSubsystem::SortGoals(UKMGoapAgentComponent* Agent, TArray<UKMGoapAgentGoal*>& InOutGoals, UKMGoapAgentGoal* MostRecentGoal)
{
	InOutGoals.Sort([Agent, MostRecentGoal](const UKMGoapAgentGoal& A, const UKMGoapAgentGoal& B)
	{
		const float PA = A.GetPriority(Agent) - ((MostRecentGoal == &A) ? RecentGoalBias : 0.f);
		const float PB = B.GetPriority(Agent) - ((MostRecentGoal == &B) ? RecentGoalBias : 0.f);
		
		return PA > PB;
	});
}

void UKMGoapPlannerSubsystem::SortActionsByCost(TArray<UKMGoapAgentAction*>& InOutActions)
{
	InOutActions.Sort([](const UKMGoapAgentAction& A, const UKMGoapAgentAction& B)
	{
		return A.Cost < B.Cost;
	});
}
