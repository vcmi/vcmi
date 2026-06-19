# Lua Scripting System

This page describes the internal working of the Lua scripting module. For usage of the public API see modding documentation.

## TODO

- (MAJOR) expand API to make scripts actually usable for modding. [ERM docs](https://azethmeron.github.io/) can serve as reference as to what should be accessible from Lua.
- (MAJOR) expand usage of scripts:
  - convert HotA map scripts into Lua form
  - convert HotA (and possibly - H3) Seer Huts into scripts
  - review ServerCallbackProxy API and expand / cleanup it
  - Implement support for scriptable map objects
  - Move damage calculator, or at least - damage formula to Lua, based on [existing PR](https://github.com/vcmi/vcmi/pull/5135)
  - Move map movement point limit calculation to Lua
  - Move starting armies and starting town building randomization to lua?
  - switch battle events to use scripts
- Review API and decide how to handle following cases:
  - reconsider approach to mutable methods (like BattleHexArrayProxy). Either remove or provide better API bindings approach for such cases. Or convert it to pure Lua class
  - Actually use comparison operator of exposed API classes - currently hard to change without breaking tests
  - consider wrapping Lua userdata into std::any for better type safety, or at least pass classes other than LuaCopyable as ApiShared / ApiPointer
  - check if there is a way to wrap Lua function into C++ wrapper and pass it into LuaFunctionWrapper, or even LuaMethodWrapper
  - add guards against loading values from .json with same name as methods in Lua spell effect script

## Future improvements

- Review UnitState class and check its mutable methods - do we need all of those? Should we name them differently?
- Expand API of classes related to spell effects
- Spell Effect: Add "preprocess" or "initialize" function to initialize parameters (e.g. load string ID and resolve it to Creature type). Would require some way to store references to Lua table in different LuaContext's in LuaSpellEffect class, for example - shared_ptr<LuaReference> in LuaContext, and weak_ptr<LuaReference> in LuaSpellEffect.
- Review usage of numeric identifiers from script such as `PlayerColor` - replace them with enum or with copyable API class.
- Remove final usage of unitID access by script - to set addinfo of bind bonus to unit that initiated binding. Perhaps unify this logic with Clone and treat it as some sort of "unit link" where two units are linked together unless *something* happens (unit dies / moves / unit bonus removed)
- Decide on how to expose random generator to scripting. Currently only generation of integer in range is exposed, but we have way more options, including ability roll system. Expose as separate api class?
- try to remove remaining hardcoded bits of SpellID's: CLONE, STONE_GAZE, SLAYER, AIR_SHIELD, POISON, RESURRECTION, FIRE_SHIELD, DEATH_STARE, as well as some entries in .lua

## General rules

Scripts must be constant and should not generate any side effects.

Exception are scripts that are executed as result of netpack apply (such scripts should be marked as such)

Global state of a Lua script must never change - script should not make assumptions on how many times it was run or in what order were functions called

## Naming rules

- Method names are in camelCase
- Method names must be verbs: `getFoo`, `isFoo`, `setFoo`, `run`, `update`
- Library classes, such as Creature must be passed as pointer like `const Creature *`, not as identifier like `CreatureID`
- If you need to expose identifier, prefer exposing its string form, like one provided via `getJsonKey`

## Documenting the public scripting API

Every binding registered by a proxy carries a description string at the registration site (see the `R.method<...>("name", "description")` pattern in any of the `api/<category>/Xxx.cpp` files). Running

```sh
./vcmiserver --export-lua-docs <output-dir>
```

regenerates two reference files from those descriptions:

- `API.md` — Markdown reference, one section per Lua type with a table of method / signature / description.
- `api.lua` — [Lua Language Server](https://luals.github.io/wiki/definition-files/) stub: `---@meta` header, `---@class` per type, per-method `---@param`/`---@return` annotations. Drop into a luals `Lua.workspace.library` path to get autocomplete in modders' editors.

Both files are emitted from the same `DocRegistrar` pass that the runtime metatable build also goes through, so they stay in lockstep with the host bindings — there is no separate source of truth to keep in sync.

## Script conventions

Every Lua script must return a table. This table becomes the script's *class table* — the set of functions the engine can call on it. The OOP convention used is:

```lua
local MyEffect = {}

function MyEffect:apply(event)
    -- 'self' receives the effect's parameters (from JSON), __index-linked to MyEffect
end

return MyEffect
```

When the engine calls a script function, it constructs a `self` table from the script's JSON parameters and sets its `__index` metamethod to point at the script table. This means `self` carries per-instance data while method lookups fall through to the shared class table.

The following global names are injected by `LuaContext` before the script runs:

| Global | Type | Description |
| ------ | ---- | ----------- |
| `GAME` | `game.Game` | Read-only query interface to the current game state |
| `LIBRARY` | `library.Services` | Access to entity databases (creatures, spells, etc.) |
| `ENUM` | table | Integer constants for all engine enumerations |
| `require` | function | Load a Lua module from the VFS by path (e.g. `require("mod:path/to/module")`) |
| `print` | function | Redirected to the VCMI logger at INFO level |

The following standard Lua globals are **removed** for safety: `collectgarbage`, `dofile`, `load`, `loadfile`, `loadstring`, `string.dump`, `math.random`, `math.randomseed`.

## Architecture overview

```text
ScriptingHandler (engine core)
  └── LuaModule  (DLL plugin, implements Service)
        ├── LuaSpellEffectFactory  (effect type "lua")
        ├── LuaUnitEffectFactory   (effect type "luaUnit")
        └── createPoolInstance()
              └── LuaScriptPool  (owned by CGameState)
                    └── LuaContext  (one per script, per session)
```

Script *source* (path + text) lives in `LuaScriptInstance` objects, which are owned by the effect factories and persist across map restarts. The runnable execution environment — the `lua_State` itself — lives in `LuaContext` and is torn down and recreated on each map restart.

## Classes

### LuaModule

Global entry point for the Lua scripting system. Loaded as a dynamic library plugin by `ScriptingHandler`. Implements the `scripting::Service` interface and exposes two C entry points:

- `GetAiName` — returns the module display name `"Lua interpreter"`
- `GetNewModule` — creates and returns a new `LuaModule` instance

On `installScripting`, registers `LuaSpellEffectFactory` under the effect type key `"lua"`. On `createPoolInstance`, creates a `LuaScriptPool` and registers all currently loaded scripts into it.

### LuaScriptInstance

Stores the source code and identity of a single Lua script. Created by `LuaSpellEffectFactory` when an effect type references a Lua script path. Persists for the lifetime of the module — across map restarts.

Key fields:

- `modScope` — the mod that owns this script (used to scope VFS lookups)
- `sourcePath` — path inside the mod's `Scripts/` directory
- `sourceText` — raw Lua source code loaded from VFS

The identifier exposed to the engine is `modScope:sourcePath`.

### LuaScriptPool

Owned by `CGameState`. Created fresh on each map load via `LuaModule::createPoolInstance`. Holds one `LuaContext` per registered script. Scripts are registered during pool construction; their contexts are initialized (i.e. the Lua chunk is executed once to produce the class table) before gameplay begins.

`getContext(script)` returns the live context for a given `LuaScriptInstance`; called by effect implementations to dispatch Lua function calls.

### LuaContext

Manages a single `lua_State` for one script. Does **not** survive map restarts — it is destroyed and recreated with the owning `LuaScriptPool`.

**Construction** (`LuaContext::LuaContext`):

1. Opens a restricted subset of the standard library (`base`, `table`, `string`, `math`).
2. Strips unsafe globals (`dofile`, `load`, `collectgarbage`, …).
3. Registers all API types from `api::Registry` into the Lua registry and populates the `modules` table.
4. Injects `GAME`, `LIBRARY`, `ENUM`, and the custom `require` function as globals.

**Initialization** (`LuaContext::initialize`):
Executes the script source once via `lua_pcall`. The script must return a table; that table is stored as `scriptTable` (a `LuaReference`). This is the script's class table.

**Dispatch** (`LuaContext::callMethod`):
Template method. Looks up the named function in `scriptTable`, builds a `self` table from a `JsonNode` parameter block (with `__index = scriptTable` as metatable), then calls the function with `self` and any additional C++ arguments pushed by `LuaStack`.

**Module loading** (`LuaContext::require` / `LuaContext::loadModule`):
Handles `require("scope:path")` from scripts. Resolves the path through the VFS (prepending `SCRIPTS/`), compiles and runs the chunk, and returns the resulting table to the script.

**Thread safety**: Each `LuaContext` holds a `std::mutex`. `hasFunction` and `callMethod` both lock it, making concurrent calls from different threads safe (but serialized).

### LuaReference

RAII wrapper around the Lua registry (`luaL_ref` / `luaL_unref`). Holds a value (table, function, etc.) in the Lua registry so it is not garbage-collected. Provides `push()` to put the referenced value back on the active stack.

Used inside `LuaContext` to hold:

- `modules` — the table of all registered API modules
- `scriptClosure` — the compiled but not-yet-executed script chunk
- `scriptTable` — the table returned by the script on first execution

### LuaStack

Central typed interface between C++ and the Lua stack. Constructed with a `lua_State *`; records `lua_gettop` at construction so `balance()` can assert the stack is unchanged.

**Pushing** (`push` overloads):
Handles all VCMI types uniformly through template specialization:

- Primitives: `bool`, integers, enums, `IdentifierBase` subtypes → `lua_pushinteger`
- `std::string`, `const char *` → `lua_pushlstring`
- `JsonNode` → Lua table (recursive)
- `std::vector<T>`, `boost::container::small_vector` → Lua array table
- `std::map<std::string, T>` → Lua hash table
- `ApiSerializable` subtypes → Lua table via `serializeScript` callback
- `ApiRawPointer *` → userdata + metatable looked up in `Registry`
- `std::shared_ptr<ApiSharedPointer>` → userdata + metatable
- `ApiCopyable` → userdata copy + metatable

**Reading** ( `get` / `getNonNull` ):
Mirror image of push; throws `LuaApiException` on type mismatch. For pointer types, validates the userdata's metatable against the registry entry before casting.

### LuaApiException

`std::runtime_error` subclass thrown by `LuaStack` when a type mismatch or missing value is encountered. Caught by the `LuaMethodWrapper` / `LuaFunctionWrapper` / `LuaCallWrapper` invoke wrappers and re-raised as a Lua error via `lua_error`.

### LuaCallWrapper, LuaMethodWrapper, LuaFunctionWrapper

Template wrappers that bridge C++ callables and Lua's `lua_CFunction` signature (`int(lua_State*)`).

**`LuaCallWrapper<func>`** — the simplest wrapper. For functions that already have the correct `int(lua_State*)` signature. Wraps the call in a `try/catch` that converts C++ exceptions to Lua errors.

**`LuaFunctionWrapper<func>`** — for plain C++ free functions or static proxy methods. Uses `LuaFunctionTraits` to decompose the function signature, pulls all arguments from the Lua stack starting at index 1, invokes the function, and pushes the return value (if any).

**`LuaMethodWrapper<ObjectType, MethodType, method>`** — for member functions of proxy classes. Pulls `self` (as raw pointer, shared_ptr, or by-value copy depending on `ObjectType`'s tag base class) from stack position 1, then pulls remaining arguments from positions 2…N. Invokes the member function and pushes the result.

All three wrappers catch `std::exception` and call `lua_error`, which performs a `longjmp` — so there must be no local variables with non-trivial destructors in the `invoke` function body.

### RawPointerWrapper, SharedPointerWrapper, CopyableWrapper

CRTP Registar implementations used to register proxy classes with the Lua engine. Each creates the appropriate Lua metatable structure when `pushMetatable` is called during `LuaContext` construction.

**`RawPointerWrapper<T, Proxy>`** — for classes tagged `ApiRawPointer`. Creates two metatables: one for `T*` and one for `const T*`. Both share the same `__index` table populated from `Proxy::REGISTER_CUSTOM`. Does **not** install `__gc` since raw pointers are not owned by Lua.

**`SharedPointerWrapper<T, Proxy>`** — for classes tagged `ApiSharedPointer`. Creates one metatable for `std::shared_ptr<T>` with `__gc` to destruct the shared_ptr (decrementing the refcount). Also pushes a static constructor table with `new()`.

**`CopyableWrapper<T, Proxy>`** — for classes tagged `ApiCopyable`. Stores a full copy inside Lua userdata. Installs `__gc` to call the destructor. Also pushes a static constructor table with `new()`.

Each wrapper builds a static table (accessible by module name in the `modules` global) and a per-instance metatable (used when accessing methods on a userdata value).

### RegistarBase

Abstract base for the three wrapper types above. Provides virtual `adjustMetatable` and `adjustStaticTable` hooks (both no-ops by default) that derived wrappers can override to add extra entries.

### api::Registar

Pure interface (`pushMetatable(lua_State*)`) implemented by `RegistarBase`. The `Registry` stores `Registar` instances and calls `pushMetatable` once per `LuaContext` during `registerPublicTypes`.

### api::Registry

Singleton (access via `Registry::get()`). Constructed once at program startup; its constructor calls `registerPrivate` for every known proxy type, associating a human-readable dotted name (e.g. `"battle.Unit"`) with the corresponding `Registar` instance.

`getTypeName<T>()` returns `typeid(T).name()` as the metatable key used in the Lua registry. This is an opaque internal key; scripts never see it directly.

`find(name)` looks up a type in the public map (currently all types are registered as private, meaning they are accessible from scripts but not listed in the public API).

### LuaSpellEffect and LuaSpellEffectFactory

`LuaSpellEffectFactory` is registered under the JSON effect type key `"lua"`. When `SpellEffectService` encounters this type during mod loading it calls `initialize(scope, name)` to load the Lua script, then `create(scope, name)` to return a `LuaSpellEffect` for each spell that uses it.

`LuaSpellEffect` implements the full `spells::effects::Effect` interface by resolving the active `LuaContext` from the current `Mechanics` object and delegating each virtual method call to the correspondingly named Lua function:

| C++ virtual | Lua function |
| ----------- | ------------ |
| `adjustTargetTypes` | `adjustTargetTypes` |
| `adjustAffectedHexes` | `adjustAffectedHexes` |
| `applicableGeneral` | `applicableGeneral` |
| `applicableTarget` | `applicableTarget` |
| `apply` | `apply` |
| `filterTarget` | `filterTarget` |
| `transformTarget` | `transformTarget` |
| `getHealthChange` | `getHealthChange` |

JSON effect parameters (from the spell definition) are serialized into the `self` table passed to each Lua call.

## Exposing a class to Lua scripts

1. **Choose a lifetime model** and inherit the C++ class from the matching API tag:
   - `scripting::ApiSerializable` — serialize as a Lua table (POD, no proxy needed); implement `serializeScript(auto & s)`
   - `scripting::ApiCopyable` — copy into Lua userdata; for small value types without inheritance
   - `scripting::ApiRawPointer` — pass raw pointer; for long-lived singletons or interfaces
   - `scripting::ApiSharedPointer` — pass `shared_ptr`; for short-lived objects or interfaces with shared ownership

2. **Create a proxy class** `XXXProxy` in the appropriate `luascript/api/` subdirectory. The proxy inherits from the matching wrapper template (`RawPointerWrapper<X, XXXProxy>` etc.) and declares a static `REGISTER_CUSTOM` array of `CustomRegType` entries, each mapping a Lua method name to a C function.

3. **Wrap each exposed method** using one of:
   - `LuaMethodWrapper<ObjectType, MethodType, &C::method>::invoke` — direct method wrapper
   - `LuaFunctionWrapper<&XXXProxy::adaptedMethod>::invoke` — static adapter with custom signature
   - `LuaCallWrapper<&XXXProxy::rawMethod>::invoke` — for methods already in `int(lua_State*)` form

4. **Register the proxy** in `Registry::Registry()` in `api/Registry.cpp` with `registerPrivate<XXXProxy>("module.Name")`.

5. **Expose instances** by returning the object from an existing API call or by passing it as an argument when invoking a script callback.

## Data flow: a spell effect call

```text
Engine calls LuaSpellEffect::apply(server, mechanics, target)
  → resolveScript(mechanics) → LuaScriptPool::getContext(script) → LuaContext
  → LuaContext::callMethod<void>("apply", parameters, server, mechanics, target)
      → lock mutex
      → LuaStack: push function from scriptTable
      → LuaStack: build self = {params...} with __index = scriptTable
      → LuaStack: push server (ServerCb userdata), mechanics (SpellMechanics userdata), target (...)
      → lua_pcall(L, argc, 1, 0)
      → Lua script: function MyEffect:apply(server, mechanics, target) ... end
      → LuaStack: pop result, unlock mutex
```
