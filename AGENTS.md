# AGENTS.md

This file provides guidance for autonomous coding agents when working with code in this repository.

## Project Overview

VCMI is an engine for the game Heroes of Might and Magic III. The project is structured around a client-server architecture with a shared library. The codebase is primarily C++ with some CMake build configuration, Qt for UI (launcher and map editor), and Lua/ERM for scripting.

**Architecture**: One server process handles game state and mechanics. One or more client processes display the game and collect player input. Both use the shared VCMI lib.

## Build System

### CMake

The project uses CMake with Conan package manager for dependency management on Windows. On other platforms, system package managers are used. See [`docs/developers/CMake.md`](docs/developers/CMake.md) for all CMake options and [`docs/developers/Conan.md`](docs/developers/Conan.md) for Conan setup.

### C++ standard

VCMI is built as **C++20**: the root `CMakeLists.txt` sets `CMAKE_CXX_STANDARD` to `20` with `CMAKE_CXX_STANDARD_REQUIRED ON`. Treat C++20 as both the minimum supported language and the dialect to prefer for new code.

For platform-specific build and test instructions see [`docs/developers/Building_Windows.md`](docs/developers/Building_Windows.md), [`docs/developers/Building_Linux.md`](docs/developers/Building_Linux.md), [`docs/developers/Building_macOS.md`](docs/developers/Building_macOS.md), [`docs/developers/Building_Android.md`](docs/developers/Building_Android.md), [`docs/developers/Building_iOS.md`](docs/developers/Building_iOS.md).

## Code Architecture

### Directory Structure

- **lib/** - Core static library `vcmiMain` with core game logic and data structures.
  - Aggregated into the shipped shared library `vcmi` (`VCMI_lib.dll` / `libvcmi.so`) by the **libFacade/** target along with all AI implementations and `vcmiLua`. Consumers always link the facade target `vcmi`, not `vcmiMain` directly.
  - **battle/** - Battle system (damage calculation, pathfinding, unit state)
  - **bonuses/** - Bonus system (core mechanics for granting attributes to units/heroes)
  - **callback/** - Interfaces for accessing game state (used by AI and client)
  - **entities/** - Game objects (heroes, creatures, artifacts, spells, buildings)
  - **gameState/** - CGameState and related classes
  - **mapping/** - Map loading/saving
  - **mapObjects/** - Adventure map objects (towns, dwellings, mines, etc.)
  - **mapObjectConstructors/** - Constructors for map objects
  - **network/** - Network layer (connection management)
  - **networkPacks/** - Network packet definitions and serialization
  - **json/** - JSON parsing/validation/writing
  - **filesystem/** - Archive and file loading
  - **spells/** - Spell definitions and casting logic
  - **modding/** - Mod loading and content management
  - **pathfinder/** - Hero pathfinding on adventure map
  - **serializer/** - Serialization framework (save/load, network)
  - **rmg/** - Random map generator
  - **campaign/** - Campaign progression and scenario logic
  - **texts/** - Text handling and localization support

- **client/** - Game client
  - **adventureMap/** - Adventure mode UI and logic
  - **battle/** - Battle UI and rendering
  - **gui/** - GUI framework (CIntObject base class and components)
  - **eventsSDL/** - Input handling (keyboard, mouse, touch, gamepad)
  - **lobby/** - Local game setup screens
  - **globalLobby/** - Online/global lobby UI and client
  - **mainmenu/** - Main menu and game selection
  - **mapView/** - Map rendering
  - **render/** - Rendering abstractions and interfaces
  - **renderSDL/** - SDL-based rendering backend
  - **widgets/** - Reusable UI widget components
  - **windows/** - Game windows and dialogs

- **server/** - Game server (compiled as vcmiservercommon library, shared by serverapp)
  - **battles/** - Battle flow processing
  - **queries/** - Player queries and responses
  - **processors/** - Game state processors (turns, heroes, etc.)

- **serverapp/** - Standalone server executable entry point
- **clientapp/** - Client executable entry point (also handles iOS/Android specifics)
- **lobby/** - Standalone global lobby server (SQLite-backed, separate from game server)
- **launcher/** - Qt-based game launcher
- **mapeditor/** - Qt-based map editor
- **luascript/** - Lua scripting host, built as the static library `vcmiLua` and aggregated into `vcmi` by libFacade
- **libFacade/** - Tiny aggregator target that produces the shipped `vcmi` shared library by linking `vcmiMain` + `vcmiLua` + every enabled AI
- **config/** - Game configuration file (json-with-comments format)
- **scripts/** - Lua scripts used by the game
- **AI/** - AI modules, each built as a static archive and linked into the `vcmi` facade. They are constructed by name through `AIFactory` in `lib/callback/AIFactory.h`; there is no dynamic library loading.
  - **BattleAI/** - combat AI (default)
  - **Nullkiller2/** - Modern adventure map AI (default)
  - **MMAI/** - Machine-learning-based combat AI (experimental)
  - **StupidAI/** - Minimal combat AI for neutral/passive players
  - **EmptyAI/** - Stub AI (no-op, used for testing)

### Key Concepts

#### Bonus System

One of the most important systems. Every bonus (attribute, resistance, spell immunity, etc.) is stored in a bonus system that propagates through a DAG of nodes. Key files:

- `lib/bonuses/Bonus.h` - Bonus structure
- `lib/bonuses/CBonusSystemNode.h` - Node in the bonus graph
- `lib/bonuses/IBonusBearer.h` - Interface for objects that can have bonuses

Bonuses have:

- **Propagators** - Rules for which ascendants receive the bonus
- **Limiters** - Restrictions on which descendants receive the bonus (e.g., only griffins)
- **Inheritance** is automatic through the DAG; propagation requires explicit propagators

More details in [`docs/developers/Bonus_System.md`](docs/developers/Bonus_System.md).

#### Game State

Accessed through `CGameState` in `lib/gameState/CGameState.h`. Contains:

- Map and all map objects
- Player information
- Heroes and their armies
- Game settings and options

See [`docs/developers/Code_Structure.md`](docs/developers/Code_Structure.md) for an overview of how game state fits into the overall architecture.

#### Callbacks

The lib exposes game state through callback interfaces:

- `CGameInfoCallback` - Read-only game info
- `CPlayerSpecificInfoCallback` - Player-specific visible state
- `CBattleCallback` - Battle-specific state
- `CCallback` - Server callback for client requests

AI and player interfaces use these callbacks rather than directly accessing game state.

#### Networking

Network layer:

- `lib/network/NetworkConnection.h` - Low-level connection
- `lib/networkPacks/` - Serializable network packets

All changes to game state must go through server via network packets. More details in [`docs/developers/Networking.md`](docs/developers/Networking.md).

#### Threading Model

- **MainGUI** - Main thread, input processing and rendering
- **runNetwork** - Network thread, processes incoming packets, runs combat AI reactions
- **runServer** - Server thread, processes requests and game state updates
- **AI tasks** - TBB-based tasks for adventure AI (Nullkiller)

See [`docs/developers/Code_Structure.md`](docs/developers/Code_Structure.md) for detailed threading information.

### Namespace Wrapping (iOS/Android Considerations)

On mobile systems, all binaries are built into a single executable & linked statically. The lib symbols are wrapped in a namespace to keep them isolated. This is handled by:

- `VCMI_LIB_NAMESPACE_BEGIN` / `VCMI_LIB_NAMESPACE_END` macros in lib code
- Forward declarations of lib symbols in external code must also use these macros
- In new project parts, add `VCMI_LIB_USING_NAMESPACE` to `StdInc.h`

See [`docs/developers/Code_Structure.md`](docs/developers/Code_Structure.md) for examples.

## Common Development Tasks

### Adding a New Game Mechanic

1. Add bonus type or modify `lib/bonuses/BonusEnum.h` if needed
2. Implement logic in lib (typically in entity handlers or callback implementations)
3. Add serialization support if it affects saved games
4. Add serialization compatibility for older saves
5. Update network packets if client-server communication is needed
6. Add tests in `test/`
7. Update client UI if player-visible changes needed

### Modifying Battle Logic

Battle logic is split:

- `lib/battle/` - Core rules and state
- `server/battles/` - Server-side processing
- `client/battle/` - Rendering and UI

Changes to rules should go in `lib/battle/` (especially `DamageCalculator.h`, `BattleInfo.h`, etc.). See [`docs/developers/Battlefield.md`](docs/developers/Battlefield.md) for details on the battle system.

### Working with Configuration Files

Game configuration uses JSON:

- `config` directory contains configuration of all game entities and settings. It also contains JSON schemas for entities, under `config/schemas` path.
- JSON parsing: `lib/json/JsonParser.h`, `lib/json/JsonNode.h`
- JSON validation: `lib/json/JsonValidator.h`

Configuration is loaded by handlers in `lib/entities/` (creature handler, spell handler, hero handler, etc.).

### Running the Game

After building, configure game data:

1. Copy Heroes III `Data`, `Maps`, `Mp3` folders to `%USERPROFILE%\Documents\My Games\vcmi\`
2. On Windows, run `build/bin/RelWithDebInfo/VCMI_launcher.bat`
3. On Linux, run `build/bin/vcmiclient` or use launcher if built

## Debugging and Logging

### Logging

- `lib/logging/CLogger.h` - Logger class
- Logs are written to `vcmi.log`
- Most subsystems have named loggers: `logGlobal`, `logNetwork`, `logAi`, etc.

More details in [`docs/developers/Logging_API.md`](docs/developers/Logging_API.md).

### Visual Studio Debugging

- Use `RelWithDebInfo` build type for debugging with optimizations
- Debug information is included in this build type
- Full Debug builds are very slow due to unoptimized dependency builds

## Important Patterns and Conventions

### C++ style

Follow the project's C++ conventions in [`docs/developers/Coding_Guidelines.md`](docs/developers/Coding_Guidelines.md) (formatting, naming, and general style).

### Cross-DLL types (`DLL_LINKAGE`)

For classes and structs that are serialized or otherwise shared across compile units, use the `DLL_LINKAGE` macro on the declaration:

```cpp
class DLL_LINKAGE CAddInfo {};
struct DLL_LINKAGE Bonus {};
```

### Constants and identifiers

Prefer existing constants over magic numbers or hard-coded strings:

- **Numeric IDs** (creatures, buildings, and other entities): `lib/constants/EntityIdentifiers.h`
- **String IDs** (configs, mods): `lib/constants/StringConstants.h`
- **Sizes and counts** (gameplay-related): `lib/constants/NumericConstants.h`
- **Game-related enums / mechanic states**: `lib/constants/Enumerations.h`

### Serialization and state

- **Serialization**: Use the custom serialization framework in `lib/serializer/`. Objects implement `h & object` pattern with serialization visitors. More details in [`docs/developers/Serialization.md`](docs/developers/Serialization.md).
- **Game state modifications**: Only server can modify state. Client can only send requests to change gamestate to server, server validates requests and sends resulting changes in gamestate to clients

## Dependencies

Major dependencies (managed by Conan):

- SDL2 - Graphics rendering
- Qt5/Qt6 - Launcher and map editor UI
- Boost - Various utilities
- FFmpeg - Video support
- Lua/LuaJIT - Scripting (optional), see [`docs/developers/Lua_Scripting_System.md`](docs/developers/Lua_Scripting_System.md)
- FuzzyLite - Fuzzy logic for AI
- Intel TBB - Parallel algorithms for AI and map generation
