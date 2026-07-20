/*
 * AdventureServer.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "../../LuaWrapper.h"
#include "../MethodRegistrar.h"

#include "../../../lib/callback/IGameEventCallback.h"
#include "../../../lib/constants/EntityIdentifiers.h"

class JsonNode;
class CGObjectInstance;
class CGHeroInstance;

namespace scripting::api
{

struct LuaMetaString;
struct LuaComponent;

class AdventureServerProxy : public RawPointerWrapper<IGameEventCallback, AdventureServerProxy>
{
public:
	static constexpr std::string_view luaName = "AdventureServer";
	static constexpr std::string_view luaDescription =
		"The authoritative adventure-map mutation interface. Available only to scripts running on "
		"the server; every call emits a network pack so clients receive the resulting state change. "
		"Grants rewards, shows dialogs, removes map objects and stores script variables.";

	static void registerMethods(MethodRegistrar & R);

	static void setMapVariable(IGameEventCallback & object, const std::string & name, const JsonNode & value);
	static void removeObject(IGameEventCallback & object, const CGObjectInstance & target);
	static int random(IGameEventCallback & object, int lower, int upper);

	static void giveExperience(IGameEventCallback & object, const CGHeroInstance & hero, int64_t amount);
	static void setOwner(IGameEventCallback & object, const CGObjectInstance & target, PlayerColor owner);
	static void grantSpell(IGameEventCallback & object, const CGHeroInstance & hero, SpellID spell);
	static void takeSpell(IGameEventCallback & object, const CGHeroInstance & hero, SpellID spell);
	static void grantPrimarySkill(IGameEventCallback & object, const CGHeroInstance & hero, PrimarySkill skill, int amount);
	static void grantSecondarySkill(IGameEventCallback & object, const CGHeroInstance & hero, SecondarySkill skill, int level);
	static void grantArtifact(IGameEventCallback & object, const CGHeroInstance & hero, ArtifactID artifact);
	static void grantScroll(IGameEventCallback & object, const CGHeroInstance & hero, SpellID spell);

	static void showMessage(IGameEventCallback & object, PlayerColor player, const LuaMetaString & text,
		const std::optional<std::vector<LuaComponent>> & components, const std::optional<int> & soundID, const std::optional<int> & windowType);
};

}
