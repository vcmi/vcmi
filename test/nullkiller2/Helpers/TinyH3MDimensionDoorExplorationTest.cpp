/*
 * TinyH3MDimensionDoorExplorationTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"

#include "AI/Nullkiller2/AIGateway.h"
#include "AI/Nullkiller2/Goals/AdventureSpellCast.h"
#include "AI/Nullkiller2/Goals/Composition.h"
#include "AI/Nullkiller2/Helpers/ExplorationHelper.h"

#include "mock/TinyH3MBuilder.h"
#include "nullkiller2/NullkillerTest.h"

#include "lib/CPlayerState.h"
#include "lib/IGameSettings.h"
#include "lib/mapObjects/CGHeroInstance.h"
#include "lib/spells/CSpell.h"

namespace
{
const PlayerColor PLAYER = PlayerColor(0);
const SpellID DIMENSION_DOOR = SpellID(8);

TinyH3M::TinyH3MBuilder makeDimensionDoorExplorationMap(bool withDimensionDoor)
{
	TinyH3M::TinyH3MBuilder builder(EMapFormat::SOD);
	builder
		.size(36, false)
		.name("DDNullkillerExploration")
		.playerActive(PLAYER)
		.hero({5, 5, 0}, HeroTypeID(0), PLAYER)
		.heroGarrison({{CreatureID(27), 1}})
		.heroPrimary(10, 10, 10, 50)
		.heroSecondarySkills({{SecondarySkill::AIR_MAGIC, 3}})
		.heroEquipped({{ArtifactPosition::SPELLBOOK, ArtifactID::SPELLBOOK}})
		.heroSpells(withDimensionDoor ? std::vector<SpellID>{DIMENSION_DOOR} : std::vector<SpellID>{});

	return builder;
}

bool containsDimensionDoorCast(const NK2AI::Goals::TGoalVec & goals)
{
	for(const auto & goal : goals)
	{
		const auto * spellCast = dynamic_cast<const NK2AI::Goals::AdventureSpellCast *>(goal.get());
		if(spellCast && spellCast->getSpell()->getId() == DIMENSION_DOOR)
			return true;
	}

	return false;
}

class TinyH3MDimensionDoorExplorationTest : public NullkillerTest
{
public:
	bool prepareForAdventureSpellPlanning(CGHeroInstance & hero) const
	{
		const CSpell * dimensionDoor = DIMENSION_DOOR.toSpell();
		if(!dimensionDoor)
			return false;

		const int spellCost = hero.getSpellCost(dimensionDoor);
		if(spellCost <= 0)
			return false;

		hero.mana = spellCost;
		hero.setMovementPoints(500);
		return true;
	}

	void setTileVisible(PlayerColor player, const int3 & tile, bool visible)
	{
		ASSERT_TRUE(map()->isInTheMap(tile)) << tile.toString();
		auto * team = gameState()->getPlayerTeam(player);
		ASSERT_NE(team, nullptr);
		team->fogOfWarMap[tile] = visible ? 1 : 0;
	}
};
}

TEST_F(TinyH3MDimensionDoorExplorationTest, GeneratedDimensionDoorHeroCastsForHiddenExploration)
{
	overrideSettingBeforeInit(EGameSettings::SPELLS_DIMENSION_DOOR_TRIGGERS_GUARDS, false);
	startWithMap(makeDimensionDoorExplorationMap(true));

	auto * hero = findHeroByOwner(PLAYER);
	ASSERT_NE(hero, nullptr);
	ASSERT_TRUE(hero->hasSpellbook());
	ASSERT_TRUE(hero->spellbookContainsSpell(DIMENSION_DOOR));
	ASSERT_TRUE(prepareForAdventureSpellPlanning(*hero));

	setMapVisibility(PLAYER, false);
	setTileVisible(PLAYER, hero->visitablePos(), true);

	const auto gateway = makeGateway(PLAYER);
	NK2AI::ExplorationHelper helper(hero, gateway->nullkiller.get());

	EXPECT_TRUE(helper.canUseDimensionDoor());
	ASSERT_TRUE(helper.considerDimensionDoorExplorationTargets());
	const auto goals = helper.makeComposition()->decompose(gateway->nullkiller.get());

	EXPECT_TRUE(containsDimensionDoorCast(goals));
}

TEST_F(TinyH3MDimensionDoorExplorationTest, GeneratedHeroWithoutDimensionDoorDoesNotCast)
{
	overrideSettingBeforeInit(EGameSettings::SPELLS_DIMENSION_DOOR_TRIGGERS_GUARDS, false);
	startWithMap(makeDimensionDoorExplorationMap(false));

	auto * hero = findHeroByOwner(PLAYER);
	ASSERT_NE(hero, nullptr);
	ASSERT_TRUE(hero->hasSpellbook());
	ASSERT_FALSE(hero->spellbookContainsSpell(DIMENSION_DOOR));
	ASSERT_TRUE(prepareForAdventureSpellPlanning(*hero));

	setMapVisibility(PLAYER, false);
	setTileVisible(PLAYER, hero->visitablePos(), true);

	const auto gateway = makeGateway(PLAYER);
	NK2AI::ExplorationHelper helper(hero, gateway->nullkiller.get());

	EXPECT_FALSE(helper.canUseDimensionDoor());
	EXPECT_FALSE(helper.considerDimensionDoorExplorationTargets());
}

TEST_F(TinyH3MDimensionDoorExplorationTest, GeneratedDimensionDoorHeroDoesNotProbeHiddenTilesWhenGuardedLandingsTrigger)
{
	overrideSettingBeforeInit(EGameSettings::SPELLS_DIMENSION_DOOR_TRIGGERS_GUARDS, true);
	startWithMap(makeDimensionDoorExplorationMap(true));

	auto * hero = findHeroByOwner(PLAYER);
	ASSERT_NE(hero, nullptr);
	ASSERT_TRUE(hero->hasSpellbook());
	ASSERT_TRUE(hero->spellbookContainsSpell(DIMENSION_DOOR));
	ASSERT_TRUE(prepareForAdventureSpellPlanning(*hero));

	setMapVisibility(PLAYER, false);
	setTileVisible(PLAYER, hero->visitablePos(), true);

	const auto gateway = makeGateway(PLAYER);
	NK2AI::ExplorationHelper helper(hero, gateway->nullkiller.get());

	EXPECT_TRUE(helper.canUseDimensionDoor());
	EXPECT_FALSE(helper.considerDimensionDoorExplorationTargets());
}
