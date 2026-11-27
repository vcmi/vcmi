/*
 * TreasurePlacerTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "../../lib/CRandomGenerator.h"
#include "../../lib/constants/EntityIdentifiers.h"
#include "../../lib/mapObjectConstructors/RandomMapInfo.h"
#include "../../lib/mapObjects/ObjectTemplate.h"
#include "../../lib/mapping/CMap.h"
#include "../../lib/rmg/CMapGenerator.h"
#include "../../lib/rmg/CMapGenOptions.h"
#include "../../lib/rmg/ObjectInfo.h"
#include "../../lib/rmg/RmgMap.h"
#include "../../lib/rmg/TileInfo.h"
#include "../../lib/rmg/Zone.h"
#include "../../lib/rmg/modificators/TreasurePlacer.h"

namespace test
{
using namespace ::testing;

class TreasurePlacerAccess : public TreasurePlacer
{
public:
	using TreasurePlacer::TreasurePlacer;
	using TreasurePlacer::getRandomObject;

	void insertUnchecked(const ObjectInfo & oi)
	{
		objects.addObject(oi);
	}

	void sortPool()
	{
		objects.sortPossibleObjects();
	}

	std::vector<ObjectInfo> & pool()
	{
		return objects.getPossibleObjects();
	}
};

class TreasurePlacerPoolTest : public Test
{
protected:
	CMapGenOptions options;
	std::unique_ptr<CMapGenerator> generator;
	std::unique_ptr<RmgMap> map;
	CRandomGenerator rng{12345};
	std::unique_ptr<Zone> zone;
	std::unique_ptr<TreasurePlacerAccess> placer;

	void SetUp() override
	{
		const auto templates = options.getPossibleTemplates();
		ASSERT_FALSE(templates.empty());
		options.setMapTemplate(templates.front());
		generator = std::make_unique<CMapGenerator>(options, nullptr, 12345);
		map = std::make_unique<RmgMap>(options, nullptr);
		zone = std::make_unique<Zone>(*map, *generator, rng);
		placer = std::make_unique<TreasurePlacerAccess>(*zone, *map, *generator);
	}

	static ObjectInfo makeCandidate(ui32 value, ui16 probability)
	{
		ObjectInfo oi(Obj::SCHOLAR, 0);
		auto appearance = std::make_shared<ObjectTemplate>();
		appearance->id = Obj::SCHOLAR;
		appearance->subid = 0;
		oi.templates.push_back(appearance);
		oi.generateObject = []() -> std::shared_ptr<CGObjectInstance> {
			return nullptr;
		};
		oi.value = value;
		oi.probability = probability;
		oi.maxPerZone = 1;
		return oi;
	}
};

TEST(TreasurePlacerTest, ApplyCommonObjectDefaultsFillsValueFromDefaults)
{
	/*
	 * Asserts: a commonObjects entry with only rarity keeps that rarity and takes value from the
	 * object type's rmg defaults.
	 * Pass: value 900 from defaults, probability 500 unchanged, returns true.
	 * Fail: the value stays 0, rarity is overwritten, or the call returns false.
	 */

	ObjectInfo object;
	object.value = 0;
	object.probability = 500;
	RandomMapInfo defaults;
	defaults.value = 900;
	defaults.rarity = 40;

	EXPECT_TRUE(TreasurePlacer::applyCommonObjectDefaults(object, defaults));
	EXPECT_EQ(object.value, 900u);
	EXPECT_EQ(object.probability, 500);
}

TEST(TreasurePlacerTest, ApplyCommonObjectDefaultsFillsRarityFromDefaults)
{
	/*
	 * Asserts: a commonObjects entry with only value keeps that value and takes rarity from the
	 * object type's rmg defaults.
	 * Pass: probability 40 from defaults, value 900 unchanged, returns true.
	 * Fail: the rarity stays 0, value is overwritten, or the call returns false.
	 */

	ObjectInfo object;
	object.value = 900;
	object.probability = 0;
	RandomMapInfo defaults;
	defaults.value = 1000;
	defaults.rarity = 40;

	EXPECT_TRUE(TreasurePlacer::applyCommonObjectDefaults(object, defaults));
	EXPECT_EQ(object.value, 900u);
	EXPECT_EQ(object.probability, 40);
}

TEST(TreasurePlacerTest, ApplyCommonObjectDefaultsKeepsBothWhenSet)
{
	/*
	 * Asserts: an entry that sets both value and rarity is left unchanged.
	 * Pass: value 900 and probability 50 stay as written, returns true.
	 * Fail: a field is replaced from the defaults.
	 */

	ObjectInfo object;
	object.value = 900;
	object.probability = 50;
	RandomMapInfo defaults;
	defaults.value = 1000;
	defaults.rarity = 40;

	EXPECT_TRUE(TreasurePlacer::applyCommonObjectDefaults(object, defaults));
	EXPECT_EQ(object.value, 900u);
	EXPECT_EQ(object.probability, 50);
}

TEST(TreasurePlacerTest, ApplyCommonObjectDefaultsRejectsMissingDefault)
{
	/*
	 * Asserts: an entry without a value is rejected when the object type also has no rmg value.
	 * Pass: returns false when defaults.value is 0 and the entry value is 0.
	 * Fail: the entry is accepted with a made-up value.
	 */

	ObjectInfo object;
	object.value = 0;
	object.probability = 50;
	RandomMapInfo defaults;
	defaults.value = 0;
	defaults.rarity = 40;

	EXPECT_FALSE(TreasurePlacer::applyCommonObjectDefaults(object, defaults));
}

TEST_F(TreasurePlacerPoolTest, GetRandomObjectReturnsNullWhenOnlyZeroProbabilityCandidate)
{
	/*
	 * Asserts: a pool whose only in-range candidate has probability 0 does not throw
	 * and yields no object.
	 * Pass: getRandomObject returns nullptr.
	 * Fail: CRandomGenerator::nextInt(1, 0) throws because total is 0.
	 */

	placer->insertUnchecked(makeCandidate(1000, 0));
	placer->sortPool();

	ObjectInfo * picked = nullptr;
	EXPECT_NO_THROW(picked = placer->getRandomObject(2000, 0, true, {}));
	EXPECT_EQ(picked, nullptr);
}

TEST_F(TreasurePlacerPoolTest, AddObjectToRandomPoolRejectsZeroProbability)
{
	/*
	 * Asserts: addObjectToRandomPool keeps probability-0 objects out of the pool,
	 * next to a non-zero entry that is stored.
	 * Pass: pool holds only the rarity-40 object.
	 * Fail: the zero-rarity object is stored and can be selected.
	 */

	placer->addObjectToRandomPool(makeCandidate(1000, 0));
	placer->addObjectToRandomPool(makeCandidate(1500, 40));

	ASSERT_EQ(placer->pool().size(), 1u);
	EXPECT_EQ(placer->pool().front().probability, 40);
	EXPECT_EQ(placer->pool().front().value, 1500u);
}

TEST_F(TreasurePlacerPoolTest, GetRandomObjectNeverReturnsZeroProbabilityAmongMixedCandidates)
{
	/*
	 * Asserts: mixed in-range candidates, some with probability 0, never yield those
	 * zero-probability objects.
	 * Pass: every draw over a fixed seed returns a candidate with probability > 0.
	 * Fail: a probability-0 object is picked, or nextInt throws when total is 0.
	 */

	placer->insertUnchecked(makeCandidate(1000, 0));
	placer->insertUnchecked(makeCandidate(1000, 80));
	placer->insertUnchecked(makeCandidate(1200, 20));
	placer->sortPool();

	for(int i = 0; i < 100; ++i)
	{
		ObjectInfo * picked = nullptr;
		EXPECT_NO_THROW(picked = placer->getRandomObject(2000, 0, true, {}));
		ASSERT_NE(picked, nullptr);
		EXPECT_NE(picked->probability, 0);
	}
}

}
