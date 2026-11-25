/*
 * StandaloneObjectRulesTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"

#include "../../lib/GameLibrary.h"
#include "../../lib/mapObjectConstructors/AObjectTypeHandler.h"
#include "../../lib/mapObjectConstructors/CObjectClassesHandler.h"
#include "../../lib/mapObjectConstructors/RandomMapInfo.h"
#include "../../lib/rmg/Functions.h"
#include "../../lib/rmg/ObjectInfo.h"

namespace
{
AObjectTypeHandler & handlerFor(const std::string & objectName)
{
	return LIBRARY->objtypeh->getHandlerFor(*LIBRARY->identifiers(), "vcmi-test", objectName, "object");
}

ObjectInfo infoFor(const std::string & objectName)
{
	const auto & handler = handlerFor(objectName);
	return ObjectInfo(handler.getIndex(), handler.getSubIndex());
}
}

TEST(StandaloneObjectRulesTest, parsesStandaloneFlag)
{
	EXPECT_TRUE(handlerFor("standaloneObject").getRMGInfo().standalone);
	EXPECT_FALSE(handlerFor("pileObject").getRMGInfo().standalone);
}

TEST(StandaloneObjectRulesTest, canJoinTreasurePileRules)
{
	ObjectInfo standalone = infoFor("standaloneObject");
	ObjectInfo plain = infoFor("pileObject");
	std::vector<ObjectInfo *> empty;
	std::vector plainPile{&plain};
	std::vector standalonePile{&standalone};

	EXPECT_TRUE(canJoinTreasurePile(standalone, empty));
	EXPECT_FALSE(canJoinTreasurePile(standalone, plainPile));
	EXPECT_FALSE(canJoinTreasurePile(plain, standalonePile));
	EXPECT_TRUE(canJoinTreasurePile(plain, plainPile));
}
