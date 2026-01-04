# VCMI Project Changelog

## 1.7.0 -> 1.7.1

### Stability

* Fixed game state corruption on loading save made in 1.6 in 1.7 version
* Fixed possible crash on mod install on some systems
* Fixed rare crash when mod with not loaded dependency is incorrectly added to load list
* Fixed crash on loading saves made with same set but different load order of mods
* Fixed picked up artifacts turning into Spell Books on maps in .vmap format
* Fixed possible crash on invalid OPENING_BATTLE_SPELL bonus
* Fixed possible crash on invalid SUMMON_GUARDIANS bonus
* Fixed crash on adding large number of spells to hero in battle-only mode
* Fixed crash on rearranging troops in some cases in multiplayer
* Fixed possible crash on AI shutdown
* Avoid crash and display error message on failure to install mod after opening screenshot tab during mod download on some systems
* Fixed desynchronization between client and server caused by temporary artifacts generated during AI turn
* Fixed rare crash on having growing artifacts in inventory in the same time as having expired charging artifact

### Mechanics

* Fixed randomization of damage range when unit can kill whole unit stack at max damage

### Interface

* Added support for generating screenshots in-game. By default this action is bound to F3 hotkey
* It is now possible to abort selection of a campaign set after pressing button to open list of campaign sets
* Fixed HD assets incorrectly used for .def files of mods if file names inside .def clash with H3 filenames
* Disabled black bars that were present on iOS on both sides of the screen to avoid areas with camera notch and rounded corners

### AI

* Removed map hack from Nullkiller2 AI
* Adjusted Nullkiller2 AI settings to improve performance
* Fixes an issue where fallback from MMAI to BattleAI was not correctly triggered.
* Fixed handling of battles with over 20 units present by MMAI
* Improved performance of MMAI by up to 50%

### Launcher

* Fixed reset of AI settings on opening Launcher. AI settings will be reset to default for all players
* Added option to disable before visit saves
* Fixed not visible star in mod list title on some systems

### Map Editor

* Fixed saving of customized map object parameters for RMG in template editor
* Fixed bug that made it impossible to select gold as reward for Seer Hut
* Fixed passability overlay not visible on 3rd and later layers
* Fixed several cases of broken layout in map editor

## 1.6.8 -> 1.7.0

### Key Changes

* Windows ARM and Android x86_64 platforms are now officially supported
* Added support for loading h3m maps and campaigns from HotA 1.7 (also needs support from HotA mod)
* Implemented network lag compensation system for some actions like hero movement and troops rearrangement
* It is now possible to view and configure keybinding in game
* Battle-only mode has been implemented, consisting solely of combat between heroes without an adventure map phase.
* Implemented unit action panel in combat for easy selection of unit's alternative actions
* Implemented quick save & quick load options in game
* Game will now automatically make save before visiting objects
* Added an improved Nullkiller2 adventure map AI as the default option.
* Added an experimental machine learning-based combat AI, MMAI
* Removed the deprecated VCAI adventure map AI
* Added translation to Romanian and Turkish languages
* Initial support for maps with more than two layers
* It is now possible to import data from Heroes 3: HD Edition into VCMI. Shadow of Death or Complete edition is still required as a base game
* The Launcher now shows the number of GitHub stars as the mod ranking
* Added support for exporting game logs via Launcher

### Stability

* Fixed possible crash when player with Chinese localization attack with Vampire Lords, causing one of them to resurrect
* Fixed possible crash on combining Drain Life with area-targeted attack by units from mods
* Fixed multiple possible crashes on game shutdown
* Fixed memory leak on playing video file with embedded audio track
* Fixed small memory leak on playing a video file
* Fixed large number of small memory leaks when restating map or returning to main menu
* Fixed crash on attempting to access map located in .zip archive
* Fixed rare freeze on starting combat with opening spells present, such as from Armor of the Damned
* Fixed a possible crash when opening the stack window in the fort window for units with an ability limited to a specific terrain
* Fixed crash on loading of community-made maps with out-of-bounds towns
* Fixed possible crashes on ending battle in a draw in objects like town, pandora boxes or creature banks
* Fixed a crash when accessing a shipyard placed adjacent to the left border of the map
* Fixed crash when unit with attack-and-return attack is slowed by retaliation and can’t return to origin
* Fixed possible client crash on ending the game when playing through online lobby
* Fixed crash on attempt to import mod preset that contains unknown mod

### Interface - General

* Fixed not functioning keybindings when non-lating keyboard layout is in use
* Fixed movement of software cursor when touch input is in use, for example on mobile systems
* "Show FPS" option has been replaced with "Show Performance Overlay" option that also displays time and remaining battery level, if any
* Change scroll direction for horizontal slider when using mouse wheel to match HD mod
* Spell selection in the spellbook now includes haptic feedback
* Fixed possible mid-word wrapping in icon descriptions on items with long names, such as some artifacts
* The length of the slider in all dialogs now depends on the number of available items, rather than always being square
* The creature window and the quest log now use the same background as the rest of the in-game menus for a more native look

### Interface - Campaign and Map selection

* Added button to access campaign sets from mods (such as Chronicles) from custom campaigns screen
* Custom map sizes can now be selected for random map generation
* Option to search for a random map template by name has been added.
* Added option to view game statistics for campaign scenario
* Game will no longer show chat button in campaign selection screen
* It is now possible to skip campaign and main menu video with Esc or Enter keyboard keys, in addition to mouse click
* Added option to toggle video subtitles
* Added missing tooltips for campaign set buttons (RoE/AB/SoD/etc)
* Maps with non-latin names will now be sorted correctly according to selected language
* Fixed corrupted background for Armaggeddon's Blade campaign selection screen
* Fixed a possible text overflow issue with long skill or spell names in the hero overview window when selecting a starting hero
* It is no longer possible to deselect underground on random map templates that only support maps with underground

### Interface - Adventure Map

* Right-clicking on a hero, castle, or garrison will show a window whose color is determined by the owner of the target rather than the current player
* It is now possible for players to access game statistics for themselves and their allies during gameplay
* Heroes on minimaps are now shown as icon, to help with readability on large maps or on small displays
* Right-click tooltip on Refugee Camp will now show preview of available creatures if current player have visited it this week
* Right-click on previously visited University will now show skills available in it
* Implemented "Show Path" setting option from H3
* Engaging into a combat with wandering monster will cause monster to show attack sprite on adventure map
* It is now possible to search for any objects on map, including individual heroes or wandering monsters
* Improved handling of non-latin texts in spell search and adventure map object search
* Fixed text alignment on hero status window popup
* Fixed hero path not updating instantly after using infinite movement cheat
* Game will now show correct visitation text for map dwellings from mods with 2 or 3 available creatures
* Fixed not replaced text placeholders in visitation message for unique dwellings with single available unit
* Fixed positioning of GUI elements when improved hero exchange from vcmi extras mod is in use
* Fixed army transfer during hero exchange sometimes not transferring all possible units
* Swapping two armies on hero exchange window will now maintain unit ordering
* It is now possible to sort the artifacts in the backpack in reverse order by clicking on the same sorting mode again
* A new backpack icon created by Andruids has been added
* Right-clicking on the resource bar will now also show the expected daily income, in addition to the current resource amount
* It is now possible to see the name of the artifact even when the slot is locked
* Combined or fused artifacts now show their list of components as part of their description
* Changed background for right-click popup of on-map garrisons

### Interface - Towns

* Equipping or unequipping artifacts that affect unit growth will now instantly update town interface with new growth values
* Fixed quick recruitment dialog failing if there are no free slots in army
* It is now possible to rename a town that the player owns during gameplay
* Conflux with Aurora Borealis build will now correctly display all available spells in the Mages Guild.
* Removed water wheel animation from Fortress town screen in line with H3

### Interface - Combat

* Implemented unit action panel in combat for easy selection of unit's alternative actions
* Battle window now uses thin border and require less screen space. Quick actions side panels are now active on all 900px-wide resolutions
* Hovering over target with spell will now show spell impact (damage, health, raised or summoned unit amount) in status bar
* Units that will be affected by multi-hex attacks like Dragon Breath will now be selected similar to primary target
* It is now possible to permanently open creature window of any unit using "show info" button from unit action panel
* Right-click tooltip on list of spells affecting unit in unit window will now show full spell description
* It is now possible to move double-wide unit one hex backwards
* It is now possible to finish battle with autocombat during tactics phase
* Angel, Devils, and Bone-Dragon battle-wide abilities that change luck and morale now show up in UI with proper description
* Orb of Vulnerability effect now clearly displayed in unit window and only if unit has magic resistance, instead of displaying strange 0% resistance ability on all units
* Fixed bonuses from terrain (such as Holy Grounds) not showing up in unit window
* Fixed possible overflow of unit health bar if unit health is different from max health of unit type

### Mechanics - General

* During simultaneous turns, it is no longer possible to interact (for example, start battle) with player that is already engaged into a different combat
* Heroes that are marked as unavailable for specific player will now be correctly blocked from use as starting heroes
* Removed default limit of 16 total heroes per player
* Winning combat with no enemy casualties will no longer result in raising of a single skeleton when victor has Necromancy
* If hero has Necromancy and no available slot, game will now also consider upgrades of upgrades as potentially rised creature
* Fixed incorrect recalculation of stack experience when splitting-off a single unit via shortcut
* Implemented `vcmiteleport x y z` cheat that teleports current hero to target position
* Gold Golems and Diamond Golems now have correct level, in line with H3

### Mechanics - Heroes and Commanders

* Lord Haart is now available instead of Sir Mullich on random maps, in line with H3
* Implemented stacking of artifacts (e.g. resource-producing artifacts) in line with H3 - multiple such artifacts will all provide their bonuses
* Heroes with neutral units are now always subject to terrain penalties (unless hero has sufficiently advanced Pathfinding skill)
* Artifacts that provide multiple spells, such as Tomes of Spells and Spellbinder Hat will no longer provide spells banned on maps (only for random maps and maps in HotA format)
* Added missing ability for commanders that mastered Defense and Speed skills
* Fixed commanders not receiving levelups once hero fills all his secondary skills
* Commanders will now automatically gain no melee penalty bonus on receiving ranged attack
* Commanders can now hold up to 9 artifacts
* Commanders experience is now capped to map hero level limit, if any exists on map

### Mechanics - Quests and Events

* Implemented HotA 1.7 map feature: scrolls can be part of seer hut or pandora box reward
* Implemented HotA 1.7 map feature: seer hut and quest guards can ask for specific scroll
* Added disabled by default option that allows quests that take entire army and leave hero without army to emulate H3 bug
* Quests that take entire army can now be completed if quests give replacement units
* Fixed events that are available for all players not activating on maps in .vmap format
* Fixed bug that caused events that only contain text message to not trigger in premade .vmap's
* Guardians of owned dwellings no longer count towards "accumulate X creatures" win condition

### Mechanics - Adventure Map Objects

* Purchasing map from a cartographer will now correctly mark all cartographers of this type as visited
* Leaving units in cleared creature bank will no longer turn left creatures into guards of an empty bank
* Visiting a configurable town building will now always show popup instead of displaying effect in inactive infobox of an adventure map
* Fixed invalid positioning of Portal of Glory when selected as replacement for random dwelling on premade maps
* It is no longer possible to summon boat on top of another boat with hero in it
* Multi-creature dwellings like Elemental Conflux now correctly provide town growth bonus for every available creature
* Garrisoned hero will no longer receive bonuses from town buildings that give bonuses during siege, and will only receive them for the duration of combat
* Fixed bug that allowed to attack heroes inside Sanctuary
* Assembling an artifact will now correctly trigger victory conditions, if any
* Game will now correctly disassemble combined artifact when its part is needed for Seer Hut quest
* University, Scholar, and Witch Hut will no longer grant special secondary skills from mods
* Fixed bug that could cause battle to start on battlefield from nearby map object, instead of current terrain

### Mechanics - Combat

* Luck, morale, and most of combat abilities with chance to trigger now use randomization with memory, preventing long streaks of "bad" rolls.
* Fixed inability of unit to cast spell after receiving morale
* Fixed inability of unit to receive morale if hero casts a spell after unit receives turn
* It is no longer possible for a unit that ends its turn in moat or other obstacle to attack another unit
* Hypnotized units with multi-target attacks such as Cerberi or Hydra will now attack all units that they view as enemy
* Attack skill provided by equipped artifact will now correctly modify damage range of Ballista
* Familiar's Magic Channel ability will no longer work after Familiars are dead
* Resistance now correctly interacts with Chain Lightning spell, in line with H3
* Units that are unable to act (e.g. Blinded) will no longer be able to use preemptive attack ability
* Fixed functionality of Adela specialty
* Fixed inability to use ranged attack if enemy clone that was blocking unit was killed in this round before
* Fixed inability of creatures to cast spells when controlled by defending player in hotseat mode
* Fixed miss chance for catapult not computed correctly when hero has no ballistics skill
* vcmiistari cheat now also allows casting spell up to 100 times per combat round for ease of testing
* It is now possible to flee the combat while having shackles of war when other side got no hero
* Changed formula of town arrow towers to be more in line with H3
* Fixed large number of issue with Magic Mirror spell. The spell logic should now be in line with H3
* Unit under Berserk effect will now correctly move towards nearest unit if they can't reach any target
* Ranged unit under Berserk effect will now use ranged attack if able
* Multiple issues where it was possible to select illegal units for the Sacrifice spell have been fixed
* Fixed a possible overlap between the commander and a double-wide creature in the fifth slot of the army

### Campaigns

* Fixed selection of starting faction in last scenario in Spoils of War campaign
* Magic heroes without spellbook will no longer receive one on transferring to next scenario (e.g. Yog)

## AI

* Added an improved Nullkiller2 adventure map AI as the default option.
* Removed the deprecated VCAI adventure map AI
* Fixed possible infinite loop when hero can't decide whether to equip mana regeneration artifact or knowledge boosting artifact
* Fixed incorrect BattleAI estimation of multi-hex attacks when attacking from behind if at least one of the units is double-wide
* Fixed a bug where the adventure AI might attempt to act before all the side effects from the battle end have been processed.
* Fixed bug where BattleAI would only consider head position of double-wide unit leading to inability to move such unit
* BattleAI is now aware of invincible units and will not attempt to attack them
* Units under BattleAI control will now cast healing, resurrection, and summoning spells, if any

### Random Maps Generator

* Placement of main town in center of zone will now account for water tiles
* It is now possible to enable normally banned heroes, spells or skills in RMG template
* Added support for town type hints `likeZone`, `notLikeZone`, `relatedToZoneTerrain`
* Map generator will now place curved roads
* Fixed missing road on 6lm10a template
* It is now possible to force zone to be placed on specific map level
* Removed roads that don't end with towns or zone entry points
* Fixed possible freeze on attempt to revert unsucessful treasure placement
* Simplified syntax and fixed logic of object customization of map objects in random map template
* Random map generator will now respect customized RMG parameters for dwellings
* Seer Huts will have low RMG value while quest artifacts will be guarded instead

### Launcher

* Fixed a crash when importing the new version of the Heroes Chronicles installer from GOG.com
* Fixed importing of maps on Android
* Fixed a bug causing mods to be disabled if they are marked as soft-depending on submod of mod that was disabled
* Mod download will now keep screen active on mobile system to prevent corrupted downloads
* It is now possible to import a .zip archive containing multiple maps
* Fixed mod selection reset on installation of a new mod
* The mod installation phase now correctly shows the mod name instead of the mod ID
* Added built-in editor for configuration file
* Correctly hide gog install button after successful install
* Fixed sorting of mods by their enabled status in launcher
* The launcher will now use the system proxy (if available) for all downloads

### Map Editor

* Massive performance and memory usage issues when editing huge maps have been fixed
* Added random map template editor, available from map editor
* Added initial version of campaign editor, available from map editor
* Added support for drag-and-drop of maps into map editor
* Added new keyboard shortcuts
* Keyboard shortcuts are now visible in menu
* It is now possible to undo or redo operations that change the terrain, rivers, or roads
* Add filter to hero spell selection widget
* Map validation results window now dynamically adjusts its size based on message lengths
* A valid map will now properly display an appropriate message in the map validation results
* Added word wrap for item text activates only when the map validation window reaches a defined width limit
* Fixed potentially corrupted tiling of terrains in some edge cases when multiple types of terrain are adjacent to each other
* When changing terrain, the orientation of pre-existing rivers on affected tiles will now be preserved correctly
* Fixed discrepancy in map object rendering between editor and client
* Fixed selected spells changing to artifacts in reward box on reopening
* Fixed possible crash on changing inspector table instantly after reloading a map
* Fixed changing neutral armies character when non-English locale is in use
* Fixed possible crash on changing victory conditions
* Map editor now uses .ini format to save settings on all platforms

### Modding - General

* Added support for custom magic schools
* Added support for custom resource types and mines. Hardcoded Mithil support has been removed
* Added support for icons in the list of owned dwellings for objects from mods in the Kingdom Overview.
* It is now possible to replace or append individual entries in json lists (`[ 1, 2, 3 ]`) without replacing entire list
* It is now possible to specify both nominator and denominator (roll difficulty & dice size) for luck and morale probabilities
* It is now possible to define hero secondary skill specialties in form `"secondary" : "archery"`
* Added option to show additional images in main menu, scenario selection, and loading screen
* It is now possible to add additional campaign sets without causing mod conflicts
* Game will now automatically generate campaign screen backgrounds, depending on number of campaigns in set
* Added "Campaigns" mod type
* It is now possible to list terrain ID's from mods that are not dependency in map object template definition
* Added support for banning heroes, artifacts, spells, and secondary skills in random map templates
* Added support for configuring parameters of H3 adventure map spells and for creating similar spells in mods
* It is now possible to change number of skills offered to hero on levelup
* Game will now generate shadow and selection overlay if enabled in animation config
* Implemented validation of `targetCondition` in spells to automatically detect invalid mods
* Implemented validation of `mapObject` in towns and heroes to automatically detect invalid mods
* It is now possible to define cheat aliases in mod

### Modding - Adventure Map Objects

* It is now possible to completely remove skill as part of reward of a configurable adventure map object
* It is now possible to configure cost of purchasing skill in on-map University and Magic University in Conflux
* Mod validation will now report map dwellings with invalid dimensions that were found in mods
* Added `playerGlobal` visit mode to configurable map objects. After visit of such object, all map objects of the same type are considered as visited by player
* Added `forceCombat` property to configurable map objects. If such object is guarded, visiting it would immediately force combat without asking a player
* Added `commanderBonuses` property to configurable map objects rewards that gives bonuses to hero commander (if exists)
* Added `playerBonuses` property to configurable map objects rewards that gives bonuses to hero owner. Unlike propagator, this bonus will remain after hero is lost
* Added `takenArtifacts` property to configurable map objects rewards that takes a specific artifact if hero has it
* Added `takenArtifactsSlots` property to configurable map objects rewards that takes an artifact from specific slot, if slot is not empty
* Added `scrolls` property to configurable map objects rewards that gives hero scroll with specific spell
* Added `takenScrolls` property to configurable map objects rewards that takes a scroll with specifc spell if hero has it
* Added `takenCreatures` property to configurable map objects rewards that takes specified amount of creatures if hero has them
* Added `commanderAlive` property to configurable map objects limiter that requires hero to have alive commander
* Added `hasExtraCreatures` property to configurable map objects limiter that requires hero to have any creatures other than ones requested (for purpose of removing them)
* Added `canReceiveCreatures` property to configurable map objects limiter that requires hero to have enough free slots or same units to accept tested creatures
* Added `scrolls` property to configurable map objects limiter that requires hero to have scroll with specific spell
* Added `availableSlots` property to configurable map objects limiter that requires hero to have specific artifact slots empty

### Modding - Towns

* Removed `height` property from town buildings. Towns now use SIGHT_RADIUS bonus to determine terrain reveal radius similar to heroes
* It is now possible to configure skills available in Magic University in Conflux or similar building from mods
* It is now possible to give units to visiting hero using configurable town building, but only if hero can accept these units into his army
* Added support for configuring positions of spells in mage guild window per town
* Added `auroraBorealis` and `deityOfFire` as building types instead of hardcoded checks for such buildings

### Modding - Artifacts

* It is now possible to have growing artifacts for heroes, not only for commanders
* Added support for charged artifacts that can be used a limited number of times
* Added support for instance bonuses for artifacts that stack if multiple copies of the same artifacts are equipped on hero

### Modding - Bonus System

* Added basic support for addition of new bonuses via mods, for use in limiters
* Added support for custom description and icons for creature abilities and artifacts
* It is now possible to use limiters in unit stack experience bonuses
* It is now possible to configure power of H3-like skill and creature specialties (5% / level) via game config
* Added support for configuring icons for bonus icons in creature window per bonus subtype or per bonus value
* It is now possible to hide creature ability from creature window

### Modding - Bonus Types

* Added bonus BASE_TILE_MOVEMENT_COST that allows configuring minimal cost for moving between tiles for heroes
* Added bonus MULTIHEX_UNIT_ATTACK - configurable version of Dragon Breath
* Added bonus MULTIHEX_ENEMY_ATTACK - configurable version of Cerberi multi-headed attack that only hits enemies
* Added bonus MULTIHEX_ANIMATION - optional bonus that does not affects gameplay, but allows to define in which cases game should use alternative attack animation
* Added bonus STACK_EXPERIENCE_GAIN_PERCENT that affects amount of stack experience received by units after combat
* Added bonus HERO_SPELL_CASTS_PER_COMBAT_TURN bonus that affects number of time a hero can cast spell during combat round
* Added VULNERABLE_FROM_BACK bonus that increases damage if unit with this bonus is attacked from behind (e.g. needs to reverse)
* Added FEARFUL bonus that replaces less configurable FEAR and FEARLESS bonuses
* Added FULL_MAP_DARKNESS bonus that covers with fow everything outside of sight radius of enemies
* Added FULL_MAP_SCOUTING bonus for Skyship. Skyship now provides bonus instead of being weird case of lookout tower.
* Added LIVING bonus that is automatically given to all creatures that don't have other creature nature bonus
* Added SKELETON_TRANSFORMER_TARGET that indicates which creature this unit should be transformed into when placed into Skeleton Transformer
* Added TRANSMUTATION_IMMUNITY bonus that prevents affected unit from TRANSMUTATION effects
* Added COMBAT_MANA_BONUS that gives hero additional mana for the duration of combat (Skyship)
* Added SPECIFIC_SPELL_RANGE that limits possible range for casting spells by creatures in combat
* Added MANA_PERCENTAGE_REGENERATION bonus that replaces less configurable FULL_MANA_REGENERATION bonus
* Added HATES_TRAIT bonus that functions like HATE bonus but allows to target all units with specific bonus
* Added FORCE_NEUTRAL_ENCOUNTER_STACK_COUNT bonus that allows to override number of enemy stacks when attacking wandering monsters
* GENERATE_RESOURCE bonus is now also checked in town, mine, and garrison scope
* ENCHANTER bonus will no longer cast mass spells by default. Spell would still be massive it is massive on specified school master level
* CREATURE_UPGRADE bonus is now checked from unit scope and will only allow upgrades of this unit (unless propagated to hero)
* Reworked DARKNESS bonus. Bonus now can be used on both towns and heroes, and only applies on start of turn
* DRAGON_NATURE bonus will no longer affect result of Skeleton Transformer use
* SPELL_LIKE_ATTACK bonus now correctly works when used with limiters
* OPENING_BATTLE_SPELL bonus can now use addInfo to select desired spell mastery level
* Removed SYNERGY_TARGET bonus

### Modding - Bonus Limiters

* All limiters now use named parameters instead of ambiguous `parameters` value
* Creature terrain limiter will now correctly evaluate outside of combat
* Creature terrain limiter applied to hero primary skills now works as expected when combined with noneOf limiter
* Creature terrain limiter has been renamed to terrain limiter and can be used for heroes and towns
* Creature type limiter will now correctly handle upgrades of upgrades
* Bonuses with terrain limiter will now correctly update on hero stepping onto different terrain
* HAS_ANOTHER_BONUS_LIMITER no longer requires bonus type present

### Modding - Bonus Updaters

* Added DIVIDE_STACK_LEVEL updater that functions similar to TIMES_STACK_LEVEL
* Added TIMES_HERO_LEVEL_DIVIDE_STACK_LEVEL updater that combines effects of TIMES_HERO_LEVEL and DIVIDE_STACK_LEVEL updaters
* Owner updater now correctly works with opposite side limiter when composite limiters `noneOf` or `anyOf` are used
* TIMES_HERO_LEVEL updater now accepts stepSize parameter
* Added TIMES_STACK_SIZE and TIMES_ARMY_SIZE updaters that multiplies bonus value by size of unit stack

### Other

* It is now possible to change default directory paths used by VCMI on Windows
* Beta and Develop builds will now generate separate desktop and start menu shortcuts
* Added alternative versions of cheats from RoE and AB editions
* Implemented nwcphisherprice and a new grayscale cheat
* Cheat parameters (e.g. creature, artifact and skill identifiers) are now case-insensitive
* Added option for VCMI to honor mute switch on iOS

## 1.6.7 -> 1.6.8

### Stability

* Fixed crash when visiting Cannon Yard from HotA mod that was occuring on some systems
* Fixed crash when moving through an event that gives enough experience to level up if the player is using instant movement speed
* Fixed crash when trying to sort maps in reverse order that was occurring on some systems when there were multiple folders containing maps
* Fixed crash when using one-click upgrade when unit upgrade cost is zero
* Fixed crash on opening town screen with invisible buildings from mods
* Fixed crash when a player changes their starting hero in multiplayer game while another player has town or starting bonus right-click popup open
* Fixed crash when merging two units located in town garrison without a hero if both units are equipped with artifacts (WoG feature)

### General

* Fixed not working check for hypnotize spell effect presence
* Fixed calculation of the total cost of the hero's movement path when the hero has spent some of his movement points today.
* Fixed Launcher not closing after game start on Mac OS.
* Launcher will no longer tracks clipboard state on mobile systems to prevent OS notifications
* Automatic selection of xbrz2 upscaler now only happens on systems with 4GB of RAM
* Game will now detect broken internal mod left over from VCMI 0.99 installations and notify player on startup instead of crashing

## 1.6.6 -> 1.6.7

### Stability

* Fixed regression causing crash when trying to create lobby room
* Fixed regression causing crash when upscaling image in background thread on some systems
* Fixed possible crash on opening Custom Campaigns window while having campaign with unsupported format in maps directory
* Fixed possible crash on misconfigured `compatibilityIdentifiers` field in mods
* Fixed rare crash on AI turn that could sometimes happen after AI dismissed a hero

### General

* Added alternative layout for global lobby window that supports H3-like 4:3 screen ratio
* Added option in launcher to disable in-game overlay available with Alt or two-finger touch.
* Game will now save and restore map zoom level between sessions.
* Fixed regression that caused Brotherhood of the Sword to open the Thieves' Guild window instead of the Tavern window when clicked.
* Fixed regression causing black pixels on some city building sprites from mods when played without upscaling filter
* Improved handling of very slow taps on mobile systems
* Added snapping of marker when mouse cursor is next to data point for easy selection in game statistics window
* Fixed some graphical artifacts in the game statistics window.
* Fixed client not checking if submod is compatible with current VCMI version

## 1.6.5 -> 1.6.6

### General

* Game no longer requires local network connection for single player games
* Reduced size of obstacle-filled junction zones in Coldshadow Fantasy template
* Upscaling filter xbrz x2 is now enabled by default on mobile systems
* Fixes failure to import Chronicles on Windows with non-ascii characters in username
* Added support for importing Chronicles using old All-in-One installer from gog.com
* It is now possible to enable portrait mode on mobile systems.
* Fixed grey bar at top of screen when returning to app while in game on Android

### Stability

* Fixed possible crash on opening unit description with unavailable upgrades
* Fixed crash on winning game after last player loses the game due to not controlling a town for 7 days

### Interface

* Pressing Q during hero exchange will now swap both army and artifacts and will no longer trigger a quest log
* Spellbook search is no longer enabled by default, allowing standard h3 shortcuts to work. Search can now be activated by pressing Tab
* Ctrl/Shift + click on arrow buttons below creature slots during hero exchange now works in the familiar way from hd mod
* On mobile systems, clicking on a blocked tile of a visitable object on the adventure map will now build a path to it
* It is now possible to activate the adventure map overlay on the mobile system using the two-finger tap gesture
* Fixed incorrect pinch event calculation that caused problems when zooming with touchscreen gestures
* Game now displays both total cost in movement points and estimated time to arrive in turns when hovering over an accessible location
* Artifact sort buttons in the Hero Backpack window now have correct text describing the sort order
* Fixed non-standard color handling for shadows under selection highlight in creature animations from mods such as HotA's Iron Golem
* Effects such as Bloodlust, Clone, and Petrify will now display correctly when xbrz is in use
* Fixed broken Chronicles campaign screen available with new main menu themes mod
* Fixed empty bonus shown in unit info window when unit is in Necropolis with Cover of Darkness built
* Right-clicking on the difficulty button will now display the difficulty description popup
* Fixed regression causing two minus signs in Fountain of Fortune description
* Added option to upgrade all creatures in the radial menu when in town
* Added option to display remaining unit health in the form of a health bar
* Fixed regression that caused unavailable tiles to be displayed on the left and right sides of the battlefield when hovering with the mouse
* Fixed regression that caused all spells to be displayed as having a duration of 16 rounds
* Scrolling in the lobby window now only happens when hovering over the appropriate item, instead of scrolling all scrollable widgets at once
* Fixed regression that caused black pixels on some hero portraits in mods that use 8-bit palette images
* Fixed memory leak when upscaling images with xbrz filter
* Fixed creature windows text align and buttons background

### Mechanics

* It is no longer possible to attack heroes standing on a visitable object from blocked tiles or from water when the attacker uses Fly
* Fixed regression from 1.6 that caused multiple taverns in towns of the same faction to not be counted towards the level of information available for the thieves' guild
* Fixed regression that caused Cove towns placed on map to be replaced with Castles on HotA maps
* The amount of gold a player can receive from a bonfire is now always equal to the amount of rare resources received multiplied by 100
* Disabled default victory conditions on all Elixir of Life campaign maps that require an artifact to be found, in line with H3

### Nullkiller AI

* Improved scoring of town buildings by the AI
* AI will now prefer to give faster units to its scout heroes to optimize their movement points in future turns
* Fixed AI not constructing prerequisites for town buildings in some cases, like not building Stables when attempting to build Training Grounds
* AI will now avoid recruiting heroes if AI is low on gold or if the town is threatened by an enemy hero
* AI will no longer attempt to use more than one hero to defend a town
* AI will now devalue non-flying units when attacking towns with fortifications to prevent suicides against castles
* Increased the priority of building unupgraded dwellings, as they provide units that can be hired immediately, rather than next week like citadels and castles
* When multiple cities are threatened, the AI will now prefer to defend the one that takes the least number of turns to reach
* Fixed AI attempting to restore mana points in town without a mage guild built
* Reduced AI prioritization of army merging to the same level as general gathering
* AI will now prioritize army merging before attacking enemies
* Increased AI defense prioritization
* AI will no longer leave the defense of a threatened town in order to bring the army to another hero
* AI will no longer send heroes to die outside of towns that already have a garrisoning hero inside, if there's a stronger enemy hero lurking around the town
* AI will no longer focus excessively on reaching Keymaster tents
* AI will no longer rush towns that don't have a citadel or better if there is a strong enemy hero in the area
* AI will no longer try to maximize defenses by using the strongest defender possible, but will instead try to use the most appropriate defender
* Heroes that are currently threatened will be braver and not worry about attacking things that are also threatened if nothing safe is in range

### Launcher

* Added context menu for mod lists that allows disabling, enabling, installing, uninstalling, updating, opening installed mod location, and opening mod repository

## 1.6.4 -> 1.6.5

### General

* Fixed corrupted graphics of generated assets like water tiles on mobile systems
* All generated assets are now used directly from memory without saving them to disk
* Launcher will now correctly show screenshots for already installed mods
* Fixed broken icons in commander information dialog

### Stability

* Fixed regression causing crashes in combat when touchscreen input is in use
* Fixed regression causing crash on attempt to upscale empty image
* Fixed crash on some creature abilities from mods that cast targeted spells on unit with battle propagator
* Fixed crash on accepting next turn in multiplayer when local player has game settings window open
* Fixed crash in multiplayer when one player changes his starting options while another player has hero overview window open
* Fixed crash on double-clicking login to global lobby button

## 1.6.3 -> 1.6.4

### General

* xbrz image upscaling is now performed in background threads to avoid freezes in UI
* Fixed a bug which caused importing data to fail on some Android devices.
* It is now possible to add or remove per-language channels in lobby.
* Fixed bug causing broken water tile animation when player opens launcher while game is running
* Fixed smooth map dragging not working with right click drag
* Game will no longer play sound on new chat message in global chat unless lobby UI is currently open
* Fixed new building sound playing twice on costructing some buildings, such as town hall

### AI

* Significantly improved Battle AI performance
* Slightly improved performance of Nullkiller AI
* Improved scoring of on-map artifacts by Nullkiller AI
* Nullkiller AI will now select artefact loadouts based on the hero's army, skills, spells, and mana points.
* Nullkiller AI will now consider building resource silos in towns.
* Fixed possible bug which could lead to AI avoiding map locations with placed events

### Stability

* Fixed a crash that could occur when winning a game by capturing a city that was set as a victory condition without first killing all enemies.
* Fixed a possible crash on some platforms when opening the creature window if the hero has equipped artefacts that provide spell immunity.
* Fixed crash when renaming preset to same name as before
* Fixed possible crash when opening mod screenshots tab without selected mod
* Fixed possible crash when loading game with broken mods active
* Fixed crash on loading some user-made maps with objects that have unknown to VCMI map object ID or subID
* Fixed crash on loading map in Wake of Gods format with pre-placed Mithril resource pile on map

### Mechanics

* Fixed war machines or units under the Bow of the Sharpshooter effect being unable to fire when blocked by enemy units.
* Enemy corpses will no longer block adjacent enemy ranged units from using ranged attacks.
* Spells banned on the map can no longer appear in towns.
* Arrow towers will now consider units standing on wall tiles as inside town for target selection
* Fixed possible integer overflow when player has more than 20 million gold or other resources.
* Fixed the loading of vcmp campaigns when a specific hero is used in the bonuses of a scenario.
* Fixed regression causing movement bonus from Stables (adventure map objects) and Stables (Castle town building) to stack with each other.
* Fixed regression causing Pathfinding skill to reduce movement costs by only 1 movement point
* Fixed bug causing hero paths not updating immediately after leveling up Pathfinding skill

### Interface

* Fixed multiple cases where town buidings were not ordered correctly and overlapping each other on town screen
* Object search functionality is now case-insensitive and can search for similar strings to protect from typos
* Thieves Guild will now show icons instead of text for resources comparison
* Added support for custom images in multiplayer mode selection
* TCP host/join dialogue now displays correct text in header
* Main menu buttons for unavailable campaigns automatically hidden
* Fixed graphical artefact near 3DO video when resolution is high and interface scaling is low

### Map Editor

* Object properties now show actual values instead of '...'.
* Hovering over object properties now shows tooltip with full value

### Modding

* It is now possible to configure amount of creatures that would join on successful diplomacy check
* It is now possible to disable joining for free for diplomacy
* It is now possible to use images with `-shadow` or `-overlay` suffixes for 1x / unscaled mode
* It is now possible to load pregenerated player-colored interface images using suffixes like `-red` or `-blue` in place of palette-based effects

## 1.6.2 -> 1.6.3

### Stability

* Fixed possible crash on attempt to play corrupted video file
* Fixed possible crash on invalid or corrupted game data
* Fixed possible crash on invalid upscaling filter

### Interface

* Added right-click popup to Keymasters, Border Guards, and Border Gates that show all discovered objects of the same color
* Added right-click popup to Obelisks that shows all discovered objects and their visitation status
* Added support for randomly selected main menu backgrounds
* Fixed display of long text in text-only right-click popups
* Hero overview screen on map setup will now have scrollbars for hero desription when necessary
* Fixed teleporter right-click popup appearing out of screen when interacting with teleporter near screen edge
* Scenario Information window will now correctly replace random hero with actual starting hero

### AI

* Improved performance of Battle AI
* Improved performance of pathfinding calculations
* Reduced calculation of threat range, especially on low difficulties to improve performance
* Fixed Nullkiller AI not considering nearby objects for visiting in some cases, breaking its exploration logic
* Fixed Nullkiller AI not building town dwellings

### Map Editor

* Added option to convert .h3c files into .vcmp
* It is now possible to configure town to have same faction as player

## 1.6.1 -> 1.6.2

### General

* Holding Alt while in town will now highlight all interactive buildings in town
* Fixed missing surrender video on battle results dialog
* Game will no longer show custom campaigns dialog for short period when selecting Heroes III campaign
* Added workaround for right mouse button being recognized as 'back' button on some Android devices
* Fixed regression that caused second and further upgrades that were not functioning correctly
* Fixed regression that sometime caused curved path that leads to a blocked monolith in random map generation
* Fixed regression that broke several mechanics, such as issues with Legion artifacts or with Gelu/Dracon specialties
* Fixed broken positioning for some images when selected scaling factor is different from prescaled texture factor

### Stability

* Game will now show list of mods with critical issues that might cause crash after game start
* Fixed crash in map editor on attempt to copy wandering monster
* Fixed crash on having unsupported mod (e.g. Era mod) in a preset
* Fixed crash on attempt to update a mod that depends on unknown mod that is not installed or not available in repository
* Fixed possible crash on attempt to flip an empty image
* Fixed possible crash on attempt to remove old saves

### Campaigns

* Game will now select correct scenario in campaigns with multiple available scenarios
* Added blink animation for campaign scenario selection

### AI

* Nullkiller AI will now only upgrade units if this would increase their AI value. Fixes possible freeze on upgrading units in some mods that can be upgraded in both directions
* Nullkiller AI will no longer attempt to build Mage Guild of 4th or 5th levels in towns without such guild
* Battle AI now correctly estimates damage for spells that completely eliminate a unit
* Fixed bug preventing AI from casting damaging spells if his troops are unable to reach enemies, for example during siege

## 1.6.0 -> 1.6.1

### General

* Right-click popup for Monoliths, Subterranean Gates and Whirlpools now shows location of all known entrances and exits.
* Added support for importing and exporting mod presets in the launcher.
* Added option to VCMI popup menu on Android to skip launcher and start game immediately.
* Fixed defeat music not stopping when skipping defeat movie playback.
* Launcher will now open start game tab instead of mods tab after initial setup if no mods were chosen for install

### Performance

* xbrz is no longer auto-selected on mobile platforms, and only xbrz2 can be auto-selected on PC platforms. Manual selection is unaffected.
* Fixed a performance regression that more than doubled the time of random map generation.
* Improved performance of the Nullkiller AI, which should now take turns up to twice as fast.
* Minor xbrz performance improvements.

### Stability

* Fixed application freeze when clicking buttons with popup message on some versions of iOS
* Fixed crash when trying to install a mod when no mod is selected
* Fixed possible crash when trying to load non-existing frame from .def file
* Fixed crash when right-clicking on modded market items, such as HotA's Junkman.
* Fixed crash when trying to activate mod with recursive dependencies
* Fixed crash when trying to activate Chronicles mod after failed import
* Fixed crash when attempting to access a non-installed mod when repository checkout is off.
* Show error message on failure to load filesystem instead of crashing in launcher
* Added workaround for crash on attempt to delete non-existent save/map

### Adventure AI

* AI can now use all possible upgrades on Hill Fort, including alternative upgrades.
* AI will now correctly identify and use 8th creature dwelling (e.g. Factory)
* AI will now correctly identify 2nd level upgrades to dwellings (e.g. Cove) and score them accordingly.

### Interface

* Fixed poor alignment of hero primary skill descriptions in the Thieves' Guild menu.
* Fixed missing description for spell school immunity in creature window
* Fixed bonuses such as OPENING_BATTLE_SPELL not appearing in creature window.

## 1.5.7 -> 1.6.0

### Major changes

* Greatly improved decision-making of NullkillerAI
* Implemented support for multiple mod presets allowing player to quickly switch between them in Launcher
* Implemented handicap system, with options to reduce income and growth in addition to starting resources restriction
* Game will now show statistics after scenario completion, such as resources or army strength over time
* Implemented spell quick selection panel in combat
* Implemented adventure map overlay accessible via Alt key that highlights all interactive objects on screen
* Implemented xBRZ upscaling filter
* Added support for high-resolution graphical assets
* It is now possible to import data from Heroes Chronicles (gog.com installer only) as custom campaigns
* Added simple support for spell research feature from HotA that can be enabled via mod or game configuration editing
* Implemented automatic selection of interface scaling. Selecting interface scaling manually will restore old behavior
* VCMI will now launch in fullscreen on desktop systems. Use F4 hotkey or toggle option in settings to restore old behavior

### General

* Saved game size reduced by approximately 3 times, especially for large maps or games with a large number of mods.
* Mods that modify game texts, such as descriptions of secondary skills, will now correctly override translation mods
* Game will now correctly restore information such as hero path, order of heroes and towns, and list of sleeping heroes on loading a save game
* Added translation for missing texts, such as random map descriptions, quick exchange buttons, wog commander abilities, moat names
* When playing in non-English language using English Heroes III data files, game will now load all maps and campaigns using player language
* Added `vcmiscrolls` cheat code that gives spell scrolls for every possible spells

### Multiplayer

* Added option to start vcmi server on randomly selected TCP port
* Fixed potential desynchronization between server and clients on randomization of map objects if client and server run on different operating systems
* Fixed possible freeze on receiving turn in multiplayer when player has town window opened
* Fixed possible freeze if player is attacked by another player on first day of the week
* If player disconnects from a multiplayer game, all other players will now receive notification in form of popup message instead of chat message
* Fixed potentially missing disconnection notification in multiplayer if player disconnects due to connection loss
* Game will now correctly show turn timers and simultaneous turns state on loading game

### Stability

* Fixed possible crash on connecting bluetooth mouse during gameplay on Android
* VCMI will now write more detailed information to log file on crash due to uncaught exception
* Fixed crash on transfer of multiple artifacts in a backpack to another hero on starting next campaign scenario without hero that held these artifacts before
* Fixed crash on dismissing hero after picking up an artifact from hero doll
* Fixed possible crash if creature with spell before attack bonus kills unit it was attacking with spell

### Mechanics

* Arrow tower will now prefer to attack more units that are viewed most dangerous instead of simply attacking top-most unit
* Score in campaigns will now be correctly tracked for games loaded from a save
* Fixed incorrect direction of Dragon Breath attack in some cases if wide creature attacks another wide creature
* Map events and town events are now triggered on start of turn of player affected by event, in line with H3 instead of triggering on start of new day for all players
* Neutral towns should now have initial garrison and weekly growth of garrison identical to H3
* It is now possible to buy a new war machine in town if hero has different war machine in the slot
* Fixed possible integer overflow if hero has unit with total AI value of over 2^31
* Unicorn Glade in Rampart now correctly requires Dendroid Arches and not Homestead
* Game will no longer place obstacles on ship-to-ship battles, in line with H3
* Game will now place obstacles in battles in villages (towns without forts)
* Battles in villages (towns without forts) now always occur on battlefield of native terrain
* Fixed pathfinding through subterranean gates located on right edge of the map or next to terra incognita
* Chain Lightning will now skip over creatures that are immune to this spell instead of targeting them but dealing no damage
* Commanders spell resistance now uses golem-like logic which reduces damage instead of using dwarf-style change to block spell
* It is now possible to target empty hex for shooters with area attack, such as Magog or Lich
* View Earth will no longer reveal position of enemy heroes and towns
* It is now possible to sell Grail, in line with Heroes III
* Jeddite is no longer female
* Mutare and Mutare Drake are now Overlord and not Warlock
* Elixir of Life no longer affects siege machines
* Banned skills known by hero now have minimal chance (1) instead of 0 to appear on levelup
* The Transport Artifact victory condition fulfilled by the enemy AI will no longer trigger a victory for human players if "standard victory" is enabled on the map

### Video / Audio

* Fixed playback of audio stream with different formats from video files in some Heroes 3 versions
* Video playback will not be replaced by a black square when another dialogue box is on top of the video.
* When resuming video playback, the video will now be continued instead of being restarted.
* Reduced video decompression artefacts for video formats that store RGB rather than YUV data.
* Intro videos are now played inside a frame on resolutions higher than 800x600 instead of filling entire screen
* Re-enabled idle animations for Conflux creatures in battles
* .webm video with vp8 / vp9 codec are now supported on every platform
* It is now possible to provide external audio stream for a video file (e.g. for translations)
* It is now possible to provide external subtitles for a video file
* Game will now correctly resume playback of terrain music on closing scenario information window in campaigns instead of playing main theme
* Background music theme will now play at lower volume while intro speech in campaign intro / outro is playing
* Added workaround for playback of corrupted `BladeFWCampaign.mp3` music file
* Fixed computation of audio length for formats other than .wav. This fixes incorrect text scrolling speed in campaign intro/outro
* Game will now use Noto family true type font to display characters not preset in Heroes III fonts
* Added option to scale all in-game fonts when scalable true type fonts are in use
* Some of the assets provided by VCMI are now available in higher resolution
* Implemented support for semi-transparent spell effects, such as Life Drain or Resurrection (and many others)

### Interface

* It is now possible to search for a map object using Ctrl+F hotkey
* Holding Alt while using move unit button on Exchange screen will now move entire army except for single unit in this slot
* Added option to drag map with right-click
* Added hotkeys to reorder list of owned towns or heroes
* The number of units resurrected using the Life Drain ability is now written to the combat log.
* Fixed order of popup dialogs after battle.
* Creature window now displays source of all bonuses that creature has, such as creature ability, hero skill, hero artifact, etc.
* Reduced font used for creature abilities description to reduce text clipping
* Right-click on wandering monster on adventure map will now also show creature level and faction it belongs to
* Added additional information to map right-click popup dialog: map author, map creation date, map version
* Added scrollbars for selection of starting town, starting hero, and tavern invite if number of objects is too large to fit into the screen
* Fixed incorrect battle turn queue displaying incorrect turn order when all units have waited
* Semi-transparent shadows now correctly update their transparency during fading effects, such as resource pickups
* Fixed swapped Overlord and Warlock models on adventure map
* Fixed Heroes III bug - swapped icons of View Earth and View Air
* Game will now save all names for human player in hotseat mode
* Added unassigned by default shortcuts for toggling visibility of visitable and blocked tiles
* Spellbook button in battle is now blocked if hero has no spellbook
* Adventure map will no longer scroll if window is not in focus
* Removed second info window when player loses his last town
* Fixed hero path not updating correctly after hiring or dismissing creatures
* Fixed missing description of a stack artifact when accessed through unit window
* Fixed text overflow on campaign scenario window if campaign name is too long
* Recruiting hero in town will now play "new construction" sound
* Game will now correctly update displayed hero path when hiring or dismissing creatures that give movement penalty
* Game will now show level, faction and attack range of wandering monsters in right-click popup window
* Hovering over owned hero will now show movement points information in status bar
* Quick backpack window is now also accessible via Shift+mouse click, similar to HD Mod
* It is now possible to sort artifacts in backpack by cost, slot, or rarity class
* Fixed incorrect display of names of VCMI maps in scenario selection if multiple VCMI map are present in list
* Fixed bug leading to inability to select larger number of "CPU only players" in random map generation menu
* It is now possible to delete saved games
* Game will now promt to delete saves from no longer supported versions of VCMI
* It is now possible to use scroll in touch popup windows
* Name of spell provided by Shrine is now displayed in yellow color
* Fixed right-click popup on hero in town placed outside of screen boundaries on low resolutions
* Fixed misaligned button in 2-player alliance selector in random map generation menu
* Damage range of Ballista in unit window now accounts for hero attack skill, in line with Heroes III
* Changed format of automatic autosave to more human-readable version
* Names of autosave folders are now left-aligned in save game list

### Random Maps Generator

* Implemented connection option 'forcePortal'
* It is now possible to connect zone to itself using pair of portals
* It is now possible for a random map template to change game settings
* Road settings will now be correctly loaded when opening random map setup tab
* Roads placed on the maps will now be curved a little bit to improve the look of the maps.
* Added support for banning objects per zones
* Added support for customizing objects frequency, value, and count per zone
* Fixed values of Pandora Boxes with creatures to be in line with H3:SoD
* Added sealed zone types, for entirely unpassable zones.
* It is now possible to connect two zones with multiple connections of same or different types

### Campaigns

* It is now possible to use .zip archive for VCMI campaigns instead of raw gzip stream
* Fixed handling of hero placeholders in VCMI map format (.vmap)
* Fixed not functioning hero carryover in VCMI campaigns
* Added support for campaign outro videos, such as outro in "Song for the Father" campaign
* Added support for rim image for intro video, such as opening videos in Heroes Chronicles
* Added support for custom loading screen in campaigns
* Added support for custom region definitions (such as background images) for VCMI campaigns

### Adventure AI

* Greatly improved decision-making of NullkillerAI
* NullkillerAI will now act differently based on difficulty level
* Fixed several cases where Nullkiller AI can count same dangerous object twice, doubling expected army loss.
* Nullkiller is now capable of visiting configurable objects from mods
* Nullkiller now uses whirlpools for map movement
* Fixed possible crash on AI attempting to visit town that is already being visited by this hero
* It is now possible to configure NullkillerAI parameters separately for each game difficulty
* Extended hardcoded logic of AI not taking creatures from Garrisons to all Heroes III: Restoration of Erathia campaigns, in line with original game

### Combat AI

* VCMI will now use BattleAI for battles with neutral enemies by default
* Fixed bug where BattleAI attempts to move double-wide unit to an unreachable hex
* Fixed a bug causing BattleAI to focus on unreachable targets and ignoring reachable enemies
* AI can now correctly estimate effect of Dragon Breath and other similar abilities
* Battle AI should now avoid ending turn on the moat
* Fixed case where BattleAI will go around the map to attack ranged units if direct path is blocked by another unit
* Fixed evaluation of effects of waiting if unit is under haste effect by Battle AI
* Battle AI can now use location spells
* Battle AI will now correctly avoid moving flying units into dangerous obstacles such as moat

### Launcher

* Implemented support for multiple mod presets allowing player to quickly switch between them
* Added new Start Game page to Launcher which is now used when starting the game
* Added option to create empty mod preset to quickly disable all mods
* Added button to update all installed mods to Start Game page
* Added diagnostics to detect common issues with Heroes III data files
* Added built-in help descriptions for functionalities such as data files import to better explain them to players
* It is now always possible to disable or uninstall a mod. Any mods that depend on this mod will be automatically disabled
* It is now always possible to update a mod, even if there are mods that depend on this mod.
* It is now possible to enable mod that conflicts with already active mod. Conflicting mods will be automatically disabled
* If main mod is disabled, all its submods will have their active or inactive status shown as greyed-out for clarity
* If mod depends or conflicts with a submod, Launcher will now also show name of parent mod in list of dependencies / conflicts
* Game will now cache result of mod repository checkout and restore it immediately on next start. This removes flickering when game fills list of available mods.
* Screenshot and Changelog tabs in mod description are now disabled for mods that do not have them.
* Launcher will now correctly show conflicts on both sides - if mod A is marked as conflicting with B, then information on this conflict will be shown in description of both mod A and mod B (instead of only in mod B)
* Added Swedish translation
* Added better diagnostics for gog installer extraction errors
* It is no longer possible to start installation or update for a mod that is already being downloaded
* Fixed detection of existing Heroes III Complete or Shadow of Death data files during import

### Map Editor

* It is now possible to remove any map object as part of timed event
* Implemented tracking of building requirements for Building Dialog
* Added build/demolish/enable/disable all buildings options to Building Dialog in town properties
* Added support for customization of heroes artifacts
* Implemented configuration of patrol radius for heroes
* It is now possible to set spells allowed or required to be present in Mages Guild
* It is now possible to add timed events to a town
* Fixed editor not marking mod as dependency if spells from mod are used in town Mages Guild or in hero starting spells
* It is now possible to choose road types for random map generation in editor
* Validator will now warn in case if map has players with no heroes or towns
* Fixed broken transparency handling on some adventure map objects from mods
* Fixed duplicated list of spells in Mage Guild in copy-pasted towns
* Removed separate versioning of map editor. Map editor now has same version as VCMI
* Timed events interfaces now counts days from 1, instead of from 0
* Added Recent Files to File Menu and Toolbar
* Fixed crash on attempting to save map with random dwelling

### Modding

* Json configuration files from different mods no longer can override each other to reduce possibility of a file name clash
* Game will now load high-resolution assets when xbrz upscaler is in use from Data2x, Data3x, Data4x, or Sprites2x, Sprites3x, Sprites4x directories.
* Game will now load high-resolution movies when xbrz upscaler is in use from Video2x, Video3x, Video4x directories
* Added support for configurable flaggable objects that can provide bonuses or daily income to owning player
* Added support for soft dependencies for mods, that only affect mod loading order (and as result - override order), without requiring dependent mod or allowing access to its identifiers
* It is now possible to provide translations for mods that modify strings from original game, such as secondary skill descriptions
* It is now possible to embed json data directly into mod.json instead of using list of files
* Implemented detection of potential conflicts between mods. To enable, open Launcher and set "Mod Validation" option to "Full"
* Fixed multiple issues with configurable town buildings
* Added documentation for configurable town buildings. See docs/Moddders/Entities_Format/Town_Buildings_Format.md
* Replaced some of hardcoded town buildings with configurable buildings. These building types are now deprecated and will be removed in future.
* Added support for explicitly visitable town buildings that will activate only on click and not on construction or on hero visit (Mana Vortex from HotA)
* It is now possible to add guards to a configurable objects. All H3 creature banks are now implemented as configurable object.
* It is now possible to define starting position of units in a guarded configurable object
* It is now possible to add description to an object with "market" handler
* Added `canCastWithoutSkip` parameter to a spell. If such spell is cast by a creature, its turn will not end after a spellcast
* Added `castOnlyOnSelf` parameter to a spell. Creature that can cast this spell can only cast it on themselves
* Mod can now provide pregenerated assets in place of autogenerated, such as large spellbook.
* Added support for 'fused' artifacts, as alternative to combined artifacts
* Added support for custom music and opening sound for a battlefield
* Added support for multiple music tracks for towns
* Added support for multiple music tracks for terrains on adventure map
* Fixed several cases where vcmi will report errors in json without specifying filename of invalid file
* Fixed selection of gendered sprites for heroes on adventure map
* It is now possible to change minimal values of primary skills for heroes
* Added support for HotA Bank building from Factory
* Added support for HotA Grotto buiding from Cove
* Added support for HotA-style 8th creature in town
* Town building can now define war machine produced in this building (Blacksmith or Ballista Yard)
* Town building can now define provided fortifications - health of walls, towers, presence of moat, identifier of creature shooter on tower
* War Machines Factory no longer unconditionally contain war machines from the original game, allowing mods to define list of war machines from scratch
* Added MECHANICAL bonus
* Added DISINTEGRATE bonus
* Added INVINCIBLE bonus
* Added PRISM_HEX_ATTACK_BREATH bonus
* Added THIEVES_GUILD_ACCESS bonus that changes amount of information available in thieves guild
* TimesStackLevelUpdater now supports commanders
* Black market restock period setting now correctly restocks on specified date instead of restocking on all dates other than specified one
* Json Validator will now attempt to detect typos when encountering unknown property in Json
* Added `translate missing` command that will export only untranslated strings into `translationsMissing` directory, separated per mod
* Added support for text subtitiles for video files
* Added validation of objects with "market" and "flaggable" handlers
* Added "special" property for secondary skills

## 1.5.6 -> 1.5.7

* Fixed game freeze if player is attacked in online multiplayer game by another player when he has unread dialogs, such as new week notification
* Fixed possible game crash after being attacked by enemy with artifact that blocks spellcasting
* Fixed heroes on map limit game setting not respected when moving hero from town garrison.
* Add workaround to fix possible crash on attempt to start previously generated random map that has players without owned heroes or towns
* Fixed crash on right-clicking spell icon when receiving unlearnable spells from Pandora
* Fixed possible text overflow in match information box in online lobby
* Fixed overlapping text in lobby login window
* Fixed excessive removal of open dialogs such as new week or map events on new turn
* Fixed objects like Mystical Gardens not resetting their state on new week correctly

## 1.5.5 -> 1.5.6

### Stability

* Fixed possible crash on transferring hero to next campaign scenario if hero has combined artifact some components of which can be transferred
* Fixed possible crash on transferring hero to next campaign scenario that has creature with faction limiter in his army
* Fixed possible crash on application shutdown due to incorrect destruction order of UI entities

### Multiplayer

* Mod compatibility issues when joining a lobby room now use color coding to make them less easy to miss.
* Incompatible mods are now placed before compatible mods when joining lobby room.
* Fixed text overflow in online lobby interface
* Fixed jittering simultaneous turns slider after moving it twice over short period
* Fixed non-functioning slider in invite to game room dialog

### Interface

* Fixed some shortcuts that were not active during the enemy's turn, such as Thieves' Guild.
* Game now correctly uses melee damage calculation when forcing a melee attack with a shooter.
* Game will now close all open dialogs on start of our turn, to avoid bugs like locked right-click popups

### Map Objects

* Spells the hero can't learn are no longer hidden when received from a rewardable object, such as the Pandora Box
* Spells that cannot be learned are now displayed with gray text in the name of the spell.
* Configurable objects with scouted state such as Witch Hut in HotA now correctly show their reward on right click after vising them but refusing to accept reward
* Right-click tooltip on map dwelling now always shows produced creatures. Player that owns the dwelling can also see number of creatures available for recruit

### Modding

* Fixed possible crash on invalid SPELL_LIKE_ATTACK bonus
* Added compatibility check when loading maps with old names for boats

## 1.5.4 -> 1.5.5

* Fixed crash when advancing to the next scenario in campaigns when the hero not transferring has a combination artefact that can be transferred to the next scenario.
* Fixed game not updating information such as hero path and current music on new day
* Changed default battle queue hotkey from Q to Z to match HD Mod / HotA
* Changed default hotkey for finishing battle with quick combat from E to Z to match HD Mod / HotA
* Creature casting now uses both F and G keyboard hotkeys
* Shift+left click now directly opens the hero window when two heroes are in town
* Fixed handling of alternative actions for creatures that have more than two potential actions, such as move, shoot, and cast spells.

## 1.5.3 -> 1.5.4

### Stability

* Fixed a possible crash when clicking on an adventure map when another player is taking a turn in multiplayer mode.
* Failure to extract a mod will now display an error message instead of a silent crash.
* Fixed crash on opening town hall screen of a town from a mod with invalid building identifier
* Fixed crash when faerie dragons die after casting Ice Ring on themselves.

### Mechanics

* The scholar will now correctly upgrade a skill if the visiting hero has offered a skill at either the basic or advanced level.
* Hero now reveals Fog of War when receiving new or upgraded secondary skills (such as scouting).
* AI will now always act after all human players during simturns instead of acting after host player

### Interface

* Pressing the up and down keys on the town screen will now move to the next or previous town instead of scrolling through the list of towns.
* Long text in scenario name and highscore screen now shortened to fit the interface
* Game now moves cursor to tap event position when using software cursor with touch screen input
* Right-click popup on spell scroll campaign bonus now shows spell name instead of artefact name
* Damage estimation tooltip will no longer show damage greater than the targeted unit's health.

### Random Maps Generator

* Generator will try to place roads even further away from zone borders
* Fixed rare crash when placing two quest artefacts in the same location at the same time

### AI

* Improved performance of Nullkiller AI
* Stupid AI no longer overestimates damage when killing entire unit
* Fixed a bug leading to Battle AI not using spells when sieging town with Citadel or Castle built
* Fixed an unsigned integer overflow that caused the Nullkiller AI to overestimate the total army strength after merging two armies.

### Launcher

* Added button to reset touchscreen tutorial on mobile systems
* Launcher will now warn if player selects Gog Galaxy installer instead of offline installer
* Launcher will now ask for the .bin file first as it is usually listed first in the file system view
* Extraction failure now displays error message instead of crashing
* Launcher will now use the header signature to check the file type instead of the extension when using the gog.com installer.
* Fixed broken controller sensitivity configuration options
* Fixed manual file installation on Android

### Map Editor

* Icons and translations now embedded in executable file

### Modding

* Improved bonus format validation
* Validator now reports valid values for enumeration fields
* Fixed missing addInfo field for bonuses that use the BONUS_OWNER_UPDATER propagation updater.

## 1.5.2 -> 1.5.3

### Stability

* Fixed possible crash when hero class has no valid commander.
* Fixed crash when pressing spacebar or enter during combat when hero has no tactics skill.
* Fixed crash when receiving a commander level-up after winning a battle in a garrison owned by an enemy player.
* Fixed possible crash when exiting a multiplayer game.
* Game will now display an error message and exit after loading instead of crashing silently if a creature's combat animation is missing.
* Game should now generate crash dump on uncaught c++ exception throw
* Fixed crash when player finishes game with negative score
* Fixed crash when opening tavern window in some localisations
* Fixed crash on loading previously generated random map when mods that add object with same name are used
* Game will now display an error message instead of silent crash if game data directory is not accessible

### Mechanics

* Transport Artefact victory condition will no longer trigger if another player has completed it.
* Fixed wandering monster combat not triggering when landing in its zone of control when flying from above the monster using the Fly spell.
* Fixed potentially infinite movement loop when the hero has Admiral's Hat whirlpool immunity and the hero tries to enter and exit the same whirlpool.
* If game picks gold for a random resource pile that has predetermined by map amount, its amount will be correctly multiplied by 100
* Fixed hero not being able to learn spells from a mod in some cases, even if they are available from the town's mage guild.
* The game will now actually take resources from seers' huts with the Gather Resources mission instead of awarding them.
* Heroes with double spell points will no longer trigger the Mana Vortex.
* If turn timer runs out during pve battle game will end player turn after a battle instead of forcing retreat

### Interface

* Fixed reversed button functions in Exchange Window
* Fixed allied towns being missing from the list when using the advanced or expert Town Portal spell.
* Fixed corrupted UI that could appear for a frame under certain conditions
* The '*' symbol and non-printable characters can no longer be used in savegames due to Windows file system restrictions.
* Pressing Ctrl while hovering over the adventure map will now display tile coordinates in the status bar.
* Selection of another hero while hero is selected now requires Shift press instead of Ctrl
* Fixed hero troops in the info box view flashing briefly during hero movement.
* Reduced excessive memory usage on adventure map by several hundreds of megabytes (most noticeable on systems with large screen resolution)
* Haptic feedback is now enabled by default on Android and on iOS
* It is now possible to scroll through artifacts backpack using mouse wheel or swipe

### Launcher

* Android now uses the same Qt-based launcher as other systems
* Fixed attempt to install a submod when installing new mod that depends on a submod of another mod
* Fixed wrong order of activating mods in chain when installing multiple mods at once
* Mod list no longer shows mod version column. Version is now only shown in the mod description.
* Launcher will now skip the Heroes 3 data import step if data has been found automatically
* Fixed import of existing data files on iOS. This option now requires iOS 13 or later
* Fixed import using offline installer on iOS.
* Buttons to open data directories in the Help tab are now hidden on mobile systems if they can't be opened with file browser
* Added the configuration files directory to the Help tab as it is located separately on Linux systems
* Removed H3 data language selection during setup in favor of auto-detection
* Replaced checkboxes with toggle buttons for easier of access on touchscreens.
* Icons and translations now embedded in executable file
* Added interface for configuring several previously existing but inaccessible options in Launcher:
  * Selection of input tolerance precision for all input types
  * Relative cursor mode for mobile systems (was only available on Android)
  * Haptic feedback toggle for mobile systems (was only available on Android)
  * Sound and music volume (was only available in game)
  * Selection of long touch interval (was only available in game)
  * Selection of upscaling filter used by SDL
  * Controller input sensitivity and acceleration.

### AI

* Fixed crash when Nullkiller AI tries to explore after losing the hero in combat.
* Fixed rare crash when Nullkiller AI tries to use portals
* Fixed potential crash when Nullkiller AI has access to Town Portal spell
* Fixed potential crash when Battle AI selects a spell to cast from a hero with summon spells.
* Several fixes to Nullkiller AI exploration logic
* Fixed bug leading to Battle AI doing nothing if targeted unit is unreachable

### Random Maps Generator

* Fixed crash when player selects a random number of players and selects a different colour to play, resulting in a non-continuous list of players.
* Fixed rare crash when generating maps with water

### Map Editor

* Fixed crash on closing map editor

### Modding

* Added new building type 'thievesGuild' which implements HotA building in Cove.
* Creature terrain limiter now actually accepts terrain as parameter

## 1.5.1 -> 1.5.2

### Stability

* Fixed crash on closing game while combat or map animations are playing
* Fixed crash on closing game while network thread is waiting for dialog to be closed
* Fixed random crash on starting random map with 'random' number of players
* Fixed crash caused by thread races on loading map list
* Failure to read data from network connection will show up as 'disconnection' and not as a crash
* Fixed a possible crash when replaying a manually played battle with the 'unlimited battle replay' option set
* Fixed crash when loading save made on a 64-bit system or connecting to multiplayer game with a 64-bit host on a 32-bit system (and vice versa)
* Fixed crash when ending a battle in a draw when a hero has the Necromancy skill
* Fixed crash when having SPELL_LIKE_ATTACK bonus with invalid spell ID
* Fixed transfer of non-first artefacts in backpack if hero does not transfer as well
* Game will now abort loading if a corrupt mod is detected instead of crashing without explanation later

### Multiplayer

* Contact between allied players will no longer break simturns
* Having hero in range of object owned by another player will now be registered as contact
* Multiplayer saves are now visible when starting a single player game
* Added chat command '!vote' to initiate a vote to change the duration of simultaneous turns or to change turn timers
* Added chat command '!help' to list all available chat commands
* All multiplayer chat commands now use a leading exclamation mark

### Campaigns

* If the hero attacks an enemy player and is defeated, he will be correctly registered as defeated by the defending player.
* Allow standard victory condition on 'To kill a hero' campaign mission in line with H3
* Fixes Adrienne starting without Inferno spell in campaign

### Interface

* For artefacts that are part of a combined artefact, the game will now show which component of that artefact your hero has.
* Fixed broken in 1.5.1 shortcut for artifact sets saving
* Fixed full screen toggle (F4) not applying changes immediately
* Retaliation preview now accounts for creatures that don't receive retaliations (Sprites, Archdevils, etc)
* Fixed not visible retaliation preview if damage estimation string is longer than battle log line due to long creature name
* Game will now select last save on loading screen
* High Scores screen and Campaign Epilogue screen are now displayed with background on resolutions higher than 800x600
* Fixed non-functioning shortcut 'P' to access Puzzle Map from adventure map
* Added keyboard shortcuts to markets and altars. 'Space' to confirm deal and 'M' to trade maximum possible amount
* Pressing 'Escape' in main menu will now trigger 'Back' and 'Quit' buttons
* Added keyboard shortcuts to hero exchange window:
  * 'F10' will now swap armies
  * 'F11' will now swap artifacts. Additionally, 'Ctrl+F11' will swap equipped artifacts, and 'Shift+F11' will swap backpacks
  * Added unassigned shortcuts to move armies or artifacts to left or right side
* Added keyboard shortcuts to access buildings from town interface:
  * 'F' will now open Fort window
  * 'B' will now open Town Hall window
  * 'G' will now open Mage Guild window
  * 'M' will now open Marketplace
  * 'R' will now open recruitment interface
  * 'T' will now open Tavern window
  * 'G' will now open Thieves Guild
  * 'E' will now open hero exchange screen, if both heroes are present in town
  * 'H' will now open hero screen. Additionally, 'Shift+H' will open garrisoned hero screen, and 'Ctrl+H' will open visiting hero screen
  * 'Space' will now swap visiting and garrisoned heroes
* Added keyboard shortcuts to switch between tabs in Scenario Selection window:
  * 'E' will open Extra Options tab
  * 'T' will open Turn Options tab
  * 'I' will open Invite Players window (only for lobby games)
  * 'R' will now replay video in campaigns
* Added keyboard shortcuts to Adventure map:
  * 'Ctrl+L' will now prompt to open Load Game screen
  * 'Ctrl+M' will now prompt to go to main menu
  * 'Ctrl+N' will now prompt to go to New Game screen
  * 'Ctrl+Q' will now prompt to quit game
  * Page Up, Page Down, Home and End keys will now move hero on adventure map similar to numpad equivalents
  * Fixed non-functioning shortcuts '+' and '-' on numpad to zoom adventure map
* Added keyboard shortcuts to Battle interface:
  * 'V' now allows to view information of hovered unit
  * 'I' now allows to view information of active unit

### Mechanics

* Game will no longer pick creatures exclusive to AB campaigns for random creatures or for Refugee Camp, in line with H3
* If original movement rules are on, it is not possible to attack guards from visitable object directly, only from free tile
* Fixed bug leading that allowed picking up objects while flying on top of water
* Hero can now land when flying from guarded tile to accessible guarded tile irregardless of original movement rules switch
* Interface will now use same arrow for U-turns in path as H3

### AI

* Nullkiller AI can now explore the map
* Nullkiller AI will no longer use the map reveal cheat when allied with a human or when playing on low difficulty
* Nullkiller AI is now used by default for allied players

### Launcher

* When extracting data from gog.com offline installer game will extract files directly into used data directory instead of temporary directory

### Map Editor

* Fixed victory / loss conditions widget initialization

### Modding

* Hero specialties with multiple bonuses that have TIMES_HERO_LEVEL updater now work as expected
* Spells that apply multiple bonuses with same type and subtype but different value type now work as expected
* Added option to toggle layout of guards in creature banks

## 1.5.0 -> 1.5.1

### Stability

* Fixed possible crash on accessing faction description
* Fixed possible thread race on exit to main menu
* Game will now show error message instead of silent crash on corrupted H3 data
* Fixed possible crash on double-deletion of quest artifacts placed by RMG
* Fixed crash on loading save made in version 1.4 with removed from map Quest Guards
* Added workaround for crash on accessing Altar of Sacrifice on saves made in 1.4
* Fixed possible crash on map restart request
* Fixed crash on attempt to open scenario list with no save or map selected
* Fixed crash on host resolving error when connecting to online lobby
* If json file specified in mod.json is missing, vcmi will now only log an error instead of crashing

### Interface

* Added retaliation damage and kills preview when hovering over units that can be attacked in melee during combat
* Clicking on combat log would now open a window with full combat log history
* Removed message length limit in text input fields, such as global lobby chat
* Tapping on already active text input field will display on-screen keyboard on systems with one
* Fixed possible freeze when trying to move hero if hero has non-zero movement points but not enough to reach first tile in path
* Fixed selection of the wrong reward in dialogs such as the level-up window when double-clicking on it
* Fixed launch of wrong map or save when double-clicking in scenario list screen
* Right-clicking on a hero in a tavern will now select that hero as well, in line with H3
* Fixed slow map list parsing when hota map format is enabled
* MacOS and iOS can now use either Ctrl or Cmd key for all keyboard shortcuts
* Small windows no longer dim the entire screen by default

### Mechanics

* Recruiting a hero will now immediately reveal the fog of war around him
* When both a visiting hero and a garrisoned hero are in town, the garrisoned hero will visit town buildings first.

### Multiplayer

* Fixed in-game chat text not being visible after switching from achannel with a long history
* Fixed lag when switching to channel with long history
* Game now automatically scrolls in-game chat on new messages
* Game will now only plays chat sound for active channel and for private channels
* Cheats are now disabled by default in multiplayer
* Game will now show status of cheats and battle replays on map start
* It is possible to change cheats or battle replay on game loading
* It is now possible to join rooms hosted by different hotfix versions, e.g. 1.5.1 can join 1.5.0 games
* Fixed game rooms remaining visible in the lobby even after they have been closed
* Fixed possible lag when there is a player in lobby with a very slow (or dying) connection
* Game will show correctly if player has been invited into a room
* Fixed overflow in invite window when there are more than 8 players in the lobby

### Random Maps Generator

* Generator will now prefer to place roads away from zone borders

### AI

* Fixed possible crash when Nullkiller AI tries to upgrade army
* Nullkiller AI will now recruit new heroes if he left with 0 heroes
* AI in combat now knows when an enemy unit has used all of its retaliations.

### Map Editor

* Fixed setting up hero types of heroes in Prisons placed in map editor
* Fixed crash on setting up Seer Hut in map editor
* Added text auto-completion hints for army widget
* Editor will now automatically add .vmap extensions when saving map
* Fixed text size in map validation window

## 1.4.5 -> 1.5.0

### General

* Added Portuguese (Brazilian) translation
* Added basic support for game controllers
* Added option to disable cheats in game
* Game will no longer run vcmiserver as a separate process on desktop systems
* Game will no longer show server error messages in game chat in release builds
* Implemented switchable artifact sets from HD Mod

### Stability

* Fixed possible crash in Altar of Sacrifice
* Fixed possible crash on activation of 'Enchanted' bonus
* Fixed possible race condition on random maps generation on placement treasures near border with water zone
* Fixed crash on missing video files
* Fixed crash on using healing spell as 'casts before/after attack' bonus
* Fixed crash on defeating hero that was located in boat on game start
* Fixed possible crash on turn timer running out while player has town screen open
* Fixed crash when player has manual control of arrow towers during siege
* Fixed crash on attempt to attack with Magma Elementals with Erdamon as hero
* Fixed crash on attempt to access removed Quest Guard
* Fixed crash on moving through whirlpool when hero has no troops other than commander
* Fixed possible freeze when moving hero over events that give enough experience to cause a level-up
* Fixed possible crash on movement of double-wide creatures next to gates during siege
* Fixed possible hanging app on attempt to close game during loading

### Multiplayer

* Game map will no longer be locked during turn of other human players, allowing to change hero paths or inspect towns or heroes
* Game will now correctly block most of player actions outside of their turn
* Implemented new lobby, available in game with persistent accounts and chat
* Removed old lobby previously available in launcher
* Fixed potential crash that could occur if two players act at the very same time
* Game will no longer pause due to network lag after every tile when instant movement speed is selected in multiplayer
* Game will now show "X player's turn" dialog on new turn in online multiplayer
* Fixed loading of turn timers state from saved games
* Simultaneous turns will now break when players are 1 turn away from each other instead of 2 turns
* Implemented rolling and banning of towns before game start

### Interface

* Implemented configurable keyboard shortcuts, editable in file config/shortcutsConfig.json
* Fixed broken keyboard shortcuts in main menu
* If UI Enhancements are enabled, the game will skip confirmation dialogs when entering owned dwellings or refugee camp.
* It is now possible to move artifact to or from backpack using Alt+click
* It is now possible to transfer artifact to another hero during exchange using Ctrl+click
* It is no longer possible to start single scenario by pressing "Enter", in line with H3 and to prevent interference with game chat
* Empty treasure banks will no longer ask for confirmation when entering
* Game will now save last used difficulty settings
* Opening random map tab or scenario selection tab in pregame will no longer reset starting towns or heroes unless different map was selected
* Town Portal dialog will now show town icons
* Town Portal dialog will now show town info on right click
* Town Portal dialog will center on town on clicking it
* Town Portal dialog now uses same town ordering as in adventure map interface
* Game will now remember scrolling position of hero backpack
* Heroes can now be recruited from the tavern by double-clicking on them
* Added status bar to the backpack window
* Quick backpack window is now only available when enabled Interface enhancements
* Fixed assembly of artifacts in the backpack when backpack is full
* Attempt to use enemy turn replay feature will now show "Not implemented" message
* It is now possible to configure size of small battle queue in config file
* Opening hero window in town will now open exchange dialog if there are two heroes in town, allowing artifact exchange
* Fixed positioning of FPS counter after resolution change
* It is now possible to access extra options window from campaigns startup dialog
* Size of message boxes should now match H3 better. Maximum-size message box will always be smaller than screen size
* If monsters are willing to join for money, game will now show gold icon in this dialog box
* Fixed visual duplication of artifacts on Altar of Sacrifice
* Fixed translation of some bonuses using incorrect language
* Added option to use 'nearest' rounding mode for UI scaling
* Fixed various minor bugs in trade window interface
* Removed animation of spawning of every single new monster on new month
* Game will now correctly reset artifact drag-and-drop cursor if player opens another dialog on top of hero window
* If player has no valid saves, game will pick "NEWGAME" as proposed save name instead of empty field
* Fixed incorrect visitation sounds of Crypt, Shipwreck and Abandoned Ship
* Fixed double sound playback on capturing mines
* Recruitment costs that consist from 3 different resources should now fit recruitment window UI better

### Campaigns

* Game will now correctly track who defeated the hero or wandering monsters for related quests and victory conditions
* Birth of a Barbarian: Yog will now start the third scenario with Angelic Alliance in his inventory
* Birth of a Barbarian: Heroes with Angelic Alliance components are now considered to be mission-critical and can't be dismissed or lost in combat
* Birth of a Barbarian: Yog can no longer purchase spellbook from the Mage Guild
* Birth of a Barbarian: Yog will no longer gain Spellpower or Knowledge when leveling up
* Birth of a Barbarian: Scenarios with mission to deliver an artifact will no longer end after just defeating enemies
* Dungeons and Devils: AI will no longer take troops from garrisons in "Fall of Steadwick" scenario, in line with H3
* Gem will now have her class set to "Sorceress" in campaigns
* Fixed missing names for heroes who have their names customized in map after being transferred to the next scenario
* Artifact transfer will now work correctly if the hero holding the transferable artifact is not also transferring
* Fixed crash on opening of some campaigns in the French version from gog.com
* Fixed crash on advancing to campaign mission in which you can pick hero as starting bonus
* It is now possible to replay the intro movie from the scenario information window
* When playing the intro video, the subtitles are now correctly synchronized with the audio
* Fixed invalid string on right-clicking secondary skill starting bonus

### Battles

* Added option to enable unlimited combat replays during game setup
* Added option to instantly end battle using quick combat (shortcut: 'e')
* Added option to replace auto-combat button action with instant end using quick combat
* Battles against AI players can now be done using quick combat
* Disabling battle queue will now correctly reposition hero statistics preview popup
* Fixed positioning of unit stack size label

### Mechanics

* It is no longer possible to learn spells from Pandora or events if hero can not learn them
* Fixed behavior of 'Dimension Door' spell to be in line with H3:SoD
* Fixed behavior of 'Fly' spell to be in line with H3:SoD
* If it is not possible to cast 'Dimension Door', game will show message immediately on picking spell in spellbook
* Added options to configure 'Dimension Door' spell to be in line with HotA
* Casting 'Town Portal' while in boat will now show correct message box instead of server error
* Game will now take mana before visiting town after casting 'Town Portal', allowing Mana Vortex to correctly replenish all mana points
* Fixed loading of negative luck and morale in events, pandoras and quests on h3m maps
* Fixed incorrect 'duplicate hero' error on loading of some vmap maps
* Fixed previously broken digging of the Grail
* Successful digging for Grail will now show correct message
* Game will now correctly update movement range after rearranging armies
* It is no longer possible for two towns with random names to have same name, just like in H3
* Creatures that were consumed by Demon Summon ability will no longer return to life after the battle
* Effects of melee-only or ranged-only spells, such as Bloodlust or Precision are no longer cumulative
* It is no longer possible to use summoning spells if such spell would summon 0 creatures
* It is now possible to assemble or disassemble artifacts while in Altar of Sacrifice
* It is no longer possible to move war machines to Altar of Sacrifice
* If HotA mod is enabled, game will no longer incorrectly replace all prisons on map with HotA version
* Fixed regression leading to large elemental dwellings being used as replacements for random dwellings

### Random Maps Generator

* Game will now save last used RMG settings in game and in editor
* Reduced number of obstacles placed in water zones
* Treasure values in water zone should now be similar to values from HotA, due to bugs in H3:SoD values
* Random map templates can now have optional description visible in random map setup
* Implemented biomes system, for more consistent and natural obstacles placement
* Implemented Penrose tiling to produce more natural zone edges
* Increased minimal density of obstacles on surface level of the map
* Decreased minimal density of obstacles on undergound level of the map
* Density of objects should now closely resemble H3 RMG
* Generator will now avoid routing road under guarded objects whenever possible
* Generator will now avoid placing guards near roads
* Generator will not place a guard near the road if it's stronger than 1/3 of max guard strength for this zone
* Interactive objects will now appear on top of static objects
* Windmill will now appear on top of all other objects

### Launcher

* Launcher now supports installation of Heroes 3 data using gog.com offline installer thanks to innoextract tool
* Fixed loading of mod screenshots if player opens screenshots tab without any preloaded screenshots
* Fixed installation of mods if it has non-installed submod as dependency
* It is now possible to import game settings using drag-and-drop
* Added button to import mods, maps, or settings in addition to drag-and-drop
* Added Spanish translation to launcher
* Added Portuguese translation to launcher

### Map Editor

* Added Chinese translation to map editor
* Added Portuguese translation to map editor
* Mod list in settings will now correctly show submods of submods
* Fixed display of resource type and quantity in mines
* Fixed inability to change object owner in editor
* Added map sizes larger than XL in map editor
* It is now possible to customize hero spells

### AI

* Fixed possible crash on updating NKAI pathfinding data
* Fixed possible crash if hero has only commander left without army
* Fixed possible crash on attempt to build tavern in a town
* Fixed counting mana usage cost of Fly spell
* Added estimation of value of Pyramid and Cyclops Stockpile
* Reduced memory usage and improved performance of AI pathfinding
* Added experimental and disabled by default implementation of object graph
* It is now possible to configure AI settings via config file
* Improved parallelization when AI has multiple heroes
* AI-controlled creatures will now correctly move across wide moat in Fortress
* Fixed system error messages caused by visitation of Trading Posts by VCAI
* Patrolling heroes will never retreat from the battle
* AI will now consider strength of town garrison and not just strength of visiting hero when deciding to attack town

### Modding

* Added new game setting that allows inviting heroes to taverns
* It is now possible to add creature or faction description accessible via right-click of the icon
* Fixed reversed Overlord and Warlock classes mapping
* Added 'selectAll' mode for configurable objects which grants all potential rewards
* It is now possible to use most of json5 format in vcmi json files
* Main mod.json file (including any submods) now requires strict json, without comments or extra commas
* Replaced bonus MANA_PER_KNOWLEDGE with MANA_PER_KNOWLEDGE_PERCENTAGE to avoid rounding error with mysticism
* Factions can now be marked as 'special', banning them from random selection
* Replaced 'convert txt' text export command with more convenient 'translate' and 'translate maps' commands
* Game will now report cases where minimal damage of a creature is greater than maximal damage
* Added bonuses RESOURCES_CONSTANT_BOOST and RESOURCES_TOWN_MULTIPLYING_BOOST

## 1.4.4 -> 1.4.5

### Stability

* Fixed crash on creature spellcasting
* Fixed crash on unit entering magical obstacles such as quicksands
* Fixed freeze on map loading on some systems
* Fixed crash on attempt to start campaign with unsupported map
* Fixed crash on opening creature information window with invalid SPELL_IMMUNITY bonus

### Random Maps Generator

* Fixed placement of guards sometimes resulting into open connection into third zone
* Fixed rare crash on multithreaded access during placement of artifacts or wandering monsters

### Map Editor

* Fixed inspector using wrong editor for some values

### AI

* Fixed bug leading to AI not attacking wandering monsters in some cases
* Fixed crash on using StupidAI for autocombat or for enemy players

## 1.4.3 -> 1.4.4

### General

* Fixed crash on generation of random maps

## 1.4.2 -> 1.4.3

### General

* Fixed the synchronisation of the audio and video of the opening movies.
* Fixed a bug that caused spells from mods to not show up in the Mage's Guild.
* Changed the default SDL driver on Windows from opengl to autodetection
* When a hero visits a town with a garrisoned hero, they will now automatically exchange spells if one of them has the Scholar skill.
* Movement and mana points are now replenished for new heroes in taverns.

### Multiplayer

* Simturn contact detection will now correctly check for hero moving range
* Simturn contact detection will now ignore wandering monsters
* Right-clicking the Simturns AI option now displays a tooltip
* Interaction attempts with other players during simturns will now have more concise error messages
* Turn timers are now limited to 24 hours in order to prevent bugs caused by an integer overflow.
* Fixed delays when editing turn timer duration
* Ending a turn during simturns will now block the interface correctly.

### Campaigns

* Player will no longer start the United Front of Song for the Father campaign with two Nimbuses.
* Fixed missing campaign description after loading saved game
* Campaign completion checkmarks will now be displayed after the entire campaign has been completed, rather than just after the first scenario.
* Fixed positioning of prologue and epilogue text during campaign scenario intros

### Interface

* Added an option to hide adventure map window when town or battle window are open
* Fixed switching between pages on small version of spellbook
* Saves with long filenames are now truncated in the UI to prevent overflow.
* Added option to sort saved games by change date
* Game now shows correct resource when selecting start bonus
* It is now possible to inspect commander skills during battles.
* Fixed incorrect cursor being displayed when hovering over navigable water tiles
* Fixed incorrect cursor display when hovering over water objects accessible from shore

### Stability

* Fixed a crash when using the 'vcmiobelisk' cheat more than once.
* Fixed crash when reaching level 201. The maximum level is now limited to 197.
* Fixed crash when accessing a spell with an invalid SPELLCASTER bonus
* Fixed crash when trying to play music for an inaccessible tile
* Fixed memory corruption on loading of old mods with illegal 'index' field
* Fixed possible crash on server shutdown on Android
* Fixed possible crash when the affinity of the hero class is set to an invalid value
* Fixed crash on invalid creature in hero army due to outdated or broken mods
* Failure to initialise video subsystem now displays error message instead of silent crash

### Random Maps Generator

* Fixed possible creation of a duplicate hero in a random map when the player has chosen the starting hero.
* Fixed banning of quest artifacts on random maps
* Fixed banning of heroes in prison on random maps

### Battles

* Battle turn queue now displays current turn
* Added option to show unit statistics sidebar in battle
* Right-clicking on a unit in the battle turn queue now displays the unit details popup.
* Fixed error messages for SUMMON_GUARDIANS and TRANSMUTATION bonuses
* Fixed Dendroid Bind ability
* Black Dragons no longer hate Giants, only Titans
* Spellcasting units such as Archangels can no longer cast spells on themselves.
* Coronius specialty will now correctly select affected units

### Launcher

* Welcome screen will automatically detect existing Heroes 3 installation on Windows
* It is now possible to install mods by dragging and dropping onto the launcher.
* It is now possible to install maps and campaigns by dragging and dropping onto the launcher.
* Czech launcher translation added
* Added option to select preferred SDL driver in launcher

### Map Editor

* Fixed saving of allowed abilities, spells, artifacts or heroes

### AI

* AI will no longer attempt to move immobilized units, such as those under the effect of Dendroid Bind.
* Fixed shooters not shooting when they have a range penalty
* Fixed Fire Elemental spell casting
* Fixed rare bug where unit would sometimes do nothing in battle

### Modding

* Added better reporting of "invalid identifiers" errors with suggestions on how to fix them
* Added FEROCITY bonus (HotA Aysiud)
* Added ENEMY_ATTACK_REDUCTION bonus (HotA Nix)
* Added REVENGE bonus (HotA Haspid)
* Extended DEATH_STARE bonus to support Pirates ability (HotA)
* DEATH_STARE now supports spell ID in addInfo field to override used spell
* SPELL_BEFORE_ATTACK bonus now supports spell priorities
* FIRST_STRIKE bonus supports subtypes damageTypeMelee, damageTypeRanged and damageTypeAll
* BLOCKS_RETALIATION now also blocks FIRST_STRIKE bonus
* Added 'canCastOnSelf' field for spells to allow creatures to cast spells on themselves.

## 1.4.1 -> 1.4.2

### General

* Restored support for Windows 7
* Restored support for 32-bit builds
* Implemented quick backpack window for slot-specific artifact selection, activated via mouse wheel / swipe gesture
* Added option to search for specific spell in the spellbook
* Added option to skip fading animation on adventure map
* Using alt-tab to switch to another application will no longer activate in-game console/chat
* Increased frequency of checks for server startup to improve server connection time
* added nwcfollowthewhiterabbit / vcmiluck cheat: the currently selected hero permanently gains maximum luck.
* added nwcmorpheus / vcmimorale cheat: the currently selected hero permanently gains maximum morale.
* added nwcoracle / vcmiobelisk cheat: the puzzle map is permanently revealed.
* added nwctheone / vcmigod cheat: reveals the whole map, gives 5 archangels in each empty slot, unlimited movement points and permanent flight to currently selected hero

### Launcher

* Launcher will now properly show mod installation progress
* Launcher will now correctly select preferred language on first start

### Multiplayer

* Timers for all players will now be visible at once
* Turn options menu will correctly open for guests when host switches to it
* Guests will correctly see which roads are allowed for random maps by host
* Game will now correctly deactivate unit when timer runs out in pvp battle
* Game will show turn, battle and unit timers separately during battles
* Timer in pvp battles will be only active if unit timer is non-zero
* Timer during adventure map turn will be active only if turn timer is non-zero
* Game will now send notifications to players when simultaneous turns end

### Stability

* Fixed crash on clicking town or hero list on MacOS and iOS
* Fixed crash on closing vcmi on Android
* Fixed crash on disconnection from multiplayer game
* Fixed crash on finishing game on last day of the month
* Fixed crash on loading h3m maps with mods that alter Witch Hut, Shrine or Scholar
* Fixed crash on opening creature morale detalisation in some localizations
* Fixed possible crash on starting a battle when opening sound from previous battle is still playing
* Fixed crash on map loading in case if there is no suitable option for a random dwelling
* Fixed crash on usage of radial wheel to reorder towns or heroes
* Fixed possible crash on random map generation
* Fixed crash on attempting to transfer last creature when stack experience is enabled
* Fixed crash on accessing invalid settings options
* Fixed server crash on receiving invalid message from player
* Added check for presence of Armageddon Blade campaign files to avoid crash on some Heroes 3 versions

### Random Maps Generator

* Improved performance of random maps generation
* Rebalance of treasure values and density
* Improve junction zones generation by spacing Monoliths
* Reduced amount of terrain decorations to level more in line with H3
* Generator will now avoid path routing near map border
* Generator will now check full object area for minimum distance requirement
* Fixed routing of roads behind Subterranean Gates, Monoliths and Mines
* Fixed remaining issues with placement of Corpse
* Fixed placement of one-tile prisons from HotA
* Fixed spawning of Armageddon's Blade and Vial of Dragon Blood on random maps

### Interface

* Right-clicking hero icon during levelup dialog will now show hero status window
* Added indicator of current turn to unit turn order panel in battles
* Reduces upscaling artifacts on large spellbook
* Game will now display correct date of saved games on Android
* Fixed black screen appearing during spellbook page flip animation
* Fixed description of "Start map with hero" bonus in campaigns
* Fixed invisible chat text input in game lobby
* Fixed positioning of chat history in game lobby
* "Infobar Creature Management" option is now enabled by default
* "Large Spellbook" option is now enabled by default

### Mechanics

* Anti-magic garrison now actually blocks spell casting
* Berserk spell will no longer cancel if affected unit performs counterattack
* Frenzy spell can no longer be casted on units that should be immune to it
* Master Genie will no longer attempt to cast beneficial spell on creatures immune to it
* Vitality and damage skills of a commander will now correctly grow with level

### Modding

* Added UNTIL_OWN_ATTACK duration type for bonuses
* Configurable objects with visit mode "first" and "random" now respect "canRefuse" flag

## 1.4.0 -> 1.4.1

### General

* Fixed position for interaction with starting heroes
* Fixed smooth map scrolling when running at high framerate
* Fixed calculation of Fire Shield damage when caster has artifacts that increase its damage
* Fixed untranslated message when visiting signs with random text
* Fixed slider scrolling to maximum value when clicking on "scroll right" button
* Fixed events and seer huts not activating in some cases
* Fixed bug leading to Artifact Merchant selling Grails in loaded saved games
* Fixed placement of objects in random maps near the top border of the map
* Creatures under Slayer spell will no longer deal additional damage to creatures not affected by Slayer
* Description of a mod in Launcher will no longer be converted to lower-case
* Game will no longer fail to generate random map when AI-only players option is set to non-zero value
* Added option to mute audio when VCMI window is not active
* Added option to disable smooth map scrolling
* Reverted ban on U-turns in pathfinder

### Stability

* Fixed crash on using mods made for VCMI 1.3
* Fixed crash on generating random map with large number of monoliths
* Fixed crash on losing mission-critical hero in battle
* Fixed crash on generating growth detalization in some localizations
* Fixed crash on loading of some user-made maps

## 1.3.2 -> 1.4.0

### General

* Implemented High Score screen
* Implemented tracking of completed campaigns
* "Secret" Heroes 3 campaigns now require completion of prerequisite campaigns first
* Completing a campaign will now return player to campaign selection window instead of main menu
* Game will now play video on winning or losing a game
* Game will now correctly check for mod compatibility when loading saved games
* Game client will no longer load conflicting mods if player have both of them enabled
* If some mods fail to load due to missing dependencies or conflicts, game client will display message on opening main menu
* Game will no longer crash on loading save with different mod versions and will show error message instead
* Saved games are now 2-3 times smaller than before
* Added Vietnamese translation
* Failure to connect to a MP game will now show proper error message
* Added VSync support
* Implemented tutorial
* Implemented support for playback of audio from video files
* Windows Installer will now automatically add required firewall rules
* Game audio will now be disabled if game window is not focused
* Fixed formatting of date and time of a savegame on Android
* Added list of VCMI authors to credits screen
* Quick combat is now disabled by default
* Spectator mode in single player is now disabled

### Multiplayer

* Implemented simultaneous turns
* Implemented turn timers, including chess timers version
* Game will now hide entire adventure map on hotseat turn transfer
* Added option to pause game timer while on system options window
* Implemented localization support for maps
* Game will now use texts from local player instead of host
* Multiple fixes to validation of player requests by server

### Android

* Heroes 3 data import now accepts files in any case
* Fixed detection of Heroes 3 data presence when 'data' directory uses lower case

### Touchscreen

* Added tutorial video clips that explain supported touch gestures
* Double tap will now be correctly interpreted as double click, e.g. to start scenario via double-click
* Implemented snapping to 100% scale for adventure map zooming
* Implemented smooth scrolling for adventure map
* Implemented radial wheel to reorder list of owned towns and heroes
* Implemented radial wheel for hero exchange in towns

### Launcher

* When a mod is being downloaded, the launcher will now correctly show progress as well as its total size
* Double-clicking mod name will now perform expected action, e.g. install/update/enable or disable
* Launcher will now show mod extraction progress instead of freezing
* "Friendly AI" option will now correctly display current type of friendly AI
* Player can now correctly switch to global chat after disconnect
* "Resolve mods conflicts" button now attempts to fix all mods if nothing is selected
* Implemented support for mention in game lobby
* Implemented support for global and room channels in game lobby
* Added option to reconnect to game lobby

### Editor

* It is now possible to configure rewards for Seer Hut, Pandora Boxes and Events
* It is now possible to configure quest (limiter) in Seer Hut and Quest Guards
* It is now possible to configure events and rumors in map editor
* Improved army configuration interface
* Added option to customize hero skills
* It is now possible to select object on map for win/loss conditions or for main town
* Random dwellings can now be linked to a random town
* Added map editor zoom
* Added objects lock functionality
* It is now possible to configure hero placeholders in map editor
* Fixed duplicate artifact image on mouse drag
* Lasso tool will no longer skip tiles
* Fixed layout of roads and rivers

### Stability

* Fix possible crash on generating random map
* Fixed multiple memory leaks in game client
* Fixed crash on casting Hypnotize multiple times
* Fixed crash on attempting to move all artifacts from hero that has no artifacts
* Fixed crash on attempting to load corrupted .def file
* Fixed crash on clicking on empty Altar of Sacrifice slots

### AI

* BattleAI should now see strong stacks even if blocked by weak stacks.
* BattleAI will now prefers targets slower than own stack even if they are not reachable this turn.
* Improved BattleAI performance when selecting spell to cast
* Improved BattleAI performance when selection unit action
* Improved BattleAI spell selection logic
* Nullkiller AI can now use Fly and Water Walk spells

### Campaigns

* Implemented voice-over audio support for Heroes 3 campaigns
* Fixes victory condition on 1st scenario of "Long Live the King" campaign
* Fixed loading of defeat/victory icon and message for some campaign scenarios

### Interface

* Implemented adventure map dimming on opening windows
* Clicking town hall icon on town screen will now open town hall
* Clicking buildings in town hall will now show which resources are missing (if any)
* Fixed incorrect positioning of total experience text on Altar of Sacrifice
* Game will now show correct video file on battle end
* Game will now correctly loop battle end animation video
* Implemented larger version of spellbooks that displays up to 24 spells at once
* Spell scrolls in hero inventory now show icon of contained spell
* Fixed incorrect hero morale tooltip after visiting adventure map objects
* Fixed incorrect information for skills in hero exchange window
* Confirmation button will now be disabled on automatic server connect dialog
* Attempting to recruit creature in town with no free slots in garrisons will now correctly show error message

### Main Menu

* Implemented window for quick selection of starting hero, town and bonus
* Implemented map preview in scenario selection and game load screen accessible via right click on map
* Show exact map size in map selection
* Added support for folders in scenario selection and save/load screens
* Added support for "Show Random Maps" button in random map setup screen
* Added starting hero preview screen
* Added option to change name of player while in map setup screen
* Implemented loading screen with progress bar
* Game will now stay on loading screen while random map generation is in process
* Team Alignments popup in scenario options will no longer show empty teams
* Fixed missing borders on team alignments configuration window in random maps
* Fixed map difficulty icon on save/load screen
* Main menu animation will no longer appear on top of new game / load game text

### Adventure Map Interface

* Picking up an artifact on adventure map will now show artifact assembly dialog if such option exists
* Minimap will now preserve correct aspect ratio on rectangular maps
* Fixed slot highlighting when an artifact is being assembled
* Ctrl-click on hero will now select him instead of changing path of active hero
* In selection windows (level up window, treasure chest pickup, etc) it is now possible to select reward via double-click
* Attacking wandering monsters with preconfigured message will now correctly show the message
* Revisit object button will now be blocked if there is no object to revisit
* Fixed missing tooltip for "revisit object" button
* Fixed calculation of fow reveal range for all objects
* Attempt to close game will now ask for confirmation
* Right-clicking previously visited Seer Huts or Quest Guards will show icon with preview of quest goal
* Right-clicking owned dwellings will show amount of creatures available to for recruitment
* Right-clicking previously visited creature banks will show exact guards composition with their portraits
* Right-clicking artifacts on map will show artifact description
* Right-clicking objects that give bonus to hero will show object description

### Mechanics

* Heroes in tavern will correctly lose effects from spells or visited objects on new day
* Fixed multiple bugs in offering of Wisdom and Spell Schools on levelup. Mechanic should now work identically to Heroes 3
* Retreated heroes will no longer restore their entire mana pool on new day
* Fixed Grail in Crypt on some custom maps
* Added support for repeatable quests in Seer Huts
* Using "Sacrifice All" on Altar will now correctly place all creatures but one on altar
* Fixed probabilities of luck and morale
* Blinded stack no longer can get morale
* Creature that attacks while standing in moat will now correctly receive moat damage
* Player resources are now limited to 1 000 000 000 to prevent overflow
* It is no longer possible to escape from town without fort
* Pathfinder will no longer make U-turns when moving onto visitable objects while flying
* Pathfinder will no longer make paths that go over teleporters without actually using them
* Game will now correctly update guard status of tiles that are guarded by multiple wandering monsters
* Moving onto Garrisons and Border Guards entrance tiles that are guarded by wandering monsters will now correctly trigger battle
* It is no longer possible to build second boat in shipyard when shipyard should be blocked by boat with hero
* Gundula is now Offense specialist and not Sorcery, as in H3

### Random Maps Generator

* Increased tolerance for placement of Subterranean Gates
* Game will now select random object template out of available options instead of picking first one
* It is no longer possible to create map with a single team
* Game will no longer route roads through non-removable treasure objects, such as Corpse
* Fixed placement of treasure piles with non-removable objects, such as Corpse
* Fixed interface no displaying correct random map settings in some cases
* Fixed misleading error "no info for player X found"
* Fixed bug leading to AI players defeated on day one.

### Modding

* All bonuses now require string as a subtype. See documentation for exact list of possible strings for each bonus.
* Changes to existing objects parameters in mods will now be applied to ongoing saves
* Fixed handling of engine version compatibility check
* Added support for giving arbitrary bonuses to AI players
* Most mods of type "Translation" are now hidden in Launcher
* Added new mod type: "Compatibility". Mods of this type are hidden in Launcher and are always active if they are compatible.
* Added new mod type: "Maps"
* Added new TERRAIN_NATIVE bonus that makes any terrain native to affected units
* SPELL_DURATION now allows subtypes. If set to spell, bonus will only affect specified spell
* Both game client and launcher will now correctly handle dependencies that are not in lower case
* Implemented support for refusable Witch Hut and Scholar
* Added "variables" to configurable objects that are shared between all rewards
* Added visit mode "limiter" for configurable objects. Hero will be considered as "visited this object" if he fulfills provided condition
* Added option to customize text displayed for visited objects, e.g. show "Already learned" instead of "Visited"
* Added option to define custom description of configurable object, accessible via right-click
* Added option to show object content icons on right-click
* Object now allows checking whether hero can learn spell
* Object limiter now allows checking whether hero can learn skill
* Object reward may now reveal terrain around visiting hero (e.g. Redwood Observatory)

## 1.3.1 -> 1.3.2

### GENERAL

* VCMI now uses new application icon
* Added initial version of Czech translation
* Game will now use tile hero is moving from for movement cost calculations, in line with H3
* Added option to open hero backpack window in hero screen
* Added detection of misclicks for touch inputs to make hitting small UI elements easier
* Hero commander will now gain option to learn perks on reaching master level in corresponding abilities
* It is no longer possible to stop movement while moving over water with Water Walk
* Game will now automatically update hero path if it was blocked by another hero
* Added "vcmiartifacts angelWings" form to "give artifacts" cheat

### STABILITY

* Fixed freeze in Launcher on repository checkout and on mod install
* Fixed crash on loading VCMI map with placed Abandoned Mine
* Fixed crash on loading VCMI map with neutral towns
* Fixed crash on attempting to visit unknown object, such as Market of Time
* Fixed crash on attempting to teleport unit that is immune to a spell
* Fixed crash on switching fullscreen mode during AI turn

### CAMPAIGNS

* Fixed reorderging of hero primary skills after moving to next scenario in campaigns

### BATTLES

* Conquering a town will now correctly award additional 500 experience points
* Quick combat is now enabled by default
* Fixed invisible creatures from SUMMON_GUARDIANS and TRANSMUTATION bonuses
* Added option to toggle spell usage by AI in quick combat
* Fixed updating of spell point of enemy hero in game interface after spell cast
* Fixed wrong creature spellcasting shortcut (now set to "F")
* It is now possible to perform melee attack by creatures with spells, especially area spells
* Right-click will now properly end spellcast mode
* Fixed cursor preview when casting spell using touchscreen
* Long tap during spell casting will now properly abort the spell

### INTERFACE

* Added "Fill all empty slots with 1 creature" option to radial wheel in garrison windows
* Context popup for adventure map monsters will now show creature icon
* Game will now show correct victory message for gather troops victory condition
* Fixed incorrect display of number of owned Sawmills in Kingdom Overview window
* Fixed incorrect color of resource bar in hotseat mode
* Fixed broken toggle map level button in world view mode
* Fixed corrupted interface after opening puzzle window from world view mode
* Fixed blocked interface after attempt to start invalid map
* Add yellow border to selected commander grandmaster ability
* Always use bonus description for commander abilities instead of not provided wog-specific translation  
* Fix scrolling when commander has large number of grandmaster abilities
* Fixed corrupted message on another player defeat
* Fixed unavailable Quest Log button on maps with quests
* Fixed incorrect values on a difficulty selector in save load screen
* Removed invalid error message on attempting to move non-existing unit in exchange window

### RANDOM MAP GENERATOR

* Fixed bug leading to unreachable resources around mines

### MAP EDITOR

* Fixed crash on maps containing abandoned mines
* Fixed crash on maps containing neutral objects
* Fixed problem with random map initialized in map editor
* Fixed problem with initialization of random dwellings

## 1.3.0 -> 1.3.1

### GENERAL

* Fixed framerate drops on hero movement with active hota mod
* Fade-out animations will now be skipped when instant hero movement speed is used
* Restarting loaded campaign scenario will now correctly reapply starting bonus
* Reverted FPS limit on mobile systems back to 60 fps
* Fixed loading of translations for maps and campaigns
* Fixed loading of preconfigured starting army for heroes with preconfigured spells
* Background battlefield obstacles will now appear below creatures
* it is now possible to load save game located inside mod
* Added option to configure reserved screen area in Launcher on iOS
* Fixed border scrolling when game window is maximized

### AI PLAYER

* BattleAI: Improved performance of AI spell selection
* NKAI: Fixed freeze on attempt to exchange army between garrisoned and visiting hero
* NKAI: Fixed town threat calculation
* NKAI: Fixed recruitment of new heroes
* VCAI: Added workaround to avoid freeze on attempting to reach unreachable location
* VCAI: Fixed spellcasting by Archangels

### RANDOM MAP GENERATOR

* Fixed placement of roads inside rock in underground
* Fixed placement of shifted creature animations from HotA
* Fixed placement of treasures at the boundary of wide connections
* Added more potential locations for quest artifacts in zone

### STABILITY

* When starting client without H3 data game will now show message instead of silently crashing
* When starting invalid map in campaign, game will now show message instead of silently crashing
* Blocked loading of saves made with different set of mods to prevent crashes
* Fixed crash on starting game with outdated mods
* Fixed crash on attempt to sacrifice all your artifacts in Altar of Sacrifice
* Fixed crash on leveling up after winning battle as defender
* Fixed possible crash on end of battle opening sound
* Fixed crash on accepting battle result after winning battle as defender
* Fixed possible crash on casting spell in battle by AI
* Fixed multiple possible crashes on managing mods on Android
* Fixed multiple possible crashes on importing data on Android
* Fixed crash on refusing rewards from town building
* Fixed possible crash on threat evaluation by NKAI
* Fixed crash on using haptic feedback on some Android systems
* Fixed crash on right-clicking flags area in RMG setup mode
* Fixed crash on opening Blacksmith window and Build Structure dialogs in some localizations
* Fixed possible crash on displaying animated main menu
* Fixed crash on recruiting hero in town located on the border of map

## 1.2.1 -> 1.3.0

### GENERAL

* Implemented automatic interface scaling to any resolution supported by monitor
* Implemented UI scaling option to scale game interface
* Game resolution and UI scaling can now be changed without game restart
* Fixed multiple issues with borderless fullscreen mode
* On mobile systems game will now always run at native resolution with configurable UI scaling
* Implemented support for Horn of the Abyss map format
* Implemented option to replay results of quick combat
* Added translations to French and Chinese
* All in-game cheats are now case-insensitive
* Added high-definition icon for Windows
* Fix crash on connecting to server on FreeBSD and Flatpak builds
* Save games now consist of a single file
* Added H3:SOD cheat codes as alternative to vcmi cheats
* Fixed several possible crashes caused by autocombat activation
* Fixed artifact lock icon in localized versions of the game
* Fixed possible crash on changing hardware cursor

### TOUCHSCREEN SUPPORT

* VCMI will now properly recognizes touch screen input
* Implemented long tap gesture that shows popup window. Tap once more to close popup
* Long tap gesture duration can now be configured in settings
* Implemented radial menu for army management, activated via swiping creature icon
* Implemented swipe gesture for scrolling through lists
* All windows that have sliders in UI can now be scrolled using swipe gesture
* Implemented swipe gesture for attack direction selection: swipe from enemy position to position you want to attack from
* Implemented pinch gesture for zooming adventure map
* Implemented haptic feedback (vibration) for long press gesture

### LAUNCHER

* Launcher will now attempt to automatically detect language of OS on first launch
* Added "About" tab with information about project and environment
* Added separate options for Allied AI and Enemy AI for adventure map
* Patially fixed displaying of download progress for mods
* Fixed potential crash on opening mod information for mods with a changelog
* Added option to configure number of autosaves

### MAP EDITOR

* Fixed crash on cutting random town
* Added option to export entire map as an image
* Added validation for placing multiple heroes into starting town
* It is now possible to have single player on a map
* It is now possible to configure teams in editor

### AI PLAYER

* Fixed potential crash on accessing market (VCAI)
* Fixed potentially infinite turns (VCAI)
* Reworked object prioritizing
* Improved town defense against enemy heroes
* Improved town building (mage guild and horde)
* Various behavior fixes

### GAME MECHANICS

* Hero retreating after end of 7th turn will now correctly appear in tavern
* Implemented hero backpack limit (disabled by default)
* Fixed Admiral's Hat movement points calculation
* It is now possible to access Shipwrecks from coast
* Hero path will now be correctly updated on equipping/unequipping Levitation Boots or Angel Wings
* It is no longer possible to abort movement while hero is flying over water
* Fixed digging for Grail
* Implemented "Survive beyond a time limit" victory condition
* Implemented "Defeat all monsters" victory condition
* 100% damage resistance or damage reduction will make unit immune to a spell
* Game will now randomly select obligatory skill for hero on levelup instead of always picking Fire Magic
* Fixed duration of bonuses from visitable object such as Idol of Fortune
* Rescued hero from prison will now correctly reveal map around him
* Lighthouses will no longer give movement bonus on land

### CAMPAIGNS

* Fixed transfer of artifacts into next scenario
* Fixed crash on advancing to next scenario with heroes from mods
* Fixed handling of "Start with building" campaign bonus
* Fixed incorrect starting level of heroes in campaigns
* Game will now play correct music track on scenario selection window
* Dracon woll now correctly start without spellbook in Dragon Slayer campaign
* Fixed frequent crash on moving to next scenario during campaign
* Fixed inability to dismiss heroes on maps with "capture town" victory condition

### RANDOM MAP GENERATOR

* Improved zone placement, shape and connections
* Improved zone passability for better gameplay
* Improved treasure distribution and treasure values to match SoD closely
* Navigation and water-specific spells are now banned on maps without water
* RMG will now respect road settings set in menu
* Tweaked many original templates so they allow new terrains and factions
* Added "bannedTowns", "bannedTerrains", "bannedMonsters" zone properties
* Added "road" property to connections
* Added monster strength "none"
* Support for "wide" connections
* Support for new "fictive" and "repulsive" connections
* RMG will now run faster, utilizing many CPU cores
* Removed random seed number from random map description

### INTERFACE

* Adventure map is now scalable and can be used with any resolution without mods
* Adventure map interface is now correctly blocked during enemy turn
* Visiting creature banks will now show amount of guards in bank
* It is now possible to arrange army using status window
* It is now possible to zoom in or out using mouse wheel or pinch gesture
* It is now possible to reset zoom via Backspace hotkey
* Receiving a message in chat will now play sound
* Map grid will now correctly display on map start
* Fixed multiple issues with incorrect updates of save/load game screen
* Fixed missing fortifications level icon in town tooltip
* Fixed positioning of resource label in Blacksmith window
* Status bar on inactive windows will no longer show any tooltip from active window
* Fixed highlighting of possible artifact placements when exchanging with allied hero
* Implemented sound of flying movement (for Fly spell or Angel Wings)
* Last symbol of entered cheat/chat message will no longer trigger hotkey
* Right-clicking map name in scenario selection will now show file name
* Right-clicking save game in save/load screen will now show file name and creation date
* Right-clicking in town fort window will now show creature information popup
* Implemented pasting from clipboard (Ctrl+V) for text input

### BATTLES

* Implemented Tower moat (Land Mines)
* Implemented defence reduction for units in moat
* Added option to always show hero status window
* Battle opening sound can now be skipped with mouse click
* Fixed movement through moat of double-hexed units
* Fixed removal of Land Mines and Fire Walls
* Obstacles will now correctly show up either below or above unit
* It is now possible to teleport a unit through destroyed walls
* Added distinct overlay image for showing movement range of highlighted unit
* Added overlay for displaying shooting range penalties of units

### MODDING

* Implemented initial version of VCMI campaign format
* Implemented spell cast as possible reward for configurable object
* Implemented support for configurable buildings in towns
* Implemented support for placing prison, tavern and heroes on water
* Implemented support for new boat types
* It is now possible for boats to use other movement layers, such as "air"
* It is now possible to use growing artifacts on artifacts that can be used by hero
* It is now possible to configure town moat
* Palette-cycling animation of terrains and rivers can now be configured in json
* Game will now correctly resolve identifier in unexpected form (e.g. 'bless' vs 'spell.bless' vs 'core:bless')
* Creature specialties that use short form ( "creature" : "pikeman" ) will now correctly affect all creature upgrades
* It is now possible to configure spells for Shrines
* It is now possible to configure upgrade costs per level for Hill Forts
* It is now possible to configure boat type for Shipyards on adventure map and in town
* Implemented support for HotA-style adventure map images for monsters, with offset
* Replaced (SCHOOL)_SPELL_DMG_PREMY with SPELL_DAMAGE bonus (uses school as subtype).
* Removed bonuses (SCHOOL)_SPELLS - replaced with SPELLS_OF_SCHOOL
* Removed DIRECT_DAMAGE_IMMUNITY bonus - replaced by 100% spell damage resistance
* MAGIC_SCHOOL_SKILL subtype has been changed for consistency with other spell school bonuses
* Configurable objects can now be translated
* Fixed loading of custom battlefield identifiers for map objects

## 1.2.0 -> 1.2.1

### GENERAL

* Implemented spell range overlay for Dimension Door and Scuttle Boat
* Fixed movement cost penalty from terrain
* Fixed empty Black Market on game start
* Fixed bad morale happening after waiting
* Fixed good morale happening after defeating last enemy unit
* Fixed death animation of Efreeti killed by petrification attack
* Fixed crash on leaving to main menu from battle in hotseat mode
* Fixed music playback on switching between towns
* Special months (double growth and plague) will now appear correctly
* Adventure map spells are no longer visible on units in battle
* Attempt to cast spell with no valid targets in hotseat will show appropriate error message
* RMG settings will now show all existing in game templates and not just those suitable for current settings
* RMG settings (map size and two-level maps) that are not compatible with current template will be blocked
* Fixed centering of scenario information window
* Fixed crash on empty save game list after filtering
* Fixed blocked progress in Launcher on language detection failure
* Launcher will now correctly handle selection of Ddata directory in H3 install
* Map editor will now correctly save message property for events and pandoras
* Fixed incorrect saving of heroes portraits in editor

## 1.1.1 -> 1.2.0

### GENERAL

* Adventure map rendering was entirely rewritten with better, more functional code
* Client battle code was heavily reworked, leading to better visual look & feel and fixing multiple minor battle bugs / glitches
* Client mechanics are now framerate-independent, rather than speeding up with higher framerate
* Implemented hardware cursor support
* Heroes III language can now be detected automatically
* Increased targeted framerate from 48 to 60
* Increased performance of UI updates
* Fixed bonus values of heroes who specialize in secondary skills
* Fixed bonus values of heroes who specialize in creatures
* Fixed damage increase from Adela's Bless specialty
* Fixed missing obstacles in battles on subterranean terrain
* Video files now play at correct speed
* Fixed crash on switching to second mission in campaigns
* New cheat code: vcmiazure - give 5000 azure dragons in every empty slot
* New cheat code: vcmifaerie - give 5000 faerie dragons in every empty slot
* New cheat code: vcmiarmy or vcminissi - give specified creatures in every empty slot. EG: vcmiarmy imp
* New cheat code: vcmiexp or vcmiolorin - give specified amount of experience to current hero. EG: vcmiexp 10000
* Fixed oversided message window from Scholar skill that had confirmation button outside game window
* Fixed loading of prebuilt creature hordes from h3m maps
* Fixed volume of ambient sounds when changing game sounds volume
* Fixed might&magic affinities of Dungeon heroes
* Fixed Roland's specialty to affect Swordsmen/Crusaders instead of Griffins
* Buying boat in town of an ally now correctly uses own resources instead of stealing them from ally
* Default game difficulty is now set to "normal" instead of "easy"
* Fixed crash on missing music files

### MAP EDITOR

* Added translations to German, Polish, Russian, Spanish, Ukrainian
* Implemented cut/copy/paste operations
* Implemented lasso brush for terrain editing
* Toolbar actions now have names
* Added basic victory and lose conditions

### LAUNCHER

* Added initial Welcome/Setup screen for new players
* Added option to install translation mod if such mod exists and player's H3 version has different language
* Icons now have higher resolution, to prevent upscaling artifacts
* Added translations to German, Polish, Russian, Spanish, Ukrainian
* Mods tab layout has been adjusted based on feedback from players
* Settings tab layout has been redesigned to support longer texts
* Added button to start map editor directly from Launcher
* Simplified game starting flow from online lobby
* Mod description will now show list of languages supported by mod
* Launcher now uses separate mod repository from vcmi-1.1 version to prevent mod updates to unsupported versions
* Size of mod list and mod details sub-windows can now be adjusted by player

### AI PLAYER

* Nullkiller AI is now used by default
* AI should now be more active in destroying heroes causing treat on AI towns
* AI now has higher priority for resource-producing mines
* Increased AI priority of town dwelling upgrades
* AI will now de-prioritize town hall upgrades when low on resources
* Messages from cheats used by AI are now hidden
* Improved army gathering from towns
* AI will now attempt to exchange armies between main heroes to get the strongest hero with the strongest army.
* Improved Pandora handling
* AI takes into account fort level now when evaluating enemy town capturing priority.
* AI can not use allied shipyard now to avoid freeze
* AI will avoid attacking creatures standing on draw-bridge tile during siege if the bridge is closed.
* AI will consider retreat during siege if it can not do anything (catapult is destroyed, no destroyed walls exist)

### RANDOM MAP GENERATOR

* Random map generator can now be used without vcmi-extras mod
* RMG will no longer place shipyards or boats at very small lakes
* Fixed placement of shipyards in invalid locations
* Fixed potential game hang on generation of random map
* RMG will now generate additional monolith pairs to create required number of zone connections
* RMG will try to place Subterranean Gates as far away from other objects (including each other) as possible
* RMG will now try to place objects as far as possible in both zones sharing a guard, not only the first one.
* Use only one template for an object in zone
* Objects with limited per-map count will be distributed evenly among zones with suitable terrain
* Objects above zone treasure value will not be considered for placement
* RMG will prefer terrain-specific templates for objects placement
* RMG will place Towns and Monoliths first in order to generate long roads across the zone.
* Adjust the position of center town in the zone for better look & feel on S maps.
* Description of random map will correctly show number of levels
* Fixed amount of creatures found in Pandora Boxes to match H3
* Visitable objects will no longer be placed on top of the map, obscured by map border

### ADVENTURE MAP

* Added option to replace popup messages on object visiting with messages in status window
* Implemented different hero movement sounds for offroad movement
* Cartographers now reveal terrain in the same way as in H3
* Status bar will now show movement points information on pressing ALT or after enabling option in settings
* It is now not possible to receive rewards from School of War without required gold amount
* Owned objects, like Mines and Dwellings will always show their owner in status bar
* It is now possible to interact with on-map Shipyard when no hero is selected
* Added option to show amount of creatures as numeric range rather than adjective
* Added option to show map grid
* Map swipe is no longer exclusive for phones and can be enabled on desktop platforms
* Added more graduated settings for hero movement speed
* Map scrolling is now more graduated and scrolls with pixel-level precision
* Hero movement speed now matches H3
* Improved performance of adventure map rendering
* Fixed embarking and disembarking sounds
* Fixed selection of "new week" animation for status window
* Object render order now mostly matches H3
* Fixed movement cost calculation when using "Fly" spell or "Angel Wings"
* Fixed game freeze on using Town Portal to teleport into town with unvisited Battle Scholar Academy
* Fixed invalid ambient sound of Whirlpool
* Hero path will now be correctly removed on defeating monsters that are at the end of hero path
* Seer Hut tooltips will now show messages for correct quest type

### INTERFACE

* Implemented new settings window
* Added framerate display option
* Fixed white status bar on server connection screen
* Buttons in battle window now correctly show tooltip in status bar
* Fixed cursor image during enemy turn in combat
* Game will no longer prompt to assemble artifacts if they fall into backpack
* It is now possible to use in-game console for vcmi commands
* Stacks sized 1000-9999 units will not be displayed as "1k"
* It is now possible to select destination town for Town Portal via double-click
* Implemented extended options for random map tab: generate G+U size, select RMG template, manage teams and roads

### HERO SCREEN

* Fixed cases of incorrect artifact slot highlighting
* Improved performance of artifact exchange operation
* Picking up composite artifact will immediately unlock slots
* It is now possible to swap two composite artifacts

### TOWN SCREEN

* Fixed gradual fade-in of a newly built building
* Fixed duration of building fade-in to match H3
* Fixed rendering of Shipyard in Castle
* Blacksmith purchase button is now properly locked if artifact slot is occupied by another warmachine
* Added option to show number of available creatures in place of growth
* Fixed possible interaction with hero / town list from adventure map while in town screen
* Fixed missing left-click message popup for some town buildings
* Moving hero from garrison by pressing space will now correctly show message "Cannot have more than 8 adventuring heroes"

### BATTLES

* Added settings for even faster animation speed than in H3
* Added display of potential kills numbers into attack tooltip in status bar
* Added option to skip battle opening music entirely
* All effects will now wait for battle opening sound before playing
* Hex highlighting will now be disabled during enemy turn
* Fixed incorrect log message when casting spell that kills zero units
* Implemented animated cursor for spellcasting
* Fixed multiple issues related to ordering of creature animations
* Fixed missing flags from hero animations when opening menus
* Fixed rendering order of moat and grid shadow
* Jousting bonus from Champions will now be correctly accounted for in damage estimation
* Building Castle building will now provide walls with additional health point
* Speed of all battle animations should now match H3
* Fixed missing obstacles on subterranean terrain
* Ballistics mechanics now matches H3 logic
* Arrow Tower base damage should now match H3
* Destruction of wall segments will now remove ranged attack penalty
* Force Field cast in front of drawbridge will now block it as in H3
* Fixed computations for Behemoth defense reduction ability
* Bad luck (if enabled) will now multiple all damage by 50%, in line with other damage reducing mechanics
* Fixed highlighting of movement range for creatures standing on a corpse
* All battle animations now have same duration/speed as in H3
* Added missing combat log message on resurrecting creatures
* Fixed visibility of blue border around targeted creature when spellcaster is making turn
* Fixed selection highlight when in targeted creature spellcasting mode
* Hovering over hero now correctly shows hero cursor
* Creature currently making turn is now highlighted in the Battle Queue
* Hovering over creature icon in Battle Queue will highlight this creature in the battlefield
* New battle UI extension allows control over creatures' special abilities
* Fixed crash on activating auto-combat in battle
* Fixed visibility of unit creature amount labels and timing of their updates
* Firewall will no longer hit double-wide units twice when passing through
* Unicorn Magic Damper Aura ability now works multiplicatively with Resistance
* Orb of Vulnerability will now negate Resistance skill

### SPELLS

* Hero casting animation will play before spell effect
* Fire Shield: added sound effect
* Fire Shield: effect now correctly plays on defending creature
* Earthquake: added sound effect
* Earthquake: spell will not select sections that were already destroyed before cast
* Remove Obstacles: fixed error message when casting on maps without obstacles
* All area-effect spells (e.g. Fireball) will play their effect animation on top
* Summoning spells: added fade-in effect for summoned creatures
* Fixed timing of hit animation for damage-dealing spells
* Obstacle-creating spells: UI is now locked during effect animation
* Obstacle-creating spells: added sound effect
* Added reverse death animation for spells that bring stack back to life
* Bloodlust: implemented visual effect
* Teleport: implemented visual fade-out and fade-in effect for teleporting
* Berserk: Fixed duration of effect
* Frost Ring: Fixed spell effect range
* Fixed several cases where multiple different effects could play at the same time
* All spells that can affecte multiple targets will now highlight affected stacks
* Bless and Curse now provide +1 or -1 to base damage on Advanced & Expert levels

### ABILITIES

* Rebirth (Phoenix): Sound will now play in the same time as animation effect
* Master Genie spellcasting: Sound will now play in the same time as animation effect
* Power Lich, Magogs: Sound will now play in the same time as attack animation effect
* Dragon Breath attack now correctly uses different attack animation if multiple targets are hit
* Petrification: implemented visual effect
* Paralyze: added visual effect
* Blind: Stacks will no longer retaliate on attack that blinds them
* Demon Summon: Added animation effect for summoning
* Fire shield will no longer trigger on non-adjacent attacks, e.g. from Dragon Breath
* Weakness now has correct visual effect
* Added damage bonus for opposite elements for Elementals
* Added damage reduction for Magic Elemental attacks against creatures immune to magic
* Added incoming damage reduction to Petrify
* Added counter-attack damage reduction for Paralyze

### MODDING

* All configurable objects from H3 now have their configuration in json
* Improvements to functionality of configurable objects
* Replaced `SECONDARY_SKILL_PREMY` bonus with separate bonuses for each skill.
* Removed multiple bonuses that can be replaced with another bonus.
* It is now possible to define new hero movement sounds in terrains
* Implemented translation support for mods
* Implemented translation support for .h3m maps and .h3c campaigns
* Translation mods are now automatically disabled if player uses different language
* Files with new Terrains, Roads and Rivers are now validated by game
* Parameters controlling effect of attack and defences stats on damage are now configurable in defaultMods.json
* New bonus: `LIMITED_SHOOTING_RANGE`. Creatures with this bonus can only use ranged attack within specified range
* Battle window and Random Map Tab now have their layout defined in json file
* Implemented code support for alternative actions mod
* Implemented code support for improved random map dialog
* It is now possible to configure number of creature stacks in heroes' starting armies
* It is now possible to configure number of constructed dwellings in towns on map start
* Game settings previously located in defaultMods.json are now loaded directly from mod.json
* It is now possible for spellcaster units to have multiple spells (but only for targeting different units)
* Fixed incorrect resolving of identifiers in commander abilities and stack experience definitions

## 1.1.0 -> 1.1.1

### GENERAL

* Fixed missing sound in Polish version from gog.com
* Fixed positioning of main menu buttons in localized versions of H3
* Fixed crash on transferring artifact to commander
* Fixed game freeze on receiving multiple artifact assembly dialogs after combat
* Fixed potential game freeze on end of music playback
* macOS/iOS: fixed sound glitches
* Android: upgraded version of SDL library
* Android: reworked right click gesture and relative pointer mode
* Improved map loading speed
* Ubuntu PPA: game will no longer crash on assertion failure

### ADVENTURE MAP

* Fixed hero movement lag in single-player games
* Fixed number of drowned troops on visiting Sirens to match H3
* iOS: pinch gesture visits current object (Spacebar behavior) instead of activating in-game console

### TOWNS

* Fixed displaying growth bonus from Statue of Legion
* Growth bonus tooltip ordering now matches H3
* Buy All Units dialog will now buy units starting from the highest level

### LAUNCHER

* Local mods can be disabled or uninstalled
* Fixed styling of Launcher interface

### MAP EDITOR

* Fixed saving of roads and rivers
* Fixed placement of heroes on map

## 1.0.0 -> 1.1.0

### GENERAL

* iOS is supported
* Mods and their versions and serialized into save files. Game checks mod compatibility before loading
* Logs are stored in system default logs directory
* LUA/ERM libs are not compiled by default
* FFMpeg dependency is optional now
* Conan package manager is supported for MacOS and iOS

### MULTIPLAYER

* Map is passed over network, so different platforms are compatible with each other
* Server self-killing is more robust
* Unlock in-game console while opponent's turn
* Host can control game session by using console commands
* Control over player is transferred to AI if client escaped the game
* Reconnection mode for crashed client processes
* Playing online is available using proxy server

### ADVENTURE MAP

* Fix for digging while opponent's turn
* Supported right click for quick recruit window
* Fixed problem with quests are requiring identical artefacts
* Bulk move and swap artifacts
* Pause & resume for towns and terrains music themes
* Feature to assemble/disassemble artefacts in backpack
* Clickable status bar to send messages
* Heroes no longer have chance to receive forbidden skill on leveling up
* Fixed visibility of newly recruited heroes near town
* Fixed missing artifact slot in Artifact Merchant window

### BATTLES

* Fix healing/regeneration behaviour and effect
* Fix crashes related to auto battle
* Implemented ray projectiles for shooters
* Introduced default tower shooter icons
* Towers destroyed during battle will no longer be listed as casualties

### AI

* BattleAI: Target prioritizing is now based on damage difference instead of health difference
* Nullkiller AI can retreat and surrender
* Nullkiller AI doesn't visit allied dwellings anymore
* Fixed a few freezes in Nullkiller AI

### RANDOM MAP GENERATOR

* Speedup generation of random maps
* Necromancy cannot be learned in Witch Hut on random maps

### MODS

* Supported rewardable objects customization
* Battleground obstacles are extendable now with VLC mechanism
* Introduced "compatibility" section into mods settings
* Fixed bonus system for custom advmap spells
* Supported customisable town entrance placement
* Fixed validation of mods with new adventure map objects

### LAUNCHER

* Fixed problem with duplicated mods in the list
* Launcher shows compatible mods only
* Uninstall button was moved to the left of layout
* Unsupported resolutions are not shown
* Lobby for online gameplay is implemented

### MAP EDITOR

* Basic version of Qt-based map editor

## 0.99 -> 1.0.0

### GENERAL

* Spectator mode was implemented through command-line options
* Some main menu settings get saved after returning to main menu - last selected map, save etc.
* Restart scenario button should work correctly now
* Skyship Grail works now immediately after capturing without battle
* Lodestar Grail implemented
* Fixed Gargoyles immunity
* New bonuses:
  * SOUL_STEAL - "WoG ghost" ability, should work somewhat same as in H3
  * TRANSMUTATION - "WoG werewolf"-like ability
  * SUMMON_GUARDIANS - "WoG santa gremlin"-like ability + two-hex unit extension
  * CATAPULT_EXTRA_SHOTS - defines number of extra wall attacks for units that can do so
  * RANGED_RETALIATION - allows ranged counterattack
  * BLOCKS_RANGED_RETALIATION - disallow enemy ranged counterattack
  * SECONDARY_SKILL_VAL2 - set additional parameter for certain secondary skills
  * MANUAL_CONTROL - grant manual control over war machine
  * WIDE_BREATH - melee creature attacks affect many nearby hexes
  * FIRST_STRIKE - creature counterattacks before attack if possible
  * SYNERGY_TARGET - placeholder bonus for Mod Design Team (subject to removal in future)
  * SHOOTS_ALL_ADJACENT - makes creature shots affect all neighbouring hexes
  * BLOCK_MAGIC_BELOW - allows blocking spells below particular spell level. HotA cape artifact can be implemented with this
  * DESTRUCTION - creature ability for killing extra units after hit, configurable

### MULTIPLAYER

* Loading support. Save from single client could be used to load all clients.
* Restart support. All clients will restart together on same server.
* Hotseat mixed with network game. Multiple colors can be controlled by each client.

### SPELLS

* Implemented cumulative effects for spells

### MODS

* Improve support for WoG commander artifacts and skill descriptions
* Added support for modding of original secondary skills and creation of new ones.
* Map object sounds can now be configured via json
* Added bonus updaters for hero specialties
* Added allOf, anyOf and noneOf qualifiers for bonus limiters
* Added bonus limiters: alignment, faction and terrain
* Supported new terrains, new battlefields, custom water and rock terrains
* Following special buildings becomes available in the fan towns:
  * attackVisitingBonus
  * defenceVisitingBonus
  * spellPowerVisitingBonus
  * knowledgeVisitingBonus
  * experienceVisitingBonus
  * lighthouse
  * treasury

### SOUND

* Fixed many missing or wrong pickup and visit sounds for map objects
* All map objects now have ambient sounds identical to OH3

### RANDOM MAP GENERATOR

* Random map generator supports water modes (normal, islands)
* Added config randomMap.json with settings for map generator
* Added parameter for template allowedWaterContent
* Extra resource packs appear nearby mines
* Underground can be player starting place for factions allowed to be placed underground
* Improved obstacles placement aesthetics
* Rivers are generated on the random maps
* RMG works more stable, various crashes have been fixed
* Treasures requiring guards are guaranteed to be protected

### VCAI

* Reworked goal decomposition engine, fixing many loopholes. AI will now pick correct goals faster.
* AI will now use universal pathfinding globally
* AI can use Summon  Boat and Town Portal
* AI can gather and save resources on purpose
* AI will only buy army on demand instead of every turn
* AI can distinguish the value of all map objects
* General speed optimizations

### BATTLES

* Towers should block ranged retaliation
* AI can bypass broken wall with moat instead of standing and waiting until gate is destroyed
* Towers do not attack war machines automatically
* Draw is possible now as battle outcome in case the battle ends with only summoned creatures (both sides loose)

### ADVENTURE MAP

* Added buttons and keyboard shortcuts to quickly exchange army and artifacts between heroes
* Fix: Captured town should not be duplicated on the UI

### LAUNCHER

* Implemented notifications about updates
* Supported redirection links for downloading mods

## 0.98 -> 0.99

### GENERAL

* New Bonus NO_TERRAIN_PENALTY
* Nomads will remove Sand movement penalty from army
* Flying and water walking is now supported in pathfinder
* New artifacts supported
  * Angel Wings
  * Boots of Levitation
* Implemented rumors in tavern window
* New cheat codes:
  * vcmiglaurung - gives 5000 crystal dragons into each slot
  * vcmiungoliant - conceal fog of war for current player
* New console commands:
  * gosolo - AI take control over human players and vice versa
  * controlai - give control of one or all AIs to player
  * set hideSystemMessages on/off - suppress server messages in chat

### BATTLES

* Drawbridge mechanics implemented (animation still missing)
* Merging of town and visiting hero armies on siege implemented
* Hero info tooltip for skills and mana implemented

### ADVENTURE AI

* Fixed AI trying to go through underground rock
* Fixed several cases causing AI wandering aimlessly
* AI can again pick best artifacts and exchange artifacts between heroes
* AI heroes with patrol enabled won't leave patrol area anymore

### RANDOM MAP GENERATOR

* Changed fractalization algorithm so it can create cycles
* Zones will not have straight paths anymore, they are totally random
* Generated zones will have different size depending on template setting
* Added Thieves Guild random object (1 per zone)
* Added Seer Huts with quests that match OH3
* RMG will guarantee at least 100 pairs of Monoliths are available even if there are not enough different defs

## 0.97 -> 0.98

### GENERAL

* Pathfinder can now find way using Monoliths and Whirlpools (only used if hero has protection)

### ADVENTURE AI

* AI will try to use Monolith entrances for exploration
* AI will now always revisit each exit of two way monolith if exit no longer visible
* AI will eagerly pick guarded and blocked treasures

### ADVENTURE MAP

* Implemented world view
* Added graphical fading effects

### SPELLS

* New spells handled:
  * Earthquake
  * View Air
  * View Earth
  * Visions
  * Disguise
* Implemented CURE spell negative dispel effect
* Added LOCATION target for spells castable on any hex with new target modifiers

### BATTLES

* Implemented OH3 stack split / upgrade formulas according to AlexSpl

### RANDOM MAP GENERATOR

* Underground tunnels are working now
* Implemented "junction" zone type
* Improved zone placing algorithm
* More balanced distribution of treasure piles
* More obstacles within zones

## 0.96 -> 0.97 (Nov 01 2014)

### GENERAL

* (windows) Moved VCMI data directory from '%userprofile%\vcmi' to '%userprofile%\Documents\My Games\vcmi'
* (windows) (OSX) Moved VCMI save directory from 'VCMI_DATA\Games' to 'VCMI_DATA\Saves'
* (linux)
* Changes in used librries:
  * VCMI can now be compiled with SDL2
  * Movies will use ffmpeg library
  * change boost::bind to std::bind
  * removed boost::assign
  * Updated FuzzyLite to 5.0
* Multiplayer load support was implemented through command-line options

### ADVENTURE AI

* Significantly optimized execution time, AI should be much faster now.

### ADVENTURE MAP

* Non-latin characters can now be entered in chat window or used for save names.
* Implemented separate speed for owned heroes and heroes owned by other players

### GRAPHICS

* Better upscaling when running in fullscreen mode.
* New creature/commader window
* New resolutions and bonus icons are now part of a separate mod
* Added graphics for GENERAL_DAMAGE_REDUCTION bonus (Kuririn)

### RANDOM MAP GENERATOR

* Random map generator now creates complete and playable maps, should match original RMG
* All important features from original map templates are implemented
* Fixed major crash on removing objects
* Undeground zones will look just like surface zones

### LAUNCHER

* Implemented switch to disable intro movies in game

## 0.95 -> 0.96 (Jul 01 2014)

### GENERAL

* (linux) now VCMI follows XDG specifications. See <http://forum.vcmi.eu/viewtopic.php?t=858>

### ADVENTURE AI

* Optimized speed and removed various bottlenecks.

### ADVENTURE MAP

* Heroes auto-level primary and secondary skill levels according to experience

### BATTLES

* Wall hit/miss sound will be played when using catapult during siege

### SPELLS

* New configuration format

### RANDOM MAP GENERATOR

* Towns from mods can be used
* Reading connections, terrains, towns and mines from template
* Zone placement
* Zone borders and connections, fractalized paths inside zones
* Guard generation
* Treasure piles generation (so far only few removable objects)

### MODS

* Support for submods - mod may have their own "submods" located in <modname>/Mods directory
* Mods may provide their own changelogs and screenshots that will be visible in Launcher
* Mods can now add new (offensive, buffs, debuffs) spells and change existing
* Mods can use custom mage guild background pictures and videos for taverns, setting of resources daily income for buildings

### GENERAL

* Added configuring of heroes quantity per player allowed in game

## 0.94 -> 0.95 (Mar 01 2014)

### GENERAL

* Components of combined artifacts will now display info about entire set.
* Implements level limit
* Added WoG creature abilities by Kuririn
* Implemented a confirmation dialog when pressing Alt + F4 to quit the game
* Added precompiled header compilation for CMake (can be enabled per flag)
* VCMI will detect changes in text files using crc-32 checksum
* Basic support for unicode. Internally vcmi always uses utf-8
* (linux) Launcher will be available as "VCMI" menu entry from system menu/launcher
* (linux) Added a SIGSEV violation handler to vcmiserver executable for logging stacktrace (for convenience)

### ADVENTURE AI

* AI will use fuzzy logic to compare and choose multiple possible subgoals.
* AI will now use SectorMap to find a way to guarded / covered objects.
* Significantly improved exploration algorithm.
* Locked heroes now try to decompose their goals exhaustively.
* Fixed (common) issue when AI found neutral stacks infinitely strong.
* Improvements for army exchange criteria.
* GatherArmy may include building dwellings in town (experimental).
* AI should now conquer map more aggressively and much faster
* Fuzzy rules will be printed out at map launch (if AI log is enabled)

### CAMPAIGNS

* Implemented move heroes to next scenario
* Support for non-standard victory conditions for H3 campaigns
* Campaigns use window with bonus & scenario selection than scenario information window from normal maps
* Implemented hero recreate handling (e.g. Xeron will be recreated on AB campaign)
* Moved place bonus hero before normal random hero and starting hero placement -> same behaviour as in OH3
* Moved placing campaign heroes before random object generation -> same behaviour as in OH3

### TOWNS

* Extended building dependencies support

### MODS

* Custom victory/loss conditions for maps or campaigns
* 7 days without towns loss condition is no longer hardcoded
* Only changed mods will be validated

## 0.93 -> 0.94 (Oct 01 2013)

### GENERAL

* New Launcher application, see
* Filesystem now supports zip archives. They can be loaded similarly to other archives in filesystem.json. Mods can use Content.zip instead of Content/ directory.
* fixed "get txt" console command
* command "extract" to extract file by name
* command "def2bmp" to convert def into set of frames.
* fixed crash related to cammander's SPELL_AFTER_ATTACK spell id not initialized properly (text id was resolved on copy of bonus)
* fixed duels, added no-GUI mode for automatic AI testing
* Sir Mullich is available at the start of the game
* Upgrade cost will never be negative.
* support for Chinese fonts (GBK 2-byte encoding)

### ADVENTURE MAP

* if Quick Combat option is turned on, battles will be resolved by AI
* first hero is awakened on new turn
* fixed 3000 gems reward in shipwreck

### BATTLES

* autofight implemented
* most of the animations is time-based
* simplified postioning of units in battle, should fix remaining issues with unit positioning
* synchronized attack/defence animation
* spell animation speed uses game settings
* fixed disrupting ray duration
* added logging domain for battle animations
* Fixed crashes on Land Mines / Fire Wall casting.
* UI will be correctly greyed-out during opponent turn
* fixed remaining issues with blit order
* Catapult attacks should be identical to H3. Catapult may miss and attack another part of wall instead (this is how it works in H3)
* Fixed Remove Obstacle.
* defeating hero will yield 500 XP
* Added lots of missing spell immunities from Strategija
* Added stone gaze immunity for Troglodytes (did you know about it?)
* damage done by turrets is properly increased by built buldings
* Wyverns will cast Poison instead of Stone Gaze.

### TOWN

* Fixed issue that allowed to build multiple boats in town.
* fix for lookout tower

## 0.92 -> 0.93 (Jun 01 2013)

### GENERAL

* Support for SoD-only installations, WoG becomes optional addition
* New logging framework
* Negative luck support, disabled by default
* Several new icons for creature abilities (Fire Shield, Non-living, Magic Mirror, Spell-like Attack)
* Fixed stack artifact (and related buttons) not displaying in creature window.
* Fixed crash at month of double population.

### MODS

* Improved json validation. Now it support most of features from latest json schema draft.
* Icons use path to icon instead of image indexes.
* It is possible to edit data of another mod or H3 data via mods.
* Mods can access only ID's from dependencies, virtual "core" mod and itself (optional for some mods compatibility)
* Removed no longer needed field "projectile spins"
* Heroes: split heroes.json in manner similar to creatures\factions; string ID's for H3 heroes; h3 hero classes and artifacts can be modified via json.

### BATTLES

* Fixed Death Stare of Commanders
* Projectile blitting should be closer to original H3. But still not perfect.
* Fixed missing Mirth effects
* Stack affected by Berserk should not try to attack itself
* Fixed several cases of incorrect positioning of creatures in battles
* Fixed abilities of Efreet.
* Fixed broken again palette in some battle backgrounds

### TOWN

* VCMI will not crash if building selection area is smaller than def
* Detection of transparency on selection area is closer to H3
* Improved handling buildings with mode "auto":
  * they will be properly processed (new creatures will be added if dwelling, spells learned if mage guild, and so on)
  * transitive dependencies are handled (A makes B build, and B makes C and D)

### SOUND

* Added missing WoG creature sounds (from Kuririn).
* The Windows package comes with DLLs needed to play .ogg files
* (linux) convertMP3 option for vcmibuilder for systems where SDL_Mixer can't play mp3's
* some missing sounds for battle effects

### ARTIFACTS

* Several fixes to combined artifacts added via mods.
* Fixed Spellbinder's Hat giving level 1 spells instead of 5.
* Fixed incorrect components of Cornucopia.
* Cheat code with grant all artifacts, including the ones added by mods

## 0.91 -> 0.92 (Mar 01 2013)

### GENERAL

* hero crossover between missions in campaigns
* introduction before missions in campaigns

### MODS

* Added CREATURE_SPELL_POWER for commanders
* Added spell modifiers to various spells: Hypnotize (Astral), Firewall (Luna), Landmine
* Fixed ENEMY_DEFENCE_REDUCTION, GENERAL_ATTACK_REDUCTION
* Extended usefulness of ONLY_DISTANCE_FIGHT, ONLY_MELEE_FIGHT ranges
* Double growth creatures are configurable now
* Drain Life now has % effect depending on bonus value
* Stack can use more than 2 attacks. Additional attacks can now be separated as "ONLY_MELEE_FIGHT and "ONLY_DISTANCE_FIGHT".
* Moat damage configurable
* More config options for spells:
  * mind immunity handled by config
  * direct damage immunity handled by config
  * immunity icon configurable
  * removed mind_spell flag
* creature config use string ids now.
* support for string subtype id in short bonus format
* primary skill identifiers for bonuses

## 0.9 -> 0.91 (Feb 01 2013)

### GENERAL

* VCMI build on OS X is now supported
* Completely removed autotools
* Added RMG interace and ability to generate simplest working maps
* Added loading screen

### MODS

* Simplified mod structure. Mods from 0.9 will not be compatible.
* Mods can be turned on and off in config/modSettings.json file
* Support for new factions, including:
  * New towns
  * New hero classes
  * New heroes
  * New town-related external dwellings
* Support for new artifact, including combined, commander and stack artifacts
* Extended configuration options
  * All game objects are referenced by string identifiers
  * Subtype resolution for bonuses

### BATTLES

* Support for "enchanted" WoG ability

### ADVENTURE AI

* AI will try to use Subterranean Gate, Redwood Observatory and Cartographer for exploration
* Improved exploration algorithm
* AI will prioritize dwellings and mines when there are no opponents visible

## 0.89 -> 0.9 (Oct 01 2012)

### GENERAL

* Provisional support creature-adding mods
* New filesystem allowing easier resource adding/replacing
* Reorganized package for better compatibility with HotA and not affecting the original game
* Moved many hard-coded settings into text config files
* Commander level-up dialog
* New Quest Log window
* Fixed a number of bugs in campaigns, support for starting hero selection bonus.

### BATTLES

* New graphics for Stack Queue
* Death Stare works identically to H3
* No explosion when catapult fails to damage the wall
* Fixed crash when attacking stack dies before counterattack
* Fixed crash when attacking stack dies in the Moat just before the attack
* Fixed Orb of Inhibition and Recanter's Cloak (they were incorrectly implemented)
* Fleeing hero won't lose artifacts.
* Spellbook won't be captured.

### ADVENTURE AI

* support for quests (Seer Huts, Quest Guardians, and so)
* AI will now wander with all the heroes that have spare movement points. It should prevent stalling.
* AI will now understand threat of Abandoned Mine.
* AI can now exchange armies between heroes. By default, it will pass army to main hero.
* Fixed strange case when AI found allied town extremely dangerous
* Fixed crash when AI tried to "revisit" a Boat
* Fixed crash when hero assigned to goal was lost when attempting realizing it
* Fixed a possible freeze when exchanging resources at marketplace

### BATTLE AI

* It is possible to select a battle AI module used by VCMI by typing into the console "setBattleAI <name>". The names of available modules are "StupidAI" and "BattleAI". BattleAI may be a little smarter but less stable. By the default, StupidAI will be used, as in previous releases.
* New battle AI module: "BattleAI" that is smarter and capable of casting some offensive and enchantment spells

## 0.88 -> 0.89 (Jun 01 2012)

### GENERAL

* Mostly implemented Commanders feature (missing level-up dialog)
* Support for stack artifacts
* New creature window graphics contributed by fishkebab
* Config file may have multiple upgrades for creatures
* CTRL+T will open marketplace window
* G will open thieves guild window if player owns at least one town with tavern
* Implemented restart functionality. CTRL+R will trigger a quick restart
* Save game screen and returning to main menu will work if game was started with --start option
* Simple mechanism for detecting game desynchronization after init
* 1280x800 resolution graphics, contributed by Topas

### ADVENTURE MAP

* Fixed monsters regenerating casualties from battle at the start of new week.
* T in adventure map will switch to next town

### BATTLES

* It's possible to switch active creature during tacts phase by clicking on stack
* After battle artifacts of the defeated hero (and his army) will be taken by winner
* Rewritten handling of battle obstacles. They will be now placed following H3 algorithm.
* Fixed crash when death stare or acid breath activated on stack that was just killed
* First aid tent can heal only creatures that suffered damage
* War machines can't be healed by tent
* Creatures casting spells won't try to cast them during tactic phase
* Console tooltips for first aid tent
* Console tooltips for teleport spell
* Cursor is reset to pointer when action is requested
* Fixed a few other missing or wrong tooltips/cursors
* Implemented opening creature window by l-clicking on stack
* Fixed crash on attacking walls with Cyclop Kings
* Fixed and simplified Teleport casting
* Fixed Remove Obstacle spell
* New spells supported:
  * Chain Lightning
  * Fire Wall
  * Force Field
  * Land Mine
  * Quicksands
  * Sacrifice

### TOWNS

* T in castle window will open a tavern window (if available)

### PREGAME

* Pregame will use same resolution as main game
* Support for scaling background image
* Customization of graphics with config file.

### ADVENTURE AI

* basic rule system for threat evaluation
* new town development logic
* AI can now use external dwellings
* AI will weekly revisit dwellings & mills
* AI will now always pick best stacks from towns
* AI will recruit multiple heroes for exploration
* AI won't try attacking its own heroes

## 0.87 -> 0.88 (Mar 01 2012)

* added an initial version of new adventure AI: VCAI
* system settings window allows to change default resolution
* introduced unified JSON-based settings system
* fixed all known localization issues
* Creature Window can handle descriptions of spellcasting abilities
* Support for the clone spell

## 0.86 -> 0.87 (Dec 01 2011)

### GENERAL

* Pathfinder can find way using ships and subterranean gates
* Hero reminder & sleep button

### PREGAME

* Credits are implemented

### BATTLES

* All attacked hexes will be highlighted
* New combat abilities supported:
  * Spell Resistance aura
  * Random spellcaster (Genies)
  * Mana channeling
  * Daemon summoning
  * Spellcaster (Archangel Ogre Mage, Elementals, Faerie Dragon)
  * Fear
  * Fearless
  * No wall penalty
  * Enchanter
  * Bind
  * Dispel helpful spells

## 0.85 -> 0.86 (Sep 01 2011)

### GENERAL

* Reinstated music support
* Bonus system optimizations (caching)
* converted many config files to JSON
* .tga file support
* New artifacts supported
  * Admiral's Hat
  * Statue of Legion
  * Titan's Thunder

### BATTLES

* Correct handling of siege obstacles
* Catapult animation
* New combat abilities supported
  * Dragon Breath
  * Three-headed Attack
  * Attack all around
  * Death Cloud / Fireball area attack
  * Death Blow
  * Lightning Strike
  * Rebirth
* New WoG abilities supported
  * Defense Bonus
  * Cast before attack
  * Immunity to direct damage spells
* New spells supported
  * Magic Mirror
  * Titan's Lightning Bolt

## 0.84 -> 0.85 (Jun 01 2011)

### GENERAL

* Support for stack experience
* Implemented original campaign selection screens
* New artifacts supported:
  * Statesman's Medal
  * Diplomat's Ring
  * Ambassador's Sash

### TOWNS

* Implemented animation for new town buildings
* It's possible to sell artifacts at Artifact Merchants

### BATTLES

* Neutral monsters will be split into multiple stacks
* Hero can surrender battle to keep army
* Support for Death Stare, Support for Poison, Age, Disease, Acid Breath, Fire / Water / Earth / Air immunities and Receptiveness
* Partial support for Stone Gaze, Paralyze, Mana drain

## 0.83 -> 0.84 (Mar 01 2011)

### GENERAL

* Bonus system has been rewritten
* Partial support for running VCMI in duel mode (no adventure map, only one battle, ATM only AI-AI battles)
* New artifacts supported:
  * Angellic Alliance
  * Bird of Perception
  * Emblem of Cognizance
  * Spell Scroll
  * Stoic Watchman

### BATTLES

* Better animations handling
* Defensive stance is supported

### HERO

* New secondary skills supported:
  * Artillery
  * Eagle Eye
  * Tactics

### AI PLAYER

* new AI leading neutral creatures in combat, slightly better then previous

## 0.82 -> 0.83 (Nov 01 2010)

### GENERAL

* Alliances support
* Week of / Month of events
* Mostly done pregame for MP games (temporarily only for local clients)
* Support for 16bpp displays
* Campaigns:
  * support for building bonus
  * moving to next map after victory
* Town Portal supported
* Vial of Dragon Blood and Statue of Legion supported

### HERO

* remaining specialities have been implemented

### TOWNS

* town events supported
* Support for new town structures: Deiety of Fire and Escape Tunnel

### BATTLES

* blocked retreating from castle

## 0.81 -> 0.82 (Aug 01 2010)

### GENERAL

* Some of the starting bonuses in campaigns are supported
* It's possible to select difficulty level of mission in campaign
* new cheat codes:
  * vcmisilmaril - player wins
  * vcmimelkor - player loses

### ADVENTURE MAP

* Neutral armies growth implemented (10% weekly)
* Power rating of neutral stacks
* Favourable Winds reduce sailing cost

### HERO

* Learning secondary skill supported.
* Most of hero specialities are supported, including:
  * Creature specialities (progressive, fixed, Sir Mullich)
  * Spell damage specialities (Deemer), fixed bonus (Ciele)
  * Secondary skill bonuses
  * Creature Upgrades (Gelu)
  * Resource generation
  * Starting Skill (Adrienne)

### TOWNS

* Support for new town structures:
  * Artifact Merchant
  * Aurora Borealis
  * Castle Gates
  * Magic University
  * Portal of Summoning
  * Skeleton transformer
  * Veil of Darkness

### OBJECTS

* Stables will now upgrade Cavaliers to Champions.
* New object supported:
  * Abandoned Mine
  * Altar of Sacrifice
  * Black Market
  * Cover of Darkness
  * Hill Fort
  * Refugee Camp
  * Sanctuary
  * Tavern
  * University
  * Whirlpool

## 0.8 -> 0.81 (Jun 01 2010)

### GENERAL

* It's possible to start campaign
* Support for build grail victory condition
* New artifacts supported:
  * Angel's Wings
  * Boots of levitation
  * Orb of Vulnerability
  * Ammo cart
  * Golden Bow
  * Hourglass of Evil Hour
  * Bow of Sharpshooter
  * Armor of the Damned

### ADVENTURE MAP

* Creatures now guard surrounding tiles
* New adventura map spells supported:
  * Summon Boat
  * Scuttle Boat
  * Dimension Door
  * Fly
  * Water walk

### BATTLES

* A number of new creature abilities supported
* First Aid Tent is functional
* Support for distance/wall/melee penalties & no * penalty abilities
* Reworked damage calculation to fit OH3 formula better
* Luck support
* Teleportation spell

### HERO

* First Aid secondary skill
* Improved formula for necromancy to match better OH3

### TOWNS

* Sending resources to other players by marketplace
* Support for new town structures:
  * Lighthouse
  * Colossus
  * Freelancer's Guild
  * Guardian Spirit
  * Necromancy Amplifier
  * Soul Prison

### OBJECTS

* New object supported:
  * Freelancer's Guild
  * Trading Post
  * War Machine Factory

## 0.75 -> 0.8 (Mar 01 2010)

### GENERAL

* Victory and loss conditions are supported. It's now possible to win or lose the game.
* Implemented assembling and disassembling of combination artifacts.
* Kingdom Overview screen is now available.
* Implemented Grail (puzzle map, digging, constructing ultimate building)
* Replaced TTF fonts with original ones.

### ADVENTURE MAP

* Implemented rivers animations (thx to GrayFace).

### BATTLES

* Fire Shield spell (and creature ability) supported
* affecting morale/luck and casting spell after attack creature abilities supported

### HERO

* Implementation of Scholar secondary skill

### TOWN

* New left-bottom info panel functionalities.

### TOWNS

* new town structures supported:
  * Ballista Yard
  * Blood Obelisk
  * Brimstone Clouds
  * Dwarven Treasury
  * Fountain of Fortune
  * Glyphs of Fear
  * Mystic Pond
  * Thieves Guild
  * Special Grail functionalities for Dungeon, Stronghold and Fortress

### OBJECTS

* New objects supported:
  * Border gate
  * Den of Thieves
  * Lighthouse
  * Obelisk
  * Quest Guard
  * Seer hut

A lot of of various bugfixes and improvements:
<http://bugs.vcmi.eu/changelog_page.php?version_id=14>

## 0.74 -> 0.75 (Dec 01 2009)

### GENERAL

* Implemented "main menu" in-game option.
* Hide the mouse cursor while displaying a popup window.
* Better handling of huge and empty message boxes (still needs more changes)
* Fixed several crashes when exiting.

### ADVENTURE INTERFACE

* Movement cursor shown for unguarded enemy towns.
* Battle cursor shown for guarded enemy garrisons.
* Clicking on the border no longer opens an empty info windows

### HERO WINDOW

* Improved artifact moving. Available slots are highlighted. Moved artifact is bound to mouse cursor.

### TOWNS

* new special town structures supported:
  * Academy of Battle Scholars
  * Cage of Warlords
  * Mana Vortex
  * Stables
  * Skyship (revealing entire map only)

### OBJECTS

* External dwellings increase town growth
* Right-click info window for castles and garrisons you do not own shows a rough amount of creatures instead of none
* Scholar won't give unavailable spells anymore.

A lot of of various bugfixes and improvements:
<http://bugs.vcmi.eu/changelog_page.php?version_id=2>

## 0.73 -> 0.74 (Oct 01 2009)

### GENERAL

* Scenario Information window
* Save Game window
* VCMI window should start centered
* support for Necromancy and Ballistics secondary skills
* new artifacts supported, including those improving Necromancy, Legion Statue parts, Shackles of War and most of combination artifacts (but not combining)
* VCMI client has its own icon (thx for graphic to Dikamilo)
* Ellipsis won't be split when breaking text on several lines
* split button will be grayed out when no creature is selected
* fixed issue when splitting stack to the hero with only one creatures
* a few fixes for shipyard window

### ADVENTURE INTERFACE

* Cursor shows if tile is accessible and how many turns away
* moving hero with arrow keys / numpad
* fixed Next Hero button behaviour
* fixed Surface/Underground switch button in higher resolutions

### BATTLES

* partial siege support
* new stack queue for higher resolutions (graphics made by Dru, thx!)
* 'Q' pressing toggles the stack queue displaying (so it can be enabled/disabled it with single key press)
* more creatures special abilities supported
* battle settings will be stored
* fixed crashes occurring on attacking two hex creatures from back
* fixed crash when clicking on enemy stack without moving mouse just after receiving action
* even large stack numbers will fit the boxes
* when active stack is killed by spell, game behaves properly
* shooters attacking twice (like Grand Elves) won't attack twice in melee
* ballista can shoot even if there's an enemy creature next to it
* improved obstacles placement, so they'll better fit hexes (thx to Ivan!)
* selecting attack directions works as in H3
* estimating damage that will be dealt while choosing stack to be attacked
* modified the positioning of battle effects, they should look about right now.
* after selecting a spell during combat, l-click is locked for any action other than casting.
* flying creatures will be blitted over all other creatures, obstacles and wall
* obstacles and units should be printed in better order (not tested)
* fixed armageddon animation
* new spells supported:
  * Anti-Magic
  * Cure
  * Resurrection
  * Animate Dead
  * Counterstrike
  * Berserk
  * Hypnotize
  * Blind
  * Fire Elemental
  * Earth Elemental
  * Water Elemental
  * Air Elemental
  * Remove obstacle

### TOWNS

* enemy castle can be taken over
* only one capitol per player allowed (additional ones will be lost)
* garrisoned hero can buy a spellbook
* heroes available in tavern should be always different
* ship bought in town will be correctly placed
* new special town structures supported:
  * Lookout Tower
  * Temple of Valhalla
  * Wall of Knowledge
  * Order of Fire

### HERO WINDOW

* war machines cannot be unequiped

### PREGAME

* sorting: a second click on the column header sorts in descending order.
* advanced options tab: r-click popups for selected town, hero and bonus
* starting scenario / game by double click
* arrows in options tab are hidden when not available
* subtitles for chosen hero/town/bonus in pregame

### OBJECTS

* fixed pairing Subterranean Gates
* New objects supported:
  * Borderguard & Keymaster Tent
  * Cartographer
  * Creature banks
  * Eye of the Magi & Hut of the Magi
  * Garrison
  * Stables
  * Pandora Box
  * Pyramid

## 0.72 -> 0.73 (Aug 01 2009)

### GENERAL

* infowindow popup will be completely on screen
* fixed possible crash with in game console
* fixed crash when gaining artifact after r-click on hero in tavern
* Estates / hero bonuses won't give resources on first day.
* video handling (intro, main menu animation, tavern animation, spellbook animation, battle result window)
* hero meeting window allowing exchanging armies and artifacts between heroes on adventure map
* 'T' hotkey opens marketplace window
* giving starting spells for heroes
* pressing enter or escape close spellbook
* removed redundant quotation marks from skills description and artifact events texts
* disabled autosaving on first turn
* bonuses from bonus artifacts
* increased char per line limit for subtitles under components
* corrected some exp/level values
* primary skills cannot be negative
* support for new artifacts: Ring of Vitality, Ring of Life, Vial of Lifeblood, Garniture of Interference, Surcoat of Counterpoise, Boots of Polarity
* fixed timed events reappearing
* saving system options
* saving hero direction
* r-click popups on enemy heroes and towns
* hero leveling formula matches the H3

### ADVENTURE INTERFACE

* Garrisoning, then removing hero from garrison move him at the end of the heroes list
* The size of the frame around the map depends on the screen size.
* spellbook shows adventure spells when opened on adventure map
* erasing path after picking objects with last movement point

### BATTLES

* spell resistance supported (secondary skill, artifacts, creature skill)
* corrected damage inflicted by spells and ballista
* added some missing projectile infos
* added native terrain bonuses in battles
* number of units in stack in battle should better fit the box
* non-living and undead creatures have now always 0 morale
* displaying luck effect animation
* support for battleground overlays:
  * cursed ground
  * magic plains
  * fiery fields
  * rock lands
  * magic clouds
  * lucid pools
  * holy ground
  * clover field
  * evil fog

### TOWNS

* fixes for horde buildings
* garrisoned hero can buy a spellbook if he is selected or if there is no visiting hero
* capitol bar in town hall is grey (not red) if already one exists
* fixed crash on entering hall when town was near map edge

### HERO WINDOW

* garrisoned heroes won't be shown on the list
* artifacts will be present on morale/luck bonuses list

### PREGAME

* saves are sorted primary by map format, secondary by name
* fixed displaying date of saved game (uses local time, removed square character)

### OBJECTS

* Fixed primary/secondary skill levels given by a scholar.
* fixed problems with 3-tiles monoliths
* fixed crash with flaggable building next to map edge
* fixed some descriptions for events
* New objects supported:
  * Buoy
  * Creature Generators
  * Flotsam
  * Mermaid
  * Ocean bottle
  * Sea Chest
  * Shipwreck Survivor
  * Shipyard
  * Sirens

## 0.71 -> 0.72 (Jun 1 2009)

### GENERAL

* many sound effects and music
* autosave (to 5 subsequent files)
* artifacts support (most of them)
* added internal game console (activated on TAB)
* fixed 8 hero limit to check only for wandering heroes (not garrisoned)
* improved randomization
* fixed crash on closing application
* VCMI won't always give all three stacks in the starting armies
* fix for drawing starting army creatures count
* Diplomacy secondary skill support
* timed events won't cause resources amount to be negative
* support for sorcery secondary skill
* redundant quotation marks from artifact descriptions are removed
* no income at the first day

### ADVENTURE INTERFACE

* fixed crasbug occurring on revisiting objects (by pressing space)
* always restoring default cursor when movng mouse out of the terrain
* fixed map scrolling with ctrl+arrows when some windows are opened
* clicking scrolling arrows in town/hero list won't open town/hero window
* pathfinder will now look for a path going via printed positions of roads when it's possible
* enter can be used to open window with selected hero/town

### BATTLES

* many creatures special skills implemented
* battle will end when one side has only war machines
* fixed some problems with handling obstacles info
* fixed bug with defending / waiting while no stack is active
* spellbook button is inactive when hero cannot cast any spell
* obstacles will be placed more properly when resolution is different than 800x600
* canceling of casting a spell by pressing Escape or R-click (R-click on a creatures does not cancel a spell)
* spellbook cannot be opened by L-click on hero in battle when it shouldn't be possible
* new spells:
  * frost ring
  * fireball
  * inferno
  * meteor shower
  * death ripple
  * destroy undead
  * dispel
  * armageddon
  * disrupting ray
  * protection from air
  * protection from fire
  * protection from water
  * protection from earth
  * precision
  * slayer

### TOWNS

* resting in town with mage guild will replenih all the mana points
* fixed Blacksmith
* the number of creatures at the beginning of game is their base growth
* it's possible to enter Tavern via Brotherhood of Sword

### HERO WINDOW

* fixed mana limit info in the hero window
* war machines can't be removed
* fixed problems with removing artifacts when all visible slots in backpack are full

### PREGAME

* clicking on "advanced options" a second time now closes the tab instead of refreshing it.
* Fix position of maps names.
* Made the slider cursor much more responsive. Speedup the map select screen.
* Try to behave when no maps/saves are present.
* Page Up / Page Down / Home / End hotkeys for scrolling through scenarios / games list

### OBJECTS

* Neutral creatures can join or escape depending on hero strength (escape formula needs to be improved)
* leaving guardians in flagged mines.
* support for Scholar object
* support for School of Magic
* support for School of War
* support for Pillar of Fire
* support for Corpse
* support for Lean To
* support for Wagon
* support for Warrior's Tomb
* support for Event
* Corpse (Skeleton) will be accessible from all directions

## 0.7 -> 0.71 (Apr 01 2009)

### GENERAL

* fixed scrolling behind window problem (now it's possible to scroll with CTRL + arrows)
* morale/luck system and corresponding sec. skills supported
* fixed crash when hero get level and has less than two sec. skills to choose between
* added keybindings for components in selection window (eg. for treasure chest dialog): 1, 2, and so on. Selection dialog can be closed with Enter key
* proper handling of custom portraits of heroes
* fixed problems with non-hero/town defs not present in def list but present on map (occurring probably only in case of def substitution in map editor)
* fixed crash when there was no hero available to hire for some player
* fixed problems with 1024x600 screen resolution
* updating blockmap/visitmap of randomized objects
* fixed crashes on loading maps with flag all mines/dwelling victory condition
* further fixes for leveling-up (stability and identical offered skills bug)
* splitting window allows to rebalance two stack with the same creatures
* support for numpad keyboard
* support for timed events

### ADVENTURE INTERFACE

* added "Next hero" button functionality
* added missing path arrows
* corrected centering on hero's position
* recalculating hero path after reselecting hero
* further changes in pathfinder making it more like original one
* orientation of hero can't be change if movement points are exhausted
* campfire, borderguard, bordergate, questguard will be accessible from the top
* new movement cost calculation algorithm
* fixed sight radious calculation
* it's possible to stop hero movement
* faster minimap refreshing
* provisional support for "Save" button in System Options Window
* it's possible to revisit object under hero by pressing Space

### BATTLES

* partial support for battle obstacles
* only one spell can be casted per turn
* blocked opening sepllbook if hero doesn't have a one
* spells not known by hero can't be casted
* spell books won't be placed in War Machine slots after battle
* attack is now possible when hex under cursor is not displayed
* glowing effect of yellow border around creatures
* blue glowing border around hovered creature
* made animation on battlefield more smooth
* standing stacks have more static animation
* probably fixed problem with displaying corpses on battlefield
* fixes for two-hex creatures actions
* fixed hero casting spell animation
* corrected stack death animation
* battle settings will be remembered between battles
* improved damage calculation formula
* correct handling of flying creatures in battles
* a few tweaks in battle path/available hexes calculation (more of them is needed)
* amounts of units taking actions / being an object of actions won't be shown until action ends
* fixed positions of stack queue and battle result window when resolution is != 800x600
* corrected duration of frenzy spell which was incorrect in certain cases
* corrected hero spell casting animation
* better support for battle backgrounds
* blocked "save" command during battle
* spellbook displays only spells known by Hero
* New spells supported:
  * Mirth
  * Sorrow
  * Fortune
  * Misfortune

### TOWN INTERFACE

* cannot build more than one capitol
* cannot build shipyard if town is not near water
* Rampart's Treasury requires Miner's Guild
* minor improvements in Recruitment Window
* fixed crash occurring when clicking on hero portrait in Tavern Window, minor improvements for Tavern Window
* proper updating resdatabar after building structure in town or buying creatures (non 800x600 res)
* fixed blinking resdatabar in town screen when buying (800x600)
* fixed horde buildings displaying in town hall
* forbidden buildings will be shown as forbidden, even if there are no res / other conditions are not fulfilled

### PREGAME

* added scrolling scenario list with mouse wheel
* fixed mouse slow downs
* cannot select heroes for computer player (pregame)
* no crash if uses gives wrong resolution ID number
* minor fixes

### OBJECTS

* windmill gives 500 gold only during first week ever (not every month)
* After the first visit to the Witch Hut, right-click/hover tip mentions the skill available.
* New objects supported:
  * Prison
  * Magic Well
  * Faerie Ring
  * Swan Pond
  * Idol of Fortune
  * Fountain of Fortune
  * Rally Flag
  * Oasis
  * Temple
  * Watering Hole
  * Fountain of Youth
  * support for Redwood Observatory
  * support for Shrine of Magic Incantation / Gesture / Thought
  * support for Sign / Ocean Bottle

### AI PLAYER

* Minor improvements and fixes.

## 0.64 -> 0.7 (Feb 01 2009)

### GENERAL

* move some settings to the config/settings.txt file
* partial support for new screen resolutions
* it's possible to set game resolution in pregame (type 'resolution' in the console)
* /Data and /Sprites subfolders can be used for adding files not present in .lod archives
* fixed crashbug occurring when hero levelled above 15 level
* support for non-standard screen resolutions
* F4 toggles between full-screen and windowed mode
* minor improvements in creature card window
* splitting stacks with the shift+click
* creature card window contains info about modified speed

### ADVENTURE INTERFACE

* added water animation
* speed of scrolling map and hero movement can be adjusted in the System Options Window
* partial handling r-clicks on adventure map

### TOWN INTERFACE

* the scroll tab won't remain hanged to our mouse position if we move the mouse is away from the scroll bar
* fixed cloning creatures bug in garrisons (and related issues)

### BATTLES

* support for the Wait command
* magic arrow *really* works
* war machines support partially added
* queue of stacks narrowed
* spell effect animation displaying improvements
* positive/negative spells cannot be cast on hostile/our stacks
* showing spell effects affecting stack in creature info window
* more appropriate coloring of stack amount box when stack is affected by a spell
* battle console displays notifications about wait/defend commands
* several reported bugs fixed
* new spells supported:
  * Haste
  * lightning bolt
  * ice bolt
  * slow
  * implosion
  * forgetfulness
  * shield
  * air shield
  * bless
  * curse
  * bloodlust
  * weakness
  * stone skin
  * prayer
  * frenzy

### AI PLAYER

* Genius AI (first VCMI AI) will control computer creatures during the combat.

### OBJECTS

* Guardians property for resources is handled
* support for Witch Hut
* support for Arena
* support for Library of Enlightenment

And a lot of minor fixes

## 0.63 -> 0.64 (Nov 01 2008)

### GENERAL

* sprites from /Sprites folder are handled correctly
* several fixes for pathfinder and path arrows
* better handling disposed/predefined heroes
* heroes regain 1 mana point each turn
* support for mistycisim and intelligence skills
* hero hiring possible
* added support for a number of hotkeys
* it's not possible anymore to leave hero level-up window without selecting secondary skill
* many minor improvements

* Added some kind of simple chatting functionality through console. Implemented several WoG cheats equivalents:
  * woggaladriel -> vcmiainur
  * wogoliphaunt -> vcminoldor
  * wogshadowfax -> vcminahar
  * wogeyeofsauron -> vcmieagles
  * wogisengard -> vcmiformenos
  * wogsaruman -> vcmiistari
  * wogpathofthedead -> vcmiangband
  * woggandalfwhite -> vcmiglorfindel

### ADVENTURE INTERFACE

* clicking on a tile in advmap view when a path is shown will not only hide it but also calculate a new one
* slowed map scrolling
* blocked scrolling adventure map with mouse when left ctrl is pressed
* blocked map scrolling when dialog window is opened
* scholar will be accessible from the top

### TOWN INTERFACE

* partially done tavern window (only hero hiring functionality)

### BATTLES

* water elemental will really be treated as 2 hex creature
* potential infinite loop in reverseCreature removed
* better handling of battle cursor
* fixed blocked shooter behavior
* it's possible in battles to check remeaining HP of neutral stacks
* partial support for Magic Arrow spell
* fixed bug with dying unit
* stack queue hotkey is now 'Q'
* added shots limit

## 0.62 -> 0.63 (Oct 01 2008)

### GENERAL

* coloured console output, logging all info to txt files
* it's possible to use other port than 3030 by passing it as an additional argument
* removed some redundant warnings
* partially done spellbook
* Alt+F4 quits the game
* some crashbugs was fixed
* added handling of navigation, logistics, pathfinding, scouting end estates secondary skill
* magical hero are given spellbook at the beginning
* added initial secondary skills for heroes

### BATTLES

* very significant optimization of battles
* battle summary window
* fixed crashbug occurring sometimes on exiting battle
* confirm window is shown before retreat
* graphic stack queue in battle (shows when 'c' key is pressed)
* it's possible to attack enemy hero
* neutral monster army disappears when defeated
* casualties among hero army and neutral creatures are saved
* better animation handling in battles
* directional attack in battles
* mostly done battle options (although they're not saved)
* added receiving exp (and leveling-up) after a won battle
* added support for archery, offence and armourer secondary abilities
* hero's primary skills accounted for damage dealt by creatures in battle

### TOWNS

* mostly done marketplace
* fixed crashbug with battles on swamps and rough terrain
* counterattacks
* heroes can learn new spells in towns
* working resource silo
* fixed bug with the mage guild when no spells available
* it's possible to build lighthouse

### HERO WINDOW

* setting army formation
* tooltips for artifacts in backpack

### ADVENTURE INTERFACE

* fixed bug with disappearing head of a hero in adventure map
* some objects are no longer accessible from the top
* no tooltips for objects under FoW
* events won't be shown
* working Subterranean Gates, Monoliths
* minimap shows all flaggable objects (towns, mines, etc.)
* artifacts we pick up go to the appropriate slot (if free)

## 0.61 -> 0.62 (Sep 01 2008)

### GENERAL

* restructured to the server-client model
* support for heroes placed in towns
* upgrading creatures
* working gaining levels for heroes (including dialog with skill selection)
* added graphical cursor
* showing creature amount in the creature info window
* giving starting bonus

### CASTLES

* icon in infobox showing that there is hero in town garrison
* fort/citadel/castle screen
* taking last stack from the heroes army should be impossible (or at least harder)
* fixed reading forbidden structures
* randomizing spells in towns
* viewing hero window in the town screen
* possibility of moving hero into the garrison
* mage guild screen
* support for blacksmith
* if hero doesn't have a spell book, he can buy one in a mage guild
* it's possible to build glyph of fear in fortress
* creatures placeholders work properly

### ADVENTURE INTERFACE

* hopefully fixed problems with wrong town defs (village/fort/capitol)

### HERO WINDOW

* bugfix: splitting stacks works in hero window
* removed bug causing significant increase of CPU consumption

### BATTLES

* shooting
* removed some displaying problems
* showing last group of frames in creature animation won't crash
* added start moving and end moving animations
* fixed moving two-hex creatures
* showing/hiding graphic cursor
* a part of using graphic cursor
* slightly optimized showing of battle interface
* animation of getting hit / death by shooting is displayed when it should be
* improved pathfinding in battles, removed problems with displaying movement, adventure map interface won't be called during battles.
* minor optimizations

### PREGAME

* updates settings when selecting new map after changing sorting criteria
* if sorting not by name, name will be used as a secondary criteria
* when filter is applied a first available map is selected automatically
* slider position updated after sorting in pregame

### OBJECTS

* support for the Tree of knowledge
* support for Campfires
* added event message when picking artifact

## 0.6 -> 0.61 (Jun 15 2008)

### IMPROVEMENTS

* improved attacking in the battles
* it's possible to kill hostile stack
* animations won't go in the same phase
* Better pathfinder
* "%s" substitutions in Right-click information in town hall
* windmill won't give wood
* hover text for heroes
* support for ZSoft-style PCX files in /Data
* Splitting: when moving slider to the right so that 0 is left in old slot the army is moved
* in the townlist in castle selected town will by placed on the 2nd place (not 3rd)
* stack at the limit of unit's range can now be attacked
* range of unit is now properly displayed
* battle log is scrolled down when new event occurs
* console is closed when application exits

### BUGFIXES

* stack at the limit of unit's range can now be attacked
* good background for the town hall screen in Stronghold
* fixed typo in hall.txt
* VCMI won't crash when r-click neutral stack during the battle
* water won't blink behind shipyard in the Castle
* fixed several memory leaks
* properly displaying two-hex creatures in recruit/split/info window
* corrupted map file won't cause crash on initializing main menu

## 0.59 -> 0.6 (Jun 1 2008)

* partially done attacking in battles
* screen isn't now refreshed while blitting creature info window
* r-click creature info windows in battles
* no more division by 0 in slider
* "plural" reference names for Conflux creatures (starting armies of Conflux heroes should now be working)
* fixed estate problems
* fixed blinking mana vortex
* grail increases creature growths
* new pathfinder
* several minor improvements

## 0.58 -> 0.59 (May 24 2008 - closed, test release)

* fixed memory leak in battles
* blitting creature animations to rects in the recruitment window
* fixed wrong creatures def names
* better battle pathfinder and unit reversing
* improved slider ( #58 )
* fixed problems with horde buildings (won't block original dwellings)
* giving primary skill when hero get level (but there is still no dialog)
* if an upgraded creature is available it'll be shown as the first in a recruitment window
* creature levels not messed in Fortress
* war machines are added to the hero's inventory, not to the garrison
* support for H3-style PCX graphics in Data/
* VCMI won't crash when is unable to initialize audio system
* fixed displaying wrong town defs
* improvements in recruitment window (slider won't allow to select more creatures than we can afford)
* creature info window (only r-click)
* callback for buttons/lists based on boost::function
* a lot of minor improvements

## 0.55 -> 0.58 (Apr 20 2008 - closed, test release)

### TOWNS

* recruiting creatures
* working creature growths (including castle and horde building influences)
* towns give income
* town hall screen
* building buildings (requirements and cost are handled)
* hints for structures
* updating town infobox

### GARRISONS

* merging stacks
* splitting stacks

### BATTLES

* starting battles
* displaying terrain, animations of heroes, units, grid, range of units, battle menu with console, amounts of units in stacks
* leaving battle by pressing flee button
* moving units in battles and displaying their ranges
* defend command for units

### GENERAL

* a number of minor fixes and improvements

## 0.54 -> 0.55 (Feb 29 2008)

* Sprites/ folder works for h3sprite.lod same as Data/ for h3bitmap.lod (but it's still experimental)
* randomization quantity of creatures on the map
* fix of Pandora's Box handling
* reading disposed/predefined heroes
* new command - "get txt" - VCMI will extract all .txt files from h3bitmap.lod to the Extracted_txts/ folder.
* more detailed logs
* reported problems with hero flags resolved
* heroes cannot occupy the same tile
* hints for most of creature generators
* some minor stuff

## 0.53b -> 0.54 (Feb 23 2008 - first public release)

* given hero is placed in the town entrance
* some objects such as river delta won't be blitted "on" hero
* tiles under FoW are inaccessible
* giving random hero on RoE maps
* improved protection against hero duplication
* fixed starting values of primary abilities of random heroes on RoE/AB maps
* right click popups with infoboxes for heroes/towns lists
* new interface coloring (many thanks to GrayFace ;])
* fixed bug in object flag's coloring
* added hints in town lists
* eliminated square from city hints

## 0.53 - 0.53b (Feb 20 2008)

* added giving default buildings in towns
* town infobox won't crash on empty town

## 0.52 - 0.53 (Feb 18 2008)

* hopefully the last bugfix of Pandora's Box
* fixed blockmaps of generated heroes
* disposed hero cannot be chosen in scenario settings (unless he is in prison)
* fixed town randomization
* fixed hero randomization
* fixed displaying heroes in preGame
* fixed selecting/deselecting artifact slots in hero window
* much faster pathfinder
* memory usage and load time significantly decreased
* it's impossible to select empty artifact slot in hero window
* fixed problem with FoW displaying on minimap on L-sized maps
* fixed crashbug in hero list connected with heroes dismissing
* mostly done town infobox
* town daily income is properly calculated

## 0.51 - 0.52 (Feb 7 2008)

* [feature] giving starting hero
* [feature] VCMI will try to use files from /Data folder instead of those from h3bitmap.lod
* [feature] picked artifacts are added to hero's backpack
* [feature] possibility of choosing player to play
* [bugfix] ZELP.TXT file *should* be handled correctly even it is non-english
* [bugfix] fixed crashbug in reading defs with negative left/right margins
* [bugfix] improved randomization
* [bugfix] pathfinder can't be cheated (what caused errors)

## 0.5 - 0.51 (Feb 3 2008)

* close button properly closes (same does 'q' key)
* two players can't have selected same hero
* double click on "Show Available Scenarios" won't reset options
* fixed possible crashbug in town/hero lists
* fixed crashbug in initializing game caused by wrong prisons handling
* fixed crashbug on reading hero's custom artifacts in RoE maps
* fixed crashbug on reading custom Pandora's Box in RoE maps
* fixed crashbug on reading blank Quest Guards
* better console messages
* map reading speed up (though it's still slow, especially on bigger maps)

## 0.0 -> 0.5 (Feb 2 2008 - first closed release)

* Main menu and New game screens
* Scenario selection, part of advanced options support
* Partially done adventure map, town and hero interfaces
* Moving hero
* Interactions with several objects (mines, resources, mills, and others)
