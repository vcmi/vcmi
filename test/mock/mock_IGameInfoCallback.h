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
	//player
	MOCK_CONST_METHOD0(getLocalPlayer, PlayerColor());

	//hero
	MOCK_CONST_METHOD1(getHeroWithSubid, const CGHeroInstance *(int));

	//objects
	MOCK_CONST_METHOD2(getVisitableObjs, std::vector<const CGObjectInstance*>(int3, bool));
};



