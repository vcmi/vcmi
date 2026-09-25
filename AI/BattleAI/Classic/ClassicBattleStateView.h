/*
 * ClassicBattleStateView.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../../../lib/battle/BattleSide.h"

class CBattleInfoCallback;
class CStack;

struct ClassicMoveOrderEntry
{
	const CStack * stack = nullptr;
	int32_t key = 0;
	uint32_t order = 0;
};

/// Stable, original-style ordering and allegiance view over a VCMI battle.
class ClassicBattleStateView
{
	std::shared_ptr<CBattleInfoCallback> battle;

public:
	explicit ClassicBattleStateView(std::shared_ptr<CBattleInfoCallback> battle);

	std::vector<const CStack *> orderedStacks(bool includeDead = false, bool includeTurrets = false) const;
	std::vector<const CStack *> orderedEnemies(const CStack * attacker, bool includeDead = false) const;
	std::vector<const CStack *> orderedFriendlies(const CStack * subject, bool includeDead = false) const;
	std::vector<ClassicMoveOrderEntry> moveOrder(BattleSide activePhysicalSide, bool secondPhase) const;
	BattleSide controllingSide(const CStack * stack) const;
	bool isEnemy(const CStack * attacker, const CStack * candidate) const;
};
