Following cheat codes have been implemented in VCMI and must be used
within in-game chat:

-   **vcmiistari** - gives all spells and 999 mana to currently selected
    hero
-   **vcmiainur** - gives 5 Archangels to every empty slot of currently
    selected hero
-   **vcmiangband** - gives 10 black knight into each slot
-   **vcmiglaurung** - gives 5000 crystal dragons into each slot
-   **vcmiarmenelos** - build all buildings in currently selected town
-   **vcminoldor** - all war machines
-   **vcminahar** - 1000000 movement points
-   **vcmiformenos** - give resources (100 wood, ore and rare resources
    and 100000 gold)
-   **vcmieagles** - reveals FoW
-   **vcmiungoliant** - conceal FoW
-   **vcmiglorfindel** - advances currently selected hero to the next
    level
-   **vcmisilmaril** - player wins
-   **vcmimelkor** - player loses
-   **vcmiforgeofnoldorking** - Hero gets all artifacts except spell
    book, spell scrolls and war machines. This includes artifacts added
    via mods.

All cheat codes can be applied for specific players or all of them:

-   **vcmieagles ai** - Will reveal FoW only for AI players.
-   **vcmieagles all** - Will reveal FoW for all players on map.
-   **vcmieagles blue** - Will reveal FoW only for blue player.
-   **vcminahar ai** - give 1000000 movement points to each hero of
    every AI player

Some cheats can also be used with certain ObjectInstanceID:

-   **vcminahar 123** - give 1000000 movement points to hero with id of
    123
-   **vcmiarmenelos 123** - build all buildings in town with id of 123

These codes are not a cheats, but can be used in multiplayer by host
player to control the session

-   **game exit/quit/end** - finish the game
-   **game save mygame** - saves the game into mygame file
-   **game kick red/blue/...** - kick player from the game. instead of
    color, can be used a number, e.g. **game kick 0/1/2...**, number is
    a corresponding player number (0 - red, 1 - blue, etc)

# Console commands

Following commands must be used in vcmiclient console (CMD on Windows or
terminal on Linux / Mac):

-   **autoskip** - Toggles autoskip mode on and off. In this mode,
    player turns are automatically skipped and only AI moves. However,
    GUI is still present and allows to observe AI moves. After this
    option is activated, you need to end first turn manually. Press
    \[Shift\] before your turn starts to not skip it.
-   **onlyai** - When typed in pregame, it completely removes human
    players and GUI from game. Use console and bugtracker to test AI
    quickly. Additionally, game cna be launched with --onlyAI parameter
    to enable this mode by default.
-   **crash** - force game crash. It is sometimes useful to generate
    memory dump file in certain situations, for example game freeze.
-   **set <command> <on/off>** - toggle one of debug options on or off.
    Possible commands:
    -   **autoskip** - identical to "autoskip" option
    -   **showGrid** - displays grid on adventure map
    -   **showBlock** - shows blocked tiles on map. Requires image
        **data/blocked.bmp** present.
    -   **showVisit** - shows visitable tiles on map. Requires image
        **data/visitable.bmp** present.
    -   **hideSystemMessages** - supress server messages in chat.
-   **gosolo** - AI take control over human players and vice versa
-   **controlai** - give control of one (if color specified) or all AIs
    to player