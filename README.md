# KMGoap
GOAP Plugin to allow better AI Authoring and smarter agents with less effort


```mermaid
classDiagram
    direction LR

    %% ==========================================================
    %% Cross-namespace relationships (examples)
    %% ==========================================================
    UGoapAgentComponent --> UGoapPlannerService : uses
    UGoapAgentComponent --> UGoapWorldStateKeyRegistryAsset : uses
    UGoapAgentComponent --> UGoapActionAsset : AvailableActions
    UGoapAgentComponent --> UGoapGoalAsset : AvailableGoals
    UGoapAgentComponent --> FGoapPlan : CurrentPlan
    FGoapPlan --> FGoapPlanStep : Steps
    FGoapPlanStep --> UGoapActionAsset : Action

    UGoapActionAsset --> FGoapCondition : Preconditions
    UGoapActionAsset --> FGoapEffect : Effects
    UGoapGoalAsset --> FGoapCondition : Desired

    FGoapCondition --> FGoapWorldStateValue : Value
    FGoapEffect --> FGoapWorldStateValue : Value
    FGoapWorldStateValue --> EGoapValueType : Type
    FGoapCondition --> EGoapCompareOp : Op
    FGoapEffect --> EGoapEffectOp : Op

    FGoapBuildPlanTask --> UGoapAgentComponent : BuildBestPlan()
    FGoapExecutePlanTask --> UGoapAgentComponent : Execute/Tick
    FGoapUpdateWorldStateEvaluator --> UGoapAgentComponent : Sensors

    UGoapAssetValidationSubsystem --> UGoapActionAsset : validate
    UGoapAssetValidationSubsystem --> UGoapGoalAsset : validate
    UGoapAssetValidationSubsystem --> UGoapWorldStateKeyRegistryAsset : validate

    UEdGraph_Goap --> UEdGraphNode_GoapGoal : contains
    UEdGraph_Goap --> UEdGraphNode_GoapAction : contains
    UEdGraph_Goap --> UEdGraphNode_GoapCondition : contains
    UEdGraph_Goap --> UEdGraphNode_GoapEffect : contains
    UEdGraphNode_GoapGoal --> UGoapGoalAsset : wraps
    UEdGraphNode_GoapAction --> UGoapActionAsset : wraps

    UK2Node_GoapSetWorldValue --> UGoapAgentComponent : expandsTo
    UK2Node_GoapGetWorldValue --> UGoapAgentComponent : expandsTo
    UK2Node_GoapRequestPlan --> UGoapAgentComponent : expandsTo

    %% ==========================================================
    %% RUNTIME MODULE
    %% ==========================================================
    namespace Runtime {

        class UGoapWorldStateKeyRegistryAsset ["UGoapWorldStateKeyRegistryAsset -> UCLASS(BlueprintType)"]{
            +Keys: TArray_FGoapWorldKeyDef -> UPROPERTY(BlueprintReadOnly)
            +IsValidKey(Key: Name) const bool -> UFUNCTION(BlueprintCallable)
            +GetKeyType(Key: Name, OutFound: bool) const EGoapValueType -> UFUNCTION(BlueprintCallable)
        }

        class FGoapWorldKeyDef ["FGoapWorldKeyDef -> USTRUCT(BlueprintType)"]{
            +Key: Name -> UPROPERTY(BlueprintReadWrite)
            +Type: EGoapValueType -> UPROPERTY(BlueprintReadWrite)
            +Description: String -> UPROPERTY(BlueprintReadWrite)
            +DesignerFacing: bool -> UPROPERTY(BlueprintReadWrite)
        }

        class EGoapValueType ["EGoapValueType -> UENUM(BlueprintType)"]{
            Boolean
            Integer
            Float
            Vector
            Object
            Tag
        }

        class EGoapCompareOp ["EGoapCompareOp -> UENUM(BlueprintType)"]{
            Equal
            NotEqual
            Greater
            GreaterOrEqual
            Less
            LessOrEqual
            InRange
            IsTrue
            IsFalse
            IsValid
            HasTag
            AnyOfTags
        }

        class EGoapEffectOp ["EGoapEffectOp -> UENUM(BlueprintType)"]{
            Set
            Clear
            Add
            Subtract
            Toggle
        }

        class FGoapWorldStateValue ["FGoapWorldStateValue -> USTRUCT(BlueprintType)"]{
            +Type: EGoapValueType -> UPROPERTY(BlueprintReadWrite)
            +BoolValue: bool -> UPROPERTY(BlueprintReadWrite)
            +IntValue: int32 -> UPROPERTY(BlueprintReadWrite)
            +FloatValue: float -> UPROPERTY(BlueprintReadWrite)
            +VectorValue: Vector -> UPROPERTY(BlueprintReadWrite)
            +ObjectValue: Object -> UPROPERTY(BlueprintReadWrite)
            +TagValue: GameplayTag -> UPROPERTY(BlueprintReadWrite)
            +Equals(Other: FGoapWorldStateValue) const bool -> CppOnly
        }

        class FGoapCondition ["FGoapCondition -> USTRUCT(BlueprintType)"]{
            +Key: Name -> UPROPERTY(BlueprintReadWrite)
            +Op: EGoapCompareOp -> UPROPERTY(BlueprintReadWrite)
            +Value: FGoapWorldStateValue -> UPROPERTY(BlueprintReadWrite)
            +RangeMax: FGoapWorldStateValue -> UPROPERTY(BlueprintReadWrite)
            +Evaluate(World: TMap_Name_WorldValue) const bool -> CppOnly
        }

        class FGoapEffect ["FGoapEffect -> USTRUCT(BlueprintType)"]{
            +Key: Name -> UPROPERTY(BlueprintReadWrite)
            +Op: EGoapEffectOp -> UPROPERTY(BlueprintReadWrite)
            +Value: FGoapWorldStateValue -> UPROPERTY(BlueprintReadWrite)
            +ClampMin: FGoapWorldStateValue -> UPROPERTY(BlueprintReadWrite)
            +ClampMax: FGoapWorldStateValue -> UPROPERTY(BlueprintReadWrite)
            +Apply(World: TMap_Name_WorldValue) void -> CppOnly
        }

        class FGoapPlanStep ["FGoapPlanStep -> USTRUCT(BlueprintType)"]{
            +Action: UGoapActionAsset -> UPROPERTY(BlueprintReadWrite)
            +StepId: Name -> UPROPERTY(BlueprintReadWrite)
            +StepCost: float -> UPROPERTY(BlueprintReadWrite)
        }

        class FGoapPlan ["FGoapPlan -> USTRUCT(BlueprintType)"]{
            +Goal: UGoapGoalAsset -> UPROPERTY(BlueprintReadWrite)
            +Steps: TArray_FGoapPlanStep -> UPROPERTY(BlueprintReadWrite)
            +TotalCost: float -> UPROPERTY(BlueprintReadWrite)
            +IsValid: bool -> UPROPERTY(BlueprintReadWrite)
        }

        class IGoapAgentContext ["IGoapAgentContext -> UINTERFACE(BlueprintType)"]{
            +GetContextObject() const Object -> UFUNCTION(BlueprintCallable)
            +GetAgentActor() const Actor -> UFUNCTION(BlueprintCallable)
            +GetAIController() const AIController -> UFUNCTION(BlueprintCallable)
            +GetGoapAgent() const UGoapAgentComponent -> UFUNCTION(BlueprintCallable)
        }

        class EGoapActionTickResult ["EGoapActionTickResult -> UENUM(BlueprintType)"]{
            Running
            Succeeded
            Failed
            Aborted
        }

        class UGoapActionAsset ["UGoapActionAsset -> UCLASS(BlueprintType)"]{
            +ActionName: Name -> UPROPERTY(BlueprintReadWrite)
            +BaseCost: float -> UPROPERTY(BlueprintReadWrite)
            +Preconditions: TArray_FGoapCondition -> UPROPERTY(BlueprintReadWrite)
            +Effects: TArray_FGoapEffect -> UPROPERTY(BlueprintReadWrite)
            +CanInterrupt: bool -> UPROPERTY(BlueprintReadWrite)
            +DefaultTimeoutSec: float -> UPROPERTY(BlueprintReadWrite)

            +ComputeCost(Context: IGoapAgentContext) const float -> UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
            +IsActionUsable(Context: IGoapAgentContext) const bool -> UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
            +SelectTargets(Context: IGoapAgentContext) bool -> UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
            +OnActionStart(Context: IGoapAgentContext) void -> UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
            +OnActionTick(Context: IGoapAgentContext, DeltaTime: float) EGoapActionTickResult -> UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
            +OnActionEnd(Context: IGoapAgentContext, Succeeded: bool) void -> UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
        }

        class UGoapGoalAsset ["UGoapGoalAsset -> UCLASS(BlueprintType)"]{
            +GoalName: Name -> UPROPERTY(BlueprintReadWrite)
            +BasePriority: int32 -> UPROPERTY(BlueprintReadWrite)
            +Desired: TArray_FGoapCondition -> UPROPERTY(BlueprintReadWrite)
            +AllowMidPlanSwitch: bool -> UPROPERTY(BlueprintReadWrite)

            +ComputePriority(Context: IGoapAgentContext) const int32 -> UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
            +IsGoalActive(Context: IGoapAgentContext) const bool -> UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
        }

        class UGoapPlannerService ["UGoapPlannerService -> UCLASS(BlueprintType)"]{
            +MaxExpansions: int32 -> UPROPERTY(BlueprintReadWrite)
            +MaxPlanLength: int32 -> UPROPERTY(BlueprintReadWrite)
            +TimeBudgetMs: float -> UPROPERTY(BlueprintReadWrite)
            +BackwardSearch: bool -> UPROPERTY(BlueprintReadWrite)
            +UseHeuristic: bool -> UPROPERTY(BlueprintReadWrite)

            +BuildPlan(CurrentWorld: TMap_Name_WorldValue, Goal: UGoapGoalAsset, Actions: TArray_UGoapActionAsset, OutPlan: FGoapPlan) bool -> UFUNCTION(BlueprintCallable)
            +HeuristicEstimate(NodeWorld: TMap_Name_WorldValue, Desired: TArray_FGoapCondition) const float -> CppOnly
            +ValidatePlanStep(Context: IGoapAgentContext, Action: UGoapActionAsset, CurrentWorld: TMap_Name_WorldValue) const bool -> CppOnly
        }

        class UGoapBlackboardBridgeComponent ["UGoapBlackboardBridgeComponent -> UCLASS(BlueprintType)"]{
            +Blackboard: BlackboardComponent -> UPROPERTY(BlueprintReadWrite)
            +SyncWorldToBB: bool -> UPROPERTY(BlueprintReadWrite)
            +SyncBBToWorld: bool -> UPROPERTY(BlueprintReadWrite)
            +PushWorldToBlackboard(Agent: UGoapAgentComponent) void -> UFUNCTION(BlueprintCallable)
            +PullBlackboardToWorld(Agent: UGoapAgentComponent) void -> UFUNCTION(BlueprintCallable)
        }

        class FGoapActionRuntime ["FGoapActionRuntime -> USTRUCT(BlueprintType)"]{
            +Action: UGoapActionAsset -> UPROPERTY(BlueprintReadWrite)
            +StartTimeSec: float -> UPROPERTY(BlueprintReadWrite)
            +TimeoutSec: float -> UPROPERTY(BlueprintReadWrite)
            +RetryCount: int32 -> UPROPERTY(BlueprintReadWrite)
            +TargetObject: Object -> UPROPERTY(BlueprintReadWrite)
            +TargetLocation: Vector -> UPROPERTY(BlueprintReadWrite)
        }

        class EGoapDirtyReason ["EGoapDirtyReason -> UENUM(BlueprintType)"]{
            SensorsUpdated
            GoalPriorityChanged
            ActionFailed
            WorldStateChanged
            ExternalCommand
        }

        class UGoapAgentComponent ["UGoapAgentComponent -> UCLASS(BlueprintType, ActorComponent)"]{
            +KeyRegistry: UGoapWorldStateKeyRegistryAsset -> UPROPERTY(BlueprintReadWrite)
            +AvailableActions: TArray_UGoapActionAsset -> UPROPERTY(BlueprintReadWrite)
            +AvailableGoals: TArray_UGoapGoalAsset -> UPROPERTY(BlueprintReadWrite)
            +WorldState: TMap_Name_WorldValue -> UPROPERTY(BlueprintReadWrite)
            +CurrentPlan: FGoapPlan -> UPROPERTY(BlueprintReadWrite)
            +CurrentStepIndex: int32 -> UPROPERTY(BlueprintReadWrite)
            +CurrentRuntime: FGoapActionRuntime -> UPROPERTY(BlueprintReadWrite)
            +Planner: UGoapPlannerService -> UPROPERTY(BlueprintReadWrite)

            +SetWorldBool(Key: Name, Value: bool) void -> UFUNCTION(BlueprintCallable)
            +SetWorldInt(Key: Name, Value: int32) void -> UFUNCTION(BlueprintCallable)
            +SetWorldFloat(Key: Name, Value: float) void -> UFUNCTION(BlueprintCallable)
            +SetWorldVector(Key: Name, Value: Vector) void -> UFUNCTION(BlueprintCallable)

            +GetWorldBool(Key: Name, OutValue: bool) const bool -> UFUNCTION(BlueprintCallable)
            +GetWorldInt(Key: Name, OutValue: int32) const bool -> UFUNCTION(BlueprintCallable)
            +GetWorldFloat(Key: Name, OutValue: float) const bool -> UFUNCTION(BlueprintCallable)
            +GetWorldVector(Key: Name, OutValue: Vector) const bool -> UFUNCTION(BlueprintCallable)

            +MarkDirty(Reason: EGoapDirtyReason) void -> UFUNCTION(BlueprintCallable)
            +BuildBestPlan() bool -> UFUNCTION(BlueprintCallable)
            +HasValidPlan() const bool -> UFUNCTION(BlueprintCallable)
            +GetActiveGoal() const UGoapGoalAsset -> UFUNCTION(BlueprintCallable)
            +GetCurrentAction() const UGoapActionAsset -> UFUNCTION(BlueprintCallable)
            +StartCurrentAction() bool -> UFUNCTION(BlueprintCallable)
            +TickCurrentAction(DeltaTime: float) EGoapActionTickResult -> UFUNCTION(BlueprintCallable)
            +FinishCurrentAction(Succeeded: bool) void -> UFUNCTION(BlueprintCallable)
            +AdvanceToNextStep() bool -> UFUNCTION(BlueprintCallable)
            +ClearPlan() void -> UFUNCTION(BlueprintCallable)

            +OnUpdateWorldStateFromSensors() void -> UFUNCTION(BlueprintImplementableEvent)
            +OnPlanBuilt(Plan: FGoapPlan) void -> UFUNCTION(BlueprintImplementableEvent)
            +OnPlanFailed() void -> UFUNCTION(BlueprintImplementableEvent)
            +OnActionChanged(NewAction: UGoapActionAsset) void -> UFUNCTION(BlueprintImplementableEvent)
        }

        class FGoapStateTreeContext ["FGoapStateTreeContext -> USTRUCT(BlueprintType)"]{
            +HasPlan: bool -> UPROPERTY(BlueprintReadWrite)
            +PlanFailed: bool -> UPROPERTY(BlueprintReadWrite)
            +SelectedGoal: UGoapGoalAsset -> UPROPERTY(BlueprintReadWrite)
            +CurrentAction: UGoapActionAsset -> UPROPERTY(BlueprintReadWrite)
            +StepIndex: int32 -> UPROPERTY(BlueprintReadWrite)
            +ActionRunning: bool -> UPROPERTY(BlueprintReadWrite)
        }

        class EGoapReplanPolicy ["EGoapReplanPolicy -> UENUM(BlueprintType)"]{
            OnFailure
            OnDirty
            OnInterval
            OnHigherPriorityGoal
        }

        class EGoapPlanValidationMode ["EGoapPlanValidationMode -> UENUM(BlueprintType)"]{
            CurrentStepOnly
            WholePlan
            WholePlanLight
        }

        class FGoapUpdateWorldStateEvaluator ["FGoapUpdateWorldStateEvaluator -> StateTreeEvaluator"]{
            +UpdateIntervalSec: float
            +UpdateEveryTick: bool
            +Tick(Context: IGoapAgentContext, DeltaTime: float) void
        }

        class FGoapBuildPlanTask ["FGoapBuildPlanTask -> StateTreeTask"]{
            +ForceReplan: bool
            +ReplanPolicy: EGoapReplanPolicy
            +EnterState(Context: IGoapAgentContext) StateTreeRunStatus
        }

        class FGoapExecutePlanTask ["FGoapExecutePlanTask -> StateTreeTask"]{
            +ValidationMode: EGoapPlanValidationMode
            +AllowInterrupt: bool
            +EnterState(Context: IGoapAgentContext) StateTreeRunStatus
            +Tick(Context: IGoapAgentContext, DeltaTime: float) StateTreeRunStatus
            +ExitState(Context: IGoapAgentContext) void
        }
    }

    %% ==========================================================
    %% EDITOR MODULE
    %% ==========================================================
    namespace Editor {

        class UGoapAssetValidationSubsystem ["UGoapAssetValidationSubsystem -> UCLASS()"]{
            +LastErrors: TArray_String -> UPROPERTY()
            +ValidateAction(Action: UGoapActionAsset) bool
            +ValidateGoal(Goal: UGoapGoalAsset) bool
            +ValidateKeyRegistry(Registry: UGoapWorldStateKeyRegistryAsset) bool
            +ValidateGraphConnectivity(Actions: TArray_UGoapActionAsset, Goals: TArray_UGoapGoalAsset) bool
        }

        class UGoapActionAssetFactory ["UGoapActionAssetFactory -> UCLASS()"]{
            +FactoryCreateNew() UGoapActionAsset
        }

        class UGoapGoalAssetFactory ["UGoapGoalAssetFactory -> UCLASS()"]{
            +FactoryCreateNew() UGoapGoalAsset
        }

        class FGoapActionAssetDetails ["FGoapActionAssetDetails -> DetailsCustomization"]{
            +CustomizeDetails() void
        }

        class FGoapGoalAssetDetails ["FGoapGoalAssetDetails -> DetailsCustomization"]{
            +CustomizeDetails() void
        }

        class FGoapKeyRegistryDetails ["FGoapKeyRegistryDetails -> DetailsCustomization"]{
            +CustomizeDetails() void
        }

        class UGoapDebugWidgetSettings ["UGoapDebugWidgetSettings -> UCLASS(Config)"]{
            +ShowWorldState: bool -> UPROPERTY(Config)
            +ShowPlan: bool -> UPROPERTY(Config)
            +ShowCosts: bool -> UPROPERTY(Config)
        }
    }

    %% ==========================================================
    %% GRAPH MODULE
    %% ==========================================================
    namespace Graph {

        class UEdGraph_Goap ["UEdGraph_Goap -> UCLASS()"]{
            +KeyRegistry: UGoapWorldStateKeyRegistryAsset -> UPROPERTY()
            +Goal: UGoapGoalAsset -> UPROPERTY()
            +ActionLibrary: TArray_UGoapActionAsset -> UPROPERTY()
        }

        class UEdGraphSchema_Goap ["UEdGraphSchema_Goap -> UCLASS()"]{
            +GetGraphContextActions() void
            +CanCreateConnection() bool
            +TryCreateConnection() bool
        }

        class UEdGraphNode_GoapGoal ["UEdGraphNode_GoapGoal -> UCLASS()"]{
            +Goal: UGoapGoalAsset -> UPROPERTY()
            +AllocateDefaultPins() void
            +GetNodeTitle() String
        }

        class UEdGraphNode_GoapAction ["UEdGraphNode_GoapAction -> UCLASS()"]{
            +Action: UGoapActionAsset -> UPROPERTY()
            +AllocateDefaultPins() void
            +GetNodeTitle() String
        }

        class UEdGraphNode_GoapCondition ["UEdGraphNode_GoapCondition -> UCLASS()"]{
            +Condition: FGoapCondition -> UPROPERTY()
            +AllocateDefaultPins() void
            +GetNodeTitle() String
        }

        class UEdGraphNode_GoapEffect ["UEdGraphNode_GoapEffect -> UCLASS()"]{
            +Effect: FGoapEffect -> UPROPERTY()
            +AllocateDefaultPins() void
            +GetNodeTitle() String
        }

        class UK2Node_GoapSetWorldValue ["UK2Node_GoapSetWorldValue -> UCLASS()"]{
            +ExpandNode() void
            +GetMenuActions() void
            +AllocateDefaultPins() void
        }

        class UK2Node_GoapGetWorldValue ["UK2Node_GoapGetWorldValue -> UCLASS()"]{
            +ExpandNode() void
            +GetMenuActions() void
            +AllocateDefaultPins() void
        }

        class UK2Node_GoapRequestPlan ["UK2Node_GoapRequestPlan -> UCLASS()"]{
            +ExpandNode() void
            +GetMenuActions() void
            +AllocateDefaultPins() void
        }
    }
```