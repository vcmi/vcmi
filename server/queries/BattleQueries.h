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
	BattleSideArray<const CArmedInstance *> belligerents;
	BattleSideArray<int> initialHeroMana;

	BattleID battleID;
	std::optional<BattleResult> result;

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
	CBattleDialogQuery(CGameHandler * owner, const IBattleInfo * Bi, const std::optional<BattleResult> & Br);
	void onRemoval(PlayerColor color) override;
};
