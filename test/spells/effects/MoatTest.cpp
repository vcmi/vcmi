/*
 * MoatTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "EffectFixture.h"

#include "../../../lib/battle/CObstacleInstance.h"
#include "../../../lib/json/JsonNode.h"
#include "../../../lib/networkPacks/PacksForClient.h"
#include "../../../lib/networkPacks/PacksForClientBattle.h"

#include "../../mock/TownFake.h"

namespace test
{
using namespace ::spells;
using namespace ::spells::effects;
using namespace ::testing;

namespace
{

JsonNode makeMoatHex(int hex)
{
	JsonNode node;
	node.Integer() = hex;
	return node;
}

JsonNode makeHexPatch(std::initializer_list<int> hexes)
{
	JsonNode patch;
	for(int h : hexes)
		patch.Vector().push_back(makeMoatHex(h));
	return patch;
}

}

class MoatApplyTest : public Test, public EffectFixture
{
public:
	const SpellID testSpellId = SpellID::CURSE;

	MoatApplyTest()
		: EffectFixture("core:moat")
	{
	}

	BattleObstaclesChanged capturedPack;

	void captureObstaclePack()
	{
		EXPECT_CALL(serverMock, apply(Matcher<BattleObstaclesChanged &>(_)))
			.Times(AnyNumber())
			.WillRepeatedly(Invoke([this](const BattleObstaclesChanged & pack)
			{
				for(const auto & change : pack.changes)
					capturedPack.changes.push_back(change);
			}));
	}

	void setDefaultApplyExpectations()
	{
		EXPECT_CALL(mechanicsMock, getEffectPower()).WillRepeatedly(Return(5));
		EXPECT_CALL(mechanicsMock, getEffectLevel()).WillRepeatedly(Return(2));
		EXPECT_CALL(mechanicsMock, getSpell()).WillRepeatedly(Return(&spellStub));
		EXPECT_CALL(spellStub, getId()).WillRepeatedly(Return(testSpellId));
		EXPECT_CALL(spellStub, getJsonKey()).WillRepeatedly(Return(SpellID::encode(testSpellId.getNum())));
		EXPECT_CALL(mechanicsMock, isMassive()).WillRepeatedly(Return(true));
	}

protected:
	void SetUp() override
	{
		EffectFixture::setUp();
		mechanicsMock.casterSide = BattleSide::DEFENDER;
	}
};

TEST_F(MoatApplyTest, PlacesMoatObstaclesWhenTownHasMoat)
{
	JsonNode config;
	config["dispellable"].Bool() = false;
	config["moatDamage"].Integer() = 70;
	config["moatHexes"].Vector().push_back(makeHexPatch({11, 28, 44}));

	setupEffect(config);

	TownFake fakeTown;
	fakeTown.withBuilding(BuildingID::FORT);
	fakeTown.withBuilding(BuildingID::CITADEL);

	EXPECT_CALL(*battleFake, getDefendedTown()).WillRepeatedly(Return(fakeTown.get()));
	EXPECT_CALL(*battleFake, getAllObstacles()).WillRepeatedly(Return(IBattleInfo::ObstacleCList()));
	setDefaultApplyExpectations();
	captureObstaclePack();

	subject->apply(&serverMock, &mechanicsMock, Target());

	ASSERT_EQ(capturedPack.changes.size(), 1u);
	EXPECT_EQ(capturedPack.changes[0].operation, BattleChanges::EOperation::ADD);
}

TEST_F(MoatApplyTest, PlacesDispellableMoatAsSpellCreatedObstacle)
{
	JsonNode config;
	config["dispellable"].Bool() = true;
	config["moatDamage"].Integer() = 150;
	config["moatHexes"].Vector().push_back(makeHexPatch({11}));

	setupEffect(config);

	TownFake fakeTown;
	fakeTown.withBuilding(BuildingID::FORT);
	fakeTown.withBuilding(BuildingID::CITADEL);

	EXPECT_CALL(*battleFake, getDefendedTown()).WillRepeatedly(Return(fakeTown.get()));
	EXPECT_CALL(*battleFake, getAllObstacles()).WillRepeatedly(Return(IBattleInfo::ObstacleCList()));
	setDefaultApplyExpectations();
	captureObstaclePack();

	subject->apply(&serverMock, &mechanicsMock, Target());

	ASSERT_EQ(capturedPack.changes.size(), 1u);
	EXPECT_EQ(capturedPack.changes[0].operation, BattleChanges::EOperation::ADD);
}

TEST_F(MoatApplyTest, MultipleMoatPatchesCreateMultipleObstacles)
{
	JsonNode config;
	config["dispellable"].Bool() = false;
	config["moatDamage"].Integer() = 70;
	config["moatHexes"].Vector().push_back(makeHexPatch({11}));
	config["moatHexes"].Vector().push_back(makeHexPatch({28}));
	config["moatHexes"].Vector().push_back(makeHexPatch({44}));

	setupEffect(config);

	TownFake fakeTown;
	fakeTown.withBuilding(BuildingID::FORT);
	fakeTown.withBuilding(BuildingID::CITADEL);

	EXPECT_CALL(*battleFake, getDefendedTown()).WillRepeatedly(Return(fakeTown.get()));
	EXPECT_CALL(*battleFake, getAllObstacles()).WillRepeatedly(Return(IBattleInfo::ObstacleCList()));
	setDefaultApplyExpectations();
	captureObstaclePack();

	subject->apply(&serverMock, &mechanicsMock, Target());

	ASSERT_EQ(capturedPack.changes.size(), 3u);
	for(const auto & change : capturedPack.changes)
		EXPECT_EQ(change.operation, BattleChanges::EOperation::ADD);
}

TEST_F(MoatApplyTest, NoApplyWhenTownLacksMoat)
{
	JsonNode config;
	config["dispellable"].Bool() = false;
	config["moatDamage"].Integer() = 70;
	config["moatHexes"].Vector().push_back(makeHexPatch({11, 28, 44}));

	setupEffect(config);

	TownFake fakeTown;  // no CITADEL ⇒ no moat

	EXPECT_CALL(*battleFake, getDefendedTown()).WillRepeatedly(Return(fakeTown.get()));
	EXPECT_CALL(mechanicsMock, isMassive()).WillRepeatedly(Return(true));
	EXPECT_CALL(serverMock, apply(Matcher<BattleObstaclesChanged &>(_))).Times(0);

	subject->apply(&serverMock, &mechanicsMock, Target());
}

TEST_F(MoatApplyTest, AppliesBonuses)
{
	JsonNode config;
	config["dispellable"].Bool() = false;
	config["moatDamage"].Integer() = 70;
	config["moatHexes"].Vector().push_back(makeHexPatch({11, 28, 44}));

	JsonNode bonusNode;
	bonusNode["val"].Integer() = -3;
	bonusNode["type"].String() = "PRIMARY_SKILL";
	bonusNode["subtype"].String() = "primarySkill.defence";
	bonusNode["valueType"].String() = "ADDITIVE_VALUE";
	config["bonus"]["defenceDebuff"] = bonusNode;

	setupEffect(config);

	TownFake fakeTown;
	fakeTown.withBuilding(BuildingID::FORT);
	fakeTown.withBuilding(BuildingID::CITADEL);

	EXPECT_CALL(*battleFake, getDefendedTown()).WillRepeatedly(Return(fakeTown.get()));
	EXPECT_CALL(*battleFake, getAllObstacles()).WillRepeatedly(Return(IBattleInfo::ObstacleCList()));
	setDefaultApplyExpectations();
	captureObstaclePack();

	GiveBonus capturedBonus;
	bool bonusCaptured = false;
	EXPECT_CALL(serverMock, apply(Matcher<CPackForClient &>(_)))
		.WillOnce(Invoke([&](CPackForClient & p)
		{
			auto * gb = dynamic_cast<GiveBonus *>(&p);
			ASSERT_NE(gb, nullptr);
			capturedBonus = *gb;
			bonusCaptured = true;
		}));

	subject->apply(&serverMock, &mechanicsMock, Target());

	ASSERT_TRUE(bonusCaptured);
	EXPECT_EQ(capturedBonus.who, GiveBonus::ETarget::BATTLE);
	EXPECT_EQ(capturedBonus.bonus.type, BonusType::PRIMARY_SKILL);
	EXPECT_EQ(capturedBonus.bonus.val, -3);
	EXPECT_EQ(capturedBonus.bonus.duration, BonusDuration::ONE_BATTLE);
	EXPECT_EQ(capturedBonus.bonus.source, BonusSource::SPELL_EFFECT);
}

}
