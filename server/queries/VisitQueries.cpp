/*
 * VisitQueries.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "VisitQueries.h"

#include "../../lib/gameState/CGameState.h"
#include "../../lib/mapObjects/CGHeroInstance.h"
#include "../../lib/mapObjects/CGTownInstance.h"
#include "../../lib/mapObjects/TownBuildingInstance.h"
#include "../CGameHandler.h"
#include "QueriesProcessor.h"

VisitQuery::VisitQuery(CGameHandler * owner, const CGObjectInstance * Obj, const CGHeroInstance * Hero)
	: CQuery(owner)
	, visitedObject(Obj->id)
	, visitingHero(Hero->id)
{
	addPlayer(Hero->tempOwner);
}

bool VisitQuery::blocksPack(const CPackForServer * pack) const
{
	//During the visit itself ALL actions are blocked.
	//(However, the visit may trigger a query above that'll pass some.)
	return true;
}

void MapObjectVisitQuery::onExposure(QueryPtr topQuery)
{
	auto object = gh->gameState().getObjInstance(visitedObject);
	auto hero = gh->gameState().getHero(visitingHero);

	//Object may have been removed and deleted.
	if (object)
		topQuery->notifyObjectAboutRemoval(object, hero);

	owner->popIfTop(*this);
}

MapObjectVisitQuery::MapObjectVisitQuery(CGameHandler * owner, const CGObjectInstance * Obj, const CGHeroInstance * Hero)
	: VisitQuery(owner, Obj, Hero)
	, removeObjectAfterVisit(false)
{
}

void MapObjectVisitQuery::onRemoval(PlayerColor color)
{
	auto object = gh->gameState().getObjInstance(visitedObject);

	gh->objectVisitEnded(visitingHero, players.front());

	//Can object visit affect 2 players and what would be desired behavior?
	if(removeObjectAfterVisit)
		gh->removeObject(object, color);
}

TownBuildingVisitQuery::TownBuildingVisitQuery(CGameHandler * owner, const CGTownInstance * Obj, std::vector<const CGHeroInstance *> heroes, std::vector<BuildingID> buildingToVisit)
	: VisitQuery(owner, Obj, heroes.front())
	, visitedTown(Obj)
{
	// generate in reverse order - first building-hero pair to handle must be in the end of vector
	for (auto const * hero : boost::adaptors::reverse(heroes))
		for (auto const & building : boost::adaptors::reverse(buildingToVisit))
			visitedBuilding.push_back({ hero, building});
}

void TownBuildingVisitQuery::onExposure(QueryPtr topQuery)
{
	auto object = gh->gameState().getObjInstance(visitedObject);
	auto hero = gh->gameState().getHero(visitingHero);

	topQuery->notifyObjectAboutRemoval(object, hero);

	onAdded(players.front());
}

void TownBuildingVisitQuery::onAdded(PlayerColor color)
{
	while (!visitedBuilding.empty() && owner->topQuery(color).get() == this)
	{
		visitingHero = visitedBuilding.back().hero->id;
		const auto & building = visitedTown->rewardableBuildings.at(visitedBuilding.back().building);
		building->onHeroVisit(*gh, visitedBuilding.back().hero);
		visitedBuilding.pop_back();
	}

	if (visitedBuilding.empty() && owner->topQuery(color).get() == this)
		owner->popIfTop(*this);
}
