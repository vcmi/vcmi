/*
 * CatapultTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "EffectFixture.h"

#include <vstd/RNG.h>

#include "../../../lib/mapObjects/CGTownInstance.h"
#include "../../../lib/json/JsonNode.h"
#include "../../mock/TownFake.h"


namespace test
{
using namespace ::spells;
using namespace ::spells::effects;
using namespace ::testing;

namespace
{
	/// Any random roll picks the middle of its range, which also selects the first of two equal candidates
	int midpointRng(int low, int high) { return (low + high) / 2; }

	/// Hexes that wall parts are aimed at, see wallParts in CBattleInfoCallback.cpp
	const std::map<EWallPart, BattleHex> WALL_HEXES = {
		{ EWallPart::KEEP,         BattleHex(50)  },
		{ EWallPart::BOTTOM_TOWER, BattleHex(183) },
		{ EWallPart::BOTTOM_WALL,  BattleHex(182) },
		{ EWallPart::BELOW_GATE,   BattleHex(130) },
		{ EWallPart::OVER_GATE,    BattleHex(78)  },
		{ EWallPart::UPPER_WALL,   BattleHex(29)  },
		{ EWallPart::UPPER_TOWER,  BattleHex(12)  },
		{ EWallPart::GATE,         BattleHex(96)  },
	};

	using WallStates = std::map<EWallPart, EWallState>;

	/// Wall states of a town with every fortification built, with specified parts overridden
	WallStates fortifiedTown(const WallStates & overrides = {})
	{
		WallStates states;
		for(const auto & [part, hex] : WALL_HEXES)
			states[part] = EWallState::INTACT;

		for(const auto & [part, state] : overrides)
			states[part] = state;

		return states;
	}
}

class CatapultTest : public Test, public EffectFixture
{
public:
	CatapultTest()
		:EffectFixture("core:catapult")
	{
	}

	void expectDefendedTown(const CGTownInstance * town)
	{
		EXPECT_CALL(*battleFake, getDefendedTown()).WillRepeatedly(Return(town));
		EXPECT_CALL(mechanicsMock, isSmart()).WillRepeatedly(Return(true));
	}

protected:
	void SetUp() override
	{
		EffectFixture::setUp();
		setupEffect(JsonNode());
	}
};

TEST_F(CatapultTest, NotApplicableWithoutTown)
{
	expectDefendedTown(nullptr);
	EXPECT_CALL(mechanicsMock, adaptProblem(_, _)).WillOnce(Return(false));

	EXPECT_FALSE(subject->applicableGeneral(problemMock, &mechanicsMock));
}

TEST_F(CatapultTest, NotApplicableInVillage)
{
	TownFake fakeTown;

	expectDefendedTown(fakeTown.get());
	EXPECT_CALL(mechanicsMock, adaptProblem(_, _)).WillOnce(Return(false));

	EXPECT_FALSE(subject->applicableGeneral(problemMock, &mechanicsMock));
}

TEST_F(CatapultTest, NotApplicableForDefenderIfSmart)
{
	TownFake fakeTown;
	fakeTown.withBuilding(BuildingID::FORT);
	mechanicsMock.casterSide = BattleSide::DEFENDER;

	expectDefendedTown(fakeTown.get());
	EXPECT_CALL(mechanicsMock, adaptProblem(_, _)).WillOnce(Return(false));

	EXPECT_FALSE(subject->applicableGeneral(problemMock, &mechanicsMock));
}

TEST_F(CatapultTest, ApplicableInTown)
{
	TownFake fakeTown;
	fakeTown.withBuilding(BuildingID::FORT);

	expectDefendedTown(fakeTown.get());
	EXPECT_CALL(mechanicsMock, adaptProblem(_, _)).Times(0);
	EXPECT_CALL(*battleFake, getWallState(_)).WillRepeatedly(Return(EWallState::INTACT));

	EXPECT_TRUE(subject->applicableGeneral(problemMock, &mechanicsMock));
}

class CatapultApplyTest : public Test, public EffectFixture
{
public:
	CatapultApplyTest()
		: EffectFixture("core:catapult")
	{
	}

	/// Sets up a catapult that always deals one damage per shot and never hits its intended target,
	/// so that every test only needs to configure what it actually cares about
	void setupCatapult(const JsonNode & config)
	{
		JsonNode effectConfig = config;
		if(effectConfig["targetsToAttack"].isNull())
			effectConfig["targetsToAttack"].Integer() = 1;
		if(effectConfig["chanceToNormalHit"].isNull())
			effectConfig["chanceToNormalHit"].Integer() = 100;

		EffectFixture::setupEffect(effectConfig);

		EXPECT_CALL(*battleFake, getDefendedTown()).WillRepeatedly(Return(fakeTown.get()));
		EXPECT_CALL(mechanicsMock, isSmart()).WillRepeatedly(Return(true));
		EXPECT_CALL(mechanicsMock, isMassive()).WillRepeatedly(Return(false));
		EXPECT_CALL(mechanicsMock, getEffectLevel()).WillRepeatedly(Return(0));
		setupDefaultRNG();
		EXPECT_CALL(rngMock, nextInt(Matcher<int>(_), Matcher<int>(_))).WillRepeatedly(Invoke(&midpointRng));

		caster = &unitsFake.add(BattleSide::ATTACKER);
		mechanicsMock.caster = caster;
		EXPECT_CALL(mechanicsMock, getUnitCaster()).WillRepeatedly(Return(caster));
		EXPECT_CALL(*caster, unitId()).WillRepeatedly(Return(static_cast<uint32_t>(-1)));

		EXPECT_CALL(serverMock, apply(Matcher<CatapultAttack &>(_)))
			.WillRepeatedly(Invoke([this](CatapultAttack & pack)
			{
				attacks.push_back(pack);
				BattleStatePackVisitor visitor(*battleFake);
				pack.visit(visitor);
			}));
	}

	/// Wall parts keep track of damage they receive, so that shots can be tested one after another
	void setupWalls(const WallStates & states)
	{
		wallStates = states;

		EXPECT_CALL(*battleFake, getWallState(_)).WillRepeatedly(Invoke([this](EWallPart part)
		{
			auto it = wallStates.find(part);
			return it == wallStates.end() ? EWallState::NONE : it->second;
		}));

		EXPECT_CALL(*battleFake, setWallState(_, _)).WillRepeatedly(Invoke([this](EWallPart part, EWallState state)
		{
			wallStates[part] = state;
		}));
	}

	void applyAimedAt(EWallPart part)
	{
		Target target;
		target.emplace_back(WALL_HEXES.at(part));
		subject->apply(&serverMock, &mechanicsMock, target);
	}

	/// Applies a shot that was not aimed by a hero, so catapult has to pick its own target
	void applyAutomatic()
	{
		Target target;
		target.emplace_back();
		subject->apply(&serverMock, &mechanicsMock, target);
	}

	std::vector<EWallPart> attackedParts() const
	{
		std::vector<EWallPart> result;
		for(const auto & attack : attacks)
			result.push_back(attack.attackedPart);
		return result;
	}

protected:
	void SetUp() override
	{
		EffectFixture::setUp();
		fakeTown.withBuilding(BuildingID::FORT);
	}

	TownFake fakeTown;
	battle::UnitFake * caster = nullptr;
	WallStates wallStates;
	std::vector<CatapultAttack> attacks;
};

TEST_F(CatapultApplyTest, AimedShotHitsChosenPart)
{
	JsonNode config;
	config["chanceToHitWall"].Integer() = 100;
	setupCatapult(config);
	setupWalls(fortifiedTown());

	applyAimedAt(EWallPart::BELOW_GATE);

	EXPECT_THAT(attackedParts(), ElementsAre(EWallPart::BELOW_GATE));
	EXPECT_EQ(attacks.at(0).damageDealt, 1u);
	EXPECT_EQ(attacks.at(0).killedTowerShooter, -1);
}

TEST_F(CatapultApplyTest, MissedShotNeverRedirectsToIntendedTarget)
{
	setupCatapult(JsonNode()); // no chance to hit anything - every shot misses
	setupWalls(fortifiedTown({
		{ EWallPart::UPPER_WALL, EWallState::DESTROYED },
		{ EWallPart::OVER_GATE,  EWallState::DESTROYED },
	}));

	// Intended target is the only standing wall segment besides the one next to it
	applyAimedAt(EWallPart::BELOW_GATE);

	EXPECT_THAT(attackedParts(), ElementsAre(EWallPart::BOTTOM_WALL));
}

TEST_F(CatapultApplyTest, MissedShotHitsIntendedTargetIfNoWallSegmentsLeft)
{
	setupCatapult(JsonNode());
	setupWalls(fortifiedTown({
		{ EWallPart::UPPER_WALL,  EWallState::DESTROYED },
		{ EWallPart::OVER_GATE,   EWallState::DESTROYED },
		{ EWallPart::BELOW_GATE,  EWallState::DESTROYED },
		{ EWallPart::BOTTOM_WALL, EWallState::DESTROYED },
	}));

	applyAimedAt(EWallPart::KEEP);

	EXPECT_THAT(attackedParts(), ElementsAre(EWallPart::KEEP));
}

TEST_F(CatapultApplyTest, SecondShotIsReaimedOnceTargetIsDestroyed)
{
	JsonNode config;
	config["targetsToAttack"].Integer() = 2;
	config["chanceToHitWall"].Integer() = 100;
	config["chanceToNormalHit"].Integer() = 0;
	config["chanceToCrit"].Integer() = 100; // two damage per shot destroys an intact segment
	setupCatapult(config);
	setupWalls(fortifiedTown({
		{ EWallPart::UPPER_WALL, EWallState::DESTROYED },
		{ EWallPart::OVER_GATE,  EWallState::DESTROYED },
	}));

	applyAimedAt(EWallPart::BELOW_GATE);

	// Destroying the target does not consume the second shot, which moves on to the nearest target left
	EXPECT_THAT(attackedParts(), ElementsAre(EWallPart::BELOW_GATE, EWallPart::BOTTOM_WALL));
}

TEST_F(CatapultApplyTest, AutomaticShotWithoutBallisticsHitsMostDamagedWall)
{
	JsonNode config;
	config["chanceToHitWall"].Integer() = 100;
	setupCatapult(config);
	setupWalls(fortifiedTown({{ EWallPart::OVER_GATE, EWallState::DAMAGED }}));

	applyAutomatic();

	EXPECT_THAT(attackedParts(), ElementsAre(EWallPart::OVER_GATE));
}

TEST_F(CatapultApplyTest, AutomaticShotWithoutBallisticsTurnsToGateOnlyAfterWallsFall)
{
	JsonNode config;
	config["chanceToHitGate"].Integer() = 100;
	setupCatapult(config);
	setupWalls(fortifiedTown({
		{ EWallPart::UPPER_WALL,  EWallState::DESTROYED },
		{ EWallPart::OVER_GATE,   EWallState::DESTROYED },
		{ EWallPart::BELOW_GATE,  EWallState::DESTROYED },
		{ EWallPart::BOTTOM_WALL, EWallState::DESTROYED },
	}));

	applyAutomatic();

	EXPECT_THAT(attackedParts(), ElementsAre(EWallPart::GATE));
}

TEST_F(CatapultApplyTest, AutomaticShotWithBallisticsHitsGateFirst)
{
	JsonNode config;
	config["chanceToHitGate"].Integer() = 100;
	setupCatapult(config);
	EXPECT_CALL(mechanicsMock, getEffectLevel()).WillRepeatedly(Return(1)); // hero has Ballistics
	setupWalls(fortifiedTown());

	applyAutomatic();

	EXPECT_THAT(attackedParts(), ElementsAre(EWallPart::GATE));
}

TEST_F(CatapultApplyTest, AutomaticShotWithBallisticsTurnsToKeepOnceWallIsDestroyed)
{
	JsonNode config;
	config["chanceToHitKeep"].Integer() = 100;
	setupCatapult(config);
	EXPECT_CALL(mechanicsMock, getEffectLevel()).WillRepeatedly(Return(1)); // hero has Ballistics
	setupWalls(fortifiedTown({
		{ EWallPart::GATE,       EWallState::DESTROYED },
		{ EWallPart::UPPER_WALL, EWallState::DESTROYED },
	}));

	applyAutomatic();

	EXPECT_THAT(attackedParts(), ElementsAre(EWallPart::KEEP));
}

TEST_F(CatapultApplyTest, RemovesTowerShooterOnKeepDestroyed)
{
	JsonNode config;
	config["chanceToHitKeep"].Integer() = 100;
	config["chanceToNormalHit"].Integer() = 0;
	config["chanceToCrit"].Integer() = 100; // two damage destroys a damaged keep
	setupCatapult(config);
	setupWalls(fortifiedTown({{ EWallPart::KEEP, EWallState::DAMAGED }}));

	auto & towerShooter = unitsFake.add(BattleSide::DEFENDER);
	const uint32_t shooterId = 99;
	EXPECT_CALL(towerShooter, getPosition()).WillRepeatedly(Return(BattleHex(BattleHex::CASTLE_CENTRAL_TOWER)));
	EXPECT_CALL(towerShooter, isGhost()).WillRepeatedly(Return(false));
	EXPECT_CALL(towerShooter, unitId()).WillRepeatedly(Return(shooterId));
	EXPECT_CALL(towerShooter, doubleWide()).WillRepeatedly(Return(false));
	EXPECT_CALL(*battleFake, removeUnit(Eq(shooterId))).Times(1);

	applyAimedAt(EWallPart::KEEP);

	EXPECT_THAT(attackedParts(), ElementsAre(EWallPart::KEEP));
	EXPECT_EQ(attacks.at(0).damageDealt, 2u);
	EXPECT_EQ(attacks.at(0).killedTowerShooter, static_cast<int32_t>(shooterId));
}

TEST_F(CatapultApplyTest, EarthquakeSpreadsAllOfItsDamageOverStandingParts)
{
	JsonNode config;
	config["targetsToAttack"].Integer() = 4;
	setupCatapult(config);
	EXPECT_CALL(mechanicsMock, isMassive()).WillRepeatedly(Return(true));
	setupWalls(fortifiedTown());

	applyAutomatic();

	uint32_t damageDealt = 0;
	for(const auto & attack : attacks)
		damageDealt += attack.damageDealt;

	EXPECT_EQ(damageDealt, 4u); // no damage is wasted on parts that are already about to fall

	const auto parts = attackedParts();
	const std::set<EWallPart> distinctParts(parts.begin(), parts.end());
	EXPECT_EQ(distinctParts.size(), attacks.size()); // each part is attacked by a single pack
}

TEST_F(CatapultApplyTest, EarthquakeIgnoresPartsThatTownHasNotBuilt)
{
	JsonNode config;
	config["targetsToAttack"].Integer() = 4;
	setupCatapult(config);
	EXPECT_CALL(mechanicsMock, isMassive()).WillRepeatedly(Return(true));

	// Town with a fort only - it has neither keep nor towers
	setupWalls({
		{ EWallPart::UPPER_WALL,  EWallState::INTACT },
		{ EWallPart::OVER_GATE,   EWallState::INTACT },
		{ EWallPart::BELOW_GATE,  EWallState::INTACT },
		{ EWallPart::BOTTOM_WALL, EWallState::INTACT },
		{ EWallPart::GATE,        EWallState::INTACT },
	});

	applyAutomatic();

	uint32_t damageDealt = 0;
	for(const auto & attack : attacks)
	{
		damageDealt += attack.damageDealt;
		EXPECT_THAT(attack.attackedPart, Not(AnyOf(EWallPart::KEEP, EWallPart::UPPER_TOWER, EWallPart::BOTTOM_TOWER)));
	}

	EXPECT_EQ(damageDealt, 4u);
}

struct RedirectCase
{
	EWallPart intended;
	EWallPart expected;
};

/// Checks which wall segment a missed shot lands on, in a town where every segment still stands
class CatapultRedirectTest : public CatapultApplyTest, public WithParamInterface<RedirectCase>
{
};

TEST_P(CatapultRedirectTest, MissedShotHitsNearestWallSegment)
{
	setupCatapult(JsonNode()); // no chance to hit anything - every shot misses
	setupWalls(fortifiedTown());

	applyAimedAt(GetParam().intended);

	EXPECT_THAT(attackedParts(), ElementsAre(GetParam().expected));
}

INSTANTIATE_TEST_SUITE_P
(
	ByIntendedTarget,
	CatapultRedirectTest,
	Values
	(
		RedirectCase{ EWallPart::UPPER_WALL,   EWallPart::OVER_GATE   },
		RedirectCase{ EWallPart::OVER_GATE,    EWallPart::UPPER_WALL  },
		RedirectCase{ EWallPart::BELOW_GATE,   EWallPart::BOTTOM_WALL },
		RedirectCase{ EWallPart::BOTTOM_WALL,  EWallPart::BELOW_GATE  },
		RedirectCase{ EWallPart::KEEP,         EWallPart::BOTTOM_WALL },
		RedirectCase{ EWallPart::UPPER_TOWER,  EWallPart::UPPER_WALL  },
		RedirectCase{ EWallPart::BOTTOM_TOWER, EWallPart::BOTTOM_WALL },
		// Both segments next to the gate are equally close, so one of them is picked at random
		RedirectCase{ EWallPart::GATE,         EWallPart::OVER_GATE   }
	)
);

}
