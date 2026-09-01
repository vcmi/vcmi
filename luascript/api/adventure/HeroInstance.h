/*
 * HeroInstance.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include <vcmi/HeroClass.h>

#include "../../LuaWrapper.h"
#include "../MethodRegistrar.h"

#include "../../../lib/mapObjects/CGHeroInstance.h"

namespace scripting::api
{

class HeroInstanceProxy : public RawPointerWrapper<const CGHeroInstance, HeroInstanceProxy>
{
	static bool isMale(const CGHeroInstance & hero);
	static bool isFemale(const CGHeroInstance & hero);
	static int getLevel(const CGHeroInstance & hero);
	static int64_t getExperience(const CGHeroInstance & hero);
	static bool hasArtifact(const CGHeroInstance & hero, ArtifactID artifact);
	static int ownedArtifacts(const CGHeroInstance & hero, ArtifactID artifact);
	static int creatureCountInArmy(const CGHeroInstance & hero, CreatureID creature);

public:
	static constexpr std::string_view luaName = "HeroInstance";
	static constexpr std::string_view luaDescription =
		"A hero placed on the adventure map. Provides access to such information as owner, "
		"type (Orrin, Kyrre, Astral...), position on map, artifacts, and secondary skills, "
		"army composition, primary skills, and all bonuses affecting hero.";

	static void registerMethods(MethodRegistrar & R);
};

}
