/*
 * BattleFake.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "BattleFake.h"

namespace test
{
namespace battle
{

void UnitFake::addNewBonus(const std::shared_ptr<Bonus> & b)
{
	bonusFake.addNewBonus(b);
}

void UnitFake::makeAlive()
{
	EXPECT_CALL(*this, alive()).Times(AtLeast(1)).WillRepeatedly(Return(true));
}

void UnitFake::makeDead()
{
	EXPECT_CALL(*this, alive()).Times(AtLeast(1)).WillRepeatedly(Return(false));
	EXPECT_CALL(*this, isGhost()).Times(AtLeast(1)).WillRepeatedly(Return(false));
}

void UnitFake::redirectBonusesToFake()
{
	ON_CALL(*this, getAllBonuses(_, _, _, _)).WillByDefault(Invoke(&bonusFake, &BonusBearerMock::getAllBonuses));
	ON_CALL(*this, getTreeVersion()).WillByDefault(Invoke(&bonusFake, &BonusBearerMock::getTreeVersion));
}

void UnitFake::expectAnyBonusSystemCall()
{
	EXPECT_CALL(*this, getAllBonuses(_, _, _, _)).Times(AtLeast(0));
	EXPECT_CALL(*this, getTreeVersion()).Times(AtLeast(0));
}

UnitFake & UnitsFake::add(ui8 side)
{
	UnitFake * unit = new UnitFake();
	ON_CALL(*unit, unitSide()).WillByDefault(Return(side));
	unit->redirectBonusesToFake();

	allUnits.emplace_back(unit);
	return *allUnits.back().get();
}

Units UnitsFake::getUnitsIf(UnitFilter predicate) const
{
	Units ret;

	for(auto & unit : allUnits)
	{
		if(predicate(unit.get()))
			ret.push_back(unit.get());
	}
	return ret;
}

void UnitsFake::setDefaultBonusExpectations()
{
	for(auto & unit : allUnits)
	{
		unit->expectAnyBonusSystemCall();
	}
}

BattleFake::~BattleFake() = default;

BattleFake::BattleFake()
	: CBattleInfoCallback(),
	BattleStateMock()
{
}

void BattleFake::setUp()
{
	CBattleInfoCallback::setBattle(this);
}

void BattleFake::setupEmptyBattlefield()
{
	EXPECT_CALL(*this, getDefendedTown()).WillRepeatedly(Return(nullptr));
	EXPECT_CALL(*this, getAllObstacles()).WillRepeatedly(Return(IBattleInfo::ObstacleCList()));
	EXPECT_CALL(*this, getBattlefieldType()).WillRepeatedly(Return(BattleField::fromString("grass_hills")));
}


}
}
