/*
 * ERM_UN.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "../scripting/ScriptFixture.h"

#include "../../lib/NetPacks.h"
#include "../../lib/serializer/Cast.h"

#include "../mock/mock_CreatureService.h"
#include "../mock/mock_Creature.h"

namespace test
{
namespace scripting
{

using namespace ::testing;

class ERM_UN : public Test, public ScriptFixture
{
public:
	std::vector<EntityChanges> actualChanges;

	void onCommit(CPackForClient * pack)
	{
		EntitiesChanged * ec = dynamic_ptr_cast<EntitiesChanged>(pack);

		if(ec)
			onEntitiesChanged(ec);
		else
			GTEST_FAIL() << "Invalid NetPack";
	}

	void onEntitiesChanged(EntitiesChanged * pack)
	{
		vstd::concatenate(actualChanges, pack->changes);
	}

protected:
	void SetUp() override
	{
		ScriptFixture::setUp();
	}
};

class ERM_UN_G1 : public ERM_UN
{
public:
	CreatureServiceMock creatureService;
	CreatureMock oldCreature;
	const std::string NAME = "A monster";
	const std::string NAMEP = "Many monsters";

protected:
	void SetUp() override
	{
		ERM_UN::SetUp();

		EXPECT_CALL(servicesMock, creatures()).WillRepeatedly(Return(&creatureService));
		EXPECT_CALL(creatureService, getByIndex(Eq(68))).WillRepeatedly(Return(&oldCreature));
	}
};

TEST_F(ERM_UN_G1, get)
{
	EXPECT_CALL(oldCreature, getSingularName()).WillOnce(ReturnRef(NAME));
	EXPECT_CALL(oldCreature, getPluralName()).WillOnce(ReturnRef(NAMEP));
	loadScript(VLC->scriptHandler->erm,
	{
		"VERM",
		"!#UN:G1/68/0/?z1;",
		"!#UN:G1/68/1/?z2;"
	});

	SCOPED_TRACE("\n" + subject->code);
	const JsonNode actualState = runServer();
	EXPECT_EQ(actualState["ERM"]["z"]["1"].String(), NAME);
	EXPECT_EQ(actualState["ERM"]["z"]["2"].String(), NAMEP);
}

TEST_F(ERM_UN_G1, set)
{
	EXPECT_CALL(serverMock, apply(Matcher<CPackForClient *>(_))).Times(AtLeast(1)).WillRepeatedly(Invoke(this, &ERM_UN::onCommit));

	loadScript(VLC->scriptHandler->erm,
	{
		"VERM",
		"!#VRz1:S^A monster^;",
		"!#VRz2:S^Many monsters^;",
		"!#UN:G1/68/0/z1;",
		"!#UN:G1/68/1/2;"
	});

	SCOPED_TRACE("\n" + subject->code);
	const JsonNode actualState = runServer();

	JsonNode merged(JsonNode::JsonType::DATA_STRUCT);

	for(EntityChanges & change : actualChanges)
	{
		EXPECT_EQ(change.metatype, Metatype::CREATURE);
		EXPECT_EQ(change.entityIndex,68);
		JsonUtils::merge(merged, change.data);
	}

	JsonNode expectedMerged(JsonNode::JsonType::DATA_STRUCT);

	JsonNode & config = expectedMerged["config"];
	config["name"]["singular"].String() = NAME;
	config["name"]["plural"].String() = NAMEP;

	{
		JsonComparer c(false);
		c.compare("updateData", merged, expectedMerged);
	}
}


}
}
