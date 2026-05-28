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
#include "../ISpellMechanics.h"
#include "../../battle/IBattleState.h"
#include "../../battle/CBattleInfoCallback.h"
#include "../../battle/Unit.h"
#include "../../entities/building/TownFortifications.h"
#include "../../networkPacks/PacksForClientBattle.h"
#include "../../serializer/JsonSerializeFormat.h"

VCMI_LIB_NAMESPACE_BEGIN

namespace spells
{
namespace effects
{

void Teleport::adjustTargetTypes(std::vector<TargetType> & types, const Mechanics * m) const
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

bool Teleport::applicableTarget(Problem & problem, const Mechanics * m, const Target & target) const
{
	if(target.size() == 1) //Assume, this is check only for selecting a unit
		return UnitEffect::applicableTarget(problem, m, target);

	if(target.size() != 2)
		return m->adaptProblem(ESpellCastProblem::WRONG_SPELL_TARGET, problem);

	const auto *targetUnit = target[0].unitValue;
	const auto & targetHex = target[1].hexValue;

	if(!targetUnit)
		return m->adaptProblem(ESpellCastProblem::WRONG_SPELL_TARGET, problem);

	if(!targetHex.isValid() || !m->battle()->getAccessibility(targetUnit).accessible(targetHex, targetUnit))
		return m->adaptProblem(ESpellCastProblem::WRONG_SPELL_TARGET, problem);

	if(m->battle()->battleGetFortifications().wallsHealth > 0 && !(isWallPassable && isMoatPassable))
	{
		return !m->battle()->battleHasPenaltyOnLine(target[0].hexValue, target[1].hexValue, !isWallPassable, !isMoatPassable);
	}
	return true;
}

void Teleport::apply(ServerCallback * server, const Mechanics * m, const Target & target) const
{
	const auto *targetUnit = target[0].unitValue;
	const auto destination = target[1].hexValue;

	BattleStackMoved pack;
	pack.battleID = m->battle()->getBattle()->getBattleID();
	pack.distance = 0;
	pack.stack = targetUnit->unitId();
	BattleHexArray tiles;
	tiles.insert(destination);
	pack.tilesToMove = tiles;
	pack.teleporting = true;
	server->apply(pack);

	if(triggerObstacles)
	{
		auto spellEnv = dynamic_cast<SpellCastEnvironment*>(server);
		m->battle()->handleObstacleTriggersForUnit(*spellEnv, *targetUnit);
	}
}

void Teleport::serializeJsonUnitEffect(JsonSerializeFormat & handler)
{
	handler.serializeBool("triggerObstacles", triggerObstacles);
	handler.serializeBool("isWallPassable", isWallPassable);
	handler.serializeBool("isMoatPassable", isMoatPassable);
}

Target Teleport::transformTarget(const Mechanics * m, const Target & aimPoint, const Target & spellTarget) const
{
	//first transformed destination is unit to teleport, let base class handle immunity etc.
	//second spell destination is destination tile, use it directly
	Target transformed = UnitEffect::transformTarget(m, aimPoint, spellTarget);

	Target ret;
	if(!transformed.empty())
		ret.push_back(transformed.front());
	if(aimPoint.size() == 2)
		ret.push_back(aimPoint.back());

	return ret;
}

}
}

VCMI_LIB_NAMESPACE_END
