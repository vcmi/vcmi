/*
 * AttackPossibility.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "AttackPossibility.h"

int AttackPossibility::damageDiff() const
{
	if (!priorities)
		priorities = new Priorities();
	const auto dealtDmgValue = priorities->stackEvaluator(enemy) * damageDealt;
	const auto receivedDmgValue = priorities->stackEvaluator(attack.attacker) * damageReceived;
	return dealtDmgValue - receivedDmgValue;
}

int AttackPossibility::attackValue() const
{
	return damageDiff() + tacticImpact;
}

AttackPossibility AttackPossibility::evaluate(const BattleAttackInfo &AttackInfo, const HypotheticChangesToBattleState &state, BattleHex hex)
{
	auto attacker = AttackInfo.attacker;
	auto enemy = AttackInfo.defender;

	const int remainingCounterAttacks = getValOr(state.counterAttacksLeft, enemy, enemy->counterAttacks.available());
	const bool counterAttacksBlocked = attacker->hasBonusOfType(Bonus::BLOCKS_RETALIATION) || enemy->hasBonusOfType(Bonus::NO_RETALIATION);
	const int totalAttacks = 1 + AttackInfo.attackerBonuses->getBonuses(Selector::type(Bonus::ADDITIONAL_ATTACK), (Selector::effectRange (Bonus::NO_LIMIT).Or(Selector::effectRange(Bonus::ONLY_MELEE_FIGHT))))->totalValue();

	AttackPossibility ap = {enemy, hex, AttackInfo, 0, 0, 0};

	auto curBai = AttackInfo; //we'll modify here the stack counts
	for(int i  = 0; i < totalAttacks; i++)
	{
		std::pair<ui32, ui32> retaliation(0,0);
		auto attackDmg = getCbc()->battleEstimateDamage(CRandomGenerator::getDefault(), curBai, &retaliation);
		ap.damageDealt = (attackDmg.first + attackDmg.second) / 2;
		ap.damageReceived = (retaliation.first + retaliation.second) / 2;

		if(remainingCounterAttacks <= i || counterAttacksBlocked)
			ap.damageReceived = 0;

		curBai.attackerHealth = attacker->healthAfterAttacked(ap.damageReceived);
		curBai.defenderHealth = enemy->healthAfterAttacked(ap.damageDealt);
		if(curBai.attackerHealth.getCount() <= 0)
			break;
		//TODO what about defender? should we break? but in pessimistic scenario defender might be alive
	}

	//TODO other damage related to attack (eg. fire shield and other abilities)

	return ap;
}


Priorities* AttackPossibility::priorities = nullptr;
