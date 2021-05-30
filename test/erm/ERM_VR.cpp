/*
 * ERM_VR.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "../scripting/ScriptFixture.h"
#include "../../../lib/CRandomGenerator.h"


namespace test
{
namespace scripting
{

using namespace ::testing;

class ERM_VR : public Test, public ScriptFixture
{
protected:
	void SetUp() override
	{
		ScriptFixture::setUp();
	}
};

TEST_F(ERM_VR, C)
{
	std::stringstream source;
	source << "VERM" << std::endl;
	source << "!?PI;" << std::endl;
	source << "!!VRv2:S99;" << std::endl;
	source << "!!VRv45:S100;" << std::endl;
	source << "!!VRv42:C3/5/75/?k/v2;" << std::endl;
	JsonNode actualState = runScript(VLC->scriptHandler->erm, source.str());

	SCOPED_TRACE("\n" + subject->code);

	EXPECT_EQ(actualState["ERM"]["Q"]["k"], JsonUtils::floatNode(100)) << actualState.toJson(true);

	const JsonNode & v = actualState["ERM"]["v"];

	EXPECT_EQ(v["42"], JsonUtils::floatNode(3)) << actualState.toJson(true);
	EXPECT_EQ(v["43"], JsonUtils::floatNode(5)) << actualState.toJson(true);
	EXPECT_EQ(v["44"], JsonUtils::floatNode(75)) << actualState.toJson(true);
	EXPECT_EQ(v["46"], JsonUtils::floatNode(99)) << actualState.toJson(true);
}

TEST_F(ERM_VR, H)
{
	std::stringstream source;
	source << "VERM" << std::endl;
	source << "!?PI;" << std::endl;
	source << "!!VRz100:S^Test!^;" << std::endl;
	source << "!!VRz101:S^^;" << std::endl;
	source << "!!VRz102:S^ ^;" << std::endl;
	source << "!!VRz100:H200;" << std::endl;
	source << "!!VRz101:H201;" << std::endl;
	source << "!!VRz102:H202;" << std::endl;

	JsonNode actualState = runScript(VLC->scriptHandler->erm, source.str());

	SCOPED_TRACE("\n" + subject->code);

	const JsonNode & f = actualState["ERM"]["F"];

	EXPECT_EQ(f["200"], JsonUtils::boolNode(true)) << actualState.toJson(true);
	EXPECT_EQ(f["201"], JsonUtils::boolNode(false)) << actualState.toJson(true);
	EXPECT_EQ(f["202"], JsonUtils::boolNode(false)) << actualState.toJson(true);
}

TEST_F(ERM_VR, U)
{
	std::stringstream source;
	source << "VERM" << std::endl;
	source << "!?PI;" << std::endl;
	source << "!!VRz100:S^Test!^;" << std::endl;
	source << "!!VRz101:S^est^;" << std::endl;
	source << "!!VRz100:Uz101;" << std::endl;

	JsonNode actualState = runScript(VLC->scriptHandler->erm, source.str());

	SCOPED_TRACE("\n" + subject->code);

	const JsonNode & f = actualState["ERM"]["F"];

	EXPECT_EQ(f["1"], JsonUtils::boolNode(true)) << actualState.toJson(true);
}

class ERM_VR_RNG : public ERM_VR
{
protected:
	void doTest(char rngType)
	{
		std::stringstream source;
		source << "VERM" << std::endl;
		source << "!?PI;" << std::endl;
		source << "!!VRv1:S10 " << rngType << "20;" << std::endl;

		double average = 0;
		int testCount = 100;

		for(int i = 0; i < testCount; i++)
		{
			JsonNode actualState;

			if(rngType == 'R')
			{
				loadScript(VLC->scriptHandler->erm, source.str());
				runServer();
				actualState = context->saveState();
			}
			else
			{
				actualState = runScript(VLC->scriptHandler->erm, source.str());
			}

			SCOPED_TRACE("\n" + subject->code);

			const JsonNode & v = actualState["ERM"]["v"];

			EXPECT_TRUE(v["1"].isNumber()) << actualState.toJson(true);

			int rngValue = v["1"].Integer();

			average += rngValue;

			ASSERT_GE(rngValue, 10);
			ASSERT_LE(rngValue, 30);
		}

		average /= testCount;

		EXPECT_NEAR(average, 20, 3) << "rng median should be in the middle of range ";
	}
};

TEST_F(ERM_VR_RNG, T)
{
	doTest('T');
}

TEST_F(ERM_VR_RNG, R)
{
	CRandomGenerator rng;
	EXPECT_CALL(serverMock, getRNG()).WillRepeatedly(Return(&rng));

	doTest('R');
}

TEST_F(ERM_VR_RNG, R_SEEDED)
{
	int expectedRandomValue = 2;

	CRandomGenerator rng;
	rng.setSeed(0x7ade6321);
	EXPECT_CALL(serverMock, getRNG()).WillRepeatedly(Return(&rng));

	std::stringstream source;
	source << "VERM" << std::endl;
	source << "!?PI;" << std::endl;
	source << "!!VRv1:R20;" << std::endl;

	JsonNode actualState;

	loadScript(VLC->scriptHandler->erm, source.str());
	runServer();
	actualState = context->saveState();

	SCOPED_TRACE("\n" + subject->code);

	const JsonNode & v = actualState["ERM"]["v"];

	EXPECT_TRUE(v["1"].isNumber()) << actualState.toJson(true);

	int rngValue = v["1"].Integer();

	EXPECT_EQ(rngValue, expectedRandomValue);
}

}
}


