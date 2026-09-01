# Script Types

Every script, whatever it does, is declared the same way - in the `scripts` section of a mod. What a script *is* comes from `implements`, which decides the interface the game calls it through and so which functions the script has to define:

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
- [Damage Calculator Script](Damage_Calculator_Script.md) - `"implements" : "damageCalculator"`, what an attack is worth. Unlike the other two there is one of these for the whole game, and a mod changes the rules by patching it rather than by declaring its own

## What a script can reach

Three globals are there whatever the script is:

- `LIBRARY` - the game's content, looked up by identifier: creatures, heroes, factions, spells. See [Services](../Lua_Reference/Services.md)
- `ENUM` - every enumeration the engine exports. See [Enums](../Lua_Reference/Enums.md)
- `GAME` - the ongoing game session. See [Game](../Lua_Reference/Game.md)

What each type of script is handed on top of those - a battle, a unit, a server to apply changes through - is described on its own page above.

[**Lua API Reference**](../Lua_Reference/API.md) lists every class and enumeration the engine exposes, one page each. It is generated from the bindings themselves, so it is the one place that cannot fall behind them. `api.lua` next to it is a [Lua Language Server](https://luals.github.io/) stub - point `Lua.workspace.library` at it for completion and type checks while writing a script.

VCMI also supports a subset of the Lua standard library; see [Lua Standard Library](Standard_Library.md) for what is in it.

## Shared format

Fields every script declares, whatever its type:

- `implements` - what this script is, see above
- `script` - path to the source, relative to the `SCRIPTS/` directory of the mod, without the extension. Sources are kept in a directory per type, so `spells/damage` or `combat/lifeDrain`
- `patches` - other sources stacked over the base one, in the order given, so that a mod can change a script it does not own instead of replacing it. Declare it as an empty list when the script has none, so that other mods have a place to append to
- `schema` - a json schema validating the parameters every user of this script passes to it. Errors are reported when the game loads, naming whoever passed the bad parameters. Declare an empty, closed one when the script takes no parameters, rather than leaving it out

A `damageCalculator` script declares nothing beyond the shared fields - nothing runs alongside it and nothing shows it to the player.

Fields a `combatEvent` script declares on top of those:

- `description` - text shown to the player for an ability that runs this script. `${val}` is replaced with the value of the bonus and `${parameterName}` with a parameter the bonus passed. Every scripted ability shares one bonus type, so this is the only thing that tells one from another in the creature window - a script that is deliberately invisible declares it empty
- `priority` - the order in which scripts reacting to the same event run, from lowest to highest. Required rather than defaulted, because which of two abilities acts first is part of what each of them does. `0` is the usual answer

Fields a script may declare:

- `stringRegistrations` - names of parameters that hold text shown to the player. Such a parameter is registered for translation instead of being used as-is. A value starting with `@` is taken to be a reference to a string some other entity already registered

Scripts of every type share one namespace, so a script is referred to simply by its name - or by `<modName>:<name>` when the reference has to name the mod that provides it.

## Parameters

Whatever configures a single use of a script is its parameters, and they reach the script as fields of `self`. Where they are written depends on the type - a spell effect is configured by the spell that uses it, a combat event script by the bonus that runs it - but `schema` and `stringRegistrations` apply to both in the same way:

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

A script that takes no parameters at all still declares a schema - an empty, closed one, so that anything passed to it by mistake is reported rather than silently ignored:

```json
"schema" : { "properties" : {}, "additionalProperties" : false }
```

## Parameters that name something

A parameter holding the identifier of a creature, a spell or any other entity says so with `entity`, next to its type:

```json
"schema" : {
    "required" : [ "creature" ],
    "properties" : {
        "creature" : { "type" : "string", "entity" : "creature", "description" : "creature to summon as guardian" }
    },
    "additionalProperties" : false
}
```

Two things follow from it. The identifier is resolved when the mod loads, whatever kind of entity it names, so a typo is reported by name instead of quietly turning into an ability that does nothing. And the `description` of the script prints a creature or a spell by its own translated name where it writes `${parameterName}`, instead of the raw json key - as the kind of entity the parameter declares, which matters because one key can name a creature and a spell at once. A parameter that declares no `entity` is printed as written, so an ordinary string is never mistaken for an identifier.
