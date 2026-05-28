# Battle Obstacle Format

## Configuration reference

```json
	// List of terrains on which this obstacle can be used
	
	"allowedTerrains" : [
		"dirt",
		"sand",
		"asphalt", // terrain from mod, such form can only be used if mod has dependency on mod with such terrain
		"hota:wasteland", // terrain from mod, such form can be used even without dependency, entry will be ignored if such mod does not exists
	]
	
	// List of special battlefields on which this obstacle can be used
	"specialBattlefields" : []
	
	// If set to true, this obstacle will use absolute coordinates. Only one such obstacle can appear on the battlefield
	"absolute" : false
	
	// Width of an obstacle, in hexes
	// Only affects how far on the right side of the battlefield this obstacle can be placed
	// make sure that this number matches maximum width of blocked tiles 
	"width" : 1
	
	// Height of an obstacle, in hexes. It determines:
	// 1. Maximum height on a battlefield at which obstacle can be placed
	// 2. Image positioning (see below)
	"height" : 1
	
	// List of tiles blocked by an obstacles. See below for description
	"blockedTiles" : [ 0, 20, 50 ]
	
	// For absolute obstacle - image with static obstacle. For non-absolute - animation with an obstacle
	"animation" : "",
	
	// If set to true, obstacle will appear in front of units or other battlefield objects
	"foreground" : false
```

## Obstacle image positioning

Obstacle image will always be placed in the same way:

- left side of obstacle image is aligned with left side of hex on the bottom row of hexes
- top side of image is aligned with top of the hex in topmost row of hexes (based on height of the obstacle)  

## Blocked tiles definition

How blocked tiles are defined depends on whether obstacle is `absolute` or not:

### Non-absolute obstacles

Non-absolute obstacles specify their coordinates relative to bottom-left corner of obstacle. If you wish to have obstacle that takes multiple rows, substracting 17 from hex number would block tile directly above bottom-left corner of your obstacle.

For example, obstacle that blocks tiles `[1, 2, 3, -14, -15, -31]` would result in following layout on the battlefield:

![Battlefield Relative Obstacle Example](../../images/Battle_Field_Relative_Obstacle.svg)

### Absolute obstacles

Absolute obstacles operate in absolute coordinates. Because of that, blocked tiles contains list of indexes of blocked tiles. For reference on tiles indexes see image below:

![Battlefield Hexes Layout](../../images/Battle_Field_Hexes.svg)

## Troubleshooting

To help with tracking down problems with image positioning or blocked tiles, you can use `obstacles debug` command. To use it, enter any combat in game, open chat (for example by pressing tab) and enter `/obstacles debug` in chat.

This would export all images into `<User cache directory>/extracted/obstacles` (see Launcher -> Help for actual path on your system)

This directory will contain images of all non-absolute obstaclese loaded in game with additional debug info:

- hex grid placed over your image (size of hex grid matches width / height of your obstacle)
- shaded tiles that are blocked by your obstacle
- boundaries of your image

This command can also be used to quickly check all obstacles added by your mod instead of relying on random rolls and reentering multiple combats to locate all added obstacles.\
