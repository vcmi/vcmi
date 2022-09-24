/*
 * Metatype.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

VCMI_LIB_NAMESPACE_BEGIN

enum class Metatype : uint32_t
{
	UNKNOWN = 0,
	ARTIFACT,
	ARTIFACT_INSTANCE,
	CREATURE,
	CREATURE_INSTANCE,
	FACTION,
	HERO_CLASS,
	HERO_TYPE,
	HERO_INSTANCE,
	MAP_OBJECT_GROUP,
	MAP_OBJECT_TYPE,
	MAP_OBJECT_INSTANCE,
	SKILL,
	SPELL
};


VCMI_LIB_NAMESPACE_END
