/*
 * Obstacle.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "Obstacle.h"

#include "Registry.h"
#include "../ISpellMechanics.h"

#include "../../NetPacks.h"
#include "../../battle/IBattleState.h"
#include "../../battle/CBattleInfoCallback.h"
#include "../../serializer/JsonSerializeFormat.h"

VCMI_LIB_NAMESPACE_BEGIN

static const std::string EFFECT_NAME = "core:obstacle";

namespace spells
{
namespace effects
{

VCMI_REGISTER_SPELL_EFFECT(Obstacle, EFFECT_NAME);

ObstacleSideOptions::ObstacleSideOptions()
	: shape(),
	range()
{
}

void ObstacleSideOptions::serializeJson(JsonSerializeFormat & handler)
{
	serializeRelativeShape(handler, "shape", shape);
	serializeRelativeShape(handler, "range", range);

	handler.serializeString("appearSound", appearSound);
	handler.serializeString("appearAnimation", appearAnimation);
	handler.serializeString("triggerSound", triggerSound);
	handler.serializeString("triggerAnimation", triggerAnimation);
	handler.serializeString("animation", animation);

	handler.serializeInt("offsetY", offsetY);
}

void ObstacleSideOptions::serializeRelativeShape(JsonSerializeFormat & handler, const std::string & fieldName, RelativeShape & value)
{
	static const std::vector<std::string> EDirMap =
	{
		"TL",
		"TR",
		"R",
		"BR",
		"BL",
		"L",
	};

	{
		JsonArraySerializer outer = handler.enterArray(fieldName);
		outer.syncSize(value, JsonNode::JsonType::DATA_VECTOR);

		for(size_t outerIndex = 0; outerIndex < outer.size(); outerIndex++)
		{
			JsonArraySerializer inner = outer.enterArray(outerIndex);
			inner.syncSize(value.at(outerIndex), JsonNode::JsonType::DATA_STRING);

			for(size_t innerIndex = 0; innerIndex < inner.size(); innerIndex++)
			{
				std::string temp;

				if(handler.saving)
				{
					auto index = value.at(outerIndex).at(innerIndex);
					if (index < EDirMap.size())
						temp = EDirMap[index];
				}

				inner.serializeString(innerIndex, temp);

				if(!handler.saving)
				{
					value.at(outerIndex).at(innerIndex) = (BattleHex::EDir) vstd::find_pos(EDirMap, temp);
				}
			}
		}
	}

	if(!handler.saving)
	{
		if(value.empty())
			value.emplace_back();

		if(value.back().empty())
			value.back().emplace_back(BattleHex::EDir::NONE);
	}
}

Obstacle::Obstacle()
	: LocationEffect(),
	hidden(false),
	passable(false),
	trigger(false),
	trap(false),
	removeOnTrigger(false),
	patchCount(1),
	turnsRemaining(-1)
{
}

Obstacle::~Obstacle() = default;

void Obstacle::adjustAffectedHexes(std::set<BattleHex> & hexes, const Mechanics * m, const Target & spellTarget) const
{
	EffectTarget effectTarget = transformTarget(m, spellTarget, spellTarget);

	const ObstacleSideOptions & options = sideOptions.at(m->casterSide);

	for(auto & destination : effectTarget)
	{
		for(auto & trasformation : options.shape)
		{
			BattleHex hex = destination.hexValue;

			for(auto direction : trasformation)
				hex.moveInDirection(direction, false);

			if(hex.isValid())
				hexes.insert(hex);
		}
	}
}

bool Obstacle::applicable(Problem & problem, const Mechanics * m) const
{
	return LocationEffect::applicable(problem, m);
}

bool Obstacle::applicable(Problem & problem, const Mechanics * m, const EffectTarget & target) const
{
	if(!m->isMassive())
	{
		const bool requiresClearTiles = m->requiresClearTiles();
		const ObstacleSideOptions & options = sideOptions.at(m->casterSide);

		if(target.empty())
			return noRoomToPlace(problem, m);

		for(const auto & destination : target)
		{
			for(auto & trasformation : options.shape)
			{
				BattleHex hex = destination.hexValue;
				for(auto direction : trasformation)
					hex.moveInDirection(direction, false);

				if(!isHexAvailable(m->battle(), hex, requiresClearTiles))
					return noRoomToPlace(problem, m);
			}
		}
	}

	return LocationEffect::applicable(problem, m, target);
}

EffectTarget Obstacle::transformTarget(const Mechanics * m, const Target & aimPoint, const Target & spellTarget) const
{
	const ObstacleSideOptions & options = sideOptions.at(m->casterSide);

	EffectTarget ret;

	if(!m->isMassive())
	{
		for(auto & spellDestination : spellTarget)
		{
			for(auto & rangeShape : options.range)
			{
				BattleHex hex = spellDestination.hexValue;

				for(auto direction : rangeShape)
					hex.moveInDirection(direction, false);

				ret.emplace_back(hex);
			}
		}
	}

	return ret;
}

void Obstacle::apply(ServerCallback * server, const Mechanics * m, const EffectTarget & target) const
{
	if(m->isMassive())
	{
		std::vector<BattleHex> availableTiles;
		for(int i = 0; i < GameConstants::BFIELD_SIZE; i++)
		{
			BattleHex hex = i;
			if(isHexAvailable(m->battle(), hex, true))
				availableTiles.push_back(hex);
		}
		RandomGeneratorUtil::randomShuffle(availableTiles, *server->getRNG());

		const int patchesToPut = std::min(patchCount, (int)availableTiles.size());

		EffectTarget randomTarget;
		randomTarget.reserve(patchesToPut);
		for(int i = 0; i < patchesToPut; i++)
			randomTarget.emplace_back(availableTiles.at(i));

		placeObstacles(server, m, randomTarget);
	}
	else
	{
		placeObstacles(server, m, target);
	}
}

void Obstacle::serializeJsonEffect(JsonSerializeFormat & handler)
{
	handler.serializeBool("hidden", hidden);
	handler.serializeBool("passable", passable);
	handler.serializeBool("trigger", trigger);
	handler.serializeBool("trap", trap);
    handler.serializeBool("removeOnTrigger", removeOnTrigger);

	handler.serializeInt("patchCount", patchCount);
	handler.serializeInt("turnsRemaining", turnsRemaining, -1);

	handler.serializeStruct("attacker", sideOptions.at(BattleSide::ATTACKER));
	handler.serializeStruct("defender", sideOptions.at(BattleSide::DEFENDER));
}

bool Obstacle::isHexAvailable(const CBattleInfoCallback * cb, const BattleHex & hex, const bool mustBeClear)
{
	if(!hex.isAvailable())
		return false;

	if(!mustBeClear)
		return true;

	if(cb->battleGetUnitByPos(hex, true))
		return false;

	auto obst = cb->battleGetAllObstaclesOnPos(hex, false);

	for(auto & i : obst)
		if(i->obstacleType != CObstacleInstance::MOAT)
			return false;

	if(cb->battleGetSiegeLevel() != 0)
	{
		EWallPart::EWallPart part = cb->battleHexToWallPart(hex);

		if(part == EWallPart::INVALID || part == EWallPart::INDESTRUCTIBLE_PART_OF_GATE)
			return true;//no fortification here
		else if(static_cast<int>(part) < 0)
			return false;//indestructible part (cant be checked by battleGetWallState)
		else if(part == EWallPart::BOTTOM_TOWER || part == EWallPart::UPPER_TOWER)
			return false;//destructible, but should not be available
		else if(cb->battleGetWallState(part) != EWallState::DESTROYED && cb->battleGetWallState(part) != EWallState::NONE)
			return false;
	}

	return true;
}

bool Obstacle::noRoomToPlace(Problem & problem, const Mechanics * m)
{
	MetaString text;
	text.addTxt(MetaString::GENERAL_TXT, 181);//No room to place %s here
	text.addReplacement(m->getSpellName());
	problem.add(std::move(text));
	return false;
}

void Obstacle::placeObstacles(ServerCallback * server, const Mechanics * m, const EffectTarget & target) const
{
	const ObstacleSideOptions & options = sideOptions.at(m->casterSide);

	BattleObstaclesChanged pack;

	boost::optional<BattlePerspective::BattlePerspective> perspective;

	if(!m->battle()->getPlayerID())
		perspective = boost::make_optional(BattlePerspective::ALL_KNOWING);

	auto all = m->battle()->battleGetAllObstacles(perspective);

	int obstacleIdToGive = 1;
	for(auto & one : all)
		if(one->uniqueID >= obstacleIdToGive)
			obstacleIdToGive = one->uniqueID + 1;

	for(const Destination & destination : target)
	{
		SpellCreatedObstacle obstacle;
		obstacle.uniqueID = obstacleIdToGive++;
		obstacle.pos = destination.hexValue;
		obstacle.obstacleType = CObstacleInstance::USUAL;
		obstacle.ID = m->getSpellIndex();

		obstacle.turnsRemaining = turnsRemaining;
		obstacle.casterSpellPower = m->getEffectPower();
		obstacle.spellLevel = m->getEffectLevel();//todo: level of indirect effect should be also configurable
		obstacle.casterSide = m->casterSide;

		obstacle.hidden = hidden;
		obstacle.passable = passable;
		obstacle.trigger = trigger;
		obstacle.trap = trap;
		obstacle.removeOnTrigger = removeOnTrigger;

		obstacle.appearSound = options.appearSound;
		obstacle.appearAnimation = options.appearAnimation;
		obstacle.triggerSound = options.triggerSound;
		obstacle.triggerAnimation = options.triggerAnimation;
		obstacle.animation = options.animation;

		obstacle.animationYOffset = options.offsetY;

		obstacle.customSize.clear();
		obstacle.customSize.reserve(options.shape.size());

		for(auto & shape : options.shape)
		{
			BattleHex hex = destination.hexValue;

			for(auto direction : shape)
				hex.moveInDirection(direction, false);

			obstacle.customSize.emplace_back(hex);
		}

		pack.changes.emplace_back();
		obstacle.toInfo(pack.changes.back());
	}

	if(!pack.changes.empty())
		server->apply(&pack);
}


}
}

VCMI_LIB_NAMESPACE_END
