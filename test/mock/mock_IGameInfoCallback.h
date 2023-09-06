/*
 * mock_IGameInfoCallback.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "../../lib/CGameInfoCallback.h"

class IGameInfoCallbackMock : public IGameInfoCallback
{
public:
	//various
	MOCK_CONST_METHOD1(getDate, int(Date));
	MOCK_CONST_METHOD2(isAllowed, bool(int32_t, int32_t));

	//player
	MOCK_CONST_METHOD1(getPlayer, const Player *(PlayerColor));
	MOCK_CONST_METHOD0(getLocalPlayer, PlayerColor());
	MOCK_CONST_METHOD0(getPlayerID, std::optional<PlayerColor>());

	//hero
	MOCK_CONST_METHOD1(getHero, const CGHeroInstance *(ObjectInstanceID));
	MOCK_CONST_METHOD1(getHeroWithSubid, const CGHeroInstance *(int));

	//objects
	MOCK_CONST_METHOD2(getObj, const CGObjectInstance *(ObjectInstanceID, bool));
	MOCK_CONST_METHOD2(getVisitableObjs, std::vector<const CGObjectInstance*>(int3, bool));
};



