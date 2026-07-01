/*
 * Registry.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "Registry.h"
#include "SerializableRegistar.h"

#include "../../lib/battle/CBattleInfoCallback.h"
#include "../../lib/battle/IBattleState.h"
#include "../../lib/json/JsonNode.h"
#include "../../lib/mapObjects/CGObjectInstance.h"

#include <boost/core/demangle.hpp>

#include "Enums.h"
#include "LuaMetaString.h"
#include "battle/SpellObstacleDescriptor.h"
#include "battle/Unit.h"
#include "battle/UnitState.h"
#include "battle/BattleHex.h"
#include "battle/BattleHexArray.h"
#include "battle/Obstacle.h"
#include "spells/Mechanics.h"
#include "spells/Problem.h"
#include "library/Artifact.h"
#include "library/Bonus.h"
#include "library/BonusDescriptor.h"
#include "callback/IBattleInfoCallback.h"
#include "library/Creature.h"
#include "library/Faction.h"
#include "callback/IGameInfoCallback.h"
#include "library/HeroClass.h"
#include "adventure/HeroInstance.h"
#include "library/HeroType.h"
#include "callback/ServerCallback.h"
#include "library/Services.h"
#include "library/Skill.h"
#include "library/Spell.h"
#include "library/SpellSchool.h"
#include "adventure/StackInstance.h"

VCMI_LIB_NAMESPACE_BEGIN

namespace scripting::api
{

Registry::Registry()
{
	registerPrivate<ServicesProxy>();
	registerPrivate<ArtifactProxy>();
	registerPrivate<BonusProxy>();
	registerPrivate<BonusListProxy>();
	registerPrivate<CreatureProxy>();
	registerPrivate<FactionProxy>();
	registerPrivate<HeroClassProxy>();
	registerPrivate<HeroTypeProxy>();
	registerPrivate<SkillProxy>();
	registerPrivate<SpellProxy>();
	registerPrivate<SpellSchoolProxy>();

	registerPrivate<HeroInstanceProxy>();
	registerPrivate<StackInstanceProxy>();

	registerPrivate<BattleHexProxy>();
	registerPrivate<BattleHexArrayProxy>();
	registerPrivate<UnitProxy>();
	registerPrivate<LuaUnitStateProxy>();
	registerPrivate<ObstacleProxy>();
	registerPrivate<ProblemProxy>();
	registerPrivate<MechanicsProxy>();

	registerPrivate<IBattleInfoCallbackProxy>();
	registerPrivate<IGameInfoCallbackProxy>();
	registerPrivate<ServerCallbackProxy>();

	registerSerializable<Enums>();
	registerSerializable<LuaMetaString>();
	registerSerializable<BonusDescriptor>();
	registerSerializable<SpellObstacleDescriptor>();

	// Aliases for C++ types that have no dedicated proxy but appear in binding signatures.
	registerLuaName<CBattleInfoCallback>("Battle");
	registerLuaName<CGObjectInstance>("MapObject");
	registerLuaName<battle::UnitInfo>("UnitInfo");
	// JsonNode fields accept any Lua value (string / number / table / …) and are funneled
	// through JsonUtils::parseBonus — surface that openness rather than `userdata`.
	registerLuaName<JsonNode>("any");

	// Named enum aliases — the proxy registrations above don't cover these because they have no proxy of their own
	registerLuaName<EHealLevel>("HealLevel");
	registerLuaName<EHealPower>("HealPower");
	registerLuaName<ESpellCastProblem>("SpellCastProblem");
	registerLuaName<::spells::AimType>("AimType");
	registerLuaName<BonusDuration::BonusDuration>("BonusDuration");
	registerLuaName<BonusSource>("BonusSource");
	registerLuaName<BonusValueType>("BonusValueType");
	registerLuaName<CObstacleInstance::EObstacleType>("ObstacleType");
	registerLuaName<EWallPart>("WallPart");
	registerLuaName<BattleSide>("BattleSide");

	// EWallState has no enum group of its own and is exposed as integer to Lua
	registerLuaName<EWallState>("integer");
}

std::string Registry::lookupLuaName(std::type_index t) const
{
	auto it = luaNameByType.find(t);
	if(it == luaNameByType.end())
		throw std::runtime_error("No Lua type name registered for C++ type: " + boost::core::demangle(t.name()));
	return it->second;
}

const Registry * Registry::get()
{
	static const Registry instance;
	return &instance;
}

void Registry::addPrivate(const std::string & name, const std::shared_ptr<Registar> & item)
{
	privateData[name] = item;
}

void Registry::addPublic(const std::string & name, const std::shared_ptr<Registar> & item)
{
	privateData[name] = item;
	publicData[name] = item;
}

const Registar * Registry::find(const std::string & name) const
{
	auto iter = publicData.find(name);
	if(iter == publicData.end())
		return nullptr;
	else
		return iter->second.get();
}

}

VCMI_LIB_NAMESPACE_END
