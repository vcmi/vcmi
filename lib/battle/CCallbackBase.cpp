/*
 * CCallbackBase.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "CCallbackBase.h"
#include "BattleInfo.h"
#include "CGameState.h"

bool CCallbackBase::duringBattle() const
{
	return getBattle() != nullptr;
}

const BattleInfo *CCallbackBase::getBattle() const
{
	return battle;
}

CCallbackBase::CCallbackBase(CGameState * GS, boost::optional<PlayerColor> Player)
	: battle(nullptr), gs(GS), player(Player)
{}

CCallbackBase::CCallbackBase()
	: battle(nullptr), gs(nullptr)
{}

void CCallbackBase::setBattle(const BattleInfo *B)
{
	battle = B;
}

boost::optional<PlayerColor> CCallbackBase::getPlayerID() const
{
	return player;
}

