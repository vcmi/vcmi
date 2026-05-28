# Lua API Reference

## Available globals

VCMI provides following global fields, that are accessible from any script:

- `LIBRARY` - constant game data access, such as creatures, heroes, factions. See [services](#services).
- `ENUM` - container for all enumerations accessible for Lua scripting. See [enums](#enums).
- `GAME` - Allows access to state of the ongoing game session. See [game](#game).

Additionally, VCMI supports subset of Lua Standard Library:

- `print()` - emulates standard Lua function with the same name, available for use as logging. Printed text will be available in log file and in console output
- `table` - standard Lua table manipulation library
- `string` - standard Lua string manipulation library
- `math` - standard Lua math operations library
- ...as well as several other globals.

See [Lua Standard Library](Lua_Standard_Library.md) page for more details.

## Persistent data

These classes represent global, non-changeable entities

### enums

This class is used to export all enumerations from engine that can be accessed by mods. Currently it contains following enumerations:

- `HealLevel`
  - `heal`
  - `resurrect`
  - `overheal`
- `HealPower`
  - `oneBattle`
  - `permanent`

### services

Primary way to access all library entries. Available as `LIBRARY` global.

NOTE: all `get???ByName` methods require string identifier of an object, for example `grandElf` or `core:grandElf`, and not human-readable name such as `Grand Elf`

- `:getArtifactByName(string name)`
- `:getCreatureByName(string name)`
- `:getHeroClassByName(string name)`
- `:getHeroTypeByName(string name)`
- `:getSpellByName(string name)`
- `:getSecondarySkillByName(string name)`

### Artifact

### Creature

### Faction

### HeroClass

### HeroType

### Skill

### Spell

## game

These classes represent global gamestate. Read-only by scripts, can be changed indirectly via network packs

### EventBus

NOTE: inaccessible in current API

### EventSubscription

NOTE: inaccessible in current API

### Battle

### Game

### Player

### Server

## adventure

These classes represent objects present on adventure map. Read-only by scripts, can be changed indirectly via network packs

### ObjectInstance

### HeroInstance

### StackInstance

## battle

These classes represent objects present during combat. Read-only by scripts, can be changed indirectly via network packs

### Unit

- `:getMinDamage()`
- `:getMaxDamage()`
- `:getAttack()`
- `:getDefense()`
- `:isAlive()`
- `:isClone()`
- `:isSummoned()`
- `:getOwner()`
- `:getSlot()`
- `:getCreature()`

### SpellMechanics

- `:isPositive()`
- `:isNegative()`
- `:getEffectLevel()`
- `:getRangeLevel()`
- `:getEffectPower()`
- `:getEffectDuration()`
- `:getEffectValue()`
- `:getCasterColor()`
- `:getBattle()`

## netpacks

These classes are used for making changes to a gamestate, and are the only way to make things happen either on adventure map or in combat. General workflow is to create a pack by calling `new`, fill it with data and apply on server

### BattleLogMessage

- `.new()`
- `:addText(text)`
- `:toNetpackLight()`

### BattleStackMoved

- `.new()`
- `:addTileToMove(integer)`
- `:setUnitId(integer)`
- `:setDistance(integer)`
- `:setTeleporting(bool)`
- `:toNetpackLight()`

### BattleUnitsChanged

- `.new()`
- `:add(integer id, Json data, integer healthDelta)`
- `:update(integer id, Json data, integer healthDelta)`
- `:resetState()` (not implemented)
- `:remove()` (not implemented)
- `:toNetpackLight()`

### EntitiesChanged

- `.new()`
- `:update(integer metaIndex, integer index, Json data)`
- `:toNetpackLight()`

### InfoWindow

FIXME: move this to metastring

- `.new()`
- `:addReplacement(string text)`
- `:addReplacement(integer number)`
- `:addText(string text)`
- `:addText(integer number)`
- `:setPlayer(integer index)`
- `:toNetpackLight()`

### SetResources

- `.new()`
- `:getPlayer() -> integer`
- `:setPlayer(integer playerIndex)`
- `:getAmount() -> integer`
- `:setAmount(integer which, integer amount)`
- `:clear()`
- `:toNetpackLight()`

## texts

Classes for managing text translations

### MetaString
