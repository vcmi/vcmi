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

#include <vstd/RNG.h>

#include <vcmi/Environment.h>
#include <vcmi/ServerCallback.h>

#include "../../lib/HeroBonus.h"
#include "../../lib/battle/BattleProxy.h"
#include "../../lib/battle/CUnitState.h"

VCMI_LIB_NAMESPACE_BEGIN

class CStack;

VCMI_LIB_NAMESPACE_END

class HypotheticBattle;

///Fake random generator, used by AI to evaluate random server behavior
class RNGStub : public vstd::RNG
{
public:
	vstd::TRandI64 getInt64Range(int64_t lower, int64_t upper) override
	{
		return [=]()->int64_t
		{
			return (lower + upper)/2;
		};
	}

	vstd::TRand getDoubleRange(double lower, double upper) override
	{
		return [=]()->double
		{
			return (lower + upper)/2;
		};
	}
};

class StackWithBonuses : public battle::CUnitState, public virtual IBonusBearer
{
public:
	std::vector<Bonus> bonusesToAdd;
	std::vector<Bonus> bonusesToUpdate;
	std::set<std::shared_ptr<Bonus>> bonusesToRemove;

	StackWithBonuses(const HypotheticBattle * Owner, const CStack * Stack);

	StackWithBonuses(const HypotheticBattle * Owner, const battle::UnitInfo & info);

	virtual ~StackWithBonuses() = default;

	StackWithBonuses & operator= (const battle::CUnitState & other);

	///IUnitInfo
	const CCreature * unitType() const override;

	int32_t unitBaseAmount() const override;

	uint32_t unitId() const override;
	ui8 unitSide() const override;
	PlayerColor unitOwner() const override;
	SlotID unitSlot() const override;

	///IBonusBearer
	TConstBonusListPtr getAllBonuses(const CSelector & selector, const CSelector & limit,
		const CBonusSystemNode * root = nullptr, const std::string & cachingStr = "") const override;

	int64_t getTreeVersion() const override;

	void addUnitBonus(const std::vector<Bonus> & bonus);
	void updateUnitBonus(const std::vector<Bonus> & bonus);
	void removeUnitBonus(const std::vector<Bonus> & bonus);

	void removeUnitBonus(const CSelector & selector);

	void spendMana(ServerCallback * server, const int spellCost) const override;
	std::string getDescription() const override;

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

	const Environment * env;

	HypotheticBattle(const Environment * ENV, Subject realBattle);

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
	void updateUnit(uint32_t id, const JsonNode & data) override;

	void addUnitBonus(uint32_t id, const std::vector<Bonus> & bonus) override;
	void updateUnitBonus(uint32_t id, const std::vector<Bonus> & bonus) override;
	void removeUnitBonus(uint32_t id, const std::vector<Bonus> & bonus) override;

	void setWallState(EWallPart partOfWall, EWallState state) override;

	void addObstacle(const ObstacleChanges & changes) override;
	void updateObstacle(const ObstacleChanges& changes) override;
	void removeObstacle(uint32_t id) override;

	uint32_t nextUnitId() const override;

	int64_t getActualDamage(const TDmgRange & damage, int32_t attackerCount, vstd::RNG & rng) const override;

	int64_t getTreeVersion() const;

#if SCRIPTING_ENABLED
	scripting::Pool * getContextPool() const override;
#endif

	ServerCallback * getServerCallback();

private:

	class HypotheticServerCallback : public ServerCallback
	{
	public:
		HypotheticServerCallback(HypotheticBattle * owner_);

		void complain(const std::string & problem) override;
		bool describeChanges() const override;

		vstd::RNG * getRNG() override;

		void apply(CPackForClient * pack) override;

		void apply(BattleLogMessage * pack) override;
		void apply(BattleStackMoved * pack) override;
		void apply(BattleUnitsChanged * pack) override;
		void apply(SetStackEffect * pack) override;
		void apply(StacksInjured * pack) override;
		void apply(BattleObstaclesChanged * pack) override;
		void apply(CatapultAttack * pack) override;
	private:
		HypotheticBattle * owner;
		RNGStub rngStub;
	};

	class HypotheticEnvironment : public Environment
	{
	public:
		HypotheticEnvironment(HypotheticBattle * owner_, const Environment * upperEnvironment);

		const Services * services() const override;
		const BattleCb * battle() const override;
		const GameCb * game() const override;
		vstd::CLoggerBase * logger() const override;
		events::EventBus * eventBus() const override;

	private:
		HypotheticBattle * owner;
		const Environment * env;
	};

	int32_t bonusTreeVersion;
	int32_t activeUnitId;
	mutable uint32_t nextId;

	std::unique_ptr<HypotheticServerCallback> serverCallback;
	std::unique_ptr<HypotheticEnvironment> localEnvironment;

#if SCRIPTING_ENABLED
	mutable std::shared_ptr<scripting::Pool> pool;
#endif
	mutable std::shared_ptr<events::EventBus> eventBus;
};
