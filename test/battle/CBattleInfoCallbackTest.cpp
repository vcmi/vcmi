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

	void addCreatureAbility(BonusType bonusType)
	{
		addNewBonus(
			std::make_shared<Bonus>(
				BonusDuration::PERMANENT,
				bonusType,
				BonusSource::CREATURE_ABILITY,
				0,
				CreatureID(0)));
	}

	void makeAlive()
	{
		EXPECT_CALL(*this, alive()).WillRepeatedly(Return(true));
		EXPECT_CALL(*this, isValidTarget(_)).WillRepeatedly(Return(true));
	}

	void makeDead()
	{
		EXPECT_CALL(*this, alive()).WillRepeatedly(Return(false));
		EXPECT_CALL(*this, isGhost()).WillRepeatedly(Return(false));
		EXPECT_CALL(*this, isValidTarget(_)).WillRepeatedly(Return(false));
	}

	void setupPoisition(BattleHex pos)
	{
		EXPECT_CALL(*this, getPosition()).WillRepeatedly(Return(pos));
	}

	void makeDoubleWide()
	{
		EXPECT_CALL(*this, doubleWide()).WillRepeatedly(Return(true));
	}

	void makeWarMachine()
	{
		addNewBonus(std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::SIEGE_WEAPON, BonusSource::CREATURE_ABILITY, 1, BonusSourceID()));
	}

	bool isHypnotized() const override
	{
		return hasBonusOfType(BonusType::HYPNOTIZED);
	}

	bool isInvincible() const override
	{
		return hasBonusOfType(BonusType::INVINCIBLE);
	}

	void redirectBonusesToFake()
	{
		ON_CALL(*this, getAllBonuses(_, _)).WillByDefault(Invoke(&bonusFake, &BonusBearerMock::getAllBonuses));
		ON_CALL(*this, getTreeVersion()).WillByDefault(Invoke(&bonusFake, &BonusBearerMock::getTreeVersion));
	}

	void expectAnyBonusSystemCall()
	{
		EXPECT_CALL(*this, getAllBonuses(_, _)).Times(AtLeast(0));
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

	UnitFake & add(BattleSide side)
	{
		auto * unit = new UnitFake();
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

		const IBattleInfo * battle;
		scripting::Pool * pool;

		TestSubject(scripting::Pool * p)
			: CBattleInfoCallback(),
			pool(p)
		{
		}

		const IBattleInfo * getBattle() const override
		{
			return battle;
		}

		std::optional<PlayerColor> getPlayerID() const override
		{
			return std::nullopt;
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
		subject.battle = &battleMock;
	}

	void redirectUnitsToFake()
	{
		ON_CALL(battleMock, getUnitsIf(_)).WillByDefault(Invoke(&unitsFake, &UnitsFake::getUnitsIf));
	}
};

class AttackableHexesTest : public CBattleInfoCallbackTest
{
public:
	UnitFake & addRegularMelee(BattleHex hex, BattleSide side)
	{
		auto & unit = unitsFake.add(side);

		unit.makeAlive();
		unit.setDefaultState();
		unit.setupPoisition(hex);
		unit.redirectBonusesToFake();

		return unit;
	}

	UnitFake & addCerberi(BattleHex hex, BattleSide side)
	{
		auto & unit = addRegularMelee(hex, side);

		unit.addCreatureAbility(BonusType::THREE_HEADED_ATTACK);
		unit.makeDoubleWide();

		return unit;
	}

	UnitFake & addDragon(BattleHex hex, BattleSide side)
	{
		auto & unit = addRegularMelee(hex, side);

		unit.addCreatureAbility(BonusType::TWO_HEX_ATTACK_BREATH);
		unit.makeDoubleWide();

		return unit;
	}

	UnitFake & addLongWeaponUnit(BattleHex hex, BattleSide side)
	{
		auto & unit = addRegularMelee(hex, side);
		unit.addCreatureAbility(BonusType::LONG_WEAPON);
		return unit;
	}

	UnitFake & addLongWeaponDoubleWide(BattleHex hex, BattleSide side)
	{
		auto & unit = addLongWeaponUnit(hex, side);
		unit.makeDoubleWide();
		return unit;
	}

	Units getAttackedUnits(UnitFake & attacker, UnitFake & defender, BattleHex defenderHex)
	{
		startBattle();
		redirectUnitsToFake();

		return subject.getAttackedBattleUnits(
			&attacker, &defender,
			defenderHex, false,
			attacker.getPosition(),
			defender.getPosition());
	}
};

///// getAttackableHexes tests

TEST_F(AttackableHexesTest, getAttackableHexes_SingleWideAttacker_SingleWideDefender)
{
	UnitFake & attacker = addRegularMelee(60, BattleSide::ATTACKER);
	UnitFake & defender = addRegularMelee(90, BattleSide::DEFENDER);

	static const BattleHexArray expectedDef =
	{
		72,
		73,
		89,
		91,
		106,
		107
	};

	auto attackable = defender.getAttackableHexes(&attacker);
	attackable.sort([](const auto & l, const auto & r) { return l < r; });
	EXPECT_EQ(expectedDef, attackable);
}

TEST_F(AttackableHexesTest, getAttackableHexes_SingleWideAttacker_DoubleWideDefender)
{
	UnitFake & attacker = addRegularMelee(60, BattleSide::ATTACKER);
	UnitFake & defender = addDragon(90, BattleSide::DEFENDER);

	static const BattleHexArray expectedDef =
	{
		72,
		73,
		74,
		89,
		92,
		106,
		107,
		108
	};

	auto attackable = defender.getAttackableHexes(&attacker);
	attackable.sort([](const auto & l, const auto & r) { return l < r; });
	EXPECT_EQ(expectedDef, attackable);
}

TEST_F(AttackableHexesTest, getAttackableHexes_DoubleWideAttacker_SingleWideDefender)
{
	UnitFake & attacker = addDragon(60, BattleSide::ATTACKER);
	UnitFake & defender = addRegularMelee(90, BattleSide::DEFENDER);

	static const BattleHexArray expectedDef =
	{
		72,
		73,
		74,
		89,
		92,
		106,
		107,
		108
	};

	auto attackable = defender.getAttackableHexes(&attacker);
	attackable.sort([](const auto & l, const auto & r) { return l < r; });
	EXPECT_EQ(expectedDef, attackable);
}

TEST_F(AttackableHexesTest, getAttackableHexes_DoubleWideAttacker_DoubleWideDefender)
{
	UnitFake & attacker = addDragon(60, BattleSide::ATTACKER);
	UnitFake & defender = addDragon(90, BattleSide::DEFENDER);

	static const BattleHexArray expectedDef =
	{
		72,
		73,
		74,
		75,
		89,
		93,
		106,
		107,
		108,
		109
	};

	auto attackable = defender.getAttackableHexes(&attacker);
	attackable.sort([](const auto & l, const auto & r) { return l < r; });
	EXPECT_EQ(expectedDef, attackable);
}

TEST_F(AttackableHexesTest, LongWeaponCanAttackWithOneEmptyHexGap)
{
	UnitFake & attacker = addLongWeaponUnit(60, BattleSide::ATTACKER);
	UnitFake & defender = addRegularMelee(attacker.getPosition().cloneInDirection(BattleHex::RIGHT).cloneInDirection(BattleHex::RIGHT), BattleSide::DEFENDER);

	startBattle();
	redirectUnitsToFake();
	ON_CALL(battleMock, getAllObstacles()).WillByDefault(Return(IBattleInfo::ObstacleCList()));

	BattleHexArray availableHexes;
	availableHexes.insert(attacker.getPosition());

	EXPECT_EQ(subject.fromWhichHexAttack(&attacker, defender.getPosition(), BattleHex::LEFT), attacker.getPosition());
	EXPECT_TRUE(subject.battleCanAttackHex(availableHexes, &attacker, defender.getPosition(), BattleHex::LEFT));
}

TEST_F(AttackableHexesTest, LongWeaponCanBeDisabledForManualAttack)
{
	UnitFake & attacker = addLongWeaponUnit(60, BattleSide::ATTACKER);
	UnitFake & defender = addRegularMelee(attacker.getPosition().cloneInDirection(BattleHex::RIGHT).cloneInDirection(BattleHex::RIGHT), BattleSide::DEFENDER);

	startBattle();
	redirectUnitsToFake();

	const BattleHex adjacentAttackHex = defender.getPosition().cloneInDirection(BattleHex::LEFT);
	EXPECT_EQ(subject.fromWhichHexAttack(&attacker, defender.getPosition(), BattleHex::LEFT, false), adjacentAttackHex);
}

TEST_F(AttackableHexesTest, LongWeaponRequiresEmptyMiddleHex)
{
	UnitFake & attacker = addLongWeaponUnit(60, BattleSide::ATTACKER);
	const BattleHex middleHex = attacker.getPosition().cloneInDirection(BattleHex::RIGHT);
	UnitFake & blocker = addRegularMelee(middleHex, BattleSide::ATTACKER);
	UnitFake & defender = addRegularMelee(middleHex.cloneInDirection(BattleHex::RIGHT), BattleSide::DEFENDER);
	(void)blocker;

	startBattle();
	redirectUnitsToFake();
	ON_CALL(battleMock, getAllObstacles()).WillByDefault(Return(IBattleInfo::ObstacleCList()));

	BattleHexArray availableHexes;
	availableHexes.insert(attacker.getPosition());

	EXPECT_FALSE(subject.battleCanAttackHex(availableHexes, &attacker, defender.getPosition(), BattleHex::LEFT));
}

TEST_F(AttackableHexesTest, LongWeaponCanAttackOverCorpseInMiddleHex)
{
	UnitFake & attacker = addLongWeaponUnit(60, BattleSide::ATTACKER);
	const BattleHex middleHex = attacker.getPosition().cloneInDirection(BattleHex::RIGHT);
	UnitFake & corpse = addRegularMelee(middleHex, BattleSide::ATTACKER);
	UnitFake & defender = addRegularMelee(middleHex.cloneInDirection(BattleHex::RIGHT), BattleSide::DEFENDER);
	corpse.makeDead();

	startBattle();
	redirectUnitsToFake();
	ON_CALL(battleMock, getAllObstacles()).WillByDefault(Return(IBattleInfo::ObstacleCList()));

	BattleHexArray availableHexes;
	availableHexes.insert(attacker.getPosition());

	EXPECT_TRUE(subject.battleCanAttackHex(availableHexes, &attacker, defender.getPosition(), BattleHex::LEFT));
}

TEST_F(AttackableHexesTest, LongWeaponDoubleWideCanAttackWithOneEmptyHexGap)
{
	UnitFake & attacker = addLongWeaponDoubleWide(60, BattleSide::ATTACKER);
	UnitFake & defender = addRegularMelee(attacker.getPosition().cloneInDirection(BattleHex::RIGHT).cloneInDirection(BattleHex::RIGHT), BattleSide::DEFENDER);

	startBattle();
	redirectUnitsToFake();
	ON_CALL(battleMock, getAllObstacles()).WillByDefault(Return(IBattleInfo::ObstacleCList()));

	BattleHexArray availableHexes;
	availableHexes.insert(attacker.getPosition());

	EXPECT_EQ(subject.fromWhichHexAttack(&attacker, defender.getPosition(), BattleHex::LEFT), attacker.getPosition());
	EXPECT_TRUE(subject.battleCanAttackHex(availableHexes, &attacker, defender.getPosition(), BattleHex::LEFT));
}

TEST_F(AttackableHexesTest, LongWeaponDoubleWideFallsBackToAdjacentReachableHexWhenLongHexIsUnavailable)
{
	UnitFake & attacker = addLongWeaponDoubleWide(60, BattleSide::ATTACKER);
	UnitFake & defender = addRegularMelee(attacker.getPosition().cloneInDirection(BattleHex::RIGHT).cloneInDirection(BattleHex::RIGHT).cloneInDirection(BattleHex::RIGHT), BattleSide::DEFENDER);

	startBattle();
	redirectUnitsToFake();
	ON_CALL(battleMock, getAllObstacles()).WillByDefault(Return(IBattleInfo::ObstacleCList()));

	const BattleHex expectedAttackHex = defender.getPosition().cloneInDirection(BattleHex::LEFT);
	EXPECT_EQ(subject.fromWhichHexAttack(&attacker, defender.getPosition(), BattleHex::LEFT), expectedAttackHex);
}

TEST_F(AttackableHexesTest, LongWeaponSingleWideInvalidDirectionReturnsInvalidHex)
{
	UnitFake & attacker = addLongWeaponUnit(60, BattleSide::ATTACKER);

	startBattle();
	redirectUnitsToFake();

	EXPECT_NO_THROW(
	{
		const BattleHex attackFrom = subject.fromWhichHexAttack(&attacker, BattleHex(0), BattleHex::LEFT);
		EXPECT_EQ(attackFrom, BattleHex::INVALID);
	});
}

TEST_F(AttackableHexesTest, LongWeaponSingleWideGetAttackedUnitsFromGapThrows)
{
	UnitFake & attacker = addLongWeaponUnit(60, BattleSide::ATTACKER);
	UnitFake & defender = addRegularMelee(attacker.getPosition().cloneInDirection(BattleHex::RIGHT).cloneInDirection(BattleHex::RIGHT), BattleSide::DEFENDER);

	EXPECT_THROW(getAttackedUnits(attacker, defender, defender.getPosition()), std::runtime_error);
}

//// CERBERI 3-HEADED ATTACKS

TEST_F(AttackableHexesTest, CerberiAttackerRight)
{
	//    #
	// X A D
	//    #
	UnitFake & attacker = addCerberi(35, BattleSide::ATTACKER);
	UnitFake & defender = addRegularMelee(attacker.getPosition().cloneInDirection(BattleHex::RIGHT), BattleSide::DEFENDER);
	UnitFake & right = addRegularMelee(attacker.getPosition().cloneInDirection(BattleHex::BOTTOM_RIGHT), BattleSide::DEFENDER);
	UnitFake & left = addRegularMelee(attacker.getPosition().cloneInDirection(BattleHex::TOP_RIGHT), BattleSide::DEFENDER);

	auto attacked = getAttackedUnits(attacker, defender, defender.getPosition());

	EXPECT_TRUE(vstd::contains(attacked, &right));
	EXPECT_TRUE(vstd::contains(attacked, &left));
}

TEST_F(AttackableHexesTest, CerberiAttackerTopRight)
{
	//  # D
	// X A #
	//
	UnitFake & attacker = addCerberi(35, BattleSide::ATTACKER);
	UnitFake & defender = addRegularMelee(attacker.getPosition().cloneInDirection(BattleHex::TOP_RIGHT), BattleSide::DEFENDER);
	UnitFake & right = addRegularMelee(attacker.getPosition().cloneInDirection(BattleHex::RIGHT), BattleSide::DEFENDER);
	UnitFake & left = addRegularMelee(attacker.getPosition().cloneInDirection(BattleHex::TOP_LEFT), BattleSide::DEFENDER);

	auto attacked = getAttackedUnits(attacker, defender, defender.getPosition());

	EXPECT_TRUE(vstd::contains(attacked, &right));
	EXPECT_TRUE(vstd::contains(attacked, &left));
}

TEST_F(AttackableHexesTest, CerberiAttackerTopMiddle)
{
	// # D #
	//  X A
	//
	UnitFake & attacker = addCerberi(35, BattleSide::ATTACKER);
	UnitFake & defender = addRegularMelee(attacker.getPosition().cloneInDirection(BattleHex::TOP_LEFT), BattleSide::DEFENDER);
	UnitFake & right = addRegularMelee(attacker.getPosition().cloneInDirection(BattleHex::TOP_RIGHT), BattleSide::DEFENDER);
	UnitFake & left = addRegularMelee(attacker.occupiedHex().cloneInDirection(BattleHex::TOP_LEFT), BattleSide::DEFENDER);

	auto attacked = getAttackedUnits(attacker, defender, defender.getPosition());

	EXPECT_TRUE(vstd::contains(attacked, &right));
	EXPECT_TRUE(vstd::contains(attacked, &left));
}

TEST_F(AttackableHexesTest, CerberiAttackerTopLeft)
{
	//  D #
	// # X A
	//
	UnitFake & attacker = addCerberi(40, BattleSide::ATTACKER);
	UnitFake & defender = addRegularMelee(attacker.occupiedHex().cloneInDirection(BattleHex::TOP_LEFT), BattleSide::DEFENDER);
	UnitFake & right = addRegularMelee(attacker.occupiedHex().cloneInDirection(BattleHex::TOP_RIGHT), BattleSide::DEFENDER);
	UnitFake & left = addRegularMelee(attacker.occupiedHex().cloneInDirection(BattleHex::LEFT), BattleSide::DEFENDER);

	auto attacked = getAttackedUnits(attacker, defender, defender.getPosition());

	EXPECT_TRUE(vstd::contains(attacked, &right));
	EXPECT_TRUE(vstd::contains(attacked, &left));
}

TEST_F(AttackableHexesTest, CerberiAttackerLeft)
{
	//  #
	// D X A
	//  #
	UnitFake & attacker = addCerberi(40, BattleSide::ATTACKER);
	UnitFake & defender = addRegularMelee(attacker.occupiedHex().cloneInDirection(BattleHex::LEFT), BattleSide::DEFENDER);
	UnitFake & right = addRegularMelee(attacker.occupiedHex().cloneInDirection(BattleHex::TOP_LEFT), BattleSide::DEFENDER);
	UnitFake & left = addRegularMelee(attacker.occupiedHex().cloneInDirection(BattleHex::BOTTOM_LEFT), BattleSide::DEFENDER);

	auto attacked = getAttackedUnits(attacker, defender, defender.getPosition());

	EXPECT_TRUE(vstd::contains(attacked, &right));
	EXPECT_TRUE(vstd::contains(attacked, &left));
}

//// DRAGON BREATH AS ATTACKER

TEST_F(AttackableHexesTest, DragonRightRegular_RightHorithontalBreath)
{
	// X A D #
	UnitFake & attacker = addDragon(35, BattleSide::ATTACKER);
	UnitFake & defender = addRegularMelee(36, BattleSide::DEFENDER);
	UnitFake & next = addRegularMelee(37, BattleSide::DEFENDER);

	auto attacked = getAttackedUnits(attacker, defender, defender.getPosition());

	EXPECT_TRUE(vstd::contains(attacked, &next));
}

TEST_F(AttackableHexesTest, DragonDragonBottomRightHead_BottomRightBreathFromHead)
{
	// X A
	//    D X		target D
	//     #
	UnitFake & attacker = addDragon(35, BattleSide::ATTACKER);
	UnitFake & defender = addDragon(attacker.getPosition().cloneInDirection(BattleHex::BOTTOM_RIGHT), BattleSide::DEFENDER);
	UnitFake & next = addRegularMelee(defender.getPosition().cloneInDirection(BattleHex::BOTTOM_RIGHT), BattleSide::DEFENDER);

	auto attacked = getAttackedUnits(attacker, defender, defender.getPosition());

	EXPECT_TRUE(vstd::contains(attacked, &next));
}

TEST_F(AttackableHexesTest, DragonDragonVerticalDownHead_VerticalDownBreathFromHead)
{
	// X A
	//  D X		target D
	//     #
	UnitFake & attacker = addDragon(35, BattleSide::ATTACKER);
	UnitFake & defender = addDragon(attacker.getPosition().cloneInDirection(BattleHex::BOTTOM_LEFT), BattleSide::DEFENDER);
	UnitFake & next = addRegularMelee(defender.occupiedHex().cloneInDirection(BattleHex::BOTTOM_RIGHT), BattleSide::DEFENDER);

	auto attacked = getAttackedUnits(attacker, defender, defender.getPosition());

	EXPECT_TRUE(vstd::contains(attacked, &next));
}

TEST_F(AttackableHexesTest, DragonDragonVerticalDownHead_VerticalRightBreathFromHead)
{
	// X A
	//  D X		target X
	//     #
	UnitFake & attacker = addDragon(35, BattleSide::ATTACKER);
	UnitFake & defender = addDragon(attacker.getPosition().cloneInDirection(BattleHex::BOTTOM_LEFT), BattleSide::DEFENDER);
	UnitFake & next = addRegularMelee(defender.occupiedHex().cloneInDirection(BattleHex::BOTTOM_RIGHT), BattleSide::DEFENDER);

	auto attacked = getAttackedUnits(attacker, defender, defender.occupiedHex());

	EXPECT_TRUE(vstd::contains(attacked, &next));
}

TEST_F(AttackableHexesTest, DragonDragonVerticalDownBack_VerticalDownBreath)
{
	//  X A
	// D X		target X
	//  #
	UnitFake & attacker = addDragon(37, BattleSide::ATTACKER);
	UnitFake & defender = addDragon(attacker.occupiedHex().cloneInDirection(BattleHex::BOTTOM_LEFT), BattleSide::DEFENDER);
	UnitFake & next = addRegularMelee(defender.getPosition().cloneInDirection(BattleHex::BOTTOM_RIGHT), BattleSide::DEFENDER);

	auto attacked = getAttackedUnits(attacker, defender, defender.occupiedHex());

	EXPECT_TRUE(vstd::contains(attacked, &next));
}

TEST_F(AttackableHexesTest, DragonDragonHeadBottomRight_BottomRightBreathFromHead)
{
	//  X A
	// D X		target D
	//  #
	UnitFake & attacker = addDragon(37, BattleSide::ATTACKER);
	UnitFake & defender = addDragon(attacker.occupiedHex().cloneInDirection(BattleHex::BOTTOM_LEFT), BattleSide::DEFENDER);
	UnitFake & next = addRegularMelee(defender.getPosition().cloneInDirection(BattleHex::BOTTOM_RIGHT), BattleSide::DEFENDER);

	auto attacked = getAttackedUnits(attacker, defender, defender.getPosition());

	EXPECT_TRUE(vstd::contains(attacked, &next));
}

TEST_F(AttackableHexesTest, DragonLeftBottomDragonBackToBack_LeftBottomBreathFromBackHex)
{
	//    X A
	// D X		target X
	//  #
	UnitFake & attacker = addDragon(8, BattleSide::ATTACKER);
	UnitFake & defender = addDragon(attacker.occupiedHex().cloneInDirection(BattleHex::BOTTOM_LEFT).cloneInDirection(BattleHex::LEFT), BattleSide::DEFENDER);
	UnitFake & next = addRegularMelee(defender.getPosition().cloneInDirection(BattleHex::BOTTOM_RIGHT), BattleSide::DEFENDER);

	auto attacked = getAttackedUnits(attacker, defender, defender.occupiedHex());

	EXPECT_TRUE(vstd::contains(attacked, &next));
}

//// DRAGON BREATH AS DEFENDER

TEST_F(AttackableHexesTest, DragonDragonVerticalDownHeadReverse_VerticalDownBreathFromHead)
{
	//   A X
	//  X D		target D
	// #
	UnitFake & attacker = addDragon(36, BattleSide::DEFENDER);
	UnitFake & defender = addDragon(attacker.getPosition().cloneInDirection(BattleHex::BOTTOM_RIGHT), BattleSide::ATTACKER);
	UnitFake & next = addRegularMelee(defender.occupiedHex().cloneInDirection(BattleHex::BOTTOM_LEFT), BattleSide::ATTACKER);

	auto attacked = getAttackedUnits(attacker, defender, defender.getPosition());

	EXPECT_TRUE(vstd::contains(attacked, &next));
}

TEST_F(AttackableHexesTest, DragonVerticalDownDragonBackReverse_VerticalDownBreath)
{
	// A X
	//  X D		target X
	//   #
	UnitFake & attacker = addDragon(36, BattleSide::DEFENDER);
	UnitFake & defender = addDragon(attacker.occupiedHex().cloneInDirection(BattleHex::BOTTOM_RIGHT), BattleSide::ATTACKER);
	UnitFake & next = addRegularMelee(defender.getPosition().cloneInDirection(BattleHex::BOTTOM_LEFT), BattleSide::ATTACKER);

	auto attacked = getAttackedUnits(attacker, defender, defender.occupiedHex());

	EXPECT_TRUE(vstd::contains(attacked, &next));
}

TEST_F(AttackableHexesTest, DragonRightBottomDragonHeadReverse_RightBottomBreathFromHeadHex)
{
	// A X
	//  X D		target D
	UnitFake & attacker = addDragon(36, BattleSide::DEFENDER);
	UnitFake & defender = addDragon(attacker.occupiedHex().cloneInDirection(BattleHex::BOTTOM_RIGHT), BattleSide::ATTACKER);
	UnitFake & next = addRegularMelee(defender.getPosition().cloneInDirection(BattleHex::BOTTOM_LEFT), BattleSide::ATTACKER);

	auto attacked = getAttackedUnits(attacker, defender, defender.getPosition());

	EXPECT_TRUE(vstd::contains(attacked, &next));
}

TEST_F(AttackableHexesTest, DefenderPositionOverride_BreathCountsHypoteticDefenderPosition)
{
	// # N
	//  X D		target D
	//   A X
	UnitFake & attacker = addDragon(35, BattleSide::DEFENDER);
	UnitFake & defender = addDragon(8, BattleSide::ATTACKER);
	UnitFake & next = addDragon(1, BattleSide::ATTACKER);

	startBattle();
	redirectUnitsToFake();

	auto attacked = subject.getAttackedBattleUnits(
		&attacker,
		&defender,
		19,
		false,
		attacker.getPosition(),
		19);

	EXPECT_TRUE(vstd::contains(attacked, &next));
}

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
		EXPECT_EQ(*ret, BattleSide::NONE);
	}

	void expectBattleWinner(BattleSide side)
	{
		auto ret = subject.battleIsFinished();

		EXPECT_TRUE(ret);
		EXPECT_EQ(*ret, side);
	}

	void expectBattleLooser(BattleSide side)
	{
		auto ret = subject.battleIsFinished();

		EXPECT_TRUE(ret);
		EXPECT_NE(*ret, side);
	}

	void setDefaultExpectations()
	{
		redirectUnitsToFake();
		unitsFake.setDefaultBonusExpectations();
		EXPECT_CALL(battleMock, getUnitsIf(_)).Times(1);
	}
};

TEST_F(BattleFinishedTest, DISABLED_NoBattleIsDraw)
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
	UnitFake & unit = unitsFake.add(BattleSide::DEFENDER);
	unit.makeAlive();
	unit.setDefaultState();

	setDefaultExpectations();
	startBattle();
	expectBattleWinner(BattleSide::DEFENDER);
}

TEST_F(BattleFinishedTest, TwoUnitsContinueFight)
{
	UnitFake & unit1 = unitsFake.add(BattleSide::ATTACKER);
	unit1.makeAlive();

	UnitFake & unit2 = unitsFake.add(BattleSide::DEFENDER);
	unit2.makeAlive();

	setDefaultExpectations();
	startBattle();

	expectBattleNotFinished();
}

TEST_F(BattleFinishedTest, LastWarMachineNotWins)
{
	UnitFake & unit = unitsFake.add(BattleSide::ATTACKER);
	unit.makeAlive();
	unit.makeWarMachine();
	unit.setDefaultState();

	setDefaultExpectations();
	startBattle();

	expectBattleLooser(BattleSide::ATTACKER);
}

TEST_F(BattleFinishedTest, LastWarMachineLoose)
{
	try
	{
		UnitFake & unit1 = unitsFake.add(BattleSide::ATTACKER);
		unit1.makeAlive();
		unit1.setDefaultState();

		UnitFake & unit2 = unitsFake.add(BattleSide::DEFENDER);
		unit2.makeAlive();
		unit2.makeWarMachine();
		unit2.setDefaultState();

		setDefaultExpectations();
		startBattle();

		expectBattleWinner(BattleSide::ATTACKER);
	}
	catch(const std::exception & e)
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
	unit1.addNewBonus(std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::HYPNOTIZED, BonusSource::CREATURE_ABILITY, 0, BonusSourceID()));

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
	unit1.addNewBonus(std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::HYPNOTIZED, BonusSource::CREATURE_ABILITY, 0, BonusSourceID()));

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
	unit2.addNewBonus(std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::HYPNOTIZED, BonusSource::CREATURE_ABILITY, 0, BonusSourceID()));

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
	unit1.addNewBonus(std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::HYPNOTIZED, BonusSource::CREATURE_ABILITY, 0, BonusSourceID()));

	UnitFake & unit2 = unitsFake.add(BattleSide::ATTACKER);
	EXPECT_CALL(unit2, unitId()).WillRepeatedly(Return(4242));
	unit2.addNewBonus(std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::HYPNOTIZED, BonusSource::CREATURE_ABILITY, 0, BonusSourceID()));

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
	unit1.addNewBonus(std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::HYPNOTIZED, BonusSource::CREATURE_ABILITY, 0, BonusSourceID()));

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
	unit2.addNewBonus(std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::HYPNOTIZED, BonusSource::CREATURE_ABILITY, 0, BonusSourceID()));

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
	unit1.addNewBonus(std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::HYPNOTIZED, BonusSource::CREATURE_ABILITY, 0, BonusSourceID()));

	UnitFake & unit2 = unitsFake.add(BattleSide::DEFENDER);
	EXPECT_CALL(unit2, unitId()).WillRepeatedly(Return(4242));
	unit2.addNewBonus(std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::HYPNOTIZED, BonusSource::CREATURE_ABILITY, 0, BonusSourceID()));

	setDefaultExpectations();

	startBattle();

	EXPECT_TRUE(subject.battleMatchOwner(&unit1, &unit2, true));
	EXPECT_TRUE(subject.battleMatchOwner(&unit1, &unit2, boost::logic::indeterminate));
	EXPECT_FALSE(subject.battleMatchOwner(&unit1, &unit2, false));
}
