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
#include "../../bonuses/BonusSelector.h"
#include "../../callback/IGameInfoCallback.h"
#include "../../networkPacks/PacksForClientBattle.h"
#include "../../networkPacks/SetStackEffect.h"
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

MetaString BadMoraleAction::getStartLogLine(const CBattleInfoCallback & battle, const BattleAction & action) const
{
	const battle::Unit * unit = battle.battleGetUnitByID(action.stackNumber);

	MetaString text;
	unit->addText(text, EMetaText::GENERAL_TXT, -34);
	unit->addNameReplacement(text);
	return text;
}

bool WaitAction::isUnitAction() const
{
	return true;
}

bool WaitAction::validate(const IGameInfoCallback & game, const CBattleInfoCallback & battle, const BattleAction & action, spells::Problem & problem) const
{
	return checkUnitCanAct(battle, battle.battleGetUnitByID(action.stackNumber), problem);
}

void WaitAction::applyStartState(battle::CUnitState & actor, const BattleAction & action) const
{
	actor.waiting = true;
	actor.waitedThisTurn = true;
}

void WaitAction::apply(IBattleActionEnvironment & env, const CBattleInfoCallback & battle, const BattleAction & action) const
{
	const battle::Unit * unit = battle.battleGetUnitByID(action.stackNumber);

	env.fireCombatEvent(CombatEventType::WAIT, unit, nullptr);
	env.triggerObstaclesUnder(*unit);
}

MetaString WaitAction::getStartLogLine(const CBattleInfoCallback & battle, const BattleAction & action) const
{
	const battle::Unit * unit = battle.battleGetUnitByID(action.stackNumber);

	MetaString text;
	unit->addText(text, EMetaText::GENERAL_TXT, 136);
	unit->addNameReplacement(text);
	return text;
}

bool DefendAction::isUnitAction() const
{
	return true;
}

bool DefendAction::validate(const IGameInfoCallback & game, const CBattleInfoCallback & battle, const BattleAction & action, spells::Problem & problem) const
{
	return checkUnitCanAct(battle, battle.battleGetUnitByID(action.stackNumber), problem);
}

void DefendAction::applyStartState(battle::CUnitState & actor, const BattleAction & action) const
{
	actor.defending = true;
	actor.waiting = false;
}

void DefendAction::apply(IBattleActionEnvironment & env, const CBattleInfoCallback & battle, const BattleAction & action) const
{
	const battle::Unit * unit = battle.battleGetUnitByID(action.stackNumber);

	//defensive stance, TODO: filter out spell boosts from bonus (stone skin etc.)
	SetStackEffect sse;
	sse.battleID = battle.getBattle()->getBattleID();

	Bonus defenseBonusToAdd(BonusDuration::STACK_GETS_TURN, BonusType::PRIMARY_SKILL, BonusSource::OTHER, 20, BonusSourceID(), BonusSubtypeID(PrimarySkill::DEFENSE), BonusValueType::PERCENT_TO_ALL);
	Bonus bonus2(BonusDuration::STACK_GETS_TURN, BonusType::PRIMARY_SKILL, BonusSource::OTHER, unit->valOfBonuses(BonusType::DEFENSIVE_STANCE), BonusSourceID(), BonusSubtypeID(PrimarySkill::DEFENSE), BonusValueType::ADDITIVE_VALUE);
	Bonus alternativeWeakCreatureBonus(BonusDuration::STACK_GETS_TURN, BonusType::PRIMARY_SKILL, BonusSource::OTHER, 1, BonusSourceID(), BonusSubtypeID(PrimarySkill::DEFENSE), BonusValueType::ADDITIVE_VALUE);
	Bonus tagBonus(BonusDuration::STACK_GETS_TURN, BonusType::UNIT_DEFENDING, BonusSource::OTHER, 0, BonusSourceID());

	BonusList defence = *unit->getBonuses(Selector::typeSubtype(BonusType::PRIMARY_SKILL, BonusSubtypeID(PrimarySkill::DEFENSE)));
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

	sse.toUpdate.emplace_back(action.stackNumber, buffer);
	env.apply(sse);

	BattleLogMessage message;
	message.battleID = battle.getBattle()->getBattleID();

	MetaString text;
	unit->addText(text, EMetaText::GENERAL_TXT, 120);
	unit->addNameReplacement(text);
	text.replaceNumber(difference);

	message.lines.push_back(text);

	env.apply(message);

	env.fireCombatEvent(CombatEventType::DEFEND, unit, nullptr);
	env.triggerObstaclesUnder(*unit);
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
