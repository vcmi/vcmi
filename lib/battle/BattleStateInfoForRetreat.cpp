/*
 * BattleStateInfoForRetreat.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "BattleStateInfoForRetreat.h"
#include "Unit.h"
#include "CBattleInfoCallback.h"
#include "../CCreatureSet.h"
#include "../mapObjects/CGHeroInstance.h"

VCMI_LIB_NAMESPACE_BEGIN

BattleStateInfoForRetreat::BattleStateInfoForRetreat():
	canFlee(false),
	canSurrender(false),
	isLastTurnBeforeDie(false),
	ourHero(nullptr),
	enemyHero(nullptr),
	ourSide(-1)
{
}

uint64_t getFightingStrength(const std::vector<const battle::Unit *> & stacks, const CGHeroInstance * hero = nullptr)
{
	uint64_t result = 0;

	for(const battle::Unit * stack : stacks)
	{
		result += stack->creatureId().toCreature()->getAIValue() * stack->getCount();
	}

	if(hero)
	{
		result = static_cast<uint64_t>(result * hero->getFightingStrength());
	}

	return result;
}

uint64_t BattleStateInfoForRetreat::getOurStrength() const
{
	return getFightingStrength(ourStacks, ourHero);
}

uint64_t BattleStateInfoForRetreat::getEnemyStrength() const
{
	return getFightingStrength(enemyStacks, enemyHero);
}

VCMI_LIB_NAMESPACE_END
