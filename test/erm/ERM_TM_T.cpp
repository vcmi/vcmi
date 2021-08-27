/*
 * ERM_TM_T.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include <vcmi/events/GenericEvents.h>

#include "../scripting/ScriptFixture.h"


namespace test
{
namespace scripting
{

using namespace ::testing;

class ERM_TM_T : public Test, public ScriptFixture
{
public:
	int day = 1;
protected:
	void SetUp() override
	{
		ScriptFixture::setUp();
	}
};

TEST_F(ERM_TM_T, NewGame)
{
	loadScript(VLC->scriptHandler->erm,
	{
		"VERM",
		"!#TM5:S2/10/3/2;",
		"!#TM6:S2/20/3/3;",
		"!?TM5;",
		"!!VRv42: +1;",
		"!?TM6;",
		"!!VRv43: +1;"
	});
	SCOPED_TRACE("\n" + subject->code);
	runClientServer();

	day = 1;

	auto getDate = [&](Date::EDateType mode)
	{
		return day;
	};

	EXPECT_CALL(infoMock, getDate(Eq(Date::DAY))).WillRepeatedly(Invoke(getDate));

	auto onGetTurn = [](events::PlayerGotTurn & event)
	{
	};

	for(; day < 25; day++)
	{
		events::TurnStarted::defaultExecute(&eventBus);

		for(auto playerId = 0; playerId < 5; playerId++)
		{
			PlayerColor p(playerId);
			::events::PlayerGotTurn::defaultExecute(&eventBus, onGetTurn, p);
		}

	}

	const JsonNode actualState = context->saveState();

	EXPECT_EQ(actualState["ERM"]["v"]["42"], JsonUtils::floatNode(3)) << actualState.toJson();
	EXPECT_EQ(actualState["ERM"]["v"]["43"], JsonUtils::floatNode(14)) << actualState.toJson();
}

}
}
