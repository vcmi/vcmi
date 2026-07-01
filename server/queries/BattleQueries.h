/*
 * BattleQueries.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "CQuery.h"
#include "../../lib/networkPacks/PacksForClientBattle.h"
#include "../../lib/battle/BattleSide.h"

VCMI_LIB_NAMESPACE_BEGIN
class IBattleInfo;
struct SideInBattle;
VCMI_LIB_NAMESPACE_END

class CBattleQuery : public CQuery
{
public:
	static constexpr QueryType TYPE = QueryType::Battle;

	BattleSideArray<const CArmedInstance *> belligerents;

	BattleID battleID;
	std::optional<BattleResult> result;
	std::vector<ObjectInstanceID> heroesWithDeferredLevelUp;
	mutable bool deferredLevelUpsApplied = false;

	bool hasPendingBattleOrVisitQueries() const;
	std::vector<ObjectInstanceID> takeDeferredLevelUps();
	void completeDeferredLevelUps() const;

	CBattleQuery(CGameHandler * owner);
	CBattleQuery(CGameHandler * owner, const IBattleInfo * Bi);
	void notifyObjectAboutRemoval(const CGObjectInstance * visitedObject, const CGHeroInstance * visitingHero) const override;
	bool blocksPack(const CPackForServer *pack) const override;
	void onRemoval(PlayerColor color) override;
	void onExposure(QueryPtr topQuery) override;
};

class CBattleDialogQuery : public CDialogQuery
{
	bool resultProcessed = false;
	const IBattleInfo * bi;
	std::optional<BattleResult> result;

public:
	static constexpr QueryType TYPE = QueryType::BattleDialog;
	CBattleDialogQuery(CGameHandler * owner, const IBattleInfo * Bi, const std::optional<BattleResult> & Br);
	void onRemoval(PlayerColor color) override;
};
