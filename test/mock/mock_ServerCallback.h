/*
 * mock_ServerCallback.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include <vcmi/ServerCallback.h>

class ServerCallbackMock : public ServerCallback
{
public:
	MOCK_CONST_METHOD0(describeChanges, bool());

	MOCK_METHOD1(complain, void(const std::string &));
	MOCK_METHOD0(getRNG, vstd::RNG *());

	MOCK_METHOD1(apply, void(CPackForClient *));

	MOCK_METHOD1(apply, void(BattleLogMessage *));
	MOCK_METHOD1(apply, void(BattleStackMoved *));
	MOCK_METHOD1(apply, void(BattleUnitsChanged *));
	MOCK_METHOD1(apply, void(SetStackEffect *));
	MOCK_METHOD1(apply, void(StacksInjured *));
	MOCK_METHOD1(apply, void(BattleObstaclesChanged *));
	MOCK_METHOD1(apply, void(CatapultAttack *));
};
