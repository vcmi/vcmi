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

AttackPossibility::AttackPossibility(BattleHex tile_, const BattleAttackInfo & attack_)
	: tile(tile_),
	attack(attack_)
{
}


int64_t AttackPossibility::damageDiff() const
{
	//TODO: use target priority from HypotheticBattle
	const auto dealtDmgValue = damageDealt;
	const auto receivedDmgValue = damageReceived;

	int64_t diff = 0;

	//friendly fire or not
	if(attack.attacker->unitSide() == attack.defender->unitSide())
		diff = -dealtDmgValue - receivedDmgValue;
	else
		diff = dealtDmgValue - receivedDmgValue;

	//mind control
	auto actualSide = getCbc()->playerToSide(getCbc()->battleGetOwner(attack.attacker));
	if(actualSide && actualSide.get() != attack.attacker->unitSide())
		diff = -diff;
	return diff;
}

int64_t AttackPossibility::attackValue() const
{
	return damageDiff() + tacticImpact;
}

AttackPossibility AttackPossibility::evaluate(const BattleAttackInfo & attackInfo, BattleHex hex)
{
	const std::string cachingStringBlocksRetaliation = "type_BLOCKS_RETALIATION";
	static const auto selectorBlocksRetaliation = Selector::type(Bonus::BLOCKS_RETALIATION);

	const bool counterAttacksBlocked = attackInfo.attacker->hasBonus(selectorBlocksRetaliation, cachingStringBlocksRetaliation);

	AttackPossibility ap(hex, attackInfo);

	ap.attackerState = attackInfo.attacker->acquireState();

	const int totalAttacks = ap.attackerState->getTotalAttacks(attackInfo.shooting);

	if(!attackInfo.shooting)
		ap.attackerState->setPosition(hex);

	auto defenderState = attackInfo.defender->acquireState();
	ap.affectedUnits.push_back(defenderState);

	for(int i = 0; i < totalAttacks; i++)
	{
		TDmgRange retaliation(0,0);
		auto attackDmg = getCbc()->battleEstimateDamage(ap.attack, &retaliation);

		vstd::amin(attackDmg.first, defenderState->getAvailableHealth());
		vstd::amin(attackDmg.second, defenderState->getAvailableHealth());

		vstd::amin(retaliation.first, ap.attackerState->getAvailableHealth());
		vstd::amin(retaliation.second, ap.attackerState->getAvailableHealth());

		ap.damageDealt += (attackDmg.first + attackDmg.second) / 2;

		ap.attackerState->afterAttack(attackInfo.shooting, false);

		//FIXME: use ranged retaliation
		if(!attackInfo.shooting && defenderState->ableToRetaliate() && !counterAttacksBlocked)
		{
			ap.damageReceived += (retaliation.first + retaliation.second) / 2;
			defenderState->afterAttack(attackInfo.shooting, true);
		}

		ap.attackerState->damage(ap.damageReceived);
		defenderState->damage(ap.damageDealt);

		if(!ap.attackerState->alive() || !defenderState->alive())
			break;
	}

	//TODO other damage related to attack (eg. fire shield and other abilities)

	return ap;
}
