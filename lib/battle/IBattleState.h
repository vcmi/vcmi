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
#include "BattleUnitTurnReason.h"

VCMI_LIB_NAMESPACE_BEGIN

class ObstacleChanges;
class UnitChanges;
struct Bonus;
struct BattleLayout;
class JsonNode;
class JsonSerializeFormat;
class BattleField;
class int3;

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

	virtual BattleID getBattleID() const = 0;

	virtual int32_t getActiveStackID() const = 0;

	virtual TStacks getStacksIf(const TStackFilter & predicate) const = 0;

	virtual battle::Units getUnitsIf(const battle::UnitFilter & predicate) const = 0;

	virtual BattleField getBattlefieldType() const = 0;
	virtual TerrainId getTerrainType() const = 0;

	virtual ObstacleCList getAllObstacles() const = 0;

	virtual const CGTownInstance * getDefendedTown() const = 0;
	virtual EWallState getWallState(EWallPart partOfWall) const = 0;
	virtual EGateState getGateState() const = 0;

	virtual PlayerColor getSidePlayer(BattleSide side) const = 0;
	virtual const CArmedInstance * getSideArmy(BattleSide side) const = 0;
	virtual const CGHeroInstance * getSideHero(BattleSide side) const = 0;
	/// Returns list of all spells used by specified side (and that can be learned by opposite hero)
	virtual std::vector<SpellID> getUsedSpells(BattleSide side) const = 0;

	virtual int32_t getCastSpells(BattleSide side) const = 0;
	virtual int32_t getEnchanterCounter(BattleSide side) const = 0;

	virtual ui8 getTacticDist() const = 0;
	virtual BattleSide getTacticsSide() const = 0;

	virtual uint32_t nextUnitId() const = 0;

	virtual int64_t getActualDamage(const DamageRange & damage, int32_t attackerCount, vstd::RNG & rng) const = 0;

	virtual int3 getLocation() const = 0;
	virtual BattleLayout getLayout() const = 0;

	virtual int32_t getRound() const = 0;
};

class DLL_LINKAGE IBattleState : public IBattleInfo
{
public:
	virtual void nextRound() = 0;
	virtual void nextTurn(uint32_t unitId, BattleUnitTurnReason reason) = 0;

	virtual void addUnit(uint32_t id, const JsonNode & data) = 0;
	virtual void setUnitState(uint32_t id, const JsonNode & data, int64_t healthDelta) = 0;
	virtual void moveUnit(uint32_t id, const BattleHex & destination) = 0;
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
