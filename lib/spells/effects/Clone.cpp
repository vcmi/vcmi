/*
 * Clone.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "Clone.h"
#include "Registry.h"
#include "../ISpellMechanics.h"
#include "../../NetPacks.h"
#include "../../battle/CBattleInfoCallback.h"
#include "../../battle/CUnitState.h"
#include "../../serializer/JsonSerializeFormat.h"

VCMI_LIB_NAMESPACE_BEGIN

static const std::string EFFECT_NAME = "core:clone";

namespace spells
{
namespace effects
{

VCMI_REGISTER_SPELL_EFFECT(Clone, EFFECT_NAME);

Clone::Clone()
	: UnitEffect(),
	maxTier(0)
{
}

Clone::~Clone() = default;

void Clone::apply(ServerCallback * server, const Mechanics * m, const EffectTarget & target) const
{
	for(const Destination & dest : target)
	{
		const battle::Unit * clonedStack = dest.unitValue;

		//we shall have all targets to be stacks
		if(!clonedStack)
		{
			server->complain("No target stack to clone! Invalid effect target transformation.");
			continue;
		}

		//should not happen, but in theory we might have stack took damage from other effects
		if(clonedStack->getCount() < 1)
			continue;

		auto hex = m->battle()->getAvaliableHex(clonedStack->creatureId(), m->casterSide, clonedStack->getPosition());

		if(!hex.isValid())
		{
			server->complain("No place to put new clone!");
			break;
		}

		auto unitId = m->battle()->battleNextUnitId();

		battle::UnitInfo info;
		info.id = unitId;
		info.count = clonedStack->getCount();
		info.type = clonedStack->creatureId();
		info.side = m->casterSide;
		info.position = hex;
		info.summoned = true;

		BattleUnitsChanged pack;
		pack.changedStacks.emplace_back(info.id, UnitChanges::EOperation::ADD);
		info.save(pack.changedStacks.back().data);
		server->apply(&pack);

		//TODO: use BattleUnitsChanged with UPDATE operation

		BattleUnitsChanged cloneFlags;

		auto cloneUnit = m->battle()->battleGetUnitByID(unitId);

		if(!cloneUnit)
		{
			server->complain("[Internal error] Cloned unit missing.");
			continue;
		}

		auto cloneState = cloneUnit->acquireState();
		cloneState->cloned = true;
		cloneFlags.changedStacks.emplace_back(cloneState->unitId(), UnitChanges::EOperation::RESET_STATE);
		cloneState->save(cloneFlags.changedStacks.back().data);

		auto originalState = clonedStack->acquireState();
		originalState->cloneID = unitId;
		cloneFlags.changedStacks.emplace_back(originalState->unitId(), UnitChanges::EOperation::RESET_STATE);
		originalState->save(cloneFlags.changedStacks.back().data);

		server->apply(&cloneFlags);

		SetStackEffect sse;
		Bonus lifeTimeMarker(Bonus::N_TURNS, Bonus::NONE, Bonus::SPELL_EFFECT, 0, SpellID::CLONE); //TODO: use special bonus type
		lifeTimeMarker.turnsRemain = m->getEffectDuration();
		std::vector<Bonus> buffer;
		buffer.push_back(lifeTimeMarker);
		sse.toAdd.push_back(std::make_pair(unitId, buffer));
		server->apply(&sse);
	}
}

bool Clone::isReceptive(const Mechanics * m, const battle::Unit * s) const
{
	int creLevel = s->creatureLevel();
	if(creLevel > maxTier)
		return false;

	//use default algorithm only if there is no mechanics-related problem
	return UnitEffect::isReceptive(m, s);
}

bool Clone::isValidTarget(const Mechanics * m, const battle::Unit * s) const
{
	//can't clone already cloned creature
	if(s->isClone())
		return false;
	//can`t clone if old clone still alive
	if(s->hasClone())
		return false;

	return UnitEffect::isValidTarget(m, s);
}

void Clone::serializeJsonUnitEffect(JsonSerializeFormat & handler)
{
	handler.serializeInt("maxTier", maxTier);
}

}
}

VCMI_LIB_NAMESPACE_END
