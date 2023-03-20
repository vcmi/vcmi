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

#include "../../NetPacks.h"
#include "../../mapObjects/CGTownInstance.h"
#include "../../battle/IBattleState.h"
#include "../../battle/CBattleInfoCallback.h"
#include "../../serializer/JsonSerializeFormat.h"

VCMI_LIB_NAMESPACE_BEGIN

static const std::string EFFECT_NAME = "core:moat";

namespace spells
{
namespace effects
{

VCMI_REGISTER_SPELL_EFFECT(Moat, EFFECT_NAME);

static void serializeMoatHexes(JsonSerializeFormat & handler, const std::string & fieldName, std::vector<std::vector<BattleHex>> & moatHexes)
{
	{
		JsonArraySerializer outer = handler.enterArray(fieldName);
		outer.syncSize(moatHexes, JsonNode::JsonType::DATA_VECTOR);

		for(size_t outerIndex = 0; outerIndex < outer.size(); outerIndex++)
		{
			JsonArraySerializer inner = outer.enterArray(outerIndex);
			inner.syncSize(moatHexes.at(outerIndex), JsonNode::JsonType::DATA_INTEGER);

			for(size_t innerIndex = 0; innerIndex < inner.size(); innerIndex++)
				inner.serializeInt(innerIndex, moatHexes.at(outerIndex).at(innerIndex));
		}
	}
}

void Moat::serializeJsonEffect(JsonSerializeFormat & handler)
{
	handler.serializeBool("hidden", hidden);
	handler.serializeBool("trigger", trigger);
	handler.serializeBool("trap", trap);
	handler.serializeBool("removeOnTrigger", removeOnTrigger);
	handler.serializeBool("dispellable", dispellable);
	handler.serializeInt("moatDamage", moatDamage);
	serializeMoatHexes(handler, "moatHexes", moatHexes);
	handler.serializeId("triggerAbility", triggerAbility, SpellID::NONE);
	handler.serializeStruct("defender", sideOptions); //Moats are defender only
}

void Moat::apply(ServerCallback * server, const Mechanics * m, const EffectTarget & target) const
{
	assert(m->isMassive());
	assert(m->battle()->battleGetDefendedTown());
	if(m->isMassive() && m->battle()->battleGetSiegeLevel() >= CGTownInstance::CITADEL)
	{
		EffectTarget moat;
		placeObstacles(server, m, moat);
	}
}

void Moat::placeObstacles(ServerCallback * server, const Mechanics * m, const EffectTarget & target) const
{
	assert(m->battle()->battleGetDefendedTown());
	assert(m->casterSide == BattleSide::DEFENDER); // Moats are always cast by defender
	assert(turnsRemaining < 0); // Moats should lasts infininte number of turns

	BattleObstaclesChanged pack;

	auto all = m->battle()->battleGetAllObstacles(BattlePerspective::ALL_KNOWING);

	int obstacleIdToGive = 1;
	for(auto & one : all)
		if(one->uniqueID >= obstacleIdToGive)
			obstacleIdToGive = one->uniqueID + 1;

	for(const auto & destination : moatHexes)  //Moat hexes can be different obstacles
	{
		SpellCreatedObstacle obstacle;
		obstacle.uniqueID = obstacleIdToGive++;
		obstacle.pos = destination.at(0);
		obstacle.obstacleType = dispellable ? CObstacleInstance::SPELL_CREATED : CObstacleInstance::MOAT;
		obstacle.ID = triggerAbility;

		obstacle.turnsRemaining = -1; //Moat cannot be expired
		obstacle.casterSpellPower = m->getEffectPower();
		obstacle.spellLevel = m->getEffectLevel(); //todo: level of indirect effect should be also configurable
		obstacle.casterSide = BattleSide::DEFENDER; // Moats are always cast by defender
		obstacle.minimalDamage = moatDamage; // Minimal moat damage
		obstacle.hidden = hidden;
		obstacle.passable = true; //Moats always passable
		obstacle.trigger = trigger;
		obstacle.trap = trap;
		obstacle.removeOnTrigger = removeOnTrigger;
		obstacle.nativeVisible = false; //Moats is invisible for native terrain
		obstacle.appearSound = sideOptions.appearSound; //For dispellable moats
		obstacle.appearAnimation = sideOptions.appearAnimation; //For dispellable moats
		obstacle.animation = sideOptions.animation;
		obstacle.customSize.insert(obstacle.customSize.end(),destination.cbegin(), destination.cend());
		obstacle.animationYOffset = sideOptions.offsetY;
		pack.changes.emplace_back();
		obstacle.toInfo(pack.changes.back());
	}

	if(!pack.changes.empty())
		server->apply(&pack);
}

}
}

VCMI_LIB_NAMESPACE_END
