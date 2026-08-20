/*
 * CMapCacheTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "../../lib/json/JsonNode.h"
#include "../../lib/mapping/CMapHeader.h"
#include "../../lib/mapping/CMapInfo.h"
#include "../../lib/serializer/CBinaryCache.h"

namespace
{
	MapListEntry roundTripEntry(const MapListEntry & original)
	{
		const std::string fileURI = "test/TerrainViewTest.h3m";

		CBinaryCacheWriter writer(BinaryCache::MAP_MAGIC);
		writer.getSerializer() & static_cast<uint32_t>(1);
		writer.getSerializer() & fileURI;
		writer.getSerializer() & original;

		const auto & buffer = writer.getBuffer();
		CBinaryCacheReader reader(buffer.data(), buffer.size(), BinaryCache::MAP_MAGIC);
		auto & deserializer = reader.getDeserializer();

		uint32_t count = 0;
		deserializer & count;
		EXPECT_EQ(count, 1);

		std::string cachedFileURI;
		deserializer & cachedFileURI;

		CMapInfo cached;
		cached.initFromCache(cachedFileURI, deserializer);

		EXPECT_NE(cached.mapEntry, nullptr);
		return *cached.mapEntry;
	}
}

TEST(MapCache, lobbyEntryFieldsRoundTrip)
{
	MapListEntry original;
	original.version = EMapFormat::HOTA;
	original.width = 108;
	original.height = 72;
	original.battleOnly = true;
	original.victoryIconIndex = 7;
	original.defeatIconIndex = 2;
	original.amountOfPlayersOnMap = 4;
	original.amountOfHumanControllablePlayers = 2;

	auto entry = roundTripEntry(original);

	EXPECT_EQ(entry.version, original.version);
	EXPECT_EQ(entry.width, original.width);
	EXPECT_EQ(entry.height, original.height);
	EXPECT_EQ(entry.battleOnly, original.battleOnly);
	EXPECT_EQ(entry.victoryIconIndex, original.victoryIconIndex);
	EXPECT_EQ(entry.defeatIconIndex, original.defeatIconIndex);
	EXPECT_EQ(entry.amountOfPlayersOnMap, original.amountOfPlayersOnMap);
	EXPECT_EQ(entry.amountOfHumanControllablePlayers, original.amountOfHumanControllablePlayers);
}

TEST(MapCache, lobbyEntryNameRoundTrip)
{
	MapListEntry original;
	original.name = MetaString::createFromTextID("map.testMap.header.name");

	auto entry = roundTripEntry(original);

	EXPECT_EQ(entry.name, original.name);
}
