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

	MINIMAL = 831,

	RELEASE_143, // 832 +text container in campaigns, +starting hero in RMG options
	HAS_EXTRA_OPTIONS, // 833 +extra options struct as part of startinfo
	DESTROYED_OBJECTS, // 834 +list of objects destroyed by player
	CAMPAIGN_MAP_TRANSLATIONS, // 835 +campaigns include translations for its maps
	JSON_FLAGS, // 836 json uses new format for flags
	MANA_LIMIT,	// 837 change MANA_PER_KNOWLEDGE to percentage
	BONUS_META_STRING,	// 838 bonuses use MetaString instead of std::string for descriptions
	TURN_TIMERS_STATE, // 839 current state of turn timers is serialized
	ARTIFACT_COSTUMES, // 840 swappable artifacts set added

	RELEASE_150 = ARTIFACT_COSTUMES, // for convenience

	VOTING_SIMTURNS, // 841 - allow modification of simturns duration via vote
	REMOVE_TEXT_CONTAINER_SIZE_T, // 842 Fixed serialization of size_t from text containers
	BANK_UNIT_PLACEMENT, // 843 Banks have unit placement flag

	RELEASE_152 = BANK_UNIT_PLACEMENT,

	COMPACT_STRING_SERIALIZATION, // 844 - optimized serialization of previously encountered strings
	COMPACT_INTEGER_SERIALIZATION, // 845 - serialize integers in forms similar to protobuf
	REMOVE_FOG_OF_WAR_POINTER, // 846 - fog of war is serialized as reference instead of pointer
	SIMPLE_TEXT_CONTAINER_SERIALIZATION, // 847 - text container is serialized using common routine instead of custom approach
	MAP_FORMAT_ADDITIONAL_INFOS, // 848 - serialize new infos in map format
	REMOVE_LIB_RNG, // 849 - removed random number generators from library classes
	HIGHSCORE_PARAMETERS, // 850 - saves parameter for campaign
  PLAYER_HANDICAP, // 851 - player handicap selection at game start

	CURRENT = PLAYER_HANDICAP
};
