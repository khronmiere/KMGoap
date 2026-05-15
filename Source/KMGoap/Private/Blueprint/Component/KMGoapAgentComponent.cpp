// All rights reserved by Khrönmière Entertainment.
#include "Blueprint/Component/KMGoapAgentComponent.h"

#include "GameplayTagContainer.h"
#include "Blueprint/KMGoapAgentAction.h"
#include "Blueprint/KMGoapAgentBelief.h"
#include "Blueprint/KMGoapAgentGoal.h"
#include "Blueprint/Behavior/KMGoapDefaultStateMachine.h"
#include "Blueprint/Behavior/KMGoapKnowledgeRuntime.h"
#include "Blueprint/Component/KMGoapKnowledgeProviderComponent.h"
#include "Blueprint/Data/KMGoapActionSet.h"
#include "Blueprint/Data/KMGoapBeliefSet.h"
#include "Blueprint/Data/KMGoapGoalSet.h"
#include "Data/KMGoapActionPlan.h"
#include "Data/KMGoapCondition.h"
#include "Interface/KMGoapAgentStateMachineInterface.h"
#include "Interface/KMGoapPlanSearchInterface.h"
#include "Interface/KMGoapSensorInterface.h"
#include "Kismet/GameplayStatics.h"
#include "Subsystem/KMGoapPlannerSubsystem.h"
#include "Subsystem/Behavior/KMGoapPlanSearchBase.h"

DEFINE_LOG_CATEGORY_STATIC(LogGoapAgent, Log, All);

// Sets default values for this component's properties
UKMGoapAgentComponent::UKMGoapAgentComponent()
{
	// The agent ticks because planning/execution belongs to the state machine runner.
	// Belief evaluation itself is timer-driven separately.
	PrimaryComponentTick.bCanEverTick = true;

	StateMachineRunnerClass = UKMGoapDefaultStateMachine::StaticClass();
}

UKMGoapAgentBelief* UKMGoapAgentComponent::GetBeliefByTag(FGameplayTag Tag) const
{
	if (auto Belief = BeliefsByTag.Find(Tag))
	{
		return *Belief;
	}
	return nullptr;
}

UActorComponent* UKMGoapAgentComponent::GetSensorByTag(FGameplayTag Tag) const
{
	if (auto Sensor = SensorsByTag.Find(Tag))
	{
		return *Sensor;
	}
	return nullptr;
}

UKMGoapAgentGoal* UKMGoapAgentComponent::GetGoalByTag(FGameplayTag Tag) const
{
	if (auto Goal = GoalsByTag.Find(Tag))
	{
		return *Goal;
	}
	return nullptr;
}

UKMGoapAgentAction* UKMGoapAgentComponent::GetActionByTag(FGameplayTag Tag) const
{
	if (auto Action = ActionsByTag.Find(Tag))
	{
		return *Action;
	}
	return nullptr;
}

EKMGoapBeliefState UKMGoapAgentComponent::EvaluateBeliefByTag(FGameplayTag Tag) const
{
	if (auto CacheEntry = BeliefCache.Find(Tag))
	{
		return CacheEntry->bValue ? EKMGoapBeliefState::Positive : EKMGoapBeliefState::Negative;
	}

	// Unknown here usually means the planner or a knowledge module references a belief
	// that was never registered on this agent.
	UE_LOG(LogGoapAgent, Error,
		TEXT("Belief not present in Cache. Are we evaluating beliefs not added in the Set?, Requested Belief with tag %s"),
		*Tag.ToString());
	return EKMGoapBeliefState::Unknown;
}

FVector UKMGoapAgentComponent::GetBeliefLocationByTag(FGameplayTag Tag) const
{
	if (UKMGoapAgentBelief* Belief = GetBeliefByTag(Tag))
	{
		return Belief->GetLocation(this);
	}
	return FVector::ZeroVector;
}

void UKMGoapAgentComponent::SetFact(FGameplayTag FactTag, bool bNewValue)
{
	if (!FactTag.IsValid()) return;

	// Facts are explicit runtime truths owned by the agent. They are treated as more
	// authoritative than derived beliefs when validating action preconditions.
	bool& bCurrentFactValue = Facts.FindOrAdd(FactTag);
	bCurrentFactValue = bNewValue;
}

EKMGoapBeliefState UKMGoapAgentComponent::GetFact(FGameplayTag Tag) const
{
	if (const bool* bValue = Facts.Find(Tag))
	{
		return *bValue ? EKMGoapBeliefState::Positive : EKMGoapBeliefState::Negative;
	}
	return EKMGoapBeliefState::Unknown;
}

TArray<FGameplayTag> UKMGoapAgentComponent::GetFactsTags() const
{
	TArray<FGameplayTag> Result;
	Facts.GetKeys(Result);
	return Result;
}

bool UKMGoapAgentComponent::AddNewKnowledgeModule(UKMGoapKnowledgeModule* NewModule)
{
	if (!NewModule || !KnowledgeRuntime)
	{
		return false;
	}

	// KnowledgeRuntime owns module identity, instancing, and execution reset behavior.
	return KnowledgeRuntime->AddKnowledge(this, NewModule);
}

void UKMGoapAgentComponent::InitializeStateMachineRunner()
{
	UE_LOG(LogGoapAgent, Log, TEXT("Initializing StateMachine Runner"));

	// The runner is created from a UObject class that implements the state-machine
	// interface, allowing projects to replace the default execution policy without
	// changing the agent component.
	if (StateMachineRunnerClass &&
		StateMachineRunnerClass->ImplementsInterface(UKMGoapAgentStateMachineInterface::StaticClass()))
	{
		StateMachineRunner = NewObject<UObject>(this, StateMachineRunnerClass);
		IKMGoapAgentStateMachineInterface::Execute_Start(StateMachineRunner, this);
		return;
	}

	UE_LOG(LogGoapAgent, Error, TEXT("Failed to initialize state machine runner due to invalid class property"))
}

void UKMGoapAgentComponent::BeginPlay()
{
	Super::BeginPlay();

	// Static asset sets are expanded into per-agent runtime objects before the state
	// machine starts planning.
	CacheSensors();
	BuildBeliefs();
	BuildActions();
	BuildGoals();

	InitializeStateMachineRunner();
	InitializeKnowledgeRuntime();
}

void UKMGoapAgentComponent::StopStateMachineRunner()
{
	if (StateMachineRunner)
	{
		IKMGoapAgentStateMachineInterface::Execute_Stop(StateMachineRunner);
		StateMachineRunner = nullptr;
	}
}

void UKMGoapAgentComponent::InitializeKnowledgeRuntime()
{
	KnowledgeRuntime = NewObject<UKMGoapKnowledgeRuntime>(this);
}

void UKMGoapAgentComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ClearSensors();
	ClearBeliefs();
	StopStateMachineRunner();
	Super::EndPlay(EndPlayReason);
}

void UKMGoapAgentComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (StateMachineRunner)
	{
		IKMGoapAgentStateMachineInterface::Execute_Tick(StateMachineRunner, DeltaTime);
	}

	// Knowledge is ticked after execution so modules learned or removed this frame can
	// invalidate future planning without interrupting the current state-machine call.
	if (KnowledgeRuntime)
	{
		KnowledgeRuntime->Tick(this);
	}
}

void UKMGoapAgentComponent::TryLearnKnowledge(FGameplayTag SourceTag)
{
	UActorComponent* Sensor = GetSensorByTag(SourceTag);
	if (!Sensor)
	{
		UE_LOG(LogGoapAgent, Error,
			TEXT("TryLearnKnowledge failed: no sensor found for tag [%s]."),
			*SourceTag.ToString());
		return;
	}

	if (!Sensor->GetClass()->ImplementsInterface(UKMGoapSensorInterface::StaticClass()))
	{
		UE_LOG(LogGoapAgent, Error,
			TEXT("TryLearnKnowledge failed: component [%s] for tag [%s] does not implement KMGoapSensorInterface."),
			*GetNameSafe(Sensor),
			*SourceTag.ToString());
		return;
	}

	const AActor* Target = IKMGoapSensorInterface::Execute_GetTarget(Sensor);
	if (!Target)
	{
		return;
	}

	// Knowledge discovery is sensor-driven: perceived actors can provide new beliefs,
	// actions, or goals to the agent at runtime.
	if (auto KnowledgeProvider = Target->GetComponentByClass<UKMGoapKnowledgeProviderComponent>())
	{
		KnowledgeProvider->Teach(this);
	}
}

void UKMGoapAgentComponent::HandleSensorTargetChanged(FGameplayTag SourceTag)
{
	UE_LOG(LogGoapAgent, Display, TEXT("Target changed, clearing current action and goal."));

	TryLearnKnowledge(SourceTag);

	if (StateMachineRunner)
	{
		IKMGoapAgentStateMachineInterface::Execute_OnSensorStateUpdate(StateMachineRunner);
	}

	OnSensorTargetChanged(SourceTag);
}

void UKMGoapAgentComponent::EvaluateBeliefs()
{
	BeliefCache.Reset();
	BeliefCache.Reserve(BeliefsByTag.Num());

	// Beliefs may query sensors or world state, so cache the result once per evaluation
	// pass instead of recomputing during every planner comparison.
	for (const TTuple<FGameplayTag, TObjectPtr<UKMGoapAgentBelief>>& Tuple : BeliefsByTag)
	{
		UKMGoapAgentBelief* Belief = Tuple.Value;
		FGameplayTag Tag = Tuple.Key;
		if (!Belief)
		{
			continue;
		}

		bool bResult = Belief->Evaluate(this);
		FKMGoapBeliefCacheEntry& Entry = BeliefCache.FindOrAdd(Tag);
		Entry.BeliefTag = Tag;
		Entry.bValue = bResult;
	}
}

void UKMGoapAgentComponent::FilterFactSatisfiedPreconditions(TSet<FKMGoapCondition>& Preconditions) const
{
	TArray<FKMGoapCondition> ToRemove;

	// Facts are checked first because they represent agent-owned runtime state and
	// should satisfy matching preconditions without consulting derived beliefs.
	for (const FKMGoapCondition& Precondition : Preconditions)
	{
		const bool* Fact = Facts.Find(Precondition.Tag);
		if (!Fact)
		{
			continue;
		}

		if (Precondition.bValue == *Fact)
		{
			ToRemove.Add(Precondition);
		}
	}

	for (const FKMGoapCondition& Condition : ToRemove)
	{
		Preconditions.Remove(Condition);
	}
}

void UKMGoapAgentComponent::FilterBeliefSatisfiedPreconditions(TSet<FKMGoapCondition>& Preconditions) const
{
	TArray<FKMGoapCondition> ToRemove;

	// Preconditions already represented by facts are skipped here so facts remain the
	// authoritative source for those tags.
	for (const FKMGoapCondition& Precondition : Preconditions)
	{
		if (Facts.Contains(Precondition.Tag))
		{
			continue;
		}

		EKMGoapBeliefState ExpectedState = Precondition.bValue ? EKMGoapBeliefState::Positive : EKMGoapBeliefState::Negative;
		if (EvaluateBeliefByTag(Precondition.Tag) == ExpectedState)
		{
			ToRemove.Add(Precondition);
		}
	}

	for (const FKMGoapCondition& Condition : ToRemove)
	{
		Preconditions.Remove(Condition);
	}
}

bool UKMGoapAgentComponent::ValidateActionPreconditions(const UKMGoapAgentAction* Action) const
{
	if (!Action)
	{
		return false;
	}

	// Validation removes satisfied conditions from a working copy. Any remaining
	// condition represents a requirement that the current world/fact state does not meet.
	TSet<FKMGoapCondition> Preconditions = Action->Preconditions;
	FilterFactSatisfiedPreconditions(Preconditions);
	FilterBeliefSatisfiedPreconditions(Preconditions);
	return Preconditions.IsEmpty();
}

void UKMGoapAgentComponent::UpdateBeliefEvaluationCache()
{
	EvaluateBeliefs();
}

bool UKMGoapAgentComponent::ComputePlanForGoals(
	const TArray<UKMGoapAgentGoal*>& GoalsToCheck,
	UKMGoapAgentGoal* LastGoal,
	FKMGoapActionPlan& OutPlan)
{
	UGameInstance* GameInstance = UGameplayStatics::GetGameInstance(GetWorld());
	if (!GameInstance)
	{
		UE_LOG(LogGoapAgent, Error, TEXT("ComputePlanForGoals: GameInstance is NULL"));
		return false;
	}

	UKMGoapPlannerSubsystem* Planner = GameInstance->GetSubsystem<UKMGoapPlannerSubsystem>();
	if (!Planner)
	{
		UE_LOG(LogGoapAgent, Error, TEXT("ComputePlanForGoals: Planner is NULL"));
		return false;
	}

	UKMGoapPlanSearchBase* Algorithm = Planner->GetSearchAlgorithm();
	if (!Algorithm)
	{
		UE_LOG(LogGoapAgent, Error, TEXT("ComputePlanForGoals: Algorithm is NULL"));
		return false;
	}

	// The planner subsystem provides the active search strategy. Passing LastGoal lets
	// the algorithm preserve goal continuity when doing so is preferable.
	return IKMGoapPlanSearchInterface::Execute_BuildPlan(Algorithm, this, GoalsToCheck, LastGoal, OutPlan);
}

void UKMGoapAgentComponent::ResetExecutionState() const
{
	if (StateMachineRunner)
	{
		IKMGoapAgentStateMachineInterface::Execute_Reset(StateMachineRunner);
	}
}

void UKMGoapAgentComponent::BuildBeliefs()
{
	UE_LOG(LogGoapAgent, Display, TEXT("Building Beliefs Cache"));
	BeliefsByTag.Reset();
	if (!BeliefSet) return;

	BuildTaggedObjects<UKMGoapAgentBelief>(
		this,
		BeliefSet->Beliefs,
		BeliefsByTag,
		[](const UKMGoapAgentBelief* Belief) { return Belief->BeliefTag; }
	);

	if (UWorld* World = GetWorld())
	{
		auto& TimerManager = World->GetTimerManager();

		// Beliefs are evaluated on a fixed cadence to decouple sensing/world queries
		// from the frame-by-frame execution tick.
		TimerManager.SetTimer(
			BeliefEvaluateTimerHandle, 
			this,
			&UKMGoapAgentComponent::EvaluateBeliefs,
			EvaluateBeliefTimeStep,
			true,
			0.f);
	}
}

void UKMGoapAgentComponent::ClearBeliefs()
{
	if (UWorld* World = GetWorld())
	{
		auto& TimerManager = World->GetTimerManager();
		TimerManager.ClearTimer(BeliefEvaluateTimerHandle);
	}
}

void UKMGoapAgentComponent::BuildActions()
{
	UE_LOG(LogGoapAgent, Display, TEXT("Building Actions Cache"));
	ActionsByTag.Reset();
	if (!ActionSet) return;
	BuildTaggedObjects<UKMGoapAgentAction>(
		this,
		ActionSet->Actions,
		ActionsByTag,
		[](const UKMGoapAgentAction* Action) { return Action->ActionTag; }
	);
}

void UKMGoapAgentComponent::BuildGoals()
{
	UE_LOG(LogGoapAgent, Display, TEXT("Building Goals Cache"));
	GoalsByTag.Reset();
	if (!GoalSet) return;
	BuildTaggedObjects<UKMGoapAgentGoal>(
		this,
		GoalSet->Goals,
		GoalsByTag,
		[](const UKMGoapAgentGoal* Action) { return Action->GoalTag; }
	);
}

void UKMGoapAgentComponent::CacheSensors()
{
	UE_LOG(LogGoapAgent, Display, TEXT("Building Sensors Cache"));

	AActor* Owner = GetOwner();
	if (!Owner) return;

	SensorsByTag.Reset();

	// Sensors are discovered by interface rather than concrete type so projects can
	// provide custom perception components while still participating in GOAP.
	TArray<UActorComponent*> SensorComponents =	GetOwner()->GetComponentsByInterface(
		UKMGoapSensorInterface::StaticClass()
	);

	for (UActorComponent* Sensor : SensorComponents)
	{
		FGameplayTag Tag = IKMGoapSensorInterface::Execute_GetTag(Sensor);
		if (!Tag.IsValid())
		{
			UE_LOG(LogGoapAgent, Warning, TEXT("Found sensor component with invalid tag: %s"),
				*GetNameSafe(Sensor));
			continue;
		}

		if (SensorsByTag.Contains(Tag))
		{
			UE_LOG(LogGoapAgent, Warning, TEXT("Duplicate sensor tag [%s]. Ignoring %s"),
				*Tag.ToString(), *GetNameSafe(Sensor));
			continue;
		}

		BindSensorEvents(Sensor);
		SensorsByTag.Add(Tag, Sensor);
	}
}

void UKMGoapAgentComponent::ClearSensors()
{
	for (auto& Pair : SensorsByTag)
	{
		TObjectPtr<UActorComponent> Sensor = Pair.Value;
		UnbindSensorEvents(Sensor);
	}

	SensorsByTag.Reset();
}

void UKMGoapAgentComponent::BindSensorEvents(UActorComponent* Sensor)
{
	// Sensor events are routed through the sensor interface so this component does not
	// need to know whether the event originated from sight, hearing, scripted target
	// selection, or another perception implementation.
	IKMGoapSensorInterface::Execute_RegisterTargetChangedListener(
			Sensor,
			this,
			GET_FUNCTION_NAME_CHECKED(UKMGoapAgentComponent, HandleSensorTargetChanged));
}

void UKMGoapAgentComponent::UnbindSensorEvents(UActorComponent* Sensor)
{
	IKMGoapSensorInterface::Execute_UnregisterTargetChangedListener(
			Sensor,
			this,
			GET_FUNCTION_NAME_CHECKED(UKMGoapAgentComponent, HandleSensorTargetChanged));
}

