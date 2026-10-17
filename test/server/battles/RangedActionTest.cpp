/*
 * RangedActionTest.cpp, part of VCMI engine
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
#include "../../../lib/battle/actions/BattleActionType.h"
#include "../../../lib/bonuses/Bonus.h"
#include "../../../lib/gameState/CGameState.h"
#include "../../../lib/texts/CGeneralTextHandler.h"

namespace
{
// creatures
constexpr int pikeman = 0;
constexpr int archer = 2; // one shot per turn
constexpr int cyclop = 94; // shoots at walls
}

/// Actions a unit makes from where it stands: shooting, and what the first aid tent and the
/// catapult do, with the previews the client shows for them.
class RangedActionTest : public BattleTestFixture
{
public:
	static constexpr int32_t stackCount = 10;

	static inline const BattleHex shooterHex = BattleHex(3, 5);
	static inline const BattleHex targetHex = BattleHex(12, 5);
};

TEST_F(RangedActionTest, shotAtMeleeUnitUsesOneShotWithoutRetaliation)
{
	startGame();
	startBattle();

	CStack * shooter = addStack(BattleSide::ATTACKER, CreatureID(archer), shooterHex, stackCount);
	CStack * target = addStack(BattleSide::DEFENDER, CreatureID(pikeman), targetHex, stackCount);

	const auto shooterHealth = shooter->getAvailableHealth();
	const auto targetHealth = target->getAvailableHealth();
	const auto shotsBefore = shooter->shots.available();

	ASSERT_TRUE(act(BattleAction::makeShotAttack(shooter, target)));
	EXPECT_LT(target->getAvailableHealth(), targetHealth);
	EXPECT_EQ(shooter->getAvailableHealth(), shooterHealth);
	EXPECT_EQ(shooter->shots.available(), shotsBefore - 1);
}

TEST_F(RangedActionTest, shooterNextToEnemyCannotShoot)
{
	startGame();
	startBattle();

	CStack * shooter = addStack(BattleSide::ATTACKER, CreatureID(archer), shooterHex, stackCount);
	CStack * target = addStack(BattleSide::DEFENDER, CreatureID(pikeman), targetHex, stackCount);
	addStack(BattleSide::DEFENDER, CreatureID(pikeman), shooterHex.cloneInDirection(BattleHex::RIGHT), stackCount);

	const auto targetHealth = target->getAvailableHealth();
	const auto shotsBefore = shooter->shots.available();

	EXPECT_FALSE(act(BattleAction::makeShotAttack(shooter, target)));
	EXPECT_EQ(target->getAvailableHealth(), targetHealth);
	EXPECT_EQ(shooter->shots.available(), shotsBefore);
}

TEST_F(RangedActionTest, shotAtEmptyHexIsRefused)
{
	startGame();
	startBattle();

	const CStack * shooter = addStack(BattleSide::ATTACKER, CreatureID(archer), shooterHex, stackCount);
	addStack(BattleSide::DEFENDER, CreatureID(pikeman), targetHex, stackCount);

	const auto shotsBefore = shooter->shots.available();

	EXPECT_FALSE(act(BattleAction::makeShotAttack(shooter, BattleHex(8, 5))));
	EXPECT_EQ(shooter->shots.available(), shotsBefore);
}

TEST_F(RangedActionTest, shotAtFriendlyUnitIsRefused)
{
	startGame();
	startBattle();

	const CStack * shooter = addStack(BattleSide::ATTACKER, CreatureID(archer), shooterHex, stackCount);
	const CStack * friendly = addStack(BattleSide::ATTACKER, CreatureID(pikeman), BattleHex(8, 5), stackCount);
	addStack(BattleSide::DEFENDER, CreatureID(pikeman), targetHex, stackCount);

	const auto friendlyHealth = friendly->getAvailableHealth();

	EXPECT_FALSE(act(BattleAction::makeShotAttack(shooter, friendly)));
	EXPECT_EQ(friendly->getAvailableHealth(), friendlyHealth);
}

TEST_F(RangedActionTest, targetWithRangedRetaliationShootsBack)
{
	startGame();
	startBattle();

	CStack * shooter = addStack(BattleSide::ATTACKER, CreatureID(archer), shooterHex, stackCount);
	CStack * target = addStack(BattleSide::DEFENDER, CreatureID(archer), targetHex, stackCount);
	target->addNewBonus(std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::RANGED_RETALIATION, BonusSource::OTHER, 0, BonusSourceID()));

	const auto shooterHealth = shooter->getAvailableHealth();
	const auto targetShots = target->shots.available();

	ASSERT_TRUE(act(BattleAction::makeShotAttack(shooter, target)));
	EXPECT_LT(shooter->getAvailableHealth(), shooterHealth);
	EXPECT_EQ(target->shots.available(), targetShots - 1);
}

TEST_F(RangedActionTest, firstAidTentHealsWoundedStack)
{
	startGame();
	startBattle();

	CStack * tent = addStack(BattleSide::ATTACKER, CreatureID(CreatureID::FIRST_AID_TENT), BattleHex(2, 8), 1);
	CStack * patient = addStack(BattleSide::ATTACKER, CreatureID(pikeman), shooterHex, stackCount);

	injure(patient, patient->getMaxHealth() / 2);
	const auto woundedHealth = patient->getAvailableHealth();
	ASSERT_LT(woundedHealth, patient->getMaxHealth() * stackCount);

	ASSERT_TRUE(act(BattleAction::makeHeal(tent, patient)));
	EXPECT_GT(patient->getAvailableHealth(), woundedHealth);
	EXPECT_LE(patient->getAvailableHealth(), patient->getMaxHealth() * stackCount);
}

TEST_F(RangedActionTest, firstAidTentRefusesEnemyAndUnwoundedStacks)
{
	startGame();
	startBattle();

	const CStack * tent = addStack(BattleSide::ATTACKER, CreatureID(CreatureID::FIRST_AID_TENT), BattleHex(2, 8), 1);
	const CStack * unwounded = addStack(BattleSide::ATTACKER, CreatureID(pikeman), shooterHex, stackCount);
	const CStack * enemy = addStack(BattleSide::DEFENDER, CreatureID(pikeman), targetHex, stackCount);

	injure(enemy, enemy->getMaxHealth() / 2);
	const auto enemyHealth = enemy->getAvailableHealth();

	EXPECT_FALSE(act(BattleAction::makeHeal(tent, enemy)));
	EXPECT_EQ(enemy->getAvailableHealth(), enemyHealth);
	EXPECT_FALSE(act(BattleAction::makeHeal(tent, unwounded)));
}

TEST_F(RangedActionTest, firstAidTentWithoutTargetHealsMostWoundedCreature)
{
	startGame();
	startBattle();

	CStack * tent = addStack(BattleSide::ATTACKER, CreatureID(CreatureID::FIRST_AID_TENT), BattleHex(2, 8), 1);
	CStack * ballista = addStack(BattleSide::ATTACKER, CreatureID(CreatureID::BALLISTA), BattleHex(2, 2), 1);
	CStack * lightlyWounded = addStack(BattleSide::ATTACKER, CreatureID(pikeman), shooterHex, stackCount);
	CStack * mostWounded = addStack(BattleSide::ATTACKER, CreatureID(pikeman), BattleHex(4, 2), stackCount);
	CStack * enemy = addStack(BattleSide::DEFENDER, CreatureID(pikeman), targetHex, stackCount);

	// war machines and enemies lost more, but first aid can't heal them
	injure(tent, 50);
	injure(ballista, 100);
	injure(enemy, 5);
	injure(lightlyWounded, 1);
	injure(mostWounded, 3);

	const auto healthOf = [](const std::vector<CStack *> & stacks)
	{
		std::vector<int64_t> result;
		for(const auto * stack : stacks)
			result.push_back(stack->getAvailableHealth());
		return result;
	};
	const std::vector<CStack *> notHealed = {tent, ballista, enemy, lightlyWounded};
	const auto notHealedHealth = healthOf(notHealed);
	const auto woundedHealth = mostWounded->getAvailableHealth();

	ASSERT_TRUE(act(BattleAction::makeHeal(tent)));
	EXPECT_GT(mostWounded->getAvailableHealth(), woundedHealth);
	EXPECT_EQ(healthOf(notHealed), notHealedHealth);
}

TEST_F(RangedActionTest, firstAidTentWithoutTargetSkipsTurnWhenNoneIsWounded)
{
	startGame();
	startBattle();

	const CStack * tent = addStack(BattleSide::ATTACKER, CreatureID(CreatureID::FIRST_AID_TENT), BattleHex(2, 8), 1);
	addStack(BattleSide::ATTACKER, CreatureID(pikeman), shooterHex, stackCount);

	EXPECT_TRUE(act(BattleAction::makeHeal(tent)));
}

TEST_F(RangedActionTest, catapultShootsAtWalls)
{
	startGame();
	startSiege();

	CStack * catapult = addStack(BattleSide::ATTACKER, CreatureID(CreatureID::CATAPULT), BattleHex(2, 8), 1);
	// the town has no garrison, and a side without units would lose the battle after the shot
	addStack(BattleSide::DEFENDER, CreatureID(pikeman), targetHex, stackCount);

	BattleAction shot = BattleAction::makeCatapultShot(catapult);
	shot.aimToHex(battle()->wallPartToBattleHex(EWallPart::UPPER_WALL));

	ASSERT_TRUE(act(shot));

	// the hero's own catapult takes its automatic turn right after, so only this one's shots count
	std::vector<CatapultAttack> shots;
	for(const auto & attack : server.catapultAttacks)
		if(attack.attacker == static_cast<int>(catapult->unitId()))
			shots.push_back(attack);

	// a catapult without Ballistics makes one shot, which may still miss the wall it aimed at
	ASSERT_EQ(shots.size(), 1u);
	EXPECT_NE(shots.front().attackedPart, EWallPart::INVALID);
}

TEST_F(RangedActionTest, catapultShotAtNonWallHexIsRefused)
{
	startGame();
	startSiege();

	const CStack * catapult = addStack(BattleSide::ATTACKER, CreatureID(CreatureID::CATAPULT), BattleHex(2, 8), 1);
	addStack(BattleSide::DEFENDER, CreatureID(pikeman), targetHex, stackCount);

	BattleAction shot = BattleAction::makeCatapultShot(catapult);
	shot.aimToHex(BattleHex(8, 5));

	EXPECT_FALSE(act(shot));
	// the hero's own catapult takes its automatic turn right after
	for(const auto & attack : server.catapultAttacks)
		EXPECT_NE(attack.attacker, static_cast<int>(catapult->unitId()));
}

TEST_F(RangedActionTest, wallShotOutsideSiegeIsRefused)
{
	startGame();
	startBattle();

	const CStack * cyclops = addStack(BattleSide::ATTACKER, CreatureID(cyclop), shooterHex, stackCount);
	addStack(BattleSide::DEFENDER, CreatureID(pikeman), targetHex, stackCount);

	EXPECT_FALSE(act(BattleAction::makeCatapultShot(cyclops)));
}

TEST_F(RangedActionTest, catapultOptionTargetsStandingWall)
{
	startGame();
	startSiege();

	const CStack * catapult = addStack(BattleSide::ATTACKER, CreatureID(CreatureID::CATAPULT), BattleHex(2, 8), 1);
	addStack(BattleSide::DEFENDER, CreatureID(pikeman), targetHex, stackCount);

	const auto options = optionsOf(catapult);
	ASSERT_EQ(options.size(), 1);
	const auto & option = options.front();

	const BattleHex wall = battle()->wallPartToBattleHex(EWallPart::UPPER_WALL);
	EXPECT_EQ(preview(option, {catapult, wall}).cursor, "combatShootCatapult");
	EXPECT_EQ(preview(option, {catapult, BattleHex(8, 5)}).cursor, "combatBlocked");
}

TEST_F(RangedActionTest, shotPreviewShowsShotsLeftAndDamage)
{
	startGame();
	startBattle();

	const CStack * shooter = addStack(BattleSide::ATTACKER, CreatureID(archer), shooterHex, stackCount);
	const CStack * target = addStack(BattleSide::DEFENDER, CreatureID(pikeman), BattleHex(8, 5), stackCount);

	const auto shown = previewOf(shooter, EActionType::SHOOT, target->getPosition());
	EXPECT_EQ(shown.cursor, "combatShoot");
	// an archer has 12 shots
	EXPECT_THAT(textIDsOf(shown.statusText), ::testing::AllOf(
		::testing::Contains(::testing::StartsWith("vcmi.battleWindow.damageEstimation.ranged")),
		::testing::Contains(pluralNameOf(CreatureID(pikeman))),
		::testing::Contains("vcmi.battleWindow.damageEstimation.shots"),
		::testing::Contains("vcmi.battleWindow.damageEstimation.damage")));
	EXPECT_THAT(numbersOf(shown.statusText), ::testing::Contains(12));

	const auto option = optionOf(shooter, EActionType::SHOOT);
	EXPECT_TRUE(act(option.type->build(*battle(), option, {shooter, target->getPosition()}).value()));
}

TEST_F(RangedActionTest, distantTargetShowsPenaltyCursor)
{
	startGame();
	startBattle();

	const CStack * shooter = addStack(BattleSide::ATTACKER, CreatureID(archer), BattleHex(1, 5), stackCount);
	const CStack * target = addStack(BattleSide::DEFENDER, CreatureID(pikeman), BattleHex(15, 5), stackCount);

	EXPECT_EQ(previewOf(shooter, EActionType::SHOOT, target->getPosition()).cursor, "combatShootPenalty");
}

TEST_F(RangedActionTest, shotPreviewAtEmptyHexIsBlocked)
{
	startGame();
	startBattle();

	const CStack * shooter = addStack(BattleSide::ATTACKER, CreatureID(archer), shooterHex, stackCount);
	addStack(BattleSide::DEFENDER, CreatureID(pikeman), targetHex, stackCount);

	EXPECT_EQ(previewOf(shooter, EActionType::SHOOT, BattleHex(8, 5)).cursor, "combatBlocked");
}

TEST_F(RangedActionTest, healPreviewShowsWoundedStack)
{
	startGame();
	startBattle();

	const CStack * tent = addStack(BattleSide::ATTACKER, CreatureID(CreatureID::FIRST_AID_TENT), BattleHex(2, 8), 1);
	const CStack * patient = addStack(BattleSide::ATTACKER, CreatureID(pikeman), shooterHex, stackCount);

	EXPECT_EQ(previewOf(tent, EActionType::STACK_HEAL, patient->getPosition()).cursor, "combatBlocked");

	injure(patient, patient->getMaxHealth() / 2);

	const auto shown = previewOf(tent, EActionType::STACK_HEAL, patient->getPosition());
	EXPECT_EQ(shown.cursor, "combatHeal");
	EXPECT_THAT(textIDsOf(shown.statusText), ::testing::IsSupersetOf(std::vector<std::string>{"core.genrltxt.419", pluralNameOf(CreatureID(pikeman))}));
}
