# Main format

``` javascript
{
	"spellName":
	{	//numeric id of spell required only for original spells, prohibited for new spells
		"index": 0,
		//Original Heroes 3 info
		//Mandatory, spell type 
		"type": "adventure",//"adventure", "combat", "ability"
		
		//Mandatory, spell target type
		"targetType":"NO_TARGET",//"CREATURE","OBSTACLE"."LOCATION"

		//Mandatory
		"name": "Localizable name",
		//Mandatory, flags structure of school names, Spell schools this spell belongs to
		"school": {"air":true, "earth":true, "fire":true, "water":true},
		//number, mandatory, Spell level, value in range 1-5
		"level": 1,
		//Mandatory, base power
		"power": 10,
		//Mandatory, default chance for this spell to appear in Mage Guilds
		//Used only if chance for a faction is not set in gainChance field
		"defaultGainChance": 0, 
		//Optional, chance for it to appear in Mage Guild of a specific faction
		//NOTE: this field is linker with faction configuration
		"gainChance":
		{
			"factionName": 3
		},
		//VCMI info
			
		"animation":{<Animation format>},
			
		//countering spells, flags structure of spell ids (spell. prefix is required)
		"counters": {"spell.spellID1":true, ...}

		//Mandatory,flags structure:
		//              indifferent, negative, positive - Positiveness of spell for target (required)
		//		damage - spell does damage (direct or indirect)
		//		offensive - direct damage (implicitly sets damage and negative)
		//		rising - rising spell (implicitly sets positive)
		//		summoning //todo:
		//		special - can be obtained only with bonus::SPELL

		"flags" : {"flag1": true, "flag2": true},

		//DEPRECATED | optional| no default | flags structure of bonus names,any one of these bonus grants immunity. Negatable by the Orb.
		"immunity": {"BONUS_NAME":true, ...},
			
		//DEPRECATED | optional| no default | flags structure of bonus names
		//any one of these bonus grants immunity, cant be negated
		"absoluteImmunity": {"BONUS_NAME": true, ...},

		//DEPRECATED | optional| no default | flags structure of bonus names, presence of all bonuses required to be affected by. Negatable by the Orb.
		"limit": {"BONUS_NAME": true, ...},

		//DEPRECATED | optional| no default | flags structure of bonus names, presence of all bonuses required to be affected by. Cant be negated
		"absoluteLimit": {"BONUS_NAME": true, ...},
		
		//[WIP] optional | default no limit no immunity
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


		//graphics; mandatory; object;
		"graphics":
		{
			//  ! will be moved to bonus type config in next bonus config version
			//  iconImmune - OPTIONAL; string; 
			//resource path of icon for SPELL_IMMUNITY bonus (relative to DATA or SPRITES)
			"iconImmune":"ZVS/LIB1.RES/E_SPMET",


			//  iconScenarioBonus- mandatory, string, image resource path
			//resource path of icon for scenario bonus
			"iconScenarioBonus": "MYSPELL_B",

			//  iconEffect- mandatory, string, image resource path
			//resource path of icon for spell effects during battle
			"iconEffect": "MYSPELL_E",

			//  iconBook- mandatory, string, image resource path
			//resource path of icon for spellbook
			"iconBook": "MYSPELL_E",

			//  iconScroll- mandatory, string, image resource path
			//resource path of icon for spell scrolls
			"iconScroll": "MYSPELL_E"

		},

		//OPTIONAL; object; TODO
		"sounds":
		{
			//OPTIONAL; resourse path, casting sound
			"cast":"LIGHTBLT"

		},

		//Mandatory structure
		//configuration for no skill, basic, adv, expert
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

# Animation format

``` javascript

{
	"projectile": [
		{"minimumAngle": 0 ,"defName":"C20SPX4"},
		{"minimumAngle": 0.60 ,"defName":"C20SPX3"},
		{"minimumAngle": 0.90 ,"defName":"C20SPX2"},
		{"minimumAngle": 1.20 ,"defName":"C20SPX1"},
		{"minimumAngle": 1.50 ,"defName":"C20SPX0"}
	],
	"hit":["C20SPX"],
	"affect":[{"defName":"C03SPA0", "verticalPosition":"bottom"}, "C11SPA1"]
}
```

# Spell level base format

Json object with data common for all levels can be put here. These
configuration parameters will be default for all levels. All mandatory
level fields become optional if they equal "base" configuration.

## Example

This will make spell affect single target on all levels except expert,
where it is massive spell.

``` javascript

"base":{

   "range": 0
},
"expert":{
"range": "X"
}
```

# Spell level format

``` javascript

{
	//Mandatory, localizable description
	//Use {xxx} for formatting
	"description": "",


	//Mandatory, number, 
	//cost in mana points
	"cost": 1,

	//Mandatory, number
	"power": 10,

	//Mandatory, number
	"aiValue": 20,

	//Mandatory, flags structure //TODO
	// modifiers make sense for creature target
	//
	//
	"targetModifier":
	{
		"smart": false,	//true: friendly/hostile based on positiveness; false: all targets
		"clearTarget": false,
		"clearAffected": false,
	}
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

# Configurable battle effects

**If spell have at least one special effect it become configurable spell
and spell configuration processed different way**

## Configurable spell

Configurable spells ignore *offensive* flag, *effects* and
*cumulativeEffects*. For backward compatibility *offensive* flag define
Damage effect, *effects* and *cumulativeEffects* define Timed effect.

## Special effect common format

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

## catapult

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

## Clone

Configurable version of Clone spell.

``` javascript

"mod:effectId":{

"type": "core:clone"

"maxTier" : 3//unit tier
}
```

## Damage effect

If effect is automatic, spell behave like offensive spell (uses power,
levelPower etc)

``` javascript

"mod:effectId":{

"type": "core:damage",
"killByCount": false, //if true works like Death Stare
"killByPercentage" : false, //if true works like DESTRUCTION ability

//TODO: options override

}
```

## Dispel

documetation

## Heal

documetation

## Obstacle

documetation

## Remove obstacle

documetation

## Sacrifice

documetation

## Summon

documetation

## Teleport

documetation

## Timed

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

# Additional documentation

## Targets, ranges, modifiers

-   CREATURE target (only battle spells)
    -   range 0: smart assumed single creature target
    -   range "X" + smart modifier = enchanter casting, expert massive
        spells
    -   range "X" + no smart modifier = armageddon, death ripple,
        destroy undead
    -   any other range (including chain effect)
        -   smart modifier: smth like cloud of confusion in H4 (if I
            remember correctly :) )
        -   no smart modifier: like inferno, fireball etc. but target
            only creature

<!-- -->

-   NO_TARGET
    -   no target selection,(abilities, most adventure spells)

<!-- -->

-   LOCATION
    -   any tile on map/battlefield (inferno, fireball etc.), DD also
        here but with special handling
    -   clearTarget - destination hex must be clear (unused so far)
    -   clearAfffected - all affected hexes must be clear (forceField,
        fireWall)

<!-- -->

-   OBSTACLE target
    -   range 0: any single obstacle
    -   range X: all obstacles