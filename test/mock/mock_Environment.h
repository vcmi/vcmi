/*
 * mock_Environment.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include <vcmi/Environment.h>

class EnvironmentMock : public Environment
{
public:
	MOCK_CONST_METHOD0(services, const Services *());
	MOCK_CONST_METHOD1(battle, const BattleCb *(const BattleID & battleID));
	MOCK_CONST_METHOD0(game, const GameCb *());
	MOCK_CONST_METHOD0(logger, ::vstd::CLoggerBase *());
	MOCK_CONST_METHOD0(eventBus, ::events::EventBus *());
};
