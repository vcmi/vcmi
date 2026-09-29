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
#include "../../../lib/battle/Destination.h"
#include "../../../lib/bonuses/Bonus.h"
#include "../../../lib/mapObjects/CGHeroInstance.h"
#include "../../../lib/spells/CSpell.h"

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

	/// Rules out a second turn from good morale, so that the unit active after an action depends on
	/// the action alone.
	static void ignoreMorale(CStack * unit)
	{
		unit->addNewBonus(std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::NO_MORALE, BonusSource::OTHER, 0, BonusSourceID()));
	}

	/// The action the client sends when the attacking hero casts `spell`.
	static BattleAction heroSpell(SpellID spell, const battle::Target & target)
	{
		BattleAction action;
		action.side = BattleSide::ATTACKER;
		action.stackNumber = -1;
		action.actionType = EActionType::HERO_SPELL;
		action.spell = spell;
		action.setTarget(target);
		return action;
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
	EXPECT_TRUE(caster->castSpellThisTurn);
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

TEST_F(SpellcastActionTest, genieCastsRandomBeneficialSpell)
{
	startGame();
	startBattle();

	CStack * genie = addStack(BattleSide::ATTACKER, CreatureID(masterGenie), casterHex, stackCount);
	CStack * ally = addStack(BattleSide::ATTACKER, CreatureID(pikeman), allyHex, stackCount);
	const auto castsBefore = genie->casts.available();

	// the action carries only the target, the server rolls the spell
	BattleAction action;
	action.side = BattleSide::ATTACKER;
	action.stackNumber = genie->unitId();
	action.actionType = EActionType::MONSTER_SPELL;
	action.aimToHex(ally->getPosition());

	ASSERT_TRUE(act(action));
	ASSERT_EQ(server.casts.size(), 1u);

	const auto & cast = server.casts.front().announcement;
	EXPECT_EQ(cast.casterStack, static_cast<si32>(genie->unitId()));
	EXPECT_EQ(cast.affectedCres, std::vector<ui32>{ally->unitId()});
	EXPECT_TRUE(cast.spellID.toSpell()->isPositive());
	EXPECT_TRUE(isAffectedBy(ally, cast.spellID));
	EXPECT_EQ(genie->casts.available(), castsBefore - 1);
}

TEST_F(SpellcastActionTest, adjacentCasterWalksUpToTargetAndCasts)
{
	startGame();
	startBattle();

	const BattleHex castFrom = allyHex.cloneInDirection(BattleHex::LEFT);

	CStack * caster = addStack(BattleSide::ATTACKER, CreatureID(pikeman), casterHex, stackCount);
	CStack * ally = addStack(BattleSide::ATTACKER, CreatureID(pikeman), allyHex, stackCount);
	grantSpell(caster, BonusType::ADJACENT_SPELLCASTER, SpellID::BLESS, 0);
	const auto castsBefore = caster->casts.available();

	ASSERT_TRUE(act(BattleAction::makeWalkAndCast(caster, castFrom, ally, SpellID::BLESS)));
	EXPECT_EQ(caster->getPosition(), castFrom);
	EXPECT_TRUE(isAffectedBy(ally, SpellID::BLESS));
	EXPECT_EQ(caster->casts.available(), castsBefore - 1);
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

	CStack * ally = addStack(BattleSide::ATTACKER, CreatureID(pikeman), allyHex, stackCount);
	teachSpell(attackerSideHero, SpellID::BLESS);
	const auto manaBefore = attackerSideHero->mana;

	// a hero spell needs an active unit of the hero's side, which stays active after the cast
	battle()->activeStack = ally->unitId();

	battle::Target target;
	target.emplace_back(ally);
	ASSERT_TRUE(act(heroSpell(SpellID::BLESS, target)));

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

	CStack * ally = addStack(BattleSide::ATTACKER, CreatureID(pikeman), allyHex, stackCount);
	teachSpell(attackerSideHero, SpellID::BLESS);
	battle()->activeStack = ally->unitId();

	battle::Target target;
	target.emplace_back(ally);
	ASSERT_TRUE(act(heroSpell(SpellID::BLESS, target)));
	const auto manaAfterFirstCast = attackerSideHero->mana;

	EXPECT_FALSE(act(heroSpell(SpellID::BLESS, target)));
	EXPECT_EQ(attackerSideHero->mana, manaAfterFirstCast);
	EXPECT_EQ(battle()->getSide(BattleSide::ATTACKER).castSpellsCount, 1u);
}

TEST_F(SpellcastActionTest, heroTeleportsUnit)
{
	startGame();
	startBattle();

	const BattleHex destination = BattleHex(10, 2);

	CStack * ally = addStack(BattleSide::ATTACKER, CreatureID(pikeman), allyHex, stackCount);
	teachSpell(attackerSideHero, SpellID::TELEPORT);
	battle()->activeStack = ally->unitId();

	// the unit to move, then where to
	battle::Target target;
	target.emplace_back(ally);
	target.emplace_back(destination);
	ASSERT_TRUE(act(heroSpell(SpellID::TELEPORT, target)));

	EXPECT_EQ(ally->getPosition(), destination);
}

TEST_F(SpellcastActionTest, heroSacrificesUnitToRaiseDeadOne)
{
	startGame();
	startBattle();

	CStack * active = addStack(BattleSide::ATTACKER, CreatureID(pikeman), casterHex, stackCount);
	CStack * fallen = addStack(BattleSide::ATTACKER, CreatureID(pikeman), allyHex, stackCount);
	CStack * victim = addStack(BattleSide::ATTACKER, CreatureID(pikeman), BattleHex(7, 8), 2 * stackCount);
	const auto victimId = victim->unitId();

	injure(fallen, fallen->getAvailableHealth());
	ASSERT_FALSE(fallen->alive());

	teachSpell(attackerSideHero, SpellID::SACRIFICE);
	battle()->activeStack = active->unitId();

	// the dead unit to raise, then the living one to give up for it
	battle::Target target;
	target.emplace_back(fallen);
	target.emplace_back(victim);
	ASSERT_TRUE(act(heroSpell(SpellID::SACRIFICE, target)));

	EXPECT_TRUE(fallen->alive());
	EXPECT_GT(fallen->getCount(), 0);

	const auto * victimAfter = battle()->battleGetStackByID(victimId, false);
	EXPECT_TRUE(victimAfter == nullptr || !victimAfter->alive());
}
