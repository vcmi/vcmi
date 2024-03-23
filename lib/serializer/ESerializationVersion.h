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
	MANA_LIMIT,	// 837 change MANA_PER_KNOWLEGDE to percentage

	CURRENT = MANA_LIMIT
};
