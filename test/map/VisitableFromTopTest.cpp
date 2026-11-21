/*
 * VisitableFromTopTest.cpp, part of VCMI engine
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

static bool isVisitableFromTop(const std::string & objectName)
{
	return LIBRARY->objtypeh->getHandlerFor(*LIBRARY->identifiers(), "vcmi-test", objectName, "object").isVisitableFromTop();
}

TEST(VisitableFromTopTest, coreObjectTypes)
{
	EXPECT_TRUE(LIBRARY->objtypeh->getHandlerFor(Obj::MONSTER, 0).isVisitableFromTop());
	EXPECT_TRUE(LIBRARY->objtypeh->getHandlerFor(Obj::BORDER_GATE, 0).isVisitableFromTop());
	EXPECT_FALSE(LIBRARY->objtypeh->getHandlerFor(Obj::CREATURE_GENERATOR1, 0).isVisitableFromTop());
	EXPECT_FALSE(LIBRARY->objtypeh->getHandlerFor(Obj::PRISON, 0).isVisitableFromTop());
}

TEST(VisitableFromTopTest, derivedFromTemplatesOfOlderMods)
{
	EXPECT_TRUE(isVisitableFromTop("legacyVisitableFromTop"));
}

TEST(VisitableFromTopTest, explicitValueOverridesTemplates)
{
	EXPECT_FALSE(isVisitableFromTop("explicitlyNotVisitableFromTop"));
}
