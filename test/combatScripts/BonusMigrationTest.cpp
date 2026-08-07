/*
 * BonusMigrationTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"

#include "../../lib/CBonusTypeHandler.h"
#include "../../lib/GameLibrary.h"
#include "../../lib/combatScripts/CombatScriptService.h"
#include "../../lib/bonuses/Bonus.h"
#include "../../lib/bonuses/BonusMigration.h"
#include "../../lib/bonuses/BonusParameters.h"
#include "../../lib/json/JsonBonus.h"
#include "../../lib/json/JsonNode.h"
#include <vcmi/Creature.h>
#include <vcmi/CreatureService.h>
#include <vcmi/spells/Spell.h>
#include <vcmi/spells/Service.h>
#include "../../lib/modding/ModScope.h"

namespace test
{

/// Abilities that used to be hardcoded bonus types are combat scripts now. Content declaring the
/// old bonus must keep working, both when read from a mod config and when restored from a save.
class BonusMigrationTest : public ::testing::Test
{
public:
	/// Bonus configs always come from some mod, and identifier resolution relies on knowing which.
	static std::shared_ptr<Bonus> parse(JsonNode ability)
	{
		ability.setModScope(ModScope::scopeBuiltin());
		return JsonUtils::parseBonus(ability);
	}

	static const BonusParametersCombatScript & scriptOf(const std::shared_ptr<Bonus> & bonus)
	{
		EXPECT_EQ(bonus->type, BonusType::COMBAT_EVENT_TRIGGER);
		EXPECT_NE(bonus->parameters, nullptr);
		return bonus->parameters->toCustom<BonusParametersCombatScript>();
	}

	static void expectRunsScript(const std::shared_ptr<Bonus> & bonus, const std::string & script)
	{
		const auto & data = scriptOf(bonus);

		EXPECT_NE(data.eventScript, CombatScriptID::NONE);
		EXPECT_NE(LIBRARY->combatScripts()->get(data.eventScript), nullptr);
		EXPECT_NE(LIBRARY->combatScripts()->getDescriptionTextID(data.eventScript).find(script), std::string::npos);
	}
};

TEST_F(BonusMigrationTest, LifeDrainConfigRunsTheScript)
{
	JsonNode ability;
	ability["type"].String() = "LIFE_DRAIN";
	ability["val"].Integer() = 50;

	auto bonus = parse(ability);

	expectRunsScript(bonus, "lifeDrain");
	EXPECT_EQ(scriptOf(bonus).eventParameters["percentage"].Integer(), 50);
}

TEST_F(BonusMigrationTest, SoulStealSubtypeBecomesParameter)
{
	JsonNode permanent;
	permanent["type"].String() = "SOUL_STEAL";
	permanent["val"].Integer() = 2;
	permanent["subtype"].String() = "soulStealPermanent";

	JsonNode oneBattle = permanent;
	oneBattle["subtype"].String() = "soulStealBattle";

	auto migratedPermanent = parse(permanent);
	auto migratedOneBattle = parse(oneBattle);

	expectRunsScript(migratedPermanent, "soulSteal");
	EXPECT_EQ(scriptOf(migratedPermanent).eventParameters["creaturesPerKill"].Integer(), 2);
	EXPECT_TRUE(scriptOf(migratedPermanent).eventParameters["permanent"].Bool());
	EXPECT_FALSE(scriptOf(migratedOneBattle).eventParameters["permanent"].Bool());
}

TEST_F(BonusMigrationTest, TransmutationSubtypeBecomesParameter)
{
	JsonNode perHealth;
	perHealth["type"].String() = "TRANSMUTATION";
	perHealth["val"].Integer() = 40;
	perHealth["subtype"].String() = "transmutationPerHealth";
	perHealth["addInfo"].String() = "core:goldGolem";

	JsonNode perUnit = perHealth;
	perUnit["subtype"].String() = "transmutationPerUnit";

	auto migrated = parse(perHealth);

	expectRunsScript(migrated, "transmutation");
	EXPECT_EQ(scriptOf(migrated).eventParameters["chance"].Integer(), 40);
	EXPECT_EQ(scriptOf(migrated).eventParameters["transmuteBy"].String(), "health");
	EXPECT_EQ(scriptOf(migrated).eventParameters["creature"].String(), "core:goldGolem");
	EXPECT_EQ(scriptOf(parse(perUnit)).eventParameters["transmuteBy"].String(), "count");
}

/// Without a creature the script falls back to the attacker's own, so it must stay unset.
TEST_F(BonusMigrationTest, TransmutationWithoutCreatureSetsNone)
{
	JsonNode ability;
	ability["type"].String() = "TRANSMUTATION";
	ability["val"].Integer() = 10;
	ability["subtype"].String() = "transmutationPerUnit";

	EXPECT_TRUE(scriptOf(parse(ability)).eventParameters["creature"].isNull());
}

TEST_F(BonusMigrationTest, SummonGuardiansSubtypeBecomesCreature)
{
	JsonNode ability;
	ability["type"].String() = "SUMMON_GUARDIANS";
	ability["val"].Integer() = 50;
	ability["subtype"].String() = "core:woodElf";

	auto migrated = parse(ability);

	expectRunsScript(migrated, "summonGuardians");
	EXPECT_EQ(scriptOf(migrated).eventParameters["creature"].String(), "core:woodElf");
	EXPECT_EQ(scriptOf(migrated).eventParameters["percentage"].Integer(), 50);
}

/// ENCHANTED packed the mastery level and the "affects the whole side" flag into a single value.
TEST_F(BonusMigrationTest, EnchantedValueSplitsIntoLevelAndMassive)
{
	JsonNode single;
	single["type"].String() = "ENCHANTED";
	single["subtype"].String() = "core:bless";
	single["val"].Integer() = 2;

	JsonNode massive = single;
	massive["val"].Integer() = 5;

	auto migratedSingle = parse(single);
	auto migratedMassive = parse(massive);

	expectRunsScript(migratedSingle, "enchanted");
	EXPECT_EQ(scriptOf(migratedSingle).eventParameters["spell"].String(), "core:bless");
	EXPECT_EQ(scriptOf(migratedSingle).eventParameters["level"].Integer(), 2);
	EXPECT_FALSE(scriptOf(migratedSingle).eventParameters["massive"].Bool());

	EXPECT_EQ(scriptOf(migratedMassive).eventParameters["level"].Integer(), 2);
	EXPECT_TRUE(scriptOf(migratedMassive).eventParameters["massive"].Bool());
}

/// Saves hold resolved identifiers, which have to come back out as the json keys scripts expect.
TEST_F(BonusMigrationTest, SavedEntityReferencesBecomeJsonKeys)
{
	auto creature = LIBRARY->creatures()->getByName("core:woodElf");
	ASSERT_NE(creature, nullptr);

	Bonus guardians(BonusDuration::PERMANENT, BonusType::SUMMON_GUARDIANS, BonusSource::CREATURE_ABILITY, 50, BonusSourceID(), BonusSubtypeID(CreatureID(creature->getIndex())));
	Bonus enchanted(BonusDuration::PERMANENT, BonusType::ENCHANTED, BonusSource::CREATURE_ABILITY, 5, BonusSourceID(), BonusSubtypeID(SpellID(SpellID::BLESS)));

	ASSERT_TRUE(BonusMigration::migrateCombatAbility(guardians));
	ASSERT_TRUE(BonusMigration::migrateCombatAbility(enchanted));

	const auto & guardianData = guardians.parameters->toCustom<BonusParametersCombatScript>();
	const auto & enchantedData = enchanted.parameters->toCustom<BonusParametersCombatScript>();

	EXPECT_NE(LIBRARY->creatures()->getByName(guardianData.eventParameters["creature"].String()), nullptr);
	EXPECT_NE(LIBRARY->spells()->getByName(enchantedData.eventParameters["spell"].String()), nullptr);
	EXPECT_TRUE(enchantedData.eventParameters["massive"].Bool());
}

/// The conversion replaces what the ability is, not the settings applied on top of it.
TEST_F(BonusMigrationTest, UnrelatedFieldsSurvive)
{
	JsonNode ability;
	ability["type"].String() = "LIFE_DRAIN";
	ability["val"].Integer() = 100;
	ability["duration"].String() = "ONE_BATTLE";
	ability["turns"].Integer() = 3;

	auto bonus = parse(ability);

	EXPECT_EQ(bonus->duration, BonusDuration::ONE_BATTLE);
	EXPECT_EQ(bonus->turnsRemain, 3);
}

TEST_F(BonusMigrationTest, OtherBonusesAreUntouched)
{
	JsonNode ability;
	ability["type"].String() = "BLOCKS_RETALIATION";

	EXPECT_EQ(parse(ability)->type, BonusType::BLOCKS_RETALIATION);
}

/// Saves store the resolved bonus, so the same conversion has to work without any json around.
TEST_F(BonusMigrationTest, SavedBonusIsConverted)
{
	auto bonus = std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::SOUL_STEAL, BonusSource::CREATURE_ABILITY, 3, BonusSourceID(), BonusCustomSubtype::soulStealBattle);

	ASSERT_TRUE(BonusMigration::migrateCombatAbility(*bonus));

	expectRunsScript(bonus, "soulSteal");
	EXPECT_EQ(scriptOf(bonus).eventParameters["creaturesPerKill"].Integer(), 3);
	EXPECT_FALSE(scriptOf(bonus).eventParameters["permanent"].Bool());
	EXPECT_EQ(bonus->subtype, BonusSubtypeID());
}

TEST_F(BonusMigrationTest, SavedBonusOfOtherTypeIsUntouched)
{
	Bonus bonus(BonusDuration::PERMANENT, BonusType::BLOCKS_RETALIATION, BonusSource::CREATURE_ABILITY, 0, BonusSourceID());

	EXPECT_FALSE(BonusMigration::migrateCombatAbility(bonus));
	EXPECT_EQ(bonus.type, BonusType::BLOCKS_RETALIATION);
}

/// A migrated ability that renders as nothing would silently vanish from the creature window.
TEST_F(BonusMigrationTest, MigratedAbilityIsStillDescribed)
{
	JsonNode ability;
	ability["type"].String() = "LIFE_DRAIN";
	ability["val"].Integer() = 100;

	EXPECT_FALSE(LIBRARY->bth->bonusToString(parse(ability)).empty());
}

}
