# KMGoap Plugin for Unreal Engine

Welcome to **KMGoap**, a powerful, data-driven Goal-Oriented Action Planning (GOAP) plugin for Unreal Engine. 

GOAP is an AI architecture that enables emergent and intelligent behavior by allowing agents to decide their own course of action based on high-level goals and dynamic world states. Rather than following rigid, pre-scripted logic like traditional Behavior Trees or State Machines, KMGoap agents dynamically chain actions together at runtime to solve complex problems.

This plugin provides a robust C++ core with full Blueprint exposure, empowering designers and engineers to create complex, context-aware AI agents with ease.

---

## Key Features

KMGoap is built with modern Unreal Engine architecture in mind, focusing on modularity, performance, and ease of authoring.

- **Data-Driven by Design**: Define Actions, Goals, and Beliefs as separate Blueprint or C++ assets. Agent capabilities are composed using `UDataAsset` wrappers, allowing you to change an agent's behavior simply by swapping data assets.
- **GameplayTag Integration**: The entire system uses `FGameplayTag` as the universal identifier for beliefs, actions, goals, and sensors. This ensures type safety, hierarchical namespacing, and excellent editor tooling integration without the overhead of string lookups.
- **Dynamic Knowledge Injection**: Agents can learn and forget behaviors at runtime. The `KnowledgeRuntime` system allows you to inject or remove bundles of actions, beliefs, and goals dynamically, complete with deactivation rules based on belief states.
- **Rigorous Planning Algorithm**: The core planner utilizes a highly optimized Dijkstra search (Uniform Cost Search) over simulated world states. It features a min-heap open set, canonical state hashing for pruning, and strict budget limits (nodes, depth, and time) to ensure performance stability.
- **Sensor Abstraction**: A clean `IKMGoapSensorInterface` decouples perception from belief evaluation. Sensors use multicast delegates to notify the agent of target changes, enabling immediate, reactive replanning.
- **Highly Extensible**: Implement custom logic with pluggable search algorithms, agent state machines, and sensor components using standard Unreal Engine interfaces.

---

## Core Concepts

Understanding KMGoap requires familiarity with a few core components that work together to drive agent behavior.

- **`UKMGoapAgentComponent`**: The central brain of the agent. It manages all GOAP data, caches belief evaluations, and coordinates with the planner and state machine.
- **`UKMGoapAgentAction`**: A single, atomic action the agent can perform. Actions are defined by their preconditions (what must be true to start) and their postconditions (what becomes true after completion).
- **`UKMGoapAgentGoal`**: A high-level objective the agent wants to achieve. Goals are defined by a desired world state and a priority value.
- **`UKMGoapAgentBelief`**: A queryable fact about the world from the agent's perspective. Beliefs bridge the gap between raw sensor data and the abstract conditions required by the planner.
- **`IKMGoapSensorInterface`**: An interface for creating sensors that automatically update the agent's beliefs and trigger replanning when the world changes.

---

## Documentation & Wiki

To keep this repository clean and focused, detailed documentation, tutorials, and deep dives into specific systems are hosted in the repository's Wiki. 

Please refer to the following sections to get started and master KMGoap:

| Topic | Description | Link |
| :--- | :--- | :--- |
| **Getting Started & Usage** | Installation instructions, basic setup, and a step-by-step tutorial on creating your first GOAP agent. | [Read the Usage Guide](wiki/Usage.md) |
| **Behavior Breakdown** | Deep dive into how the planner works, the distinction between Facts and Effects, goal priority filtering, and the dynamic knowledge system. | [Read the Behavior Breakdown](wiki/Behavior.md) |
| **Debugging Tools** | Information on how to debug GOAP agents, including planned integrations with Unreal Engine's Visual Logger (`UE_VLOG`) and the `KMGoapGraph` module. | [Read the Debugging Guide](wiki/Debugging.md) |
| **Productivity & Authoring** | Best practices for data-driven authoring, managing `GameplayTags`, and structuring your Action, Belief, and Goal sets for maximum reusability. | [Read the Productivity Guide](wiki/Productivity.md) |

---

## Installation

The recommended way to install KMGoap is as a Git submodule in your project's `Plugins` directory.

```bash
git submodule add https://github.com/khronmiere/kmgoap.git Plugins/KMGoap
```

After cloning, regenerate your project files and enable the plugin via **Edit > Plugins** in the Unreal Editor.

---

## Contributing

Contributions are welcome! Whether it's fixing bugs, improving the planner, or building out the editor tooling modules (`KMGoapEditor` and `KMGoapGraph`), please feel free to fork the repository and submit a pull request.

## License

This plugin is licensed under the MIT License. See the `LICENSE` file for more details.
