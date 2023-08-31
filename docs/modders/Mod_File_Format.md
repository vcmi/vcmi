## Fields with description of mod

``` javascript
 
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

    // Type of mod, list of all possible values:
    // "Translation", "Town", "Test", "Templates", "Spells", "Music", "Sounds", "Skills", "Other", "Objects", 
    // "Mechanics", "Interface", "Heroes", "Graphical", "Expansion", "Creatures", "Artifacts", "AI"
    "modType" : "Graphical",

    // List of mods that are required to run this one
    "depends" :
    [
        "baseMod"
    ],
 
    // List of mods that can't be enabled in the same time as this one
    "conflicts" :
    [
        "badMod"
    ],

    //List of changes/new features in each version
    "changelog" :
    {
        "1.0"   : [ "initial release" ],
        "1.0.1" : [ "change 1", "change 2" ],
        "1.1"   : [ "change 3", "change 4" ]
    },

    // If set to true, mod will not be enabled automatically on install
    "keepDisabled" : false
}
```

## Fields with description of mod content

These are fields that are present only in local mod.json file

``` javascript
 
{
    // Following section describes configuration files with content added by mod
    // It can be split into several files in any way you want but recommended organization is
    // to keep one file per object (creature/hero/etc) and, if applicable, add separate file
    // with translatable strings for each type of content
    // See "additional links" at the bottom of page for descriptions of each of these formats

    // list of factions/towns configuration files
    "factions" :
    [
        "config/myMod/faction.json"
    ]

    // List of hero classes configuration files
    "heroClasses" :
    [
        "config/myMod/heroClasses.json"
    ],

    // List of heroes configuration files
    "heroes" :
    [
        "config/myMod/heroes.json"
    ],

    // list of creature configuration files
    "creatures" :
    [
        "config/myMod/creatures.json"
    ],

    // List of artifacts configuration files
    "artifacts" :
    [
        "config/myMod/artifacts.json"
    ],

    // List of objects defined in this mod
    "objects" :
    [
        "config/myMod/objects.json"
    ],

    // List of spells defined in this mod
    "spells" :
    [
        "config/myMod/spells.json"
    ],

    // List of RMG templates defined in this mod
    "templates" :
    [
        "config/myMod/templates.json"
    ],

    // Optional, description on how files are organized in your mod
    // In most cases you do not need to use this field
    // Needed mostly to port any existing mods to vcmi (e.g. WoG distributed with Era)
    // Example below is default value, which is "Content" directory that acts as H3 root directory
    "filesystem":
    {
        "":
            [
                {"type" : "dir",  "path" : "/Content"}
            ]
    }
}
```

## Notes

For mod description it is possible to use certain subset of HTML as
described here:

<http://qt-project.org/doc/qt-5.0/qtgui/richtext-html-subset.html>