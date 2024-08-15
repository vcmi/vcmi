/*
 * mock_IBattleInfoCallback.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "../../lib/battle/IBattleInfoCallback.h"
#include "../../lib/GameConstants.h"

class IBattleInfoCallbackMock : public IBattleInfoCallback
{
public:
#if SCRIPTING_ENABLED
	MOCK_CONST_METHOD0(getContextPool, scripting::Pool *());
#endif
	MOCK_CONST_METHOD0(battleTerrainType, TerrainId());
	MOCK_CONST_METHOD0(battleGetBattlefieldType, BattleField());

	MOCK_CONST_METHOD0(battleIsFinished, std::optional<BattleSide>());

	MOCK_CONST_METHOD0(battleTacticDist, si8());
	MOCK_CONST_METHOD0(battleGetTacticsSide, BattleSide());

	MOCK_CONST_METHOD0(battleNextUnitId, uint32_t());

	MOCK_CONST_METHOD1(battleGetUnitsIf, battle::Units(const battle::UnitFilter &));

	MOCK_CONST_METHOD1(battleGetUnitByID, const battle::Unit *(uint32_t));
	MOCK_CONST_METHOD2(battleGetUnitByPos, const battle::Unit *(BattleHex, bool));
	MOCK_CONST_METHOD0(battleActiveUnit, const battle::Unit *());

	MOCK_CONST_METHOD0(getBonusBearer, IBonusBearer*());
	MOCK_CONST_METHOD0(getBattle, IBattleInfo*());
	MOCK_CONST_METHOD0(getPlayerID, std::optional<PlayerColor>());

	MOCK_CONST_METHOD2(battleGetAllObstaclesOnPos, std::vector<std::shared_ptr<const CObstacleInstance>>(BattleHex, bool));
	MOCK_CONST_METHOD2(getAllAffectedObstaclesByStack, std::vector<std::shared_ptr<const CObstacleInstance>>(const battle::Unit *, const std::set<BattleHex> &));

};




