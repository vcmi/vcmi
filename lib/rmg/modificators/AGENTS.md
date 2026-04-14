# Modifier Agent Notes (`lib/rmg/modificators`)

This file is for modifier-specific work only.
Top-level RMG guidance is in [`../AGENTS.md`](../AGENTS.md).

## Core Model

- Each modifier derives from [`Modificator`](./Modificator.h) ([`Modificator.cpp`](./Modificator.cpp)).
- `init()` declares dependencies/postfunctions.
- `process()` performs the actual generation step.
- Scheduling readiness/finish state is managed by the base class and dependency graph.

## Where Modifiers Are Wired

- Registration happens in [`../RmgMap.cpp`](../RmgMap.cpp) (`RmgMap::addModificators`).
- If you add a modifier, wire it there and verify which zone types/levels should receive it.

## Dependency Rules

- Always express ordering via explicit dependencies.
- Use existing dependency helpers/macros consistently with nearby code.
- Do not rely on incidental execution order from container traversal.

When changing dependencies, check all affected modifiers that consume the same shared state.

## Concurrency And Locking

- Be careful with cross-zone operations and lock ordering.
- Reuse existing lock-order patterns (for example, zone-lock coordination in connection-related code).
- Protect shared zone area mutations with zone-level synchronization.
- Use map-edit paths that already serialize writes (through existing map proxy/edit helpers).

Avoid introducing new lock orders unless necessary.

## Practical Workflow For Modifier Changes

1. Identify state read/written by the modifier (`area*`, object queues, routes, guards, map tiles).
2. Confirm all producers are listed as dependencies in `init()`.
3. Implement `process()` changes in small steps and keep side effects local.
4. Re-check registration in [`../RmgMap.cpp`](../RmgMap.cpp) for scope (land/water/underground/global).
5. Verify no downstream modifier now depends on undocumented side effects.
