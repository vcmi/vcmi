# Lua Scripting System

This page describes the internal working of the Lua scripting module. For usage of the public API see modding documentation.

## TODO

- (MAJOR) expand API to make scripts actually usable for modding. [ERM docs](https://azethmeron.github.io/) can serve as reference as to what should be accessible from Lua.
- (MAJOR) expand usage of scripts:
  - convert HotA map scripts into Lua form
  - convert HotA (and possibly - H3) Seer Huts into scripts
  - review ServerCallbackProxy API and expand / cleanup it
  - Implement support for scriptable map objects
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

Global state of a Lua script must never change - script should not make assumptions on how many times it was run or in what order were functions called. `LuaScriptPool` creates a separate `lua_State` of every script for each thread, so state accumulated in a global or in the script table differs between threads. The per-call `self` table is the only state a script may rely on. Map event scripts run by `LuaMapEventDispatcher` are the exception: the dispatcher owns its context directly instead of taking it from the pool, so it can suspend coroutines.

## Naming rules

- Method names are in camelCase
- Method names must be verbs: `getFoo`, `isFoo`, `setFoo`, `run`, `update`
- Library classes, such as Creature must be passed as pointer like `const Creature *`, not as identifier like `CreatureID`
- If you need to expose identifier, prefer exposing its string form, like one provided via `getJsonKey` or `getNameTextID`, instead of a numeric index

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
| `GAME` | `Game` | Read-only query interface to the current game state |
| `LIBRARY` | `Services` | Access to entity databases (creatures, spells, etc.) |
| `ENUM` | table | Integer constants for all engine enumerations |
| `require` | function | Load a Lua module from the VFS by path (e.g. `require("mod:path/to/module")`) |
| `print` | function | Redirected to the VCMI logger at INFO level |
| `error`, `assert` | function | Standard behavior; the message is also logged as a warning |

The following standard Lua globals are **removed** for safety: `collectgarbage`, `dofile`, `load`, `loadfile`, `loadstring`, `string.dump`, `math.random`, `math.randomseed`.

## Architecture overview

```text
GameLibrary::initializeLibrary
  └── LuaModule  (implements scripting::Service)
        ├── LuaScriptStore    (every loaded Lua script, of every kind)
        ├── LuaScriptFactory  (the script factory, registered with ScriptService)
        │     ├── createSpellEffect()             -> LuaSpellEffect
        │     ├── createCombatEventScript()       -> LuaCombatEventScript
        │     └── createDamageCalculatorScript()  -> LuaDamageCalculatorScript
        ├── createPoolInstance()
        │     └── LuaScriptPool  (owned by CGameState)
        │           └── LuaContext  (one per script, per thread)
        └── createMapScriptDispatcher()
              └── LuaMapEventDispatcher  (owned by CGameState, owns its LuaContext)
```

`vcmiLua` is a static library aggregated into the `vcmi` shared library by `libFacade`. It links against `luajit::luajit`, `lua::lua`, or `Lua::Lua`, depending on what Conan or the system provides.

Scripts of every kind are one entity - see [Script Types](../modders/Lua/Script_Types.md). `ScriptHandler` owns the registry and knows nothing about Lua; `LuaScriptFactory` is the only place that knows both a script kind and a language, because marshalling a call is specific to both.

Script *source* (path + text) lives in `LuaScriptInstance` objects, which are owned by `LuaScriptStore` and persist across map restarts. The runnable execution environment — the `lua_State` itself — lives in `LuaContext` and is torn down and recreated on each map restart.

## Classes

### LuaModule

Entry point of the Lua scripting system. `GameLibrary::initializeLibrary` constructs it directly. Implements the `scripting::Service` interface.

On `installScripting`, registers `LuaScriptFactory` with the `ScriptService` as the factory every script goes through - scripts do not name the language they are written in. On `createPoolInstance`, creates a `LuaScriptPool` and registers all currently loaded scripts into it. On `createMapScriptDispatcher`, creates a `LuaMapEventDispatcher` for the map script, if the map has one. `exportDocs` writes the API reference, see [Documenting the public scripting API](#documenting-the-public-scripting-api).

### LuaScriptInstance

Stores the source code and identity of a single Lua script. Created by `LuaScriptStore` for every script declared in json. Persists for the lifetime of the module — across map restarts.

`layers` holds the source of the script: `layers[0]` is the base script, the remaining entries are its patches in declared order. Each layer carries its source text and an identifier `modScope:sourcePath` used for error reporting and chunk naming. See [Patched scripts](#patched-scripts).

### LuaScriptPool

Owned by `CGameState`. Created fresh on each map load via `LuaModule::createPoolInstance`. Scripts are registered during pool construction.

`getContext(script)` returns the context of the given script for the calling thread, creating and initializing it on first use. Each thread has its own set of contexts (`tbb::enumerable_thread_specific`), so scripts run concurrently without locking; this is what allows AI to evaluate spells in parallel. `registerScript` creates the context of the registering thread immediately, so a broken script is reported when the session starts.

### LuaContext

Manages a single `lua_State` for one script. Does **not** survive map restarts — it is destroyed and recreated with the owning `LuaScriptPool`. A context is used only by the thread that created it and has no locking.

**Construction** (`LuaContext::LuaContext`):

1. Opens a restricted subset of the standard library (`base`, `table`, `string`, `math`, `coroutine`).
2. Strips unsafe globals (`dofile`, `load`, `collectgarbage`, …).
3. Registers all API types from `api::Registry` into the Lua registry and populates the `modules` table.
4. Injects `GAME`, `LIBRARY`, `ENUM`, and the custom `require` function as globals.

**Initialization** (`LuaContext::initialize`):
Executes every layer of the script in order in the same `lua_State`. Each layer must return a table. The table returned by the last successful layer is stored as `scriptTable` (a `LuaReference`); this is the script's class table.

**Dispatch** (`LuaContext::callMethod`, `LuaContext::callFunction`):
Template methods. Look up the named function in `scriptTable` and call it with the C++ arguments pushed by `LuaStack`. `callMethod` additionally builds a `self` table from a `JsonNode` parameter block (with `__index = scriptTable` as metatable) and passes it first. A missing function, a Lua error, or a return value of the wrong type is logged and the call returns a default-constructed value. If `scriptTable` is null, because the base layer failed, the call does nothing.

**Module loading** (`LuaContext::require` / `LuaContext::loadModule`):
Handles `require("scope:path")` from scripts. Resolves the path through the VFS (prepending `SCRIPTS/`), compiles and runs the chunk, and returns the resulting table to the script.

### LuaReference

RAII wrapper around the Lua registry (`luaL_ref` / `luaL_unref`). Holds a value (table, function, etc.) in the Lua registry so it is not garbage-collected. Provides `push()` to put the referenced value back on the active stack.

Used inside `LuaContext` to hold:

- `modules` — the table of all registered API modules
- `scriptTable` — the table returned by the script on first execution

### LuaStack

Central typed interface between C++ and the Lua stack. New code uses it instead of calling `lua_push*` / `lua_to*` directly. Constructed with a `lua_State *`; records `lua_gettop` at construction.

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

**Stack discipline**: every code path that pushes values either consumes them or calls `restoreInitialTop()` before returning, including paths that throw. `clear()` empties the whole stack frame; outside of a cfunction body use `restoreInitialTop()` instead, because a script can re-enter `callMethod` and `clear()` would remove the values of the caller.

### LuaApiException

`std::runtime_error` subclass thrown by `LuaStack` when a type mismatch or missing value is encountered. Caught by the `LuaMethodWrapper` / `LuaFunctionWrapper` / `LuaCallWrapper` invoke wrappers and re-raised as a Lua error via `lua_error`.

### LuaCallWrapper, LuaMethodWrapper, LuaFunctionWrapper

Template wrappers that bridge C++ callables and Lua's `lua_CFunction` signature (`int(lua_State*)`). Bindings do not use them directly: `MethodRegistrar` selects the wrapper for each registered method.

**`LuaCallWrapper<func>`** — the simplest wrapper. For functions that already have the correct `int(lua_State*)` signature. Wraps the call in a `try/catch` that converts C++ exceptions to Lua errors.

**`LuaFunctionWrapper<func>`** — for plain C++ free functions or static proxy methods. Uses `LuaFunctionTraits` to decompose the function signature, pulls all arguments from the Lua stack starting at index 1, invokes the function, and pushes the return value (if any).

**`LuaMethodWrapper<ObjectType, MethodType, method>`** — for member functions of proxy classes. Pulls `self` (as raw pointer, shared_ptr, or by-value copy depending on `ObjectType`'s tag base class) from stack position 1, then pulls remaining arguments from positions 2…N. Invokes the member function and pushes the result.

All three wrappers catch `std::exception` and call `lua_error`, which performs a `longjmp` — so there must be no local variables with non-trivial destructors in the `invoke` function body. For the same reason, bound methods report errors by throwing a C++ exception and never call `lua_error` themselves.

### RawPointerWrapper, SharedPointerWrapper, CopyableWrapper

CRTP Registar implementations used to register proxy classes with the Lua engine. Each creates the appropriate Lua metatable structure when `pushMetatable` is called during `LuaContext` construction. The methods of the metatable come from `Proxy::registerMethods`, driven through `LuaRegistrar`. Each wrapper `static_assert`s that the bound class inherits exactly one API tag.

**`RawPointerWrapper<T, Proxy>`** — for classes tagged `ApiRawPointer`. Creates two metatables: one for `T*` and one for `const T*`. Does **not** install `__gc` since raw pointers are not owned by Lua.

**`SharedPointerWrapper<T, Proxy>`** — for classes tagged `ApiSharedPointer`. Creates one metatable for `std::shared_ptr<T>` with `__gc` to destruct the shared_ptr (decrementing the refcount). Also pushes a static constructor table with `new()`.

**`CopyableWrapper<T, Proxy>`** — for classes tagged `ApiCopyable`. Stores a full copy inside Lua userdata. Installs `__gc` to call the destructor. Also pushes a static constructor table with `new()`.

Each wrapper builds a static table (accessible by module name in the `modules` global) and a per-instance metatable (used when accessing methods on a userdata value).

`SerializableRegistar<T>` is the Registar of `ApiSerializable` types. They have no methods and no metatable; it exists so that the doc exporter can list their fields through `FieldDocRegistrar`.

### RegistarBase

Abstract base for the three wrapper types above. Provides virtual `adjustMetatable` and `adjustStaticTable` hooks (both no-ops by default) that derived wrappers can override to add extra entries.

### api::Registar

Interface stored by the `Registry`, one instance per Lua-visible type:

- `pushMetatable(lua_State*)` — called once per `LuaContext` during `registerPublicTypes`
- `collectDocs(MethodRegistrar &)` — runs `Proxy::registerMethods` against the given visitor
- `collectFields(FieldDocRegistrar &)` — lists the fields of an `ApiSerializable` type
- `getDescription()` — returns `Proxy::luaDescription`, the class-level description

### api::Registry

Singleton (access via `Registry::get()`). Constructed once at program startup; its constructor calls `registerPrivate<Proxy>()` for every proxy type and `registerSerializable<T>()` for every `ApiSerializable` type. The Lua-facing name of each type is `Proxy::luaName`.

`getTypeName<T>()` returns `typeid(T).name()` as the metatable key used in the Lua registry. This is an opaque internal key; scripts never see it directly.

`lookupLuaName(typeid(T))` returns the Lua-facing name of a C++ type, used to derive binding signatures. Registration of a proxy adds `typeid(Proxy::ObjectType) → Proxy::luaName`. Types without a proxy of their own - abstract interfaces, POD descriptors, named enums - are added by explicit `registerLuaName<T>("Name")` calls at the end of `Registry::Registry()`, for example `CGObjectInstance` → `MapObject`, `JsonNode` → `any`, `CBattleInfoCallback` → `Battle`. A type that is not registered makes `lookupLuaName` throw `std::runtime_error` with the demangled C++ name, which aborts both Lua context creation and doc export.

`find(name)` looks up a type in the public map (currently all types are registered as private, meaning they are accessible from scripts but not listed in the public API).

### api::MethodRegistrar

Visitor interface that every proxy's `static void registerMethods(MethodRegistrar &)` is written against. The templated helpers build one `DocEntry` per binding:

- `method<&Class::method>` — a member function of the bound class. The optional second template argument names the class that declares the method, required when it is declared on an untagged base class.
- `function<&Proxy::helper>` — a static proxy helper whose first argument is `self`.
- `cfunction<&Proxy::raw>` — a raw `int(lua_State*)` function.

Each helper takes the binding name, an array of parameters (`LuaParam`: name and optional description), a return description (`LuaReturn`), and the mandatory binding description. `method` and `function` derive parameter and return types from the C++ signature through `SignatureOf.h` and `static_assert` that the parameter array matches the argument count; `cfunction` has no signature to derive from, so it takes `LuaCParam` / `LuaCReturn`, which spell out the Lua types.

Two implementations consume the entries:

- `LuaRegistrar` — pushes name → C function pairs onto a Lua table at runtime and ignores the documentation.
- `DocRegistrar` — collects the complete entries for the doc exporter.

Because both run the same `registerMethods`, the generated reference cannot drift from the runtime bindings.

Bindings shared by several proxies live in leaf-templated binding groups in `api/library/`: `EntityBindings<Leaf>` (`getJsonKey`, for every `Entity`) and `BonusBearerBindings<Leaf>` (`getBonuses`, `getBonusesValue`, `hasBonuses`, for `IBonusBearer`). A leaf proxy calls `XxxBindings<Leaf>::registerMethods(R)` from its own `registerMethods`. A group is worth creating only when the leaves share the C++ method pointers, i.e. the methods live on a common base. Bindings that share Lua names but call different C++ methods (the stat accessors of `UnitProxy` and `LuaUnitStateProxy`) stay in their leaves.

### LuaSpellEffect and LuaScriptFactory

`LuaScriptFactory` is the single factory `ScriptService` knows. When `ScriptHandler` loads a script it calls `initialize(description)`, which stores the sources in `LuaScriptStore`. Adding a second language means replacing this factory, or teaching it to dispatch - a script itself declares only what it implements, never what it is written in. A script declaring `"implements" : "spellEffect"` is then wrapped into a `LuaSpellEffect` by `createSpellEffect`, once per spell that uses it; one declaring `"implements" : "combatEvent"` is wrapped once into a shared, stateless `LuaCombatEventScript`; one declaring `"implements" : "damageCalculator"` is wrapped into a `LuaDamageCalculatorScript`.

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

Scripts that implement a spell effect use the proxies in `api/spells/` (`Mechanics`, `Problem`) and do not access the `spells::effects` interfaces directly.

### LuaMapEventDispatcher

Runs the Lua event script generated for a map and forwards object visits, player turn starts and town turn starts to its handlers. It creates its own `LuaScriptInstance` from the map's script source and owns the resulting `LuaContext`, so it is not bound by the pool's statelessness rule and can suspend and resume coroutines.

## Patched scripts

A `scripts` entry declares a `patches` array (see [Script Types](../modders/Lua/Script_Types.md)). `ScriptHandler` reads each entry as a pair of the mod scope of that json node (`getModScope()`) and the script path, so several mods can append patches to the same script without knowing about each other. `LuaScriptStore` passes the list to `LuaScriptInstance`, which loads it into `layers`.

`LuaContext::initialize` runs the layers in order in one `lua_State`. Before running a patch layer, `installChunkEnvWithBase` replaces the environment of its chunk with a table that holds the table returned by the previous layer as `Base` and falls back to `_G` through `__index`. On Lua 5.1 and LuaJIT this uses `setfenv`; on Lua 5.2+ it replaces the `_ENV` upvalue of the chunk. All `LUA_VERSION_NUM` checks for this live at the top of `LuaContext.cpp`.

A layer that fails to compile, fails to run, or does not return a table is logged via `logMod->error` and skipped; the next layer is stacked over the last successful one. If the base layer fails, `scriptTable` stays null and calls to the script do nothing.

## Documenting the public scripting API

Every binding carries its descriptions at the registration site, and every proxy declares a `luaDescription` for its class. The binding description is a mandatory argument. By convention every binding also has a non-empty name, every parameter a non-empty name, binding names are unique per type and parameter names are unique per binding. Parameter and return descriptions are optional; a void return has no return description. Descriptions follow [`docs/AGENTS.md`](../AGENTS.md).

Running

```sh
./vcmiserver --export-lua-docs <output-dir>
```

writes the reference into the given directory. The reference in [`docs/modders/Lua_Reference/`](../modders/Lua_Reference/API.md) is generated this way and is never edited by hand. The exporter (`api/DocExport.cpp`) does not need an initialized `GameLibrary`: `serverapp/EntryPoint.cpp` constructs `scripting::LuaModule` directly and calls `exportDocs`. It writes:

- `API.md` — index linking to every class and enum page.
- `<ClassName>.md` — one page per Lua-visible class: H1 is the class name, H3 is each method or field. A method block shows the description, then a list of parameters (omitted when there are none) and the returned value (omitted for void). Custom types in parameter and return positions link to their own page.
- `<EnumName>.md` — one page per enum group: H1 is the enum name, followed by a table of keys, values and descriptions. `Enums.md` links to every group.
- `api.lua` — [Lua Language Server](https://luals.github.io/wiki/definition-files/) stub: `---@meta` header, `---@class` per type, per-method `---@param`/`---@return` annotations. Drop into a luals `Lua.workspace.library` path to get autocomplete in modders' editors.

The generated pages use no H2, so that other tooling can place a class page under its own H2 heading.

Doc export calls `collectDocs` for every registered type, so it also reports any type missing from `Registry::Registry()`.

## Exposing a class to Lua scripts

1. **Choose a lifetime model** and inherit the C++ class from exactly one API tag from `include/vcmi/scripting/ApiTags.h`:
   - `scripting::ApiSerializable` — serialize as a Lua table (POD, no proxy and no methods); implement `serializeScript(auto & s)` and register with `registerSerializable<T>()`
   - `scripting::ApiCopyable` — copy into Lua userdata; for small value types without inheritance. Prefer const methods: mutators change only the copy
   - `scripting::ApiRawPointer` — pass raw pointer; for long-lived singletons or interfaces. Lifetime is owned by C++, and the pointer dangles if a script keeps it after the object is destroyed
   - `scripting::ApiSharedPointer` — pass `shared_ptr`; for short-lived objects or interfaces with shared ownership

2. **Create a proxy class** `XxxProxy` in the appropriate `luascript/api/` subdirectory, in namespace `scripting::api`. The proxy inherits from the matching wrapper template and declares `luaName`, `luaDescription` and `registerMethods`:

   ```cpp
   class CreatureProxy : public RawPointerWrapper<const Creature, CreatureProxy>
   {
   public:
       static constexpr std::string_view luaName = "Creature";
       static constexpr std::string_view luaDescription = "A creature type from the database.";

       static void registerMethods(MethodRegistrar & R);

       static std::string getNameTextID(const Creature & creature, int amount);
   };
   ```

3. **Implement `registerMethods`** with the `MethodRegistrar` helpers:

   ```cpp
   void CreatureProxy::registerMethods(MethodRegistrar & R)
   {
       EntityBindings<Creature>::registerMethods(R);

       // no parameters: pass {} as LuaReturn
       R.method<&Creature::isDoubleWide>("isDoubleWide", {},
           "True if the creature occupies two hexes on the battlefield.");

       // method declared on an untagged base class
       R.method<&Creature::getMaxHealth, Creature>("getMaxHealth", {},
           "Returns the base maximum hit points of a single creature of this type.");

       // with parameters
       R.method<&Creature::getRecruitCost>("getRecruitCost",
           {{"resourceID", "JSON key of the resource whose cost is being queried."}}, {},
           "Returns the recruitment cost in the specified resource.");

       // static proxy helper; the first C++ argument is self and is not listed
       R.function<&CreatureProxy::getNameTextID>("getNameTextID",
           {{"amount", "Stack size; 1 returns the singular form, anything else returns the plural."}}, {},
           "Selects either the singular or plural text ID for the requested amount.");
   }
   ```

4. **Register the proxy** in `Registry::Registry()` in `api/Registry.cpp` with `registerPrivate<XxxProxy>()`. If a binding signature mentions a C++ type that has no proxy, add `registerLuaName<TheType>("LuaName")` and include the header of that type, since `typeid` requires a complete type.

5. **Add the files** to `lualib_SRCS` and `lualib_HDRS` in `luascript/CMakeLists.txt`.

6. **Expose instances** by returning the object from an existing API call or by passing it as an argument when invoking a script callback.

7. **Regenerate** [`docs/modders/Lua_Reference/`](../modders/Lua_Reference/API.md) with `vcmiserver --export-lua-docs`.

API headers do not use `using namespace` and use fully qualified names. All proxies live directly in `scripting::api`, without nested namespaces.

## Data flow: a spell effect call

```text
Engine calls LuaSpellEffect::apply(server, mechanics, target)
  → resolveScript(mechanics) → LuaScriptPool::getContext(script) → LuaContext of the calling thread
  → LuaContext::callMethod<void>("apply", parameters, server, mechanics, target)
      → LuaStack: push function from scriptTable
      → LuaStack: build self = {params...} with __index = scriptTable
      → LuaStack: push server (ServerCb userdata), mechanics (SpellMechanics userdata), target (...)
      → lua_pcall(L, argc, 1, 0)
      → Lua script: function MyEffect:apply(server, mechanics, target) ... end
      → LuaStack: restoreInitialTop
```
