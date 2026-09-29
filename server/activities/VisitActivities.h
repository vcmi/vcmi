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

/// Common base for activities that run an object's interaction with a hero: a visit to a
/// map object, a walk through a town's buildings. Holds what the interaction needs to hand
/// results back to the object, not what identifies a visit - only a map object visit makes
/// that object busy.
class ObjectInteractionActivity : public Activity
{
protected:
	ObjectInteractionActivity(CGameHandler * owner, const CGHeroInstance * Hero, ActivityType type);

public:
	/// Hero whose interaction is running now. A town walks several heroes through its
	/// buildings, so this changes as the routine advances.
	ObjectInstanceID visitingHero;

	/// Value set by the visited object, passed back to it once the activity that it started
	/// finishes, so that it can identify its own step without deducing it from state that
	/// may have changed in the meantime.
	int32_t continuationTag = 0;

	bool blocksPack(const CPackForServer * pack) const final;
};

/// Hero visit to a map object: starts the visit, waits for the object, then applies the
/// level-ups postponed by a battle during the visit. One of these makes its object busy,
/// so at most one exists per object and per hero.
class MapObjectVisitActivity final : public ObjectInteractionActivity, public IRoutine
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

	ObjectInstanceID visitedObject;
	bool removeObjectAfterVisit;

	MapObjectVisitActivity(CGameHandler * owner, const CGObjectInstance * Obj, const CGHeroInstance * Hero);

	IRoutine * asRoutine() final { return this; }
	StepResult advance() final;
	void onChildCompleted(const ActivityPtr & child) final;
	void onAdded(PlayerColor color) final;
	void onRemoval(PlayerColor color) final;
};

/// Visits a list of hero/building pairs one at a time. A building may open a dialog or
/// start a battle, which suspends the routine until it finishes.
class TownBuildingVisitActivity final : public ObjectInteractionActivity, public IRoutine
{
	struct BuildingVisit
	{
		ObjectInstanceID hero;
		BuildingID building;
	};

	std::vector<BuildingVisit> visits;

	size_t cursor = 0; ///< index of the next pair to visit

	/// Town whose buildings are being walked. Not a visited object: entering the town is a
	/// separate MapObjectVisitActivity, and a building may also be visited from the town
	/// screen with no such visit around.
	ObjectInstanceID town;

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

/// Runs what happens at the start of a player's turn: the pause that the player has to
/// accept, then a visit to each object their heroes stand on, one at a time. Both live in
/// one routine because a stack can not be inserted into - adding the visits on their own
/// would put them in front of the pause.
class TurnStartRoutine final : public Activity, public IRoutine
{
	struct PendingVisit
	{
		ObjectInstanceID object;
		ObjectInstanceID hero;
	};

	enum class Step : uint8_t
	{
		Pause,
		CollectVisits,
		Visits
	};

	Step activeStep = Step::Pause;

	/// Pause that ends when the player accepts the start of their turn. Null when turn
	/// timers are off, and for an AI.
	ActivityPtr turnPause;

	/// Collected only once the pause is over, so that a town captured meanwhile by another
	/// player acting at the same time is not visited.
	std::vector<PendingVisit> visits;

	size_t cursor = 0; ///< index of the next visit

	void collectVisits();
	StepResult visitNext();

public:
	static constexpr ActivityType TYPE = ActivityType::TurnStart;

	TurnStartRoutine(CGameHandler * owner, PlayerColor player, ActivityPtr turnPause);

	IRoutine * asRoutine() final { return this; }
	StepResult advance() final;
};
