/*
 * QuestTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "QuestTest.h"

#include "../../lib/CPlayerState.h"
#include "../../lib/mapObjects/CGHeroInstance.h"
#include "../../lib/mapObjects/CGObjectInstance.h"
#include "../../lib/pathfinder/CGPathNode.h"
#include "../../lib/pathfinder/CPathfinder.h"
#include "../../lib/pathfinder/PathfinderOptions.h"

void QuestTest::onMapStarted()
{
	gameEvents().setGameInfoCallback(gameState().get());
}

void QuestTest::markObjectDestroyed(PlayerColor player, ObjectInstanceID target)
{
	auto it = gameState()->players.find(player);
	ASSERT_NE(it, gameState()->players.end()) << "Player " << player.getNum() << " not initialised";
	it->second.destroyedObjects.insert(target);
}

void QuestTest::visit(CGHeroInstance * hero, CGObjectInstance * obj)
{
	ASSERT_NE(hero, nullptr);
	ASSERT_NE(obj, nullptr);
	visitedObject = obj;
	obj->onHeroVisit(gameEvents(), hero);
	finishExperienceGrants(hero);
}

void QuestTest::answerDialog(CGHeroInstance * hero, int32_t answer)
{
	// Pop the most recent (innermost) dialog first — nested visits enqueue
	// in LIFO order and an answer should resolve the topmost prompt.
	auto & queue = gameEvents().blockingDialogs;
	ASSERT_FALSE(queue.empty())
		<< "answerDialog called with no pending BlockingDialog — visit must enqueue one first";
	queue.pop_back();

	ASSERT_NE(visitedObject, nullptr) << "answerDialog called before any visit";
	visitedObject->blockingDialogAnswered(gameEvents(), hero, answer, std::exchange(gameEvents().visitState, {}));
	finishExperienceGrants(hero);
}

void QuestTest::finishExperienceGrants(CGHeroInstance * hero)
{
	while(std::exchange(gameEvents().experienceGranted, false))
		visitedObject->experienceApplied(gameEvents(), hero, std::exchange(gameEvents().visitState, {}));
}

void QuestTest::advanceDays(int days)
{
	// Bump the calendar, then run newTurn on every object so per-turn expiry
	// (quest timeout / HotA reach-date) fires through the real code path rather
	// than needing a manual setObjProperty in each test.
	ASSERT_GE(days, 0);
	gameState()->day += static_cast<ui32>(days);

	GameRandomizer randomizer(*gameState());
	for(const auto & obj : map()->objects)
		if(obj)
			obj->newTurn(gameEvents(), randomizer);
}

EPathNodeAction QuestTest::pathActionAt(const CGHeroInstance * hero, const int3 & tile)
{
	revealMap(hero->getOwner());

	CPathsInfo paths(gameState()->getMapSize(), hero);
	auto config = std::make_shared<SingleHeroPathfinderConfig>(paths, *gameState(), hero);
	CPathfinder pathfinder(*gameState(), config);
	pathfinder.calculatePaths();
	return paths.getNode(tile, EPathfindingLayer::LAND)->action;
}
