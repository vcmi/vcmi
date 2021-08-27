/*
 * ERM_MC.cpp, part of VCMI engine
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

class ERM_MC : public Test, public ScriptFixture
{
protected:
	void SetUp() override
	{
		ScriptFixture::setUp();
	}
};

TEST_F(ERM_MC, SetNameForVVar)
{
	std::stringstream source;
	source << "VERM" << std::endl;
	source << "!#MCv10:S@test@;" << std::endl;
	source << "!?PI;" << std::endl;
	source << "!!VR$test$:S42;" << std::endl;
	source << "!!VRv11:S$test$ +1;" << std::endl;

	const JsonNode actualState = runScript(VLC->scriptHandler->erm, source.str());

	SCOPED_TRACE("\n" + subject->code);

	const JsonNode & v = actualState["ERM"]["v"];
	EXPECT_EQ(v["10"], JsonUtils::floatNode(42)) << actualState.toJson(true);
	EXPECT_EQ(v["11"], JsonUtils::floatNode(43)) << actualState.toJson(true);
	const JsonNode & m = actualState["ERM"]["MDATA"]["test"];
	EXPECT_EQ(m["name"], JsonUtils::stringNode("v")) << actualState.toJson(true);
	EXPECT_EQ(m["index"], JsonUtils::stringNode("10")) << actualState.toJson(true);
}

TEST_F(ERM_MC, DeclareNewVariable)
{
	std::stringstream source;
	source << "VERM" << std::endl;
	source << "!#MC:S@test2@;" << std::endl;
	source << "!?PI;" << std::endl;
	source << "!!VR$test2$:S42;" << std::endl;
	source << "!!VRv11:S$test2$ +1;" << std::endl;

	const JsonNode actualState = runScript(VLC->scriptHandler->erm, source.str());

	SCOPED_TRACE("\n" + subject->code);

	const JsonNode & v = actualState["ERM"]["v"];
	EXPECT_EQ(v["11"], JsonUtils::floatNode(43)) << actualState.toJson(true);
	EXPECT_EQ(v["test2"], JsonUtils::floatNode(42)) << actualState.toJson(true);
	const JsonNode & m = actualState["ERM"]["MDATA"]["test2"];
	EXPECT_EQ(m["name"], JsonUtils::stringNode("v")) << actualState.toJson(true);
	EXPECT_EQ(m["index"], JsonUtils::stringNode("test2")) << actualState.toJson(true);
}

}
}


