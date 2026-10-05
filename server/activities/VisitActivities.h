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

#include "../../lib/json/JsonNode.h"

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

	/// The object's own record of where its interaction stopped, handed back to it once the
	/// activity that suspended the interaction finishes. Opaque to the server.
	JsonNode visitState;

	bool blocksPack(const CPackForServer * pack) const final;
	std::string toString() const override;
};

/// Hero visit to a map object: starts the visit and waits for the object. One of these makes
/// its object busy, so at most one exists per object and per hero.
class MapObjectVisitActivity final : public ObjectInteractionActivity, public IRoutine
{
	bool started = false;

	void startVisit();

public:
	static constexpr ActivityType TYPE = ActivityType::MapObjectVisit;

	ObjectInstanceID visitedObject;
	bool removeObjectAfterVisit;

	MapObjectVisitActivity(CGameHandler * owner, const CGObjectInstance * Obj, const CGHeroInstance * Hero);

	IRoutine * asRoutine() final { return this; }
	StepResult advance() final;
	void onChildCompleted(const ActivityPtr & child) final;
	void onAdded() final;
	void onRemoval() final;
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

/// Runs what happens at the start of a player's turn, in order: the pause that the player
/// has to accept, the scenario and town events of the day, then a visit to each object
/// their heroes stand on. All of it is one routine because a stack can not be inserted
/// into - adding any of these on their own would put it in front of the earlier ones.
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
		Events,
		CollectVisits,
		Visits
	};

	Step activeStep = Step::Pause;

	/// Pause that ends when the player accepts the start of their turn. Null when turn
	/// timers are off, and for an AI.
	ActivityPtr turnPause;

	/// Collected only once the events are over, so that a town captured meanwhile by another
	/// player acting at the same time, or a hero a script moved, is not visited.
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
