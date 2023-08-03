/*
 * BattleFake.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include <vstd/RNG.h>

#include "mock_BonusBearer.h"
#include "mock_battle_IBattleState.h"
#include "mock_battle_Unit.h"
#if SCRIPTING_ENABLED
#include "mock_scripting_Pool.h"
#endif

#include "../../lib/JsonNode.h"
#include "../../lib/NetPacksBase.h"
#include "../../lib/battle/CBattleInfoCallback.h"

namespace test
{
namespace battle
{
using namespace ::testing;

using ::battle::Unit;
using ::battle::Units;
using ::battle::UnitFilter;

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
#if SCRIPTING_ENABLED
	std::shared_ptr<scripting::PoolMock> pool;
#endif
public:
#if SCRIPTING_ENABLED
	BattleFake(std::shared_ptr<scripting::PoolMock> pool_);
#else
	BattleFake();
#endif
	virtual ~BattleFake();

	void setUp();

	void setupEmptyBattlefield();

#if SCRIPTING_ENABLED
	scripting::Pool * getContextPool() const override;
#endif

	template <typename T>
	void accept(T * pack)
	{
		pack->applyBattle(this);
	}

protected:

private:
};

}
}
