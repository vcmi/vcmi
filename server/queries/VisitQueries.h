/*
 * VisitQueries.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "CQuery.h"

//Created when hero visits object.
//Removed when query above is resolved (or immediately after visit if no queries were created)
class CObjectVisitQuery : public CQuery
{
public:
	const CGObjectInstance *visitedObject;
	const CGHeroInstance *visitingHero;

	bool removeObjectAfterVisit;

	CObjectVisitQuery(CGameHandler * owner, const CGObjectInstance *Obj, const CGHeroInstance *Hero);

	bool blocksPack(const CPack *pack) const override;
	void onRemoval(PlayerColor color) override;
	void onExposure(QueryPtr topQuery) override;
};
