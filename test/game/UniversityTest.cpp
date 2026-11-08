/*
 * UniversityTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "GameStateTest.h"

#include "../../lib/GameLibrary.h"
#include "../../lib/constants/EntityIdentifiers.h"
#include "../../lib/mapObjectConstructors/AObjectTypeHandler.h"
#include "../../lib/mapObjectConstructors/CObjectClassesHandler.h"
#include "../../lib/mapObjects/CGMarket.h"

class UniversityTest : public GameStateTest
{
protected:
	std::vector<SecondarySkill> rollUniversitySkills(GameRandomizer & randomizer)
	{
		auto handler = LIBRARY->objtypeh->getHandlerFor(Obj::UNIVERSITY, 0);
		auto object = handler->create(gameState.get(), nullptr);
		handler->configureObject(object.get(), randomizer);

		std::vector<SecondarySkill> result;
		for(const auto & skill : dynamic_cast<const CGUniversity &>(*object).availableItemsIds(EMarketMode::RESOURCE_SKILL))
			result.push_back(skill.as<SecondarySkill>());
		return result;
	}
};

TEST_F(UniversityTest, offeredSkillsAreUnique)
{
	startTestGame();

	GameRandomizer randomizer(*gameState);
	for(int i = 0; i < 50; ++i)
	{
		const auto skills = rollUniversitySkills(randomizer);
		const std::set<SecondarySkill> uniqueSkills(skills.begin(), skills.end());
		EXPECT_EQ(skills.size(), 4u) << "roll " << i;
		EXPECT_EQ(uniqueSkills.size(), skills.size()) << "roll " << i;
	}
}

// Issue #6803: map allows only 3 skills + Necromancy, which universities never offer
TEST_F(UniversityTest, offersFewerSkillsWhenMapAllowsTooFew)
{
	startTestGame();

	const std::set<SecondarySkill> offerable = {SecondarySkill::WISDOM, SecondarySkill::LOGISTICS, SecondarySkill::LEADERSHIP};
	map->allowedAbilities = offerable;
	map->allowedAbilities.insert(SecondarySkill::NECROMANCY);

	GameRandomizer randomizer(*gameState);
	const auto skills = rollUniversitySkills(randomizer);
	const std::set<SecondarySkill> offered(skills.begin(), skills.end());

	EXPECT_EQ(skills.size(), offerable.size());
	EXPECT_EQ(offered, offerable);
}
