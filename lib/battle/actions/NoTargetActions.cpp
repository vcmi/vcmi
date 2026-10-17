/*
 * NoTargetActions.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "NoTargetActions.h"

#include "IBattleActionEnvironment.h"

#include "../BattleAction.h"
#include "../CBattleInfoCallback.h"
#include "../CUnitState.h"
#include "../../bonuses/Bonus.h"
#include "../../bonuses/BonusList.h"
#include "../../bonuses/BonusSelector.h"
#include "../../callback/IGameInfoCallback.h"
#include "../../spells/Problem.h"

bool SkipTurnAction::isUnitAction() const
{
	return true;
}

bool SkipTurnAction::validate(const IGameInfoCallback & game, const CBattleInfoCallback & battle, const BattleAction & action, spells::Problem & problem) const
{
	return true;
}

void SkipTurnAction::apply(IBattleActionEnvironment & env, const CBattleInfoCallback & battle, const BattleAction & action) const
{
}

void BadMoraleAction::apply(IBattleActionEnvironment & env, const CBattleInfoCallback & battle, const BattleAction & action) const
{
	env.addBattleLogLine(getActor(battle, action).formatGeneralMessage(-34));
}

bool WaitAction::isUnitAction() const
{
	return true;
}

bool WaitAction::validate(const IGameInfoCallback & game, const CBattleInfoCallback & battle, const BattleAction & action, spells::Problem & problem) const
{
	return checkUnitCanAct(battle, battle.battleGetUnitByID(action.stackNumber), problem);
}

std::optional<battle::UnitTurnStatus> WaitAction::getStartTurnStatus(const BattleAction & action) const
{
	return battle::UnitTurnStatus::WAITING;
}

void WaitAction::apply(IBattleActionEnvironment & env, const CBattleInfoCallback & battle, const BattleAction & action) const
{
	const battle::Unit & unit = getActor(battle, action);

	env.addBattleLogLine(unit.formatGeneralMessage(136));
	env.fireCombatEvent(CombatEventType::WAIT, &unit, nullptr, nullptr);
	env.triggerObstaclesUnder(unit);
}

bool DefendAction::isUnitAction() const
{
	return true;
}

bool DefendAction::validate(const IGameInfoCallback & game, const CBattleInfoCallback & battle, const BattleAction & action, spells::Problem & problem) const
{
	return checkUnitCanAct(battle, battle.battleGetUnitByID(action.stackNumber), problem);
}

std::optional<battle::UnitTurnStatus> DefendAction::getStartTurnStatus(const BattleAction & action) const
{
	return battle::UnitTurnStatus::DEFENDED;
}

void DefendAction::apply(IBattleActionEnvironment & env, const CBattleInfoCallback & battle, const BattleAction & action) const
{
	const battle::Unit & unit = getActor(battle, action);

	//defensive stance, TODO: filter out spell boosts from bonus (stone skin etc.)
	Bonus defenseBonusToAdd(BonusDuration::STACK_GETS_TURN, BonusType::PRIMARY_SKILL, BonusSource::OTHER, 20, BonusSourceID(), BonusSubtypeID(PrimarySkill::DEFENSE), BonusValueType::PERCENT_TO_ALL);
	Bonus bonus2(BonusDuration::STACK_GETS_TURN, BonusType::PRIMARY_SKILL, BonusSource::OTHER, unit.valOfBonuses(BonusType::DEFENSIVE_STANCE), BonusSourceID(), BonusSubtypeID(PrimarySkill::DEFENSE), BonusValueType::ADDITIVE_VALUE);
	Bonus alternativeWeakCreatureBonus(BonusDuration::STACK_GETS_TURN, BonusType::PRIMARY_SKILL, BonusSource::OTHER, 1, BonusSourceID(), BonusSubtypeID(PrimarySkill::DEFENSE), BonusValueType::ADDITIVE_VALUE);
	Bonus tagBonus(BonusDuration::STACK_GETS_TURN, BonusType::UNIT_DEFENDING, BonusSource::OTHER, 0, BonusSourceID());

	BonusList defence = *unit.getBonuses(Selector::typeSubtype(BonusType::PRIMARY_SKILL, BonusSubtypeID(PrimarySkill::DEFENSE)));
	int oldDefenceValue = defence.totalValue();

	defence.push_back(std::make_shared<Bonus>(defenseBonusToAdd));
	defence.push_back(std::make_shared<Bonus>(bonus2));

	int difference = defence.totalValue() - oldDefenceValue;
	std::vector<Bonus> buffer;
	if(difference == 0) //give replacement bonus for creatures not reaching 5 defense points (20% of def becomes 0)
	{
		difference = 1;
		buffer.push_back(alternativeWeakCreatureBonus);
	}
	else
	{
		buffer.push_back(defenseBonusToAdd);
	}

	buffer.push_back(bonus2);
	buffer.push_back(tagBonus);

	env.updateUnitBonuses(unit, buffer);

	MetaString text = unit.formatGeneralMessage(120);
	text.replaceNumber(difference);

	env.addBattleLogLine(text);

	env.fireCombatEvent(CombatEventType::DEFEND, &unit, nullptr, nullptr);
	env.triggerObstaclesUnder(unit);
}

bool EndTacticsAction::isTacticsAction() const
{
	return true;
}

bool EndTacticsAction::validate(const IGameInfoCallback & game, const CBattleInfoCallback & battle, const BattleAction & action, spells::Problem & problem) const
{
	return true;
}

void EndTacticsAction::apply(IBattleActionEnvironment & env, const CBattleInfoCallback & battle, const BattleAction & action) const
{
	env.endTacticsPhase();
}

bool RetreatAction::isTacticsAction() const
{
	return true;
}

bool RetreatAction::isBattleEndAction() const
{
	return true;
}

bool RetreatAction::validate(const IGameInfoCallback & game, const CBattleInfoCallback & battle, const BattleAction & action, spells::Problem & problem) const
{
	if(!battle.battleCanFlee(battle.sideToPlayer(action.side)))
	{
		problem.add(MetaString::createFromRawString("Cannot retreat!"));
		return false;
	}
	return true;
}

void RetreatAction::apply(IBattleActionEnvironment & env, const CBattleInfoCallback & battle, const BattleAction & action) const
{
	env.endBattle(EBattleResult::ESCAPE, battle.otherSide(action.side));
}

bool SurrenderAction::isTacticsAction() const
{
	return true;
}

bool SurrenderAction::isBattleEndAction() const
{
	return true;
}

bool SurrenderAction::validate(const IGameInfoCallback & game, const CBattleInfoCallback & battle, const BattleAction & action, spells::Problem & problem) const
{
	PlayerColor player = battle.sideToPlayer(action.side);
	int cost = battle.battleGetSurrenderCost(player);
	if(cost < 0)
	{
		problem.add(MetaString::createFromRawString("Cannot surrender!"));
		return false;
	}

	if(game.getResource(player, EGameResID::GOLD) < cost)
	{
		problem.add(MetaString::createFromRawString("Not enough gold to surrender!"));
		return false;
	}
	return true;
}

void SurrenderAction::apply(IBattleActionEnvironment & env, const CBattleInfoCallback & battle, const BattleAction & action) const
{
	PlayerColor player = battle.sideToPlayer(action.side);
	env.giveResource(player, EGameResID::GOLD, -battle.battleGetSurrenderCost(player));
	env.endBattle(EBattleResult::SURRENDER, battle.otherSide(action.side));
}
