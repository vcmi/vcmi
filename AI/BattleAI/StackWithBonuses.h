/*
 * StackWithBonuses.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once
#include "../../lib/HeroBonus.h"
#include "../../lib/battle/BattleProxy.h"
#include "../../lib/battle/CUnitState.h"

class HypotheticBattle;
class CStack;

class StackWithBonuses : public battle::CUnitState, public virtual IBonusBearer
{
public:

	std::vector<Bonus> bonusesToAdd;
	std::vector<Bonus> bonusesToUpdate;
	std::set<std::shared_ptr<Bonus>> bonusesToRemove;

	StackWithBonuses(const HypotheticBattle * Owner, const CStack * Stack);

	StackWithBonuses(const HypotheticBattle * Owner, const battle::UnitInfo & info);

	virtual ~StackWithBonuses();

	StackWithBonuses & operator= (const battle::CUnitState & other);

	///IUnitInfo
	const CCreature * unitType() const override;

	int32_t unitBaseAmount() const override;

	uint32_t unitId() const override;
	ui8 unitSide() const override;
	PlayerColor unitOwner() const override;
	SlotID unitSlot() const override;

	///IBonusBearer
	const TBonusListPtr getAllBonuses(const CSelector & selector, const CSelector & limit,
		const CBonusSystemNode * root = nullptr, const std::string & cachingStr = "") const override;

	int64_t getTreeVersion() const override;

	void addUnitBonus(const std::vector<Bonus> & bonus);
	void updateUnitBonus(const std::vector<Bonus> & bonus);
	void removeUnitBonus(const std::vector<Bonus> & bonus);

	void removeUnitBonus(const CSelector & selector);

	void spendMana(const spells::PacketSender * server, const int spellCost) const override;

private:
	const IBonusBearer * origBearer;
	const HypotheticBattle * owner;

	const CCreature * type;
	ui32 baseAmount;
	uint32_t id;
	ui8 side;
	PlayerColor player;
	SlotID slot;
};

class HypotheticBattle : public BattleProxy, public battle::IUnitEnvironment
{
public:
	std::map<uint32_t, std::shared_ptr<StackWithBonuses>> stackStates;

	HypotheticBattle(Subject realBattle);

	bool unitHasAmmoCart(const battle::Unit * unit) const override;
	PlayerColor unitEffectiveOwner(const battle::Unit * unit) const override;

	std::shared_ptr<StackWithBonuses> getForUpdate(uint32_t id);

	int32_t getActiveStackID() const override;

	battle::Units getUnitsIf(battle::UnitFilter predicate) const override;

	void nextRound(int32_t roundNr) override;
	void nextTurn(uint32_t unitId) override;

	void addUnit(uint32_t id, const JsonNode & data) override;
	void setUnitState(uint32_t id, const JsonNode & data, int64_t healthDelta) override;
	void moveUnit(uint32_t id, BattleHex destination) override;
	void removeUnit(uint32_t id) override;

	void addUnitBonus(uint32_t id, const std::vector<Bonus> & bonus) override;
	void updateUnitBonus(uint32_t id, const std::vector<Bonus> & bonus) override;
	void removeUnitBonus(uint32_t id, const std::vector<Bonus> & bonus) override;

	void setWallState(int partOfWall, si8 state) override;

	void addObstacle(const ObstacleChanges & changes) override;
	void updateObstacle(const ObstacleChanges& changes) override;
	void removeObstacle(uint32_t id) override;

	uint32_t nextUnitId() const override;

	int64_t getActualDamage(const TDmgRange & damage, int32_t attackerCount, vstd::RNG & rng) const override;

	int64_t getTreeVersion() const;

private:
	int32_t bonusTreeVersion;
	int32_t activeUnitId;
	mutable uint32_t nextId;
};
