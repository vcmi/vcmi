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

#include "../../lib/mapObjects/CGHeroInstance.h"
#include "../CGameHandler.h"
#include "QueriesProcessor.h"

VisitQuery::VisitQuery(CGameHandler * owner, const CGObjectInstance * Obj, const CGHeroInstance * Hero)
	: CQuery(owner)
	, visitedObject(Obj)
	, visitingHero(Hero)
{
	addPlayer(Hero->tempOwner);
}

bool VisitQuery::blocksPack(const CPack * pack) const
{
	//During the visit itself ALL actions are blocked.
	//(However, the visit may trigger a query above that'll pass some.)
	return true;
}

void VisitQuery::onExposure(QueryPtr topQuery)
{
	//Object may have been removed and deleted.
	if(gh->isValidObject(visitedObject))
		topQuery->notifyObjectAboutRemoval(visitedObject, visitingHero);

	owner->popIfTop(*this);
}

MapObjectVisitQuery::MapObjectVisitQuery(CGameHandler * owner, const CGObjectInstance * Obj, const CGHeroInstance * Hero)
	: VisitQuery(owner, Obj, Hero)
	, removeObjectAfterVisit(false)
{
}

void MapObjectVisitQuery::onRemoval(PlayerColor color)
{
	gh->objectVisitEnded(visitingHero, players.front());

	//TODO or should it be destructor?
	//Can object visit affect 2 players and what would be desired behavior?
	if(removeObjectAfterVisit)
		gh->removeObject(visitedObject, color);
}

TownBuildingVisitQuery::TownBuildingVisitQuery(CGameHandler * owner, const CGObjectInstance * Obj, const CGHeroInstance * Hero, BuildingID buildingToVisit)
	: VisitQuery(owner, Obj, Hero)
	, visitedBuilding(buildingToVisit)
{
}

void TownBuildingVisitQuery::onRemoval(PlayerColor color)
{

}
