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

CCallbackBase::CCallbackBase(std::optional<PlayerColor> Player):
	player(std::move(Player))
{
}

std::optional<PlayerColor> CCallbackBase::getPlayerID() const
{
	return player;
}


VCMI_LIB_NAMESPACE_END
