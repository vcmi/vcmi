/*
 * RemoveObstacle.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "RemoveObstacle.h"

#include "Registry.h"
#include "../ISpellMechanics.h"

#include "../../NetPacks.h"
#include "../../battle/IBattleState.h"
#include "../../battle/CBattleInfoCallback.h"
#include "../../battle/obstacle/Obstacle.h"
#include "../../serializer/JsonSerializeFormat.h"

static const std::string EFFECT_NAME = "core:removeObstacle";

namespace spells
{
namespace effects
{

VCMI_REGISTER_SPELL_EFFECT(RemoveObstacle, EFFECT_NAME);

RemoveObstacle::RemoveObstacle()
	: LocationEffect(),
	removeAbsolute(false),
	removeUsual(false),
	removeAllSpells(false)
{
}

RemoveObstacle::~RemoveObstacle() = default;

bool RemoveObstacle::applicable(Problem & problem, const Mechanics * m) const
{
	return !getTargets(m, EffectTarget(), true).empty();
}

bool RemoveObstacle::applicable(Problem & problem, const Mechanics * m, const EffectTarget & target) const
{
	return !getTargets(m, target, false).empty();
}

void RemoveObstacle::apply(BattleStateProxy * battleState, RNG & rng, const Mechanics * m, const EffectTarget & target) const
{
	BattleObstaclesChanged pack;

	for(const auto & obstacle : getTargets(m, target, false))
		pack.changes.emplace_back(obstacle->ID, BattleChanges::EOperation::REMOVE);

	if(!pack.changes.empty())
		battleState->apply(&pack);
}

void RemoveObstacle::serializeJsonEffect(JsonSerializeFormat & handler)
{
	handler.serializeBool("removeAbsolute", removeAbsolute, false);
	handler.serializeBool("removeUsual", removeUsual, false);
	handler.serializeBool("removeAllSpells", removeAllSpells, false);
	handler.serializeIdArray("removeSpells", removeSpells);
}

bool RemoveObstacle::canRemove(const Obstacle * obstacle) const
{
	if(removeAbsolute && obstacle->getType() == ObstacleType::STATIC)
		return true;
	if(removeUsual && obstacle->getType() == ObstacleType::STATIC)
		return true;
	auto spellObstacle = dynamic_cast<const SpellCreatedObstacle *>(obstacle);

	if(removeAllSpells && spellObstacle)
		return true;

	if(spellObstacle && !removeSpells.empty())
	{
		if(vstd::contains(removeSpells, spellObstacle->spellID))
			return true;
	}

	return false;
}

std::set<const Obstacle *> RemoveObstacle::getTargets(const Mechanics * m, const EffectTarget & target, bool alwaysMassive) const
{
	std::set<const Obstacle *> possibleTargets;
	if(m->isMassive() || alwaysMassive)
	{
		for(const auto & obstacle : m->cb->battleGetAllObstacles())
			if(canRemove(obstacle.get()))
				possibleTargets.insert(obstacle.get());
	}
	else
	{
		for(const auto & destination : target)
			if(destination.hexValue.isValid())
				for(const auto & obstacle : m->cb->battleGetAllObstaclesOnPos(destination.hexValue, false))
					if(canRemove(obstacle.get()))
						possibleTargets.insert(obstacle.get());
	}

	return possibleTargets;
}


}
}
