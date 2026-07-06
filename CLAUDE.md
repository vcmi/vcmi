# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

VCMI is an open-source C++20 recreation of the Heroes of Might & Magic III engine. One server process manages all game state and mechanics; one or more client processes display the game and collect input. Both share `libvcmi` (`VCMI_lib.dll` / `libvcmi.so`).

## Build System

Uses CMake 3.16+ with `CMakePresets.json`. Linux/macOS use system packages; Windows uses Conan. See `docs/developers/CMake.md` and `docs/developers/Conan.md` for full options.

**Key CMake options:**
- `ENABLE_CLIENT` / `ENABLE_SERVER` / `ENABLE_EDITOR` / `ENABLE_LAUNCHER` — component toggles (all ON by default)
- `ENABLE_TEST` — unit tests (OFF by default)
- `ENABLE_PCH` — precompiled headers via `StdInc.h` (ON by default)
- `CMAKE_BUILD_TYPE` — Debug, Release, or RelWithDebInfo

**Platform presets** (use `cmake -S . --preset=<name>` then `cmake --build --preset=<name>`):
- Linux: `linux-gcc-release`, `linux-gcc-debug`, `linux-gcc-test`, `linux-clang-release`
- Windows: `windows-msvc-release`, `windows-msvc-ninja-release`
- macOS: `macos-conan-ninja-release`, `macos-arm-conan-ninja-release`
- Mobile: `ios-release-conan`, `android-conan-ninja-release`

**Manual Linux build:**
```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build -j$(nproc)
```

## Testing

Tests use Google Test, built under `test/` by subsystem (battle, bonus, entity, spells, serializer, etc.).

```bash
# Configure and build tests
cmake -S . --preset=linux-gcc-test
cmake --build --preset=linux-gcc-test

# Run all tests
ctest --preset linux-gcc-test
# or directly:
./build/bin/vcmitest

# Run a single test suite or test case
./build/bin/vcmitest --gtest_filter=BonusSystemTest.*
./build/bin/vcmitest --gtest_filter=BonusSystemTest.propagation
```

Test fixtures live in `test/testdata/` and are copied to the build directory at configure time.

## Code Style

- **Formatting**: `.clang-format` (Mozilla-based, tabs, next-line braces). Run `clang-format -i <file>` on modified files.
- **Static analysis**: `.clang-tidy`. Run `clang-tidy -p build <file>`.
- **Naming**: Classes `CClassName`, interfaces `IInterfaceName`, members `camelCase`, constants `ALL_CAPS`, enums `EEnumName`. `.cpp`/`.h` filenames start with an uppercase letter; JSON config files start lowercase camelCase.
- **Headers**: `#pragma once`. All `.cpp` files include `StdInc.h` first (precompiled header), before any macros or C++ statements.
- **File content order**: license block → `#pragma once` → includes → forward declarations → code.
- **Enums**: Never in global namespace. Use `enum class` or wrap in a namespace/class.

Every new `.cpp`/`.h` file must start with this license block:
```cpp
/*
 * FileName.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
```

## Architecture

### Directory layout

| Directory | Purpose |
|-----------|---------|
| `lib/` | Core game logic — built as static lib `vcmiMain` |
| `libFacade/` | Aggregates `vcmiMain` + `vcmiLua` + all AI into the shipped `vcmi` shared lib |
| `luascript/` | Lua scripting, built as static lib `vcmiLua` |
| `AI/` | AI modules (static libs linked into facade via `AIFactory`) |
| `client/` | Game client — compiled as `vcmiclientcommon`, linked into `vcmiclient` |
| `server/` | Game server — compiled as `vcmiservercommon` |
| `clientapp/` / `serverapp/` | Executable entry points |
| `launcher/` | Qt6-based launcher |
| `mapeditor/` | Qt6-based map editor |
| `lobby/` | Standalone global lobby server (SQLite) |
| `config/` | JSON game data (creatures, spells, buildings, bonuses, schemas) |
| `scripts/` | Lua game scripts |
| `test/` | Unit tests |
| `docs/developers/` | Developer documentation |

### Key `lib/` subsystems

- **`bonuses/`** — Bonus system (DAG of nodes; propagators push bonuses to ancestors, limiters restrict descendants). Core files: `Bonus.h`, `CBonusSystemNode.h`, `IBonusBearer.h`. See `docs/developers/Bonus_System.md`.
- **`gameState/`** — `CGameState`: map, players, heroes, armies, settings.
- **`callback/`** — Read-only (`CGameInfoCallback`), player-visible (`CPlayerSpecificInfoCallback`), battle (`CBattleCallback`), and server (`CCallback`) interfaces. AI and client use these rather than `CGameState` directly.
- **`networkPacks/`** — All serializable client↔server packets.
- **`serializer/`** — Save/load and network serialization via `h & object` visitor pattern. See `docs/developers/Serialization.md`.
- **`entities/`** — Hero, creature, artifact, spell, building, faction, skill definitions.
- **`mapObjects/`** — Adventure map object types; constructed via `mapObjectConstructors/`.
- **`modding/`** — Mod loading and content registration.
- **`rmg/`** — Random map generator.
- **`pathfinder/`** — Adventure map hero pathfinding.
- **`battle/`** — Core battle rules (damage calc, hexes, unit state). Server processing in `server/battles/`; client UI in `client/battle/`.
- **`spells/`** — Spell definitions and casting logic.
- **`json/`** — JSON parser (`JsonParser.h`), node (`JsonNode.h`), validator (`JsonValidator.h`).
- **`filesystem/`** — LOD archive and mod zip loading.
- **`logging/`** — Named loggers (`logGlobal`, `logNetwork`, `logAi`, etc.) → `vcmi.log`. See `docs/developers/Logging_API.md`.

### Threading model

- **MainGUI** — input + rendering
- **runNetwork** — incoming packet processing + combat AI reactions
- **runServer** — game state updates and request processing
- **AI tasks** — TBB-based parallel tasks (Nullkiller, RMG)

Only the server modifies game state. Clients send request packets; server validates and broadcasts state-change packets to all clients.

### AI modules

Built as static libraries, linked into the `vcmi` facade. Constructed by name through `AIFactory` (`lib/callback/AIFactory.h`). No dynamic loading.

| Module | Role |
|--------|------|
| `BattleAI/` | Default combat AI |
| `Nullkiller2/` | Adventure map AI (TBB-based) |
| `MMAI/` | ML-based combat AI via ONNX Runtime (experimental) |
| `StupidAI/` | Minimal neutral/passive AI |
| `EmptyAI/` | No-op stub for testing |

### Namespace wrapping (iOS/Android)

On mobile all binaries are statically linked into one executable. Lib symbols are isolated by wrapping in a namespace:
- Lib headers/sources: `VCMI_LIB_NAMESPACE_BEGIN` / `VCMI_LIB_NAMESPACE_END`
- Client/server/AI `StdInc.h`: `VCMI_LIB_USING_NAMESPACE`
- Forward declarations of lib types in external code must also use the macros

See `Global.h` for macro definitions and `docs/developers/Code_Structure.md` for examples.

### Important patterns

**`DLL_LINKAGE`** — Required on any class/struct that crosses DLL boundaries (serialized, shared across units):
```cpp
class DLL_LINKAGE CAddInfo {};
struct DLL_LINKAGE Bonus {};
```

**Serialization** — Objects implement `serialize` with the `h & field` pattern (works for both read and write):
```cpp
template <typename Handler>
void serialize(Handler & h) { h & field1 & field2; }
```

**Constants** — Prefer over magic numbers/strings:
- Entity IDs: `lib/constants/EntityIdentifiers.h`
- String IDs: `lib/constants/StringConstants.h`
- Sizes/counts: `lib/constants/NumericConstants.h`
- Game enums: `lib/constants/Enumerations.h`

**Config-driven** — Most game data lives in JSON under `config/` with schemas at `config/schemas/`. Handlers in `lib/entities/` load them.

## Developer Documentation

`docs/developers/` contains detailed guides:
- `Code_Structure.md` — Threading, client-server flow, detailed component map
- `Bonus_System.md` — Bonus DAG architecture
- `Serialization.md` — Serialization framework
- `Battlefield.md` — Battle system
- `Networking.md` — Network protocol
- `Logging_API.md` — Logger usage
- `RMG_Description.md` — Random map generator
- `Lua_Scripting_System.md` — Scripting API
- `Coding_Guidelines.md` — Full C++ style guide
- `Building_*.md` — Platform-specific build instructions
