# Secondary Skill Format

## Main format

```json
{
	// Skill be only be available on maps with water
	"onlyOnWaterMap" : false,
	// Skill is not available on maps at random
	"special" : true,
    // Skill is not available on map objects: such as Witch Hut, University and Scholars
    "banInMapObjects" : true
}
```

```json
{
	"skillName":
	{
		//Mandatory, localizable skill name
		"name":     "Localizable name",

		// Optional base format, will be merged with basic/advanced/expert
		"base":     {Skill level base format},

		// Configuration for different skill levels
		"basic":    {Skill level format},
		"advanced": {Skill level format},
		"expert":   {Skill level format},
		
		// Names of bonuses of the skill that are affected by default secondary skill specialty of a hero
		"specialty" : [
			"main"
		],
		
		// Chance for the skill to be offered on level-up (heroClass may override)
		/// Identifier without modID specifier MUST exist in base game or in one of dependencies
		/// Identifier with explicit modID specifier will be silently skipped if corresponding mod is not loaded
		"gainChance" : {
			// Chance for hero classes with might affinity
			"might" : 4,
			// Chance for hero classes with magic affinity
			"magic" : 6,
			// Chance for specific classes
			"knight" : 2,
			"cleric" : 8,
			...
			"modName:heroClassName" : 5
		},
		
		// This skill is major obligatory (like H3 Wisdom) and is guaranteed to be offered once per specific number of levels
		"obligatoryMajor" : false,
		
		// This skill is minor obligatory (like H3 Magic school) and is guaranteed to be offered once per specific number of levels
		"obligatoryMinor" : false,
	}
}
```

## Skill level base format

Json object with data common for all levels can be put here. These
configuration parameters will be default for all levels. All mandatory
level fields become optional if they equal "base" configuration.

## Skill level format

```json
{
	// Localizable description
	// Use {xxx} for formatting
	"description": "",

	// Bonuses provided by skill at given level
	// If different levels provide same bonus with different val, only the highest applies
	"effects":
	{
		"firstEffect":  {bonus format},
		"secondEffect": {bonus format}
		//...
	},
	
	// Skill icons of varying size
	"images" : {
		// 32x32 skill icon
		"small" : "",
		// 44x44 skill icon
		"medium" : "",
		// 82x93 skill icon
		"large" : "",
		// 58x64 skill icon for campaign scenario bonus
		"scenarioBonus" : ""
	}
}
```

## Example

The following modifies the tactics skill to grant an additional speed
boost at advanced and expert levels.

```json
"core:tactics" : {
	"base" : {
		"effects" : {
			"main" : {
				"subtype" : "skill.tactics",
				"type" : "SECONDARY_SKILL_PREMY",
				"valueType" : "BASE_NUMBER"
			},
			"xtra" : {
				"type" : "STACKS_SPEED",
				"valueType" : "BASE_NUMBER"
			}
		}
	},
	"basic" : {
		"effects" : {
			"main" : { "val" : 3 },
			"xtra" : { "val" : 0 }
		}
	},
	"advanced" : {
		"description" : "{Advanced Tactics}\n\nAllows you to rearrange troups within 5 hex rows, and increases their speed by 1.",
		"effects" : {
			"main" : { "val" : 5 },
			"xtra" : { "val" : 1 }
		}
	},
	"expert" : {
		"description" : "{Expert Tactics}\n\nAllows you to rearrange troups within 7 hex rows, and increases their speed by 2.",
		"effects" : {
			"main" : { "val" : 7 },
			"xtra" : { "val" : 2 }
		}
	}
}
```
