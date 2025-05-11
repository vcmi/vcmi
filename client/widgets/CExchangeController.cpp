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
#include "../GameInstance.h"

#include "../widgets/CGarrisonInt.h"

#include "../lib/callback/CCallback.h"
#include "../lib/mapObjects/CGHeroInstance.h"

CExchangeController::CExchangeController(ObjectInstanceID hero1, ObjectInstanceID hero2)
	: left(GAME->interface()->cb->getHero(hero1))
	, right(GAME->interface()->cb->getHero(hero2))
{
}

void CExchangeController::swapArmy()
{
	const auto & leftSlots = left->Slots();
	const auto & rightSlots = right->Slots();

	auto i = leftSlots.begin();
	auto j = rightSlots.begin();

	for(; i != leftSlots.end() && j != rightSlots.end(); i++, j++)
	{
		GAME->interface()->cb->swapCreatures(left, right, i->first, j->first);
	}

	if(i != leftSlots.end())
	{
		auto freeSlots = right->getFreeSlots();
		auto slot = freeSlots.begin();

		for(; i != leftSlots.end() && slot != freeSlots.end(); i++, slot++)
		{
			GAME->interface()->cb->swapCreatures(left, right, i->first, *slot);
		}
	}
	else if(j != rightSlots.end())
	{
		auto freeSlots = left->getFreeSlots();
		auto slot = freeSlots.begin();

		for(; j != rightSlots.end() && slot != freeSlots.end(); j++, slot++)
		{
			GAME->interface()->cb->swapCreatures(left, right, *slot, j->first);
		}
	}
}

void CExchangeController::moveArmy(bool leftToRight, std::optional<SlotID> heldSlot)
{
	const auto source = leftToRight ? left : right;
	const auto target = leftToRight ? right : left;

	if(!heldSlot.has_value())
	{
		const auto & weakestSlot = vstd::minElementByFun(source->Slots(),
			[](const auto & s) -> int
			{
				return s.second->getCreatureID().toCreature()->getAIValue();
			});
		heldSlot = weakestSlot->first;
	}
	
	if (source->getCreature(heldSlot.value()) == nullptr)
		return;

	GAME->interface()->cb->bulkMoveArmy(source->id, target->id, heldSlot.value());
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
			GAME->interface()->cb->splitStack(source, target, sourceSlot, targetSlot,
				target->getStackCount(targetSlot) + source->getStackCount(sourceSlot) - 1);
		}
		else
		{
			GAME->interface()->cb->mergeOrSwapStacks(source, target, sourceSlot, targetSlot);
		}
	}
}

void CExchangeController::moveSingleStackCreature(bool leftToRight, SlotID sourceSlot, bool forceEmptySlotTarget)
{
	const auto source = leftToRight ? left : right;
	const auto target = leftToRight ? right : left;
	auto creature = source->getCreature(sourceSlot);

	if(creature == nullptr || source->stacksCount() == 1)
		return;

	SlotID targetSlot = forceEmptySlotTarget ? target->getFreeSlot() : target->getSlotFor(creature);
	if(targetSlot.validSlot())
	{
		GAME->interface()->cb->splitStack(source, target, sourceSlot, targetSlot, target->getStackCount(targetSlot) + 1);
	}
}

void CExchangeController::swapArtifacts(bool equipped, bool baclpack)
{
	GAME->interface()->cb->bulkMoveArtifacts(left->id, right->id, true, equipped, baclpack);
}

void CExchangeController::moveArtifacts(bool leftToRight, bool equipped, bool baclpack)
{
	const auto source = leftToRight ? left : right;
	const auto target = leftToRight ? right : left;

	GAME->interface()->cb->bulkMoveArtifacts(source->id, target->id, false, equipped, baclpack);
}
