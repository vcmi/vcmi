# Lua Scripting System

This page describes internal working of Lua scripting module. For usage of public API see modding documentation

## TODO

- (MAJOR) switch from using global to `return table` pattern when loading scripts. This would allow implementing inheritance / extension of existing scripts
- (MAJOR) expand API to make scripts actually usable for modding. [ERM docs](https://azethmeron.github.io/) can serve as reference as to what should be accessible from Lua.
- (MAJOR) expand usage of scripts:
  - convert HotA map scripts into Lua form
  - convert HotA (and possibly - H3) Seer Huts into scripts
  - Move all spell effects to Lua form
  - Implement support for scriptable map objects
  - Move damage calculator, or at least - damage formula to Lua, based on [existing PR](https://github.com/vcmi/vcmi/pull/5135)
  - Move map movement point limit calculation to Lua
- Document public scripting API. Decide approach on how to handle it:
  - Use .md form and place them as part of our docs, accessible from website
  - Use [Lua Language Server format](https://luals.github.io/wiki/definition-files/) to make docs accessible from IDE
  - Both .md and Lua Language Server
  - Document everything in code and make exporter to both .md and Lua Language Server
- Currently C++ bindings rely on undefined behavior, which previously was causing issues on Android. We need to remove `VCMI_REGISTER_CORE_SCRIPT_API` and `VCMI_REGISTER_CORE_SCRIPT_API` macro and perform these actions explicitly on initialization of scripting
- Review existing API and ensure that it follows rules described here
  - replace `getAnyUnitIf` method with `getUnitsIf` that returns array
- Document C++ classes
- Review class names and rename them where necessary. Same for file names.
- Remove usage of numeric identifiers from script. In cases where entity does not exists such as `PlayerColor`, replace them with copyable API class
- Add error handling instead of returning nil values whenever something goes wrong. Lua already provides `luaL_error` for such cases. Make sure that Lua stack size is correct whenever error occurs
  - For API calls, ensure that all use some wrapper, such as `LuaCallWrapper` and not a direct function call
- Add "preprocess" or "initialize" function to initialize parameters (e.g. load string ID and resolve it to Creature type)
- Consider config validation as part of the script. Options:
  - external json schema support
  - implement "validate" function that performs all checks manually
  - have Lua generate json schema?
- ensure that there is debug logging available to scripts:
  - information logging
  - assertions
  - error messages
- implement comparison operator of exposed API classes by auto-implementing `__eq` Lua field for all exported classes
- consider wrapping Lua userdata into std::any for better type safety
- decide how to handle MetaString in Lua API. Make it Lua serializeable?

## General rules

Scripts must be constant and should not generate any side effects.

Exception are scripts that are executed as result of netpack apply (such scripts should be marked as such)

Global state of a Lua script must never change - script should not make assumptions on how many times it was run or in what order were functions called

## Naming rules

- Method names are in camelCase
- Method names must be verbs: `getFoo`, `isFoo`, `setFoo`, `run`, `update`
- Library classes, such as Creature must be passed as pointer like `const Creature *`, not as identifier like `CreatureID`
- If you need to expose identifier, prefer exposing its string form, like one provided via `getJsonKey`

## Exposing class to Lua script

1. Decide how this class should be passed into Lua
   - As Lua table (POD type): inherit class from `scripting::ApiSerializable` and implement `void serializeScript(Serializer & s)` method. Skip all subsequent steps. This approach is preferred for POD types
   - As copy: inherit class from `scripting::ApiCopyable`. This approach is preferred for small classes that don't use inheritance to avoid lifetime management
   - As raw pointer: inherit class from `scripting::ApiRawPointer`. Preferred for classes that are guaranteed to exists longer than scripts or for interfaces
   - As shared pointer: inherit class from `scripting::ApiSharedPointer`. Preferred for short-lived classes or for interfaces
2. Create class named XXXProxy and place it into one of `luascript/api` subdirectories
3. Provide all methods that needs to be exposed to a script in `REGISTER_CUSTOM` field:
   - if method can be used as it, use `LuaMethodWrapper` or `LuaSharedMethodWrapper` adapters
   - if method needs a simple adaptor, like provide a fixed parameter to a method, use `LuaFunctionWrapper` and implement static method with adapted signature in `XXXProxy` class
   - if method needs a compex adaptor, like to allow optional parameters, implement static method `int doXXX(lua_State * L)` and use it directly. WARNING: Should be avoided
4. Allow access to this class in script by returning it via API call or by passing it into script callback

## Classes

#### LuaModule

Global class responsible for loading Lua scripts on startup. Part of `ScriptingHandler`.

#### LuaScriptInstance

Instance of loaded script content (e.g. script source code). One is created for each loaded script. Persists between map restarts. Owned by `LuaModule`

#### LuaScriptPool

Owned by CGameState. Contains runnable instances of all scripts

#### LuaContext

Manages a single script. Game maintains one LuaContext for each loaded script. Each script has its own independent LuaState. Does not persists between map restarts. Owned by `LuaScriptPool`

#### CopyableWrapper, RawPointerWrapper and SharedPointerWrapper

Wrapper to expose C++ class to Lua in form of table with metadata

#### LuaFunctionWrapper and LuaMethodWrapper

Wrappers to adapt C++ method call into Lua callback signature `int function(lua_State * L)`

#### LuaStack

Helper to manage Lua state. Provides helper functions to:

- push variables onto Lua stack
- query variables located as specific position of the stack
- adjust stack size, including automatic restoration of stack size to its initial size

#### LuaReference

TODO

#### LuaSpellEffect

Adapter to provide spell effect implementation via Lua
