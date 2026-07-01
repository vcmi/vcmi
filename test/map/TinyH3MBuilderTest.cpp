/*
 * TinyH3MBuilderTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"

#include "../mock/TinyH3MBuilder.h"

#include "../../lib/callback/EditorCallback.h"
#include "../../lib/entities/artifact/CArtifactInstance.h"
#include "../../lib/filesystem/CMemoryBuffer.h"
#include "../../lib/mapObjects/CGHeroInstance.h"
#include "../../lib/mapObjects/CQuest.h"
#include "../../lib/mapObjects/MiscObjects.h"
#include "../../lib/mapping/CMap.h"
#include "../../lib/mapping/CMapHeader.h"
#include "../../lib/mapping/MapFormat.h"
#include "../../lib/mapping/MapFormatH3M.h"

namespace
{
auto loadHeader(std::vector<uint8_t> bytes)
{
	CMemoryBuffer buf;
	buf.write(bytes.data(), static_cast<si64>(bytes.size()));
	buf.seek(0);

	// modName "core" so readLocalizedString -> mapRegisterLocalizedString can resolve mod language
	CMapLoaderH3M loader("TinyH3MBuilderTest", "core", "ASCII", &buf);
	return loader.loadMapHeader();
}

// Returned aggregate keeps the backing buffer and EditorCallback alive for the lifetime of the CMap.
struct LoadedMap
{
	std::unique_ptr<CMap>           map;
	std::unique_ptr<CMemoryBuffer>  stream;
	std::unique_ptr<EditorCallback> cb;
};

LoadedMap loadMap(std::vector<uint8_t> bytes)
{
	LoadedMap r;
	r.stream = std::make_unique<CMemoryBuffer>();
	r.stream->write(bytes.data(), static_cast<si64>(bytes.size()));
	r.stream->seek(0);
	// Same callback the mapeditor uses — provides the non-null IGameInfoCallback that
	// per-type object handlers (Keymaster / Border Guard / ...) assert on in their factory.
	r.cb = std::make_unique<EditorCallback>(nullptr);

	CMapLoaderH3M loader("TinyH3MBuilderTest", "core", "ASCII", r.stream.get());
	r.map = loader.loadMap(r.cb.get());
	r.cb->setMap(r.map.get());
	return r;
}
}

TEST(TinyH3MBuilderTest, TwoLevelMapHasTwoLayers)
{
	auto bytes = TinyH3M::TinyH3MBuilder(EMapFormat::ROE)
		.size(36, /*twoLevel*/ true)
		.name("TwoLevelROE")
		.buildAndDump("TwoLevelMapHasTwoLayers");

	auto header = loadHeader(std::move(bytes));
	ASSERT_NE(header, nullptr);
	EXPECT_EQ(header->mapLayers.size(), 2u);
}

TEST(TinyH3MBuilderTest, EmptyROEFullLoad)
{
	auto bytes = TinyH3M::TinyH3MBuilder(EMapFormat::ROE)
		.size(36, /*twoLevel*/ false)
		.name("EmptyROEFull")
		.difficulty(EMapDifficulty::NORMAL)
		.buildAndDump("EmptyROEFullLoad");

	auto loaded = loadMap(std::move(bytes));
	ASSERT_NE(loaded.map, nullptr);
	EXPECT_EQ(loaded.map->width, 36);
	EXPECT_EQ(loaded.map->height, 36);
	EXPECT_EQ(loaded.map->levels(), 1);
	EXPECT_TRUE(loaded.map->getHeroesOnMap().empty());
}

TEST(TinyH3MBuilderTest, EmptySODFullLoad)
{
	auto bytes = TinyH3M::TinyH3MBuilder(EMapFormat::SOD)
		.size(36, /*twoLevel*/ false)
		.name("EmptySODFull")
		.difficulty(EMapDifficulty::NORMAL)
		.buildAndDump("EmptySODFullLoad");

	auto loaded = loadMap(std::move(bytes));
	ASSERT_NE(loaded.map, nullptr);
	EXPECT_EQ(loaded.map->width, 36);
	EXPECT_EQ(loaded.map->height, 36);
	EXPECT_EQ(loaded.map->levels(), 1);
	EXPECT_TRUE(loaded.map->getHeroesOnMap().empty());
}

TEST(TinyH3MBuilderTest, AllSupportedObjectsLoad)
{
	// One instance of every object kind the builder currently emits. Acceptance
	// bar is "CMapLoaderH3M::loadMap returns a non-null CMap" — per-type body
	// assertions live in dedicated tests.
	auto bytes = TinyH3M::TinyH3MBuilder(EMapFormat::SOD)
		.size(36, /*twoLevel*/ false)
		.name("AllObjects")
		.playerActive(PlayerColor(0))
		.randomTown ({ 5,  5, 0}, PlayerColor(0))
		.monster    ({10, 10, 0}, CreatureID(27), /*count*/ 7) // Gold Dragon
		.resource   ({11, 10, 0}, GameResID(GameResID::GOLD), /*amount*/ 1000)
		.artifact   ({12, 10, 0}, ArtifactID(7))               // Centaur Axe — first artifact with a map sprite
		.keymaster  ({15, 15, 0}, /*color*/ 0)
		.borderGuard({16, 15, 0}, /*color*/ 0)
		.borderGate ({17, 15, 0}, /*color*/ 0)
		.questGuard ({20, 20, 0})
		.seerHut    ({22, 22, 0})
		.buildAndDump("AllSupportedObjectsLoad");

	auto loaded = loadMap(std::move(bytes));
	ASSERT_NE(loaded.map, nullptr);
	EXPECT_EQ(loaded.map->width, 36);
	// 1 town + 1 monster + 1 resource + 1 artifact + 3 border + 1 quest guard + 1 seer = 9
	size_t live = 0;
	for(const auto & obj : loaded.map->objects)
		if(obj) ++live;
	EXPECT_EQ(live, 9u);
}

namespace
{
template<class T>
const T * findFirst(const CMap & map)
{
	for(const auto & obj : map.objects)
	{
		if(const auto * casted = dynamic_cast<const T *>(obj.get()))
			return casted;
	}
	return nullptr;
}

template<class T>
std::vector<const T *> findAll(const CMap & map)
{
	std::vector<const T *> out;
	for(const auto & obj : map.objects)
	{
		if(const auto * casted = dynamic_cast<const T *>(obj.get()))
			out.push_back(casted);
	}
	return out;
}
}

TEST(TinyH3MBuilderTest, HeroesPlacement)
{
	// Fixed hero (Orrin, type 0) owned by red, plus a random hero owned by blue.
	auto bytes = TinyH3M::TinyH3MBuilder(EMapFormat::SOD)
		.size(36, /*twoLevel*/ false)
		.name("HeroesPlacement")
		.playerActive(PlayerColor(0))
		.playerActive(PlayerColor(1))
		.hero      ({5, 5, 0}, HeroTypeID(0), PlayerColor(0))
		.randomHero({6, 6, 0}, PlayerColor(1))
		.buildAndDump("HeroesPlacement");

	auto loaded = loadMap(std::move(bytes));
	ASSERT_NE(loaded.map, nullptr);

	const auto heroes = findAll<CGHeroInstance>(*loaded.map);
	ASSERT_EQ(heroes.size(), 2u);

	const CGHeroInstance * fixed = nullptr;
	const CGHeroInstance * random = nullptr;
	for(const auto * h : heroes)
	{
		if(h->getOwner() == PlayerColor(0))
			fixed = h;
		else if(h->getOwner() == PlayerColor(1))
			random = h;
	}

	ASSERT_NE(fixed, nullptr);
	ASSERT_NE(random, nullptr);
	EXPECT_EQ(fixed->getHeroTypeID(), HeroTypeID(0));
	EXPECT_EQ(fixed->anchorPos(), int3(5, 5, 0));
	EXPECT_EQ(random->anchorPos(), int3(6, 6, 0));
}

TEST(TinyH3MBuilderTest, SpellScrollLoads)
{
	// SpellID 15 = Magic Arrow (always available, no expansion required).
	const SpellID spell{15};

	auto bytes = TinyH3M::TinyH3MBuilder(EMapFormat::SOD)
		.size(36, /*twoLevel*/ false)
		.name("ScrollMagicArrow")
		.scroll({10, 10, 0}, spell)
		.buildAndDump("SpellScrollLoads");

	auto loaded = loadMap(std::move(bytes));
	ASSERT_NE(loaded.map, nullptr);

	const auto * scroll = findFirst<CGArtifact>(*loaded.map);
	ASSERT_NE(scroll, nullptr);
	EXPECT_EQ(scroll->anchorPos(), int3(10, 10, 0));

	const auto * inst = scroll->getArtifactInstance();
	ASSERT_NE(inst, nullptr);
	EXPECT_EQ(inst->getScrollSpellID(), spell);
}

TEST(TinyH3MBuilderTest, HeroCustomisation)
{
	// Hero with garrison, experience, primary skills, and a backpack artifact.
	auto bytes = TinyH3M::TinyH3MBuilder(EMapFormat::SOD)
		.size(36, /*twoLevel*/ false)
		.name("HeroCustom")
		.playerActive(PlayerColor(0))
		.hero({5, 5, 0}, HeroTypeID(0), PlayerColor(0))
		.heroGarrison({{CreatureID(0), 10}, {CreatureID(1), 5}}) // 10 pikemen + 5 archers
		.heroExperience(40000)                                    // ~level 5
		.heroPrimary(5, 3, 1, 2)
		.heroBackpack({ArtifactID(7)})                            // Centaur Axe in backpack
		.buildAndDump("HeroCustomisation");

	auto loaded = loadMap(std::move(bytes));
	ASSERT_NE(loaded.map, nullptr);

	const auto * hero = findFirst<CGHeroInstance>(*loaded.map);
	ASSERT_NE(hero, nullptr);
	EXPECT_EQ(hero->exp, 40000);
	// Garrison: 7 slots, first two populated.
	EXPECT_TRUE(hero->hasStackAtSlot(SlotID(0)));
	EXPECT_TRUE(hero->hasStackAtSlot(SlotID(1)));
	EXPECT_EQ(hero->getStackCount(SlotID(0)), 10);
	EXPECT_EQ(hero->getStackCount(SlotID(1)), 5);
}

TEST(TinyH3MBuilderTest, KillCreatureQuest)
{
	// Monster + quest guard that targets the monster's wire identifier.
	TinyH3M::TinyH3MBuilder b(EMapFormat::SOD);
	b.size(36).name("KillQuest")
		.playerActive(PlayerColor(0))
		.hero({5, 5, 0}, HeroTypeID(0), PlayerColor(0))
		.monster({10, 10, 0}, CreatureID(27), 3); // Gold Dragons
	const auto target = b.lastHandle();
	ASSERT_NE(target.wireIdentifier, 0u);

	b.questGuard({15, 15, 0}, TinyH3M::TinyH3MBuilder::missionKillCreature(target)
		.withLastDay(10)
		.withFirstVisitText("Slay the dragons"));

	auto loaded = loadMap(b.buildAndDump("KillCreatureQuest"));
	ASSERT_NE(loaded.map, nullptr);

	const auto * guard = findFirst<CGQuestGuard>(*loaded.map);
	ASSERT_NE(guard, nullptr);
	EXPECT_EQ(guard->getQuest().lastDay, 10);
	// Loader resolves the uint32 wire id to an ObjectInstanceID in afterRead;
	// the resolved target lives on quest.killTarget once mapping completes.
	EXPECT_TRUE(guard->getQuest().killTarget.hasValue());
}
