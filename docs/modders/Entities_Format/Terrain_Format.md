# Terrain Format

## Format

```jsonc
"newTerrain" :
{
	// Two-letters unique identifier for this terrain. Used in map format
	"shortIdentifier" : "mt",
	
	// Human-readable name of the terrain
	"text" : "My Road",
	
	// Type(s) of this terrain.
	// WATER - this terrain is water-like terrains that requires boat for movement
	// ROCK - this terrain is unpassable "rock" terrain that is used for inaccessible parts of underground layer
	// SUB - this terrain can be placed in underground map layer by RMG
	// SURFACE - this terrain can be placed in surface map layer by RMG
	"type" : [ "WATER", "SUB", "ROCK", "SURFACE" ],
	
	// Name of file with road graphics
	"tiles" : "myRoad.def",
	
	// How many movement points needed to move hero on this terrain
	"moveCost" : 150,
	
	// The name of rock type terrain which will be used as borders in the underground
	// By default, H3 terrain "rock" will be used
	"rockTerrain" : "rock",
	
	// River type which should be used for that terrain
	"river" : "",
	
	// If defined, terrain will be animated using palette color cycling effect
	// Game will cycle "length" colors starting from "start" (zero-based index) on each animation update every 180ms
	// Color numbering uses palette color indexes, as seen in image editor
	// Note that some tools for working with .def files may reorder palette. 
	// To avoid this, it is possible to use json with indexed png images instead of def files
	"paletteAnimation" : [
		{ "start" : 10, "length" : 5 },
		...
	],
	
	// List of battleFields that can be used on this terrain
	"battleFields" : [ ]
	
	// Color of terrain on minimap without unpassable objects. RGB triplet, 0-255 range
	"minimapUnblocked" : [ 150, 100, 50 ],
	
	// Color of terrain on minimap with unpassable objects. RGB triplet, 0-255 range
	"minimapBlocked" : [ 150, 100, 50 ],
	
	// List of music files to play on this terrain on adventure map. At least one file is required
	"music" : [ "" ],
	
	"sounds" : {
		// List of ambient sounds for this terrain
		"ambient" : [ "" ]
	},
	
	// Hero movement sound for this terrain, version for moving on tiles with road
	"horseSound" : "",
	
	// Hero movement sound for this terrain, version for moving on tiles without road
	"horseSoundPenalty" : "",
	
	// List or terrain names, which is prohibited to make transition from/to
	"prohibitTransitions" : [ "" ],
	
	// If sand/dirt transition required from/to other terrains
	"transitionRequired" : false,
	
	// Represents layout of tile orientations in terrain tiles file
	// Can be normal, dirt, water, rock, or hota
	"terrainViewPatterns" : "",
	
}
```