< [Documentation](../../Readme.md) / [Modding](../Readme.md) / Entities Format / Battlefield Format

```jsonc
	// Human-readable name of the battlefield
	"name" : ""
	
	// If set to true, obstacles will be taken from "specialBattlefields" property of an obstacle
	// If set to false, obstacles will be taken from "allowedTerrains" instead
	"isSpecial" : false
	
	// List of bonuses that will affect all battles on this battlefield
	"bonuses" : { BONUS_FORMAT }
	
	// Background image for this battlefield
	"graphics" : ""
	
	// List of battle hexes that will be always blocked on this battlefield (e.g. ship to ship battles)
	"impassableHexes" : [ 10, 20, 50 ]
```