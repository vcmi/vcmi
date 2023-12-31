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

VCMI_LIB_NAMESPACE_BEGIN

namespace battle
{
	class Unit;
}

class CGHeroInstance;

class DLL_LINKAGE BattleStateInfoForRetreat
{
public:
	bool canFlee;
	bool canSurrender;
	bool isLastTurnBeforeDie;
	ui8 ourSide;
	std::vector<const battle::Unit *> ourStacks;
	std::vector<const battle::Unit *> enemyStacks;
	const CGHeroInstance * ourHero;
	const CGHeroInstance * enemyHero;
	int turnsSkippedByDefense;

	BattleStateInfoForRetreat();
	uint64_t getOurStrength() const;
	uint64_t getEnemyStrength() const;
};

VCMI_LIB_NAMESPACE_END
