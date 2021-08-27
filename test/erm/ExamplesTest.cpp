/*
 * ExamplesTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "../scripting/ScriptFixture.h"
#include "../../lib/VCMI_Lib.h"
#include "../../lib/ScriptHandler.h"
#include "../../lib/NetPacks.h"
#include "../../lib/serializer/Cast.h"
#include "../../lib/VCMIDirs.h"
#include "../../lib/filesystem/Filesystem.h"
#include "../../lib/filesystem/FileInfo.h"


///All unsorted ERM tests goes here

namespace test
{

using namespace ::testing;
using namespace ::scripting;

class ExamplesTest : public Test, public ScriptFixture
{
public:
	std::vector<std::string> actualMessages;

	ExamplesTest()
		: ScriptFixture()
	{
	}

	void setDefaultExpectaions()
	{
		EXPECT_CALL(infoMock, getLocalPlayer()).WillRepeatedly(Return(PlayerColor(3)));
		EXPECT_CALL(serverMock, apply(Matcher<CPackForClient *>(_))).WillRepeatedly(Invoke(this, &ExamplesTest::onCommit));
	}

	void onCommit(CPackForClient * pack)
	{
		InfoWindow * iw = dynamic_ptr_cast<InfoWindow>(pack);

		if(iw)
		{
			actualMessages.push_back(iw->text.toString());
			EXPECT_EQ(iw->player, PlayerColor(3));
		}
		else
		{
			GTEST_FAIL() << "Invalid NetPack";
		}

	}

	void saveScript(const std::string & name)
	{
#ifdef VCMI_DUMP_TEST_SCRIPTS

	auto path = VCMIDirs::get().userDataPath() / name;

	boost::filesystem::ofstream tmp(path, boost::filesystem::ofstream::trunc);

	tmp.write(subject->code.c_str(), subject->code.size());
	tmp.flush();
	tmp.close();

#endif
	}

protected:
	void SetUp() override
	{
		ScriptFixture::setUp();
	}
};

TEST_F(ExamplesTest, ALL)
{
	setDefaultExpectaions();
	auto sources = CResourceHandler::get()->getFilteredFiles([](const ResourceID & ident)
	{
		return ident.getType() == EResType::ERM && boost::algorithm::starts_with(ident.getName(), "SCRIPTS/TEST/ERM/");
	});

	for(const ResourceID & file : sources)
	{
		actualMessages.clear();
		boost::filesystem::path scriptPath(file.getName() + ".VERM");
		boost::filesystem::path baseName = scriptPath.stem();
		boost::filesystem::path basePath = boost::filesystem::path("TEST/ERM/") / scriptPath.stem();

		std::string dataName = basePath.string()+".JSON";
		std::string scriptName = basePath.string()+".VERM";

		ResourceID dataPath(dataName);

		if(!CResourceHandler::get()->existsResource(dataPath))
		{
			GTEST_FAIL() << dataName << " does not exists";
			return;
		}

		const JsonNode expectedState(dataPath);

		loadScriptFromFile(scriptName);

		saveScript(baseName.string()+".lua");

		const JsonNode actualState = runServer();

		SCOPED_TRACE("\n"+actualState.toJson(true));

		JsonComparer c(false);
		c.compare(baseName.string(), actualState["ERM"], expectedState["ERM"]);

		auto expectedMessages = expectedState["messages"].convertTo<std::vector<std::string>>();

		EXPECT_THAT(actualMessages, Eq(expectedMessages));
	}
}

}
