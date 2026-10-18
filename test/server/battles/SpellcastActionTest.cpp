/*
 * SpellcastActionTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "BattleTestFixture.h"

#include "../../../lib/battle/BattleAction.h"
#include "../../../lib/battle/CPlayerBattleCallback.h"
#include "../../../lib/battle/Destination.h"
#include "../../../lib/battle/actions/BattleActionType.h"
#include "../../../lib/bonuses/Bonus.h"
#include "../../../lib/mapObjects/CGHeroInstance.h"
#include "../../../lib/spells/CSpell.h"
#include "../../../lib/spells/ISpellMechanics.h"
#include "../../../lib/spells/Problem.h"

namespace
{
// creatures
constexpr int pikeman = 0; // walks 4 hexes
constexpr int masterGenie = 37;
constexpr int stormElemental = 127; // casts Protection from Air

bool isAffectedBy(const CStack * unit, SpellID spell)
{
	return unit->hasBonusFrom(BonusSource::SPELL_EFFECT, BonusSourceID(spell));
}

}

/// Spells cast as a battle action: by a creature on its turn, by a creature after walking up to
/// its target, and by a hero.
class SpellcastActionTest : public BattleTestFixture
{
public:
	static constexpr int32_t stackCount = 10;

	static inline const BattleHex casterHex = BattleHex(3, 5);
	static inline const BattleHex allyHex = BattleHex(7, 5);
	static inline const BattleHex enemyHex = BattleHex(12, 5);
	static inline const BattleHex freeHex = BattleHex(7, 8);

	/// Rules out a second turn from good morale, so that the unit active after an action depends on
	/// the action alone.
	static void ignoreMorale(CStack * unit)
	{
		unit->addNewBonus(std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::NO_MORALE, BonusSource::OTHER, 0, BonusSourceID()));
	}

	static BattleAction randomSpell(const CStack * genie, const BattleHex & targetHex)
	{
		return BattleAction::makeRandomSpellcast(genie, targetHex);
	}
};

TEST_F(SpellcastActionTest, creatureCastsSpellOnTarget)
{
	startGame();
	startBattle();

	CStack * caster = addStack(BattleSide::ATTACKER, CreatureID(stormElemental), casterHex, stackCount);
	CStack * ally = addStack(BattleSide::ATTACKER, CreatureID(pikeman), allyHex, stackCount);
	const auto castsBefore = caster->casts.available();

	battle::Target target;
	target.emplace_back(ally);
	ASSERT_TRUE(act(BattleAction::makeCreatureSpellcast(caster, target, SpellID::PROTECTION_FROM_AIR)));

	EXPECT_TRUE(isAffectedBy(ally, SpellID::PROTECTION_FROM_AIR));
	EXPECT_EQ(caster->casts.available(), castsBefore - 1);
	EXPECT_TRUE(caster->castSpellThisTurn());
}

TEST_F(SpellcastActionTest, creatureCastsMassSpellWithoutTarget)
{
	startGame();
	startBattle();

	CStack * caster = addStack(BattleSide::ATTACKER, CreatureID(pikeman), casterHex, stackCount);
	CStack * ally = addStack(BattleSide::ATTACKER, CreatureID(pikeman), allyHex, stackCount);
	CStack * enemy = addStack(BattleSide::DEFENDER, CreatureID(pikeman), enemyHex, stackCount);

	// at expert level Bless affects the whole army, so the client sends no destination
	grantSpell(caster, BonusType::SPELLCASTER, SpellID::BLESS, 3);
	ignoreMorale(caster);

	BattleAction action;
	action.side = BattleSide::ATTACKER;
	action.stackNumber = caster->unitId();
	action.actionType = EActionType::MONSTER_SPELL;
	action.spell = SpellID::BLESS;
	action.aimToHex(BattleHex::INVALID);

	ASSERT_TRUE(act(action));
	EXPECT_TRUE(isAffectedBy(caster, SpellID::BLESS));
	EXPECT_TRUE(isAffectedBy(ally, SpellID::BLESS));
	EXPECT_FALSE(isAffectedBy(enemy, SpellID::BLESS));
	EXPECT_NE(battle()->getActiveStackID(), static_cast<int32_t>(caster->unitId()));
}

TEST_F(SpellcastActionTest, spellCastWithoutSkipKeepsCasterActive)
{
	startGame();
	startBattle();

	const SpellID spell = spellByName("vcmi-test:testCastWithoutSkip");

	CStack * caster = addStack(BattleSide::ATTACKER, CreatureID(pikeman), casterHex, stackCount);
	CStack * ally = addStack(BattleSide::ATTACKER, CreatureID(pikeman), allyHex, stackCount);
	grantSpell(caster, BonusType::SPELLCASTER, spell, 0);
	ignoreMorale(caster);

	battle::Target target;
	target.emplace_back(ally);
	ASSERT_TRUE(act(BattleAction::makeCreatureSpellcast(caster, target, spell)));

	EXPECT_TRUE(isAffectedBy(ally, spell));
	EXPECT_EQ(battle()->getActiveStackID(), static_cast<int32_t>(caster->unitId()));
}

TEST_F(SpellcastActionTest, secondCastInTurnIsRefusedAndSkipsTurn)
{
	startGame();
	startBattle();

	const SpellID spell = spellByName("vcmi-test:testCastWithoutSkip");

	CStack * caster = addStack(BattleSide::ATTACKER, CreatureID(pikeman), casterHex, stackCount);
	CStack * ally = addStack(BattleSide::ATTACKER, CreatureID(pikeman), allyHex, stackCount);
	// casts are left for a second spell, but a unit casts only once per turn
	grantSpell(caster, BonusType::SPELLCASTER, spell, 0);
	grantSpell(caster, BonusType::SPELLCASTER, spell, 0);
	ignoreMorale(caster);

	battle::Target target;
	target.emplace_back(ally);
	ASSERT_TRUE(act(BattleAction::makeCreatureSpellcast(caster, target, spell)));
	ASSERT_EQ(battle()->getActiveStackID(), static_cast<int32_t>(caster->unitId()));

	EXPECT_FALSE(act(BattleAction::makeCreatureSpellcast(caster, target, spell)));
	EXPECT_TRUE(caster->moved());
	EXPECT_NE(battle()->getActiveStackID(), static_cast<int32_t>(caster->unitId()));
}

TEST_F(SpellcastActionTest, casterCannotCastSpellItDoesNotHave)
{
	startGame();
	startBattle();

	CStack * caster = addStack(BattleSide::ATTACKER, CreatureID(stormElemental), casterHex, stackCount);
	CStack * ally = addStack(BattleSide::ATTACKER, CreatureID(pikeman), allyHex, stackCount);

	battle::Target target;
	target.emplace_back(ally);
	EXPECT_FALSE(act(BattleAction::makeCreatureSpellcast(caster, target, SpellID::BLESS)));
	EXPECT_FALSE(isAffectedBy(ally, SpellID::BLESS));
}

TEST_F(SpellcastActionTest, genieCastsOnlyOnOtherLivingUnitsOfItsSide)
{
	startGame();
	startBattle();

	const CStack * genie = addStack(BattleSide::ATTACKER, CreatureID(masterGenie), casterHex, stackCount);
	const CStack * enemy = addStack(BattleSide::DEFENDER, CreatureID(pikeman), enemyHex, stackCount);
	const auto castsBefore = genie->casts.available();

	EXPECT_FALSE(act(randomSpell(genie, enemy->getPosition())));
	EXPECT_FALSE(act(randomSpell(genie, genie->getPosition())));
	EXPECT_FALSE(act(randomSpell(genie, allyHex)));

	EXPECT_TRUE(server.casts.empty());
	EXPECT_EQ(genie->casts.available(), castsBefore);
}

TEST_F(SpellcastActionTest, genieWithoutCastsLeftIsRefused)
{
	startGame();
	startBattle();

	CStack * genie = addStack(BattleSide::ATTACKER, CreatureID(masterGenie), casterHex, stackCount);
	CStack * ally = addStack(BattleSide::ATTACKER, CreatureID(pikeman), allyHex, stackCount);
	genie->casts.use(genie->casts.available());

	EXPECT_FALSE(act(randomSpell(genie, ally->getPosition())));
	EXPECT_TRUE(server.casts.empty());
}

TEST_F(SpellcastActionTest, walkAndCastNeedsAdjacentSpellcasterAbility)
{
	startGame();
	startBattle();

	const BattleHex castFrom = allyHex.cloneInDirection(BattleHex::LEFT);

	CStack * caster = addStack(BattleSide::ATTACKER, CreatureID(pikeman), casterHex, stackCount);
	CStack * ally = addStack(BattleSide::ATTACKER, CreatureID(pikeman), allyHex, stackCount);
	grantSpell(caster, BonusType::SPELLCASTER, SpellID::BLESS, 0);

	EXPECT_FALSE(act(BattleAction::makeWalkAndCast(caster, castFrom, ally, SpellID::BLESS)));
	EXPECT_EQ(caster->getPosition(), casterHex);
	EXPECT_FALSE(isAffectedBy(ally, SpellID::BLESS));
}

TEST_F(SpellcastActionTest, walkAndCastIsRefusedWithoutMoving)
{
	startGame();
	startBattle();

	const BattleHex castFrom = allyHex.cloneInDirection(BattleHex::LEFT);

	CStack * caster = addStack(BattleSide::ATTACKER, CreatureID(pikeman), casterHex, stackCount);
	CStack * ally = addStack(BattleSide::ATTACKER, CreatureID(pikeman), allyHex, stackCount);
	grantSpell(caster, BonusType::ADJACENT_SPELLCASTER, SpellID::BLESS, 0);
	grantSpell(caster, BonusType::ADJACENT_SPELLCASTER, SpellID::RESURRECTION, 0);

	// a hex in reach that is not next to the target
	EXPECT_FALSE(act(BattleAction::makeWalkAndCast(caster, castFrom.cloneInDirection(BattleHex::LEFT), ally, SpellID::BLESS)));
	// the unit has no losses to restore
	EXPECT_FALSE(act(BattleAction::makeWalkAndCast(caster, castFrom, ally, SpellID::RESURRECTION)));

	caster->casts.use(caster->casts.available());
	EXPECT_FALSE(act(BattleAction::makeWalkAndCast(caster, castFrom, ally, SpellID::BLESS)));

	EXPECT_EQ(caster->getPosition(), casterHex);
	EXPECT_TRUE(server.casts.empty());
}

TEST_F(SpellcastActionTest, quicksandOnTheWayCancelsAdjacentCast)
{
	startGame();
	startBattle();

	const BattleHex castFrom = allyHex.cloneInDirection(BattleHex::LEFT);
	const BattleHex halfway = BattleHex(5, 5);

	CStack * caster = addStack(BattleSide::ATTACKER, CreatureID(pikeman), casterHex, stackCount);
	CStack * ally = addStack(BattleSide::ATTACKER, CreatureID(pikeman), allyHex, stackCount);
	grantSpell(caster, BonusType::ADJACENT_SPELLCASTER, SpellID::BLESS, 0);
	addQuicksand(halfway);

	// stepping into a trap is a legal move that ends the action early
	ASSERT_TRUE(act(BattleAction::makeWalkAndCast(caster, castFrom, ally, SpellID::BLESS)));
	EXPECT_EQ(caster->getPosition(), halfway);
	EXPECT_FALSE(isAffectedBy(ally, SpellID::BLESS));
}

TEST_F(SpellcastActionTest, heroCastsSpellOnUnit)
{
	startGame();
	startBattle();

	const CStack * ally = addStack(BattleSide::ATTACKER, CreatureID(pikeman), allyHex, stackCount);
	teachSpell(attackerSideHero, SpellID::BLESS);
	const auto manaBefore = attackerSideHero->mana;

	// a hero spell needs an active unit of the hero's side, which stays active after the cast
	battle()->activeStack = ally->unitId();

	ASSERT_TRUE(castAsHero(attackerSideHero, SpellID::BLESS, ally));

	EXPECT_TRUE(isAffectedBy(ally, SpellID::BLESS));
	EXPECT_LT(attackerSideHero->mana, manaBefore);
	EXPECT_EQ(battle()->getSide(BattleSide::ATTACKER).castSpellsCount, 1u);
	EXPECT_EQ(battle()->getSide(BattleSide::ATTACKER).usedSpellsHistory, std::vector<SpellID>{SpellID::BLESS});
	EXPECT_EQ(battle()->getActiveStackID(), static_cast<int32_t>(ally->unitId()));
}

TEST_F(SpellcastActionTest, heroCastsOnlyOncePerRound)
{
	startGame();
	startBattle();

	const CStack * ally = addStack(BattleSide::ATTACKER, CreatureID(pikeman), allyHex, stackCount);
	teachSpell(attackerSideHero, SpellID::BLESS);

	ASSERT_TRUE(castAsHero(attackerSideHero, SpellID::BLESS, ally));
	const auto manaAfterFirstCast = attackerSideHero->mana;

	EXPECT_FALSE(castAsHero(attackerSideHero, SpellID::BLESS, ally));
	EXPECT_EQ(attackerSideHero->mana, manaAfterFirstCast);
	EXPECT_EQ(battle()->getSide(BattleSide::ATTACKER).castSpellsCount, 1u);
}

TEST_F(SpellcastActionTest, heroCastsSpellAtHexOfUnit)
{
	startGame();
	startBattle();

	const CStack * ally = addStack(BattleSide::ATTACKER, CreatureID(pikeman), allyHex, stackCount);
	teachSpell(attackerSideHero, SpellID::BLESS);

	// the form BattleAI sends
	ASSERT_TRUE(castAsHero(attackerSideHero, SpellID::BLESS, battle::Target{battle::Destination(ally->getPosition())}));
	EXPECT_TRUE(isAffectedBy(ally, SpellID::BLESS));
}

TEST_F(SpellcastActionTest, heroCastsAreaSpellAtHex)
{
	startGame();
	startBattle();

	addStack(BattleSide::ATTACKER, CreatureID(pikeman), casterHex, stackCount);
	const CStack * enemy = addStack(BattleSide::DEFENDER, CreatureID(pikeman), enemyHex, stackCount);
	teachSpell(attackerSideHero, SpellID::FIREBALL);
	const auto healthBefore = enemy->getAvailableHealth();

	// next to the enemy, which is in the area of the spell
	ASSERT_TRUE(castAsHero(attackerSideHero, SpellID::FIREBALL, battle::Target{battle::Destination(enemyHex.cloneInDirection(BattleHex::LEFT))}));
	EXPECT_LT(enemy->getAvailableHealth(), healthBefore);
}

namespace
{
/// A hero spell sent with a target that the spell can't be cast at.
struct RefusedTargetCase
{
	const char * name;
	SpellID spell;
	battle::Target (*makeTarget)(const CStack * ally, const CStack * enemy);
};
}

class RefusedHeroSpellTest : public SpellcastActionTest, public ::testing::WithParamInterface<RefusedTargetCase>
{
};

TEST_P(RefusedHeroSpellTest, spellAtInvalidTargetIsRefused)
{
	const auto & scenario = GetParam();

	startGame();
	startBattle();

	const CStack * ally = addStack(BattleSide::ATTACKER, CreatureID(pikeman), allyHex, stackCount);
	const CStack * enemy = addStack(BattleSide::DEFENDER, CreatureID(pikeman), enemyHex, stackCount);
	teachSpell(attackerSideHero, scenario.spell);
	const auto manaBefore = attackerSideHero->mana;

	EXPECT_FALSE(castAsHero(attackerSideHero, scenario.spell, scenario.makeTarget(ally, enemy))) << scenario.name;
	EXPECT_EQ(attackerSideHero->mana, manaBefore) << scenario.name;
	EXPECT_TRUE(server.casts.empty()) << scenario.name;
	EXPECT_EQ(ally->getPosition(), allyHex) << scenario.name;
}

INSTANTIATE_TEST_SUITE_P(Targets, RefusedHeroSpellTest, ::testing::Values(
	RefusedTargetCase{"unitSpellAtFreeHex", SpellID::BLESS, [](const CStack *, const CStack *)
	{
		return battle::Target{battle::Destination(SpellcastActionTest::freeHex)};
	}},
	RefusedTargetCase{"unitSpellWithoutDestination", SpellID::BLESS, [](const CStack *, const CStack *)
	{
		return battle::Target{battle::Destination()};
	}},
	RefusedTargetCase{"areaSpellWithoutDestination", SpellID::FIREBALL, [](const CStack *, const CStack *)
	{
		return battle::Target{battle::Destination()};
	}},
	RefusedTargetCase{"freeHexSpellAtUnit", SpellID::FORCE_FIELD, [](const CStack *, const CStack * enemy)
	{
		return battle::Target{battle::Destination(enemy->getPosition())};
	}},
	RefusedTargetCase{"teleportWithoutDestinationHex", SpellID::TELEPORT, [](const CStack * ally, const CStack *)
	{
		return battle::Target{battle::Destination(ally)};
	}},
	RefusedTargetCase{"teleportToOccupiedHex", SpellID::TELEPORT, [](const CStack * ally, const CStack * enemy)
	{
		return battle::Target{battle::Destination(ally), battle::Destination(enemy->getPosition())};
	}}
),
	[](const ::testing::TestParamInfo<RefusedTargetCase> & info) { return info.param.name; });

TEST_F(SpellcastActionTest, creatureSpellAtInvalidTargetIsRefused)
{
	startGame();
	startBattle();

	const CStack * caster = addStack(BattleSide::ATTACKER, CreatureID(stormElemental), casterHex, stackCount);
	const auto castsBefore = caster->casts.available();

	// the spell needs a unit
	EXPECT_FALSE(castAsUnit(caster, SpellID::PROTECTION_FROM_AIR, allyHex));
	EXPECT_EQ(caster->casts.available(), castsBefore);
	EXPECT_TRUE(server.casts.empty());
}

TEST_F(SpellcastActionTest, creatureWithoutCastsLeftIsRefused)
{
	startGame();
	startBattle();

	CStack * caster = addStack(BattleSide::ATTACKER, CreatureID(stormElemental), casterHex, stackCount);
	const CStack * ally = addStack(BattleSide::ATTACKER, CreatureID(pikeman), allyHex, stackCount);
	caster->casts.use(caster->casts.available());

	EXPECT_FALSE(castAsUnit(caster, SpellID::PROTECTION_FROM_AIR, ally->getPosition()));
	EXPECT_FALSE(isAffectedBy(ally, SpellID::PROTECTION_FROM_AIR));
}

/// What the client shows and sends for the spell options of a hero and of a unit.
class SpellOptionTest : public SpellcastActionTest
{
public:
	static ActionOption heroOption(SpellID spell)
	{
		return BattleActionType::makeHeroSpellOption(spell);
	}

	/// Query of the attacking hero about the hex, after the destinations chosen earlier
	static ActionContext heroContext(const BattleHex & hex, const battle::Target & chosen = {})
	{
		return {nullptr, hex, std::nullopt, chosen, BattleSide::ATTACKER};
	}

	void prepareHero(SpellID spell, const CStack * active)
	{
		teachSpell(attackerSideHero, spell);
		battle()->activeStack = active->unitId();
	}
};

TEST_F(SpellOptionTest, heroSpellIsAimedAtUnit)
{
	startGame();
	startBattle();

	const CStack * ally = addStack(BattleSide::ATTACKER, CreatureID(pikeman), allyHex, stackCount);
	prepareHero(SpellID::BLESS, ally);
	const auto option = heroOption(SpellID::BLESS);

	EXPECT_TRUE(isLegal(option, heroContext(allyHex)));
	EXPECT_EQ(preview(option, heroContext(allyHex)).shadedHexes, BattleHexArray({allyHex}));

	const auto blocked = preview(option, heroContext(freeHex));
	EXPECT_EQ(blocked.cursor, "combatBlocked");
	EXPECT_FALSE(blocked.statusText.empty());

	// one destination: no click adds a target step, and nothing is cast without a click
	EXPECT_FALSE(option.type->buildTargetStep(*battle(), option, heroContext(allyHex)).has_value());
	EXPECT_FALSE(option.type->build(*battle(), option, heroContext(BattleHex::INVALID)).has_value());

	ASSERT_TRUE(act(option.type->build(*battle(), option, heroContext(allyHex)).value()));
	EXPECT_TRUE(isAffectedBy(ally, SpellID::BLESS));
}

TEST_F(SpellOptionTest, areaSpellShowsItsAreaAndUnitsInIt)
{
	startGame();
	startBattle();

	const CStack * active = addStack(BattleSide::ATTACKER, CreatureID(pikeman), casterHex, stackCount);
	const CStack * enemy = addStack(BattleSide::DEFENDER, CreatureID(pikeman), enemyHex, stackCount);
	prepareHero(SpellID::FIREBALL, active);
	const auto option = heroOption(SpellID::FIREBALL);

	const BattleHex nextToEnemy = enemyHex.cloneInDirection(BattleHex::LEFT);
	const auto shown = preview(option, heroContext(nextToEnemy));
	EXPECT_EQ(shown.cursor, "castSpell");
	EXPECT_EQ(shown.shadedHexes.size(), 7);
	EXPECT_TRUE(shown.shadedHexes.contains(enemyHex));
	EXPECT_EQ(shown.affectedUnits, std::vector<uint32_t>{enemy->unitId()});

	// the unit on the hovered hex is not listed
	EXPECT_TRUE(preview(option, heroContext(enemyHex)).affectedUnits.empty());
}

TEST_F(SpellOptionTest, massSpellIsCastWithoutClick)
{
	startGame();
	setMagicMastery(attackerSideHero, 3);
	startBattle();

	const CStack * ally = addStack(BattleSide::ATTACKER, CreatureID(pikeman), allyHex, stackCount);
	const CStack * enemy = addStack(BattleSide::DEFENDER, CreatureID(pikeman), enemyHex, stackCount);
	prepareHero(SpellID::BLESS, ally);
	const auto option = heroOption(SpellID::BLESS);

	EXPECT_FALSE(isLegal(option, heroContext(allyHex)));
	EXPECT_EQ(preview(option, heroContext(allyHex)).cursor, "combatBlocked");

	EXPECT_TRUE(isLegal(option, heroContext(BattleHex::INVALID)));
	ASSERT_TRUE(act(option.type->build(*battle(), option, heroContext(BattleHex::INVALID)).value()));
	EXPECT_TRUE(isAffectedBy(ally, SpellID::BLESS));
	EXPECT_FALSE(isAffectedBy(enemy, SpellID::BLESS));
}

TEST_F(SpellOptionTest, teleportIsAimedAtUnitThenAtFreeHex)
{
	startGame();
	startBattle();

	CStack * ally = addStack(BattleSide::ATTACKER, CreatureID(pikeman), allyHex, stackCount);
	const CStack * enemy = addStack(BattleSide::DEFENDER, CreatureID(pikeman), enemyHex, stackCount);
	prepareHero(SpellID::TELEPORT, ally);
	const auto option = heroOption(SpellID::TELEPORT);

	// the first click chooses the unit and sends nothing
	EXPECT_FALSE(option.type->build(*battle(), option, heroContext(allyHex)).has_value());
	EXPECT_FALSE(option.type->buildTargetStep(*battle(), option, heroContext(freeHex)).has_value());
	EXPECT_EQ(preview(option, heroContext(freeHex)).cursor, "combatBlocked");

	const auto unitStep = option.type->buildTargetStep(*battle(), option, heroContext(allyHex));
	ASSERT_TRUE(unitStep.has_value());
	EXPECT_EQ(unitStep->unitValue, ally);
	const battle::Target chosen = {*unitStep};

	EXPECT_FALSE(option.type->buildTargetStep(*battle(), option, heroContext(freeHex, chosen)).has_value());

	const auto blocked = preview(option, heroContext(enemy->getPosition(), chosen));
	EXPECT_EQ(blocked.cursor, "combatBlocked");
	EXPECT_FALSE(blocked.statusText.empty());

	ASSERT_TRUE(isLegal(option, heroContext(freeHex, chosen)));
	ASSERT_TRUE(act(option.type->build(*battle(), option, heroContext(freeHex, chosen)).value()));
	EXPECT_EQ(ally->getPosition(), freeHex);
}

TEST_F(SpellOptionTest, sacrificeIsAimedAtDeadUnitThenAtVictim)
{
	startGame();
	startBattle();

	CStack * active = addStack(BattleSide::ATTACKER, CreatureID(pikeman), casterHex, stackCount);
	const CStack * fallen = addStack(BattleSide::ATTACKER, CreatureID(pikeman), allyHex, stackCount);
	const CStack * victim = addStack(BattleSide::ATTACKER, CreatureID(pikeman), freeHex, 2 * stackCount);
	const auto victimId = victim->unitId();

	injure(fallen, fallen->getAvailableHealth());
	ASSERT_FALSE(fallen->alive());

	prepareHero(SpellID::SACRIFICE, active);
	const auto option = heroOption(SpellID::SACRIFICE);

	const auto fallenStep = option.type->buildTargetStep(*battle(), option, heroContext(allyHex));
	ASSERT_TRUE(fallenStep.has_value());
	const battle::Target chosen = {*fallenStep};

	// no unit to give up
	EXPECT_FALSE(isLegal(option, heroContext(BattleHex(9, 8), chosen)));

	ASSERT_TRUE(act(option.type->build(*battle(), option, heroContext(victim->getPosition(), chosen)).value()));
	EXPECT_TRUE(fallen->alive());
	EXPECT_GT(fallen->getCount(), 0);

	const auto * victimAfter = battle()->battleGetStackByID(victimId, false);
	EXPECT_TRUE(victimAfter == nullptr || !victimAfter->alive());
}

TEST_F(SpellOptionTest, unitSpellIsTriedFirstOnUnitsItSuits)
{
	startGame();
	startBattle();

	CStack * caster = addStack(BattleSide::ATTACKER, CreatureID(stormElemental), casterHex, stackCount);
	const CStack * ally = addStack(BattleSide::ATTACKER, CreatureID(pikeman), allyHex, stackCount);
	const CStack * enemy = addStack(BattleSide::DEFENDER, CreatureID(pikeman), enemyHex, stackCount);
	grantSpell(caster, BonusType::SPELLCASTER, SpellID::CURSE, 0);

	const auto options = optionsOf(caster);

	const auto optionOf = [&options](SpellID spell)
	{
		for(const auto & option : options)
			if(option.spell == spell)
				return option;

		ADD_FAILURE() << "no option of the spell";
		return ActionOption{};
	};

	const auto positive = optionOf(SpellID::PROTECTION_FROM_AIR);
	const auto negative = optionOf(SpellID::CURSE);
	ASSERT_TRUE(positive.type && negative.type);
	EXPECT_EQ(positive.button.value().spell, SpellID::PROTECTION_FROM_AIR);

	const int melee = BattleActionType::find(EActionType::WALK_AND_ATTACK)->getPriority(*battle(), {}, *caster, enemy);

	EXPECT_LT(positive.type->getPriority(*battle(), positive, *caster, ally), melee);
	EXPECT_GT(positive.type->getPriority(*battle(), positive, *caster, enemy), melee);
	EXPECT_LT(negative.type->getPriority(*battle(), negative, *caster, enemy), melee);
	EXPECT_GT(negative.type->getPriority(*battle(), negative, *caster, ally), melee);
	EXPECT_GT(positive.type->getPriority(*battle(), positive, *caster, nullptr), melee);

	const ActionContext onAlly{caster, allyHex};
	EXPECT_TRUE(isLegal(positive, onAlly));
	ASSERT_TRUE(act(positive.type->build(*battle(), positive, onAlly).value()));
	EXPECT_TRUE(isAffectedBy(ally, SpellID::PROTECTION_FROM_AIR));
}

TEST_F(SpellOptionTest, unitThatCastThisTurnHasNoSpellOptions)
{
	startGame();
	startBattle();

	CStack * caster = addStack(BattleSide::ATTACKER, CreatureID(stormElemental), casterHex, stackCount);
	caster->setCastSpellThisTurn(true);

	const auto options = optionsOf(caster);
	ASSERT_FALSE(options.empty());

	for(const auto & option : options)
		EXPECT_FALSE(option.spell.hasValue());
}

TEST_F(SpellOptionTest, genieOffersRandomSpellForOtherUnitsOfItsSide)
{
	startGame();
	startBattle();

	CStack * genie = addStack(BattleSide::ATTACKER, CreatureID(masterGenie), casterHex, stackCount);
	const CStack * ally = addStack(BattleSide::ATTACKER, CreatureID(pikeman), allyHex, stackCount);
	const CStack * enemy = addStack(BattleSide::DEFENDER, CreatureID(pikeman), enemyHex, stackCount);
	const auto castsBefore = genie->casts.available();

	const auto option = optionOf(genie, EActionType::RANDOM_SPELL);
	EXPECT_TRUE(option.button.has_value());

	const auto shown = preview(option, {genie, allyHex});
	EXPECT_EQ(shown.cursor, "castSpell");
	EXPECT_THAT(textIDsOf(shown.statusText), ::testing::ElementsAre("core.genrltxt.301", pluralNameOf(CreatureID(pikeman)))); // Cast a spell on %s

	const auto blocked = preview(option, {genie, enemy->getPosition()});
	EXPECT_EQ(blocked.cursor, "combatBlocked");
	EXPECT_FALSE(blocked.statusText.empty());

	ASSERT_TRUE(act(option.type->build(*battle(), option, {genie, allyHex}).value()));
	// the action carries only the target, the server rolls the spell
	ASSERT_EQ(server.casts.size(), 1u);
	const auto & cast = server.casts.front().announcement;
	EXPECT_EQ(cast.casterStack, static_cast<si32>(genie->unitId()));
	EXPECT_EQ(cast.affectedCres, std::vector<ui32>{ally->unitId()});
	EXPECT_TRUE(cast.spellID.toSpell()->isPositive());
	EXPECT_TRUE(isAffectedBy(ally, cast.spellID));
	EXPECT_EQ(genie->casts.available(), castsBefore - 1);
}

TEST_F(SpellOptionTest, adjacentCasterApproachesTargetFromChosenSide)
{
	startGame();
	startBattle();

	CStack * caster = addStack(BattleSide::ATTACKER, CreatureID(pikeman), casterHex, stackCount);
	const CStack * ally = addStack(BattleSide::ATTACKER, CreatureID(pikeman), allyHex, stackCount);
	grantSpell(caster, BonusType::ADJACENT_SPELLCASTER, SpellID::BLESS, 0);
	const auto castsBefore = caster->casts.available();

	const auto option = optionOf(caster, EActionType::WALK_AND_CAST);
	EXPECT_EQ(option.spell, SpellID::BLESS);

	const auto approaches = option.type->getApproaches(*battle(), option, *caster, allyHex);
	const BattleHex castFrom = allyHex.cloneInDirection(BattleHex::LEFT);
	const auto fromLeft = std::find_if(approaches.begin(), approaches.end(), [&castFrom](const ActionApproach & approach)
	{
		return approach.from == castFrom;
	});
	ASSERT_NE(fromLeft, approaches.end());

	// nothing to cast on, and no cast on itself
	EXPECT_TRUE(option.type->getApproaches(*battle(), option, *caster, freeHex).empty());
	EXPECT_TRUE(option.type->getApproaches(*battle(), option, *caster, casterHex).empty());
	EXPECT_FALSE(isLegal(option, {caster, freeHex}));

	const ActionContext context{caster, allyHex, *fromLeft};
	const auto shown = preview(option, context);
	EXPECT_EQ(shown.cursor, "castSpell");
	EXPECT_THAT(textIDsOf(shown.statusText), ::testing::ElementsAre("core.genrltxt.27", SpellID(SpellID::BLESS).toSpell()->getNameTextID(), pluralNameOf(CreatureID(pikeman)))); // Cast %s on %s
	EXPECT_EQ(shown.shadedHexes, BattleHexArray({castFrom, allyHex}));

	ASSERT_TRUE(act(option.type->build(*battle(), option, context).value()));
	EXPECT_EQ(caster->getPosition(), castFrom);
	EXPECT_TRUE(isAffectedBy(ally, SpellID::BLESS));
	EXPECT_EQ(caster->casts.available(), castsBefore - 1);
}

/// Remove Obstacle is also checked on player side, where enemy quicksand stays hidden from the caster
TEST_F(SpellcastActionTest, removeObstacleSeesQuicksandHiddenFromCaster)
{
	startGame();
	startBattle();
	addQuicksand(enemyHex);

	CPlayerBattleCallback playerBattle(battle(), attackerSideHero->getOwner());
	ASSERT_TRUE(playerBattle.battleGetAllObstacles().empty());

	const auto * spell = SpellID(SpellID::REMOVE_OBSTACLE).toSpell();
	// passive mode skips spellbook and mana checks, so the check reaches spell effects
	spells::BattleCast cast(&playerBattle, attackerSideHero, spells::Mode::PASSIVE, spell);
	cast.setSpellLevel(3);
	auto mechanics = spell->battleMechanics(&cast);

	spells::detail::ProblemImpl problem;
	EXPECT_TRUE(mechanics->canBeCast(problem));
	EXPECT_TRUE(mechanics->canBeCastAt(spells::Target{spells::Destination(enemyHex)}, problem));
}
