/*
 * TaskFailureTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 */
#include "StdInc.h"

#include "AI/Nullkiller2/AIGateway.h"
#include "AI/Nullkiller2/Behaviors/EscapeBehavior.h"
#include "AI/Nullkiller2/Engine/Nullkiller.h"
#include "AI/Nullkiller2/Goals/CGoal.h"
#include "AI/Nullkiller2/Goals/ExecuteHeroChain.h"

#include "mock/TinyH3MBuilder.h"
#include "quest/QuestTest.h"

#include "lib/callback/CCallback.h"
#include "lib/mapObjects/CGHeroInstance.h"

#include <tbb/global_control.h>

namespace NK2AI
{
class NullkillerTaskFailureTestAccess
{
public:
	static bool executeTask(Nullkiller & ai, const Goals::TTask & task)
	{
		return ai.executeTask(task);
	}

	static void prepareState(Nullkiller & ai)
	{
		ai.resetState();
		ai.updateState();
	}
};
}

namespace
{
const PlayerColor PLAYER(0);
const PlayerColor ENEMY(1);

class FailingMultiNodePathGoal : public NK2AI::Goals::ElementarGoal<FailingMultiNodePathGoal>
{
	NK2AI::Nullkiller * nullkiller;
	const CGHeroInstance * movingHero;
	int3 destination;
	int3 failedStep;

public:
	FailingMultiNodePathGoal(
		NK2AI::Nullkiller & nullkillerToFail,
		const CGHeroInstance & heroToMove,
		const int3 & routeDestination,
		const int3 & failedRouteStep)
		: ElementarGoal(NK2AI::Goals::EXECUTE_HERO_CHAIN)
		, nullkiller(&nullkillerToFail)
		, movingHero(&heroToMove)
		, destination(routeDestination)
		, failedStep(failedRouteStep)
	{
	}

	bool operator==(const FailingMultiNodePathGoal & other) const override
	{
		return movingHero == other.movingHero
			&& destination == other.destination
			&& failedStep == other.failedStep;
	}

	void accept(NK2AI::AIGateway *) override
	{
		nullkiller->setActive(movingHero, destination);
		nullkiller->setActive(movingHero, failedStep);
		throw NK2AI::cannotFulfillGoalException("expected route failure");
	}

	std::string toString() const override
	{
		return "FailingMultiNodePathGoal";
	}
};

const NK2AI::Goals::ExecuteHeroChain * findHeroChain(
	const NK2AI::Goals::TGoalVec & goals,
	const CGHeroInstance * hero)
{
	for(const auto & goal : goals)
	{
		const auto * chain = dynamic_cast<const NK2AI::Goals::ExecuteHeroChain *>(goal.get());
		if(chain && chain->getHero() == hero)
			return chain;
	}

	return nullptr;
}

class FailedEscapeRouteTest : public QuestTest
{
protected:
	void startGame()
	{
		TinyH3M::TinyH3MBuilder builder(EMapFormat::SOD);
		builder
			.size(36, false)
			.playerActive(PLAYER)
			.playerActive(ENEMY)
			.hero({5, 5, 0}, HeroTypeID(0), PLAYER)
			.heroGarrison({{CreatureID(0), 1}})
			.hero({7, 5, 0}, HeroTypeID(1), ENEMY)
			.heroGarrison({{CreatureID(13), 20}});

		startWithMap(std::move(builder));
	}
};
}

TEST_F(FailedEscapeRouteTest, replanningAvoidsFailedMultiNodeEscapeRoute)
{
	tbb::global_control singleThread(tbb::global_control::max_allowed_parallelism, 1);
	startGame();
	revealMap(PLAYER);

	auto * escapingHero = findHeroByOwner(PLAYER);
	auto * threateningHero = findHeroByOwner(ENEMY);
	ASSERT_NE(escapingHero, nullptr);
	ASSERT_NE(threateningHero, nullptr);
	escapingHero->setMovementPoints(2000);
	threateningHero->setMovementPoints(1000);

	const auto callback = makeCallback(PLAYER);
	auto gateway = std::make_unique<NK2AI::AIGateway>();
	gateway->initGameInterface(std::shared_ptr<Environment>(), callback);
	auto & ai = *gateway->nullkiller;
	NK2AI::NullkillerTaskFailureTestAccess::prepareState(ai);

	NK2AI::Goals::EscapeBehavior escape;
	const auto initialGoals = escape.decompose(&ai);
	const auto * initialChain = findHeroChain(initialGoals, escapingHero);
	ASSERT_NE(initialChain, nullptr);
	const int3 failedDestination = initialChain->getPath().targetTile();

	const auto failedTask = NK2AI::Goals::taskptr(FailingMultiNodePathGoal(
		ai,
		*escapingHero,
		failedDestination,
		escapingHero->visitablePos()));
	ASSERT_FALSE(NK2AI::NullkillerTaskFailureTestAccess::executeTask(ai, failedTask));

	const auto replannedGoals = escape.decompose(&ai);
	const auto * replannedChain = findHeroChain(replannedGoals, escapingHero);
	ASSERT_NE(replannedChain, nullptr);
	EXPECT_NE(replannedChain->getPath().targetTile(), failedDestination)
		<< "the failed final destination must not be selected again";
}
