/*
 * mock_CStack.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "../../lib/CStack.h"
#include "mock_BonusBearer.h"

namespace test
{

class MockCStack : public CStack
{
public:
	BattleSide mockedSide = BattleSide::ATTACKER;
	BonusBearerMock bonusFake;

	MockCStack() : CStack() {}

	void makeNativeOnSide(BattleSide s)
	{
		mockedSide = s;
		bonusFake.addNewBonus(std::make_shared<Bonus>(
			BonusDuration::PERMANENT,
			BonusType::TERRAIN_NATIVE,
			BonusSource::CREATURE_ABILITY,
			1,
			BonusSourceID()));
	}

	BattleSide unitSide() const override { return mockedSide; }
	bool isGhost() const override { return false; }
	int32_t creatureIndex() const override { return 0; }  // not ARROW_TOWERS

	const IBonusBearer * getBonusBearer() const override { return &bonusFake; }
};

}
