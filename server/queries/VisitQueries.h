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

class CGTownInstance;

//Created when hero visits object.
//Removed when query above is resolved (or immediately after visit if no queries were created)
class VisitQuery : public CQuery
{
protected:
	VisitQuery(CGameHandler * owner, const CGObjectInstance * Obj, const CGHeroInstance * Hero, QueryType type);

public:
	ObjectInstanceID visitedObject;
	ObjectInstanceID visitingHero;

	bool blocksPack(const CPackForServer * pack) const final;
};

class MapObjectVisitQuery final : public VisitQuery
{
	std::vector<ObjectInstanceID> deferredBattleLevelUps;
	bool processingDeferredBattleLevelUps = false;

public:
	static constexpr QueryType TYPE = QueryType::MapObjectVisit;

	bool removeObjectAfterVisit;

	MapObjectVisitQuery(CGameHandler * owner, const CGObjectInstance * Obj, const CGHeroInstance * Hero);

	void onRemoval(PlayerColor color) final;
	void onExposure(QueryPtr topQuery) final;
};

/// Visits a list of hero/building pairs one at a time. A building may open a dialog
/// or start a battle, in which case the routine suspends until that finishes and then
/// carries on from the next pair.
class TownBuildingVisitQuery final : public VisitQuery, public IRoutine
{
	struct BuildingVisit
	{
		ObjectInstanceID hero;
		BuildingID building;
	};

	std::vector<BuildingVisit> visits;

	/// Index of the next pair to visit - the routine's position within the activity.
	size_t cursor = 0;

public:
	static constexpr QueryType TYPE = QueryType::TownBuildingVisit;

	TownBuildingVisitQuery(CGameHandler * owner, const CGTownInstance * Obj, std::vector<const CGHeroInstance *> heroes, std::vector<BuildingID> buildingToVisit);

	IRoutine * asRoutine() final { return this; }
	StepResult advance() final;
	void onChildCompleted(const QueryPtr & child) final;
};
