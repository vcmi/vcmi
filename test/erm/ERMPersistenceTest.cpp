/*
 * ERMPersistenceTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#if 0

#include "../scripting/ScriptFixture.h"

namespace test
{

using namespace ::testing;
using namespace ::scripting;

class ERMPersistenceTest : public Test, public ScriptFixture
{
public:

	ERMPersistenceTest()
		: ScriptFixture()
	{
	}

protected:
	void SetUp() override
	{
		ScriptFixture::setUp();
	}
};

TEST_F(ERMPersistenceTest, Empty)
{
	std::stringstream source;
	source << "VERM" << std::endl;
	source << "!#VRj:S2;" << std::endl;

	loadScript(VLC->scriptHandler->erm, source.str());
	JsonNode actualState = runClientServer();

	SCOPED_TRACE("\n" + subject->code);

	JsonNode state;
	state["ERM"]["Q"]["j"].Float() = 2;
	state["ERM"]["instructionsCalled"].Bool() = true;

	JsonComparer c(false);
	c.compare("state", actualState, state);
}

TEST_F(ERMPersistenceTest, QuickVar)
{
	const int VALUE = 5678;

	JsonNode state;
	state["foo"].String() = "foo";
	state["ERM"]["Q"]["m"].Integer() = VALUE;

	std::stringstream source;
	source << "VERM" << std::endl;
	source << "!#VRm:+2;" << std::endl;

	loadScript(VLC->scriptHandler->erm, source.str());
	JsonNode actualState = runClientServer(state);

	SCOPED_TRACE("\n" + subject->code);

	state["ERM"]["Q"]["m"].Float() = VALUE + 2;
	state["ERM"]["instructionsCalled"].Bool() = true;

	JsonComparer c(false);
	c.compare("state", actualState, state);
}

TEST_F(ERMPersistenceTest, RegularVar)
{
	const int VALUE = 4242;

	JsonNode state;
	state["ERM"]["v"]["57"].Integer() = VALUE;

	std::stringstream builder;
	builder << "VERM" << std::endl;
	builder << "!#VRv57:-28;" << std::endl;

	loadScript(VLC->scriptHandler->erm, builder.str());
	JsonNode actualState = runClientServer(state);

	SCOPED_TRACE("\n" + subject->code);

	state["ERM"]["v"]["57"].Float() = VALUE - 28;
	state["ERM"]["instructionsCalled"].Bool() = true;

	JsonComparer c(false);
	c.compare("state", actualState, state);
}

}
#endif
