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

#include "QueriesProcessor.h"
#include "../CGameHandler.h"
#include "../../lib/mapObjects/CGHeroInstance.h"

CObjectVisitQuery::CObjectVisitQuery(CGameHandler * owner, const CGObjectInstance * Obj, const CGHeroInstance * Hero):
	CQuery(owner), visitedObject(Obj), visitingHero(Hero), removeObjectAfterVisit(false)
{
	addPlayer(Hero->tempOwner);
}

bool CObjectVisitQuery::blocksPack(const CPack *pack) const
{
	//During the visit itself ALL actions are blocked.
	//(However, the visit may trigger a query above that'll pass some.)
	return true;
}

void CObjectVisitQuery::onRemoval(PlayerColor color)
{
	gh->objectVisitEnded(*this);

	//TODO or should it be destructor?
	//Can object visit affect 2 players and what would be desired behavior?
	if(removeObjectAfterVisit)
		gh->removeObject(visitedObject, color);
}

void CObjectVisitQuery::onExposure(QueryPtr topQuery)
{
	//Object may have been removed and deleted.
	if(gh->isValidObject(visitedObject))
		topQuery->notifyObjectAboutRemoval(visitedObject, visitingHero);

	owner->popIfTop(*this);
}
