/*
 * BattleHexTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "../lib/bonuses/CBonusSystemNode.h"
#include "../lib/bonuses/BonusEnum.h"
#include "../lib/bonuses/Limiters.h"
#include "../lib/bonuses/Propagators.h"
#include "../lib/bonuses/Updaters.h"

namespace test
{

using namespace ::testing;

class TestBonusSystemNode : public CBonusSystemNode
{
public:
	using CBonusSystemNode::CBonusSystemNode;

	MOCK_CONST_METHOD0(getOwner, PlayerColor());

	void setPlayer(PlayerColor player)
	{
		EXPECT_CALL(*this, getOwner()).WillRepeatedly(Return(player));
	}

	void giveIntimidationSkill()
	{
		auto intimidationBonus = std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::MORALE, BonusSource::SECONDARY_SKILL, -1, SecondarySkill(0));
		intimidationBonus->propagator = std::make_shared<CPropagatorNodeType>(BonusNodeType::BATTLE_WIDE);
		intimidationBonus->propagationUpdater = std::make_shared<OwnerUpdater>();
		intimidationBonus->limiter = std::make_shared<OppositeSideLimiter>();

		addNewBonus(intimidationBonus);
	}
};

class BonusSystemTest : public Test
{
protected:

	CBonusSystemNode global{BonusNodeType::GLOBAL_EFFECTS};
	CBonusSystemNode battle{BonusNodeType::BATTLE_WIDE};

	TestBonusSystemNode playerRed{BonusNodeType::PLAYER};
	TestBonusSystemNode playerBlue{BonusNodeType::PLAYER};

	TestBonusSystemNode heroAine{BonusNodeType::HERO};
	TestBonusSystemNode heroBron{BonusNodeType::HERO};

	CBonusSystemNode artifactTypeRing{BonusNodeType::ARTIFACT};
	CBonusSystemNode artifactTypeSword{BonusNodeType::ARTIFACT};
	CBonusSystemNode artifactTypeArmor{BonusNodeType::ARTIFACT};
	CBonusSystemNode artifactTypeOrb{BonusNodeType::ARTIFACT};
	CBonusSystemNode artifactTypeLegion{BonusNodeType::ARTIFACT};

	CBonusSystemNode creatureAngel{BonusNodeType::CREATURE};
	CBonusSystemNode creatureDevil{BonusNodeType::CREATURE};
	CBonusSystemNode creaturePikeman{BonusNodeType::CREATURE};

	void startBattle()
	{
		heroAine.attachTo(battle);
		heroBron.attachTo(battle);
	}

	void SetUp() override
	{
		playerRed.attachTo(global);
		playerBlue.attachTo(global);

		heroAine.attachTo(playerRed);
		heroBron.attachTo(playerBlue);

		playerRed.setPlayer(PlayerColor(0));
		playerBlue.setPlayer(PlayerColor(1));
		heroAine.setPlayer(PlayerColor(0));
		heroBron.setPlayer(PlayerColor(1));

		auto spellpowerBonus = std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::PRIMARY_SKILL, BonusSource::ARTIFACT, 1, ArtifactID(0), PrimarySkill::SPELL_POWER);
		auto spellpowerBonus4 = std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::PRIMARY_SKILL, BonusSource::ARTIFACT, 4, ArtifactID(1), PrimarySkill::SPELL_POWER);
		auto attackBonus = std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::PRIMARY_SKILL, BonusSource::ARTIFACT, 1, ArtifactID(2), PrimarySkill::ATTACK);
		auto blockMagicBonus = std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::BLOCK_ALL_MAGIC, BonusSource::ARTIFACT, 1, ArtifactID(3));
		auto legionBonus = std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::CREATURE_GROWTH, BonusSource::ARTIFACT, 4, ArtifactID(4), BonusCustomSubtype::creatureLevel(3));

		auto luckBonusEnemies = std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::LUCK, BonusSource::CREATURE_ABILITY, -1, CreatureID(0));
		auto moraleBonusAllies = std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::MORALE, BonusSource::CREATURE_ABILITY, 1, CreatureID(1));

		luckBonusEnemies->propagator = std::make_shared<CPropagatorNodeType>(BonusNodeType::BATTLE_WIDE);
		luckBonusEnemies->propagationUpdater = std::make_shared<OwnerUpdater>();
		luckBonusEnemies->limiter = std::make_shared<OppositeSideLimiter>();
		luckBonusEnemies->stacking = "devil";
		moraleBonusAllies->propagator = std::make_shared<CPropagatorNodeType>(BonusNodeType::ARMY);
		moraleBonusAllies->stacking = "angel";
		blockMagicBonus->propagator = std::make_shared<CPropagatorNodeType>(BonusNodeType::BATTLE_WIDE);
		legionBonus->propagator = std::make_shared<CPropagatorNodeType>(BonusNodeType::TOWN_AND_VISITOR);

		artifactTypeRing.addNewBonus(spellpowerBonus);
		artifactTypeSword.addNewBonus(attackBonus);
		artifactTypeArmor.addNewBonus(spellpowerBonus4);
		artifactTypeOrb.addNewBonus(blockMagicBonus);
		artifactTypeLegion.addNewBonus(legionBonus);
		creatureAngel.addNewBonus(moraleBonusAllies);
		creatureDevil.addNewBonus(luckBonusEnemies);
	}
};

TEST_F(BonusSystemTest, multipleBonusSources)
{
	CBonusSystemNode ring1{BonusNodeType::ARTIFACT_INSTANCE};
	CBonusSystemNode ring2{BonusNodeType::ARTIFACT_INSTANCE};
	CBonusSystemNode armor{BonusNodeType::ARTIFACT_INSTANCE};

	ring1.attachToSource(artifactTypeRing);
	ring2.attachToSource(artifactTypeRing);
	armor.attachToSource(artifactTypeArmor);

	EXPECT_EQ(heroAine.valOfBonuses(BonusType::PRIMARY_SKILL, PrimarySkill::SPELL_POWER), 0);

	// single bonus produces single effect
	heroAine.attachToSource(ring1);
	EXPECT_EQ(heroAine.valOfBonuses(BonusType::PRIMARY_SKILL, PrimarySkill::SPELL_POWER), 1);

	// same bonus applied twice produces single effect
	heroAine.attachToSource(ring2);
	EXPECT_EQ(heroAine.valOfBonuses(BonusType::PRIMARY_SKILL, PrimarySkill::SPELL_POWER), 1);

	// different bonuses stack
	heroAine.attachToSource(armor);
	EXPECT_EQ(heroAine.valOfBonuses(BonusType::PRIMARY_SKILL, PrimarySkill::SPELL_POWER), 1+4);

	heroAine.detachFromSource(ring1);
	heroAine.detachFromSource(ring2);
	heroAine.detachFromSource(armor);
}

TEST_F(BonusSystemTest, battlewidePropagationToAllies)
{
	CBonusSystemNode angel1{BonusNodeType::STACK_INSTANCE};
	CBonusSystemNode angel2{BonusNodeType::STACK_INSTANCE};

	CBonusSystemNode pikemanAlly{BonusNodeType::STACK_INSTANCE};
	CBonusSystemNode pikemanEnemy{BonusNodeType::STACK_INSTANCE};

	angel1.attachToSource(creatureAngel);
	angel2.attachToSource(creatureAngel);

	pikemanAlly.attachToSource(creaturePikeman);
	pikemanEnemy.attachToSource(creaturePikeman);

	pikemanAlly.attachTo(heroAine);
	pikemanEnemy.attachTo(heroBron);

	startBattle();

	EXPECT_EQ(heroAine.valOfBonuses(BonusType::MORALE), 0);
	EXPECT_EQ(heroBron.valOfBonuses(BonusType::MORALE), 0);
	EXPECT_EQ(pikemanAlly.valOfBonuses(BonusType::MORALE), 0);
	EXPECT_EQ(pikemanEnemy.valOfBonuses(BonusType::MORALE), 0);

	angel1.attachTo(heroAine);
	EXPECT_EQ(heroAine.valOfBonuses(BonusType::MORALE), 1);
	EXPECT_EQ(heroBron.valOfBonuses(BonusType::MORALE), 0);
	EXPECT_EQ(pikemanAlly.valOfBonuses(BonusType::MORALE), 1);
	EXPECT_EQ(pikemanEnemy.valOfBonuses(BonusType::MORALE), 0);

	angel2.attachTo(heroAine);
	EXPECT_EQ(heroAine.valOfBonuses(BonusType::MORALE), 1);
	EXPECT_EQ(heroBron.valOfBonuses(BonusType::MORALE), 0);
	EXPECT_EQ(pikemanAlly.valOfBonuses(BonusType::MORALE), 1);
	EXPECT_EQ(pikemanEnemy.valOfBonuses(BonusType::MORALE), 0);
}

TEST_F(BonusSystemTest, battlewidePropagationToAll)
{
	CBonusSystemNode orb{BonusNodeType::ARTIFACT_INSTANCE};
	orb.attachToSource(artifactTypeOrb);

	heroAine.attachToSource(orb);
	startBattle();

	EXPECT_TRUE(heroAine.hasBonusOfType(BonusType::BLOCK_ALL_MAGIC));
	EXPECT_TRUE(heroBron.hasBonusOfType(BonusType::BLOCK_ALL_MAGIC));

	heroAine.detachFromSource(orb);
}

TEST_F(BonusSystemTest, battlewidePropagationToEnemies)
{
	TestBonusSystemNode devil1{BonusNodeType::STACK_INSTANCE};
	TestBonusSystemNode devil2{BonusNodeType::STACK_INSTANCE};

	TestBonusSystemNode pikemanAlly{BonusNodeType::STACK_INSTANCE};
	TestBonusSystemNode pikemanEnemy{BonusNodeType::STACK_INSTANCE};

	devil1.setPlayer(PlayerColor(0));
	devil2.setPlayer(PlayerColor(0));
	pikemanAlly.setPlayer(PlayerColor(0));
	pikemanEnemy.setPlayer(PlayerColor(1));

	devil1.attachToSource(creatureDevil);
	devil2.attachToSource(creatureDevil);

	pikemanAlly.attachToSource(creaturePikeman);
	pikemanEnemy.attachToSource(creaturePikeman);

	pikemanAlly.attachTo(heroAine);
	pikemanEnemy.attachTo(heroBron);

	startBattle();

	EXPECT_EQ(heroAine.valOfBonuses(BonusType::LUCK), 0);
	EXPECT_EQ(heroBron.valOfBonuses(BonusType::LUCK), 0);
	EXPECT_EQ(pikemanAlly.valOfBonuses(BonusType::LUCK), 0);
	EXPECT_EQ(pikemanEnemy.valOfBonuses(BonusType::LUCK), 0);

	devil1.attachTo(heroAine);
	EXPECT_EQ(heroAine.valOfBonuses(BonusType::LUCK), 0);
	EXPECT_EQ(heroBron.valOfBonuses(BonusType::LUCK), -1);
	EXPECT_EQ(pikemanAlly.valOfBonuses(BonusType::LUCK), 0);
	EXPECT_EQ(pikemanEnemy.valOfBonuses(BonusType::LUCK), -1);

	devil2.attachTo(heroAine);
	EXPECT_EQ(heroAine.valOfBonuses(BonusType::LUCK), 0);
	EXPECT_EQ(heroBron.valOfBonuses(BonusType::LUCK), -1);
	EXPECT_EQ(pikemanAlly.valOfBonuses(BonusType::LUCK), 0);
	EXPECT_EQ(pikemanEnemy.valOfBonuses(BonusType::LUCK), -1);

	EXPECT_EQ(heroAine.valOfBonuses(BonusType::LUCK), 0);
	EXPECT_EQ(heroBron.valOfBonuses(BonusType::LUCK), -1);
	EXPECT_EQ(pikemanAlly.valOfBonuses(BonusType::LUCK), 0);
	EXPECT_EQ(pikemanEnemy.valOfBonuses(BonusType::LUCK), -1);
}

TEST_F(BonusSystemTest, battlewideSkillPropagationToEnemies)
{
	TestBonusSystemNode pikemanAlly{BonusNodeType::STACK_INSTANCE};
	TestBonusSystemNode pikemanEnemy{BonusNodeType::STACK_INSTANCE};

	// #5975 - skill that affects only enemies in battle is broken,
	// but only if hero has *any* artifact (including obligatory catapult/spellbook, so always)
	CBonusSystemNode armor{BonusNodeType::ARTIFACT_INSTANCE};
	armor.attachToSource(artifactTypeArmor);
	heroAine.attachToSource(armor);

	pikemanAlly.setPlayer(PlayerColor(0));
	pikemanEnemy.setPlayer(PlayerColor(1));

	pikemanAlly.attachToSource(creaturePikeman);
	pikemanEnemy.attachToSource(creaturePikeman);

	heroAine.giveIntimidationSkill();

	pikemanAlly.attachTo(heroAine);
	pikemanEnemy.attachTo(heroBron);

	startBattle();

	EXPECT_EQ(heroAine.valOfBonuses(BonusType::MORALE), 0);
	EXPECT_EQ(heroBron.valOfBonuses(BonusType::MORALE), -1);
	EXPECT_EQ(pikemanAlly.valOfBonuses(BonusType::MORALE), 0);
	EXPECT_EQ(pikemanEnemy.valOfBonuses(BonusType::MORALE), -1);

	heroAine.nodeHasChanged();

	EXPECT_EQ(heroAine.valOfBonuses(BonusType::MORALE), 0);
	EXPECT_EQ(heroBron.valOfBonuses(BonusType::MORALE), -1);
	EXPECT_EQ(pikemanAlly.valOfBonuses(BonusType::MORALE), 0);
	EXPECT_EQ(pikemanEnemy.valOfBonuses(BonusType::MORALE), -1);
}

TEST_F(BonusSystemTest, legionPieces)
{
	TestBonusSystemNode townAndVisitor{BonusNodeType::TOWN_AND_VISITOR};
	TestBonusSystemNode town{BonusNodeType::TOWN_AND_VISITOR};
	town.attachTo(townAndVisitor);

	CBonusSystemNode legion{BonusNodeType::ARTIFACT_INSTANCE};
	legion.attachToSource(artifactTypeLegion);

	heroAine.attachToSource(legion);

	heroAine.attachTo(townAndVisitor);
	EXPECT_EQ(town.valOfBonuses(BonusType::CREATURE_GROWTH, BonusCustomSubtype::creatureLevel(3)), 4);

	heroAine.detachFromSource(legion);
	EXPECT_EQ(town.valOfBonuses(BonusType::CREATURE_GROWTH, BonusCustomSubtype::creatureLevel(3)), 0);

	heroAine.attachToSource(legion);
	EXPECT_EQ(town.valOfBonuses(BonusType::CREATURE_GROWTH, BonusCustomSubtype::creatureLevel(3)), 4);

	heroAine.detachFrom(townAndVisitor);
	EXPECT_EQ(town.valOfBonuses(BonusType::CREATURE_GROWTH, BonusCustomSubtype::creatureLevel(3)), 0);

	heroAine.detachFromSource(legion);
}

}
