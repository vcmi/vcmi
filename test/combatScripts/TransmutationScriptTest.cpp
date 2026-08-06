/*
 * TransmutationScriptTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "../spells/effects/EffectFixture.h"

#include "../../lib/GameLibrary.h"
#include "../../lib/battle/Unit.h"
#include "../../lib/combatScripts/CombatScriptService.h"
#include "../../lib/combatScripts/CombatEventPayload.h"
#include "../../lib/combatScripts/ICombatEventScript.h"
#include "../../lib/json/JsonNode.h"
#include "../../lib/modding/IdentifierStorage.h"
#include "../../lib/modding/ModScope.h"
#include "../../lib/networkPacks/PacksForClientBattle.h"

namespace test
{
using namespace ::testing;

/// Covers the built-in transmutation combat script - the scripted equivalent of the
/// TRANSMUTATION bonus. Nothing applies the packs back onto the battle, so the victim stays
/// readable after the script removed it.
class TransmutationScriptTest : public Test, public EffectFixture
{
public:
	static constexpr int32_t chance = 20;
	static constexpr int32_t victimCount = 10;
	static constexpr uint32_t newUnitId = 77;
	const BattleHex victimPosition = BattleHex(50);
	const CreatureID resultingCreature = CreatureID(42);

	TransmutationScriptTest()
		: EffectFixture("core:damage") // unused, this test drives a combat script instead
	{
	}

	std::shared_ptr<ICombatEventScript> script;

	JsonNode parameters()
	{
		JsonNode result;
		result["chance"].Integer() = chance;
		result["creature"].String() = "core:goldGolem";
		result["transmuteBy"].String() = "count";
		return result;
	}

	/// Living victim with no immunity - the case in which transmutation is allowed to trigger
	battle::UnitFake & addVictim()
	{
		auto & victim = unitsFake.add(BattleSide::DEFENDER);
		victim.makeAlive();
		victim.addNewBonus(std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::LIVING, BonusSource::CREATURE_ABILITY, 1, BonusSourceID()));
		EXPECT_CALL(victim, creatureId()).WillRepeatedly(Return(CreatureID(0)));
		EXPECT_CALL(victim, getCount()).WillRepeatedly(Return(victimCount));
		EXPECT_CALL(victim, getPosition()).WillRepeatedly(Return(victimPosition));
		return victim;
	}

	void expectCreatureLookup()
	{
		EXPECT_CALL(creatureServiceMock, getByName("core:goldGolem")).WillRepeatedly(Return(&creatureStub));
		EXPECT_CALL(creatureStub, getId()).WillRepeatedly(Return(resultingCreature));
		EXPECT_CALL(creatureStub, getJsonKey()).WillRepeatedly(Return("core:goldGolem"));
	}

protected:
	void SetUp() override
	{
		EffectFixture::setUp();

		auto index = LIBRARY->identifiers()->getIdentifier(ModScope::scopeGame(), "combatScript", "transmutation", false);
		ASSERT_TRUE(index.has_value());

		script = LIBRARY->combatScripts()->get(CombatScriptID(*index));
		ASSERT_TRUE(script);
	}
};

TEST_F(TransmutationScriptTest, ReplacesVictimOnSuccessfulRoll)
{
	auto & attacker = unitsFake.add(BattleSide::ATTACKER);
	auto & victim = addVictim();
	unitsFake.setDefaultBonusExpectations();
	expectCreatureLookup();

	EXPECT_CALL(serverMock, rollCombatAbility(_, _, Eq(chance))).WillOnce(Return(true));
	EXPECT_CALL(*battleFake, nextUnitId()).WillOnce(Return(newUnitId));

	std::vector<BattleUnitsChanged> packs;
	EXPECT_CALL(serverMock, apply(Matcher<BattleUnitsChanged &>(_))).Times(2)
		.WillRepeatedly(Invoke([&packs](BattleUnitsChanged & pack){ packs.push_back(pack); }));
	EXPECT_CALL(serverMock, apply(Matcher<StacksInjured &>(_))).Times(1);

	script->run(&serverMock, *battleFake, CombatEventType::AFTER_ATTACK, &attacker, &victim, parameters(), CombatEventPayload());

	ASSERT_EQ(packs.size(), 2u);

	// the victim is removed first, so its hex is free by the time the replacement is placed
	ASSERT_EQ(packs[0].changedStacks.size(), 1u);
	EXPECT_EQ(packs[0].changedStacks[0].operation, UnitChanges::EOperation::REMOVE);

	ASSERT_EQ(packs[1].changedStacks.size(), 1u);
	EXPECT_EQ(packs[1].changedStacks[0].operation, UnitChanges::EOperation::ADD);

	::battle::UnitInfo added;
	added.load(newUnitId, packs[1].changedStacks[0].data);

	EXPECT_EQ(added.count, victimCount);
	EXPECT_EQ(added.position, victimPosition);
	EXPECT_EQ(added.side, BattleSide::DEFENDER);
	EXPECT_FALSE(added.summoned);
	EXPECT_EQ(added.type, resultingCreature);
}

TEST_F(TransmutationScriptTest, FailedRollChangesNothing)
{
	auto & attacker = unitsFake.add(BattleSide::ATTACKER);
	auto & victim = addVictim();
	unitsFake.setDefaultBonusExpectations();

	EXPECT_CALL(serverMock, rollCombatAbility(_, _, Eq(chance))).WillOnce(Return(false));

	// serverMock is strict, so any mutation attempted by the script fails the test
	script->run(&serverMock, *battleFake, CombatEventType::AFTER_ATTACK, &attacker, &victim, parameters(), CombatEventPayload());
}

TEST_F(TransmutationScriptTest, ImmuneVictimIsNotRolledFor)
{
	auto & attacker = unitsFake.add(BattleSide::ATTACKER);
	auto & victim = addVictim();
	victim.addNewBonus(std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::TRANSMUTATION_IMMUNITY, BonusSource::CREATURE_ABILITY, 1, BonusSourceID()));
	unitsFake.setDefaultBonusExpectations();

	script->run(&serverMock, *battleFake, CombatEventType::AFTER_ATTACK, &attacker, &victim, parameters(), CombatEventPayload());
}

}
