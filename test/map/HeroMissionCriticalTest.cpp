/*
 * HeroMissionCriticalTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "../mock/TinyMapGameTest.h"

#include "GameLibrary.h"
#include "mapObjects/CGHeroInstance.h"
#include "mapping/CMap.h"

class HeroMissionCriticalTest : public TinyMapGameTest
{
protected:
	const Services * gameServices() override { return LIBRARY; }

	const CGHeroInstance * first = nullptr;
	const CGHeroInstance * second = nullptr;

	using Equipment = std::vector<std::pair<ArtifactPosition, ArtifactID>>;

	void startWithHeroes(const Equipment & firstArtifacts, const Equipment & secondArtifacts)
	{
		TinyH3M::TinyH3MBuilder builder(EMapFormat::SOD);
		builder.size(36, false)
			.playerActive(PlayerColor(0))
			.hero({5, 5, 0}, HeroTypeID(0), PlayerColor(0))
			.heroEquipped(firstArtifacts)
			.hero({10, 10, 0}, HeroTypeID(1), PlayerColor(0))
			.heroEquipped(secondArtifacts);
		startWithMap(std::move(builder));

		first = findHeroAt({5, 5, 0});
		second = findHeroAt({10, 10, 0});
		ASSERT_NE(first, nullptr);
		ASSERT_NE(second, nullptr);
	}

	/// Adds a defeat triggered once the player owns none of the given artifacts
	void addArtifactLossDefeat(const std::vector<ArtifactID> & artifacts)
	{
		EventExpression::OperatorNone noneOf;
		for(const auto & artifact : artifacts)
		{
			EventCondition condition(EventCondition::HAVE_ARTIFACT);
			condition.objectType = artifact;
			noneOf.expressions.emplace_back(condition);
		}

		TriggeredEvent event;
		event.identifier = "artifactLost";
		event.effect.type = EventEffect::DEFEAT;
		event.trigger = EventExpression(noneOf);
		map()->triggeredEvents.push_back(event);
	}
};

static const ArtifactID amulet(54); // Amulet of the Undertaker
static const ArtifactID cowl(55); // Vampire's Cowl
static const ArtifactID cloak(130); // Cloak of the Undead King, combined from the two above and Dead Man's Boots
static const std::pair<ArtifactPosition, ArtifactID> amuletOnNeck(ArtifactPosition::NECK, amulet);
static const std::pair<ArtifactPosition, ArtifactID> cowlOnShoulders(ArtifactPosition::SHOULDERS, cowl);

TEST_F(HeroMissionCriticalTest, OnlyCarrierOfLastCopyIsCritical)
{
	startWithHeroes({cowlOnShoulders}, {amuletOnNeck});
	addArtifactLossDefeat({cloak, cowl});

	EXPECT_TRUE(first->isMissionCritical());
	EXPECT_FALSE(second->isMissionCritical());
}

TEST_F(HeroMissionCriticalTest, CarrierIsNotCriticalWhenAnotherHeroHasArtifact)
{
	startWithHeroes({cowlOnShoulders}, {cowlOnShoulders});
	addArtifactLossDefeat({cowl});

	EXPECT_FALSE(first->isMissionCritical());
	EXPECT_FALSE(second->isMissionCritical());
}

TEST_F(HeroMissionCriticalTest, NobodyIsCriticalWhenDefeatAlreadyHolds)
{
	startWithHeroes({amuletOnNeck}, {});
	addArtifactLossDefeat({cowl});

	EXPECT_FALSE(first->isMissionCritical());
	EXPECT_FALSE(second->isMissionCritical());
}
