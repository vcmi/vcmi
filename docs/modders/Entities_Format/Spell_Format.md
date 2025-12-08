# Spell Format

## Main format

```json
{
	"spellName":
	{	
		// Allowed values: "adventure", "combat", "ability"
		// adventure spells can only be cast by hero on adventure map
		// combat spells can be cast by hero or by creatures during combat
		// ability-type spells can not be rolled in town mage guild 
		// learned by hero and can only be used by creatures
		"type": "adventure",
		
		// Mandatory. Spell target type
		// "NO_TARGET" - instant cast no aiming (e.g. Armageddon)
		// "CREATURE" - target is unit (e.g. Resurrection)
		// "OBSTACLE" - target is obstacle (e.g. Remove Obstacle)
		// "LOCATION" - target is location (e.g. Fire Wall)
		"targetType":"NO_TARGET",

		// Localizable name of this spell
		"name": "Localizable name",
	
		// List of spell schools this spell belongs to. Require for spells other than abilities
		"school": {"air":true, "earth":true, "fire":true, "water":true},
	
		// Spell level, value in range 1-5, or 0 for abilities
		"level": 1,

		// Base power of the spell. To see how it affects spell, 
		// see description of corresponding battle effect(s)
		"power": 10,
	
		// Default chance for this spell to appear in Mage Guilds
		// Used only if chance for a faction is not set in gainChance field
		"defaultGainChance": 0, 
	
		// Chance for this spell to appear in Mage Guild of a specific faction
		// Symmetric property of "guildSpells" property in towns
		/// Identifier without modID specifier MUST exist in base game or in one of dependencies
		/// Identifier with explicit modID specifier will be silently skipped if corresponding mod is not loaded
		"gainChance":
		{
			"factionName" : 3,
			"modID:anotherFactionName" : 5
		},

		"animation":{<Animation format>},
			
		// List of spells that will be countered by this spell
		// If unit is affected by any spells from this list, 
		// then casting this spell will remove effect of countered spell
		"counters": {
			"spellID" : true, 
			...
		},

		// List of flags that describe this spell
		// positive - this spell is positive to target (buff) and can target allies
		// negative - this spell is negative to target (debuff) and can target enemies
		// indifferent - spell is neither positive, nor negative
		// damage - spell does damage (direct or indirect). 
		// If set, AI will avoid obstacles with such effect, and spellbook popup will also list damage of the spell
		// offensive - (Deprecated?) direct damage (implicitly sets damage and negative)
		// rising - (Deprecated?) rising spell (implicitly sets positive)
		// special - this spell can not be present in mage guild, or leared by hero, and can only be received explicitly, e.g. from bonus SPELL
		// nonMagical - this spell is not affected by Sorcery or magic resistance. School resistances (if any) apply.
		"flags" : {
			"positive": true,
		},
		
		// If true, then creature capable of casting this spell can cast this spell on itself
		// If false, then creature can only cast this spell on other units
		"canCastOnSelf" : false,
		
		// If true, then creature capable of casting this spell can cast this spell only on itself
		"canCastOnlyOnSelf" : false,

		// If true the creature will not skip the turn after casting a spell
		"canCastWithoutSkip": false,
		
		// If true, spell won't be available on a map without water
		"onlyOnWaterMap" : true,

		//TODO: DEPRECATED | optional| no default | flags structure of bonus names,any one of these bonus grants immunity. Negatable by the Orb.
		"immunity": {"BONUS_NAME":true, ...},
			
		//TODO: DEPRECATED | optional| no default | flags structure of bonus names
		//any one of these bonus grants immunity, cant be negated
		"absoluteImmunity": {"BONUS_NAME": true, ...},

		//TODO: DEPRECATED | optional| no default | flags structure of bonus names, presence of all bonuses required to be affected by. Negatable by the Orb.
		"limit": {"BONUS_NAME": true, ...},

		//TODO: DEPRECATED | optional| no default | flags structure of bonus names, presence of all bonuses required to be affected by. Cant be negated
		"absoluteLimit": {"BONUS_NAME": true, ...},
		
		//TODO: optional | default no limit no immunity
		//
		"targetCondition" {
			//at least one required to be affected
			"anyOf" : {
				//generic format
				"mod:metaClassName.typeName":"absolute",//"normal", null or empty ignored - use for overrides
			},
			//all required to be affected (like [absolute]limit)
			"allOf" : {
				//bonus type format
				"bonus.BONUS_TYPE":"absolute"//"normal" Short bonus type format
				"modId:bonus.bonusTypeName":"absolute"//"normal" Future bonus format for configurable bonuses
			},
			//at least one grants immunity	(like [absolute]immunity)
			"noneOf": {
				//some more examples
				"core:creature.imp":"absolute", //[to be in initial version] this creature explicitly absolutely immune
				"core:bonus.MIND_IMMUITY":"normal", // [to be in initial version] new format of existing mind spell immunity
				"core:artifact.armorOfWonder":"absolute",  //[possible future extension] this artifact on target itself (!) explicitly grant absolute immune
				"core:luck":["absolute", 3], // [possible future extension] lack value of at least 3 grant absolute immunity from this horrible spell
				"core:custom":[<script>] // [possible future extension] script lines for arbitrary condition
			}
		}

		"graphics":
		{
			// resource path of icon for SPELL_IMMUNITY bonus (relative to DATA or SPRITES)
			"iconImmune":"ZVS/LIB1.RES/E_SPMET",

			// resource path of icon for scenario bonus
			"iconScenarioBonus": "MYSPELL_B",

			// resource path of icon for spell effects during battle
			"iconEffect": "MYSPELL_E",

			// resource path of icon for spellbook
			"iconBook": "MYSPELL_E",

			// resource path of icon for spell scrolls
			"iconScroll": "MYSPELL_E"

		},

		"sounds":
		{
			//Resourse path of cast sound
			"cast":"LIGHTBLT"

		},

		// Mandatory structure
		// configuration for no skill, basic, adv, expert
		"levels":{
			"base": {Spell level base format},
			"none": {Spell level format},
			"basic":{Spell level format},
			"advanced":{Spell level format},
			"expert":{Spell level format}
		}
	}

}
```

## Animation format

TODO

```json
{
	"projectile": [
		{"minimumAngle": 0 ,"defName":"C20SPX4"},
		{"minimumAngle": 0.60 ,"defName":"C20SPX3"},
		{"minimumAngle": 0.90 ,"defName":"C20SPX2"},
		{"minimumAngle": 1.20 ,"defName":"C20SPX1"},
		{"minimumAngle": 1.50 ,"defName":"C20SPX0"}
	],
	"cast" : []
	"hit":["C20SPX"],
	"affect":[{"defName":"C03SPA0", "verticalPosition":"bottom", "transparency" : 0.5}, "C11SPA1"]
}
```

## Spell level base format

Json object with data common for all levels can be put here. These configuration parameters will be default for all levels. All mandatory level fields become optional if they equal "base" configuration.

### Example

This will make spell affect single target on all levels except expert, where it is massive spell.

```json
"base":{
	"range": 0
},
"expert":{
	"range": "X"
}
```

## Spell level format

TODO

```json

{
	//Localizable description. Use {xxx} for formatting
	"description": "",

	//Cost in mana points
	"cost": 1,

	// Base power of the spell. To see how it affects spell, 
	// see description of corresponding battle effect(s)
	"power": 10,

	//Mandatory, flags structure //TODO
	// modifiers make sense for creature target
	"targetModifier":
	{
		// If true, then this spell will not affect units if:
		// - target is friendly and spell is negative
		// - target is enemy, and spell is positive
		// Othervice, all units in affected area will be hit by a spell, provided they are not immune
		"smart": false,
		// LOCATION target only. All affected hexes must be empty with no obstacles or units on them
		"clearAffected": false,
	},
	
	// spell range description. Only for combat spells
	// Describes distances from spell cast point that will be affected.
	// For example, "range" : "0" will only affect hex on which this spell was cast (e.g. Magic Arrow)
	// "range" : "0,1" will affect hex on which spell was cast, as well as all hexes around it (e.g. Fireball)
	// "range" : "1" will only affect hexes around target, without affecting target itself (Frost Ring)
	// "range" : "0,1,2," or "range" : "0-2" will affect all tiles 0,1 and 2 hexes away from the target (Inferno)
	// Special case: range "X" implies massive spells that affect all units (armageddon, death ripple, destroy undead)
	"range": "X",

	// DEPRECATED, please use "battleEffects" with timed effect instead
	// List of bonuses that will affect targets for duration of the spell
	"effects":
	{
		"firstEffect": {[bonus format]},
		"secondEffect": {[bonus format]}
		//...
	},

	// DEPRECATED, please use "battleEffects" with timed effect and "cumulative" set to true instead
	// List of bonuses that will affect targets for duration of the spell. Casting spell repeatetly will cumulate effect (Disrupting Ray)
	"cumulativeEffects":
	{
		"firstCumulativeEffect": {[bonus format]}
		//...
	},

	/// See Configurable battle effects section below for detailed description
	"battleEffects":
	{
		"mod:firstEffect": {[effect format]},
		"mod:secondEffect": {[effect format]}
		//...
	}
	
	/// See Configurable adventure map effects section below for detailed description
	"adventureEffect" : {
		[effect format]
	}
}
```

## Spell power

Most of battle effects are scaled based on spell effect value. This value is same across all effects and calculated as:

```text
caster spell power * base spell power + spell mastery power(caster spell school)
```

Where:

- `caster spell school` is assumed spell school level for the spell. For unit this is value of [SPELLCASTER](../bonus/Bonus_Types.md#spellcaster) bonus. For hero this is value of [MAGIC_SCHOOL_SKILL](../bonus/Bonus_Types.md#magic_school_skill) or [SPELL](../bonus/Bonus_Types.md#spell) bonus, whichever is greater
- `spell mastery power` is `power` parameter defined in config of corresponding mastery level of the spell
- `base spell power` is `power` parameter, as defined in config of spell itself
- `caster spell power` is spellpower of the hero, or [CREATURE_SPELL_POWER](../bonus/Bonus_Types.md#creature_spell_power) bonus for units

If unit has [SPECIFIC_SPELL_POWER](../bonus/Bonus_Types.md#specific_spell_power) bonus for corresponding spell, game will use value of the bonus instead

Power of `damage`, `heal`, `summon`, and `demonSummon` effects cast by hero can also be affected by following bonuses:

- [SPECIAL_SPELL_LEV](../bonus/Bonus_Types.md#special_spell_lev) bonus for the spell, scaled down by target level (Solmyr / Deemer)

Following bonuses will only affect `damage`, `heal` and `demonSummon` effects

- [SPELL_DAMAGE](../bonus/Bonus_Types.md#spell_damage) for specific spell school (Sorcery)
- [SPECIFIC_SPELL_DAMAGE](../bonus/Bonus_Types.md#specific_spell_damage) for the spell (Luna / Ciele)

## Smart target modifier

To restrict spell from casting it on "wrong" side in combat, you can use `smart` target modifier. If this flag is set, and spell has `positive` flag, it can only affect friendly units. Similarly, spells with `negative` flag and `smart` target modifier can only affect enemies. This affects both primary targets and any secondary targets in case of area of effect or massive spells.

## Configurable battle effects

### Common format

```json

"firstSpellEffect":{
	// identifier of effect type. See type-specific documentation below for possible values
	"type":"core:effectType", 

	// effect will be deferred (f.e. land mine damage) TODO: clarify. Only dispell uses this flag!
	"indirect": false,
	
	// spell can be cast even if this effect in not applicable, for example due to immunity
	// Can be used for secondary effects, to allow casting spell if only main effect is applicable
	"optional": false 

	/// following fields are only applicable to effects that are cast on units (and not locations or summon)
	
	/// Ignore immunity unless unit has SPELL_IMMUNITY bonus for this spell with addInfo set to 1
	"ignoreImmunity" : false,

	/// Specifies number of additional targets to hit in chain, similar to Chain Lightning
	"chainLength" : 4
	
	// Only applicable for damage spells and only if chain length is non-zero.
	// Multiplier for damage for each chained target
	"chainFactor" : 0.5,
}
```

### Catapult

This spell can only be used when attacking a town with existing, non-destroyed walls. Can be also cast by defender, unless spell uses "smart" targeting

Casting the spell on location with wall will deal 0-2 damage to the walls or towers, depending on spell configuration.

Casting the spell with "massive" target will randomly pick selected number of target using logic similar to H3

```json

"firstSpellEffect":{
	"type": "core:catapult"

	// How many targets will be attacked by the spell
	"targetsToAttack": 1, 

	// If it is a targeted spell, probability to hit keep
	"chanceToHitKeep" : 5, 

	// If it is a targeted spell, probability to hit gate
	"chanceToHitGate" : 25, 

	// If it is a targeted spell, probability to hit tower
	"chanceToHitTower" : 10,

	// If it is a targeted spell, probability to hit wall 
	"chanceToHitWall" : 50, 

	// probability to deal 1 damage to wall, used for both targeted and massive
	"chanceToNormalHit" : 60, 

	// probability to deal 2 damage to wall, used for both targeted and massive
	// chance to miss is defined implicitly, as remainer of 100% chance of normal and critical hits
	"chanceToCrit" : 30 
}
```

### Clone

Configurable version of Clone spell. Casting the spell will create clone of targeted unit that belongs to side of spell caster.

```json
"firstSpellEffect":{
	"type": "core:clone"

	// Maximal tier of unit on which this spell can be cast
	"maxTier" : 3
}
```

### Damage

Deals specified damage to all affected targets based on spell effect value:

- if `killByPercentage` is set, spell will deal damage equal to unit total health * [spell effect power](#spell-power) / 100
- if `killByCount` is set, spell will deal damage equal to single creature health * [spell effect power](#spell-power)
- if neither flag is set, spell will deal damage equal to [spell effect power](#spell-power)

If spell has chain effect, damage dealt to chained target will be multiplied by specified `chainFactor`

Target with [SPELL_DAMAGE_REDUCTION](../bonus/Bonus_Types.md#spell_damage_reduction) bonus with value greater than 100% for any of spell school of the spell are immune to this effect

```json
"firstSpellEffect":{
	"type": "core:damage",
	"killByCount": false, 
	"killByPercentage" : false,
}
```

### Dispel

Dispells all bonuses provided by any other spells from this unit. Following spells can not be dispelled

- Disrupting ray
- Acid Breath
- any effects from adventure spells
- any effects that comes from this spell, including effects from previous casts of the spell

Only bonuses from spells with specified positiveness(es) will be dispelled. See configuration example.

```json
"firstSpellEffect":{
	"type": "core:dispel",
	
	/// if set, spell will dispell other spells with "positive" flag
	"dispelPositive": false, 

	/// if set, spell will dispell other spells with "negative" flag
	"dispelNegative" : false,

	/// if set, spell will dispell other spells with "indifferent" flag
	"dispelNeutral" : false,
}
```

### Heal

Effect restores [spell effect power](#spell-power) health points of affected unit. Can only be cast on unit that is not a clone and have lost some health points in the battle.

If parameter `minFullUnits` is non-zero, spell can only be cast if it will at least heal enough health points to fully restore health of specified number of units. For example, a single Archangel can only use Resurrection on units with 100 health points or lower

Spell can be used on dead units, but only if corpse is not blocked by a living unit.

```json
"firstSpellEffect":{
	"type": "core:heal",
	
	/// Minimal amount of health points that this spell can restore, based on target creature health
	"minFullUnits" : 1,
	
	/// "heal" - only heals the unit, without resurrecting any creatures
	/// "resurrect" - heals, resurrecting any dead units from stack until running out of power
	/// "overHeal" - similar to resurrect, however it may also increase unit stack size over its initial size
	"healLevel" : "heal",
	
	/// "oneBattle" - any resurrected unit will only stay alive till end of battle
	/// "permanent" - any resurrected units will stay permanently after the combat
	"healPower" : "permanent"
}
```

### Sacrifice

Sacrifice spell. Allows to destroy first target, while healing the second one. Destroyed unit is completely removed from the game.

Effect configuration is identical to [Heal effect](#heal).

```json
"firstSpellEffect":{
	"type": "core:sacrifice"
	"minFullUnits" : 1,
	"healLevel" : "heal",
	"healPower" : "permanent"
}
```

### Obstacle

TODO

```json
"firstSpellEffect":{
	"type": "core:obstacle"
	
	"hidden" : false,
	"passable" : false,
	"trap" : false,
	"removeOnTrigger" : false,
	"hideNative" : false,

	"patchCount" : 1,
	"turnsRemaining" : 1,
	"triggerAbility" : "obstacleTriggerAbility",
	
	"attacker" : {
		"shape" : [],
		"range" : [],
		"appearSound" : {},
		"appearAnimation" : {},
		"animation" : {},
		"offsetY" : 0
	},
	
	"defender" : {
		"shape" : [],
		"range" : [],
		"appearSound" : {},
		"appearAnimation" : {},
		"animation" : {},
		"offsetY" : 0
	}
}
```

### Moat

TODO

```json
"firstSpellEffect":{
	"type": "core:moat"
	
	"hidden" : false,
	"trap" : false,
	"removeOnTrigger" : false,
	"dispellable" : false,

	"moatDamage" : 90,
	"moatHexes" : [],

	"triggerAbility" : "obstacleTriggerAbility",
	
	"defender" : {
		"shape" : [],
		"range" : [],
		"appearSound" : {},
		"appearAnimation" : {},
		"animation" : {},
		"offsetY" : 0
	}
}
```

### Remove obstacle

Effect removes an obstacle from targeted hex

```json
"firstSpellEffect":{
	"type": "core:removeObstacle",
	
	/// If set to true, spell can remove large ("absolute") obstacles
	"removeAbsolute" : false,

	/// If set to true, spell can remove small obstacles (H3 behavior)
	"removeUsual" : true,
	
	// If set to true, spell can remove any obstacle that was created by spell
	"removeAllSpells" : true,
	
	// If set to true, spell can remove obstacles that were created with specific spell
	"removeSpells" : [ "spellA", "spellB" ],
}
```

### Summon

Effect summons additional units to the battlefield.

If `exclusive` flag is set, attempt to summon a different creature by the same side in combat will fail (even if previous summon was non-exclusive)

Amount of summoned units is equal to [spell effect power](#spell-power).  Summoned units will disappear after combat, unless `permanent` flag in effect config is set

If `summonByHealth` option is set, then number of summoned units will be equal to [spell effect power](#spell-power) / unit health. Hero need to be able to summon at least one full unit for spell to work

if `summonSameUnit` flag is set, and same creature was already summoned before, spell will instead heal unit in "overheal" mode, using same [spell effect power](#spell-power).

```json
"firstSpellEffect":{
	"type": "core:summon",
	
	/// Unit to summon
	"id" : "airElemental",
	
	"permanent" : false,
	"exclusive" : false,
	"summonByHealth" : false,
	"summonSameUnit" : false,
}
```

### Demon Summon

Implements Pit Lord's ability with the same name. Raises targeted dead unit as unit specified in spell parameters on casters side. New unit will be placed on the same position as corpse, and corpse will be removed from the battlefield

Raised amount of units is limited by (rounded down):

- total HP of summoned unit can not be larger than [spell effect power](#spell-power) of caster
- total HP of summoned unit can not be larger than total HP of dead unit
- total stack size of summoned unit can not be greater than stack size of dead unit

```json
"firstSpellEffect":{
	"type": "core:demonSummon",
	
	/// Unit to summon
	"id" : "demon",
	
	/// If true, unit will remain after combat
	"permanent" : false
}
```

### Teleport

Effect instantly moves unit from its current location to targeted tile

```json
"firstSpellEffect":{
	"type": "core:teleport",
	
	/// If true, unit will trigger obstacles on destination location
	"triggerObstacles" : false,
	
	/// If true, unit can be teleported across moat during town siege
	"isMoatPassable" : false,

	/// If true, unit can be teleported across walls during town siege
	"isWallPassable" : false,
}
```

### Timed

Timed effect gives affected units specified bonuses for duration of the spell.

Duration of effect is:

- Hero: Spellpower + value of [SPELL_DURATION](../bonus/Bonus_Types.md#spell_duration) + [SPELL_DURATION](../bonus/Bonus_Types.md#spell_duration) for specific spell
- Units: value of [CREATURE_ENCHANT_POWER](../bonus/Bonus_Types.md#creature_enchant_power), or 3 if no such bonus

Value of all bonuses can be affected by following bonuses:

- [SPECIAL_PECULIAR_ENCHANT](../bonus/Bonus_Types.md#special_peculiar_enchant): value modified by 1-3 according to level of target unit
- [SPECIAL_ADD_VALUE_ENCHANT](../bonus/Bonus_Types.md#special_add_value_enchant): value from addInfo is added to val of bonus
- [SPECIAL_FIXED_VALUE_ENCHANT](../bonus/Bonus_Types.md#special_fixed_value_enchant): value from addInfo replaces val of bonus

```json
"firstSpellEffect" : {
	"type": "core:timed",

	// if set to true, recasting same spell will accumulate (and prolong) effects of previous spellcast
	"cumulative" : false
	
	// List of bonuses granted by this spell
	"bonus" : {
		"firstBonus" : {[bonus format]}
		//...
	}
}
```

## Target types

### CREATURE

- range 0: smart assumed single creature target
- range "X" + smart modifier = enchanter casting, expert massive spells
- range "X" + no smart modifier = armageddon, death ripple, destroy undead
- any other range (including chain effect)
- smart modifier: smth like cloud of confusion in H4 (if I remember correctly :) )
- no smart modifier: like inferno, fireball etc. but target only creature

### NO_TARGET

- no target selection,(abilities, most adventure spells)

### LOCATION

- any tile on map/battlefield (inferno, fireball etc.), DD also here but with special handling
- clearTarget - destination hex must be clear (unused so far)
- clearAfffected - all affected hexes must be clear (forceField, fireWall)

### OBSTACLE

- range 0: any single obstacle
- range X: all obstacles

## Configurable adventure map effects

Currently, VCMI does not allow completely new spell effects for adventure maps. However, it is possible to:

- modify the parameters of all H3 spells.
- create spells with similar effects to H3 spells
- create a spell that gives bonuses to the hero who cast the spell.

Unlike combat effects, adventure map spells can only have one special effect, such as the Dimension Door or Town Portal effect. The number of bonuses granted by an adventure map spell is unlimited.

The AI has a limited understanding of adventure map spells and may use the following spells:

- Spells that give `WATER_WALKING` or `FLYING_MOVEMENT` bonuses
- Spells with the Summon Boat effect, provided the spell can create new boats with a 100% success chance.
- Any spells with the Town Portal effect.

### Common format

All properties in this section can be used for all non-generic adventure map spell effects.

Parameters:

- `type` - the type of spell effect used for this spell, or `generic` if a custom mechanic is not used.
- `castsPerDay` - Optional. Defines how many times a hero can cast this spell per day; set to zero or omit for unlimited use.
- `castsPerDayXL` - Optional. An alternative cast-per-day limit that is only active on maps that are at least XL+U in size. If this value is not set or is set to zero, the game will use the value of the `castsPerDay` variable.
- `bonuses` - A list of bonuses that will be given to the hero when this spell is cast successfully. When used with effects that can fail (e.g. Summon Boat), the bonuses will only apply to a successful cast.

Example:

```json
"adventureEffect" : {
	"type" : "generic",
	"castsPerDay" : 0,
	"castsPerDayXL" : 0,
	"bonuses" : {
		"fly" : {
			"type" : "FLYING_MOVEMENT",
			"duration" : "ONE_DAY",
			"val" : 40,
			"valueType" : "INDEPENDENT_MIN"
		}
	}
}
```

### Dimension Door

The effect instantly teleports the hero to the selected location.

Parameters:

- `movementPointsRequired` - The amount of movement points the hero must have to cast this spell.
- `movementPointsTaken` - The amount of movement points that will be taken if the spell is cast successfully. If the hero does not have enough movement points, they will be reduced to zero after casting.
- `waterLandFailureTakesPoints` - If set to true, mana and movement points will be spent on an attempt to teleport to an inaccessible location (e.g. teleporting to land while in a boat).
- `cursor` - Identifier of the cursor that will be shown when hovering over a valid destination tile. See `config/cursors.json` for more details.
- `cursorGuarded` - alternative cursor that appears if using the teleport spell on a target would result in combat. This is only used if the game rule 'dimensionDoorTriggersGuards' is active.
- `exposeFow` - If this is set to true, using this spell will reveal information behind fog of war, such as whether teleportation is possible or if the location is guarded.
- `ignoreFow` - If this is set to true, it is possible to use the spell to teleport into terra incognita.
- `rangeX` - maximum distance to teleport in the X dimension (left-right axis).
- `rangeY` - maximum distance to teleport in the Y dimension (top-bottom axis).

Example:

```json
"adventureEffect" : {
	"type" : "dimensionDoor",
	"movementPointsRequired" : 0,
	"movementPointsTaken" : 300,
	"waterLandFailureTakesPoints" : true,
	"cursor" : "mapDimensionDoor",
	"cursorGuarded" : "mapTurn1Attack",
	"castsPerDay" : 2,
	"rangeX" : 9,
	"rangeY" : 8,
	"ignoreFow" : true,
	"exposeFow" : true
}
```

### Remove Object

The effect completely removes the targeted object from the map. The Scuttle Boat spell is an example of this effect. The success chance is defined as [spell effect power](#spell-power).

Parameters:

- `objects` - a list of map objects that can be removed by this spell.
- `cursor` - identifier of the cursor that will be displayed when hovering over a valid target object.  See `config/cursors.json` for more details.
- `ignoreFow` - If set to true, it is possible to use this spell to remove objects behind terra incognita.
- `rangeX` - maximum distance to remove objects in the X dimension (left-right axis).
- `rangeY` - maximum distance to remove objects in the Y dimension (top-bottom axis).

Example:

```json
"adventureEffect" : {
	"type" : "removeObject",
	"castsPerDay" : 0,
	"cursor" : "mapScuttleBoat",
	"rangeX" : 9,
	"rangeY" : 8,
	"ignoreFow" : false,
	"objects" : {
		"boat" : true
	}
}
```

### Summon Boat

The effect moves or creates a boat next to the hero who cast the spell. The success chance is defined as [spell effect power](#spell-power).

Parameters:

- `useExistingBoat` - If this is set to true, the spell can move existing boats to the hero's location.
- `createdBoat` - Optional identifier of the boat type that can be created by this spell. If this is not set, the spell cannot create new boats.

Note that if the spell can both create new boats and use existing ones, it would prefer to move existing boats and only create new ones if there are no suitable ones to move.

Example:

```json
"adventureEffect" : {
	"type" : "summonBoat",
	"castsPerDay" : 0,
	"useExistingBoat" : true,
	"createdBoat" : "boatNecropolis"
}
```

### Town Portal

Effect moves hero to a location of owned or allied town.

Parameters:

- `movementPointsRequired` - amount of movement points that hero must have to cast this spell
- `movementPointsTaken` - amount of movement points that will be taken on sucessful cast of the spell. If hero does not have enough movement points, they will be reduced to zero after cast
- `allowTownSelection` - if set to true, player will be able to select town to teleport to among all friendly non-occupied towns.
- `skipOccupiedTowns` - if set to true, hero will teleport to nearest non-occupied town, ignoring any closer towns that are occupied by a visiting hero. No effect if `allowTownSelection` is set.

Example:

```json
"adventureEffect" : {
	"type" : "townPortal",
	"castsPerDay" : 2,
	"allowTownSelection" : false,
	"skipOccupiedTowns" : false,
	"movementPointsRequired" : 300,
	"movementPointsTaken" : 300
}
```

### View World

Effect shows World View menu with specified objects behind FoW revealed to the player

Parameters:

- `objects` - list of object types that will be revealed on World View. Note that only following objects have assotiated icon, any objects not from this list will not be visible: `resource`, `mine`, `abandonedMine`, `artifact`, `hero`, `town`.
- `showTerrain` - if set to true, terrain of the entire map (but not objects on it) will be revealed to the player.

Example:

```json
"adventureEffect" : {
	"type" : "viewWorld",
	"objects" : {
		"resource" : true,
		"mine" : true,
		"abandonedMine" : true
	},
	"showTerrain" : true
}
```
