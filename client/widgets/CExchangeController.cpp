/*
 * CExchangeController.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "CExchangeController.h"

#include "../CPlayerInterface.h"

#include "../widgets/CGarrisonInt.h"

#include "../../CCallback.h"

#include "../lib/mapObjects/CGHeroInstance.h"

CExchangeController::CExchangeController(ObjectInstanceID hero1, ObjectInstanceID hero2)
	: left(LOCPLINT->cb->getHero(hero1))
	, right(LOCPLINT->cb->getHero(hero2))
{
}

void CExchangeController::swapArmy()
{
	auto getStacks = [](const CArmedInstance * source) -> std::vector<std::pair<SlotID, CStackInstance*>>
	{
		auto slots = source->Slots();
		return std::vector<std::pair<SlotID, CStackInstance*>>(slots.begin(), slots.end());
	};

	auto leftSlots = getStacks(left);
	auto rightSlots = getStacks(right);

	auto i = leftSlots.begin();
	auto j = rightSlots.begin();

	for(; i != leftSlots.end() && j != rightSlots.end(); i++, j++)
	{
		LOCPLINT->cb->swapCreatures(left, right, i->first, j->first);
	}

	if(i != leftSlots.end())
	{
		auto freeSlots = right->getFreeSlots();
		auto slot = freeSlots.begin();

		for(; i != leftSlots.end() && slot != freeSlots.end(); i++, slot++)
		{
			LOCPLINT->cb->swapCreatures(left, right, i->first, *slot);
		}
	}
	else if(j != rightSlots.end())
	{
		auto freeSlots = left->getFreeSlots();
		auto slot = freeSlots.begin();

		for(; j != rightSlots.end() && slot != freeSlots.end(); j++, slot++)
		{
			LOCPLINT->cb->swapCreatures(left, right, *slot, j->first);
		}
	}
}

void CExchangeController::moveArmy(bool leftToRight, std::optional<SlotID> heldSlot)
{
	const auto source = leftToRight ? left : right;
	const auto target = leftToRight ? right : left;

	if(!heldSlot.has_value())
	{
		auto weakestSlot = vstd::minElementByFun(source->Slots(),
			[](const std::pair<SlotID, CStackInstance*> & s) -> int
			{
				return s.second->getCreatureID().toCreature()->getAIValue();
			});
		heldSlot = weakestSlot->first;
	}
	LOCPLINT->cb->bulkMoveArmy(source->id, target->id, heldSlot.value());
}

void CExchangeController::moveStack(bool leftToRight, SlotID sourceSlot)
{
	const auto source = leftToRight ? left : right;
	const auto target = leftToRight ? right : left;
	auto creature = source->getCreature(sourceSlot);

	if(creature == nullptr)
		return;

	SlotID targetSlot = target->getSlotFor(creature);
	if(targetSlot.validSlot())
	{
		if(source->stacksCount() == 1 && source->needsLastStack())
		{
			LOCPLINT->cb->splitStack(source, target, sourceSlot, targetSlot,
				target->getStackCount(targetSlot) + source->getStackCount(sourceSlot) - 1);
		}
		else
		{
			LOCPLINT->cb->mergeOrSwapStacks(source, target, sourceSlot, targetSlot);
		}
	}
}

void CExchangeController::swapArtifacts(bool equipped, bool baclpack)
{
	LOCPLINT->cb->bulkMoveArtifacts(left->id, right->id, true, equipped, baclpack);
}

void CExchangeController::moveArtifacts(bool leftToRight, bool equipped, bool baclpack)
{
	const auto source = leftToRight ? left : right;
	const auto target = leftToRight ? right : left;

	LOCPLINT->cb->bulkMoveArtifacts(source->id, target->id, false, equipped, baclpack);
}
