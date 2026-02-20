/*
 * ERM_OB_T.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include <vcmi/events/AdventureEvents.h>

#include "../scripting/ScriptFixture.h"

namespace test
{
namespace scripting
{

using namespace ::testing;

class ObjectMock : public CGObjectInstance
{
public:
	MOCK_CONST_METHOD0(getObjGroupIndex, int32_t());
	MOCK_CONST_METHOD0(getObjTypeIndex, int32_t());
};

class ERM_OB_T : public Test, public ScriptFixture
{
public:
	ObjectMock objectMock;
protected:
	void SetUp() override
	{
		ScriptFixture::setUp();
	}
};

TEST_F(ERM_OB_T, ByTypeIndex)
{
	EXPECT_CALL(infoMock, getObj(Eq(ObjectInstanceID(234)), _)).Times(AtLeast(1)).WillRepeatedly(Return(&objectMock));
	EXPECT_CALL(objectMock, getObjGroupIndex()).Times(AtLeast(1)).WillRepeatedly(Return(420));

	loadScript(VLC->scriptHandler->erm,
		"VERM\n"
		"!?OB420;\n"
		"!!VRv42:S4;\n"
		"!?OB421;\n"
		"!!VRv43:S5;\n"
	);

	SCOPED_TRACE("\n" + subject->code);
	runClientServer();

	events::ObjectVisitStarted::defaultExecute(&eventBus, nullptr, PlayerColor(2), ObjectInstanceID(235), ObjectInstanceID(234));

	const JsonNode actualState = context->saveState();

	EXPECT_EQ(actualState["ERM"]["v"]["42"], JsonUtils::floatNode(4)) << actualState.toJson();
	EXPECT_TRUE(actualState["ERM"]["v"]["43"].isNull()) << actualState.toJson();
}

}
}
