/*
 * Moat.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "Moat.h"

#include "Registry.h"
#include "../ISpellMechanics.h"

#include "../../entities/building/CBuilding.h"
#include "../../mapObjects/CGTownInstance.h"
#include "../../bonuses/Limiters.h"
#include "../../battle/IBattleState.h"
#include "../../battle/CBattleInfoCallback.h"
#include "../../entities/building/TownFortifications.h"
#include "../../json/JsonBonus.h"
#include "../../serializer/JsonSerializeFormat.h"
#include "../../networkPacks/PacksForClient.h"
#include "../../networkPacks/PacksForClientBattle.h"

VCMI_LIB_NAMESPACE_BEGIN

namespace spells
{
namespace effects
{

static void serializeMoatHexes(JsonSerializeFormat & handler, const std::string & fieldName, std::vector<BattleHexArray> & moatHexes)
{
	{
		JsonArraySerializer outer = handler.enterArray(fieldName);
		outer.syncSize(moatHexes, JsonNode::JsonType::DATA_VECTOR);

		for(size_t outerIndex = 0; outerIndex < outer.size(); outerIndex++)
		{
			JsonArraySerializer inner = outer.enterArray(outerIndex);
			inner.syncSize(moatHexes.at(outerIndex), JsonNode::JsonType::DATA_INTEGER);

			for(size_t innerIndex = 0; innerIndex < inner.size(); innerIndex++)
			{
				si16 hex = moatHexes.at(outerIndex).at(innerIndex).toInt();
				inner.serializeInt(innerIndex, hex);
				moatHexes.at(outerIndex).set(innerIndex, hex);
			}
		}
	}
}

void Moat::serializeJsonEffect(JsonSerializeFormat & handler)
{
	handler.serializeBool("hidden", hidden);
	handler.serializeBool("trap", trap);
	handler.serializeBool("removeOnTrigger", removeOnTrigger);
	handler.serializeBool("dispellable", dispellable);
	handler.serializeInt("moatDamage", moatDamage);
	serializeMoatHexes(handler, "moatHexes", moatHexes);
	handler.serializeId("triggerAbility", triggerAbility, SpellID::NONE);
	handler.serializeStruct("defender", sideOptions); //Moats are defender only

	assert(!handler.saving);
	{
		auto guard = handler.enterStruct("bonus");
		const JsonNode & data = handler.getCurrent();

		for(const auto & p : data.Struct())
		{
			//TODO: support JsonSerializeFormat in Bonus
			auto guard = handler.enterStruct(p.first);
			const JsonNode & bonusNode = handler.getCurrent();
			auto b = JsonUtils::parseBonus(bonusNode);
			bonus.push_back(b);
		}
	}
}

void Moat::convertBonus(const Mechanics * m, std::vector<Bonus> & converted) const
{

	for(const auto & b : bonus)
	{
		Bonus nb(*b);

		//Moat battlefield effect is always permanent
		nb.duration = BonusDuration::ONE_BATTLE;

		if(m->battle()->battleGetDefendedTown() && m->battle()->battleGetFortifications().hasMoat)
		{
			nb.sid = BonusSourceID(m->battle()->battleGetDefendedTown()->getTown()->buildings.at(BuildingID::CITADEL)->getUniqueTypeID());
			nb.source = BonusSource::TOWN_STRUCTURE;
		}
		else
		{
			nb.sid = BonusSourceID(m->getSpellId()); //for all
			nb.source = BonusSource::SPELL_EFFECT;//for all
		}
		BattleHexArray flatMoatHexes;

		for(const auto & moatPatch : moatHexes)
			flatMoatHexes.insert(moatPatch);

		nb.limiter = std::make_shared<UnitOnHexLimiter>(std::move(flatMoatHexes));
		converted.push_back(nb);
	}
}

void Moat::apply(ServerCallback * server, const Mechanics * m, const EffectTarget & target) const
{
	assert(m->isMassive());
	assert(m->battle()->battleGetDefendedTown());
	if(m->isMassive() && m->battle()->battleGetFortifications().hasMoat)
	{
		EffectTarget moat;
		placeObstacles(server, m, moat);

		std::vector<Bonus> converted;
		convertBonus(m, converted);
		for(auto & b : converted)
		{
			GiveBonus gb(GiveBonus::ETarget::BATTLE);
			gb.id = m->battle()->getBattle()->getBattleID();
			gb.bonus = b;
			server->apply(gb);
		}
	}
}

void Moat::placeObstacles(ServerCallback * server, const Mechanics * m, const EffectTarget & target) const
{
	assert(m->battle()->battleGetDefendedTown());
	assert(m->casterSide == BattleSide::DEFENDER); // Moats are always cast by defender

	BattleObstaclesChanged pack;
	pack.battleID = m->battle()->getBattle()->getBattleID();

	int obstacleIdToGive = m->battle()->nextObstacleId();

	for(const auto & destination : moatHexes)  //Moat hexes can be different obstacles
	{
		SpellCreatedObstacle obstacle;
		obstacle.uniqueID = obstacleIdToGive++;
		obstacle.pos = destination.at(0);
		obstacle.obstacleType = dispellable ? CObstacleInstance::SPELL_CREATED : CObstacleInstance::MOAT;
		obstacle.ID = m->getSpellIndex();

		obstacle.turnsRemaining = -1; //Moat cannot be expired
		obstacle.casterSpellPower = m->getEffectPower();
		obstacle.spellLevel = m->getEffectLevel(); //todo: level of indirect effect should be also configurable
		obstacle.casterSide = BattleSide::DEFENDER; // Moats are always cast by defender
		obstacle.minimalDamage = moatDamage; // Minimal moat damage
		obstacle.hidden = hidden;
		obstacle.passable = true; //Moats always passable
		obstacle.trigger = triggerAbility;
		obstacle.trap = trap;
		obstacle.removeOnTrigger = removeOnTrigger;
		obstacle.nativeVisible = false; //Moats is invisible for native terrain
		obstacle.appearSound = sideOptions.appearSound; //For dispellable moats
		obstacle.appearAnimation = sideOptions.appearAnimation; //For dispellable moats
		obstacle.animation = sideOptions.animation;
		obstacle.customSize.insert(destination);
		obstacle.animationYOffset = sideOptions.offsetY;
		pack.changes.emplace_back();
		obstacle.toInfo(pack.changes.back());
	}

	if(!pack.changes.empty())
		server->apply(pack);
}

}
}

VCMI_LIB_NAMESPACE_END
