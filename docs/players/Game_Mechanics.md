# List of features added in VCMI

Some of game features have already been extended in comparison to Shadow of Death:

- Support for 32-bit graphics with alpha channel. Supported formats are .def, .bmp, .png and .tga
- Support for maps of any size, including rectangular shapes
- No limit of number of map objects, such as dwellings and stat boosters
- Hero experience capacity currently at 2^64, which equals 199 levels with typical progression
- Heroes can have primary stats up to 2^16
- Unlimited backpack
- Support for Stack Experience

The list of implemented cheat codes and console commands is [here](Cheat_codes.md).

# List of bugs fixed in VCMI

These bugs were present in original Shadow of Death game, however the team decided to fix them to bring back desired behaviour:

# List of game mechanics changes

Some of H3 mechanics can't be straight considered as bug, but default VCMI behaviour is different:

- Pathfinding. Hero can't grab artifact while flying when all tiles around it are guarded without triggering attack from guard.
- Battles. Hero that won battle, but only have temporary summoned creatures alive going to appear in tavern like if he retreated. 
- Battles. Spells from artifacts like AOTD are autocasted on beginning of the battle, not beginning of turn.
- Spells. Dimension Door spell doesn't allow to teleport to unexplored tiles. 

# List of extended game functionality

## Quick Army Management

- [LShift] + LClick – splits a half units from the selected stack into an empty slot.
- [LCtrl] + LClick – splits a single unit from the selected stack into an empty slot.
- [LCtrl] + [LShift] + LClick – split single units from the selected stack into all empty hero/garrison slots
- [Alt] + LClick – merge all splitted single units into one stack
- [Alt] + [LCtrl] + LClick - move all units of selected stack to the city's garrison or to the met hero 
- [Alt] + [LShift] + LClick - dismiss selected stack`

## Quick Recruitment

Mouse click on castle icon in the town screen open quick recruitment window, where we can purhase in fast way units.

# Manuals and guides

- https://heroes.thelazy.net//index.php/Main_Page Wiki that aims to be a complete reference to Heroes of Might and Magic III. 
