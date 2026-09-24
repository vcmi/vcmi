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

	/// Held as an id rather than a pointer: the activity outlives a guard battle, which
	/// the hero may not - a beaten hero is taken off the map and put in the pool.
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
	void notifyObjectAboutRemoval(const CGObjectInstance * visitedObject, const CGHeroInstance * visitingHero, int32_t continuationTag) const override;
	bool blocksPack(const CPackForServer *pack) const override;
};

//yes/no and component selection dialogs
class BlockingDialogActivity : public DialogActivity
{
public:
	static constexpr ActivityType TYPE = ActivityType::BlockingDialog;

	const IObjectInterface * caller;
	BlockingDialog bd; //copy of pack... debug purposes

	BlockingDialogActivity(CGameHandler * owner, const IObjectInterface * caller, const BlockingDialog &bd);

	void notifyObjectAboutRemoval(const CGObjectInstance * visitedObject, const CGHeroInstance * visitingHero, int32_t continuationTag) const override;
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

	void notifyObjectAboutRemoval(const CGObjectInstance * visitedObject, const CGHeroInstance * visitingHero, int32_t continuationTag) const override;
};

/// Asks a player to pick skills as a hero gains levels, and then as their commander
/// does. A single hero can gain several levels at once, so this asks once per level
/// without leaving the stack in between: the player cannot act between two levels,
/// and whatever is waiting underneath - usually the visit that granted the
/// experience - is told once, when the whole sequence is over.
class LevelUpActivity : public Activity, public IInteraction
{
	/// Which of the two sequences is being asked about. The hero levels first, then
	/// the commander, matching the order the game applies them in.
	enum class Phase : uint8_t
	{
		Hero,
		Commander,
		Finished
	};

	Phase phase = Phase::Hero;
	ObjectInstanceID hero;

	/// Question the player is currently looking at. The client keeps its dialog open
	/// until the server reports that exact question resolved, so each one has to be
	/// released as it is answered - not once when the whole sequence ends.
	QuestionID askedQuestionID = QuestionID::NONE;

	/// Skills offered by the question currently outstanding, so that an answer can be
	/// turned back into the skill the player picked.
	std::vector<SecondarySkill> offeredHeroSkills;
	std::vector<ui32> offeredCommanderSkills;

	PromptResult askHeroLevelUp();
	PromptResult askCommanderLevelUp();

public:
	static constexpr ActivityType TYPE = ActivityType::HeroLevelUpDialog;

	LevelUpActivity(CGameHandler * owner, const CGHeroInstance * hero);

	IInteraction * asInteraction() final { return this; }
	PromptResult askNextQuestion() final;
	void applyAnswer(std::optional<int32_t> answer) final;

	bool endsByPlayerAnswer() const final;
	bool blocksPack(const CPackForServer * pack) const final;
	void onRemoval(PlayerColor color) final;
	void notifyObjectAboutRemoval(const CGObjectInstance * visitedObject, const CGHeroInstance * visitingHero, int32_t continuationTag) const final;
};
