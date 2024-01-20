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

/// This is enumeration that controls save compatibility support
/// - 'MINIMAL' represents older supported version counter. Save can be loaded if its version is at least 'MINIMAL'
/// - 'CURRENT' represent current save version. All saves are created using 'CURRENT' version
///
/// To add save-breaking change:
/// - change 'MINIMAL' to value higher than CURRENT
/// - remove all version enumerations inbetween
/// - change 'CURRENT' to 'CURRENT = MINIMAL'
///
/// To add non-breaking change:
/// - add new enumeration value before 'CURRENT'
/// - change 'CURRENT' to 'CURRENT = NEW_TEST_KEY'
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
	RELEASE_143, // 832 + text container in campaigns, +starting hero in RMG options
	HAS_EXTRA_OPTIONS, // 833 +extra options struct as part of startinfo

	CURRENT = HAS_EXTRA_OPTIONS
};
