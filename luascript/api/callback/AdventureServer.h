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
class Artifact;
class Creature;
class ResourceType;
class Skill;
namespace spells { class Spell; }
class CGObjectInstance;
class CGHeroInstance;
class CGTownInstance;

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
	static void finishQuestOrRemoveObject(IGameEventCallback & object, const CGObjectInstance & target);
	static void markQuestProposed(IGameEventCallback & object, const CGObjectInstance & target, PlayerColor player);
	static void addToQuestLog(IGameEventCallback & object, const CGObjectInstance & target, PlayerColor player);
	static void setQuestHintText(IGameEventCallback & object, const CGObjectInstance & target, const LuaMetaString & text);
	static int random(IGameEventCallback & object, int lower, int upper);

	static void giveExperience(IGameEventCallback & object, const CGHeroInstance & hero, int64_t amount);
	static void giveResource(IGameEventCallback & object, PlayerColor player, const ResourceType & resource, int amount);
	static void setOwner(IGameEventCallback & object, const CGObjectInstance & target, PlayerColor owner);
	static void grantSpell(IGameEventCallback & object, const CGHeroInstance & hero, const spells::Spell & spell);
	static void takeSpell(IGameEventCallback & object, const CGHeroInstance & hero, const spells::Spell & spell);
	static void grantPrimarySkill(IGameEventCallback & object, const CGHeroInstance & hero, PrimarySkill skill, int amount);
	static void grantSecondarySkill(IGameEventCallback & object, const CGHeroInstance & hero, const Skill & skill, int level);
	static void grantArtifact(IGameEventCallback & object, const CGHeroInstance & hero, const Artifact & artifact);
	static void grantScroll(IGameEventCallback & object, const CGHeroInstance & hero, const spells::Spell & spell);
	static void takeArtifact(IGameEventCallback & object, const CGHeroInstance & hero, const Artifact & artifact);
	static void grantCreatures(IGameEventCallback & object, const CGHeroInstance & hero, const Creature & creature, int count);
	static void takeCreatures(IGameEventCallback & object, const CGHeroInstance & hero, const Creature & creature, int count);
	static void grantWarMachine(IGameEventCallback & object, const CGHeroInstance & hero, const Artifact & machine);
	static void takeWarMachine(IGameEventCallback & object, const CGHeroInstance & hero, const Artifact & machine);
	static void grantSpellbook(IGameEventCallback & object, const CGHeroInstance & hero);
	static void takeSpellbook(IGameEventCallback & object, const CGHeroInstance & hero);
	static void grantMorale(IGameEventCallback & object, const CGHeroInstance & hero, int amount);
	static void grantLuck(IGameEventCallback & object, const CGHeroInstance & hero, int amount);
	static void grantSpellPoints(IGameEventCallback & object, const CGHeroInstance & hero, int amount, int mode);
	static void grantMovementPoints(IGameEventCallback & object, const CGHeroInstance & hero, int amount, int mode);
	static void grantCreaturesToHire(IGameEventCallback & object, const CGTownInstance & town, int level, int count);
	static void constructBuilding(IGameEventCallback & object, const CGTownInstance & town, BuildingID building);

	static void showMessage(IGameEventCallback & object, PlayerColor player, const LuaMetaString & text,
		const std::optional<std::vector<LuaComponent>> & components, const std::optional<int> & soundID, const std::optional<int> & windowType);

	static void spawnDialog(IGameEventCallback & object, PlayerColor player, const LuaMetaString & text,
		int mode, const std::optional<std::vector<LuaComponent>> & components);
	static void spawnCombat(IGameEventCallback & object, const CGObjectInstance & host, const CGHeroInstance & hero, const JsonNode & army);
};

}
