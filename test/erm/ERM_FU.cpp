/*
 * ERM_FU.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "../scripting/ScriptFixture.h"


namespace test
{
namespace scripting
{

using namespace ::testing;

class ERM_FU : public Test, public ScriptFixture
{
protected:
	void SetUp() override
	{
		ScriptFixture::setUp();
	}
};

TEST_F(ERM_FU, P)
{
	std::stringstream source;
	source << "VERM" << std::endl;
	source << "!?PI;" << std::endl;
	source << "!!FU1:P;" << std::endl;
	source << "!?FU1;" << std::endl;
	source << "!!VRv1:S30;" << std::endl;

	JsonNode actualState = runScript(VLC->scriptHandler->erm, source.str());

	SCOPED_TRACE("\n" + subject->code);

	const JsonNode & v = actualState["ERM"]["v"];

	EXPECT_EQ(v["1"], JsonUtils::floatNode(30)) << actualState.toJson(true);
}


TEST_F(ERM_FU, E)
{
	std::stringstream source;
	source << "VERM" << std::endl;
	source << "!?PI;" << std::endl;
	source << "!!FU1:P33/?v1;" << std::endl;
	source << "!?FU1;" << std::endl;
	source << "!!VRx2:Sx1 +10;" << std::endl;
	source << "!!FU:E;" << std::endl;
	source << "!!VRx2:+1;" << std::endl;

	JsonNode actualState = runScript(VLC->scriptHandler->erm, source.str());

	SCOPED_TRACE("\n" + subject->code);

	const JsonNode & v = actualState["ERM"]["v"];

	EXPECT_EQ(v["1"], JsonUtils::floatNode(43)) << actualState.toJson(true);
}

}
}


