/*
 * HeroSecondarySkillsTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "GameStateTest.h"

#include "../../lib/constants/EntityIdentifiers.h"
#include "../../lib/json/JsonNode.h"
#include "../../lib/mapObjects/CGHeroInstance.h"
#include "../../lib/serializer/JsonSerializer.h"

class HeroSecondarySkillsTest : public GameStateTest
{
};

// Regression test for GitHub issue #7598.
// A freshly created hero carries a {NONE, -1} "use hero type default skills" marker in
// secSkills. Setting skills explicitly must drop that marker, otherwise the hero
// serializes as "has default skills" and every explicitly set skill is silently lost
// (see CGHeroInstance::serializeJsonOptions). Battle Only Mode hits this whenever all
// 8 skill slots are filled, because the marker was previously only removed as a side
// effect of assigning an empty slot.
TEST_F(HeroSecondarySkillsTest, settingSkillsClearsDefaultSkillsMarker)
{
	startTestGame();

	CGHeroInstance hero(gameState.get());

	ASSERT_EQ(hero.secSkills.size(), 1u) << "fresh hero should carry the default-skills marker";
	ASSERT_EQ(hero.secSkills[0].first, SecondarySkill(SecondarySkill::NONE));

	// Mimics BattleOnlyModeTab::startBattle filling all 8 skill slots.
	const std::vector<SecondarySkill> chosen = {
		SecondarySkill(SecondarySkill::EARTH_MAGIC),
		SecondarySkill(SecondarySkill::AIR_MAGIC),
		SecondarySkill(SecondarySkill::PATHFINDING),
		SecondarySkill(SecondarySkill::ARCHERY),
		SecondarySkill(SecondarySkill::LOGISTICS),
		SecondarySkill(SecondarySkill::SCOUTING),
		SecondarySkill(SecondarySkill::DIPLOMACY),
		SecondarySkill(SecondarySkill::NAVIGATION),
	};

	for (const auto & skill : chosen)
		hero.setSecSkillLevel(skill, MasteryLevel::EXPERT, ChangeValueMode::ABSOLUTE);

	for (const auto & entry : hero.secSkills)
		EXPECT_NE(entry.first, SecondarySkill(SecondarySkill::NONE))
			<< "default-skills marker must not survive alongside explicitly set skills";

	EXPECT_EQ(hero.secSkills.size(), chosen.size());
}

// End-to-end counterpart of the above: the skills must actually survive serialization
// into the map file. Before the fix, a hero with all 8 slots filled wrote no
// "secondarySkills" field at all, so reloading the map silently restored the hero
// type's default skills - which is what made Expert magic schools vanish in battle.
TEST_F(HeroSecondarySkillsTest, allEightSkillsAreSerializedIntoMap)
{
	startTestGame();

	// serializeJsonOptions is protected; expose it exactly as the map saver reaches it.
	struct HeroProbe : public CGHeroInstance
	{
		using CGHeroInstance::CGHeroInstance;
		using CGHeroInstance::serializeJsonOptions;
	};

	HeroProbe hero(gameState.get());
	hero.setHeroType(HeroTypeID(0));
	hero.setOwner(PlayerColor(0));

	const std::vector<SecondarySkill> chosen = {
		SecondarySkill(SecondarySkill::EARTH_MAGIC),
		SecondarySkill(SecondarySkill::AIR_MAGIC),
		SecondarySkill(SecondarySkill::PATHFINDING),
		SecondarySkill(SecondarySkill::ARCHERY),
		SecondarySkill(SecondarySkill::LOGISTICS),
		SecondarySkill(SecondarySkill::SCOUTING),
		SecondarySkill(SecondarySkill::DIPLOMACY),
		SecondarySkill(SecondarySkill::NAVIGATION),
	};

	for (const auto & skill : chosen)
		hero.setSecSkillLevel(skill, MasteryLevel::EXPERT, ChangeValueMode::ABSOLUTE);

	JsonNode written;
	JsonSerializer handler(nullptr, written);
	hero.serializeJsonOptions(handler);

	const JsonNode & skills = written["secondarySkills"];
	ASSERT_EQ(skills.getType(), JsonNode::JsonType::DATA_VECTOR)
		<< "secondarySkills field must be present in the saved map";
	EXPECT_EQ(skills.Vector().size(), chosen.size());

	for (const auto & entry : skills.Vector())
		EXPECT_EQ(entry["level"].String(), "expert");
}
