# Campaign Format

## Introduction

Starting from version 1.3, VCMI supports its own campaign format.
Campaigns have `*.vcmp` file format and it consists from campaign json and set of scenarios (can be both `*.vmap` and `*.h3m`)

To start making campaign, create file named `header.json`. See also [Packing campaign](#packing-campaign)

Basic structure of this file is here, each section is described in details below

```json
{
    "version" : 1,

    <header properties>

    "scenarios" : [
        {
            //scenario 1
            <scenario properties>
        },
        {
            //scenario 2
            <scenario properties>
        }
    ]
}
```

`"version"` defines version of campaign file. Larger versions should have more features and flexibility, but may not be supported by older VCMI engines. See [compatibility table](#compatibility-table)

## Header properties

In header are parameters describing campaign properties

```json
    ...
    "regions": {...},
    "name": "Campaign name",
    "description": "Campaign description",
    "author": "Author",
    "authorContact": "Author contact",
    "campaignVersion": "Campaign version",
    "creationDateTime": "Creation date and time",
    "allowDifficultySelection": true, 
```

- `"regions"` contains information about background and regions. See section [campaign regions](#regions-description) for more information
- `"name"` is a human readable title of campaign
- `"description"` is a human readable description of campaign
- `"author"` is the author of the campaign
- `"authorContact"` is a contact address for the author (e.g. email)
- `"campaignVersion"` is creator defined version
- `"creationDateTime"` unix time of campaign creation
- `"allowDifficultySelection"` is a boolean field (`true`/`false`) which allows or disallows to choose difficulty before scenario start
- `"loadingBackground"` is for setting a different loading screen background
- `"introVideo"` is for defining an optional intro video
- `"outroVideo"` is for defining an optional outro video
- `"videoRim"` is for the Rim around the optional video (default is INTRORIM)

## Scenario description

Scenario description looks like follow:

```json
{
    "map": "maps/SomeMap",
    "preconditions": [],
    "color": 0,
    "difficulty": 2,
    "regionText": "",
    "prolog": {},
    "epilog": {},
    "heroKeeps": [],
    "keepCreatures": [],
    "startOptions": "none",
    "playerColor": 0,
    "bonuses": [ <see bonus description> ]
}
```

- `"map"` map name without extension but with relative path. Both `*.h3m` and `*.vmap` maps are supported. If you will pack scenarios inside campaign, numerical map name should be used, see details in [packing campaign](#packing-campaign)
- `"preconditions"` enumerate scenarios indexes which must be completed to unlock this scenario. For example, if you want to make sequential missions, you should specify `"preconditions": []` for first scenario, but for second scenario it should be `"preconditions": [0]` and for third `"preconditions": [0, 1]`. But you can allow non-linear conquering using this parameter
- `"color"` defines color id for the region. Possible values are `0: red, 1: blue, tan: 2, green: 3, orange: 4, purple: 5, teal: 6, pink: 7`
- `"difficulty"` sets initial difficulty for this scenario. If `"allowDifficultySelection"`is defined for campaign, difficulty may be changed by player. Possible values are `0: pawn, 1: knight, 2: rook, 3: queen, 4: king`
- `"regionText"` is a text which will be shown if player holds right button over region
- `"prolog"`/`"epilog"` optional, defines prolog/epilog for scenario. See [prolog/epilog](#prologepilog) section for more information
- `"heroKeeps"` defines what hero will carry to the next scenario. Can be specified one or several attributes from list `"experience", "primarySkills", "secondarySkills", "spells", "artifacts"`
- `"keepCreatures"` array of creature types which hero will carry to the next scenario. Game identifiers are used to specify creature type.
- `"startOptions"` defines what type of bonuses player may have. Possible values are `"none", "bonus", "crossover", "hero"`
- `"playerColor"` defines color id of flag which player will play for. Possible values are `0: red, 1: blue, tan: 2, green: 3, orange: 4, purple: 5, teal: 6, pink: 7`
- "bonuses" array of possible bonus objects, format depends on `"startOptions"` parameter

### Prolog/Epilog

Prolog and epilog properties are optional

```json
{
    "video": ["NEUTRALA.smk"], //video to show (if second video in array: this video will looped after playing the first)
    "music": "musicFile.ogg", //music to play, should be located in music directory
    "voice": "musicFile.wav", //voice to play, should be located in sounds directory
    "text": "some long text" //text to be shown
}
```

### Bonus start option

If `startOptions` is `bonus`, bonus format may vary depending on its type.

```json
{
    "what": "",

    <attributes>
},
```

- `"what"` field defines bonus type. Possible values are: `spell, creature, building, artifact, scroll, primarySkill, secondarySkill, resource, prevHero, hero`
  - `"spell"` has following attributes (fields):
    - `"hero"`: hero who will get spell (see below)
    - `"spellType"`: spell type, string, e.g. "firewall"
  - `"creature"` has following attributes (fields):
    - `"hero"`: hero who will get spell (see below)
    - `"creatureType"`: creature type, string, e.g. "pikeman"
    - `"creatureAmount"`: amount of creatures
  - `"building"` has following attributes (fields):
    - `"buildingType"`: building type (string), e.g. "citadel" or "dwellingLvl1"
  - `"artifact"` has following attributes (fields):
    - `"hero"`: hero who will get spell (see below)
    - `"artifactType"`: artifact type, string, e.g. "spellBook"
  - `"scroll"` has following attributes (fields):
    - `"hero"`: hero who will get spell (see below)
    - `"spellType"`: spell type in the scroll, string, e.g. "firewall"
  - `"primarySkill"` has following attributes (fields):
    - `"hero"`: hero who will get spell (see below)
    - `"attack"`: amount of attack gained
    - `"defence"`: amount of defence gained
    - `"spellpower"`: amount of spellpower gained
    - `"knowledge"`: amount of knowledge gained
  - `"secondarySkill"` has following attributes (fields):
    - `"hero"`: hero who will get spell (see below)
    - `"skillType"`: skill type, string, e.g. "logistics"
    - `"skillMastery"`: skill level, `1: beginner, 2: advanced, 3: expert`
  - `"resource"` has following attributes (fields):
    - `"resourceType"`: resource type, one of `wood, ore, mercury, sulfur, crystal, gems, gold, common, rare`, where `common` is both wood and ore, `rare` means that bonus gives each rare resource
    - `"resourceAmount"`: amount of resources
  - `"prevHero"` has following attributes (fields):
    - `"scenario"`: scenario from which to pick heroes from
    - `"playerColor"`: color which player will play as in current scenario
  - `"hero"` has following attributes (fields):
    - `"hero"`: hero that player would receive on start, or `random`
    - `"playerColor"`: color which player will play as in current scenario
  
`hero` field in all bonus types can be specified as explicit hero name and as one of keywords: `strongest`, `generated`

### Regions description

Predefined campaign regions are located in file `campaignRegions.json`

```json
{
    "background": "ownRegionBackground.png",
		"suffix": ["Enabled", "Selected", "Conquered"],
    "prefix": "G3",
    "colorSuffixLength": 1,
    "desc": [
        { "infix": "A", "x": 289, "y": 376, "labelPos": { "x": 98, "y": 112 } },
        { "infix": "B", "x": 60, "y": 147, "labelPos": { "x": 98, "y": 112 } },
        { "infix": "C", "x": 131, "y": 202, "labelPos": { "x": 98, "y": 112 } }
    ]
},
```

- `"background"` optional - use own image name for background instead of adding "_BG" to the prefix as name
- `"prefix"` used to identify all images related to campaign. In this example (if background parameter wouldn't exists), background picture will be `G3_BG`
- `"suffix"` optional - use other suffixes than the default `En`, `Se` and `Co` for the three different images
- `"infix"` used to identify all images related to region. In this example, it will be pictures whose files names begin with `G3A_..., G3B_..., G3C_..."`
- `"labelPos"` optional -  to add scenario name as label on map
- `"colorSuffixLength"` identifies suffix length for region colourful frames. 0 is no color suffix (no colorisation), 1 is used for `R, B, N, G, O, V, T, P`, value 2 is used for `Re, Bl, Br, Gr, Or, Vi, Te, Pi`

## Packing campaign

After campaign scenarios and campaign description are ready, you should pack them into *.vcmp file.
This file is a zip archive.

The scenarios should be named as in `"map"` field from header. Subfolders are allowed.

## Compatibility table

| Version | Min VCMI | Max VCMI | Description     |
|---------|----------|----------|-----------------|
| 1       | 1.3      |          | Initial release |
