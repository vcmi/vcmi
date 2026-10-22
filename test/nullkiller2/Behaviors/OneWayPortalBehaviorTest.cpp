/*
 * OneWayPortalBehaviorTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 */

#include "StdInc.h"

#include "AI/Nullkiller2/Behaviors/ExplorationBehavior.h"

#include "mock/TinyH3MBuilder.h"
#include "nullkiller2/NullkillerTest.h"
#include "nullkiller2/ScriptedMovementClient.h"

namespace
{
const PlayerColor PLAYER = PlayerColor(0);
const PlayerColor ENEMY = PlayerColor(1);
constexpr int PORTAL_CHANNEL = 0;

const int3 ENTRANCE(16, 10, 0);
const int3 ENTRANCE_GUARD(17, 10, 0);
const int3 FAR_EXIT(24, 24, 0);
/// Exit in the map corner, so that a monster on GUARDED_EXIT_GUARD covers every way out of it
const int3 GUARDED_EXIT(0, 0, 0);
const int3 GUARDED_EXIT_GUARD(1, 1, 0);

class Nullkiller2_Behaviors_OneWayPortal : public NullkillerTest
{
public:
	/// Heroes west of the entrance, the first one being the farthest from it
	static TinyH3M::TinyH3MBuilder makeOneWayPortalMap(
		const std::vector<uint16_t> & heroArmySizes,
		bool withTown = true,
		const std::vector<int3> & exits = {FAR_EXIT})
	{
		TinyH3M::TinyH3MBuilder builder(EMapFormat::SOD);
		builder
			.size(36, false)
			.name("NK2OneWayPortal")
			.playerActive(PLAYER);

		if(withTown)
			builder.randomTown({5, 5, 0}, PLAYER);

		for(size_t i = 0; i < heroArmySizes.size(); ++i)
		{
			builder
				.hero({7 + static_cast<int>(i) * 2, 10, 0}, HeroTypeID(static_cast<int>(i)), PLAYER)
				.heroGarrison({{CreatureID::ARCHER, heroArmySizes[i]}});
		}

		builder.oneWayPortalEntrance(ENTRANCE, PORTAL_CHANNEL);
		for(const auto & exit : exits)
			builder.oneWayPortalExit(exit, PORTAL_CHANNEL);

		return builder;
	}

	/// Portal map with `guardSize` archers trapping whoever lands on the corner exit
	static TinyH3M::TinyH3MBuilder makeGuardedExitMap(
		const std::vector<uint16_t> & heroArmySizes,
		uint16_t guardSize,
		const std::vector<int3> & exits = {GUARDED_EXIT})
	{
		auto builder = makeOneWayPortalMap(heroArmySizes, true, exits);
		builder.monster(GUARDED_EXIT_GUARD, CreatureID::ARCHER, guardSize);
		return builder;
	}

	/// Cuts the side of the far exit off from the home side
	static void addWaterWall(TinyH3M::TinyH3MBuilder & builder)
	{
		for(int y = 0; y < 36; ++y)
			builder.terrain({20, y, 0}, TerrainId::WATER);
	}

	/// Single land row through the entrance, so the only way east leads over it
	static TinyH3M::TinyH3MBuilder makeOneWayPortalBarrierMap()
	{
		TinyH3M::TinyH3MBuilder builder(EMapFormat::SOD);
		builder
			.size(36, false)
			.name("NK2OneWayPortalBarrier")
			.playerActive(PLAYER)
			.hero({5, 10, 0}, HeroTypeID(0), PLAYER)
			.heroGarrison({{CreatureID::ARCHER, 100}})
			.oneWayPortalEntrance(ENTRANCE, PORTAL_CHANNEL)
			.oneWayPortalExit(FAR_EXIT, PORTAL_CHANNEL);

		for(int y = 0; y < 36; ++y)
		{
			for(int x = 0; x < 36; ++x)
			{
				if(y != ENTRANCE.y)
					builder.terrain({x, y, 0}, TerrainId::WATER);
			}
		}

		return builder;
	}

	static TinyH3M::TinyH3MBuilder makeIsolatedEnemyPortalMap()
	{
		auto builder = makeOneWayPortalMap({500, 1});
		builder
			.playerActive(ENEMY)
			.randomTown({30, 24, 0}, ENEMY);
		addWaterWall(builder);
		return builder;
	}

	/// Main and scout with a treasure at home; the far side is cut off by water and holds a treasure guarded against scouts
	static TinyH3M::TinyH3MBuilder makeRewardsBehindPortalMap(bool withFarTown)
	{
		auto builder = makeOneWayPortalMap({500, 1});
		builder
			.resource({8, 12, 0}, GameResID::GOLD, 1000)
			.resource({28, 26, 0}, GameResID::GOLD, 1000)
			.monster({29, 26, 0}, CreatureID::ARCHER, 50);

		if(withFarTown)
			builder.randomTown({30, 30, 0}, PlayerColor::NEUTRAL);

		addWaterWall(builder);
		return builder;
	}

	void TearDown() override
	{
		gateway.reset();
		client.reset();
		NullkillerTest::TearDown();
	}

	/// Starts the map fully revealed with planning prepared; `exitPos` selects the exit that heroes are sent to
	void start(TinyH3M::TinyH3MBuilder builder, const int3 & exitPos = FAR_EXIT)
	{
		startWithMap(std::move(builder));
		revealMap(PLAYER);

		entrance = requireObjectAt(ENTRANCE);
		exit = requireObjectAt(exitPos);
		preparePlanning();
	}

	/// Starts the rewards map with the scout landed behind the portal and off its exit
	void startWithScoutBehindPortal(bool withFarTown)
	{
		start(makeRewardsBehindPortalMap(withFarTown));

		auto * landedScout = scout();
		sendThroughPortal(*landedScout);
		moveHero(*landedScout, exit->visitablePos() + int3(1, 0, 0));
		advanceDay();
	}

	CGObjectInstance * requireObjectAt(const int3 & pos) const
	{
		auto * object = findObjectAt(pos);
		if(!object)
			throw std::runtime_error("No object at " + pos.toString());
		return object;
	}

	std::vector<CGHeroInstance *> heroesByStrength() const
	{
		auto result = findAll<CGHeroInstance>();
		std::erase_if(result, [](const CGHeroInstance * hero) { return hero->getOwner() != PLAYER; });

		std::ranges::sort(result, [](const CGHeroInstance * left, const CGHeroInstance * right)
		{
			return left->estimateHeroCombatValue() > right->estimateHeroCombatValue();
		});
		return result;
	}

	CGHeroInstance * mainHero() const
	{
		return heroesByStrength().front();
	}

	CGHeroInstance * scout() const
	{
		return heroesByStrength().back();
	}

	void markMissionCritical(const CGHeroInstance & hero)
	{
		EventCondition condition(EventCondition::CONTROL);
		condition.objectID = hero.id;
		condition.objectType = Obj(Obj::HERO);

		EventExpression::OperatorNone noneOf;
		noneOf.expressions.emplace_back(condition);

		TriggeredEvent event;
		event.identifier = "protectHero";
		event.effect.type = EventEffect::DEFEAT;
		event.trigger = EventExpression(noneOf);
		map()->triggeredEvents.push_back(event);

		ASSERT_TRUE(hero.isMissionCritical());
	}

	void preparePlanning()
	{
		if(!gateway)
		{
			client = std::make_unique<ScriptedMovementClient>(gameState());
			gateway = makeGateway(PLAYER, client.get());
			client->notify(*gateway);
		}

		for(auto * hero : heroesByStrength())
			hero->setMovementPoints(2500);

		NullkillerTest::preparePlanning(*gateway);
		// the idle check reads the objects that capturing would offer
		gateway->nullkiller->dangerHitMap->updateHitMap();
		gateway->nullkiller->objectClusterizer->clusterize();
	}

	void restartGateway()
	{
		gateway.reset();
		client.reset();
		preparePlanning();
	}

	NK2AI::Goals::TGoalVec decomposePortalBehavior()
	{
		preparePlanning();

		NK2AI::Goals::TGoalVec result;
		gateway->nullkiller->decomposer->decompose(
			result,
			NK2AI::Goals::sptr(NK2AI::Goals::ExplorationBehavior()),
			5);
		return result;
	}

	/// Tasks that send a hero into the entrance
	NK2AI::Goals::TGoalVec portalTasks(const NK2AI::Goals::TGoalVec & goals) const
	{
		NK2AI::Goals::TGoalVec result;
		for(const auto & goal : goals)
		{
			if(goal->asTask()->isObjectAffected(entrance->id))
				result.push_back(goal);
		}
		return result;
	}

	void teleportHero(
		CGHeroInstance & hero,
		const CGObjectInstance & from,
		const CGObjectInstance & to)
	{
		TryMoveHero move;
		move.id = hero.id;
		move.result = TryMoveHero::TELEPORTATION;
		move.start = hero.convertFromVisitablePos(from.visitablePos());
		move.end = hero.convertFromVisitablePos(to.visitablePos());
		move.movePoints = hero.movementPointsRemaining();

		gameState()->apply(move);
		gateway->heroMoved(move, false);
		ASSERT_EQ(hero.visitablePos(), to.visitablePos());
	}

	void sendThroughPortal(CGHeroInstance & hero)
	{
		teleportHero(hero, *entrance, *exit);
	}

	void moveHero(CGHeroInstance & hero, const int3 & destination)
	{
		TryMoveHero move;
		move.id = hero.id;
		move.result = TryMoveHero::SUCCESS;
		move.start = hero.pos;
		move.end = hero.convertFromVisitablePos(destination);
		move.movePoints = hero.movementPointsRemaining();

		gameState()->apply(move);
		gateway->heroMoved(move, false);
		ASSERT_EQ(hero.visitablePos(), destination);
	}

	/// Removes the hero as if it was defeated
	void loseHero(const CGHeroInstance & hero)
	{
		gateway->objectRemoved(&hero, PLAYER);

		RemoveObject removal(hero.id, PLAYER);
		gameState()->apply(removal);
	}

	void finishTurn()
	{
		for(auto * hero : heroesByStrength())
			hero->setMovementPoints(0);

		gateway->makeTurn();
	}

	void returnHeroToTown(CGHeroInstance & hero)
	{
		// real movement leaves the exit tile long before the hero reaches a town
		moveHero(hero, hero.visitablePos() + int3(0, 1, 0));

		moveHero(hero, findFirst<CGTownInstance>()->visitablePos());
	}

	void advanceDay()
	{
		NewTurn newTurn;
		newTurn.day = gameState()->day + 1;
		gameState()->apply(newTurn);
	}

	std::unique_ptr<ScriptedMovementClient> client;
	std::unique_ptr<NK2AI::AIGateway> gateway;
	CGObjectInstance * entrance = nullptr;
	CGObjectInstance * exit = nullptr;
};
}

TEST_F(Nullkiller2_Behaviors_OneWayPortal, scoutIsPreferredOverNearerMain)
{
	start(makeOneWayPortalMap({1, 1, 100}));

	const auto * main = mainHero();
	ASSERT_LT(
		main->visitablePos().dist2d(entrance->visitablePos()),
		scout()->visitablePos().dist2d(entrance->visitablePos()));

	const auto probes = portalTasks(decomposePortalBehavior());

	ASSERT_EQ(probes.size(), 1);
	EXPECT_FALSE(probes.front()->asTask()->isObjectAffected(main->id))
		<< "the main must not be assigned while a scout is eligible";
}

TEST_F(Nullkiller2_Behaviors_OneWayPortal, entranceIsReachableButCannotBeUsedAsCorridor)
{
	start(makeOneWayPortalBarrierMap());

	const auto entrancePaths = gateway->nullkiller->pathfinder->getPathInfo(
		entrance->visitablePos(),
		false);
	EXPECT_FALSE(entrancePaths.empty())
		<< "explicit probe goals must still be able to target the entrance";

	const auto pathsPastEntrance = gateway->nullkiller->pathfinder->getPathInfo(
		entrance->visitablePos() + int3(2, 0, 0),
		false);
	EXPECT_TRUE(pathsPastEntrance.empty())
		<< "ordinary plans must not treat a one-way entrance as a walkable corridor";
}

TEST_F(Nullkiller2_Behaviors_OneWayPortal, exhaustedPortalRouteRetainsReservationAndLocksHero)
{
	start(makeOneWayPortalMap({100}));

	auto * hero = mainHero();
	const auto probes = portalTasks(decomposePortalBehavior());
	ASSERT_EQ(probes.size(), 1);

	client->drainMovement(0);
	EXPECT_NO_THROW(probes.front()->asTask()->accept(gateway.get()));
	EXPECT_NE(hero->visitablePos(), entrance->visitablePos());
	EXPECT_EQ(
		gateway->nullkiller->getHeroLockedReason(hero),
		NK2AI::HeroLockedReason::HERO_CHAIN)
		<< "a portal route cut short by movement points must resume on a later turn";
	EXPECT_EQ(
		gateway->nullkiller->memory->getOneWayPortalReservation(entrance->id),
		std::optional<ObjectInstanceID>(hero->id));
}

TEST_F(Nullkiller2_Behaviors_OneWayPortal, twoScoutsProduceOneTaskForTheNearestScout)
{
	start(makeOneWayPortalMap({200, 1, 1}));

	std::vector<CGHeroInstance *> scouts;
	for(auto * hero : heroesByStrength())
	{
		if(gateway->nullkiller->heroManager->getHeroRoleOrDefaultInefficient(hero) == NK2AI::HeroRole::SCOUT)
			scouts.push_back(hero);
	}
	ASSERT_EQ(scouts.size(), 2);

	const auto nearest = *std::ranges::min_element(scouts, [this](const auto * left, const auto * right)
	{
		return left->visitablePos().dist2d(entrance->visitablePos())
			< right->visitablePos().dist2d(entrance->visitablePos());
	});
	const auto probes = portalTasks(decomposePortalBehavior());

	ASSERT_EQ(probes.size(), 1) << "one portal must not schedule multiple scouts";
	EXPECT_TRUE(probes.front()->asTask()->isObjectAffected(nearest->id));
}

TEST_F(Nullkiller2_Behaviors_OneWayPortal, missionCriticalMainWithoutScoutIsProtected)
{
	start(makeOneWayPortalMap({500}));
	markMissionCritical(*mainHero());

	EXPECT_TRUE(portalTasks(decomposePortalBehavior()).empty());
}

TEST_F(Nullkiller2_Behaviors_OneWayPortal, soleNoTownMissionCriticalHeroCanUseOnlyProgress)
{
	start(makeOneWayPortalMap({100}, false));
	markMissionCritical(*mainHero());

	const auto probes = portalTasks(decomposePortalBehavior());

	ASSERT_EQ(probes.size(), 1);
	EXPECT_TRUE(probes.front()->asTask()->isObjectAffected(mainHero()->id));
}

TEST_F(Nullkiller2_Behaviors_OneWayPortal, mainClearsEntranceGuard)
{
	auto builder = makeOneWayPortalMap({500, 1});
	builder.monster(ENTRANCE_GUARD, CreatureID::ARCHER, 1);
	start(std::move(builder));

	const auto * guard = requireObjectAt(ENTRANCE_GUARD);
	const auto goals = decomposePortalBehavior();
	const auto attacksGuard = [&](const auto & goal, const CGHeroInstance * hero)
	{
		return goal->asTask()->isObjectAffected(guard->id)
			&& goal->asTask()->isObjectAffected(hero->id);
	};
	const auto byMain = [&](const auto & goal) { return attacksGuard(goal, mainHero()); };
	const auto byScout = [&](const auto & goal) { return attacksGuard(goal, scout()); };

	ASSERT_EQ(std::ranges::count_if(goals, byMain), 1)
		<< "the main must receive one safe attack on the entrance guard";
	EXPECT_TRUE(portalTasks(goals).empty())
		<< "clearing the guard must not send the main through the entrance";
	EXPECT_TRUE(std::ranges::none_of(goals, byScout))
		<< "the scout must not be used to clear the entrance guard";

	EXPECT_NO_THROW((*std::ranges::find_if(goals, byMain))->asTask()->accept(gateway.get()));
	EXPECT_FALSE(client->requested(entrance->visitablePos()))
		<< "clearing the guard must not move the main onto the portal entrance";
}

TEST_F(Nullkiller2_Behaviors_OneWayPortal, trappedHeroAttacksBeatableGuardAtLandedExit)
{
	start(makeGuardedExitMap({500, 100}, 1, {FAR_EXIT, GUARDED_EXIT}), GUARDED_EXIT);

	const auto * guard = requireObjectAt(GUARDED_EXIT_GUARD);
	auto * landedScout = scout();
	sendThroughPortal(*landedScout);
	const auto goals = decomposePortalBehavior();

	const auto forcedFight = std::ranges::find_if(goals, [&](const auto & goal)
	{
		return goal->asTask()->isObjectAffected(landedScout->id)
			&& goal->asTask()->isObjectAffected(guard->id);
	});
	ASSERT_NE(forcedFight, goals.end())
		<< "the hero must fight the guard of the exit it landed on";

	const auto evaluationContext = gateway->nullkiller->priorityEvaluator->buildEvaluationContext(*forcedFight);
	EXPECT_GT(
		gateway->nullkiller->priorityEvaluator->evaluate(
			*forcedFight,
			NK2AI::PriorityEvaluator::PriorityTier::ESCAPE,
			evaluationContext),
		0.0f) << "the fight out of the exit must be ranked as an escape";
}

TEST_F(Nullkiller2_Behaviors_OneWayPortal, trappedHeroDoesNotAttackOverwhelmingExitGuard)
{
	start(makeGuardedExitMap({500, 1}, 100), GUARDED_EXIT);

	const auto * guard = requireObjectAt(GUARDED_EXIT_GUARD);
	sendThroughPortal(*scout());

	EXPECT_TRUE(std::ranges::none_of(decomposePortalBehavior(), [&](const auto & goal)
	{
		return goal->asTask()->isObjectAffected(guard->id);
	})) << "a hero with a town to lose must wait instead of dying to a visible guard";
}

TEST_F(Nullkiller2_Behaviors_OneWayPortal, strongerMainReinforcesCompletedLandingForIsolatedEnemy)
{
	start(makeIsolatedEnemyPortalMap());

	auto * main = mainHero();
	auto * landedScout = scout();
	sendThroughPortal(*landedScout);
	advanceDay();

	EXPECT_TRUE(portalTasks(decomposePortalBehavior()).empty())
		<< "a hero still resolving the portal landing must suppress another expedition";

	moveHero(*landedScout, exit->visitablePos() + int3(1, 0, 0));
	const auto reinforcements = portalTasks(decomposePortalBehavior());
	ASSERT_EQ(reinforcements.size(), 1)
		<< "an isolated enemy objective must allow one stronger main-force follow-up";
	EXPECT_TRUE(reinforcements.front()->asTask()->isObjectAffected(main->id));
	EXPECT_FALSE(reinforcements.front()->asTask()->isObjectAffected(landedScout->id));
}

TEST_F(Nullkiller2_Behaviors_OneWayPortal, lostExpeditionIsRetriedByHeroThatBeatsVisibleExitGuard)
{
	auto builder = makeGuardedExitMap({500, 1, 1}, 20);
	// the exit corner is cut off, so the guard is not a target for heroes on the home side
	for(int i = 0; i <= 3; ++i)
	{
		builder.terrain({3, i, 0}, TerrainId::WATER);
		builder.terrain({i, 3, 0}, TerrainId::WATER);
	}
	start(std::move(builder), GUARDED_EXIT);

	const auto heroes = heroesByStrength();
	ASSERT_EQ(heroes.size(), 3);
	sendThroughPortal(*heroes.back());
	loseHero(*heroes.back());
	advanceDay();

	const auto retries = portalTasks(decomposePortalBehavior());
	ASSERT_EQ(retries.size(), 1)
		<< "losing the expedition must not suppress a hero that beats the guard seen at the exit";
	EXPECT_TRUE(retries.front()->asTask()->isObjectAffected(heroes.front()->id));
	EXPECT_FALSE(retries.front()->asTask()->isObjectAffected(heroes[1]->id))
		<< "another scout must not follow into the same guard";
}

TEST_F(Nullkiller2_Behaviors_OneWayPortal, lostExpeditionIsNotRetriedIntoOverwhelmingExitGuard)
{
	start(makeGuardedExitMap({100, 1}, 1000), GUARDED_EXIT);

	auto * lostScout = scout();
	sendThroughPortal(*lostScout);
	loseHero(*lostScout);
	advanceDay();

	EXPECT_TRUE(portalTasks(decomposePortalBehavior()).empty())
		<< "no hero may follow a lost expedition into a visible guard it can not beat";
}

TEST_F(Nullkiller2_Behaviors_OneWayPortal, reloadedProbeWithoutReturnDoesNotScheduleSecondScout)
{
	start(makeOneWayPortalMap({500, 1, 1}));

	sendThroughPortal(*scout());
	finishTurn();
	advanceDay();
	restartGateway();

	EXPECT_TRUE(portalTasks(decomposePortalBehavior()).empty())
		<< "loading a save after a probe must not send another scout through the same portal";
}

TEST_F(Nullkiller2_Behaviors_OneWayPortal, scoutedPortalWithReturnRouteIsNotProbedAgain)
{
	const int3 returnEntrance(24, 20, 0);
	const int3 returnExit(12, 20, 0);

	auto builder = makeOneWayPortalMap({500, 1, 1});
	builder
		.resource({8, 12, 0}, GameResID::GOLD, 1000)
		.oneWayPortalEntrance(returnEntrance, 1)
		.oneWayPortalExit(returnExit, 1);
	addWaterWall(builder);
	start(std::move(builder));

	auto * firstScout = scout();
	sendThroughPortal(*firstScout);
	teleportHero(*firstScout, *requireObjectAt(returnEntrance), *requireObjectAt(returnExit));
	advanceDay();

	EXPECT_TRUE(portalTasks(decomposePortalBehavior()).empty())
		<< "a portal that has nothing left to reveal must not keep pulling scouts and busy heroes through";
}

TEST_F(Nullkiller2_Behaviors_OneWayPortal, ownTownBehindPortalIsNotReturnRoute)
{
	const int3 farTown(30, 30, 0);

	auto builder = makeOneWayPortalMap({500, 1, 1});
	builder
		.resource({8, 12, 0}, GameResID::GOLD, 1000)
		.randomTown(farTown, PLAYER);
	addWaterWall(builder);
	start(std::move(builder));

	auto * firstScout = scout();
	sendThroughPortal(*firstScout);
	moveHero(*firstScout, exit->visitablePos() + int3(0, 1, 0));
	moveHero(*firstScout, requireObjectAt(farTown)->visitablePos());
	advanceDay();

	EXPECT_TRUE(portalTasks(decomposePortalBehavior()).empty())
		<< "a scout resting in a town it can not leave towards home has not found a way back";
}

TEST_F(Nullkiller2_Behaviors_OneWayPortal, busyMainDoesNotProbeWhileScoutIsUnavailable)
{
	auto builder = makeOneWayPortalMap({500, 1});
	builder.resource({8, 12, 0}, GameResID::GOLD, 1000);
	start(std::move(builder));

	gateway->nullkiller->lockHero(scout(), NK2AI::HeroLockedReason::HERO_CHAIN);

	EXPECT_TRUE(portalTasks(decomposePortalBehavior()).empty())
		<< "a main hero with useful tasks on this side must not make the blind probe";
}

TEST_F(Nullkiller2_Behaviors_OneWayPortal, landedHeroStepsOffExit)
{
	start(makeOneWayPortalMap({500, 1}));

	auto * landedScout = scout();
	sendThroughPortal(*landedScout);

	const auto goals = decomposePortalBehavior();
	const auto stepAside = std::ranges::find_if(goals, [&](const auto & goal)
	{
		return goal->asTask()->isObjectAffected(landedScout->id);
	});
	ASSERT_NE(stepAside, goals.end())
		<< "a hero left on the exit blocks every later arrival";

	EXPECT_NO_THROW((*stepAside)->asTask()->accept(gateway.get()));
	EXPECT_NE(landedScout->visitablePos(), exit->visitablePos());
}

TEST_F(Nullkiller2_Behaviors_OneWayPortal, busyMainStaysWithoutTownBehindPortal)
{
	startWithScoutBehindPortal(false);

	EXPECT_TRUE(portalTasks(decomposePortalBehavior()).empty())
		<< "without a return route or a town to settle in, treasure must not lure the main hero through";
}

TEST_F(Nullkiller2_Behaviors_OneWayPortal, busyMainCrossesForTownBehindPortal)
{
	startWithScoutBehindPortal(true);

	const auto crossings = portalTasks(decomposePortalBehavior());
	ASSERT_EQ(crossings.size(), 1);
	EXPECT_TRUE(crossings.front()->asTask()->isObjectAffected(mainHero()->id));

	gateway->nullkiller->dangerHitMap->updateHitMap();
	auto & evaluator = *gateway->nullkiller->priorityEvaluator;
	const auto context = evaluator.buildEvaluationContext(crossings.front());
	EXPECT_EQ(evaluator.evaluate(crossings.front(), NK2AI::PriorityEvaluator::PriorityTier::ONE_WAY_PORTAL_PROBE, context), 0)
		<< "a crossing for rewards must not run ahead of ordinary gathering";
	EXPECT_GT(context.goldReward, 0);
}

TEST_F(Nullkiller2_Behaviors_OneWayPortal, secondMainDoesNotFollowStrongerMainAcross)
{
	auto builder = makeRewardsBehindPortalMap(true);
	// the second town lets the AI keep two main heroes
	builder
		.randomTown({5, 15, 0}, PLAYER)
		.hero({11, 10, 0}, HeroTypeID(2), PLAYER)
		.heroGarrison({{CreatureID::ARCHER, 400}});
	start(std::move(builder));

	const auto heroes = heroesByStrength();
	ASSERT_EQ(heroes.size(), 3);
	sendThroughPortal(*heroes.back());
	moveHero(*heroes.back(), exit->visitablePos() + int3(1, 0, 0));
	advanceDay();

	const auto crossings = portalTasks(decomposePortalBehavior());
	ASSERT_EQ(crossings.size(), 1);
	auto * firstMain = crossings.front()->asTask()->isObjectAffected(heroes[0]->id) ? heroes[0] : heroes[1];
	sendThroughPortal(*firstMain);
	moveHero(*firstMain, exit->visitablePos() + int3(1, 1, 0));
	advanceDay();

	EXPECT_TRUE(portalTasks(decomposePortalBehavior()).empty())
		<< "rewards that a main hero behind the portal can take must not pull the remaining heroes after it";
}

TEST_F(Nullkiller2_Behaviors_OneWayPortal, loneScoutCollectsItsSideBeforeProbing)
{
	auto builder = makeOneWayPortalMap({1}, false);
	builder
		.randomTown({30, 30, 0}, PLAYER)
		.hero({28, 28, 0}, HeroTypeID(1), PLAYER)
		.heroGarrison({{CreatureID::ARCHER, 500}})
		.resource({8, 12, 0}, GameResID::GOLD, 1000);
	addWaterWall(builder);
	start(std::move(builder));

	ASSERT_EQ(gateway->nullkiller->heroManager->getHeroRoleOrDefaultInefficient(scout()), NK2AI::HeroRole::SCOUT);
	EXPECT_TRUE(portalTasks(decomposePortalBehavior()).empty())
		<< "treasure that nobody else can reach must be collected before the hero leaves for good";
}

TEST_F(Nullkiller2_Behaviors_OneWayPortal, lastScoutStaysWithItsTown)
{
	auto builder = makeOneWayPortalMap({500, 1});
	builder.resource({8, 12, 0}, GameResID::GOLD, 1000);
	start(std::move(builder));

	EXPECT_TRUE(portalTasks(decomposePortalBehavior()).empty())
		<< "the only scout of a town must not leave it for a blind probe";
}

TEST_F(Nullkiller2_Behaviors_OneWayPortal, idleMainFollowsScoutAcross)
{
	auto builder = makeOneWayPortalMap({500, 1, 1});
	addWaterWall(builder);
	start(std::move(builder));

	auto * landedScout = scout();
	sendThroughPortal(*landedScout);
	moveHero(*landedScout, exit->visitablePos() + int3(1, 0, 0));
	advanceDay();

	const auto crossings = portalTasks(decomposePortalBehavior());
	ASSERT_EQ(crossings.size(), 1);
	EXPECT_TRUE(crossings.front()->asTask()->isObjectAffected(mainHero()->id))
		<< "a main hero with nothing left at home must carry on behind the portal";
}

TEST_F(Nullkiller2_Behaviors_OneWayPortal, busyMainLeavesNeutralRewardsToExpeditionWithTown)
{
	auto builder = makeRewardsBehindPortalMap(false);
	builder.randomTown({30, 30, 0}, PLAYER);
	start(std::move(builder));

	auto * landedScout = scout();
	sendThroughPortal(*landedScout);
	moveHero(*landedScout, exit->visitablePos() + int3(1, 0, 0));
	advanceDay();

	EXPECT_TRUE(portalTasks(decomposePortalBehavior()).empty())
		<< "heroes that hold a town behind the portal grow into its neutral rewards without help from home";
}
