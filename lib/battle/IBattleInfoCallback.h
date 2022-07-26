/*
 * IBattleInfoCallback.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "BattleHex.h"

VCMI_LIB_NAMESPACE_BEGIN

struct CObstacleInstance;
class BattleField;
class Terrain;

namespace battle
{
	class IUnitInfo;
	class Unit;
	using Units = std::vector<const Unit *>;
	using UnitFilter = std::function<bool(const Unit *)>;
}

#if SCRIPTING_ENABLED
namespace scripting
{
	class Pool;
}
#endif

class DLL_LINKAGE IBattleInfoCallback
{
public:
#if SCRIPTING_ENABLED
	virtual scripting::Pool * getContextPool() const = 0;
#endif

	virtual Terrain battleTerrainType() const = 0;
	virtual BattleField battleGetBattlefieldType() const = 0;

	///return none if battle is ongoing; otherwise the victorious side (0/1) or 2 if it is a draw
	virtual boost::optional<int> battleIsFinished() const = 0;

	virtual si8 battleTacticDist() const = 0; //returns tactic distance in current tactics phase; 0 if not in tactics phase
	virtual si8 battleGetTacticsSide() const = 0; //returns which side is in tactics phase, undefined if none (?)

	virtual uint32_t battleNextUnitId() const = 0;

	virtual battle::Units battleGetUnitsIf(battle::UnitFilter predicate) const = 0;

	virtual const battle::Unit * battleGetUnitByID(uint32_t ID) const = 0;
	virtual const battle::Unit * battleGetUnitByPos(BattleHex pos, bool onlyAlive = true) const = 0;

	virtual const battle::Unit * battleActiveUnit() const = 0;

	//blocking obstacles makes tile inaccessible, others cause special effects (like Land Mines, Moat, Quicksands)
	virtual std::vector<std::shared_ptr<const CObstacleInstance>> battleGetAllObstaclesOnPos(BattleHex tile, bool onlyBlocking = true) const = 0;
	virtual std::vector<std::shared_ptr<const CObstacleInstance>> getAllAffectedObstaclesByStack(const battle::Unit * unit) const = 0;
};



VCMI_LIB_NAMESPACE_END
