/*
 * AdventureSpellEffectTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "../../lib/spells/adventure/AdventureSpellEffect.h"

#include "../../include/vcmi/spells/Caster.h"
#include "../../lib/json/JsonNode.h"
#include "../mock/mock_IGameInfoCallback.h"

namespace
{
class TestCaster : public spells::Caster
{
public:
	int32_t getCasterUnitId() const override { return 0; }
	int32_t getSpellSchoolLevel(const spells::Spell *, SpellSchool *) const override { return 0; }
	int32_t getEffectLevel(const spells::Spell *) const override { return 0; }
	int64_t getSpellBonus(const spells::Spell *, int64_t base, const battle::Unit *) const override { return base; }
	int64_t getSpecificSpellBonus(const spells::Spell *, int64_t base) const override { return base; }
	int32_t getEffectPower(const spells::Spell *) const override { return 0; }
	int32_t getEnchantPower(const spells::Spell *) const override { return 0; }
	int64_t getEffectValue(const spells::Spell *) const override { return 0; }
	int64_t getEffectRange(const spells::Spell *) const override { return 0; }
	PlayerColor getCasterOwner() const override { return PlayerColor(0); }
	std::string getCasterNameTextID() const override { return {}; }
	void getCastDescription(const spells::Spell *, const battle::Units &, MetaString &) const override {}
	void spendMana(ServerCallback *, const int32_t) const override {}
	int32_t manaLimit() const override { return 0; }
	const CGHeroInstance * getHeroCaster() const override { return nullptr; }
};

class TestRangedAdventureEffect : public AdventureSpellRangedEffect
{
public:
	explicit TestRangedAdventureEffect(const JsonNode & config)
		: AdventureSpellRangedEffect(config)
	{
	}

	std::string getCursorForTarget(const IGameInfoCallback *, const spells::Caster *, const int3 &) const override
	{
		return {};
	}

	bool canBeCastAtImpl(spells::Problem &, const IGameInfoCallback *, const spells::Caster *, const int3 &) const override
	{
		return true;
	}
};

JsonNode makeRangedConfig()
{
	JsonNode config;
	config["rangeX"].Integer() = 3;
	config["rangeY"].Integer() = 2;
	config["ignoreFow"].Bool() = true;
	return config;
}
}

TEST(Spells_AdventureSpellRangedEffect, exposesConfiguredRange)
{
	TestRangedAdventureEffect effect(makeRangedConfig());

	EXPECT_EQ(effect.getRangeX(), 3);
	EXPECT_EQ(effect.getRangeY(), 2);
	EXPECT_TRUE(effect.ignoresFogOfWar());
}

TEST(Spells_AdventureSpellRangedEffect, validatesTargetFromPlannedSource)
{
	TestRangedAdventureEffect effect(makeRangedConfig());
	TestCaster caster;
	IGameInfoCallbackMock game;

	const int3 source(10, 10, 0);
	const int3 insideRange(13, 12, 0);
	const int3 outsideRange(14, 12, 0);

	EXPECT_CALL(game, isInTheMap(insideRange)).WillOnce(::testing::Return(true));
	EXPECT_TRUE(effect.isTargetInRangeFrom(&game, &caster, source, insideRange));

	EXPECT_CALL(game, isInTheMap(outsideRange)).WillOnce(::testing::Return(true));
	EXPECT_FALSE(effect.isTargetInRangeFrom(&game, &caster, source, outsideRange));
}
