# Script Types

Every script is declared in the `scripts` section of a mod. `implements` selects the script interface and its required functions:

```json
"lifeDrain" : {
    "implements" : "combatEvent",
    "script" : "combat/lifeDrain",
    "patches" : [ ],
    "priority" : 0,
    "schema" : { "properties" : {}, "additionalProperties" : false },
    "description" : "{Life Drain}\nRestores health equal to ${val}% of damage dealt."
}
```

## Script types

- [Spell Effect Scripts](Spell_Effect_Scripts.md) - `"implements" : "spellEffect"`, an effect of a spell, such as the built-in `core:damage` or `core:summon`
- [Combat Event Scripts](Combat_Event_Scripts.md) - `"implements" : "combatEvent"`, a reaction to events happening to a unit in combat, such as Fire Shield or Death Stare
- [Damage Calculator Script](Damage_Calculator_Script.md) - `"implements" : "damageCalculator"`, attack damage calculation. One instance handles the game, and mods change its rules with patches

## Script globals

Three globals are available to every script:

- `LIBRARY` - the game's content, looked up by identifier: creatures, heroes, factions, spells. See [Services](../Lua_Reference/Services.md)
- `ENUM` - every enumeration the engine exports. See [Enums](../Lua_Reference/Enums.md)
- `GAME` - the ongoing game session. See [Game](../Lua_Reference/Game.md)

Each script type may also receive a battle, a unit or a server callback, as described on its page.

[**Lua API Reference**](../Lua_Reference/API.md) lists every exported class and enumeration. It is generated from binding descriptions in `luascript/api/`; regenerate it after changing those descriptions. `api.lua` is a [Lua Language Server](https://luals.github.io/) stub. Add it to `Lua.workspace.library` for completion and type checks.

VCMI also supports a subset of the Lua standard library; see [Lua Standard Library](Standard_Library.md) for what is in it.

## Shared format

Fields every script declares, whatever its type:

- `implements` - script interface, as listed above
- `script` - path to the source, relative to the `SCRIPTS/` directory of the mod, without the extension. Sources are kept in a directory per type, so `spells/damage` or `combat/lifeDrain`
- `patches` - sources applied over the base script in list order. Declare an empty list to allow other mods to append patches
- `schema` - json schema for script parameters. Validation errors identify the entity with invalid parameters. Use an empty, closed schema when the script takes no parameters

A `damageCalculator` script has no additional fields and is not shown to the player.

Fields a `combatEvent` script declares on top of those:

- `description` - text shown for the ability. `${val}` is replaced with the bonus value and `${parameterName}` with a script parameter. This field distinguishes scripted abilities in the creature window. Use an empty string for a hidden script
- `priority` - handler execution order, from lowest to highest. The field is required because ordering affects behavior. `0` is the usual value

Fields a script may declare:

- `stringRegistrations` - names of parameters that hold text shown to the player. Such a parameter is registered for translation instead of being used as-is. A value starting with `@` is treated as a reference to a string registered by another entity

All script types share one namespace. Use the local name within the same scope or `<modName>:<name>` for an explicit mod scope.

## Parameters

Parameters configure one script instance and are available as fields of `self`. Spell parameters are declared by the spell; combat event parameters are declared by the bonus. `schema` and `stringRegistrations` apply to both:

```json
"deathStare" : {
    "implements" : "combatEvent",
    "script" : "combat/deathStare",
    "patches" : [ ],
    "priority" : 100,
    "schema" : {
        "properties" : {
            "situation" : {
                "type" : "string",
                "enum" : [ "melee", "ranged", "commander" ]
            },
            "spell" : { "type" : "string" }
        },
        "additionalProperties" : false
    }
}
```

A script that takes no parameters still declares a schema - an empty, closed one, so that anything passed to it by mistake is reported instead of silently ignored:

```json
"schema" : { "properties" : {}, "additionalProperties" : false }
```

## Entity parameters

A parameter holding the identifier of a creature, a spell or any other entity declares it with `entity`, next to its type:

```json
"schema" : {
    "required" : [ "creature" ],
    "properties" : {
        "creature" : { "type" : "string", "entity" : "creature", "description" : "creature to summon as guardian" }
    },
    "additionalProperties" : false
}
```

`entity` enables load-time identifier validation and translated names in `description`. Resolution uses the declared entity type because one key may identify both a creature and a spell. Parameters without `entity` are displayed as literal strings.
