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
#include "IBattleState.h"

VCMI_LIB_NAMESPACE_BEGIN

bool CCallbackBase::duringBattle() const
{
	return getBattle() != nullptr;
}

const IBattleInfo * CCallbackBase::getBattle() const
{
	return battle;
}

CCallbackBase::CCallbackBase(boost::optional<PlayerColor> Player):
	player(std::move(Player))
{
}

void CCallbackBase::setBattle(const IBattleInfo * B)
{
	battle = B;
}

boost::optional<PlayerColor> CCallbackBase::getPlayerID() const
{
	return player;
}


VCMI_LIB_NAMESPACE_END
