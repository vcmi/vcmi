/*
 * CBattleInfoCallbackTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "../../lib/battle/CBattleInfoCallback.h"
#include "../../lib/battle/CUnitState.h"

#include <vstd/RNG.h>

#include "../../lib/NetPacksBase.h"

#include "mock/mock_BonusBearer.h"
#include "mock/mock_battle_IBattleState.h"
#include "mock/mock_battle_Unit.h"
#include "mock/mock_scripting_Pool.h"

using namespace battle;
using namespace testing;

class UnitFake : public UnitMock
{
private:
	std::shared_ptr<CUnitState> state;

public:
	UnitFake()
	{
		state.reset(new CUnitStateDetached(this, this));
	}

	void addNewBonus(const std::shared_ptr<Bonus> & b)
	{
		bonusFake.addNewBonus(b);
	}

	void makeAlive()
	{
		EXPECT_CALL(*this, alive()).WillRepeatedly(Return(true));
	}

	void makeWarMachine()
	{
		addNewBonus(std::make_shared<Bonus>(Bonus::PERMANENT, Bonus::SIEGE_WEAPON, Bonus::CREATURE_ABILITY, 1, 0));
	}

	void redirectBonusesToFake()
	{
		ON_CALL(*this, getAllBonuses(_, _, _, _)).WillByDefault(Invoke(&bonusFake, &BonusBearerMock::getAllBonuses));
		ON_CALL(*this, getTreeVersion()).WillByDefault(Invoke(&bonusFake, &BonusBearerMock::getTreeVersion));
	}

	void expectAnyBonusSystemCall()
	{
		EXPECT_CALL(*this, getAllBonuses(_, _, _, _)).Times(AtLeast(0));
		EXPECT_CALL(*this, getTreeVersion()).Times(AtLeast(0));
	}

	void setDefaultExpectations()
	{
		EXPECT_CALL(*this, unitSlot()).WillRepeatedly(Return(SlotID(0)));
		EXPECT_CALL(*this, creatureIndex()).WillRepeatedly(Return(0));
	}

	void setDefaultState()
	{
		EXPECT_CALL(*this, isClone()).WillRepeatedly(Return(false));
		EXPECT_CALL(*this, isCaster()).WillRepeatedly(Return(false));
		EXPECT_CALL(*this, acquireState()).WillRepeatedly(Return(state));
	}
private:
	BonusBearerMock bonusFake;
};

class UnitsFake
{
public:
	std::vector<std::shared_ptr<UnitFake>> allUnits;

	UnitFake & add(ui8 side)
	{
		UnitFake * unit = new UnitFake();
		EXPECT_CALL(*unit, unitSide()).WillRepeatedly(Return(side));
		unit->setDefaultExpectations();

		allUnits.emplace_back(unit);
		return *allUnits.back().get();
	}

	Units getUnitsIf(UnitFilter predicate) const
	{
		Units ret;

		for(auto & unit : allUnits)
		{
			if(predicate(unit.get()))
				ret.push_back(unit.get());
		}
		return ret;
	}

	void setDefaultBonusExpectations()
	{
		for(auto & unit : allUnits)
		{
			unit->redirectBonusesToFake();
			unit->expectAnyBonusSystemCall();
		}
	}
};

class CBattleInfoCallbackTest : public Test
{
public:
	class TestSubject : public CBattleInfoCallback
	{
	public:

		scripting::Pool * pool;

		TestSubject(scripting::Pool * p)
			: CBattleInfoCallback(),
			pool(p)
		{
		}

		void setBattle(const IBattleInfo * battleInfo)
		{
			CBattleInfoCallback::setBattle(battleInfo);
		}

		scripting::Pool * getContextPool() const override
		{
			return pool;
		}
	};

	StrictMock<scripting::PoolMock> pool;

	TestSubject subject;

	BattleStateMock battleMock;
	UnitsFake unitsFake;

	CBattleInfoCallbackTest()
		: pool(),
		subject(&pool)
	{

	}

	void startBattle()
	{
		subject.setBattle(&battleMock);
	}

	void redirectUnitsToFake()
	{
		ON_CALL(battleMock, getUnitsIf(_)).WillByDefault(Invoke(&unitsFake, &UnitsFake::getUnitsIf));
	}
};

class BattleFinishedTest : public CBattleInfoCallbackTest
{
public:
	void expectBattleNotFinished()
	{
		auto ret = subject.battleIsFinished();
		EXPECT_FALSE(ret);
	}

	void expectBattleDraw()
	{
		auto ret = subject.battleIsFinished();

		EXPECT_TRUE(ret);
		EXPECT_EQ(*ret, 2);
	}

	void expectBattleWinner(ui8 side)
	{
		auto ret = subject.battleIsFinished();

		EXPECT_TRUE(ret);
		EXPECT_EQ(*ret, side);
	}

	void expectBattleLooser(ui8 side)
	{
		auto ret = subject.battleIsFinished();

		EXPECT_TRUE(ret);
		EXPECT_NE(*ret, (int)side);
	}

	void setDefaultExpectations()
	{
		redirectUnitsToFake();
		unitsFake.setDefaultBonusExpectations();
		EXPECT_CALL(battleMock, getUnitsIf(_)).Times(1);
	}
};

TEST_F(BattleFinishedTest, NoBattleIsDraw)
{
	expectBattleDraw();
}

TEST_F(BattleFinishedTest, EmptyBattleIsDraw)
{
	setDefaultExpectations();
	startBattle();
	expectBattleDraw();
}

TEST_F(BattleFinishedTest, LastAliveUnitWins)
{
	UnitFake & unit = unitsFake.add(1);
	unit.makeAlive();
	unit.setDefaultState();

	setDefaultExpectations();
	startBattle();
	expectBattleWinner(1);
}

TEST_F(BattleFinishedTest, TwoUnitsContinueFight)
{
	UnitFake & unit1 = unitsFake.add(0);
	unit1.makeAlive();

	UnitFake & unit2 = unitsFake.add(1);
	unit2.makeAlive();

	setDefaultExpectations();
	startBattle();

	expectBattleNotFinished();
}

TEST_F(BattleFinishedTest, LastWarMachineNotWins)
{
	UnitFake & unit = unitsFake.add(0);
	unit.makeAlive();
	unit.makeWarMachine();
	unit.setDefaultState();

	setDefaultExpectations();
	startBattle();

	expectBattleLooser(0);
}

TEST_F(BattleFinishedTest, LastWarMachineLoose)
{
	try
	{
		UnitFake & unit1 = unitsFake.add(0);
		unit1.makeAlive();
		unit1.setDefaultState();

		UnitFake & unit2 = unitsFake.add(1);
		unit2.makeAlive();
		unit2.makeWarMachine();
		unit2.setDefaultState();

		setDefaultExpectations();
		startBattle();

		expectBattleWinner(0);
	}
	catch(std::exception e)
	{
		logGlobal->error(e.what());
	}
}

class BattleMatchOwnerTest : public CBattleInfoCallbackTest
{
public:
	void setDefaultExpectations()
	{
		redirectUnitsToFake();
		unitsFake.setDefaultBonusExpectations();

		EXPECT_CALL(battleMock, getSidePlayer(Eq(BattleSide::ATTACKER))).WillRepeatedly(Return(PlayerColor(0)));
		EXPECT_CALL(battleMock, getSidePlayer(Eq(BattleSide::DEFENDER))).WillRepeatedly(Return(PlayerColor(1)));
	}
};

TEST_F(BattleMatchOwnerTest, normalToSelf)
{
	UnitFake & unit1 = unitsFake.add(BattleSide::ATTACKER);
	EXPECT_CALL(unit1, unitId()).WillRepeatedly(Return(42));

	setDefaultExpectations();

	startBattle();

	EXPECT_TRUE(subject.battleMatchOwner(&unit1, &unit1, true));
	EXPECT_TRUE(subject.battleMatchOwner(&unit1, &unit1, boost::logic::indeterminate));
	EXPECT_FALSE(subject.battleMatchOwner(&unit1, &unit1, false));
}

TEST_F(BattleMatchOwnerTest, hypnotizedToSelf)
{
	UnitFake & unit1 = unitsFake.add(BattleSide::ATTACKER);
	EXPECT_CALL(unit1, unitId()).WillRepeatedly(Return(42));
	unit1.addNewBonus(std::make_shared<Bonus>(Bonus::PERMANENT, Bonus::HYPNOTIZED, Bonus::CREATURE_ABILITY, 0, 0));

	setDefaultExpectations();

	startBattle();

	EXPECT_TRUE(subject.battleMatchOwner(&unit1, &unit1, true));
	EXPECT_TRUE(subject.battleMatchOwner(&unit1, &unit1, boost::logic::indeterminate));
	EXPECT_FALSE(subject.battleMatchOwner(&unit1, &unit1, false));
}

TEST_F(BattleMatchOwnerTest, normalToNormalAlly)
{
	UnitFake & unit1 = unitsFake.add(BattleSide::ATTACKER);
	EXPECT_CALL(unit1, unitId()).WillRepeatedly(Return(42));
	UnitFake & unit2 = unitsFake.add(BattleSide::ATTACKER);
	EXPECT_CALL(unit2, unitId()).WillRepeatedly(Return(4242));

	setDefaultExpectations();

	startBattle();

	EXPECT_TRUE(subject.battleMatchOwner(&unit1, &unit2, true));
	EXPECT_TRUE(subject.battleMatchOwner(&unit1, &unit2, boost::logic::indeterminate));
	EXPECT_FALSE(subject.battleMatchOwner(&unit1, &unit2, false));
}

TEST_F(BattleMatchOwnerTest, hypnotizedToNormalAlly)
{
	UnitFake & unit1 = unitsFake.add(BattleSide::ATTACKER);
	EXPECT_CALL(unit1, unitId()).WillRepeatedly(Return(42));
	unit1.addNewBonus(std::make_shared<Bonus>(Bonus::PERMANENT, Bonus::HYPNOTIZED, Bonus::CREATURE_ABILITY, 0, 0));

	UnitFake & unit2 = unitsFake.add(BattleSide::ATTACKER);
	EXPECT_CALL(unit2, unitId()).WillRepeatedly(Return(4242));

	setDefaultExpectations();

	startBattle();

	EXPECT_FALSE(subject.battleMatchOwner(&unit1, &unit2, true));
	EXPECT_TRUE(subject.battleMatchOwner(&unit1, &unit2, boost::logic::indeterminate));
	EXPECT_TRUE(subject.battleMatchOwner(&unit1, &unit2, false));
}

TEST_F(BattleMatchOwnerTest, normalToHypnotizedAlly)
{
	UnitFake & unit1 = unitsFake.add(BattleSide::ATTACKER);
	EXPECT_CALL(unit1, unitId()).WillRepeatedly(Return(42));
	UnitFake & unit2 = unitsFake.add(BattleSide::ATTACKER);
	EXPECT_CALL(unit2, unitId()).WillRepeatedly(Return(4242));
	unit2.addNewBonus(std::make_shared<Bonus>(Bonus::PERMANENT, Bonus::HYPNOTIZED, Bonus::CREATURE_ABILITY, 0, 0));

	setDefaultExpectations();

	startBattle();

	EXPECT_TRUE(subject.battleMatchOwner(&unit1, &unit2, true));
	EXPECT_TRUE(subject.battleMatchOwner(&unit1, &unit2, boost::logic::indeterminate));
	EXPECT_FALSE(subject.battleMatchOwner(&unit1, &unit2, false));
}

TEST_F(BattleMatchOwnerTest, hypnotizedToHypnotizedAlly)
{
	UnitFake & unit1 = unitsFake.add(BattleSide::ATTACKER);
	EXPECT_CALL(unit1, unitId()).WillRepeatedly(Return(42));
	unit1.addNewBonus(std::make_shared<Bonus>(Bonus::PERMANENT, Bonus::HYPNOTIZED, Bonus::CREATURE_ABILITY, 0, 0));

	UnitFake & unit2 = unitsFake.add(BattleSide::ATTACKER);
	EXPECT_CALL(unit2, unitId()).WillRepeatedly(Return(4242));
	unit2.addNewBonus(std::make_shared<Bonus>(Bonus::PERMANENT, Bonus::HYPNOTIZED, Bonus::CREATURE_ABILITY, 0, 0));

	setDefaultExpectations();

	startBattle();

	EXPECT_FALSE(subject.battleMatchOwner(&unit1, &unit2, true));
	EXPECT_TRUE(subject.battleMatchOwner(&unit1, &unit2, boost::logic::indeterminate));
	EXPECT_TRUE(subject.battleMatchOwner(&unit1, &unit2, false));
}


TEST_F(BattleMatchOwnerTest, normalToNormalEnemy)
{
	UnitFake & unit1 = unitsFake.add(BattleSide::ATTACKER);
	EXPECT_CALL(unit1, unitId()).WillRepeatedly(Return(42));
	UnitFake & unit2 = unitsFake.add(BattleSide::DEFENDER);
	EXPECT_CALL(unit2, unitId()).WillRepeatedly(Return(4242));

	setDefaultExpectations();

	startBattle();

	EXPECT_FALSE(subject.battleMatchOwner(&unit1, &unit2, true));
	EXPECT_TRUE(subject.battleMatchOwner(&unit1, &unit2, boost::logic::indeterminate));
	EXPECT_TRUE(subject.battleMatchOwner(&unit1, &unit2, false));
}

TEST_F(BattleMatchOwnerTest, hypnotizedToNormalEnemy)
{
	UnitFake & unit1 = unitsFake.add(BattleSide::ATTACKER);
	EXPECT_CALL(unit1, unitId()).WillRepeatedly(Return(42));
	unit1.addNewBonus(std::make_shared<Bonus>(Bonus::PERMANENT, Bonus::HYPNOTIZED, Bonus::CREATURE_ABILITY, 0, 0));

	UnitFake & unit2 = unitsFake.add(BattleSide::DEFENDER);
	EXPECT_CALL(unit2, unitId()).WillRepeatedly(Return(4242));

	setDefaultExpectations();

	startBattle();

	EXPECT_TRUE(subject.battleMatchOwner(&unit1, &unit2, true));
	EXPECT_TRUE(subject.battleMatchOwner(&unit1, &unit2, boost::logic::indeterminate));
	EXPECT_FALSE(subject.battleMatchOwner(&unit1, &unit2, false));
}

TEST_F(BattleMatchOwnerTest, normalToHypnotizedEnemy)
{
	UnitFake & unit1 = unitsFake.add(BattleSide::ATTACKER);
	EXPECT_CALL(unit1, unitId()).WillRepeatedly(Return(42));
	UnitFake & unit2 = unitsFake.add(BattleSide::DEFENDER);
	EXPECT_CALL(unit2, unitId()).WillRepeatedly(Return(4242));
	unit2.addNewBonus(std::make_shared<Bonus>(Bonus::PERMANENT, Bonus::HYPNOTIZED, Bonus::CREATURE_ABILITY, 0, 0));

	setDefaultExpectations();

	startBattle();

	EXPECT_FALSE(subject.battleMatchOwner(&unit1, &unit2, true));
	EXPECT_TRUE(subject.battleMatchOwner(&unit1, &unit2, boost::logic::indeterminate));
	EXPECT_TRUE(subject.battleMatchOwner(&unit1, &unit2, false));
}

TEST_F(BattleMatchOwnerTest, hypnotizedToHypnotizedEnemy)
{
	UnitFake & unit1 = unitsFake.add(BattleSide::ATTACKER);
	EXPECT_CALL(unit1, unitId()).WillRepeatedly(Return(42));
	unit1.addNewBonus(std::make_shared<Bonus>(Bonus::PERMANENT, Bonus::HYPNOTIZED, Bonus::CREATURE_ABILITY, 0, 0));

	UnitFake & unit2 = unitsFake.add(BattleSide::DEFENDER);
	EXPECT_CALL(unit2, unitId()).WillRepeatedly(Return(4242));
	unit2.addNewBonus(std::make_shared<Bonus>(Bonus::PERMANENT, Bonus::HYPNOTIZED, Bonus::CREATURE_ABILITY, 0, 0));

	setDefaultExpectations();

	startBattle();

	EXPECT_TRUE(subject.battleMatchOwner(&unit1, &unit2, true));
	EXPECT_TRUE(subject.battleMatchOwner(&unit1, &unit2, boost::logic::indeterminate));
	EXPECT_FALSE(subject.battleMatchOwner(&unit1, &unit2, false));
}
