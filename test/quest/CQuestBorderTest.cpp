/*
 * CQuestBorderTest.cpp, part of VCMI engine
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
#include "../../lib/mapObjects/CGHeroInstance.h"
#include "../../lib/mapObjects/CQuest.h"

// The H3 key-and-gate puzzle as seen by the player: keymaster tents grant
// access, border gates block until the right colour is held, border guards
// can be torn down on demand.

using namespace QuestScenarios;

class QuestBorderTest : public QuestTest {};

TEST_F(QuestBorderTest, BorderGate_BeforeKeymaster_passableForFalse)
{
	// A border gate is impassable until the player has visited the matching
	// keymaster.
	auto s = questBorderGate();
	ASSERT_NO_FATAL_FAILURE(startWithMap(std::move(s.builder)));

	const auto * gate = findObjectAt(s.questPos2);
	ASSERT_NE(gate, nullptr);
	EXPECT_FALSE(gate->passableFor(PlayerColor(0)));
}

TEST_F(QuestBorderTest, BorderGate_AfterKeymaster_passableForTrue)
{
	// Once a hero visits the matching keymaster, the border gate of that
	// colour becomes passable for the player.
	auto s = questBorderGate();
	ASSERT_NO_FATAL_FAILURE(startWithMap(std::move(s.builder)));

	auto * hero       = findHeroAt(s.heroPos);
	auto * keymaster  = findObjectAt(s.questPos);
	auto * gate       = findObjectAt(s.questPos2);
	ASSERT_NE(hero,      nullptr);
	ASSERT_NE(keymaster, nullptr);
	ASSERT_NE(gate,      nullptr);

	ASSERT_FALSE(gate->passableFor(PlayerColor(0)));

	visit(hero, keymaster);

	EXPECT_TRUE(gate->passableFor(PlayerColor(0)))
		<< "after visiting the matching keymaster, the gate should be passable for red";
}

TEST_F(QuestBorderTest, BorderGate_NeverRemoved)
{
	// A border gate stays on the map after being unlocked — heroes walk
	// through it but it never disappears (unlike a border guard).
	auto s = questBorderGate();
	ASSERT_NO_FATAL_FAILURE(startWithMap(std::move(s.builder)));

	auto * hero      = findHeroAt(s.heroPos);
	auto * keymaster = findObjectAt(s.questPos);
	ASSERT_NE(hero,      nullptr);
	ASSERT_NE(keymaster, nullptr);

	visit(hero, keymaster);

	// Gate still exists at its anchor position after the keymaster visit.
	EXPECT_NE(findObjectAt(s.questPos2), nullptr);
}

TEST_F(QuestBorderTest, Keymaster_FirstVisit_marksColorVisited)
{
	// Visiting a keymaster tent grants the visiting player the matching key
	// (recorded in the per-player "visited keymasters" set).
	auto s = questBorderGate();
	ASSERT_NO_FATAL_FAILURE(startWithMap(std::move(s.builder)));

	auto * hero      = findHeroAt(s.heroPos);
	auto * keymaster = findObjectAt(s.questPos);
	ASSERT_NE(hero,      nullptr);
	ASSERT_NE(keymaster, nullptr);

	const auto * tent = dynamic_cast<const CGKeymasterTent *>(keymaster);
	ASSERT_NE(tent, nullptr);

	EXPECT_FALSE(tent->wasMyColorVisited(PlayerColor(0)));

	visit(hero, keymaster);

	EXPECT_TRUE(tent->wasMyColorVisited(PlayerColor(0)));
}

TEST_F(QuestBorderTest, Keymaster_FirstVisit_doesNotEmitAddQuest)
{
	// Keymaster tents don't show up in the player's quest log — they pop a
	// "you found the tent" message and that's it.
	auto s = questKeymasterTent();
	ASSERT_NO_FATAL_FAILURE(startWithMap(std::move(s.builder)));

	auto * hero      = findHeroAt(s.heroPos);
	auto * keymaster = findObjectAt(s.questPos);
	ASSERT_NE(hero, nullptr);
	ASSERT_NE(keymaster, nullptr);

	ASSERT_TRUE(gameEventCallback->addedQuests.empty()) << "preconditions";

	visit(hero, keymaster);

	EXPECT_TRUE(gameEventCallback->addedQuests.empty())
		<< "keymaster visit unexpectedly emitted " << gameEventCallback->addedQuests.size()
		<< " AddQuest packet(s)";
	// And the visit still rendered the standard first-visit message:
	EXPECT_FALSE(gameEventCallback->infoWindows.empty());
}

TEST_F(QuestBorderTest, Keymaster_SecondVisit_showsAlreadyVisitedText)
{
	// Re-visiting a keymaster shows a distinct "you've already been here"
	// message rather than the original discovery message.
	auto s = questKeymasterTent();
	ASSERT_NO_FATAL_FAILURE(startWithMap(std::move(s.builder)));

	auto * hero      = findHeroAt(s.heroPos);
	auto * keymaster = findObjectAt(s.questPos);
	ASSERT_NE(hero, nullptr);
	ASSERT_NE(keymaster, nullptr);

	visit(hero, keymaster);
	const size_t windowsAfterFirst = gameEventCallback->infoWindows.size();
	ASSERT_GE(windowsAfterFirst, 1u);

	visit(hero, keymaster);
	EXPECT_GT(gameEventCallback->infoWindows.size(), windowsAfterFirst)
		<< "second visit should produce its own already-visited dialog";
}

TEST_F(QuestBorderTest, BorderGuard_BeforeKeymaster_blocksAndEmitsAddQuest)
{
	// Visiting a border guard before having its key adds the quest to the
	// player's log and tells them what they need, but doesn't offer to remove
	// the guard.
	auto s = questBorderGuard();
	ASSERT_NO_FATAL_FAILURE(startWithMap(std::move(s.builder)));

	auto * hero        = findHeroAt(s.heroPos);
	auto * borderGuard = findObjectAt(s.questPos2);
	ASSERT_NE(hero,        nullptr);
	ASSERT_NE(borderGuard, nullptr);

	visit(hero, borderGuard);

	EXPECT_EQ(gameEventCallback->addedQuests.size(), 1u);
	EXPECT_FALSE(gameEventCallback->infoWindows.empty());
	EXPECT_TRUE(gameEventCallback->blockingDialogs.empty())
		<< "border guard should not prompt for removal before the keymaster has been visited";
}

TEST_F(QuestBorderTest, BorderGuard_AfterKeymaster_promptsRemovalDialog)
{
	// Once the player has the matching key, visiting the border guard pops
	// a "tear it down?" prompt.
	auto s = questBorderGuard();
	ASSERT_NO_FATAL_FAILURE(startWithMap(std::move(s.builder)));

	auto * hero        = findHeroAt(s.heroPos);
	auto * keymaster   = findObjectAt(s.questPos);
	auto * borderGuard = findObjectAt(s.questPos2);
	ASSERT_NE(hero,        nullptr);
	ASSERT_NE(keymaster,   nullptr);
	ASSERT_NE(borderGuard, nullptr);

	visit(hero, keymaster);
	// Drop everything queued by the keymaster visit so the assertion is
	// scoped to the border-guard interaction.
	gameEventCallback->addedQuests.clear();
	gameEventCallback->infoWindows.clear();

	visit(hero, borderGuard);

	EXPECT_TRUE(gameEventCallback->addedQuests.empty())
		<< "AddQuest must be emitted on the *first* border-guard visit only";
	EXPECT_EQ(gameEventCallback->blockingDialogs.size(), 1u)
		<< "border guard should prompt the player whether to demolish";
}

TEST_F(QuestBorderTest, BorderGuard_AnsweredYes_removesObject)
{
	// Answering "yes" to the tear-it-down prompt removes the border guard
	// from the map.
	auto s = questBorderGuard();
	ASSERT_NO_FATAL_FAILURE(startWithMap(std::move(s.builder)));

	auto * hero        = findHeroAt(s.heroPos);
	auto * keymaster   = findObjectAt(s.questPos);
	auto * borderGuard = findObjectAt(s.questPos2);
	ASSERT_NE(hero,        nullptr);
	ASSERT_NE(keymaster,   nullptr);
	ASSERT_NE(borderGuard, nullptr);

	visit(hero, keymaster);
	visit(hero, borderGuard);

	ASSERT_EQ(gameEventCallback->blockingDialogs.size(), 1u);
	answerDialog(hero, /*yes*/ 1);

	EXPECT_EQ(findObjectAt(s.questPos2), nullptr)
		<< "border guard should be removed from the map after a positive answer";
}
