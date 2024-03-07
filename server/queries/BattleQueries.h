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

VCMI_LIB_NAMESPACE_BEGIN
class IBattleInfo;
struct SideInBattle;
VCMI_LIB_NAMESPACE_END

class CBattleQuery : public CQuery
{
public:
	std::array<const CArmedInstance *,2> belligerents;
	std::array<int, 2> initialHeroMana;

	bool isAiVsHuman;
	BattleID battleID;
	std::optional<BattleResult> result;

	CBattleQuery(CGameHandler * owner);
	CBattleQuery(CGameHandler * owner, const IBattleInfo * Bi); //TODO
	void notifyObjectAboutRemoval(const CObjectVisitQuery &objectVisit) const override;
	bool blocksPack(const CPack *pack) const override;
	void onRemoval(PlayerColor color) override;
	void onExposure(QueryPtr topQuery) override;
};

class CBattleDialogQuery : public CDialogQuery
{
public:
	CBattleDialogQuery(CGameHandler * owner, const IBattleInfo * Bi, std::optional<BattleResult> Br);

	const IBattleInfo * bi;
	std::optional<BattleResult> result;
	void onRemoval(PlayerColor color) override;
};
