< [Documentation](../Readme.md) / [Modding](Readme.md) / Biome Format

All parameters but templates are optional.

```javascript
{
	"objectGroupName": {
		"biome" : {
			// Type of terrain or vector of terrains. Can't be empty.
			"terrain" : "grass",
			// Faction or vector of factions. If empty, all factions are allowed
			"faction" : "rampart",
			// Alignment or vector of alignments of factions. If empty, all alignments are allowed
			"alignment" : "good"
		},
		"templates" : {
			["object1, object2, object3"]
		}
	}
}
```

