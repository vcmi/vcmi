# Biome Format

## General description

Biome is a new entity type added in VCMI 1.5.0. It defines a set of random map obstacles which will be generated together. For each zone different obstacle sets is randomized and then only obstacles from that set will be used to fill this zone.

The purpose is to create visually attractive and consistent maps, which will also vary between generations. It is advised to define a biome for a group of objects that look similar and just fit each other visually.

If not enough biomes are defined for [terrain type](Terrain_Format.md), map generator will fall back to using all available templates that match this terrain, which was original behavior before 1.5.0.

``` json
"obstacleSetId" : {
	"biome" : {
		"terrain" : "grass", // Id or vector of Ids this obstacle set can spawn at
		"level" : "underground", // or "surface", by default both
		"faction" : ["castle", "rampart"], //Id or vector of faction Ids. Set will only be used if zone belongs to this faction
		"alignment" : ["good", "evil", "neutral"], //Alignment of the zone. Set will only be used if zone has this alignment
		"objectType": "mountain"
		},
	"templates" : ["AVLmtgr1", "AVLmtgr2", "AVLmtgr3", "AVLmtgr4", "AVLmtgr5", "AVLmtgr6"] // List of template Ids taken from original game files, or from mods
}
```

### List of possible types

Possible object types: `mountain`, `tree`, `lake`, `crater`, `rock`, `plant`, `structure`, `animal`, `other`. Use your best judgement, ie. mushrooms are classified as plants and animal bones are classified as animals.

Template Ids are keys used in [Map Objects](../Map_Object_Format.md)

### Algorithm

Currently algorithm picks randomly:

- One set of **mountains** (large objects)
- One or two set of **trees** (large objects)
- A set of **lakes** or **craters** - never both (large objects)
- One or two sets of **plants** (small objects)
- One or two sets of **rocks** (small objects)
- One of each remaining types of object (**structure**, **animal**, **other**), until enough number of sets is picked.
- Obstacles marked as **other** are picked last, and are generally rare.


