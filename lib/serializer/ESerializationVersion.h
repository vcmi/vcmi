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
/// if (h.version >= Handler::Version::NEW_TEST_KEY)
///     h & newKey; // loading/saving save of a new version
/// else
///     newKey = saneDefaultValue; // loading of old save
enum class ESerializationVersion : int32_t
{
	NONE = 0,

	RELEASE_150 = 840,
	MINIMAL = RELEASE_150,

	VOTING_SIMTURNS, // 841 - allow modification of simturns duration via vote
	REMOVE_TEXT_CONTAINER_SIZE_T, // 842 Fixed serialization of size_t from text containers
	BANK_UNIT_PLACEMENT, // 843 Banks have unit placement flag

	RELEASE_156 = BANK_UNIT_PLACEMENT,

	COMPACT_STRING_SERIALIZATION, // 844 - optimized serialization of previously encountered strings
	COMPACT_INTEGER_SERIALIZATION, // 845 - serialize integers in forms similar to protobuf
	REMOVE_FOG_OF_WAR_POINTER, // 846 - fog of war is serialized as reference instead of pointer
	SIMPLE_TEXT_CONTAINER_SERIALIZATION, // 847 - text container is serialized using common routine instead of custom approach
	MAP_FORMAT_ADDITIONAL_INFOS, // 848 - serialize new infos in map format
	REMOVE_LIB_RNG, // 849 - removed random number generators from library classes
	HIGHSCORE_PARAMETERS, // 850 - saves parameter for campaign
	PLAYER_HANDICAP, // 851 - player handicap selection at game start
	STATISTICS, // 852 - removed random number generators from library classes
	CAMPAIGN_REGIONS, // 853 - configurable campaign regions
	EVENTS_PLAYER_SET, // 854 - map & town events use std::set instead of bitmask to store player list
	NEW_TOWN_BUILDINGS, // 855 - old bonusing buildings have been removed
	STATISTICS_SCREEN, // 856 - extent statistic functions
	NEW_MARKETS, // 857 - reworked market classes
	PLAYER_STATE_OWNED_OBJECTS, // 858 - player state stores all owned objects in a single list
	SAVE_COMPATIBILITY_FIXES, // 859 - implementation of previoulsy postponed changes to serialization
	CHRONICLES_SUPPORT, // 860 - support for heroes chronicles
	PER_MAP_GAME_SETTINGS, // 861 - game settings are now stored per-map
	CAMPAIGN_OUTRO_SUPPORT, // 862 - support for campaign outro video
	REWARDABLE_BANKS, // 863 - team state contains list of scouted objects, coast visitable rewardable objects
	REGION_LABEL, // 864 - labels for campaign regions
	SPELL_RESEARCH, // 865 - spell research
	LOCAL_PLAYER_STATE_DATA, // 866 - player state contains arbitrary client-side data
	REMOVE_TOWN_PTR, // 867 - removed pointer to CTown from CGTownInstance
	REMOVE_OBJECT_TYPENAME, // 868 - remove typename from CGObjectInstance

	CURRENT = REMOVE_OBJECT_TYPENAME
};
