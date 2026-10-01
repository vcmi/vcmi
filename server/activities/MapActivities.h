/*
 * MapActivities.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "Activity.h"
#include "../../lib/networkPacks/PacksForClient.h"

class CGHeroInstance;
class CGObjectInstance;
class IObjectInterface;
class CArmedInstance;

//Created when player starts turn or when player puts game on [ause
//Removed when player accepts a turn or continur play
class TimerPauseActivity : public Activity
{
public:
	static constexpr ActivityType TYPE = ActivityType::TimerPause;

	TimerPauseActivity(CGameHandler * owner, PlayerColor player);

	bool blocksPack(const CPackForServer *pack) const override;
	void onAdded() override;
	void onRemoval() override;
	bool endsByPlayerAnswer() const override;
};

//Created when hero attempts move and something happens
//(not necessarily position change, could be just an object interaction).
class HeroMovementActivity : public Activity
{
public:
	static constexpr ActivityType TYPE = ActivityType::HeroMovement;

	TryMoveHero tmh;
	bool visitDestAfterVictory; //if hero moved to guarded tile and it should be visited once guard is defeated

	/// Stored as id, not as pointer: the activity outlives a guard battle, but a defeated
	/// hero is removed from the map and put into the pool.
	ObjectInstanceID hero;

	void onChildCompleted(const ActivityPtr & child) override;

	HeroMovementActivity(CGameHandler * owner, const TryMoveHero & Tmh, const CGHeroInstance * Hero, bool VisitDestAfterVictory = false);
	void onAdded() override;
	void onRemoval() override;
};

/// Garrison or hero exchange window of one player. An exchange between allies opens one for each of them.
class GarrisonDialogActivity : public DialogActivity
{
public:
	static constexpr ActivityType TYPE = ActivityType::GarrisonDialog;

	std::array<const CArmedInstance *,2> exchangingArmies;

	GarrisonDialogActivity(CGameHandler * owner, PlayerColor player, const CArmedInstance *up, const CArmedInstance *down);
	void notifyObjectAboutRemoval(const IObjectInterface * visitedObject, const CGHeroInstance * visitingHero, const JsonNode & visitState) const override;
	bool blocksPack(const CPackForServer *pack) const override;
};

//yes/no and component selection dialogs
class BlockingDialogActivity : public DialogActivity
{
public:
	static constexpr ActivityType TYPE = ActivityType::BlockingDialog;

	BlockingDialog bd; //copy of pack... debug purposes

	BlockingDialogActivity(CGameHandler * owner, const BlockingDialog & bd);

	bool acceptsAnswer(int32_t answer) const override;

	void notifyObjectAboutRemoval(const IObjectInterface * visitedObject, const CGHeroInstance * visitingHero, const JsonNode & visitState) const override;
};

class OpenWindowActivity : public DialogActivity
{
	EOpenWindowMode mode;
public:
	static constexpr ActivityType TYPE = ActivityType::OpenWindow;

	OpenWindowActivity(CGameHandler * owner, const CGHeroInstance *hero, EOpenWindowMode mode);

	bool blocksPack(const CPackForServer *pack) const override;
};

class TeleportDialogActivity : public DialogActivity
{
public:
	static constexpr ActivityType TYPE = ActivityType::TeleportDialog;

	TeleportDialog td; //copy of pack... debug purposes

	TeleportDialogActivity(CGameHandler * owner, const TeleportDialog & dialog);

	void notifyObjectAboutRemoval(const IObjectInterface * visitedObject, const CGHeroInstance * visitingHero, const JsonNode & visitState) const override;
};

/// Drives the level-ups a hero has pending: one prompt per gained hero level, then one per
/// gained commander level. Stays on the stack for the whole chain, so the player can not act
/// between two levels and the activity below, usually the visit that granted the experience,
/// is notified only once.
class LevelUpRoutine final : public Activity, public IRoutine
{
	/// Stored as id: the routine outlives every prompt it pushes.
	ObjectInstanceID hero;

public:
	static constexpr ActivityType TYPE = ActivityType::HeroLevelUp;

	LevelUpRoutine(CGameHandler * owner, const CGHeroInstance * hero);

	IRoutine * asRoutine() final { return this; }
	StepResult advance() final;

	bool blocksPack(const CPackForServer * pack) const final;
	void notifyObjectAboutRemoval(const IObjectInterface * visitedObject, const CGHeroInstance * visitingHero, const JsonNode & visitState) const final;
};

/// Asks a player which secondary skill a hero gains for one level, and grants it.
class HeroLevelUpPrompt final : public DialogActivity
{
	ObjectInstanceID hero;
	HeroLevelUp levelUp; ///< the pack sent to the player; its skills map the answer back to a skill

public:
	static constexpr ActivityType TYPE = ActivityType::HeroLevelUpDialog;

	HeroLevelUpPrompt(CGameHandler * owner, const CGHeroInstance * hero, const HeroLevelUp & rolled);

	bool acceptsAnswer(int32_t answer) const final;

	void onAdded() final;
	void onRemoval() final;
};

/// Asks a player which skill a hero's commander gains for one level, and grants it.
class CommanderLevelUpPrompt final : public DialogActivity
{
	ObjectInstanceID hero;
	CommanderLevelUp levelUp; ///< the pack sent to the player; its skills map the answer back to a skill

public:
	static constexpr ActivityType TYPE = ActivityType::CommanderLevelUpDialog;

	CommanderLevelUpPrompt(CGameHandler * owner, const CGHeroInstance * hero, const CommanderLevelUp & rolled);

	bool acceptsAnswer(int32_t answer) const final;

	void onAdded() final;
	void onRemoval() final;
};
