/*
 * CQuestSeerMultiTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "QuestTest.h"

#include "../../lib/CPlayerState.h"
#include "../../lib/gameState/CGameState.h"
#include "../../lib/mapObjects/CGHeroInstance.h"
#include "../../lib/mapObjects/Quest.h"

// Multi-quest Seer Hut runtime. A HotA seer hut may carry several
// one-shot and repeatable quests; only one is active at a time (the same for
// every player), advancing as quests complete or expire.

using B = TinyH3M::TinyH3MBuilder;
using QR = std::pair<TinyH3M::Quest, TinyH3M::SeerReward>;

namespace
{
const int3 kHeroPos(5, 5, 0);
const int3 kHero2Pos(6, 6, 0);
const int3 kSeerPos(8, 8, 0);

// HotA v3 map with one player + knight hero and a multi-quest seer hut.
B multiSeer(std::vector<QR> oneShots, std::vector<QR> repeatables = {})
{
	B b(EMapFormat::HOTA);
	b.hotaVersion(3)
		.playerActive(PlayerColor(0))
		.randomTown(int3(2, 2, 0), PlayerColor(0))
		.hero(kHeroPos, HeroTypeID(6), PlayerColor(0)) // Christian (knight)
		.seerHutMulti(kSeerPos, std::move(oneShots), std::move(repeatables));
	return b;
}

// A trivially satisfiable mission (any level-1 hero passes), so tests can drive
// completion without staging resources / armies.
TinyH3M::Quest trivial() { return B::missionLevel(1); }
}

class QuestSeerMultiTest : public QuestTest {};

// ---- loader -----------------------------------------------------------------

TEST_F(QuestSeerMultiTest, Loader_loadsAllQuests)
{
	auto s = multiSeer({{trivial(), B::rewardExperience(500)},
	                    {trivial(), B::rewardResource(GameResID::WOOD, 7)}});
	ASSERT_NO_FATAL_FAILURE(startWithMap(std::move(s)));

	auto * seer = expectAt<SeerHut>(kSeerPos);
	EXPECT_EQ(seer->allQuests().size(), 2u);
}

TEST_F(QuestSeerMultiTest, Loader_loadsRepeatablesAsSeparateEntries)
{
	auto s = multiSeer({{trivial(), B::rewardExperience(500)}},
	                   {{trivial(), B::rewardExperience(100)},
	                    {trivial(), B::rewardResource(GameResID::WOOD, 7)}});
	ASSERT_NO_FATAL_FAILURE(startWithMap(std::move(s)));

	auto * seer = expectAt<SeerHut>(kSeerPos);
	ASSERT_EQ(seer->allQuests().size(), 3u);
	EXPECT_FALSE(seer->allQuests()[0]->repeatedQuest);
	EXPECT_TRUE (seer->allQuests()[1]->repeatedQuest);
	EXPECT_TRUE (seer->allQuests()[2]->repeatedQuest);
}

// ---- active-quest selection / advancement -----------------------------------

TEST_F(QuestSeerMultiTest, OffersFirstOneShotFirst)
{
	auto s = multiSeer({{trivial(), B::rewardExperience(500)},
	                    {trivial(), B::rewardResource(GameResID::WOOD, 7)}});
	ASSERT_NO_FATAL_FAILURE(startWithMap(std::move(s)));

	auto * seer = expectAt<SeerHut>(kSeerPos);
	EXPECT_EQ(&seer->getQuest(), seer->allQuests().front().get());
}

TEST_F(QuestSeerMultiTest, AdvancesToNextOnCompletion)
{
	auto s = multiSeer({{trivial(), B::rewardExperience(500)},
	                    {trivial(), B::rewardResource(GameResID::WOOD, 7)}});
	ASSERT_NO_FATAL_FAILURE(startWithMap(std::move(s)));

	auto * hero = findHeroAt(kHeroPos);
	auto * seer = expectAt<SeerHut>(kSeerPos);

	visit(hero, seer);
	answerDialog(hero, 1); // complete the first quest

	visit(hero, seer); // re-visit triggers advancement to the second quest
	EXPECT_EQ(&seer->getQuest(), seer->allQuests()[1].get());
}

TEST_F(QuestSeerMultiTest, PerQuestRewardIsUsedNotConfigurationInfoIndex)
{
	auto s = multiSeer({{trivial(), B::rewardExperience(500)},
	                    {trivial(), B::rewardResource(GameResID::WOOD, 7)}});
	ASSERT_NO_FATAL_FAILURE(startWithMap(std::move(s)));

	auto * hero  = findHeroAt(kHeroPos);
	auto * seer  = expectAt<SeerHut>(kSeerPos);
	auto & res   = gameState()->players.at(PlayerColor(0)).resources;
	const int woodBefore = res[GameResID::WOOD];

	visit(hero, seer);
	answerDialog(hero, 1); // first quest: experience reward, no wood

	EXPECT_EQ(res[GameResID::WOOD], woodBefore) << "first quest's reward is experience, not wood";

	visit(hero, seer);     // advance to + offer the second quest
	answerDialog(hero, 1); // second quest: 7 wood

	EXPECT_EQ(res[GameResID::WOOD], woodBefore + 7)
		<< "second quest must grant its own reward, not the first quest's";
}

TEST_F(QuestSeerMultiTest, LoopsWithinRepeatablesAfterOneShotsDone)
{
	auto s = multiSeer({{trivial(), B::rewardExperience(500)}},
	                   {{trivial(), B::rewardExperience(100)},
	                    {trivial(), B::rewardResource(GameResID::WOOD, 7)}});
	ASSERT_NO_FATAL_FAILURE(startWithMap(std::move(s)));

	auto * hero = findHeroAt(kHeroPos);
	auto * seer = expectAt<SeerHut>(kSeerPos);

	auto complete = [&]{ visit(hero, seer); answerDialog(hero, 1); };

	complete();                                              // finish the one-shot
	visit(hero, seer);                                       // advance to first repeatable
	EXPECT_EQ(&seer->getQuest(), seer->allQuests()[1].get());

	answerDialog(hero, 1);                                   // finish repeatable #1
	visit(hero, seer);                                       // advance to second repeatable
	EXPECT_EQ(&seer->getQuest(), seer->allQuests()[2].get());

	answerDialog(hero, 1);                                   // finish repeatable #2
	visit(hero, seer);                                       // loop back to the first repeatable
	EXPECT_EQ(&seer->getQuest(), seer->allQuests()[1].get());
}

// ---- expiry -----------------------------------------------------------------

TEST_F(QuestSeerMultiTest, SkipsExpiredByLastDay)
{
	auto first = trivial().withLastDay(3);
	auto s = multiSeer({{first, B::rewardExperience(500)},
	                    {trivial(), B::rewardResource(GameResID::WOOD, 7)}});
	ASSERT_NO_FATAL_FAILURE(startWithMap(std::move(s)));

	auto * seer = expectAt<SeerHut>(kSeerPos);
	EXPECT_EQ(&seer->getQuest(), seer->allQuests().front().get());

	advanceDays(5); // past the first quest's deadline -> newTurn skips it
	EXPECT_EQ(&seer->getQuest(), seer->allQuests()[1].get());
}

TEST_F(QuestSeerMultiTest, SkipsExpiredByDifficultyMismatch)
{
	// First quest is restricted to NORMAL+ difficulties; the game runs on EASY,
	// so it is skipped and the unrestricted second quest becomes active.
	auto restricted = B::missionDifficulty(/*NORMAL only*/ 1 << 1);
	auto s = multiSeer({{restricted, B::rewardExperience(500)},
	                    {trivial(), B::rewardResource(GameResID::WOOD, 7)}});
	ASSERT_NO_FATAL_FAILURE(startWithMap(std::move(s), EMapDifficulty::EASY));

	auto * seer = expectAt<SeerHut>(kSeerPos);
	EXPECT_EQ(&seer->getQuest(), seer->allQuests()[1].get());
}

TEST_F(QuestSeerMultiTest, PermanentlyEmptyWhenAllOneShotAndAllRepeatableExpired)
{
	auto repeatable = trivial().withLastDay(3);
	auto s = multiSeer({{trivial(), B::rewardExperience(500)}},
	                   {{repeatable, B::rewardResource(GameResID::WOOD, 7)}});
	ASSERT_NO_FATAL_FAILURE(startWithMap(std::move(s)));

	auto * hero = findHeroAt(kHeroPos);
	auto * seer = expectAt<SeerHut>(kSeerPos);

	visit(hero, seer);
	answerDialog(hero, 1);   // finish the only one-shot
	advanceDays(5);          // expire the repeatable past its deadline

	const size_t addQuestsBefore = gameEvents().addedQuests.size();
	gameEvents().blockingDialogs.clear();
	visit(hero, seer);       // no offerable quest: empty dialog only

	EXPECT_EQ(gameEvents().addedQuests.size(), addQuestsBefore)
		<< "emptied seer must not register a quest log entry";
	EXPECT_TRUE(gameEvents().blockingDialogs.empty())
		<< "emptied seer must not offer a reward";
}

// ---- global active quest ----------------------------------------------------

TEST_F(QuestSeerMultiTest, ActiveQuestIsGlobalAcrossPlayers)
{
	// Two players share one seer hut. When the first player completes the active
	// quest, the second player sees the advanced quest - not the original one.
	B b(EMapFormat::HOTA);
	b.hotaVersion(3)
		.playerActive(PlayerColor(0))
		.playerActive(PlayerColor(1))
		.randomTown(int3(2, 2, 0), PlayerColor(0))
		.randomTown(int3(3, 3, 0), PlayerColor(1))
		.hero(kHeroPos,  HeroTypeID(6), PlayerColor(0))
		.hero(kHero2Pos, HeroTypeID(7), PlayerColor(1))
		.seerHutMulti(kSeerPos, {{trivial(), B::rewardExperience(500)},
		                         {trivial(), B::rewardResource(GameResID::WOOD, 7)}});
	ASSERT_NO_FATAL_FAILURE(startWithMap(std::move(b)));

	auto * hero1 = findHeroAt(kHeroPos);
	auto * hero2 = findHeroAt(kHero2Pos);
	auto * seer  = expectAt<SeerHut>(kSeerPos);
	ASSERT_NE(hero1, nullptr);
	ASSERT_NE(hero2, nullptr);

	visit(hero1, seer);
	answerDialog(hero1, 1);   // player 0 completes the first quest

	visit(hero2, seer);       // player 1 visits -> sees the advanced (second) quest
	EXPECT_EQ(&seer->getQuest(), seer->allQuests()[1].get());
}

// ---- repeatable -------------------------------------------------------------

TEST_F(QuestSeerMultiTest, Repeatable_canBeCompletedRepeatedly)
{
	// A lone repeatable quest grants its reward on every completion.
	auto s = multiSeer({}, {{trivial(), B::rewardExperience(500)}});
	ASSERT_NO_FATAL_FAILURE(startWithMap(std::move(s)));

	auto * hero = findHeroAt(kHeroPos);
	auto * seer = expectAt<SeerHut>(kSeerPos);

	const auto xpStart = hero->exp;
	visit(hero, seer);
	answerDialog(hero, 1);
	const auto xpAfterFirst = hero->exp;
	EXPECT_GE(xpAfterFirst, xpStart + 500);

	visit(hero, seer);
	answerDialog(hero, 1);
	EXPECT_GE(hero->exp, xpAfterFirst + 500)
		<< "a repeatable quest must grant its reward again on the next completion";

	EXPECT_FALSE(seer->getQuest().isCompleted) << "repeatable quests never set the completed flag";
}

// ---- quest-less source ------------------------------------------------------

TEST_F(QuestSeerMultiTest, QuestlessSeer_isVisitableAndGetQuestThrows)
{
	// A seer hut carrying zero quests is a valid empty source: it owns no quests,
	// getQuest() has no active quest to return, and a visit must not crash.
	auto s = multiSeer({}, {});
	ASSERT_NO_FATAL_FAILURE(startWithMap(std::move(s)));

	auto * hero = findHeroAt(kHeroPos);
	auto * seer = expectAt<SeerHut>(kSeerPos);
	ASSERT_NE(hero, nullptr);

	EXPECT_TRUE(seer->allQuests().empty());
	EXPECT_ANY_THROW((void)seer->getQuest()) << "no active quest - getQuest() must throw";

	const size_t addQuestsBefore = gameEvents().addedQuests.size();
	ASSERT_NO_FATAL_FAILURE(visit(hero, seer)); // shows only the empty-seer dialog
	EXPECT_EQ(gameEvents().addedQuests.size(), addQuestsBefore);
	EXPECT_TRUE(gameEvents().blockingDialogs.empty());
}
