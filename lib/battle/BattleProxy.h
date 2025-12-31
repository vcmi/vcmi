/*
 * BattleProxy.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once
#include "CBattleInfoCallback.h"
#include "IBattleState.h"

VCMI_LIB_NAMESPACE_BEGIN

class DLL_LINKAGE BattleProxy : public CBattleInfoCallback, public IBattleState
{
public:
	using Subject = std::shared_ptr<CBattleInfoCallback>;

	BattleProxy(Subject subject_);
	~BattleProxy();

	//////////////////////////////////////////////////////////////////////////
	// IBattleInfo
	const IBattleInfo * getBattle() const override;
	std::optional<PlayerColor> getPlayerID() const override;

	int32_t getActiveStackID() const override;

	TStacks getStacksIf(const TStackFilter & predicate) const override;

	battle::Units getUnitsIf(const battle::UnitFilter & predicate) const override;

	BattleField getBattlefieldType() const override;
	TerrainId getTerrainType() const override;

	ObstacleCList getAllObstacles() const override;

	PlayerColor getSidePlayer(BattleSide side) const override;
	const CArmedInstance * getSideArmy(BattleSide side) const override;
	const CGHeroInstance * getSideHero(BattleSide side) const override;

	ui8 getTacticDist() const override;
	BattleSide getTacticsSide() const override;

	const CGTownInstance * getDefendedTown() const override;
	EWallState getWallState(EWallPart partOfWall) const override;
	EGateState getGateState() const override;

	int32_t getCastSpells(BattleSide side) const override;
	int32_t getEnchanterCounter(BattleSide side) const override;

	const IBonusBearer * getBonusBearer() const override;
protected:
	Subject subject;
};

VCMI_LIB_NAMESPACE_END
