# Lua Scripting Agent Notes (`luascript`)

[`docs/developers/Lua_Scripting_System.md`](../docs/developers/Lua_Scripting_System.md) describes the architecture, the binding API, and the steps to expose a class. Read it before changing this directory. Scripts themselves live in `config/` and `scripts/`, and their modder documentation in [`docs/modders/Lua/`](../docs/modders/Lua/Script_Types.md).

## Rules

- Follow "Exposing a class to Lua scripts" in the developer doc step by step. Pass no string to `registerPrivate`; the name comes from `luaName`.
- Use `LuaStack` instead of `lua_push*` / `lua_to*`. Return every code path to the initial top with `restoreInitialTop()`; use `clear()` only inside a cfunction body.
- Report errors from bound methods by throwing a C++ exception. Never call `lua_error` from a bound method: it long-jumps past C++ destructors.
- Keep `LUA_VERSION_NUM` checks at the top of `LuaContext.cpp`.
- Do not add state to pool scripts. Configuration comes from `self`; `LuaMapEventDispatcher` is the only exception.
- Extract a binding group into `api/library/` only when the leaf proxies share the C++ method pointers.
- Write binding descriptions and `luaDescription` following [`docs/AGENTS.md`](../docs/AGENTS.md).
- In Lua patches, call the previous layer with `Base.method(self, ...)`. `self:method(...)` dispatches back into the patch and recurses forever.

## Verification

- After changing a binding, run `vcmiserver --export-lua-docs docs/modders/Lua_Reference` and include the regenerated pages. The export also fails on a C++ type that is missing from `Registry::Registry()`, reporting its demangled name.
- Never edit `docs/modders/Lua_Reference/` by hand.
