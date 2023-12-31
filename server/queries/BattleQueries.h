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
VCMI_LIB_NAMESPACE_END

class CBattleQuery : public CQuery
{
public:
	std::array<const CArmedInstance *,2> belligerents;
	std::array<int, 2> initialHeroMana;

	BattleID battleID;
	std::optional<BattleResult> result;

	CBattleQuery(CGameHandler * owner);
	CBattleQuery(CGameHandler * owner, const IBattleInfo * Bi); //TODO
	virtual void notifyObjectAboutRemoval(const CObjectVisitQuery &objectVisit) const override;
	virtual bool blocksPack(const CPack *pack) const override;
	virtual void onRemoval(PlayerColor color) override;
	virtual void onExposure(QueryPtr topQuery) override;
};

class CBattleDialogQuery : public CDialogQuery
{
public:
	CBattleDialogQuery(CGameHandler * owner, const IBattleInfo * Bi);

	const IBattleInfo * bi;

	virtual void onRemoval(PlayerColor color) override;
};
