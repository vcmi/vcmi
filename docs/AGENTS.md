# Documentation Guidelines

Documentation uses the same plain, technical English as code comments; see the rules under "Comments" in the root [`AGENTS.md`](../AGENTS.md).

Pages follow existing docs, such as [`modders/Bonus/Bonus_Types.md`](modders/Bonus/Bonus_Types.md) and [`modders/Entities_Format/`](modders/Entities_Format/): a short description, then a `-` list of fields, then a json example. Descriptions are one sentence where one sentence is enough.

Pages under [`modders/Lua_Reference/`](modders/Lua_Reference/) are generated from the binding descriptions in `luascript/api/`. Edit the C++ string and regenerate with `vcmiserver --export-lua-docs <dir>`; never edit the generated file.
