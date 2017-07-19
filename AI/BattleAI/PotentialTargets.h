/*
 * PotentialTargets.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once
#include "AttackPossibility.h"

class PotentialTargets
{
public:
	std::vector<AttackPossibility> possibleAttacks;
	std::vector<const CStack *> unreachableEnemies;

	//std::function<AttackPossibility(bool,BattleHex)>  GenerateAttackInfo; //args: shooting, destHex

	PotentialTargets(){};
	PotentialTargets(const CStack * attacker, const HypotheticChangesToBattleState & state = HypotheticChangesToBattleState());

	AttackPossibility bestAction() const;
	int bestActionValue() const;
};
