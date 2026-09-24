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

	/// What the visited object said it was part way through, handed back to it when
	/// whatever it started finishes. Lets an object tell its own steps apart without
	/// working it out again afterwards, from state that may have changed in between.
	int32_t continuationTag = 0;

	bool blocksPack(const CPackForServer * pack) const final;
};

/// Drives a hero's visit to a map object: starts it, lets the object take over, and
/// once the object is finished applies any level-ups that a battle during the visit
/// postponed.
class MapObjectVisitActivity final : public VisitActivity, public IRoutine
{
	/// Position within the visit. Also tells onChildCompleted() whether a finished
	/// child belongs to the object's own reward pipeline, or is a level-up that
	/// merely follows a battle and must not be reported back to the object.
	enum class Step : uint8_t
	{
		NotStarted,
		StartVisit,
		DeferredLevelUps,
		Finished
	};

	Step activeStep = Step::NotStarted;

	/// Heroes that won experience in a battle during this visit, whose level-up
	/// prompts were held back until the object had applied the battle result.
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

/// Visits a list of hero/building pairs one at a time. A building may open a dialog
/// or start a battle, in which case the routine suspends until that finishes and then
/// carries on from the next pair.
class TownBuildingVisitActivity final : public VisitActivity, public IRoutine
{
	struct BuildingVisit
	{
		ObjectInstanceID hero;
		BuildingID building;
	};

	std::vector<BuildingVisit> visits;

	/// Index of the next pair to visit - the routine's position within the activity.
	size_t cursor = 0;

	/// Building whose visit is in progress. It, not the town, is what asked whatever
	/// is running above, so it is what has to be told when that finishes.
	BuildingID visitedBuilding;

public:
	static constexpr ActivityType TYPE = ActivityType::TownBuildingVisit;

	TownBuildingVisitActivity(CGameHandler * owner, const CGTownInstance * Obj, std::vector<const CGHeroInstance *> heroes, std::vector<BuildingID> buildingToVisit);

	IRoutine * asRoutine() final { return this; }
	StepResult advance() final;
	void onChildCompleted(const ActivityPtr & child) final;
};

/// Visits the objects a player's heroes are standing on when their turn begins, one
/// at a time. Queued to start only once the player has nothing else pending, so that
/// it does not interrupt, for example, the dialog accepting the start of the turn.
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

	/// Index of the next visit - the routine's position within the activity.
	size_t cursor = 0;
};
