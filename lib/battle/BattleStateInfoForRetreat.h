/*
 * BattleStateInfoForRetreat.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "BattleSide.h"

VCMI_LIB_NAMESPACE_BEGIN

namespace battle
{
	class Unit;
	using Units = boost::container::small_vector<const Unit *, 4>;
}

class CGHeroInstance;

class DLL_LINKAGE BattleStateInfoForRetreat
{
public:
	bool canFlee;
	bool canSurrender;
	bool isLastTurnBeforeDie;
	BattleSide ourSide;
	battle::Units ourStacks;
	battle::Units enemyStacks;
	const CGHeroInstance * ourHero;
	const CGHeroInstance * enemyHero;
	int turnsSkippedByDefense;

	BattleStateInfoForRetreat();
	uint64_t getOurStrength() const;
	uint64_t getEnemyStrength() const;
};

VCMI_LIB_NAMESPACE_END
