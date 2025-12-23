/*
 * DemonSummon.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "DemonSummon.h"
#include "Registry.h"
#include "../ISpellMechanics.h"
#include "../../battle/CBattleInfoCallback.h"
#include "../../battle/BattleInfo.h"
#include "../../battle/CUnitState.h"
#include "../../CStack.h"
#include "../../networkPacks/PacksForClientBattle.h"
#include "../../serializer/JsonSerializeFormat.h"

VCMI_LIB_NAMESPACE_BEGIN

namespace spells
{
namespace effects
{

int DemonSummon::raisedCreatureAmount(const Mechanics * m, const battle::Unit * unit) const
{
	if(!unit || unit->alive() || unit->isGhost())
		return 0;

	const auto *creatureType = creature.toEntity(m->creatures());

	int32_t deadCount         = unit->unitBaseAmount();
	int32_t deadTotalHealth   = unit->getTotalHealth();
	int32_t raisedMaxHealth   = creatureType->getMaxHealth();
	int32_t raisedTotalHealth = m->applySpellBonus(m->getEffectValue(), unit);

	// Can't raise stack with more HP than original stack
	int32_t maxAmountFromHealth     = deadTotalHealth / raisedMaxHealth;
	// Can't raise stack with more creatures than original stack
	int32_t maxAmountFromAmount     = deadCount;
	// Can't raise stack with more HP than our spellpower
	int32_t maxAmountFromSpellpower = raisedTotalHealth / raisedMaxHealth;

	int32_t finalAmount = std::min( { maxAmountFromHealth, maxAmountFromAmount, maxAmountFromSpellpower } );

	return finalAmount;
}

void DemonSummon::apply(ServerCallback * server, const Mechanics * m, const EffectTarget & target) const
{
	BattleUnitsChanged pack;
	pack.battleID = m->battle()->getBattle()->getBattleID();

	for(const Destination & dest : target)
	{
		const battle::Unit * targetStack = dest.unitValue;

		//we shall have all targets to be stacks
		if(!targetStack || targetStack->alive() || targetStack->isGhost())
		{
			server->complain("No corpse to demonize! Invalid effect target transformation.");
			continue;
		}

		auto hex = m->battle()->getAvailableHex(targetStack->creatureId(), m->casterSide, targetStack->getPosition().toInt());

		if(!hex.isValid())
		{
			server->complain("No place to put new summon!");
			break;
		}

		int32_t finalAmount = raisedCreatureAmount(m, targetStack);

		if(finalAmount < 1)
		{
			server->complain("Summoning didn't summon any!");
			continue;
		}

		battle::UnitInfo info;
		info.id       = m->battle()->battleNextUnitId();
		info.count    = finalAmount;
		info.type     = creature;
		info.side     = m->casterSide;
		info.position = dest.hexValue;
		info.summoned = !permanent;

		// add newly created creature
		pack.changedStacks.emplace_back(info.id, UnitChanges::EOperation::ADD);
		info.save(pack.changedStacks.back().data);

		// and remove corpse to prevent second raising or resurrection
		pack.changedStacks.emplace_back(targetStack->unitId(), UnitChanges::EOperation::REMOVE);
	}

	if(!pack.changedStacks.empty())
		server->apply(pack);
}

SpellEffectValue DemonSummon::getHealthChange(const Mechanics * m, const EffectTarget & spellTarget) const
{
	SpellEffectValue result;

	auto targets = m->getAffectedStacks(spellTarget);

	if(targets.empty())
		return result;

	auto unit = targets.front();
	if(unit)
	{
		result.unitsDelta = raisedCreatureAmount(m, unit);
		result.unitType = creature;
	}

	return result;
}

bool DemonSummon::isValidTarget(const Mechanics * m, const battle::Unit * unit) const
{
	if(!unit->isDead())
		return false;

	//check if alive unit blocks rising
	for(const BattleHex & hex : unit->getHexes())
	{
		auto blocking = m->battle()->battleGetUnitsIf([hex, unit](const battle::Unit * other)
		{
			return other->isValidTarget(false) && other->coversPos(hex) && other != unit;
		});

		if(!blocking.empty())
			return false;
	}

	if (unit->isGhost())
		return false;

	if (raisedCreatureAmount(m, unit) == 0)
		return false;

	return m->isReceptive(unit);
}

void DemonSummon::serializeJsonUnitEffect(JsonSerializeFormat & handler)
{
	handler.serializeId("id", creature, CreatureID());
	handler.serializeBool("permanent", permanent, false);
}

}
}

VCMI_LIB_NAMESPACE_END
