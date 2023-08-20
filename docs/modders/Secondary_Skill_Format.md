## Main format

``` javascript
{
    "skillName":
    {
        //numeric id of skill required only for original skills, prohibited for new skills
        "index":    0,
        //Mandatory
        "name":     "Localizable name",
        //optional base format, will be merged with basic/advanced/expert
        "base":     {Skill level base format},
        //configuration for different skill levels
        "basic":    {Skill level format},
        "advanced": {Skill level format},
        "expert":   {Skill level format}
    }
}
```

## Skill level base format

Json object with data common for all levels can be put here. These
configuration parameters will be default for all levels. All mandatory
level fields become optional if they equal "base" configuration.

## Skill level format

``` javascript
{
    //Optional, localizable description
    //Use {xxx} for formatting
    "description": "",
    //Bonuses provided by skill at given level
    //If different levels provide same bonus with different val, only the highest applies
    "effects":
    {
        "firstEffect":  {bonus format},
        "secondEffect": {bonus format}
        //...
    }
}
```

## Example

The following modifies the tactics skill to grant an additional speed
boost at advanced and expert levels.

``` javascript
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