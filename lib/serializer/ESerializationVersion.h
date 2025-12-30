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

	RELEASE_160 = 873,
	MINIMAL = RELEASE_160,

	MAP_HEADER_DISPOSED_HEROES, // map header contains disposed heroes list
	NO_RAW_POINTERS_IN_SERIALIZER, // large rework that removed all non-owning pointers from serializer
	STACK_INSTANCE_EXPERIENCE_FIX, // stack experience is stored as total, not as average
	STACK_INSTANCE_ARMY_FIX, // remove serialization of army that owns stack instance
	STORE_UID_COUNTER_IN_CMAP,  // fix crash caused by conflicting instanceName after loading game
	REWARDABLE_EXTENSIONS, // new functionality for rewardable objects
	FLAGGABLE_BONUS_SYSTEM_NODE, // flaggable objects now contain bonus system node
	RANDOMIZATION_REWORK, // random rolls logic has been moved to server
	CUSTOM_BONUS_ICONS, // support for custom icons in bonuses
	SERVER_STATISTICS, // statistics now only saved on server
	OPPOSITE_SIDE_LIMITER_OWNER, // opposite side limiter no longer stores owner in itself
	UNIVERSITY_CONFIG, // town university is configurable
	CAMPAIGN_BONUSES, // new format for scenario bonuses in campaigns
	BONUS_HIDDEN, // hidden bonus
	MORE_MAP_LAYERS, // more map layers
	CONFIGURABLE_RESOURCES, // configurable resources
	CUSTOM_NAMES, // custom names
	BATTLE_ONLY, // battle only mode
	CAMPAIGN_VIDEO, // second video for prolog/epilog in campaigns
	HOTA_MAP_STACK_COUNT, // support Hota 1.7 stack count feature

	CURRENT = HOTA_MAP_STACK_COUNT,
};

static_assert(ESerializationVersion::MINIMAL <= ESerializationVersion::CURRENT, "Invalid serialization version definition!");
