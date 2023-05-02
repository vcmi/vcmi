/*
 * Shortcut.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

enum class EShortcut
{
	NONE,

	// Global hotkeys that are available in multiple dialogs
	GLOBAL_ACCEPT,     // Return - Accept query
	GLOBAL_CANCEL,     // Escape - Cancel query
	GLOBAL_RETURN,     // Enter, Escape - Close current window and return to previous view
	GLOBAL_FULLSCREEN, // F4 - TODO: remove hardcoded check for key
	GLOBAL_OPTIONS,    // 'O' - Open System Options dialog
	GLOBAL_BACKSPACE,  // Backspace - erase last symbol in text input
	GLOBAL_MOVE_FOCUS, // Tab - move focus to next text input

	// Movement hotkeys, usually - for moving through lists with slider
	MOVE_LEFT,
	MOVE_RIGHT,
	MOVE_UP,
	MOVE_DOWN,
	MOVE_FIRST,
	MOVE_LAST,
	MOVE_PAGE_UP,
	MOVE_PAGE_DOWN,

	// Element selection - for multiple choice dialog popups
	SELECT_INDEX_1,
	SELECT_INDEX_2,
	SELECT_INDEX_3,
	SELECT_INDEX_4,
	SELECT_INDEX_5,
	SELECT_INDEX_6,
	SELECT_INDEX_7,
	SELECT_INDEX_8,

	// Main menu hotkeys - for navigation between main menu windows
	MAIN_MENU_NEW_GAME,
	MAIN_MENU_LOAD_GAME,
	MAIN_MENU_HIGH_SCORES,
	MAIN_MENU_CREDITS,
	MAIN_MENU_BACK,
	MAIN_MENU_QUIT,
	MAIN_MENU_SINGLEPLAYER,
	MAIN_MENU_MULTIPLAYER,
	MAIN_MENU_CAMPAIGN,
	MAIN_MENU_TUTORIAL,
	MAIN_MENU_CAMPAIGN_SOD,
	MAIN_MENU_CAMPAIGN_ROE,
	MAIN_MENU_CAMPAIGN_AB,
	MAIN_MENU_CAMPAIGN_CUSTOM,

	// Game lobby / scenario selection
	LOBBY_BEGIN_GAME, // b, Return
	LOBBY_LOAD_GAME,  // l, Return
	LOBBY_SAVE_GAME,  // s, Return
	LOBBY_RANDOM_MAP, // Open random map tab
	LOBBY_HIDE_CHAT,
	LOBBY_ADDITIONAL_OPTIONS, // Open additional options tab
	LOBBY_SELECT_SCENARIO,    // Open map list tab

	// In-game hotkeys, require game state but may be available in windows other than adventure map
	GAME_END_TURN,
	GAME_LOAD_GAME,
	GAME_SAVE_GAME,
	GAME_RESTART_GAME,
	GAME_TO_MAIN_MENU,
	GAME_QUIT_GAME,
	GAME_OPEN_MARKETPLACE,
	GAME_OPEN_THIEVES_GUILD,
	GAME_ACTIVATE_CONSOLE, // Tab, activates in-game console

	// Adventure map screen
	ADVENTURE_GAME_OPTIONS, // 'o', Open CAdventureOptions window
	ADVENTURE_TOGGLE_GRID,  // F6, Toggles map grid
	ADVENTURE_TOGGLE_SLEEP, // Toggles hero sleep status
	ADVENTURE_SET_HERO_ASLEEP, // Moves hero to sleep state
	ADVENTURE_SET_HERO_AWAKE, // Move hero to awake state
	ADVENTURE_MOVE_HERO,    // Moves hero alongside set path
	ADVENTURE_VISIT_OBJECT, // Revisits object hero is standing on
	ADVENTURE_VIEW_SELECTED,// Open window with currently selected hero/town
	ADVENTURE_NEXT_TOWN,
	ADVENTURE_NEXT_HERO,
	ADVENTURE_NEXT_OBJECT,  // TODO: context-sensitive next object - select next hero/town, depending on current selection
	ADVENTURE_FIRST_TOWN,   // TODO: select first available town in the list
	ADVENTURE_FIRST_HERO,   // TODO: select first available hero in the list
	ADVENTURE_VIEW_SCENARIO,// View Scenario Information window
	ADVENTURE_DIG_GRAIL,
	ADVENTURE_VIEW_PUZZLE,
	ADVENTURE_VIEW_WORLD,
	ADVENTURE_TOGGLE_MAP_LEVEL,
	ADVENTURE_KINGDOM_OVERVIEW,
	ADVENTURE_QUEST_LOG,
	ADVENTURE_CAST_SPELL,
	ADVENTURE_THIEVES_GUILD,

	// Move hero one tile in specified direction. Bound to cursors & numpad buttons
	ADVENTURE_MOVE_HERO_SW,
	ADVENTURE_MOVE_HERO_SS,
	ADVENTURE_MOVE_HERO_SE,
	ADVENTURE_MOVE_HERO_WW,
	ADVENTURE_MOVE_HERO_EE,
	ADVENTURE_MOVE_HERO_NW,
	ADVENTURE_MOVE_HERO_NN,
	ADVENTURE_MOVE_HERO_NE,

	// Battle screen
	BATTLE_TOGGLE_QUEUE,
	BATTLE_USE_CREATURE_SPELL,
	BATTLE_SURRENDER,
	BATTLE_RETREAT,
	BATTLE_AUTOCOMBAT,
	BATTLE_CAST_SPELL,
	BATTLE_WAIT,
	BATTLE_DEFEND,
	BATTLE_CONSOLE_UP,
	BATTLE_CONSOLE_DOWN,
	BATTLE_TACTICS_NEXT,
	BATTLE_TACTICS_END,
	BATTLE_SELECT_ACTION, // Alternative actions toggle

	// Town screen
	TOWN_OPEN_TAVERN,
	TOWN_SWAP_ARMIES, // Swap garrisoned and visiting armies

	// Creature & creature recruitment screen
	RECRUITMENT_MAX, // Set number of creatures to recruit to max
	RECRUITMENT_MIN, // Set number of creatures to recruit to min (1)
	RECRUITMENT_UPGRADE, // Upgrade current creature
	RECRUITMENT_UPGRADE_ALL, // Upgrade all creatures (Hill Fort / Skeleton Transformer)

	// Kingdom Overview window
	KINGDOM_HEROES_TAB,
	KINGDOM_TOWNS_TAB,

	// Hero screen
	HERO_DISMISS,
	HERO_COMMANDER,
	HERO_LOOSE_FORMATION,
	HERO_TIGHT_FORMATION,
	HERO_TOGGLE_TACTICS, // b

	// Spellbook screen
	SPELLBOOK_TAB_ADVENTURE,
	SPELLBOOK_TAB_COMBAT,

	AFTER_LAST
};
