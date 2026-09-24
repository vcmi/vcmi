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
	void onExposure(ActivityPtr topActivity) override;
	void onAdding(PlayerColor color) override;
	void onRemoval(PlayerColor color) override;
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

	void onExposure(ActivityPtr topActivity) override;

	HeroMovementActivity(CGameHandler * owner, const TryMoveHero & Tmh, const CGHeroInstance * Hero, bool VisitDestAfterVictory = false);
	void onAdding(PlayerColor color) override;
	void onRemoval(PlayerColor color) override;
};

class GarrisonDialogActivity : public DialogActivity //used also for hero exchange dialogs
{
public:
	static constexpr ActivityType TYPE = ActivityType::GarrisonDialog;

	std::array<const CArmedInstance *,2> exchangingArmies;

	GarrisonDialogActivity(CGameHandler * owner, const CArmedInstance *up, const CArmedInstance *down);
	void notifyObjectAboutRemoval(const IObjectInterface * visitedObject, const CGHeroInstance * visitingHero, int32_t continuationTag) const override;
	bool blocksPack(const CPackForServer *pack) const override;
};

//yes/no and component selection dialogs
class BlockingDialogActivity : public DialogActivity
{
public:
	static constexpr ActivityType TYPE = ActivityType::BlockingDialog;

	BlockingDialog bd; //copy of pack... debug purposes

	BlockingDialogActivity(CGameHandler * owner, const BlockingDialog & bd);

	void notifyObjectAboutRemoval(const IObjectInterface * visitedObject, const CGHeroInstance * visitingHero, int32_t continuationTag) const override;
};

class OpenWindowActivity : public DialogActivity
{
	EOpenWindowMode mode;
public:
	static constexpr ActivityType TYPE = ActivityType::OpenWindow;

	OpenWindowActivity(CGameHandler * owner, const CGHeroInstance *hero, EOpenWindowMode mode);

	bool blocksPack(const CPackForServer *pack) const override;
	void onExposure(ActivityPtr topActivity) override;
};

class TeleportDialogActivity : public DialogActivity
{
public:
	static constexpr ActivityType TYPE = ActivityType::TeleportDialog;

	TeleportDialog td; //copy of pack... debug purposes

	TeleportDialogActivity(CGameHandler * owner, const TeleportDialog & dialog);

	void notifyObjectAboutRemoval(const IObjectInterface * visitedObject, const CGHeroInstance * visitingHero, int32_t continuationTag) const override;
};

/// Asks a player to pick skills for the levels gained by a hero and then by their
/// commander. A hero can gain several levels at once, so this asks once per level without
/// leaving the stack in between: the player can not act between two levels, and the
/// activity below, usually the visit that granted the experience, is notified only once.
class LevelUpActivity : public Activity, public IInteraction
{
	/// Hero levels are asked about first, then commander levels, in the order in which
	/// the game applies them.
	enum class Phase : uint8_t
	{
		Hero,
		Commander,
		Finished
	};

	Phase phase = Phase::Hero;
	ObjectInstanceID hero;

	/// Skills offered by the outstanding question, to map an answer back to a skill.
	std::vector<SecondarySkill> offeredHeroSkills;
	std::vector<ui32> offeredCommanderSkills;

	PromptResult askHeroLevelUp();
	PromptResult askCommanderLevelUp();

public:
	static constexpr ActivityType TYPE = ActivityType::HeroLevelUpDialog;

	LevelUpActivity(CGameHandler * owner, const CGHeroInstance * hero);

	IInteraction * asInteraction() final { return this; }
	PromptResult askNextQuestion() final;
	void applyAnswer(QuestionID answered, std::optional<int32_t> answer) final;

	bool endsByPlayerAnswer() const final;
	bool blocksPack(const CPackForServer * pack) const final;
	void onRemoval(PlayerColor color) final;
	void notifyObjectAboutRemoval(const IObjectInterface * visitedObject, const CGHeroInstance * visitingHero, int32_t continuationTag) const final;
};
