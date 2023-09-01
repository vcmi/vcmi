# Creating mod

To make your own mod you need to create subdirectory in **<data dir>/Mods/** with name that will be used as identifier for your mod.
Main mod is file called **mod.json** and should be placed into main folder of your mod, e.g. **Mods/myMod/mod.json**
All content of your mod should go into **Content** directory, e.g. **Mods/myMod/Content/**. Alternatively, it is possible to replace this directory with single .zip archive.

Example of how directory structure of your mod may look like:

```
    Mods/
        myMod/
            mod.json
            Content/
                config/  - json configuration files
                data/    - unorganized files, mostly bitmap images (.bmp, .png, .pcx)
                maps/    - h3m maps added or modified by mod
                music/   - music files. Mp3 and ogg/vorbis are supported
                sounds/  - sound files, in wav format.
                sprites/ - animation, image sets (H3 .def files or VCMI .json files)
                video/   - video files, .bik or .smk
```

## Creating mod file

All VCMI configuration files use [JSON format](http://en.wikipedia.org/wiki/Json) so you may want to familiarize yourself with it first.
Mod.json is main file in your mod and must be present in any mod. This file contains basic description of your mod, dependencies or conflicting mods (if present), list of new content and so on.
Minimalistic version of this file:

``` javascript
{
    "name" : "My test mod",
    "description" : "My test mod that add a lot of useless stuff into the game"
}
```

See [Mod file Format](Mod_File_Format.md) for its full description.

## Creation of new objects

In order to create new object use following steps:
1. Create json file with definition of new object. See list of supported object types below.
2. Add any resources needed for this object, such as images, animations or sounds.
2. Add reference to new object in corresponding section of mod.json file

### List of supported new object types

- [Random Map Template](Entities_Format/Random_Map_Template.md)

- [Artifact](Entities_Format/Artifact_Format.md)
- [Creature](Entities_Format/Creature_Format.md)
- [Faction](Entities_Format/Faction_Format.md)
- [Hero Class](Entities_Format/Hero_Classes_Format.md)
- [Hero Type](Entities_Format/Hero_Type_Format.md)
- [Spell](Entities_Format/Spell_Format.md)
- [Secondary Skill](Entities_Format/Object_Format.md)

- [Map Objects](Entities_Format/Map_Object_Format.md)
- - [Rewardable](Map_Objects/Rewardable.md)
- - [Creature Bank](Map_Objects/Creature_Bank.md)
- - [Dwelling](Map_Objects/Dwelling.md)
- - [Market](Map_Objects/Markets.md)
- - [Boat](Map_Objects/Boat.md)

- [Terrain](Entities_Format/Terrain_Format.md)
- [River](Entities_Format/River_Format.md)
- [Road](Entities_Format/Road_Format.md)
- [Battlefield](Entities_Format/Battlefield_Format.md)
- [Battle Obstacle](Entities_Format/Battle_Obstacle_Format.md)

## Game Identifiers system

VCMI uses strings to reference objects. Examples:

- Referencing H3 objects: `"nativeTerrain" : "sand"`. 
Note: All mods can freely access any existing objects from H3 data.

- Referencing object from another mod: `"nativeTerrain" : "asphalt"`
Note: Mods can only reference object from mods that are marked as dependencies

- Referencing objects in bonus system: `"subtype" : "creature.archer"`
Note: Bonus system requires explicit definition of object type since different bonuses may require different identifier class.

- Referencing object from specific mod: `"nativeTerrain" : "hota.cove:sorceress"`
Note: In some cases, for example to resolve conflicts when multiple mods use same object name you might need to explicitly specify mod in which game needs to look up an identifier. Alternatively, you can use this form for clarity if you want to clearly specify that object comes from another mod.

### Modifying existing objects

Alternatively to creating new objects, you can edit existing objects. Normally, when creating new objects you specify object name as:
``` javascript
"newCreature" : {
    // creature parameters
}
```

In order to access and modify existing object you need to specify mod that you wish to edit:

``` javascript
/// "core" specifier refers to objects that exist in H3
"core:archer" : {
	/// This will set health of Archer to 10
	"hitPoints" : 10,
},

/// Modifying object named "jumpSoldier" in mod "forge"
"forge:jumpSoldier" : {
	/// Set attack of Jump Soldiers to 20
	"attack": 20
},

/// Modifying object named "sorceress" in submod "cove" of mod "hota"
"hota.cove:sorceress" : {
	/// Set speed of Sorceresses to 10
	"speed" : 10
},
```
Note that modification of existing objects does not requires a dependency on edited mod. Such definitions will only be used by game if corresponding mod is installed and active.

This allows using objects editing not just for rebalancing mods but also to provide compatibility between two different mods or to add interaction between two mods.

## Overriding graphical files from Heroes III

Any graphical replacer mods fall under this category. In VCMI directory **<mod name>/Content** acts as mod-specific game root directory. So for example file **<mod name>/Content/Data/AISHIELD.PNG** will replace file with same name from **H3Bitmap.lod** game archive.
Any other files can be replaced in exactly same way.
Note that replacing files from archives requires placing them into specific location:
- H3Bitmap.lod -> Data
- H3Sprite.lod -> Sprites
- Heroes3.snd  -> Sounds
- Video.vid    -> Video

This includes archives added by expansions (e.g. **H3ab_bmp.lod** uses same rules as **H3Bitmap.lod**)

- **Warning:** overriding graphical files from other mods is discouraged and may be removed in future versions of vcmi. Please use modification of existing objects instead.

### Replacing .def animation files

Heroes III uses custom format for storing animation: def files. These files are used to store all in-game animations as well as for some GUI elements like buttons and for icon sets.
These files can be replaced by another def file but in some cases original format can't be used. This includes but not limited to:
-   Replacing one (or several) icons in set
-   Replacing animation with fully-colored 32-bit images
In VCMI these animation files can also be replaced by json description of their content. See [Animation Format](Animation_Format.md) for full description of this format.
Example: replacing single icon

``` javascript
{
	// List of replaced images
	"images" :
	[
		{
			"frame" : 0, // Index of replaced frame
			"file" : "HPS000KN.bmp" //name of file that will be used as replacement
		}
	]
}
```

# Publishing mods in VCMI Repository

This will allow players to install mods directly from VCMI Launcher without visiting any 3rd-party sites.

## Where files are hosted

Mods list hosted under main VCMI organization: [vcmi-mods-repository](https://github.com/vcmi/vcmi-mods-repository).
Each mod hosted in it's own repository under separate organization [vcmi-mods](https://github.com/vcmi-mods). This way if engine become more popular in future we can create separate teams for each mod and accept as many people as needed.

## Why Git / GitHub?

It's solve a lot of problems:

- Engine developers get control over all mods and can easily update them without adding extra burden for modders / mod maintainers.
- With tools such as [GitHub Desktop](https://desktop.github.com/) it's easy for non-programmers to contribute.
- Forward and backward compatibility. Stable releases of game use compatible version of mods while users of daily builds will be able to test mods supporting bleeding edge features.
- Tracking of changes for repository and mods. It's not big deal now, but once we have scripting it's will be important to keep control over what code included in mods.
- GitHub also create ZIP archives for us so mods will be stored uncompressed and version can be identified by commit hash.

## On backward compatibility

Our mod list in vcmi-mods-repository had "develop" as primary branch. Daily builds of VCMI use mod list file from this branch.
Once VCMI get stable release there will be branching into "1.0.0", "1.1.0", etc. Launcher of released version will request mod list for particular version.
Same way we can also create special stable branch for every mod under "vcmi-mods" organization umbrella once new stable version is released. So this way it's will be easier to maintain two versions of same mod: for stable and latest version.

## Getting into vcmi-mods organization

Before your mod can be accepted into official mod list you need to get it into repository under "vcmi-mods" organization umbrella. To do this contact one of mod repository maintainers. If needed you can get own team within "vcmi-mods" organization.
Link to our mod will looks like that: https://github.com/vcmi-mods/adventure-ai-trace

## Rules of repository

### Allowed name for mod identifier

For sanity reasons mod identifier must only contain lower-case English characters, numbers and hyphens.

    my-mod-name
    2000-new-maps

Sub-mods can be named as you like, but we strongly encourage everyone to use proper identifiers for them as well.

### Rewriting History

Once you submitted certain commit into official mod list you are not allowed to rewrite history before that commit. This way we can make sure that VCMI launcher will always be able to download older version of any mod.
Branches such as "develop" or stable branches like "1.0.0" should be marked as protected on GitHub.

## Submitting mods to repository

Once mod ready for general public maintainer to make PR to [vcmi-mods-repository](https://github.com/vcmi/vcmi-mods-repository).

## Requirements

Right now main requirements for a mod to be accepted into VCMI mods list are:

- Mod must be complete. For work-in-progress mods it is better to use other way of distribution.
- Mod must met some basic quality requirements. Having high-quality content is always preferable.
- Mod must not contain any errors detectable by validation (console message you may see during loading)
- Music files must be in Ogg/Vorbis format (\*.ogg extension)
