/*
 * ESerializationVersion.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

/// This enumeration controls save compatibility support.
/// - 'MINIMAL' represents the oldest supported version counter. A saved game can be loaded if its version is at least 'MINIMAL'.
/// - 'CURRENT' represents the current save version. Saved games are created using the 'CURRENT' version.
///
/// To make a save-breaking change:
/// - change 'MINIMAL' to a value higher than 'CURRENT'
/// - remove all keys in enumeration between 'MINIMAL' and 'CURRENT' as well as all their usage (will be detected by compiler)
/// - change 'CURRENT' to 'CURRENT = MINIMAL'
///
/// To make a non-breaking change:
/// - add new enumeration value before 'CURRENT'
/// - change 'CURRENT' to 'CURRENT = NEW_TEST_KEY'.
///
/// To check for version in serialize() call use form
/// if (h.hasFeature(Handler::Version::NEW_TEST_KEY))
///     h & newKey; // loading/saving save of a new version
/// else
///     newKey = saneDefaultValue; // loading of old save
enum class ESerializationVersion : int32_t
{
	NONE = 0,

	HOTA_MAP_STACK_COUNT = 893, // support Hota 1.7 stack count feature

	HOTA_MAP_FORMAT_EXTENSIONS, // support multiple Hota 1.7 map format features
	SPELL_RESEARCH_IMPROVEMENTS, // support counting past spell rerolls
	NAME_MAP_LAYERS, // name map layers
	HOTA_MAP_FORMAT_EXTENSIONS_2, // more Hota 1.7 map format features
	TIMER_MOVEMENT_POINTS, // movement points for timer
	DISABLE_TACTICS, // disable tactics
	REWARDABLE_EXTENSIONS_2, // movement points limiter for rewardables
	BONUS_TRIGGER, // bonus that allows triggered effects in combat
	CUSTOM_GARRISON_TITLE, // GarrisonDialog pack now has custom title parameter
	LUA_SCRIPTS,
	REWARDABLE_RESET_CALENDAR, // rewardable reset period split into days/weeks/months
	CONTROL_LOSS_TRACKING, // track when players ever controlled special defeat-condition objects
	QUEST_REWORK, // quest objects reshape: persist requiredKeys / allowedDifficulties limiter fields, quest-log identity (object or keymaster-colour type)
	HERO_SPECIALTY_ROUNDING, // DivideStackLevelUpdater carries hero level for H3-correct specialty rounding
	MAP_GEN_LEVEL_MAP_LAYERS, // CMapGenOptions per-level map layer IDs
	RETREAT_PERMISSION_BONUSES, // BATTLE_NO_FLEEING replaced by BATTLE_CAN_FLEE, escape tunnel provides bonus instead of hardcoded effect
	SCRIPT_VARIABLES, // per-map script variable storage (mod-namespaced key/value store)
	GAME_REPLAY_RECORDING, // recording of the game (and the battle ID counter it needs), stored in the savegame
	COMBAT_ABILITY_SCRIPTS, // combat abilities that became combat scripts are converted on load; spell effects and combat scripts share one registry, so bonus subtype saves the script as a string
	GAME_SESSION_DIRECTORY, // persistent per-game save directory and campaign start time
	SCENARIO_EVENT_JOURNAL, // per-player history of triggered scenario event messages and components
	MUTARE_DRAKE_OVERRIDE, // campaign header stores hero type override used for Mutare Drake crossover bonus targeting
	TOWN_NAME_TEXT_ID, // renaming a town registers the new name in the map text container instead of storing free-form text
	RECORD_TEXTS_METASTRING, // highscore scenario name and statistics map name are stored unresolved, to be rendered by the reader

	RELEASE_170 = HOTA_MAP_STACK_COUNT,
	RELEASE_174 = CUSTOM_GARRISON_TITLE,

	MINIMAL = RELEASE_170,
	CURRENT = RECORD_TEXTS_METASTRING,
};

static_assert(ESerializationVersion::MINIMAL <= ESerializationVersion::CURRENT, "Invalid serialization version definition!");
