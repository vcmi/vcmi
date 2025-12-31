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
#include "../../lib/battle/BattleLayout.h"
#include "../../lib/int3.h"

class BattleStateMock : public IBattleState
{
public:
	MOCK_CONST_METHOD0(getActiveStackID, int32_t());
	MOCK_CONST_METHOD1(getStacksIf, TStacks(const TStackFilter&));
	MOCK_CONST_METHOD1(getUnitsIf, battle::Units(const battle::UnitFilter &));
	MOCK_CONST_METHOD0(getBattlefieldType, BattleField());
	MOCK_CONST_METHOD0(getTerrainType, TerrainId());
	MOCK_CONST_METHOD0(getAllObstacles, IBattleInfo::ObstacleCList());
	MOCK_CONST_METHOD0(getDefendedTown, const CGTownInstance *());
	MOCK_CONST_METHOD1(getWallState, EWallState(EWallPart));
	MOCK_CONST_METHOD0(getGateState, EGateState());
	MOCK_CONST_METHOD1(getSidePlayer, PlayerColor(BattleSide));
	MOCK_CONST_METHOD1(getSideArmy, const CArmedInstance *(BattleSide));
	MOCK_CONST_METHOD1(getSideHero, const CGHeroInstance *(BattleSide));
	MOCK_CONST_METHOD1(getCastSpells, int32_t(BattleSide));
	MOCK_CONST_METHOD1(getEnchanterCounter, int32_t(BattleSide));
	MOCK_CONST_METHOD0(getTacticDist, ui8());
	MOCK_CONST_METHOD0(getTacticsSide, BattleSide());
	MOCK_CONST_METHOD0(getBonusBearer, const IBonusBearer *());
	MOCK_CONST_METHOD0(nextUnitId, uint32_t());
	MOCK_CONST_METHOD3(getActualDamage, int64_t(const DamageRange &, int32_t, vstd::RNG &));
	MOCK_CONST_METHOD0(getBattleID, BattleID());
	MOCK_CONST_METHOD0(getLocation, int3());
	MOCK_CONST_METHOD0(getLayout, BattleLayout());
	MOCK_CONST_METHOD1(getUsedSpells, std::vector<SpellID>(BattleSide));

	MOCK_METHOD0(nextRound, void());
	MOCK_METHOD2(nextTurn, void(uint32_t, BattleUnitTurnReason));
	MOCK_METHOD2(addUnit, void(uint32_t, const JsonNode &));
	MOCK_METHOD3(setUnitState, void(uint32_t, const JsonNode &, int64_t));
	MOCK_METHOD2(moveUnit, void(uint32_t, const BattleHex &));
	MOCK_METHOD1(removeUnit, void(uint32_t));
	MOCK_METHOD2(updateUnit, void(uint32_t, const JsonNode &));
	MOCK_METHOD2(addUnitBonus, void(uint32_t, const std::vector<Bonus> &));
	MOCK_METHOD2(updateUnitBonus, void(uint32_t, const std::vector<Bonus> &));
	MOCK_METHOD2(removeUnitBonus, void(uint32_t, const std::vector<Bonus> &));
	MOCK_METHOD2(setWallState, void(EWallPart, EWallState));
	MOCK_METHOD1(addObstacle, void(const ObstacleChanges &));
	MOCK_METHOD1(removeObstacle, void(uint32_t));
	MOCK_METHOD1(updateObstacle, void(const ObstacleChanges &));
};



