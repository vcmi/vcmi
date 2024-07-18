# Battle Obstacle Format

```jsonc
	// List of terrains on which this obstacle can be used
	"allowedTerrains" : []
	
	// List of special battlefields on which this obstacle can be used
	"specialBattlefields" : []
	
	// If set to true, this obstacle will use absolute coordinates. Only one such obstacle can appear on the battlefield
	"absolute" : false
	
	// Width of an obstacle, in hexes
	"width" : 1
	
	// Height of an obstacle, in hexes
	"height" : 1
	
	// List of tiles blocked by an obstacles. For non-absolute obstacles uses relative hex indices
	"blockedTiles" : [ 0, 20, 50 ]
	
	// For absolute obstacle - image with static obstacle. For non-absolute - animation with an obstacle
	"animation" : "",
	
	// If set to true, obstacle will appear in front of units or other battlefield objects
	"foreground" : false
```