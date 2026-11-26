/*
 * OneWayConnectionTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 */

#include "StdInc.h"

#include "../../lib/GameLibrary.h"
#include "../../lib/json/JsonNode.h"
#include "../../lib/mapObjectConstructors/CObjectClassesHandler.h"
#include "../../lib/modding/IdentifierResolver.h"
#include "../../lib/rmg/CRmgTemplate.h"
#include "../../lib/serializer/JsonDeserializer.h"
#include "../../lib/serializer/JsonSerializer.h"

namespace
{

rmg::ZoneConnection loadOneWayConnection()
{
	const std::string text = R"({ "a" : "1", "b" : "2", "type" : "oneWay", "guard" : 5000 })";
	JsonNode node(text.data(), text.size(), "OneWayConnectionTest");
	rmg::ZoneConnection connection;
	InstantIdentifierResolver resolver(*LIBRARY->identifiers());
	JsonDeserializer handler(nullptr, resolver, node);
	connection.serializeJson(handler);
	return connection;
}

}

TEST(OneWayConnectionTest, DeserializeOneWay)
{
	const auto connection = loadOneWayConnection();
	EXPECT_EQ(connection.getConnectionType(), rmg::EConnectionType::ONE_WAY);
	EXPECT_EQ(connection.getZoneA(), 1);
	EXPECT_EQ(connection.getZoneB(), 2);
	EXPECT_EQ(connection.getGuardStrength(), 5000);
	EXPECT_FALSE(connection.needsPassage());
}

TEST(OneWayConnectionTest, SerializeOneWay)
{
	auto connection = loadOneWayConnection();
	JsonNode out;
	JsonSerializer handler(nullptr, out);
	connection.serializeJson(handler);
	EXPECT_EQ(out["type"].String(), "oneWay");
}

TEST(OneWayConnectionTest, OneWayMonolithSubtypesCloned)
{
	const auto entrance = LIBRARY->objtypeh->knownSubObjects(Obj::MONOLITH_ONE_WAY_ENTRANCE);
	const auto exit = LIBRARY->objtypeh->knownSubObjects(Obj::MONOLITH_ONE_WAY_EXIT);
	EXPECT_GE(entrance.size(), 100u);
	EXPECT_EQ(entrance, exit);
}
