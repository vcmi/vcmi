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

	int32_t getActiveStackID() const override;

	TStacks getStacksIf(TStackFilter predicate) const override;

	battle::Units getUnitsIf(battle::UnitFilter predicate) const override;

	BattleField getBattlefieldType() const override;
	TerrainId getTerrainType() const override;

	ObstacleCList getAllObstacles() const override;

	PlayerColor getSidePlayer(ui8 side) const override;
	const CArmedInstance * getSideArmy(ui8 side) const override;
	const CGHeroInstance * getSideHero(ui8 side) const override;

	ui8 getTacticDist() const override;
	ui8 getTacticsSide() const override;

	const CGTownInstance * getDefendedTown() const override;
	EWallState getWallState(EWallPart partOfWall) const override;
	EGateState getGateState() const override;

	uint32_t getCastSpells(ui8 side) const override;
	int32_t getEnchanterCounter(ui8 side) const override;

	const IBonusBearer * asBearer() const override;
protected:
	Subject subject;
};

VCMI_LIB_NAMESPACE_END
