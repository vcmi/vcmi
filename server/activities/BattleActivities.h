/*
 * BattleActivities.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "Activity.h"
#include "../../lib/networkPacks/PacksForClientBattle.h"
#include "../../lib/battle/BattleSide.h"

class IBattleInfo;
struct SideInBattle;

class BattleActivity : public Activity
{
public:
	static constexpr ActivityType TYPE = ActivityType::Battle;

	BattleSideArray<const CArmedInstance *> belligerents;

	BattleID battleID;
	std::optional<BattleResult> result;
	std::vector<ObjectInstanceID> heroesWithDeferredLevelUp;

	bool hasPendingBattleOrVisitActivities() const;

	/// Hands the postponed level-ups over and forgets them, so that whoever takes them is
	/// the only one that applies them.
	std::vector<ObjectInstanceID> takeDeferredLevelUps();

	BattleActivity(CGameHandler * owner);
	BattleActivity(CGameHandler * owner, const IBattleInfo * Bi);
	void notifyObjectAboutRemoval(const IObjectInterface * visitedObject, const CGHeroInstance * visitingHero, int32_t continuationTag) const override;
	bool blocksPack(const CPackForServer *pack) const override;
	void onRemoval(PlayerColor color) override;
	void onExposure(ActivityPtr topActivity) override;
};

class BattleResultActivity : public DialogActivity
{
	bool resultProcessed = false;
	const IBattleInfo * bi;
	std::optional<BattleResult> result;

public:
	static constexpr ActivityType TYPE = ActivityType::BattleDialog;
	BattleResultActivity(CGameHandler * owner, const IBattleInfo * Bi, const std::optional<BattleResult> & Br);
	void onRemoval(PlayerColor color) override;
};
