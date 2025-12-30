# KMGoap Plugin for Unreal Engine

## 1. Introduction

Welcome to **KMGoap**, a powerful and flexible Goal-Oriented Action Planning (GOAP) plugin for Unreal Engine. GOAP is an AI architecture that allows for emergent and intelligent behavior by letting agents decide their own course of action based on high-level goals, rather than following rigid, pre-scripted logic like Behavior Trees or State Machines.

This plugin provides a robust, data-driven framework for implementing GOAP in your projects, empowering you to create complex and dynamic AI agents with ease.

## 2. Features

- **Data-Driven by Design**: Define Actions, Goals, and Beliefs as separate Blueprint or C++ assets. Easily change an agent's behavior by swapping data assets, enabling rapid iteration for designers.
- **Highly Extensible**: Implement custom logic with pluggable search algorithms, agent state machines, and sensor components.
- **Blueprint & C++ Friendly**: Provides a solid C++ core with full Blueprint exposure, allowing you to work in the environment you're most comfortable with.
- **Dynamic & Context-Aware**: Action costs and Goal priorities can be calculated dynamically at runtime, allowing for nuanced and intelligent decision-making.
- **Integrated with Unreal Engine**: Leverages core engine features like `GameplayTags` for a clean and powerful workflow.

## 3. Installation

First, you need to get the plugin into your project's `Plugins` folder. If this folder doesn't exist, create it at the root of your project.

### Method 1: Git Submodule (Recommended)

If your project uses Git, the cleanest way to include the plugin is as a submodule. Navigate to your project's root directory in a terminal and run the following command:

```bash
git submodule add <repository_url> Plugins/KMGoap
```

This will clone the plugin into the correct folder and keep a reference to the specific commit you're using, making updates easy to manage.

### Method 2: Manual Installation

1.  Download the plugin source code as a ZIP file.
2.  Extract the contents.
3.  Rename the extracted folder to `KMGoap`.
4.  Move the `KMGoap` folder into your project's `Plugins` directory.

### Enabling the Plugin

After adding the plugin files, you need to enable it in your project:

1.  Open your Unreal Engine project.
2.  Go to **Edit > Plugins**.
3.  Search for "KMGoap" and check the **Enabled** box.
4.  Restart the editor when prompted.

The plugin is now ready to use!

## 4. Getting Started: Creating a Simple AI

Let's create a simple AI that needs to find a key to unlock a door. This tutorial will guide you through creating each necessary piece.

### 4.1. The Scenario

- **Goal**: The agent wants to open a door.
- **World State**: The door is initially locked. A key exists somewhere in the level.
- **Actions**: The agent can `PickUpKey` and `OpenDoor`.
- **Logic**: The `OpenDoor` action requires the agent to have the key (`HasKey` must be true).

### 4.2. Setting up the Agent

1.  Create a new `Blueprint Class` based on `Character` or `Pawn`. Let's call it `BP_GoapAgent`.
2.  Open the Blueprint and add the **`KMGoapAgentComponent`** to it.

### 4.3. Creating Beliefs

Beliefs represent the agent's knowledge. We need two for our scenario.

1.  Create a new Blueprint Class. Under "All Classes", search for and select `KMGoapAgentBelief`. Call it `BP_Belief_HasKey`.
2.  Open it and in the **Class Defaults**, set the `Belief Tag` to a new tag, e.g., `Belief.HasKey`.
3.  Repeat the process to create `BP_Belief_DoorIsOpen` with the tag `Belief.DoorIsOpen`.

For this simple example, we will control these beliefs manually through "Facts", so no further logic is needed in the Belief Blueprints.

### 4.4. Creating Actions

Actions are the core of GOAP. They have preconditions (what must be true to run) and effects (what becomes true after running).

**Action 1: PickUpKey**

1.  Create a new Blueprint Class based on `KMGoapAgentAction`. Call it `BP_Action_PickUpKey`.
2.  Open it and set the `Action Tag` to `Action.PickUpKey`.
3.  **Effects**: This action makes the agent have the key. Add a new element to the `Effects` set:
    -   `Tag`: `Belief.HasKey`
    -   `bValue`: `true`
4.  **Logic**: For this tutorial, we'll just simulate picking up the key. Override the `OnTick` function and simply return `Succeeded`.

**Action 2: OpenDoor**

1.  Create another `KMGoapAgentAction` Blueprint. Call it `BP_Action_OpenDoor`.
2.  Set the `Action Tag` to `Action.OpenDoor`.
3.  **Preconditions**: This action requires the agent to have the key. Add a new element to the `Preconditions` set:
    -   `Tag`: `Belief.HasKey`
    -   `bValue`: `true`
4.  **Effects**: This action makes the door open. Add a new element to the `Effects` set:
    -   `Tag`: `Belief.DoorIsOpen`
    -   `bValue`: `true`
5.  **Logic**: Override `OnTick` and return `Succeeded`.

### 4.5. Creating a Goal

Goals define what the agent wants to achieve.

1.  Create a new Blueprint Class based on `KMGoapAgentGoal`. Call it `BP_Goal_OpenTheDoor`.
2.  Set the `Goal Tag` to `Goal.OpenDoor`.
3.  **Desired Effects**: The goal is to have the door open. Add a new element to the `Desired Effects` set:
    -   `Tag`: `Belief.DoorIsOpen`
    -   `bValue`: `true`
4.  Set the `Base Priority` to `1.0`.

### 4.6. Creating Data Assets

Now we need to group our new Actions and Goals into sets that the agent can use.

1.  Create a `KMGoapActionSet` data asset (under Miscellaneous > Data Asset). Call it `DA_ActionSet_Basic`.
    -   Add `BP_Action_PickUpKey` and `BP_Action_OpenDoor` to its `Actions` array.
2.  Create a `KMGoapGoalSet` data asset. Call it `DA_GoalSet_Basic`.
    -   Add `BP_Goal_OpenTheDoor` to its `Goals` array.
3.  Create a `KMGoapBeliefSet` data asset. Call it `DA_BeliefSet_Basic`.
    -   Add `BP_Belief_HasKey` and `BP_Belief_DoorIsOpen` to its `Beliefs` array.

### 4.7. Configuring the Agent Component

1.  Go back to your `BP_GoapAgent` Blueprint.
2.  Select the `KMGoapAgentComponent`.
3.  In the Details panel, assign your new data assets:
    -   **Belief Set**: `DA_BeliefSet_Basic`
    -   **Action Set**: `DA_ActionSet_Basic`
    -   **Goal Set**: `DA_GoalSet_Basic`

### 4.8. Setting up the Planner

The planner needs a configuration file to tell it which search algorithm to use.

1.  Create a `KMGoapPlannerConfig` data asset. Call it `DA_PlannerConfig`.
2.  Open it and set the `Search Algorithm Class` to `KMGoapPlanSearchAStar` (the default A* implementation).
3.  Go to **Edit > Project Settings > Plugins > KMGoap**.
4.  Set the `Planner Config` property to your `DA_PlannerConfig` asset.

### 4.9. Running the Simulation

Place your `BP_GoapAgent` in a level. When you press Play, the agent will:

1.  Evaluate its goals and see that `Goal.OpenDoor` has the highest priority.
2.  Ask the planner for a plan to achieve `Belief.DoorIsOpen = true`.
3.  The planner will look at the available actions:
    -   It sees that `BP_Action_OpenDoor` achieves the goal, but its precondition (`Belief.HasKey = true`) is not met.
    -   It then looks for an action that can satisfy this precondition and finds `BP_Action_PickUpKey`.
4.  The planner returns the plan: **[`PickUpKey`, `OpenDoor`]**.
5.  The agent's state machine will execute `PickUpKey`, and once it succeeds, it will execute `OpenDoor`.

Congratulations, you've just created your first GOAP agent!

## 5. Core Concepts

- **`UKMGoapAgentComponent`**: The agent's brain. Manages all GOAP data and processes.
- **`UKMGoapAgentAction`**: A single, atomic action the agent can perform. Defined by its preconditions and effects.
- **`UKMGoapAgentGoal`**: A high-level goal the agent wants to achieve. Defined by a desired world state and a priority.
- **`UKMGoapAgentBelief`**: A queryable fact about the world from the agent's perspective.
- **`IKMGoapSensorInterface`**: An interface for creating sensors that can automatically update the agent's beliefs about the world.

## 6. Advanced Usage

- **Custom State Machines**: You can create your own state machine by implementing the `IKMGoapAgentStateMachineInterface` to handle plan execution in a unique way (e.g., to support parallel actions).
- **Custom Planners**: Implement the `IKMGoapPlanSearchInterface` to create your own search algorithm if A* doesn't fit your needs.
- **Dynamic Costs and Priorities**: Override the `Priority` function in Goals or the `GetDynamicCost` function in Actions to make your AI's decision-making more context-aware.

## 7. Contributing

Contributions are welcome! Please feel free to fork the repository, make your changes, and submit a pull request.

## 8. License

This plugin is licensed under the MIT License. See the LICENSE file for more details.
