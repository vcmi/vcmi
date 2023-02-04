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

VCMI_LIB_NAMESPACE_BEGIN

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

void Teleport::apply(ServerCallback * server, const Mechanics * m, const EffectTarget & target) const
{
	if(target.size() != 2)
	{
		server->complain("Teleport requires 2 destinations.");
		return;
	}

	auto targetUnit = target[0].unitValue;
	if(nullptr == targetUnit)
	{
		server->complain("No unit to teleport");
		return;
	}

	const BattleHex destination = target[1].hexValue;
	if(!destination.isValid())
	{
		server->complain("Invalid teleport destination");
		return;
	}

	//TODO: move here all teleport checks
	if(!m->battle()->battleCanTeleportTo(targetUnit, destination, m->getEffectLevel()))
	{
		server->complain("Forbidden teleport.");
		return;
	}

	BattleStackMoved pack;
	pack.distance = 0;
	pack.stack = targetUnit->unitId();
	std::vector<BattleHex> tiles;
	tiles.push_back(destination);
	pack.tilesToMove = tiles;
	pack.teleporting = true;
	server->apply(&pack);
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

VCMI_LIB_NAMESPACE_END
