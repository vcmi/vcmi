# Spell Format

## Main format

``` javascript
{
	"spellName":
	{	
		// Mandatory. Spell type 
		// Allowed values: "adventure", "combat", "ability"
		"type": "adventure",
		
		// Mandatory. Spell target type
		// "NO_TARGET" - instant cast no aiming (e.g. Armageddon)
		// "CREATURE" - target is unit (e.g. Resurrection)
		// "OBSTACLE" - target is obstacle (e.g. Remove Obstacle)
		// "LOCATION" - target is location (e.g. Fire Wall)
		"targetType":"NO_TARGET",

		// Localizable name of this spell
		"name": "Localizable name",
	
		// Mandatory. List of spell schools this spell belongs to
		"school": {"air":true, "earth":true, "fire":true, "water":true},
	
		// Mandatory. Spell level, value in range 1-5, or 0 for abilities
		"level": 1,

		// Mandatory. Base power of the spell
		"power": 10,
	
		// Mandatory. Default chance for this spell to appear in Mage Guilds
		// Used only if chance for a faction is not set in gainChance field
		"defaultGainChance": 0, 
	
		// Chance for this spell to appear in Mage Guild of a specific faction
		// Symmetric property of "guildSpells" property in towns
		"gainChance":
		{
			"factionName" : 3
		},

		"animation":{<Animation format>},
			
		// List of spells that will be countered by this spell
		"counters": {
			"spellID" : true, 
			...
		},

		//Mandatory. List of flags that describe this spell
		// positive - this spell is positive to target (buff)
		// negative - this spell is negative to target (debuff)
		// indifferent - spell is neither positive, nor negative
		// damage - spell does damage (direct or indirect)
		// offensive - direct damage (implicitly sets damage and negative)
		// rising - rising spell (implicitly sets positive)
		// special - this spell is normally unavailable and can only be received explicitly, e.g. from bonus SPELL
		// nonMagical - this spell is not affected by Sorcery or magic resistance. School resistances apply.
		"flags" : {
			"positive": true,
		},
		
		// If true, then creature capable of casting this spell can cast this spell on itself
		// If false, then creature can only cast this spell on other units
		"canCastOnSelf" : false,
		
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

``` javascript
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
	"affect":[{"defName":"C03SPA0", "verticalPosition":"bottom"}, "C11SPA1"]
}
```

## Spell level base format

Json object with data common for all levels can be put here. These configuration parameters will be default for all levels. All mandatory level fields become optional if they equal "base" configuration.

### Example

This will make spell affect single target on all levels except expert, where it is massive spell.

``` javascript
"base":{
	"range": 0
},
"expert":{
	"range": "X"
}
```

## Spell level format

TODO

``` javascript

{
	//Mandatory, localizable description. Use {xxx} for formatting
	"description": "",

	//Mandatory, cost in mana points
	"cost": 1,

	//Mandatory, number
	"power": 10,

	//Mandatory, number
	"aiValue": 20,

	//Mandatory, flags structure //TODO
	// modifiers make sense for creature target
	"targetModifier":
	{
		"smart": false,	//true: friendly/hostile based on positiveness; false: all targets
		"clearTarget": false,
		"clearAffected": false,
	},
	
	//Mandatory
	//spell range description in SRSL
	// range "X" + smart modifier = enchanter casting, expert massive spells
	// range "X" + no smart modifier = armageddon, death ripple, destroy undead

	"range": "X",

	//DEPRECATED, Optional, arbitrary name - bonus format map
	//timed effects, overriding by name
	"effects":
	{
		"firstEffect": {[bonus format]},
		"secondEffect": {[bonus format]}
		//...

	
	},
	//DEPRECATED, cumulative effects that stack while active
	"cumulativeEffects":
	{
		"firstCumulativeEffect": {[bonus format]}
		//...

	},
	"battleEffects":
	{
		"mod:firstEffect": {[effect format]},
		"mod:secondEffect": {[effect format]}
		//...

	}
}
```

## Configurable battle effects

**If spell have at least one special effect it become configurable spell and spell configuration processed different way**

### Configurable spell

Configurable spells ignore *offensive* flag, *effects* and *cumulativeEffects*. For backward compatibility *offensive* flag define Damage effect, *effects* and *cumulativeEffects* define Timed effect.

### Special effect common format

TODO

``` javascript

"mod:effectId":{

"type":"mod:effectType", //identifier of effect type
"indirect": false, // effect will be deferred (f.e. land mine damage) 
"optional": false // you can cast spell even if this effect in not applicable

//for unit target effects
"ignoreImmunity" : false,
"chainFactor" : 0.5,
"chainLength" : 4

//other fields depending on type
}
```

### Catapult

TODO

``` javascript

"mod:effectId":{

"type": "core:catapult"
	"targetsToAttack": 1, //How many targets will be attacked by this
	"chanceToHitKeep" : 5, //If it is a targeted spell, chances to hit keep
	"chanceToHitGate" : 25, //If it is a targeted spell, chances to hit gate
	"chanceToHitTower" : 10, //If it is a targeted spell, chances to hit tower
	"chanceToHitWall" : 50, //If it is a targeted spell, chances to hit wall
	"chanceToNormalHit" : 60, //Chance to have 1 damage to wall, used for both targeted and massive
	"chanceToCrit" : 30 //Chance to have 2 damage to wall, used for both targeted and massive
}
```

### Clone

TODO

Configurable version of Clone spell.

``` javascript

"mod:effectId":{

"type": "core:clone"

"maxTier" : 3//unit tier
}
```

### Damage effect

TODO

If effect is automatic, spell behave like offensive spell (uses power, levelPower etc)

``` javascript

"mod:effectId":{

"type": "core:damage",
"killByCount": false, //if true works like Death Stare
"killByPercentage" : false, //if true works like DESTRUCTION ability

//TODO: options override

}
```

### Dispel

TODO

### Heal

TODO

### Obstacle

TODO

### Remove obstacle

TODO

### Sacrifice

TODO

### Summon

TODO

### Teleport

TODO

### Timed

TODO

If effect is automatic, spell behave like \[de\]buff spell (effect and
cumulativeEffects ignored)

``` javascript

"mod:effectId":{

"type": "core:timed",
"cumulative": false
"bonus":
{
"firstBonus":{[bonus format]}
//...
}
}
```

## Additional documentation

### Targets, ranges, modifiers

TODO

- CREATURE target (only battle spells)
 - range 0: smart assumed single creature target
 - range "X" + smart modifier = enchanter casting, expert massive spells
 - range "X" + no smart modifier = armageddon, death ripple, destroy undead
 - any other range (including chain effect)
  - smart modifier: smth like cloud of confusion in H4 (if I remember correctly :) )
  - no smart modifier: like inferno, fireball etc. but target only creature

- NO_TARGET
 - no target selection,(abilities, most adventure spells)

- LOCATION
 - any tile on map/battlefield (inferno, fireball etc.), DD also here but with special handling
 - clearTarget - destination hex must be clear (unused so far)
 - clearAfffected - all affected hexes must be clear (forceField, fireWall)

- OBSTACLE target
 - range 0: any single obstacle
 - range X: all obstacles