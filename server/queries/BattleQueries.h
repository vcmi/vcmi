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

#include "../../lib/NetPacks.h"

struct SideInBattle;

class CBattleQuery : public CGhQuery
{
public:
	std::array<const CArmedInstance *,2> belligerents;
	std::array<int, 2> initialHeroMana;

	const BattleInfo *bi;
	std::optional<BattleResult> result;

	CBattleQuery(CGameHandler * owner);
	CBattleQuery(CGameHandler * owner, const BattleInfo * Bi); //TODO
	virtual void notifyObjectAboutRemoval(const CObjectVisitQuery &objectVisit) const override;
	virtual bool blocksPack(const CPack *pack) const override;
	virtual void onRemoval(PlayerColor color) override;
};

class CBattleDialogQuery : public CDialogQuery
{
public:
	CBattleDialogQuery(CGameHandler * owner, const BattleInfo * Bi);
	CBattleDialogQuery(CGameHandler * owner, const BattleInfo * Bi, const SideInBattle & sideToAdd);

	const BattleInfo * bi;

	virtual void onRemoval(PlayerColor color) override;
};
