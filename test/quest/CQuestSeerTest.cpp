/*
 * CQuestSeerTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "QuestScenarios.h"
#include "QuestTest.h"

#include "../../lib/CPlayerState.h"
#include "../../lib/gameState/CGameState.h"
#include "../../lib/mapObjects/CGCreature.h"
#include "../../lib/mapObjects/CGHeroInstance.h"
#include "../../lib/mapObjects/CQuest.h"
#include "../../lib/mapObjects/MiscObjects.h"

// Seer hut behaviour as a player would experience it: visiting, accepting,
// re-visiting, completing, and the various ways missions can be satisfied.

using namespace QuestScenarios;

class QuestSeerTest : public QuestTest {};

TEST_F(QuestSeerTest, Level_passesAtThreshold)
{
	// A level-5 hero can satisfy the "reach level 3" seer but not "reach level 10".
	auto s = seerLevel();
	ASSERT_NO_FATAL_FAILURE(startWithMap(std::move(s.builder)));

	const auto * hero = findHeroAt(s.heroPos);
	ASSERT_NE(hero, nullptr);
	EXPECT_GE(hero->level, 3u) << "scenario assumes hero is at least level 3 from its starting XP";

	const auto * easy = expectAt<CGSeerHut>(s.questPos);
	const auto * hard = expectAt<CGSeerHut>(s.questPos2);

	EXPECT_TRUE (easy->getQuest().checkQuest(hero));
	EXPECT_FALSE(hard->getQuest().checkQuest(hero));
}

TEST_F(QuestSeerTest, PrimarySkill_passesAtThreshold)
{
	// A hero with attack=5 satisfies the "reach attack 3" seer but not the
	// "reach attack 10" one.
	auto s = seerPrimarySkill();
	ASSERT_NO_FATAL_FAILURE(startWithMap(std::move(s.builder)));

	const auto * hero = findHeroAt(s.heroPos);
	ASSERT_NE(hero, nullptr);

	const auto * easy = expectAt<CGSeerHut>(s.questPos);
	const auto * hard = expectAt<CGSeerHut>(s.questPos2);

	EXPECT_TRUE (easy->getQuest().checkQuest(hero));
	EXPECT_FALSE(hard->getQuest().checkQuest(hero));
}

TEST_F(QuestSeerTest, BringHero_passesWhenHeroPresent)
{
	// A seer asking for hero Tyris is satisfied when Tyris visits, but not when
	// a different hero (Christian) does.
	auto s = seerHero();
	ASSERT_NO_FATAL_FAILURE(startWithMap(std::move(s.builder)));

	const auto * christian = findHeroAt(s.heroPos);
	const auto * tyris     = findHeroAt(s.secondHeroPos);
	ASSERT_NE(christian, nullptr);
	ASSERT_NE(tyris,     nullptr);

	const auto * seer = expectAt<CGSeerHut>(s.questPos);

	EXPECT_FALSE(seer->getQuest().checkQuest(christian)) << "Christian is not the target hero";
	EXPECT_TRUE (seer->getQuest().checkQuest(tyris))     << "Tyris is the target hero";
}

TEST_F(QuestSeerTest, BringPlayer_passesForCorrectColor)
{
	// A seer that asks for the blue player is satisfied only when a blue hero
	// visits — a red hero of the same scenario fails.
	auto s = seerPlayer();
	ASSERT_NO_FATAL_FAILURE(startWithMap(std::move(s.builder)));

	const auto * red  = findHeroByOwner(PlayerColor(0));
	const auto * blue = findHeroByOwner(PlayerColor(1));
	ASSERT_NE(red,  nullptr);
	ASSERT_NE(blue, nullptr);

	const auto * seer = expectAt<CGSeerHut>(s.questPos);

	EXPECT_FALSE(seer->getQuest().checkQuest(red))  << "red is not the target colour";
	EXPECT_TRUE (seer->getQuest().checkQuest(blue)) << "blue is the target colour";
}

TEST_F(QuestSeerTest, KillCreature_satisfiedAfterCreatureRemoved)
{
	// A "slay the dragons" quest stays open while the target stack is alive
	// and switches to "completed" once the player has defeated it.
	auto s = seerKillCreature();
	ASSERT_NO_FATAL_FAILURE(startWithMap(std::move(s.builder)));

	const auto * hero    = findHeroAt(s.heroPos);
	const auto * monster = dynamic_cast<const CGCreature *>(findObjectAt(s.secondHeroPos));
	const auto * seer    = expectAt<CGSeerHut>(s.questPos);
	ASSERT_NE(hero,    nullptr);
	ASSERT_NE(monster, nullptr);

	EXPECT_FALSE(seer->getQuest().checkQuest(hero))
		<< "target still alive, kill-quest should not be satisfied";

	markObjectDestroyed(PlayerColor(0), monster->id);

	EXPECT_TRUE(seer->getQuest().checkQuest(hero))
		<< "target marked destroyed, kill-quest should be satisfied";
}

TEST_F(QuestSeerTest, KillHero_satisfiedAfterHeroDefeated)
{
	auto s = seerKillHero();
	ASSERT_NO_FATAL_FAILURE(startWithMap(std::move(s.builder)));

	const auto * visitor = findHeroAt(s.heroPos);
	const auto * target  = findHeroAt(s.secondHeroPos);
	const auto * seer    = expectAt<CGSeerHut>(s.questPos);
	ASSERT_NE(visitor, nullptr);
	ASSERT_NE(target,  nullptr);

	EXPECT_FALSE(seer->getQuest().checkQuest(visitor));

	markObjectDestroyed(PlayerColor(0), target->id);

	EXPECT_TRUE(seer->getQuest().checkQuest(visitor));
}

// ---- visit-flow tests ---------------------------------------------------

TEST_F(QuestSeerTest, FirstVisit_emitsAddQuest)
{
	auto s = seerHero();
	ASSERT_NO_FATAL_FAILURE(startWithMap(std::move(s.builder)));

	auto * tyris = findHeroAt(s.secondHeroPos);
	auto * seer  = findObjectAt(s.questPos);
	ASSERT_NE(tyris, nullptr);
	ASSERT_NE(seer,  nullptr);

	visit(tyris, seer);

	EXPECT_EQ(gameEventCallback->addedQuests.size(), 1u);
}

TEST_F(QuestSeerTest, RepeatVisit_failedRequirements_showsNextVisitText)
{
	// When the requirements still aren't met, a re-visit shows a new dialog
	// but does not re-add the quest to the log.
	auto s = seerHero();
	ASSERT_NO_FATAL_FAILURE(startWithMap(std::move(s.builder)));

	auto * christian = findHeroAt(s.heroPos);
	auto * seer      = findObjectAt(s.questPos);
	ASSERT_NE(christian, nullptr);
	ASSERT_NE(seer,      nullptr);

	visit(christian, seer);
	const size_t addQuestsAfterFirst = gameEventCallback->addedQuests.size();
	const size_t windowsAfterFirst   = gameEventCallback->infoWindows.size();
	EXPECT_EQ(addQuestsAfterFirst, 1u);
	EXPECT_GE(windowsAfterFirst,   1u);

	visit(christian, seer);
	EXPECT_EQ(gameEventCallback->addedQuests.size(), addQuestsAfterFirst)
		<< "second failed visit must not re-emit AddQuest";
	EXPECT_GT(gameEventCallback->infoWindows.size(), windowsAfterFirst)
		<< "second failed visit should produce its own next-visit info dialog";
}

TEST_F(QuestSeerTest, GrantsRewardOnAcceptance)
{
	// Accepting a "give me your hero" reward pays the visiting hero the
	// promised XP.
	auto s = seerHero();
	ASSERT_NO_FATAL_FAILURE(startWithMap(std::move(s.builder)));

	auto * tyris = findHeroAt(s.secondHeroPos);
	auto * seer  = findObjectAt(s.questPos);
	ASSERT_NE(tyris, nullptr);
	ASSERT_NE(seer,  nullptr);

	const auto xpBefore = tyris->exp;
	visit(tyris, seer);
	ASSERT_EQ(gameEventCallback->blockingDialogs.size(), 1u);
	answerDialog(tyris, /*select reward 0*/ 1);

	EXPECT_GE(tyris->exp, xpBefore + 500)
		<< "seer hut reward of 500 XP should have been granted to the visiting hero";
}

TEST_F(QuestSeerTest, CompletedOneShot_subsequentVisitShowsEmptyText)
{
	// Once a seer is completed, re-visiting only shows the "nothing more for
	// you" message — no quest log entry, no prompt.
	auto s = seerHero();
	ASSERT_NO_FATAL_FAILURE(startWithMap(std::move(s.builder)));

	auto * tyris = findHeroAt(s.secondHeroPos);
	auto * seer  = findObjectAt(s.questPos);
	ASSERT_NE(tyris, nullptr);
	ASSERT_NE(seer,  nullptr);

	visit(tyris, seer);
	answerDialog(tyris, 1);

	// Snapshot after completion, then re-visit.
	const size_t addQuestsBefore     = gameEventCallback->addedQuests.size();
	const size_t blockingsBefore     = gameEventCallback->blockingDialogs.size();
	const size_t windowsBefore       = gameEventCallback->infoWindows.size();

	visit(tyris, seer);

	EXPECT_EQ(gameEventCallback->addedQuests.size(),     addQuestsBefore);
	EXPECT_EQ(gameEventCallback->blockingDialogs.size(), blockingsBefore);
	EXPECT_GT(gameEventCallback->infoWindows.size(),     windowsBefore);
}

TEST_F(QuestSeerTest, BringResources_takesResources)
{
	// Accepting a "bring 5000 gold + 5 wood" quest deducts exactly those
	// amounts from the player's treasury.
	auto s = seerResources();
	ASSERT_NO_FATAL_FAILURE(startWithMap(std::move(s.builder)));
	grantResources(PlayerColor(0), GameResID(GameResID::GOLD), 7000);
	grantResources(PlayerColor(0), GameResID(GameResID::WOOD),   10);

	auto & playerRes = gameState->players.at(PlayerColor(0)).resources;
	const int goldBefore = playerRes[GameResID::GOLD];
	const int woodBefore = playerRes[GameResID::WOOD];

	auto * hero = findHeroAt(s.heroPos);
	auto * seer = findObjectAt(s.questPos);
	ASSERT_NE(hero, nullptr);
	ASSERT_NE(seer, nullptr);

	visit(hero, seer);
	ASSERT_EQ(gameEventCallback->blockingDialogs.size(), 1u);
	answerDialog(hero, 1);

	EXPECT_EQ(goldBefore - playerRes[GameResID::GOLD], 5000);
	EXPECT_EQ(woodBefore - playerRes[GameResID::WOOD],    5);
}

TEST_F(QuestSeerTest, BringArmy_takesCreatures_keepsExtras)
{
	// A "bring 5 Griffins" quest deducts exactly 5 Griffins and leaves other
	// stacks (Royal Griffins) untouched.
	auto s = seerArmy();
	ASSERT_NO_FATAL_FAILURE(startWithMap(std::move(s.builder)));

	auto * hero = findHeroAt(s.heroPos);
	auto * seer = findObjectAt(s.questPos);
	ASSERT_NE(hero, nullptr);
	ASSERT_NE(seer, nullptr);

	// Pre-condition: garrison wired correctly (defends against regressions
	// in TinyH3MBuilder::heroGarrison).
	ASSERT_EQ(hero->getStackCount(SlotID(0)), 10);
	ASSERT_EQ(hero->getStackCount(SlotID(1)),  5);

	visit(hero, seer);
	ASSERT_EQ(gameEventCallback->blockingDialogs.size(), 1u);
	answerDialog(hero, 1);

	EXPECT_EQ(hero->getStackCount(SlotID(0)), 5) << "5 of 10 Griffins should remain";
	EXPECT_EQ(hero->getStackCount(SlotID(1)), 5) << "Royal Griffin stack must be untouched";
}

TEST_F(QuestSeerTest, BringArtifact_completesAndTakesArtifact)
{
	// Handing over a backpack artifact strips it from the hero.
	auto s = seerArtifact();
	ASSERT_NO_FATAL_FAILURE(startWithMap(std::move(s.builder)));

	auto * hero = findHeroAt(s.heroPos);
	auto * seer = findObjectAt(s.questPos);
	ASSERT_NE(hero, nullptr);
	ASSERT_NE(seer, nullptr);

	ASSERT_TRUE(hero->hasArt(kArtifactSash)) << "scenario must place the sash in the backpack";

	visit(hero, seer);
	ASSERT_EQ(gameEventCallback->blockingDialogs.size(), 1u);
	answerDialog(hero, 1);

	EXPECT_FALSE(hero->hasArt(kArtifactSash))
		<< "quest should have stripped the Ambassadors' Sash from the hero";
}

TEST_F(QuestSeerTest, BringArtifact_componentOfAssemblyInBackpack_disassembles)
{
	// Carrying the assembly in the backpack, a hero can hand the requested
	// component to the seer: the assembly is disassembled and the component taken.
	auto s = seerArtifactAssembledInBackpack();
	ASSERT_NO_FATAL_FAILURE(startWithMap(std::move(s.builder)));

	auto * hero = findHeroAt(s.heroPos);
	auto * seer = findObjectAt(s.questPos);
	ASSERT_NE(hero, nullptr);
	ASSERT_NE(seer, nullptr);

	ASSERT_TRUE(hero->hasArt(kArtifactAngelicAlly)) << "Angelic Alliance must be carried pre-visit";

	visit(hero, seer);
	ASSERT_EQ(gameEventCallback->blockingDialogs.size(), 1u);
	answerDialog(hero, 1);

	EXPECT_FALSE(hero->hasArt(kArtifactAngelicAlly))
		<< "Angelic Alliance should have been disassembled";
	EXPECT_FALSE(hero->hasArt(kArtifactHelm))
		<< "Helm of Heavenly Enlightenment (the demanded component) should be taken";
}

TEST_F(QuestSeerTest, BringArtifact_componentOfAssemblyEquipped_disassembles)
{
	// Same as above but the assembly is worn rather than carried in the backpack.
	// Equivalent player intent — the seer asks for a component the hero possesses.
	auto s = seerArtifactAssembledEquipped();
	ASSERT_NO_FATAL_FAILURE(startWithMap(std::move(s.builder)));

	auto * hero = findHeroAt(s.heroPos);
	auto * seer = findObjectAt(s.questPos);
	ASSERT_NE(hero, nullptr);
	ASSERT_NE(seer, nullptr);

	ASSERT_TRUE(hero->hasArt(kArtifactAngelicAlly)) << "Angelic Alliance must be equipped pre-visit";

	visit(hero, seer);
	ASSERT_EQ(gameEventCallback->blockingDialogs.size(), 1u);
	answerDialog(hero, 1);

	EXPECT_FALSE(hero->hasArt(kArtifactAngelicAlly))
		<< "Angelic Alliance should have been disassembled";
	EXPECT_FALSE(hero->hasArt(kArtifactHelm))
		<< "Helm of Heavenly Enlightenment (the demanded component) should be taken";
}

TEST_F(QuestSeerTest, FullArmyRemoval_h3BugSetting_enabledAllowsArmyEmpty)
{
	// With the H3-bug setting on, a seer is allowed to take a hero's last
	// stack, leaving the hero army-less (matches original H3 behaviour).
	overrideSettingBeforeInit(EGameSettings::MAP_OBJECTS_H3_BUG_QUEST_TAKES_ENTIRE_ARMY, true);
	auto s = seerEmptyArmyToggle();
	ASSERT_NO_FATAL_FAILURE(startWithMap(std::move(s.builder)));

	auto * hero = findHeroAt(s.heroPos);
	auto * seer = findObjectAt(s.questPos);
	ASSERT_NE(hero, nullptr);
	ASSERT_NE(seer, nullptr);
	ASSERT_EQ(hero->getStackCount(SlotID(0)), 1);

	visit(hero, seer);
	ASSERT_EQ(gameEventCallback->blockingDialogs.size(), 1u);
	answerDialog(hero, 1);

	EXPECT_FALSE(hero->hasStackAtSlot(SlotID(0)))
		<< "with H3-bug setting enabled, the seer should take the only stack";
}

TEST_F(QuestSeerTest, FullArmyRemoval_disabled_keepsHeroWithOneStack)
{
	// With the H3-bug setting off, a hero always keeps at least one stack
	// when handing over creatures.
	auto s = seerEmptyArmyToggle();
	ASSERT_NO_FATAL_FAILURE(startWithMap(std::move(s.builder)));

	auto * hero = findHeroAt(s.heroPos);
	auto * seer = findObjectAt(s.questPos);
	ASSERT_NE(hero, nullptr);
	ASSERT_NE(seer, nullptr);

	visit(hero, seer);
	if(!gameEventCallback->blockingDialogs.empty())
		answerDialog(hero, 1);

	EXPECT_TRUE(hero->hasStackAtSlot(SlotID(0)))
		<< "without the H3-bug setting the hero must keep at least one stack";
}

TEST_F(QuestSeerTest, Timeout_expiresOnLastDay)
{
	// A quest with a lastDay=7 deadline becomes inaccessible after day 7
	// passes — the seer no longer accepts visits.
	auto s = seerTimeout();
	ASSERT_NO_FATAL_FAILURE(startWithMap(std::move(s.builder)));
	grantResources(PlayerColor(0), GameResID(GameResID::WOOD), 5);

	auto * seer = expectAt<CGSeerHut>(s.questPos);
	EXPECT_EQ(seer->getQuest().lastDay, 7);
	EXPECT_FALSE(seer->getQuest().isCompleted);

	advanceDays(10);
	GameRandomizer randomizer(*gameState);
	seer->newTurn(*gameEventCallback, randomizer);

	EXPECT_TRUE(seer->getQuest().isCompleted)
		<< "after lastDay expires, the quest should be marked complete (i.e. inaccessible)";
}
