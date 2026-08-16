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
#include "../../lib/modding/ModVerificationInfo.h"
#include "../../lib/serializer/CBinaryCache.h"

namespace
{
	std::unique_ptr<CMapHeader> roundTripHeader(const CMapHeader & original)
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
		return std::move(cached.mapHeader);
	}
}

TEST(MapCache, playerSetupRoundTripPreservesLobbySemantics)
{
	CMapHeader original;
	original.players.resize(PlayerColor::PLAYER_LIMIT_I);

	original.players[0].canHumanPlay = true;
	original.players[0].canComputerPlay = false;
	original.players[0].allowedFactions = {FactionID(0)};
	original.players[0].isFactionRandom = true;
	original.players[0].hasRandomHero = true;
	original.players[0].aiTactic = EAiTactic::BUILDER;

	original.players[1].canHumanPlay = false;
	original.players[1].canComputerPlay = true;
	original.players[1].team = TeamID(2);

	auto header = roundTripHeader(original);

	ASSERT_NE(header, nullptr);
	ASSERT_EQ(header->players.size(), original.players.size());

	EXPECT_EQ(header->players[0].canHumanPlay, original.players[0].canHumanPlay);
	EXPECT_EQ(header->players[0].canComputerPlay, original.players[0].canComputerPlay);
	EXPECT_EQ(header->players[0].allowedFactions, original.players[0].allowedFactions);
	EXPECT_EQ(header->players[0].isFactionRandom, original.players[0].isFactionRandom);
	EXPECT_EQ(header->players[0].hasRandomHero, original.players[0].hasRandomHero);
	EXPECT_EQ(header->players[0].aiTactic, original.players[0].aiTactic);

	EXPECT_EQ(header->players[1].canHumanPlay, original.players[1].canHumanPlay);
	EXPECT_EQ(header->players[1].canComputerPlay, original.players[1].canComputerPlay);
	EXPECT_EQ(header->players[1].team, original.players[1].team);
}

TEST(MapCache, requiredModsRoundTripPreservesModIDs)
{
	CMapHeader original;
	ModVerificationInfo dependency;
	dependency.name = "Friendly display name";
	dependency.version = CModVersion::fromString("1.2.3");
	original.mods["publisher.actual-mod-id"] = dependency;

	auto header = roundTripHeader(original);

	ASSERT_NE(header, nullptr);

	EXPECT_TRUE(header->mods.contains("publisher.actual-mod-id"));
	EXPECT_FALSE(header->mods.contains("Friendly display name"));
	EXPECT_EQ(header->mods["publisher.actual-mod-id"].name, dependency.name);
	EXPECT_EQ(header->mods["publisher.actual-mod-id"].version.toString(), dependency.version.toString());
}

TEST(MapCache, localizedFieldsRoundTripPreserveTextIDs)
{
	CMapHeader original;
	original.name = MetaString::createFromTextID("map.testMap.header.name");
	original.description = MetaString::createFromRawString("Raw description");
	original.author = MetaString::createFromRawString("Fenrir");
	original.translations.Struct()["english"].Struct()["map.testMap.header.name"].String() = "Test Map";

	auto header = roundTripHeader(original);

	ASSERT_NE(header, nullptr);
	EXPECT_EQ(header->name, original.name);
	EXPECT_EQ(header->description, original.description);
	EXPECT_EQ(header->author, original.author);
	EXPECT_EQ(header->translations, original.translations);
}

TEST(MapCache, lobbyHeaderFieldsRoundTrip)
{
	CMapHeader original;
	original.mapLayers = {MapLayerId::SURFACE};
	original.battleOnly = true;
	original.victoryIconIndex = 7;
	original.defeatIconIndex = 2;
	original.victoryMessage = MetaString::createFromTextID("core.vcdesc.1");
	original.defeatMessage = MetaString::createFromTextID("core.lcdesc.1");
	original.allowedHeroes = {HeroTypeID(0), HeroTypeID(1)};

	DisposedHero disposed;
	disposed.heroId = HeroTypeID(2);
	disposed.portrait = HeroTypeID(2);
	disposed.name = "Restricted hero";
	disposed.players = {PlayerColor(0), PlayerColor(1)};
	original.disposedHeroes.push_back(disposed);

	auto header = roundTripHeader(original);

	ASSERT_NE(header, nullptr);
	EXPECT_EQ(header->mapLayers, original.mapLayers);
	EXPECT_EQ(header->battleOnly, original.battleOnly);
	EXPECT_EQ(header->victoryIconIndex, original.victoryIconIndex);
	EXPECT_EQ(header->defeatIconIndex, original.defeatIconIndex);
	EXPECT_EQ(header->victoryMessage, original.victoryMessage);
	EXPECT_EQ(header->defeatMessage, original.defeatMessage);
	EXPECT_EQ(header->allowedHeroes, original.allowedHeroes);
	ASSERT_EQ(header->disposedHeroes.size(), original.disposedHeroes.size());
	EXPECT_EQ(header->disposedHeroes[0].heroId, original.disposedHeroes[0].heroId);
	EXPECT_EQ(header->disposedHeroes[0].portrait, original.disposedHeroes[0].portrait);
	EXPECT_EQ(header->disposedHeroes[0].name, original.disposedHeroes[0].name);
	EXPECT_EQ(header->disposedHeroes[0].players, original.disposedHeroes[0].players);
}
