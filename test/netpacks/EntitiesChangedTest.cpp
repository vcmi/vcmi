/*
 * EntitiesChangedTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"

#include "NetPackFixture.h"

namespace test
{
using namespace ::testing;

class EntitiesChanged : public Test, public NetPackFixture
{
public:
	std::shared_ptr<::EntitiesChanged> subject;
protected:
	void SetUp() override
	{
		subject = std::make_shared<::EntitiesChanged>();
		NetPackFixture::setUp();
	}

};

TEST_F(EntitiesChanged, Apply)
{
	subject->changes.emplace_back();

	EntityChanges & changes = subject->changes.back();
	changes.metatype = Metatype::CREATURE;
	changes.entityIndex = 424242;
	changes.data.String() = "TEST";

	EXPECT_CALL(*gameState, updateEntity(Eq(Metatype::CREATURE), Eq(424242), _));

	gameState->apply(subject.get());
}

}
