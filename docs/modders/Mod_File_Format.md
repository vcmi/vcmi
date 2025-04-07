# Mod File Format

## Fields with description of mod

```json
{
	// Name of your mod. While it does not have hard length limit
	// it should not be longer than ~30 symbols to fit into allowed space
	"name" : "My test mod",

	// More lengthy description of mod. No hard limit. This text will be visible in launcher.
	// This field can use small subset of HTML, see link at the bottom of this page.
	"description" : "My test mod that add a lot of useless stuff into the game",

	// Author of mod. Can be nickname, real name or name of team
	"author" : "Anonymous",

	// Full name of license used by mod. Should be set only if you're author of mod
	// or received permission to use such license from original author
	"licenseName" : "Creative Commons Attribution-ShareAlike",

	// URL which user can use to see license terms and text
	"licenseURL" : "https://creativecommons.org/licenses/by-sa/4.0/",

	
	// Home page of mod or link to forum thread to contact the author
	"contact" : "http://example.com",
	
	// Current mod version, up to 3 numbers, dot-separated. Format: A.B.C
	"version" : "1.2.3"

	// Type of mod, list of all possible values:
	// "Translation", "Town", "Test", "Templates", "Spells", "Music", "Maps", "Sounds", "Skills", "Other", "Objects", 
	// "Mechanics", "Interface", "Heroes", "Graphical", "Expansion", "Creatures", "Compatibility", "Campaigns", "Artifacts", "AI"
	//
	// Some mod types have additional effects on your mod:
	// Translation: mod of this type is only active if player uses base language of this mod. See "language" property. 
	// Additionally, if such type is used for submod it will be hidden in UI and automatically activated if player uses base language of this mod. This allows to provide locale-specific resources for a mod
	// Compatibility: mods of this type are hidden in UI and will be automatically activated if all mod dependencies are active. Intended to be used to provide compatibility patches between mods
	"modType" : "Graphical",
	
	// Base language of the mod, before applying localizations. By default vcmi assumes English
	// This property is mostly needed for translation mods only
	"language" : "english"

	// List of mods that are required to run this one
	"depends" :
	[
		"baseMod"
	],
	
	// List of mods if they are enabled, should be loaded before this one. This mod will overwrite any conflicting items from its soft dependency mods.
	"softDepends" :
	[
		"baseMod"
	],
 
	// List of mods that can't be enabled in the same time as this one
	"conflicts" :
	[
		"badMod"
	],
	
	// Supported versions of vcmi engine
	"compatibility" : {
		"min" : "1.2.0"
		"max" : "1.3.0"
	}

	//List of changes/new features in each version
	"changelog" :
	{
		"1.0"   : [ "initial release" ],
		"1.0.1" : [ "change 1", "change 2" ],
		"1.1"   : [ "change 3", "change 4" ]
	},

	// If set to true, mod will not be enabled automatically on install
	"keepDisabled" : false
	
	// List of game settings changed by a mod. See <VCMI Install>/config/gameConfig.json for reference
	"settings" : {
		"combat" : {
			"goodLuckDice" : [] // disable luck
		}
	}
}
```

## Fields with description of mod content

These are fields that are present only in local mod.json file

```json
 
{
	// Following section describes configuration files with content added by mod
	// It can be split into several files in any way you want but recommended organization is
	// to keep one file per object (creature/hero/etc)
	// Alternatively, for small changes you can embed changes to content directly in here, e.g.
	// "creatures" : { "core:imp" : { "health" : 5 }}

	// list of factions/towns configuration files
	"factions" :
	[
		"config/faction.json"
	]

	// List of hero classes configuration files
	"heroClasses" :
	[
		"config/heroClasses.json"
	],

	// List of heroes configuration files
	"heroes" :
	[
		"config/heroes.json"
	],
	
	// List of configuration files for skills
	skills

	// list of creature configuration files
	"creatures" :
	[
		"config/creatures.json"
	],

	// List of artifacts configuration files
	"artifacts" :
	[
		"config/artifacts.json"
	],

	// List of objects defined in this mod
	"objects" :
	[
		"config/objects.json"
	],

	// List of spells defined in this mod
	"spells" :
	[
		"config/spells.json"
	],
	
	// List of configuration files for terrains
	"terrains" :
	[
		"config/terrains.json"
	],
	
	// List of configuration files for roads
	"roads" :
	[
		"config/roads.json"
	],
	
	// List of configuration files for rivers
	"rivers" :
	[
		"config/rivers.json"
	],
	
	// List of configuration files for battlefields
	"battlefields" :
	[
		"config/battlefields.json"
	],
	
	// List of configuration files for obstacles
	"obstacles" :
	[
		"config/obstacles.json"
	],

	// List of RMG templates defined in this mod
	"templates" :
	[
		"config/templates.json"
	],
	
	// Optional, primaly used by translation mods
	// Defines strings that are translated by mod into base language specified in "language" field
	"translations" :
	[
		"config/englishStrings.json
	]
}
```

## Translation fields

In addition to field listed above, it is possible to add following block for any language supported by VCMI. If such block is present, Launcher will use this information for displaying translated mod information and game will use provided json files to translate mod to specified language.
See [Translations](../translators/Translations.md) for more information

```json
	"<language>" : {
		"name" : "<translated name>",
		"description" : "<translated description>",
		"author" : "<translated author>",
		"translations" : [
			"config/<language>.json"
		]
	},
```

## Mod repository fields

These are fields that are present only in remote repository and are generally not used in mod.json

```json
{
	// URL to mod.json that describes this mod
	"mod" : "https://raw.githubusercontent.com/vcmi-mods/vcmi-extras/vcmi-1.4/mod.json",
	
	// URL that player can use to download mod
	"download" : "https://github.com/vcmi-mods/vcmi-extras/archive/refs/heads/vcmi-1.4.zip",
	
	// Approximate size of download, megabytes
	"downloadSize" : 4.496
}
```

## Notes

For mod description it is possible to use certain subset of HTML as
described here:

<https://doc.qt.io/qt-6/richtext-html-subset.html>
