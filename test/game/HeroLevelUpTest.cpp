/*
 * HeroLevelUpTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "GameStateTest.h"

#include "../../lib/CPlayerState.h"
#include "../../lib/bonuses/Bonus.h"
#include "../../lib/bonuses/BonusSelector.h"
#include "../../lib/bonuses/Propagators.h"
#include "../../lib/bonuses/Updaters.h"
#include "../../lib/mapObjects/CGHeroInstance.h"
#include "../../lib/networkPacks/PacksForClient.h"

class HeroLevelUpTest : public GameStateTest
{
};

TEST_F(HeroLevelUpTest, bonusHandedOnToPlayerScalesWithNewLevel)
{
	startTestGame();

	const auto * player = gameState->getPlayerState(PlayerColor(0));
	ASSERT_FALSE(player->getHeroes().empty());
	auto * hero = gameState->getHero(player->getHeroes().front()->id);

	auto bonus = std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::MORALE, BonusSource::OTHER, 1, BonusSourceID());
	bonus->propagator = std::make_shared<CPropagatorNodeType>(BonusNodeType::PLAYER);
	bonus->propagationUpdater = std::make_shared<TimesHeroLevelUpdater>();
	hero->addNewBonus(bonus);

	const auto selector = Selector::type()(BonusType::MORALE).And(Selector::sourceTypeSel(BonusSource::OTHER));
	const int levelBefore = hero->level;
	ASSERT_EQ(player->valOfBonuses(selector), levelBefore);

	HeroLevelUp levelUp;
	levelUp.player = PlayerColor(0);
	levelUp.heroId = hero->id;
	gameEventCallback->sendAndApply(levelUp);

	EXPECT_EQ(player->valOfBonuses(selector), levelBefore + 1);
}
