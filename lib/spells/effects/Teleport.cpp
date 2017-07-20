/*
 * Teleport.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "Teleport.h"
#include "Registry.h"
#include "Registry.h"
#include "../ISpellMechanics.h"
#include "../../NetPacks.h"
#include "../../battle/CBattleInfoCallback.h"
#include "../../battle/Unit.h"

//TODO: Teleport effect

static const std::string EFFECT_NAME = "core:teleport";

namespace spells
{
namespace effects
{
VCMI_REGISTER_SPELL_EFFECT(Teleport, EFFECT_NAME);

Teleport::Teleport()
	: UnitEffect()
{
}

Teleport::~Teleport() = default;

void Teleport::adjustTargetTypes(std::vector<TargetType> & types) const
{
	if(!types.empty())
	{
		if(types[0] != AimType::CREATURE)
		{
			types.clear();
			return;
		}

		if(types.size() == 1)
		{
			types.push_back(AimType::LOCATION);
		}
		else if(types.size() > 1)
		{
			if(types[1] != AimType::LOCATION)
				types.clear();
		}
	}
}

bool Teleport::applicable(Problem & problem, const Mechanics * m) const
{
	return UnitEffect::applicable(problem, m);
}

void Teleport::apply(BattleStateProxy * battleState, RNG & rng, const Mechanics * m, const EffectTarget & target) const
{
	BattleStackMoved pack;
	std::string errorMessage;

	if(prepareEffects(errorMessage, pack, m, target))
		battleState->apply(&pack);
	else
		battleState->complain(errorMessage);
}

bool Teleport::prepareEffects(std::string & errorMessage, BattleStackMoved & pack, const Mechanics * m, const EffectTarget & target) const
{
	if(target.size() != 2)
	{
		errorMessage = "Teleport requires 2 destinations.";
		return false;
	}

	auto targetUnit = target[0].unitValue;
	if(nullptr == targetUnit)
	{
		errorMessage = "No unit to teleport";
		return false;
	}

	const BattleHex destination = target[1].hexValue;
	if(!destination.isValid())
	{
		errorMessage = "Invalid teleport destination";
		return false;
	}

	//TODO: move here all teleport checks
	if(!m->cb->battleCanTeleportTo(targetUnit, destination, m->getEffectLevel()))
	{
		errorMessage = "Forbidden teleport.";
		return false;
	}

	pack.distance = 0;
	pack.stack = targetUnit->unitId();
	std::vector<BattleHex> tiles;
	tiles.push_back(destination);
	pack.tilesToMove = tiles;
	pack.teleporting = true;

	return true;
}


void Teleport::serializeJsonUnitEffect(JsonSerializeFormat & handler)
{
	//TODO: teleport options
}

EffectTarget Teleport::transformTarget(const Mechanics * m, const Target & aimPoint, const Target & spellTarget) const
{
	//first transformed destination is unit to teleport, let base class handle immunity etc.
	//second spell destination is destination tile, use it directly
	EffectTarget transformed = UnitEffect::transformTarget(m, aimPoint, spellTarget);

	EffectTarget ret;
	if(!transformed.empty())
		ret.push_back(transformed.front());
	if(aimPoint.size() == 2)
		ret.push_back(aimPoint.back());

	return ret;
}

}
}
