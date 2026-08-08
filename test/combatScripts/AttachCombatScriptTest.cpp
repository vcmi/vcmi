/*
 * AttachCombatScriptTest.cpp, part of VCMI engine
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
#include "../../lib/bonuses/BonusParameters.h"
#include "../../lib/json/JsonNode.h"
#include "../../lib/modding/IdentifierStorage.h"
#include "../../lib/modding/ModScope.h"
#include "../../lib/networkPacks/SetStackEffect.h"

namespace test
{
using namespace ::testing;

/// Covers the granter spell effect: the bonus it builds in Lua must survive the trip through
/// json parsing, including resolving the combat script identifier - json built by a script
/// carries no mod scope, so the script name has to be fully qualified by then.
class AttachCombatScriptTest : public Test, public EffectFixture
{
public:
	const SpellID testSpellId = SpellID::CURSE;
	const int32_t duration = 12;

	AttachCombatScriptTest()
		: EffectFixture("core:attachCombatScript")
	{
	}

protected:
	void SetUp() override
	{
		EffectFixture::setUp();
		mechanicsMock.caster = nullptr;
	}
};

TEST_F(AttachCombatScriptTest, GrantsTriggerBonusReferringToScript)
{
	const int32_t damage = 7;

	JsonNode options;
	options["eventScript"].String() = "testSpikes";
	options["eventValue"].Integer() = damage;
	options["eventParameters"]["poison"].Bool() = true;
	options.setModScope(ModScope::scopeBuiltin());
	setupEffect(options);

	auto & unit = unitsFake.add(BattleSide::ATTACKER);
	unit.makeAlive();
	EXPECT_CALL(unit, unitId()).WillRepeatedly(Return(42u));
	unitsFake.setDefaultBonusExpectations();

	EXPECT_CALL(mechanicsMock, getSpell()).WillRepeatedly(Return(&spellStub));
	EXPECT_CALL(spellStub, getJsonKey()).WillRepeatedly(Return(SpellID::encode(testSpellId.getNum())));
	EXPECT_CALL(mechanicsMock, getEffectDuration()).WillRepeatedly(Return(duration));
	EXPECT_CALL(mechanicsMock, getEffectPower()).WillRepeatedly(Return(5));
	EXPECT_CALL(mechanicsMock, getEffectLevel()).WillRepeatedly(Return(2));
	EXPECT_CALL(serverMock, describeChanges()).WillRepeatedly(Return(false));

	std::vector<Bonus> actualBonus;
	EXPECT_CALL(*battleFake, updateUnitBonus(_, _)).WillOnce(SaveArg<1>(&actualBonus));
	EXPECT_CALL(serverMock, apply(Matcher<SetStackEffect &>(_))).Times(1);

	Target target;
	target.emplace_back(&unit, BattleHex());

	subject->apply(&serverMock, &mechanicsMock, target);

	ASSERT_EQ(actualBonus.size(), 1u);
	EXPECT_EQ(actualBonus[0].type, BonusType::COMBAT_EVENT_TRIGGER);
	EXPECT_EQ(actualBonus[0].turnsRemain, duration);

	auto expectedScript = LIBRARY->identifiers()->getIdentifier(ModScope::scopeGame(), "combatScript", "testSpikes", false);
	ASSERT_TRUE(expectedScript.has_value());
	EXPECT_EQ(actualBonus[0].subtype, BonusSubtypeID(CombatScriptID(*expectedScript)));
	EXPECT_EQ(actualBonus[0].val, damage);

	ASSERT_TRUE(actualBonus[0].parameters);
	const auto & parameters = actualBonus[0].parameters->toCustom<JsonNode>();

	// config parameters are passed through, caster state is snapshotted for later events
	EXPECT_TRUE(parameters["poison"].Bool());
	EXPECT_EQ(parameters["casterPower"].Integer(), 5);
}

}
