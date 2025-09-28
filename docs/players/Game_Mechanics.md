# Game Mechanics

## List of features added in VCMI

### High resolutions

VCMI supports resolutions higher than original game, which ran only in 800 x 600. It also allows a number of additional features:

- High-resolution screens of any aspect ratio are supported.
- In-game GUI can be freely scaled
- Adventure map can be freely zoomed

Assets from Heroes of Might & Magic III HD - Remake released by Ubisoft in 2015 - are **not** supported.

### Extended engine limits

Some of game features have already been extended in comparison to Shadow of Death:

- Support for 32-bit graphics with alpha channel. Supported formats are .def, .bmp, .png and .tga
- Support for maps of any size, including rectangular shapes
- No limit of number of map objects, such as dwellings and stat boosters
- Hero experience capacity currently at 2^64, which equals 199 levels with typical progression
- Heroes can have primary stats up to 2^16.
- Unlimited backpack (by default). This can be toggled off to restore original 64-slot backpack limit.

The list of implemented cheat codes and console commands is [here](Cheat_Codes.md).

## New mechanics (Optional)

### Stack Experience module

VCMI natively supports stack experience feature known from WoG. Any creature - old or modded - can get stack experience bonuses. However, this feature needs to be enabled as a part of WoG VCMI submod.

Stack experience interface has been merged with regular creature window. Among old functionalities, it includes new useful info:

- Click experience icon to see detailed info about creature rank and experience needed for next level. This window works only if stack experience module is enabled (true by default).
- Abilities description contain information about actual values and types of bonuses received by creature - be it default ability, stack experience, artifact or other effect. These descriptions use custom text files which have not been translated.
- [Stack Artifact](#stack-artifact-module). You can choose enabled artifact with arrow buttons. There is also additional button below to pass currently selected artifact back to hero.

### Commanders module

VCMI offers native support for Commanders. Commanders are part of WoG mod for VCMI and require it to be enabled. However, once this is done, any new faction can use its own Commander, too.

### Stack Artifact module

In original WoG, there is one available Stack Artifact - Warlord's Banner, which is related directly to stack experience. VCMI natively supports any number of Stack Artifacts regardless if of Stack Experience module is enabled or not. However, currently no mods make use of this feature and it hasn't been tested for many years.

## List of bugs fixed in VCMI

These bugs were present in original Shadow of Death game, however the team decided to fix them to bring back desired behaviour:

### List of game mechanics changes

Some of H3 mechanics can't be straight considered as bug, but default VCMI behaviour is different:

- Pathfinding. Hero can't grab artifact while flying when all tiles around it are guarded without triggering attack from guard.
- Battles. Hero that won battle, but only have temporary summoned creatures alive going to appear in tavern like if he retreated.
- Battles. Spells from artifacts like AOTD are autocasted on beginning of the battle, not beginning of turn.

## Adventure map features

### New Shortcuts

- [LCtrl] + [R] - Quick restart of current scenario.
- [LCtrl] + Arrows - scrolls Adventure Map behind an open window.
- [LCtrl] pressed blocks Adventure Map scrolling (it allows us to leave the application window without losing current focus).
- NumPad 5 - centers view on selected hero.
- NumPad Enter functions same as normal Enter in the game (it didn't in H3).
- [LCtrl] + LClick – perform a normal click (same as no hero is selected). This make it possible to select other hero instead of changing path of current hero.

## Pathfinder

VCMI introduces improved pathfinder, which may find the way on adventure map using ships,Subterranean Gates and Monoliths. Simply click your destination anywhere on adventure map and it will find shortest path, if if target position is reachable.

### Quest log

VCMI itroduces custom Quest Log window. It can display info about Seer Hut or Quest Guard mission, but also handle Borderguard and Border Gate missions. When you choose a quest from the list on the left, it's description is shown. Additionally, on inner minimap you can see small icons indicating locations of quest object. Clicking these objects immediately centers adventure map on desired location.

### Power rating

When hovering cursor over neutral stack on adventure map, you may notice additional info about relative threat this stack poses to currently selected hero. This feature has been originally introduced in Heroes of Might and Magic V.

### Minor GUI features

Some windows and dialogs now display extra info and images to make game more accessible for new players. This can be turned off, if desired.

## Battles features

### Stack Queue

Stack queue is a feature coming straight from HoMM5, which allows you to see order of stacks on the battlefield, sorted from left to right. To toggle in on/off, press [Q] during the battle. There is smaller and bigger version of it, the second one is available only in higher resolutions.

### Attack range

In combat, some creatures, such as Dragon or Cerberi, may attack enemies on multiple hexes. All such attacked stacks will be highlighted if the attack cursor is hovered over correct destination tile. Whenever battle stack is hovered, its movement range is highlighted in darker shade. This can help when you try to avoid attacks of melee units.

## Town Screen

### Quick Army Management

- [LShift] + LClick – splits a half units from the selected stack into an empty slot.
- [LCtrl] + LClick – splits a single unit from the selected stack into an empty slot.
- [LCtrl] + [LShift] + LClick – split single units from the selected stack into all empty hero/garrison slots
- [Alt] + LClick – merge all split single units into one stack
- [Alt] + [LCtrl] + LClick - move all units of selected stack to the city's garrison or to the met hero
- [Alt] + [LShift] + LClick - dismiss selected stack`
- Directly type numbers in the Split Stack window to split them in any way you wish

### Interface Shortcuts

It's now possible to open Tavern (click on town icon), Townhall, Quick Recruitment and Marketplace (click on gold) from various places:

- Town screen (left bottom)
- Kingdom overview for each town
- Infobox (only if info box army management is enabled)

### Quick Recruitment

Mouse click on castle icon in the town screen open quick recruitment window, where we can purhase in fast way units.

## Pregame - Scenario / Saved Game list

- Mouse wheel - scroll through the Scenario list.
- [Home] - move to the top of the list.
- [End] - move to the bottom of the list.
- NumPad keys can be used in the Save Game screen (they didn't work in H3).

## Fullscreen

- [F4] - Toggle fullscreen mode on/off.

## FPS counter

It's the new feature meant for testing game performance on various platforms.

## Color support in game text

Additional color are supported for text fields (e.g. map description). Uses HTML color syntax (e.g. #abcdef) / HTML predefined colors (e.g. green).

### Original Heroes III Support

`This is white`

<span style="color:white;background-color:black;">This is white</span>

`{This is yellow}`

<span style="color:yellow;background-color:black;">This is yellow</span>

### New

`{#ff0000|This is red}`

<span style="color:red">This is red</span>

`{green|This is green}`

<span style="color:green">This is green</span>

## Multiplayer features

Opening new Turn Option menu in scenario selection dialog allows detailed configuration of turn timers and simultaneous turns

### Turn Timers

TODO

### Simultaneous turns

Simultaneous turns allow multiple players to act at the same time, speeding up early game phase in multiplayer games. During this phase if different players (allies or not) attempt to interact with each other, such as capture objects owned by other players (mines, dwellings, towns) or attack their heroes, game will block such actions. Interaction with same map objects at the same time, such as attacking same wandering monster is also blocked.

Following options can be used to configure simultaneous turns:

- Minimal duration (at least for): this is duration during which simultaneous turns will run unconditionally. Until specified number of days have passed, simultaneous turns will never break and game will not attempt to detect contacts.
- Maximal duration (at most for): this is duration after which simultaneous turns will end unconditionally, even if players still have not contacted each other. However if contact detection discovers contact between two players, simultaneous turns between them might end before specified duration.
- Simultaneous turns for AI: If this option is on, AI can act at the same time as human players. Note that AI shares settings for simultaneous turns with human players - if no simultaneous turns have been set up this option has no effect.

While simultaneous turns are active, VCMI tracks contacts for each pair of player separately.

Players are considered to be "in contact" if movement range of their heroes at the start of turn overlaps, or, in other words - if their heroes can meet on this turn if both walk towards each other. When calculating movement range, game uses rules similar to standard movement range calculation in vcmi, meaning that game will track movement through monoliths and subterranean gates, but will not account for any removable obstacles, such as pickable treasures that block path between heroes. Any existing wandering monsters that block path between heroes are ignored for range calculation. At the moment, game will not account for any ways to extend movement range - Dimension Door or Town Portal spells, visiting map objects such as Stables, releasing heroes from prisons, etc.

Once detected, contact can never be "lost". If game detected contact between two players, this contact will remain active till the end of the game, even if their heroes move far enough from each other.

Game performs contact detection once per turn, at the very start of each in-game day. Once contact detection has been performed, players that are not in contact with each other can start making turn. For example, in game with 4 players: red, blue, brown and green. If game detected contact between red and blue following will happen:

- red, brown and green will all instantly start turn
- once red ends his turn, blue will be able to start his own turn (even if brown or green are still making turn)

Once maximal duration of simultaneous turns (as specified during scenario setup) has been reached, or if all players are in contact with each other, game will return to standard turn order: red, blue, brown, green...

Differences compared to HD Mod version:

- In VCMI, players can see actions of other players immediately (provided that they have revealed fog of war) instead of waiting for next turn
- In VCMI, attempt to attack hero of another player during simultaneous turns will be blocked instead of reloading save from start of turn like in HD Mod

## Manuals and guides

- <https://heroes.thelazy.net/index.php/Main_Page> Wiki that aims to be a complete reference to Heroes of Might and Magic III.
