/*
 * ClassicBattleStateView.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "../StdInc.h"
#include "ClassicBattleStateView.h"

#include "../../../lib/CStack.h"
#include "../../../lib/battle/CBattleInfoCallback.h"
#include "../../../lib/bonuses/BonusSelector.h"

namespace
{
int32_t spellDuration(const CStack * stack, SpellID spell)
{
	const auto bonuses = stack->getBonuses(
		Selector::source(BonusSource::SPELL_EFFECT, BonusSourceID(spell)));
	int32_t result = 0;
	for(const auto & bonus : *bonuses)
		result = std::max(result, static_cast<int32_t>(bonus->turnsRemain));
	return result;
}

bool originalIncapacitated(const CStack * stack)
{
	return spellDuration(stack, SpellID(SpellID::BLIND)) > 0
		|| spellDuration(stack, SpellID(SpellID::STONE_GAZE)) > 0
		|| spellDuration(stack, SpellID(SpellID::PARALYZE)) > 0;
}

int32_t originalArrayIndex(const CStack * stack)
{
	if(stack->unitSlot().validSlot())
		return stack->unitSlot().getNum();
	// Special stacks are appended to the original side array. VCMI's unit ID
	// retains creation order when no adventure-army slot exists.
	return GameConstants::ARMY_SIZE + static_cast<int32_t>(stack->unitId());
}
}

ClassicBattleStateView::ClassicBattleStateView(std::shared_ptr<CBattleInfoCallback> battle)
	: battle(std::move(battle))
{
}

std::vector<const CStack *> ClassicBattleStateView::orderedStacks(bool includeDead, bool includeTurrets) const
{
	auto source = battle->battleGetAllStacks(includeTurrets);
	std::vector<const CStack *> result(source.begin(), source.end());
	if(!includeDead)
	{
		std::erase_if(
			result,
			[](const CStack * stack)
			{
				return !stack->alive();
			}
		);
	}

	std::stable_sort(
		result.begin(),
		result.end(),
		[](const CStack * lhs, const CStack * rhs)
		{
			if(lhs->unitSide() != rhs->unitSide())
				return lhs->unitSide() < rhs->unitSide();
			return originalArrayIndex(lhs) < originalArrayIndex(rhs);
		}
	);
	return result;
}

std::vector<ClassicMoveOrderEntry> ClassicBattleStateView::moveOrder(
	BattleSide activePhysicalSide,
	bool secondPhase) const
{
	std::vector<ClassicMoveOrderEntry> result;
	for(const CStack * stack : orderedStacks(false, true))
	{
		if(stack->isTurret())
			continue;

		int32_t key = 0;
		if(stack->isFirstAidTent() || stack->isAmmoCart())
			key = -100000;
		else if(spellDuration(stack, SpellID(SpellID::BLIND)) > 1
			|| spellDuration(stack, SpellID(SpellID::STONE_GAZE)) > 1)
		{
			key = -10000;
		}
		else
		{
			const int32_t speed = stack->getInitiative(0);
			if(stack->moved() || originalIncapacitated(stack))
				key = speed - 1000;
			else if(stack->waited() || secondPhase)
				key = -speed;
			else
				key = speed;
		}
		result.push_back({stack, key, 0});
	}

	std::stable_sort(
		result.begin(),
		result.end(),
		[](const ClassicMoveOrderEntry & lhs, const ClassicMoveOrderEntry & rhs)
		{
			if(lhs.key != rhs.key)
				return lhs.key > rhs.key;
			return originalArrayIndex(lhs.stack) < originalArrayIndex(rhs.stack);
		});

	BattleSide expectedSide = activePhysicalSide;
	for(size_t index = 0; index < result.size(); ++index)
	{
		if(result[index].stack->unitSide() != expectedSide)
		{
			for(size_t candidate = index + 1;
				candidate < result.size() && result[candidate].key == result[index].key;
				++candidate)
			{
				if(result[candidate].stack->unitSide() == expectedSide)
				{
					std::swap(result[index], result[candidate]);
					break;
				}
			}
		}
		expectedSide = result[index].stack->unitSide() == BattleSide::ATTACKER
			? BattleSide::DEFENDER
			: BattleSide::ATTACKER;
		result[index].order = static_cast<uint32_t>(result.size() - index);
	}
	return result;
}

std::vector<const CStack *> ClassicBattleStateView::orderedEnemies(const CStack * attacker, bool includeDead) const
{
	std::vector<const CStack *> result;
	for(const CStack * candidate : orderedStacks(includeDead))
	{
		if(candidate != attacker && isEnemy(attacker, candidate))
			result.push_back(candidate);
	}
	return result;
}

std::vector<const CStack *> ClassicBattleStateView::orderedFriendlies(const CStack * subject, bool includeDead) const
{
	std::vector<const CStack *> result;
	for(const CStack * candidate : orderedStacks(includeDead))
	{
		if(candidate == subject || !isEnemy(subject, candidate))
			result.push_back(candidate);
	}
	return result;
}

BattleSide ClassicBattleStateView::controllingSide(const CStack * stack) const
{
	return battle->playerToSide(battle->battleGetOwner(stack));
}

bool ClassicBattleStateView::isEnemy(const CStack * attacker, const CStack * candidate) const
{
	if(attacker->hasBonusOfType(BonusType::ATTACKS_NEAREST_CREATURE))
		return attacker != candidate;
	return battle->battleMatchOwner(attacker, candidate);
}
