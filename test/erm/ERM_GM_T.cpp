/*
 * ERM_GM_T.cpp, part of VCMI engine
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

class ERM_GM_T : public Test, public ScriptFixture
{
public:

protected:
	void SetUp() override
	{
		ScriptFixture::setUp();
	}
};

TEST_F(ERM_GM_T, NewGame)
{
	std::stringstream source;
	source << "VERM" << std::endl;
	source << "!?GM0;" << std::endl;
	source << "!!VRv42:S4;" << std::endl;
	source << "!?PI;" << std::endl;
	source << "!!VRv43:S4;" << std::endl;

	loadScript(VLC->scriptHandler->erm, source.str());
	SCOPED_TRACE("\n" + subject->code);
	runClientServer();

	events::GameResumed::defaultExecute(&eventBus);

	const JsonNode actualState = context->saveState();

	EXPECT_EQ(actualState["ERM"]["v"]["42"], JsonUtils::floatNode(4)) << actualState.toJson();
}

}
}
