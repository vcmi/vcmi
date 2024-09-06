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
class VisitQuery : public CQuery
{
protected:
	VisitQuery(CGameHandler * owner, const CGObjectInstance *Obj, const CGHeroInstance *Hero);

public:
	const CGObjectInstance *visitedObject;
	const CGHeroInstance *visitingHero;

	bool blocksPack(const CPack *pack) const final;
	void onExposure(QueryPtr topQuery) final;
};

class MapObjectVisitQuery final : public VisitQuery
{
public:
	bool removeObjectAfterVisit;

	MapObjectVisitQuery(CGameHandler * owner, const CGObjectInstance *Obj, const CGHeroInstance *Hero);

	void onRemoval(PlayerColor color) final;
};

class TownBuildingVisitQuery final : public VisitQuery
{
public:
	BuildingID visitedBuilding;

	TownBuildingVisitQuery(CGameHandler * owner, const CGObjectInstance *Obj, const CGHeroInstance *Hero, BuildingID buildingToVisit);

	void onRemoval(PlayerColor color) final;
};
