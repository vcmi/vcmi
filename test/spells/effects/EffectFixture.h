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
#include "../../../lib/spells/effects/Registry.h"

#include "../../mock/mock_spells_Mechanics.h"
#include "../../mock/mock_spells_Problem.h"
#include "../../mock/mock_spells_Spell.h"
#include "../../mock/mock_spells_SpellService.h"
#include "../../mock/mock_IGameInfoCallback.h"

#include "../../mock/mock_Creature.h"
#include "../../mock/mock_CreatureService.h"

#include "../../mock/mock_BonusBearer.h"
#include "../../mock/mock_battle_IBattleState.h"
#include "../../mock/mock_battle_Unit.h"
#include "../../mock/mock_vstd_RNG.h"
#if SCRIPTING_ENABLED
#include "../../mock/mock_scripting_Pool.h"
#endif
#include "../../mock/BattleFake.h"
#include "../../mock/mock_ServerCallback.h"


#include "../../../lib/JsonNode.h"
#include "../../../lib/NetPacksBase.h"
#include "../../../lib/battle/CBattleInfoCallback.h"

namespace battle
{
	bool operator== (const Destination & left, const Destination & right);
}

bool operator==(const Bonus & b1, const Bonus & b2);

namespace test
{

using namespace ::testing;
using namespace ::spells;
using namespace ::spells::effects;
#if SCRIPTING_ENABLED
using namespace ::scripting;
#endif

class EffectFixture
{
public:
	std::shared_ptr<Effect> subject;
	ProblemMock problemMock;
	StrictMock<MechanicsMock> mechanicsMock;
	StrictMock<CreatureServiceMock> creatureServiceMock;
	StrictMock<CreatureMock> creatureStub;
	StrictMock<SpellServiceMock> spellServiceMock;
	StrictMock<SpellMock> spellStub;
	StrictMock<IGameInfoCallbackMock> gameMock;
	vstd::RNGMock rngMock;

	battle::UnitsFake unitsFake;

#if SCRIPTING_ENABLED
	std::shared_ptr<PoolMock> pool;
#endif
	std::shared_ptr<battle::BattleFake> battleFake;

	StrictMock<ServerCallbackMock> serverMock;

	std::string effectName;

	EffectFixture(std::string effectName_);
	virtual ~EffectFixture();

	void setupEffect(const JsonNode & effectConfig);
	void setupEffect(Registry * registry, const JsonNode & effectConfig);

	void setupDefaultRNG();

protected:
	void setUp();

private:
};

}
