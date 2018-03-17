/*
 * ERM_MF.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"

#include "../scripting/ScriptFixture.h"
#include "../JsonComparer.h"

#include  "../mock/mock_events_ApplyDamage.h"

namespace test
{
namespace scripting
{
using namespace ::testing;
using ::events::ApplyDamage;
using ::events::ApplyDamageMock;

class ERM_MF : public Test, public ScriptFixture
{
public:
	StrictMock<UnitMock> targetMock;
	ApplyDamageMock event;

	void setDefaultExpectations()
	{
		EXPECT_CALL(event, execute());
	}

protected:
	void SetUp() override
	{
		ScriptFixture::setUp();
	}
};

TEST_F(ERM_MF, ChangesDamage)
{
	setDefaultExpectations();
	std::stringstream source;
	source << "VERM" << std::endl;
	source << "!?MF1;" << std::endl;
	source << "!!MF:D?y-1;" << std::endl;
	source << "!!VRy-1:+10;" << std::endl;
	source << "!!MF:Fy-1;" << std::endl;

	loadScript(VLC->scriptHandler->erm, source.str());
	SCOPED_TRACE("\n" + subject->code);
	run();

	EXPECT_CALL(event, getInitalDamage()).WillOnce(Return(23450));
	EXPECT_CALL(event, setDamage(Eq(23460))).Times(1);

	eventBus.executeEvent(event);
}

TEST_F(ERM_MF, GetsUnitId)
{
	setDefaultExpectations();

	std::stringstream source;
	source << "VERM" << std::endl;
	source << "!?MF1;" << std::endl;
	source << "!!MF:N?v1;" << std::endl;

	loadScript(VLC->scriptHandler->erm, source.str());
	SCOPED_TRACE("\n" + subject->code);
	run();

	EXPECT_CALL(event, getTarget()).WillRepeatedly(Return(&targetMock));
	EXPECT_CALL(targetMock, unitId()).WillRepeatedly(Return(42));

	eventBus.executeEvent(event);

	JsonNode actualState = context->saveState();

	EXPECT_EQ(actualState["ERM"]["v"]["1"].Float(), 42);
}

//TODO:MF:E
//TODO:MF:W

}
}

