# River Format

## Format

```jsonc
"newRiver" :
{
	// Two-letters unique identifier for this river. Used in map format
	"shortIdentifier" : "mr",
	
	// Human-readable name of the river
	"text" : "My Road",
	
	// Name of file with river graphics
	"tilesFilename" : "myRiver.def"
	
	// Name of file with river delta graphics
	// TODO: describe how exactly to use this tricky field
	"delta" : "",
	
	// If defined, river will be animated using palette color cycling effect
	// Game will cycle "length" colors starting from "start" (zero-based index) on each animation update every 180ms
	// Color numbering uses palette color indexes, as seen in image editor
	// Note that some tools for working with .def files may reorder palette. 
	// To avoid this, it is possible to use json with indexed png images instead of def files
	"paletteAnimation" : [
		{ "start" : 10, "length" : 5 },
		...
	]
}
```