/*
 * VisitActivities.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "VisitActivities.h"

#include "BattleActivities.h"

#include "../../lib/gameState/CGameState.h"
#include "../../lib/mapObjects/CGHeroInstance.h"
#include "../../lib/mapObjects/CGTownInstance.h"
#include "../../lib/mapObjects/TownBuildingInstance.h"
#include "../CGameHandler.h"
#include "ActivityProcessor.h"

#include <vcmi/scripting/MapEventDispatcher.h>

VisitActivity::VisitActivity(CGameHandler * owner, const CGObjectInstance * Obj, const CGHeroInstance * Hero, ActivityType type)
	: Activity(owner, type)
	, visitedObject(Obj->id)
	, visitingHero(Hero->id)
{
	addPlayer(Hero->tempOwner);
}

bool VisitActivity::blocksPack(const CPackForServer * pack) const
{
	// During the visit itself all actions are blocked - except answering a question,
	// which may have been asked by an activity that has since been removed or buried.
	// Refusing those is what leaves both sides waiting for each other.
	// (The visit may also trigger an activity above that lets more through.)
	return blockAllButReply(pack);
}

MapObjectVisitActivity::MapObjectVisitActivity(CGameHandler * owner, const CGObjectInstance * Obj, const CGHeroInstance * Hero)
	: VisitActivity(owner, Obj, Hero, TYPE)
	, removeObjectAfterVisit(false)
{
}

StepResult MapObjectVisitActivity::advance()
{
	// activeStep is set before the work runs, so that children pushed by a step are
	// attributed to it when they finish.
	switch(activeStep)
	{
		case Step::NotStarted:
			activeStep = Step::StartVisit;
			startVisit();
			return StepResult::Continue;

		case Step::StartVisit:
			activeStep = Step::DeferredLevelUps;
			applyDeferredLevelUps();
			return StepResult::Continue;

		default:
			activeStep = Step::Finished;
			return StepResult::Done;
	}
}

void MapObjectVisitActivity::startVisit()
{
	const auto * object = gh->gameInfo().getObj(visitedObject);
	const auto * hero = gh->gameState().getHero(visitingHero);

	if(!object || !hero)
		return;

	HeroVisit hv;
	hv.objId = visitedObject;
	hv.heroId = visitingHero;
	hv.player = hero->tempOwner;
	hv.starting = true;
	gh->sendAndApply(hv);

	// The object takes over from here. Anything it starts - a dialog, a battle - is
	// pushed on top of this routine, which resumes once that has finished.
	std::string scriptHandler = object->getVisitScriptHandler();
	auto * dispatcher = gh->gameState().getMapEventDispatcher();

	if(!scriptHandler.empty() && dispatcher)
	{
		gh->runScriptedEvent(*dispatcher, hero->getOwner(), hero->id,
			[&](scripting::MapEventDispatcher & d){ return d.onObjectVisit(*gh, scriptHandler, object, hero); });
	}
	else
	{
		object->onHeroVisit(*gh, hero);
	}
}

void MapObjectVisitActivity::applyDeferredLevelUps()
{
	auto pending = std::move(deferredBattleLevelUps);
	deferredBattleLevelUps.clear();

	for(const auto & heroID : pending)
		if(const auto * hero = gh->gameState().getHero(heroID))
			gh->expGiven(hero);
}

void MapObjectVisitActivity::onChildCompleted(const ActivityPtr & child)
{
	// A level-up prompt shown during the DeferredLevelUps step comes from experience
	// won in a battle, not from the object's reward. Reporting it to the object would
	// run heroLevelUpDone() again and grant the reward a second time.
	if(activeStep != Step::DeferredLevelUps)
	{
		const auto * object = gh->gameInfo().getObj(visitedObject);
		const auto * hero = gh->gameState().getHero(visitingHero);

		// The object may have been removed by the visit itself. The hero may be dead,
		// and is deliberately still passed on: objects such as CGCreature need to be
		// told about a battle their defender won, and check the result rather than
		// the hero.
		if(object)
			child->notifyObjectAboutRemoval(object, hero, continuationTag);
	}

	if(auto battleActivity = std::dynamic_pointer_cast<BattleActivity>(child))
	{
		auto levelUps = battleActivity->takeDeferredLevelUps();
		deferredBattleLevelUps.insert(deferredBattleLevelUps.end(), levelUps.begin(), levelUps.end());
	}
}

void MapObjectVisitActivity::onRemoval(PlayerColor color)
{
	gh->objectVisitEnded(visitingHero, players.front());

	if(removeObjectAfterVisit)
		gh->removeObject(gh->gameState().getObjInstance(visitedObject), color);
}

TownBuildingVisitActivity::TownBuildingVisitActivity(CGameHandler * owner, const CGTownInstance * Obj, std::vector<const CGHeroInstance *> heroes, std::vector<BuildingID> buildingToVisit)
	: VisitActivity(owner, Obj, heroes.front(), TYPE)
{
	for (const auto * hero : heroes)
		for (const auto & building : buildingToVisit)
			visits.push_back({ hero->id, building });
}

void TownBuildingVisitActivity::onChildCompleted(const ActivityPtr & child)
{
	const auto * object = gh->gameState().getObjInstance(visitedObject);
	const auto * hero = gh->gameState().getHero(visitingHero);

	// The town may have changed hands or the hero may have died in the meantime.
	if(object)
		child->notifyObjectAboutRemoval(object, hero, continuationTag);
}

StepResult TownBuildingVisitActivity::advance()
{
	if(cursor >= visits.size())
		return StepResult::Done;

	const auto & visit = visits.at(cursor++);

	const auto * town = gh->gameInfo().getTown(visitedObject);
	const auto * hero = gh->gameState().getHero(visit.hero);

	// Either may be gone if an earlier building started a battle - skip that pair
	// rather than abandoning the buildings that come after it.
	if(!town || !hero)
		return StepResult::Continue;

	auto building = town->rewardableBuildings.find(visit.building);
	if(building == town->rewardableBuildings.end())
		return StepResult::Continue;

	visitingHero = visit.hero;
	building->second->onHeroVisit(*gh, hero);

	return StepResult::Continue;
}

TurnStartVisitActivity::TurnStartVisitActivity(CGameHandler * owner, PlayerColor player, std::vector<PendingVisit> visits)
	: Activity(owner, TYPE)
	, visits(std::move(visits))
{
	addPlayer(player);
}

StepResult TurnStartVisitActivity::advance()
{
	if(cursor >= visits.size())
		return StepResult::Done;

	const auto & visit = visits.at(cursor++);

	const auto * object = gh->gameState().getObjInstance(visit.object);
	const auto * hero = gh->gameState().getHero(visit.hero);

	// The town may have been captured, or the hero moved away or died, between the
	// visits being collected and this one being reached.
	if(!object || !hero)
		return StepResult::Continue;

	if(hero->visitablePos() != object->visitablePos())
		return StepResult::Continue;

	if(gh->getVisitingHero(object) != nullptr)
		return StepResult::Continue;

	gh->objectVisited(object, hero);

	return StepResult::Continue;
}
