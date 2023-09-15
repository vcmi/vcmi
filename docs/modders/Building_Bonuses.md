< [Documentation](../Readme.md) / [Modding](Readme.md) / Building Bonuses

Work-in-progress page do describe all bonuses provided by town buildings
for future configuration.

## unique buildings

Hardcoded functionalities, selectable but not configurable. In future
should be moved to scripting.

Includes:

-   mystic pond
-   treasury
-   god of fire
-   castle gates
-   cover of darkness
-   portal of summoning
-   escape tunnel

Function of all of these objects can be enabled by this:

``` javascript
 "function" : "castleGates" 
```

## trade-related

Hardcoded functionality for now due to complexity of these objects.
Temporary can be handles as unique buildings. Includes:

-   resource - resource
-   resource - player
-   artifact - resource
-   resource - artifact
-   creature - resource
-   resource - skills
-   creature - skeleton

## hero visitables

Buildings that give one or another bonus to visiting hero. All should be
handled via configurable objects system.

Includes:

-   gives mana points
-   gives movement points
-   give bonus to visitor
-   permanent bonus to hero

## generic functions

Generic town-specific functions that can be implemented as part of
CBuilding class.

### unlock guild level

``` javascript
 "guildLevels" : 1 
```

### unlock hero recruitment

``` javascript
 "allowsHeroPurchase" : true 
```

### unlock ship purchase

``` javascript
 "allowsShipPurchase" : true 
```

### unlock building purchase

``` javascript
 "allowsBuildingPurchase" : true 
```

### unlocks creatures

``` javascript
 "dwelling" : { "level" : 1, "creature" : "archer" } 
```

### creature growth bonus

Turn into town bonus? What about creature-specific bonuses from hordes?

### gives resources

``` javascript
 "provides" : { "gold" : 500 } 
```

### gives guild spells

``` javascript
 "guildSpells" : [5, 0, 0, 0, 0] 
```

### gives thieves guild

``` javascript
 "thievesGuildLevels" : 1 
```

### gives fortifications

``` javascript
 "fortificationLevels" : 1 
```

### gives war machine

``` javascript
 "warMachine" : "ballista" 
```

## simple bonuses

Bonuses that can be made part of CBuilding. Note that due to how bonus
system works this bonuses won't be stackable.

TODO: how to handle stackable bonuses like Necromancy Amplifier?

Includes:

-   bonus to defender
-   bonus to alliance
-   bonus to scouting range
-   bonus to player

``` javascript
"bonuses" :
{
	"moraleToDefenders" :
	{
		"type": "MORALE",
		"val" : 1,
		"propagator" : ["VISITED_TOWN_AND_VISITOR"]
	},
	"luckToTeam" :
	{
		"type" : "LUCK",
		"val" : 2,
		"propagator" : [ "TEAM_PROPAGATOR" ]
	}
```

## misc

Some other properties of town building that does not fall under "bonus"
category.

### unique building

Possible issue - with removing of fixed ID's buildings in different town
may no longer share same ID. However Capitol must be unique across all
town. Should be fixed somehow.

``` javascript
 "onePerPlayer" : true 
```

### chance to be built on start

``` javascript
 "prebuiltChance" : 75 
```