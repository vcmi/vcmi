Some of game features have already been extended in comparison to Shadow
of Death:

-   Support for 32-bit graphics with alpha channel

.def, .bmp, .png and .tga files are supported.

-   Support for maps of any size (2 billion tiles in any direction),
    including rectangular shapes
-   No limit of number of map objects, such as dwellings and stat
    boosters
-   Hero experience capacity currently at 2^64, which equals 199 levels
    with typical progression
-   Heroes can have primary stats up to 2^16
-   Unlimited backpack
-   New flexible [bonus system](Bonus_system "wikilink")
-   Support for Stack Experience

The list of implemented cheat codes and console commands is
[here](Cheat_codes "wikilink").

# List of bugs fixed in VCMI

These bugs were present in original Shadow of Death game, however the
team decided to fix them to bring back desired behaviour.

# List of game mechanics changes

Some of H3 mechanics can't be straight considered as bug, but default
VCMI behaviour is different:

-   Dwellings. Remain creatures are accumulated instead of reset each
    week. Can be disabled using DWELLINGS_ACCUMULATE_CREATURES.
-   Pathfinding. Hero can't grab artifact while flying when all tiles
    around it are guarded without triggering attack from guard. Original
    behavior can be re-enabled using "originalMovementRules" pathfinder
    option.
-   Battles. Hero that won battle, but only have temporary summoned
    creatures alive going to appear in tavern like if he retreated. Can
    be disabled using WINNING_HERO_WITH_NO_TROOPS_RETREATS.
-   Battles. Spells from artifacts like AOTD are autocasted on beginning
    of the battle, not beginning of turn. Task on mantis:
    [1](https://bugs.vcmi.eu/view.php?id=1347)
-   Objects. Black market changes artifacts every month like town
    artifact merchants do. Can be disabled using
    BLACK_MARKET_MONTHLY_ARTIFACTS_CHANGE.
-   Spells. Dimension Door spell doesn't allow to teleport to unexplored
    tiles. Task on mantis to track this:
    [2](https://bugs.vcmi.eu/view.php?id=2751).

# List of extended game functionality

-   Quick army management in the garrison screen:

`   [LShift] + LClick – splits a half units from the selected stack into`  
`   an empty slot.`  
`   [LCtrl] + LClick – splits a single unit from the selected stack into`  
`   an empty slot.`  
`   [LCtrl] + [LShift] + LClick – split single units from the selected`  
`   stack into all empty hero/garrison slots`  
`   [Alt] + LClick – merge all splitted single units into one stack`  
`   [Alt] + [LCtrl] + LClick - move all units of selected stack to the city's garrison or to the met hero`  
`   [Alt] + [LShift] + LClick - dismiss selected stack`

-   Mouse click on castle icon in the town screen open quick recruitment
    window, where we can purhase in fast way units.

# Manuals and guides

-   \[<http://www.4shared.com/document/qLi-INYQ/Heroes_of_Might_and_Magic_III_.html>?
    Official Heroes of Might & Magic III user manual\]
-   \[<http://www.4shared.com/document/HbWI7YlH/Strategija.html>?
    Strategija\] covers detailed machanics not mentioned in official
    documents.
-   \[<http://www.4shared.com/document/F_3NbdTI/New_WoG_Features.html>?
    New WoG features\]