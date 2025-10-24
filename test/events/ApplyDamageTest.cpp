/*
 * ApplyDamageTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include <vcmi/events/EventBus.h>

#include "../../lib/events/ApplyDamage.h"
#include "../../lib/networkPacks/PacksForClientBattle.h"

#include "../mock/mock_Environment.h"
#include "../mock/mock_battle_Unit.h"

namespace test
{
using namespace ::testing;
using namespace ::events;

class ApplyDamageListenerMock
{
public:
	virtual ~ApplyDamageListenerMock() = default;

	MOCK_METHOD1(beforeEvent, void(ApplyDamage &));
	MOCK_METHOD1(afterEvent, void(const ApplyDamage &));
};

class ApplyDamageTest : public Test
{
public:
	EventBus eventBus;
	ApplyDamageListenerMock listener;
	StrictMock<EnvironmentMock> environmentMock;

	std::shared_ptr<StrictMock<UnitMock>> targetMock;
protected:
	void SetUp() override
	{
		targetMock = std::make_shared<StrictMock<UnitMock>>();
	}
};

//this should be the only subscription test for events, just in case cross-binary subscription breaks
TEST_F(ApplyDamageTest, Subscription)
{
	auto subscription1 = eventBus.subscribeBefore<ApplyDamage>(std::bind(&ApplyDamageListenerMock::beforeEvent, &listener, _1));
	auto subscription2 = eventBus.subscribeAfter<ApplyDamage>(std::bind(&ApplyDamageListenerMock::afterEvent, &listener, _1));

	EXPECT_CALL(listener, beforeEvent(_)).Times(1);
	EXPECT_CALL(listener, afterEvent(_)).Times(1);

	BattleStackAttacked pack;

	CApplyDamage event(&environmentMock, &pack, targetMock);

	eventBus.executeEvent(event);
}

}
