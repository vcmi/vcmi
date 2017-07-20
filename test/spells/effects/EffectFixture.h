/*
 * EffectFixture.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "../../../lib/spells/effects/Effect.h"

#include "mock/mock_spells_Mechanics.h"
#include "mock/mock_spells_Problem.h"

#include "mock/mock_BonusBearer.h"
#include "mock/mock_battle_IBattleState.h"
#include "mock/mock_battle_Unit.h"
#include "mock/mock_vstd_RNG.h"


#include "../../../lib/JsonNode.h"
#include "../../../lib/NetPacksBase.h"
#include "../../../lib/battle/CBattleInfoCallback.h"

namespace battle
{
	bool operator== (const Destination & left, const Destination & right);
}

namespace test
{

class EffectFixture
{
public:

	class UnitFake : public UnitMock
	{
	public:
		void addNewBonus(const std::shared_ptr<Bonus> & b);

		void makeAlive();
		void makeDead();

		void redirectBonusesToFake();

		void expectAnyBonusSystemCall();

	private:
		BonusBearerMock bonusFake;
	};

	class UnitsFake
	{
	public:
		std::vector<std::shared_ptr<UnitFake>> allUnits;

		UnitFake & add(ui8 side);

		battle::Units getUnitsIf(battle::UnitFilter predicate) const;

		void setDefaultBonusExpectations();
	};

	class BattleFake : public CBattleInfoCallback, public BattleStateMock
	{
	public:
		BattleFake();

		void setUp();
	};

	std::shared_ptr<::spells::effects::Effect> subject;
	::spells::ProblemMock problemMock;
	::spells::MechanicsMock mechanicsMock;
	vstd::RNGMock rngMock;

	UnitsFake unitsFake;
	std::shared_ptr<BattleFake> battleFake;

	std::shared_ptr<::spells::BattleStateProxy> battleProxy;

	std::string effectName;

	EffectFixture(std::string effectName_);
	virtual ~EffectFixture();

	void setupEffect(const JsonNode & effectConfig);

	void setupDefaultRNG();

	void setupEmptyBattlefield();

protected:
	void setUp();

private:
};

}
