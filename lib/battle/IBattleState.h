/*
 * IBattleState.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once
#include "CBattleInfoEssentials.h"

VCMI_LIB_NAMESPACE_BEGIN

class ObstacleChanges;
class UnitChanges;
struct Bonus;
class JsonNode;
class JsonSerializeFormat;
class BattleField;

namespace vstd
{
	class RNG;
}

namespace battle
{
	class UnitInfo;
}

class DLL_LINKAGE IBattleInfo : public IConstBonusProvider
{
public:
	using ObstacleCList = std::vector<std::shared_ptr<const CObstacleInstance>>;

	virtual ~IBattleInfo() = default;

	virtual int32_t getActiveStackID() const = 0;

	virtual TStacks getStacksIf(TStackFilter predicate) const = 0;

	virtual battle::Units getUnitsIf(battle::UnitFilter predicate) const = 0;

	virtual BattleField getBattlefieldType() const = 0;
	virtual TerrainId getTerrainType() const = 0;

	virtual ObstacleCList getAllObstacles() const = 0;

	virtual const CGTownInstance * getDefendedTown() const = 0;
	virtual EWallState getWallState(EWallPart partOfWall) const = 0;
	virtual EGateState getGateState() const = 0;

	virtual PlayerColor getSidePlayer(ui8 side) const = 0;
	virtual const CArmedInstance * getSideArmy(ui8 side) const = 0;
	virtual const CGHeroInstance * getSideHero(ui8 side) const = 0;

	virtual uint32_t getCastSpells(ui8 side) const = 0;
	virtual int32_t getEnchanterCounter(ui8 side) const = 0;

	virtual ui8 getTacticDist() const = 0;
	virtual ui8 getTacticsSide() const = 0;

	virtual uint32_t nextUnitId() const = 0;

	virtual int64_t getActualDamage(const DamageRange & damage, int32_t attackerCount, vstd::RNG & rng) const = 0;
};

class DLL_LINKAGE IBattleState : public IBattleInfo
{
public:
	//TODO: add non-const API

	virtual void nextRound(int32_t roundNr) = 0;
	virtual void nextTurn(uint32_t unitId) = 0;

	virtual void addUnit(uint32_t id, const JsonNode & data) = 0;
	virtual void setUnitState(uint32_t id, const JsonNode & data, int64_t healthDelta) = 0;
	virtual void moveUnit(uint32_t id, BattleHex destination) = 0;
	virtual void removeUnit(uint32_t id) = 0;
	virtual void updateUnit(uint32_t id, const JsonNode & data) = 0;

	virtual void addUnitBonus(uint32_t id, const std::vector<Bonus> & bonus) = 0;
	virtual void updateUnitBonus(uint32_t id, const std::vector<Bonus> & bonus) = 0;
	virtual void removeUnitBonus(uint32_t id, const std::vector<Bonus> & bonus) = 0;

	virtual void setWallState(EWallPart partOfWall, EWallState state) = 0;

	virtual void addObstacle(const ObstacleChanges & changes) = 0;
	virtual void updateObstacle(const ObstacleChanges & changes) = 0;
	virtual void removeObstacle(uint32_t id) = 0;
};

VCMI_LIB_NAMESPACE_END
