< [Documentation](../../Readme.md) / [Modding](../Readme.md) / Entities Format / Road Format

## Format

```jsonc
"newRoad" :
{
	// Two-letters unique indentifier for this road. Used in map format
	"shortIdentifier" : "mr",
	
	// Human-readable name of the road
	"text" : "My Road",
	
	// Name of file with road graphics
	"tilesFilename" : "myRoad.def"
	
	// How many movement points needed to move hero
	"moveCost" : 66
}
```