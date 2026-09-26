# Lua Scripting Guidelines

Lua scripting host. Built as a static library `vcmiLua` (see `CMakeLists.txt`) that is aggregated into the `vcmi` shared library by `libFacade`. `GameLibrary::initializeLibrary` constructs `scripting::LuaModule` directly; the host is no longer loaded as a runtime plugin. Links against either `luajit::luajit`, `lua::lua`, or `Lua::Lua` depending on what Conan/the system provides.

## File layout

- **Infrastructure (this directory):**
  - `LuaModule.{h,cpp}` — scripting `Service` entry point; registers the Lua spell-effect factory, exposes the `Pool` factory, implements `Service::exportDocs`. Instantiated directly from `GameLibrary::initializeLibrary`.
  - `LuaScriptPool.{h,cpp}` / `LuaScriptInstance.{h,cpp}` — one Lua state per script **per thread**; pool owns the instances.
  - `LuaContext.{h,cpp}` — sets up a `lua_State`, installs the registry-driven type table.
  - `LuaStack.{h,cpp}` — typed stack wrapper. Use this, do not call `lua_push*`/`lua_to*` directly in new code. `LuaStack::get` throws `LuaApiException` on type mismatch; the wrappers catch it.
  - `LuaWrapper.h` — `RawPointerWrapper` / `SharedPointerWrapper` / `CopyableWrapper`. Pick one based on the tag the bound C++ class inherits. Each wrapper drives the per-type metatable build through `Proxy::registerMethods(LuaRegistrar)`.
  - `LuaCallWrapper.h` — `LuaMethodWrapper`, `LuaFunctionWrapper`, `LuaCallWrapper`. These adapt C++ callables to `int(lua_State*)` and translate exceptions to `lua_error`.
  - `LuaSpellEffect.{h,cpp}` — bridges the `spells::effects` system to Lua spell scripts.
  - `LuaReference.{h,cpp}` — RAII handle to a value in the Lua registry.

- **`api/` — proxy classes** (one per exposed C++ type), grouped by domain: `adventure/`, `battle/`, `callback/`, `library/`, `spells/`, plus standalone `Enums` and `LuaMetaString`. `Registry.{h,cpp}` is the central type table.

- **`api/` — binding-group classes (no proxy of their own)** for methods that are shared across multiple leaf proxies. Each is a class template on the leaf type:
  - `library/EntityBindings.h` — `getJsonKey` (shared by `Artifact`, `Creature`, `Faction`, `HeroClass`, `HeroType`, `Skill`, `Spell`).
  - `library/BonusBearerBindings.h` — the filter-predicate `getBonuses` cfunction (shared by `HeroInstance`, `Unit`).

- **`api/` — binding visitor infrastructure:**
  - `MethodRegistrar.h` — visitor interface plus the `LuaParam` / `LuaCParam` / `LuaReturn` / `LuaCReturn` host-supplied descriptors and the `DocEntry` / `ParamDoc` / `ReturnDoc` records emitted to concrete sinks. Each proxy implements `static void registerMethods(MethodRegistrar &)` and calls the templated helpers below.
  - `LuaRegistrar.h` — concrete `MethodRegistrar` that pushes name → C-function pairs directly onto a Lua table; ignores everything else in the `DocEntry`.
  - `DocRegistrar.h` — concrete `MethodRegistrar` that collects fully-structured `DocEntry` records (description, typed `ParamDoc`s, `ReturnDoc`); used by the docs exporter.
  - `SignatureOf.h` — `paramTypesOfMethod` / `returnTypeOfMethod` / `paramTypesOfFunction` / `returnTypeOfFunction` derive Lua-facing type strings from the existing C++ method-pointer traits. Type names are resolved by `luaTypeName<T>()`: primitives / identifiers / `std::vector` / `std::optional` / `std::shared_ptr` / `T*` are handled by `if constexpr` at compile time; everything else goes through `Registry::lookupLuaName(typeid(T))`, which throws `std::runtime_error` on miss. The registry is populated by proxy registrations (which contribute `typeid(Proxy::ObjectType) → Proxy::luaName`) and by explicit alias calls in `Registry::Registry()`.
  - `DocExport.{h,cpp}` — Markdown + Lua Language Server emitters driven from `Registry::getAllTypes()`. Emits one Markdown file per class and per enum group into the output directory, plus an `API.md` index and a single `api.lua` LuaLS stub.

## How a binding is shaped

1. **Tag the C++ class** (in `include/vcmi/scripting/ApiTags.h`) — every scripting-visible class inherits from exactly one of:
   - `ApiRawPointer<T>` — lifetime owned by C++; scripts cannot construct. *Risk: dangling pointer if the script outlives the object.*
   - `ApiSharedPointer<T>` — managed by Lua GC; scripts can default-construct.
   - `ApiCopyable<T>` — copied into Lua userdata. Prefer const methods; mutators only affect the copy.
   - `ApiSerializable<T>` — POD round-tripped as a Lua table via `serializeScript(Handler)`. No methods allowed.

   The wrapper templates `static_assert` that exactly one tag is present — a compile error here means you picked the wrong wrapper or forgot the tag.

2. **Write `<Category>/Xxx.h`** with an `XxxProxy` inheriting the matching wrapper template. The proxy needs a `luaName` constant and a `registerMethods` declaration. Registration in `Registry.cpp` auto-binds `typeid(Proxy::ObjectType) → Proxy::luaName` for signature derivation — no separate ADL overload is required.

   ```cpp
   class CreatureProxy : public RawPointerWrapper<const Creature, CreatureProxy>
   {
   public:
       static constexpr std::string_view luaName = "Creature";

       static void registerMethods(MethodRegistrar & R);

       static std::string getNameTextID(const Creature & creature, int amount);
   };
   ```

3. **Implement `registerMethods` in `<Category>/Xxx.cpp`** by calling the templated helpers on the visitor. Each call needs a name, a `LuaParam` array (named — types come from C++ traits), a `LuaReturn` (description only — type comes from traits), and a binding description. `cfunction` uses `LuaCParam` / `LuaCReturn` instead, where the caller spells out the Lua-facing types since there are no traits to mine. The `method<>` / `function<>` templates `static_assert` that the array length matches the C++ arg count (minus self for `function<>`).

   ```cpp
   void CreatureProxy::registerMethods(MethodRegistrar & R)
   {
       EntityBindings<Creature>::registerMethods(R);          // shared base

       // Zero-arg overload — pass {} for LuaReturn when no return description is needed.
       R.method<&Creature::isDoubleWide>("isDoubleWide", {},
           "True if the creature occupies two hexes on the battlefield.");

       // ExplicitObjectType (second template arg) is required only when the method lives on
       // an untagged base class.
       R.method<&Creature::getMaxHealth, Creature>("getMaxHealth", {},
           "Returns the base maximum hit points of a single creature of this type.");

       // With parameters: array of {name, description?} pairs; description on the param is optional.
       R.method<&Creature::getRecruitCost>("getRecruitCost",
           {{"resourceID", "JSON key of the resource whose cost is being queried."}}, {},
           "Returns the recruitment cost in the specified resource.");

       // Static proxy helper — first C++ arg is self and dropped; remaining args are documented.
       R.function<&CreatureProxy::getNameTextID>("getNameTextID",
           {{"amount", "Stack size; 1 returns the singular form, anything else returns the plural."}}, {},
           "Selects either the singular or plural text ID for the requested amount.");

       // Raw `int(lua_State*)` cfunction — caller supplies both names and types per param.
       R.cfunction<&CreatureProxy::someRawHandler>("someName",
           {{"arg", "type", "Per-param description."}},
           {"retType", "Description of the return value."},
           "Binding description.");
   }
   ```

4. **Register the proxy** in `api/Registry.cpp` with the no-arg `registerPrivate<XxxProxy>()`. The Lua-facing key comes from `XxxProxy::luaName` — do not pass a string here.

5. **Add files to `CMakeLists.txt`** — `lualib_SRCS` and `lualib_HDRS`. The list is alphabetical within each subgroup; keep it that way.

### When a proxy method references a C++ type that has no proxy of its own

(Abstract interfaces, POD descriptors, named enums.) Add an explicit alias call at the bottom of `Registry::Registry()`:

```cpp
registerLuaName<TheType>("LuaName");
```

If the alias type isn't already pulled in via the proxy headers `Registry.cpp` includes, add an `#include` for its definition (`typeid` requires a complete type). Existing examples:

- `CGObjectInstance` → `"MapObject"`
- `battle::UnitInfo` → `"UnitInfo"`
- `JsonNode` → `"any"` (open-ended Lua value funneled through `JsonUtils`)
- `CBattleInfoCallback` → `"Battle"` (derived class sharing the `IBattleInfoCallback` proxy)
- The named enums in `Enums::serializeScript` (`HealLevel`, `BattleSide`, …)

Forgetting to register an alias is no longer silent: doc export (`vcmiserver --export-lua-docs`) runs `collectDocs` for every type and surfaces the missing-type exception with the demangled C++ name.

### When you find duplication across leaf proxies

Pull the shared part into a leaf-templated binding-group class in `api/library/` (next to the existing `EntityBindings`, `BonusProviderBindings`, `BonusBearerBindings`). Pattern:

```cpp
template<class Leaf>
class FooBindings
{
public:
    static void registerMethods(MethodRegistrar & R)
    {
        R.template method<&FooBase::someMethod, Leaf>("someName", "...");
        // cfunction helpers may live inside the class as static privates if they need Leaf.
    }
};
```

The leaf proxies call `FooBindings<TheirLeaf>::registerMethods(R)` from their own `registerMethods`. Don't extract a group just for naming consistency — the duplication only pays for itself when the *C++ method pointers* are shared (i.e. methods live on a common base). When the bindings only share Lua-side names but invoke different C++ methods (`UnitProxy` vs `LuaUnitStateProxy` stat accessors), keep them in their leaf and let the duplication stand.

## Conventions enforced in recent refactors

- **No `using namespace`** in API headers. Recent commits (`Removed using namespace in Lua API headers`, `Remove deeply nested namespaces in scripting::api`) intentionally stripped this. Use fully qualified names.
- **Single namespace `scripting::api`** for proxies — do not reintroduce deeper nesting like `scripting::api::library`.
- **Throw, don't `lua_error`** in your method bodies. The wrappers catch `std::exception` and translate. Calling `lua_error` directly bypasses C++ destructors (long-jump) and leaks.
- **Stack discipline** — when you take a `LuaStack S(L)`, every code path must either consume what it pushed or call `S.restoreInitialTop()` before exit. Don't leave half-constructed values on the stack across a throw. Prefer `restoreInitialTop` over `clear` outside of a cfunction body: `clear` wipes the whole frame, which corrupts the caller when a script re-enters `callMethod`.
- **Pool scripts must be stateless.** `LuaScriptPool` gives every thread its own `lua_State` per script, with no locking, so the AI can evaluate spells in parallel. A script that accumulates state in a global or in its own table diverges per thread. Configuration arrives as the per-call `self` table; that is the only state a script may rely on. `LuaMapEventDispatcher` is exempt — it owns its context directly rather than going through the pool, which is what lets it park coroutines.
- **Identifiers in Lua-visible APIs** — recent work (`Reduce usage of unitID in Lua`) is moving away from exposing numeric IDs; prefer string/JSON-key accessors (`getJsonKey`, `getNameTextID`) over `getIndex`-style returns when adding new bindings.
- **Binding-level descriptions are mandatory.** The trailing `description` argument to `method` / `function` / `cfunction` has no default. By convention every binding also has a non-empty name and description, every param a non-empty name and type, names are unique per type, and param names are unique per binding. Per-param and per-return descriptions stay optional; don't attach a return *description* to a void return. Descriptions are exported to `docs/modders/Lua_Reference/`; write them following [`docs/AGENTS.md`](../docs/AGENTS.md).

## Documentation export

`vcmiserver --export-lua-docs <dir>` writes one Markdown file per Lua-visible type (classes and enum groups alike) into the given directory, plus:

- **`API.md`** — index linking to every class and enum page.
- **`<ClassName>.md`** — H1 = class name; H3 = each method / field. Per-method blocks show the description as a paragraph, then a bullet list of parameters (omitted when the binding takes none) and returned values (omitted for void returns). Custom types in parameter / return positions render as Markdown links to their own page.
- **`<EnumName>.md`** — H1 = enum name; the key/value/description table directly underneath. The parent type that bundles enums into LuaLS (`Enums`) gets its own page that links out to each group.
- **`api.lua`** — single Lua Language Server stub: `---@meta`, `---@class <Name>` per type, per-method `---@param`/`---@return` annotations with host-supplied descriptions. Drop it into a luals `Lua.workspace.library` path to get autocomplete in modders' editors.

H2 is intentionally unused in the auto-generated Markdown so downstream tooling can wrap a class page under its own H2 section without colliding with the auto-generated headings.

The emitter lives in `api/DocExport.{h,cpp}`. It does not need the full `GameLibrary` initialised — the entry point in `serverapp/EntryPoint.cpp` instantiates `scripting::LuaModule` directly and calls `scriptHandler->exportDocs(outPath)`.

If a binding's signature mentions a C++ type that has no proxy and no alias in `Registry::Registry()`, the type-name lookup throws `std::runtime_error` during signature derivation. The exception propagates out of `paramTypesOfMethod` and crashes both doc export and Lua context creation, printing the demangled C++ name so you know exactly what to add to the registry.

## Spell effects

Lua spell effects register through `LuaSpellEffectFactory` (created in `LuaModule::installScripting`, scripts attached per pool in `createPoolInstance`). When adding a new spell-effect binding, the Mechanics/Problem proxies in `api/spells/` are the integration surface — not direct manipulation of the `spells::effects` interfaces from the script side.

## Patched scripts (chained layers)

A spell-effect entry in `spellEffects.json` may declare a `patches` array. Each patch is a script path; the mod that contributed the patch is taken from the JSON node's `modScope`, so multiple mods can append to the same effect's `patches` without knowing about each other.

`LuaScriptInstance` holds the layers as `vector<Layer>` — layer 0 is the base, layers 1..N are patches in declared order. `LuaContext::initialize` runs them sequentially in a single Lua state; for each patch chunk it installs an environment that exposes the previous layer's returned table as a global named `Base` (with `__index = _G` so other globals still resolve). The final layer's returned table becomes `scriptTable`; dispatch through `__index` walks down the chain naturally.

A patch file looks like a normal script except `Base` is injected by the host instead of imported via `require`:

```lua
local Script = setmetatable({}, {__index = Base})
Script.__index = Script

function Script:apply(mechanics, server, target)
    -- tweak parameters
    Base.apply(self, mechanics, server, target)
    -- post-effect tweaks
end

return Script
```

**Delegation footgun:** call up the chain with `Base.method(self, ...)` (dot, pass `self`), not `self:method(...)`. The latter re-dispatches through `__index` to the patch itself and infinite-loops.

Layer load/compile/run failures are logged via `logMod->error` and skip the offending layer — the chain continues with the previously successful head. The base layer failing leaves `scriptTable` null, which `callMethod` treats as a no-op.

Lua-version compat for env injection lives in one spot at the top of `LuaContext.cpp` (`installChunkEnvWithBase`): 5.1/LuaJIT uses `setfenv` against the chunk; 5.2+ uses `lua_setupvalue` on the chunk's `_ENV` slot. Don't sprinkle `LUA_VERSION_NUM` checks elsewhere.

The same chaining mechanism is intended to be reused by future handlers: read `patches` from JSON (using each array entry's `getModScope()` for scope and `String()` for path), pass `vector<pair<scope,path>>` to a script factory whose `initialize` keys the cache by the handler-side unique id (e.g. `"core:damage"` for spell effects).

