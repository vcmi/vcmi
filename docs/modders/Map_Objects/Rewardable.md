# Rewardable

## Base object definition

Rewardable object is defined similarly to other objects, with key difference being `handler`. This field must be set to `"handler" : "configurable"` in order for vcmi to use this mode.

```json
{
  "baseObjectName" : {
    "name" : "Object name",
    "handler" : "configurable", 
    "types" : {
      "objectName" : 
      {
        "rmg" : {
          "value"    : 2500,
          "rarity"  : 25,
          "zoneLimit" : 1
        },
        
        // Standard definition of object templates
        "templates" : {
          "avwrhscr" : {
            "animation" : "warehouses/avwrhscr", 
            "visitableFrom" : [ "---", "-++", "+++" ], 
            "mask" : ["VVV","VVV","VBA"]
          }
        }

        // See Configurable object definition section

      }
    }
  }
}
```

## Configurable object definition

```json
// List of potential rewards
"rewards" : [
  {
    // see Appear Chance definition section
    "appearChance" : {
    },
    
    // If specified, then when loading from h3m map object may load preconfigured set of reward instead of randomizing them via appearChance
    // Only supported for some objects, such as Scholar (SoD), and several other added in hota map format
    "mapDice" : 0,

    // Conditions to receive reward. Hero can only see this reward if he fulfills limiter
    "limiter" : {
      
      // additional list of conditions. Limiter will be valid if any of these conditions are true
      "anyOf" : [
        {
          // See "Configurable Properties" section for additional parameters
          <additional properties>
        }
      ]

      // additional list of conditions. Limiter will be valid only if none of these conditions are true
      "noneOf" : [
        {
          // See "Configurable Properties" section for additional parameters
          <additional properties>
        }
      ]

      // See "Configurable Properties" section for additional parameters
      <additional properties>
    }
    
    // message that will be shown if this is the only available award
    "message": "{Warehouse of Crystal}"

    // Alternative object description that will be used in place of generic description after player visits this object and reveals its content
    // For example, Tree of Knowledge will display cost of levelup (gems or gold) only after object has been visited once
    "description" : "",

    // object will be disappeared after taking reward is set to true
    "removeObject": false,

    // See "Configurable Properties" section for additional parameters
    <additional properties>
  }
],

/// List of variables shared between all rewards and limiters
/// See "Variables" section for description
"variables" : {
}

// If true, hero can not move to visitable tile of the object and will access this object from adjacent tile (e.g. Treasure Chest)
"blockedVisitable" : true,

// Message that will be shown if there are no applicable awards
"onEmptyMessage": "",

// Object description that will be shown when player right-clicks object
"description" : "",

// If set to true, and objects is guarded, then combat will start immediately, without asking player for confirmation on whether to attack guardians
"forceCombat" : true,

// If set to true, right-clicking previously visited object would show preview of its content. For example, Witch Hut will show icon with provided skill
"showScoutedPreview" : true,

// Text that should be used if hero has not visited this object. If not specified, game will use standard "(Not visited)" text
"notVisitedTooltip" : "",

// Text that should be used if hero has already visited this object. If not specified, game will use standard "(Already visited)" text
"visitedTooltip" : "",

// Used only if visitMode is set to "limiter"
// Hero that passes this limiter will be considered to have visited this object
// Note that if player or his allies have never visited this object, it will always show up as "not visited"
"visitLimiter" : {
},

// Alternatively, rewards for empty state:
// Format is identical to "rewards" section, allowing to fine-tune behavior in this case, including giving awards or different messages to explain why object is "empty". For example, Tree of Knowledge will give different messages depending on whether it asks for gold or crystals
"onEmpty" : [
]

// Message that will be shown if there are multiple selectable awards to choose from
"onSelectMessage" : "",

// Message that will be shown if object has undefeated guards
"onGuardedMessage" : "",

// Message that will be shown if this object has been already visited before
"onVisitedMessage" : "{Warehouse of Crystal}\r\n\r\nThe owner of the storage is apologising: 'I am sorry Milord, no crystal here. Please, return next week!'",

// Alternatively, rewards for visited state:
// Format is identical to "rewards" section, allowing to fine-tune behavior of already visited object, including potentially giving bonuses to player, e.g. Warrior's Tomb give -3 morale for visiting such object.
"onVisited" : [
]

// Layout of units in the battle (only used if guards are present)
// Predefined values:
// "default" - attacker is on the left, defender is on the right, war machine, tactics, and battlefield obstacles are present
// "creatureBankNarrow" - emulates H3 logic for banks with narrow (1-tile wide) units
// "creatureBankWide" - emulates H3 logic for banks with wide units that take 2 hexes
// Additionally, it is possible to define new layouts, see "layouts" field in (vcmi install)/config/gameConfig.json file
"guardsLayout" : "default"

// if true, then player can refuse from reward and don't select anything
// Note that in this case object will not become "visited" and can still be revisited later
"canRefuse": true,

// If set to true, then this object can be visited from land when placed next to a coast.
// NOTE: make sure that object also has "blockedVisitable" set to true. Othervice, behavior is undefined
"coastVisitable" : true

// Controls when object state will be reset, allowing potential revisits. See Reset Parameters definition section
"resetParameters" : {
}

// determines who can revisit object before reset
// "once",         - object can only be visited once. First visitor takes it all.
// "hero",         - object can be visited if this hero has not visited it before
// "limiter",      - object can be visited if hero fails to fulfill provided limiter
// "player",       - object can be visited if this player has not visited it before
// "playerGlobal", - object can be visited if this player has not visited this object or any other object of this type before
// "bonus"         - object can be visited if hero no longer has bonus from this object (including any other object of the same type)
// "unlimited"     - no restriction on revisiting.
"visitMode" : "unlimited", 

//determines way to select granted rewards if multiple options are available
// "selectFirst",  - first reward which passes "limiter" will be granted to player
// "selectPlayer", - player will be allowed to choose between rewards (e.g. treasure chest)
// "selectRandom", - granted reward will be picked randomly when hero visits object
// "selectAll" - every reward which passes "limiter" will be granted to player
"selectMode" : "selectFirst"

}
```

## Variables Parameters definition

This property allows defining "variables" that are shared between all rewards and limiters of this object.
Variables are randomized only once, so you can use them multiple times for example, to give skill only if hero does not have this skill (e.g. Witch Hut).

Example of creation of a variable named "gainedSkill" of type "secondarySkill":

```json
"variables" : {
	"secondarySkill" : {
		"gainedSkill" : {
			"noneOf" : [
				"leadership",
				"necromancy"
			]
		}
	}
}
```

Possible variable types:

- number: can be used in any place that expects a number
- artifact
- spell
- primarySkill
- secondarySkill

To reference variable in limiter prepend variable name with '@' symbol:

```json
"secondary" : {
    "@gainedSkill" : 1
},
```

## Reset Parameters definition

This property describes how object state should be reset. Objects without this field will never reset its state.

- Period describes interval between object resets in day. Periods are counted from game start and not from hero visit, so reset duration of 7 will always reset object on new week & duration of 28 will always reset on new month.
- If `visitors` is set to true, game will reset list of visitors (heroes and players) on start of new period, allowing revisits of objects with `visitMode` set to `once`, `hero`, or `player`. Objects with visit mode set to `bonus` are not affected. In order to allow revisit such objects use appropriate bonus duration (e.g. `ONE_DAY` or `ONE_WEEK`) instead.
- If `rewards` is set to true, object will re-randomize its provided rewards, similar to such H3 objects as "Fountain of Fortune" or "Windmill"

```json
"resetParameters" : {
    "period" : 7,
    "visitors" : true,
    "rewards" : true
}
```

## Appear Chance definition

This property describes chance for reward to be selected.
When object is initialized on map load, game will roll a "dice" - random number in range 0-99, and pick all awards that have appear chance within selected number.
Note that object that uses appearChance MUST have continuous range for every value in 0-99 range. For example, object with 3 different rewards may want to define them as

- `"min" :  0, "max" : 33`
- `"min" : 33, "max" : 66`
- `"min" : 66, "max" : 100`
In other words, min chance of second reward must be equal to max chance of previous reward

```json
    "appearChance": 
    {
      // (Advanced) rewards with different dice number will get different dice number
      // This allows (for example) choosing two rewards randomly, independent from each other
      // For H3 objects, this is generally not needed and this field can be omitted
      "dice": 1,
  
      // reward will be selected only if random roll value is greater or equal than this
      "min" : 33,
      
      // reward will be selected only if random roll value is lower than this
      "max" : 66
    },
```

## Configurable Properties

Unless stated othervice, all numbers in this section can be replaced with random values, e.g.

```json
"minLevel" : { "min" : 5, "max" : 10 } // select random number between 5-10, including both 5 & 10
"minLevel" : [ 2, 4, 6, 8, 10] // (VCMI 1.2) select random number out of provided list, with equal chance for each
```

In this case, actual value for minLevel will be picked randomly.
Keep in mind, that all randomization is performed on map load and on object reset (if `rewards` field in `resetParameter` was set).

### Current Day

- Can only be used as limiter. To pass, current day of week should be equal to this value. 1 = first day of the week, 7 = last day

```json
"dayOfWeek" : 0 
```

- Can only be used as limiter. To pass, number of days since game started must be at equal or greater than this value

```json
"daysPassed" : 8
```

### Resource

- Can be used as limiter. To pass, player needs to have specified resources. Note that limiter will NOT take resources.
- Can be used as reward to grant resources to player
- If negative value is used as reward, it will be used as cost and take resources from player

```json
"resources": {
    "crystal" : 6,
    "gold" : -1000,
}, 
```

- Alternative format that allows random selection of a resource type

```json
"resources": [
    {
        "anyOf" : [ "wood", "ore" ],
        "amount" : 10
    },
    {
        "type" : "gold",
        "amount" : 1000
    }
],
```

### Hero Experience

- Can be used as limiter
- Can be used as reward to grant experience to hero

```json
"heroExperience" : 1000, 
```

### Hero Level

- Can be used as limiter. Hero requires to have at least specified level
- Can be used as reward, will grant hero experience amount equal to the difference between the hero's next level and current level (Tree of Knowledge)

```json
"heroLevel" : 1,
```

### Mana Points

- Can be used as limiter. Hero must have at least specific mana amount
- Can be used as reward, to give mana points to hero. Mana points may go above mana pool limit.
- If negative value is used as reward, it will be used as cost and take mana from player

```json
"manaPoints": -10, 
```

- If giving mana points puts hero above mana pool limit, any overflow will be multiplied by specified percentage. If set to 0, mana will not go above mana pool limit.

```json
"manaOverflowFactor" : 50,
```

### Mana Percentage

- Can be used as limiter. Hero must have at least specific mana percentage
- Can be used to set hero mana level to specified percentage value, not restricted to mana pool limit (Magic Well, Mana Spring)

```json
"manaPercentage": 200, 
```

### Movement Points

- Can NOT be used as limiter
- Can be used as reward, to give movement points to hero. Movement points may go above mana pool limit.

```json
"movePoints": 200,
```

### Movement Percentage

- Can NOT be used as limiter
- Can be used to set hero movement points level to specified percentage value. Value of 0 will take away any remaining movement points

```json
"movePercentage": 50,
```

### Commander

- Can be used as limiter, hero must have alive commander
- If hero does not have commander (for example, in games without them), limiter will always fail

```json
"commanderAlive" : true
```

### Primary Skills

- Can be used as limiter, hero must have primary skill at least at specified level
- Can be used as reward, to increase hero primary skills by selected value
- If reward value is negative, value will be used as cost, decreasing primary skill
- Each primary skill can be explicitly specified or randomly selected
- Possible values: `"attack", "defence", "spellpower", "knowledge"`

```json
"primary": [
    {
        // Specific primary skill
        "type" : "defence",
        "amount" : 1
    },
    {
        // Primary skill will be selected randomly from the list
        "anyOf" : ["attack", "defence],
        "min" : 1,
        "max" : 3
    },
    {
        // Primary skill will be selected randomly, expect those
        "noneOf" : ["knowledge"],
        "amount" : 1
    },
    {
        // Primary skill will be selected randomly
        "amount" : 3
    }
]
```

### Secondary Skills

- Can be used as limiter, hero must have secondary skill at least at specified level
- Can be used as reward, to grant secondary skills to hero
- If hero already has specified skill, the skills will be leveled up specified number of times
- If hero does not have selected skill and have free skill slots, he will receive skill at specified level
- Possible values: 1 (basic), 2 (advanced), 3 (expert)
- Each secondary skill can be explicitly specified or randomly selected
- Negative values can be used to downgrade or remove secondary skills from hero

```json
"secondary": [
    {
        // Specific skill
        "type" : "wisdom",
        "amount" : 1
    },
    {
        // Skill will be selected randomly from the list
        "anyOf" : ["airMagic", "waterMagic", "earthMagic", "fireMagic"],
        "min" : 1,
        "max" : 3
    },
    {
        // Skill will be selected randomly from all allowed, expect those
        "noneOf" : ["necromancy", "leadership"],
        "amount" : 1
    },
    {
        // Skill will be selected randomly from all allowed
        "amount" : 3
    }
]
```

### Can learn skills

- Can be used as limiter. Hero must have free skill slot to pass limiter

```json
    "canLearnSkills" : true
```

### Bonus System

- Can be used as reward, to grant bonus to visiting hero
- if present, MORALE and LUCK bonus will add corresponding image component to UI.
- Note that unlike most values, parameter of bonuses can NOT be randomized
- Description can be string or number of corresponding string from `arraytxt.txt`

```json
"bonuses" : [
    {
        "type" : "MORALE", 
        "val" : 1, 
        "duration" : "ONE_BATTLE", 
        "description" : 94 
    }
]
```

- Can be used as a reward, to grant bonuses to commander of a visiting hero
- This option will have no effect if commander is not present, for example for players without WoG mod
- Granting bonuses to dead commander is possible. use `commanderAlive` limiter if you want to avoid this
- Syntax identical to regular `bonuses` field

```json
"commanderBonuses" : [ ]
```

- Can be used as a reward, to grant bonuses to player that commands visiting hero
- Bonuses will remain even if hero is lost or fired. Use `bonuses` with `PLAYER_PROPAGATOR` if you wish for bonus to be removed when hero is lost
- Syntax identical to regular `bonuses` field

```json
"playerBonuses" : [ ]
```

### Artifact Slots

- Can be used as limiter, hero must have listed slots empty to pass the limiter
- Slots occupied by components of combined artifacts are considered to be full

```json
"availableSlots": [
    "LEFT_HAND",
    "RIGHT_HAND"
],
```

- Can be used as a reward, to remove artifact that is present in slot
- Components of a combined artifact can not be removed in this way

```json
"takenArtifactSlots" : [
	"LEFT_HAND"
],
```

List of supported slots names:

- `HEAD` - helmet slot
- `SHOULDERS` - slot used by capes
- `NECK` - slot used by neclaces artifacts
- `RIGHT_HAND` - slot for weapons / swords
- `LEFT_HAND` - slot for shields
- `TORSO` - chest slot for armors
- `RIGHT_RING` - ring slot located next to weapon slot
- `LEFT_RING` - ring slot located next to shield slot
- `FEET` - slot for boots
- `MISC1` - top-most miscellaneous slot
- `MISC2` - 2nd from top miscellaneous slot
- `MISC3` - 3rd from top miscellaneous slot
- `MISC4` - bottom-right miscellaneous slot
- `MISC5` - bottom-left miscellaneous slot
- `SPELLBOOK` - Hero's spellbook. Its removal is untested and may lead to unintended side effects
- `MACH1` - Ballista, or alternative war machine from mod
- `MACH2` - Ammo Cart, or alternative war machine from mod
- `MACH3` - First Aid tent, or alternative war machine from mod
- `MACH4` - Catapult slot. Its removal is untested and may lead to unintended side effects

### Artifacts

- Can be used as limiter, hero must have artifact either equipped or in backpack
- Can be used as reward, to give new artifact to a hero
- Artifacts added as reward will be used for text substitution. First `%s` in text string will be replaced with name of an artifact

```json
"artifacts": [
    "ribCage"
],
```

- Alternative format, random artifact generation.
- For artifact class possible values are "TREASURE", "MINOR", "MAJOR", "RELIC"
- Artifact value range can be specified with min value and max value

```json
"artifacts": [
    {
        "class" : "TREASURE",
        "minValue" : 5000,
        "maxValue" : 10000
    }
],
```

### Taking Artifacts

- Can be used as reward, to take artifact from hero
- Taking part of a combined artifact would disassemble it
- Artifacts can be taken from either equipment or from backpack slots
- Format is identical to [Artifacts](#artifacts)

```json
"takenArtifacts": [
    "ribCage"
],
```

### Scrolls

- Can be used as limiter, hero must have scroll either equipped or in backpack
- Can be used as reward, to give new scroll to a hero
- Format is identical to [Spells](#spells)

```json
"scrolls": [
    "magicArrow"
],
```

### Taking Scrolls

- Can be used as reward, to take scroll from hero
- Format is identical to [Spells](#spells)

```json
"takenScrolls": [
    "magicArrow"
],
```

### Spells

- Can be used as limiter
- Can be used as reward, to give new spell to a hero
- Spells added as reward will be used for text substitution. First `%s` in text string will be replaced with spell name

```json
"spells": [
    "magicArrow"
],
```

- Alternative format, random spell selection
- Spell can be selected from specifically selected school

```json
"spells": [
    {
        "level" : 1,
        "school" : "fire",
    }
],
```

### Can learn spells

- Can be used as limiter. Hero must be able to learn spell to pass the limiter
- Hero is considered to not able to learn the spell if:
- - he already has specified spell
- - he does not have a spellbook
- - he does not have sufficient Wisdom level for this spell

```json
    "canLearnSpells" : [
        "magicArrow"
],
```

### Creatures

- Can be used as limiter
- Can be used as reward, to give new creatures to a hero
- For map objects, if hero does not have enough free slots, game will show selection dialog to pick troops to keep
- For town buildings, hero must either have free slot(s) to take them, or have creatures of this type. Othervice reward would fail to give any creatures
- It is possible to specify probability to receive upgraded creature

```json
"creatures" : [
    {
        "type" : "archer",
        "upgradeChance" : 30,
        "amount" : 20,
    }
],
```

- Additionally, it is possible to check whether tested troops can be removed, in other words - whether hero has any creatures other than those specified in `creatures` field using `hasExtraCreatures` key
- Following check will pass only if hero either has more archers than 20, or has exactly 20 archers and any creatures other than that

```json
"creatures" : [
    {
        "type" : "archer",
        "amount" : 20,
    }
],
"hasExtraCreatures" : true
```

### Taking Creatures

- Can be used as reward, to take creatures from a hero
- Attempting to take more creatures than hero has, or creatures that hero does not have is legal
- It is not possible to take entire army, however it is possible to replace entire army by providing new creatures.
- Format is identical to [Creatures](#creatures)

```json
"takenCreatures" : [
    {
        "type" : "archer",
        "amount" : 20,
    }
],
```

### Creature receiving test

- Can be used as limiter, to test whether hero can accept specified creatures without abandoning any units
- Check will pass if hero has enough free slots to accept creatures, or if hero already has specified creatures, or if hero has units that can be merged to create space for new troops
- Note that attempting to give more troops than hero can accept is legal, and will show unit selection dialog on which hero will have to abandon some of his creatures

```json
"canReceiveCreatures" : [
    {
        "type" : "archer",
        "amount" : 20,
    }
],
```

### Guards

- When used in a reward, these creatures will be added to guards of the objects
- Hero must defeat all guards before being able to receive rewards
- Guards are only reset when object rewards are reset
- Requires `guardsLayout` property to be set in main part of object configuration
- It is possible to add up to 7 slots of creatures
- Guards of the same creature type will never merge or rearrange their stacks

```json
"guards" : [
    { "type" : "archer", "amount" : 20 },
    { "type" : "archer", "amount" : 20, "upgradeChance" : 30 },
    { "type" : "archer", "amount" : 20 }
],
```

### Creatures Change

- Can NOT be used as limiter
- Can be used as reward, to replace creatures in hero army. It is possible to use this parameter both for upgrades of creatures as well as for changing them into completely unrelated creature, e.g. similar to Skeleton Transformer
- This parameter will not change creatures given by `creatures` parameter on the same visit

```json
"changeCreatures" : {
    "cavalier" : "champion"
}
```

### Spell cast

- Can NOT be used as limiter
- As reward, instantly casts adventure map spell for visiting hero. All checks for spell book, wisdom or presence of mana will be ignored. It's possible to specify school level at which spell will be casted. If it's necessary to reduce player's mana or do some checks, they shall be introduced as limiters and other rewards
- School level possible values: 1 (basic), 2 (advanced), 3 (expert)

```json
"spellCast" : {
    "spell" : "townPortal",
    "schoolLevel": 3
}
```

### Fog of War

- Can NOT be used as limiter
- Can be used as reward, to reveal or hide affected tiles
- If radius is not specified, then all matching tiles on the map will be affected
- It is possible to specify which terrain classes should be affected. Tile will be affected if sum of values its classes is positive. For example, `"water" : 1` will affect all water tiles, while `"surface" : 1, "subterra" : -1` will include terrains that have "surface" flag but do not have "subterra" flag
- If 'hide' is set to true, then instead of revealing terrain, game will hide affected tiles for all other players

```json
"revealTiles" : {
	"radius" : 20,
	"surface" : 1,
	"subterra" : 1,
	"water" : 1,
	"rock" : 1,
	"hide" : true
}
```

### Player color

- Can be used as limiter
- Can NOT be used as reward
- Only players with specific color can pass the limiter

```json
"colors" : [ "red", "blue", "tan", "green", "orange", "purple", "teal", "pink" ]
```

### Hero types

- Can be used as limiter
- Can NOT be used as reward
- Only specific heroes can pass the limiter

```json
"heroes" : [ "orrin" ]
```

### Hero classes

- Can be used as limiter
- Can NOT be used as reward
- Only heroes belonging to specific classes can pass the limiter

```json
"heroClasses" : [ "battlemage" ]
```
