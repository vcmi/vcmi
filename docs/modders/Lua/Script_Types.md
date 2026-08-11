# Script Types

Every script, whatever it does, is declared the same way - in the `scripts` section of a mod. What
a script *is* comes from `implements`, which decides the interface the game calls it through and so
which functions the script has to define:

```json
"lifeDrain" : {
    "type" : "lua",
    "implements" : "combatEvent",
    "script" : "combat/lifeDrain",
    "description" : "{Life Drain}\nRestores health equal to ${val}% of damage dealt."
}
```

## Script types

- [Spell Effect Scripts](Spell_Effect_Scripts.md) - `"implements" : "spellEffect"`, an effect of a
  spell, such as the built-in `core:damage` or `core:summon`
- [Combat Event Scripts](Combat_Event_Scripts.md) - `"implements" : "combatEvent"`, a reaction to
  events happening to a unit in combat, such as Fire Shield or Death Stare

## Shared format

Fields every script may declare, whatever its type:

- `type` - the language the script is written in. Currently only `lua`
- `implements` - what this script is, see above
- `script` - path to the source, relative to the `SCRIPTS/` directory of the mod, without the
  extension. Sources are kept in a directory per type, so `spells/damage` or `combat/lifeDrain`
- `patches` - other sources stacked over the base one, in the order given, so that a mod can change
  a script it does not own instead of replacing it
- `schema` - a json schema validating the parameters every user of this script passes to it.
  Errors are reported when the game loads, naming whoever passed the bad parameters
- `stringRegistrations` - names of parameters that hold text shown to the player. Such a parameter
  is registered for translation instead of being used as-is. A value starting with `@` is taken to
  be a reference to a string some other entity already registered
- `description` - text shown to the player for an ability that runs this script. `${val}` is
  replaced with the value of the bonus and `${parameterName}` with a parameter the bonus passed
- `priority` - for `combatEvent`, the order in which scripts reacting to the same event run

Scripts of every type share one namespace, so a script is referred to simply by its name - or by
`<modName>:<name>` when the reference has to name the mod that provides it.

## Parameters

Whatever configures a single use of a script is its parameters, and they reach the script as fields
of `self`. Where they are written depends on the type - a spell effect is configured by the spell
that uses it, a combat event script by the bonus that runs it - but `schema` and
`stringRegistrations` apply to both in the same way:

```json
"deathStare" : {
    "type" : "lua",
    "implements" : "combatEvent",
    "script" : "combat/deathStare",
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

A script that takes no parameters at all should still declare an empty, closed schema, so that
anything passed to it by mistake is reported rather than silently ignored:

```json
"schema" : { "properties" : {}, "additionalProperties" : false }
```
