/*
 * VisitActivities.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "Activity.h"

class CGTownInstance;

//Created when hero visits object.
//Removed when activity above is resolved (or immediately after visit if no activities were created)
class VisitActivity : public Activity
{
protected:
	VisitActivity(CGameHandler * owner, const CGObjectInstance * Obj, const CGHeroInstance * Hero, ActivityType type);

public:
	ObjectInstanceID visitedObject;
	ObjectInstanceID visitingHero;

	/// Value set by the visited object, passed back to it once the activity that it started
	/// finishes, so that it can identify its own step without deducing it from state that
	/// may have changed in the meantime.
	int32_t continuationTag = 0;

	bool blocksPack(const CPackForServer * pack) const final;
};

/// Hero visit to a map object: starts the visit, waits for the object, then applies the
/// level-ups postponed by a battle during the visit.
class MapObjectVisitActivity final : public VisitActivity, public IRoutine
{
	/// Position within the visit. Also tells onChildCompleted() whether a finished child
	/// belongs to the object, or is a postponed level-up that must not be reported to it.
	enum class Step : uint8_t
	{
		NotStarted,
		StartVisit,
		DeferredLevelUps,
		Finished
	};

	Step activeStep = Step::NotStarted;

	/// Heroes that gained experience in a battle during this visit. Their level-up dialogs
	/// are postponed until the object has applied the battle result.
	std::vector<ObjectInstanceID> deferredBattleLevelUps;

	void startVisit();
	void applyDeferredLevelUps();

public:
	static constexpr ActivityType TYPE = ActivityType::MapObjectVisit;

	bool removeObjectAfterVisit;

	MapObjectVisitActivity(CGameHandler * owner, const CGObjectInstance * Obj, const CGHeroInstance * Hero);

	IRoutine * asRoutine() final { return this; }
	StepResult advance() final;
	void onChildCompleted(const ActivityPtr & child) final;
	void onRemoval(PlayerColor color) final;
};

/// Visits a list of hero/building pairs one at a time. A building may open a dialog or
/// start a battle, which suspends the routine until it finishes.
class TownBuildingVisitActivity final : public VisitActivity, public IRoutine
{
	struct BuildingVisit
	{
		ObjectInstanceID hero;
		BuildingID building;
	};

	std::vector<BuildingVisit> visits;

	size_t cursor = 0; ///< index of the next pair to visit

	/// Building whose visit is in progress. Activities above belong to the building and
	/// not to the town, so results are reported to the building.
	BuildingID visitedBuilding;

public:
	static constexpr ActivityType TYPE = ActivityType::TownBuildingVisit;

	TownBuildingVisitActivity(CGameHandler * owner, const CGTownInstance * Obj, std::vector<const CGHeroInstance *> heroes, std::vector<BuildingID> buildingToVisit);

	IRoutine * asRoutine() final { return this; }
	StepResult advance() final;
	void onChildCompleted(const ActivityPtr & child) final;
};

/// Visits the objects that a player's heroes stand on at the start of their turn, one at a
/// time. Started only once the player has nothing else pending, so that it does not
/// interrupt e.g. the dialog that accepts the start of the turn.
class TurnStartVisitActivity final : public Activity, public IRoutine
{
public:
	struct PendingVisit
	{
		ObjectInstanceID object;
		ObjectInstanceID hero;
	};

	static constexpr ActivityType TYPE = ActivityType::TurnStartVisit;

	TurnStartVisitActivity(CGameHandler * owner, PlayerColor player, std::vector<PendingVisit> visits);

	IRoutine * asRoutine() final { return this; }
	StepResult advance() final;

private:
	std::vector<PendingVisit> visits;

	size_t cursor = 0; ///< index of the next visit
};
