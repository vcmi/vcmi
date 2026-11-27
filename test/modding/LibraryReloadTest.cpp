/*
 * LibraryReloadTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include <vcmi/ArtifactService.h>
#include <vcmi/Entity.h>
#include <vcmi/CreatureService.h>
#include <vcmi/FactionService.h>
#include <vcmi/HeroClassService.h>
#include <vcmi/HeroTypeService.h>
#include <vcmi/SkillService.h>
#include <vcmi/spells/Service.h>

#include "../../lib/GameLibrary.h"
#include "../../lib/json/JsonNode.h"
#include "../../lib/modding/CModHandler.h"
#include "../../lib/modding/ModManager.h"
#include "../../lib/modding/ModSnapshot.h"

namespace
{
/// Service name, mod scope, identifier and index of every entity of the library
using EntityTable = std::set<std::tuple<std::string, std::string, std::string, int32_t>>;

EntityTable collectEntities(const GameLibrary & library)
{
	EntityTable result;
	const auto collect = [&result](const std::string & serviceName, const EntityService * service)
	{
		service->forEachBase([&](const Entity * entity, bool & stop)
		{
			result.emplace(serviceName, entity->getModScope(), entity->getJsonKey(), entity->getIndex());
		});
	};

	collect("artifacts", library.artifacts());
	collect("creatures", library.creatures());
	collect("factions", library.factions());
	collect("heroClasses", library.heroClasses());
	collect("heroTypes", library.heroTypes());
	collect("skills", library.skills());
	collect("spells", library.spells());
	return result;
}

bool hasEntitiesOfMod(const EntityTable & entities, const std::string & modID)
{
	return std::ranges::any_of(entities, [&modID](const auto & entity){ return std::get<1>(entity) == modID; });
}

std::unique_ptr<GameLibrary> loadLibrary(const ModSnapshot & mods)
{
	auto library = std::make_unique<GameLibrary>();
	library->initializeFilesystem(mods);
	library->initializeLibrary();
	return library;
}
}

TEST(LibraryReloadTest, libraryLoadedWhileAnotherIsCurrentDoesNotDependOnIt)
{
	ModManager mods(JsonNode(), true);
	JsonNode noRequiredMods;
	noRequiredMods.Struct();

	const EntityTable testPresetEntities = collectEntities(*LIBRARY);
	ASSERT_TRUE(hasEntitiesOfMod(testPresetEntities, "vcmi-test"));

	// library without gameplay mods, loaded while test preset library and its filesystem are current
	auto coreLibrary = loadLibrary(mods.createSnapshot(noRequiredMods, true));
	EXPECT_FALSE(coreLibrary->modh->getModList().isModActive("vcmi-test"));
	EXPECT_FALSE(hasEntitiesOfMod(collectEntities(*coreLibrary), "vcmi-test"));

	auto testPresetLibrary = setGameLibrary(std::move(coreLibrary));

	// test preset again, loaded while library without gameplay mods is current
	auto reloadedLibrary = loadLibrary(mods.createSnapshot());

	setGameLibrary(std::move(testPresetLibrary));

	EXPECT_EQ(collectEntities(*reloadedLibrary), testPresetEntities);
}
