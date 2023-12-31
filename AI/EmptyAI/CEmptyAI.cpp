/*
 * CEmptyAI.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "CEmptyAI.h"

#include "../../lib/CRandomGenerator.h"
#include "../../lib/CStack.h"
#include "../../lib/battle/BattleAction.h"

void CEmptyAI::saveGame(BinarySerializer & h, const int version)
{
}

void CEmptyAI::loadGame(BinaryDeserializer & h, const int version)
{
}

void CEmptyAI::initGameInterface(std::shared_ptr<Environment> ENV, std::shared_ptr<CCallback> CB)
{
	cb = CB;
	env = ENV;
	human=false;
	playerID = *cb->getPlayerID();
}

void CEmptyAI::yourTurn(QueryID queryID)
{
	cb->selectionMade(0, queryID);
	cb->endTurn();
}

void CEmptyAI::activeStack(const BattleID & battleID, const CStack * stack)
{
	cb->battleMakeUnitAction(battleID, BattleAction::makeDefend(stack));
}

void CEmptyAI::yourTacticPhase(const BattleID & battleID, int distance)
{
	cb->battleMakeTacticAction(battleID, BattleAction::makeEndOFTacticPhase(cb->getBattle(battleID)->battleGetTacticsSide()));
}

void CEmptyAI::heroGotLevel(const CGHeroInstance *hero, PrimarySkill pskill, std::vector<SecondarySkill> &skills, QueryID queryID)
{
	cb->selectionMade(CRandomGenerator::getDefault().nextInt((int)skills.size() - 1), queryID);
}

void CEmptyAI::commanderGotLevel(const CCommanderInstance * commander, std::vector<ui32> skills, QueryID queryID)
{
	cb->selectionMade(CRandomGenerator::getDefault().nextInt((int)skills.size() - 1), queryID);
}

void CEmptyAI::showBlockingDialog(const std::string &text, const std::vector<Component> &components, QueryID askID, const int soundID, bool selection, bool cancel)
{
	cb->selectionMade(0, askID);
}

void CEmptyAI::showTeleportDialog(const CGHeroInstance * hero, TeleportChannelID channel, TTeleportExitsList exits, bool impassable, QueryID askID)
{
	cb->selectionMade(0, askID);
}

void CEmptyAI::showGarrisonDialog(const CArmedInstance *up, const CGHeroInstance *down, bool removableUnits, QueryID queryID)
{
	cb->selectionMade(0, queryID);
}

void CEmptyAI::showMapObjectSelectDialog(QueryID askID, const Component & icon, const MetaString & title, const MetaString & description, const std::vector<ObjectInstanceID> & objects)
{
	cb->selectionMade(0, askID);
}

std::optional<BattleAction> CEmptyAI::makeSurrenderRetreatDecision(const BattleID & battleID, const BattleStateInfoForRetreat & battleState)
{
	return std::nullopt;
}
