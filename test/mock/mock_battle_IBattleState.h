/*
 * mock_battle_IBattleState.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "../../lib/battle/IBattleState.h"

class BattleStateMock : public IBattleState
{
public:
	MOCK_CONST_METHOD0(getActiveStackID, int32_t());
	MOCK_CONST_METHOD1(getStacksIf, TStacks(TStackFilter));
	MOCK_CONST_METHOD1(getUnitsIf, battle::Units(battle::UnitFilter));
	MOCK_CONST_METHOD0(getBattlefieldType, BattleField());
	MOCK_CONST_METHOD0(getTerrainType, TerrainId());
	MOCK_CONST_METHOD0(getAllObstacles, IBattleInfo::ObstacleCList());
	MOCK_CONST_METHOD0(getDefendedTown, const CGTownInstance *());
	MOCK_CONST_METHOD1(getWallState, si8(int));
	MOCK_CONST_METHOD0(getGateState, EGateState());
	MOCK_CONST_METHOD1(getSidePlayer, PlayerColor(ui8));
	MOCK_CONST_METHOD1(getSideArmy, const CArmedInstance *(ui8));
	MOCK_CONST_METHOD1(getSideHero, const CGHeroInstance *(ui8));
	MOCK_CONST_METHOD1(getCastSpells, uint32_t(ui8));
	MOCK_CONST_METHOD1(getEnchanterCounter, int32_t(ui8));
	MOCK_CONST_METHOD0(getTacticDist, ui8());
	MOCK_CONST_METHOD0(getTacticsSide, ui8());
	MOCK_CONST_METHOD0(asBearer, const IBonusBearer *());
	MOCK_CONST_METHOD0(nextUnitId, uint32_t());
	MOCK_CONST_METHOD3(getActualDamage, int64_t(const TDmgRange &, int32_t, vstd::RNG &));

	MOCK_METHOD1(nextRound, void(int32_t));
	MOCK_METHOD1(nextTurn, void(uint32_t));
	MOCK_METHOD2(addUnit, void(uint32_t, const JsonNode &));
	MOCK_METHOD3(setUnitState, void(uint32_t, const JsonNode &, int64_t));
	MOCK_METHOD2(moveUnit, void(uint32_t, BattleHex));
	MOCK_METHOD1(removeUnit, void(uint32_t));
	MOCK_METHOD2(updateUnit, void(uint32_t, const JsonNode &));
	MOCK_METHOD2(addUnitBonus, void(uint32_t, const std::vector<Bonus> &));
	MOCK_METHOD2(updateUnitBonus, void(uint32_t, const std::vector<Bonus> &));
	MOCK_METHOD2(removeUnitBonus, void(uint32_t, const std::vector<Bonus> &));
	MOCK_METHOD2(setWallState, void(int, si8));
	MOCK_METHOD1(addObstacle, void(const ObstacleChanges &));
	MOCK_METHOD1(removeObstacle, void(uint32_t));
	MOCK_METHOD1(updateObstacle, void(const ObstacleChanges &));
};



