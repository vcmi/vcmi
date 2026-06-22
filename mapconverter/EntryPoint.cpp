/*
 * EntryPoint.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"

#include "../lib/GameLibrary.h"
#include "../lib/ScopeGuard.h"
#include "../lib/VCMIDirs.h"
#include "../lib/callback/EditorCallback.h"
#include "../lib/CRandomGenerator.h"
#include "../lib/entities/artifact/CArtifact.h"
#include "../lib/entities/hero/CHeroClass.h"
#include "../lib/entities/hero/CHeroHandler.h"
#include "../lib/filesystem/Filesystem.h"
#include "../lib/logging/CBasicLogConfigurator.h"
#include "../lib/mapObjectConstructors/AObjectTypeHandler.h"
#include "../lib/mapObjectConstructors/CObjectClassesHandler.h"
#include "../lib/mapObjectConstructors/CommonConstructors.h"
#include "../lib/mapObjects/CGHeroInstance.h"
#include "../lib/mapObjects/CGTownInstance.h"
#include "../lib/mapObjects/MiscObjects.h"
#include "../lib/mapping/CMap.h"
#include "../lib/mapping/CMapService.h"
#include "../lib/spells/CSpellHandler.h"

#include <boost/program_options.hpp>
#include <fstream>
#include <limits>

namespace po = boost::program_options;

VCMI_LIB_USING_NAMESPACE

namespace
{
std::vector<uint8_t> readFile(const boost::filesystem::path & path)
{
	const auto size = boost::filesystem::file_size(path);
	if(size > static_cast<uintmax_t>(std::numeric_limits<int>::max()))
		throw std::runtime_error("Input map is too large: " + path.string());

	std::vector<uint8_t> result(size);
	std::ifstream input(path.string(), std::ios::binary);
	if(!input)
		throw std::runtime_error("Cannot open input map: " + path.string());

	input.read(reinterpret_cast<char *>(result.data()), static_cast<std::streamsize>(result.size()));
	if(!input)
		throw std::runtime_error("Cannot read input map: " + path.string());

	return result;
}

void repairMapForSaving(CMap * map)
{
	if(!map)
		return;

	int emptyNameId = 1;
	for(auto & event : map->events)
	{
		if(event.name.empty())
			event.name = "event_" + std::to_string(emptyNameId++);
	}

	emptyNameId = 1;
	for(auto & rumor : map->rumors)
	{
		if(rumor.name.empty())
			rumor.name = "rumor_" + std::to_string(emptyNameId++);
	}

	std::vector<CGObjectInstance *> impactedObjects;
	for(const auto & object : map->objects)
		impactedObjects.push_back(object.get());

	for(const auto & hero : map->getHeroesInPool())
		impactedObjects.push_back(map->tryGetFromHeroPool(hero));

	for(auto * object : impactedObjects)
	{
		if(!object)
			continue;

		if(object->asOwnable() && object->getOwner() == PlayerColor::UNFLAGGABLE)
			object->tempOwner = PlayerColor::NEUTRAL;

		if(auto * hero = dynamic_cast<CGHeroInstance *>(object))
		{
			map->allowedHeroes.insert(hero->getHeroTypeID());

			const auto & heroType = LIBRARY->heroh->objects[hero->subID];
			if(hero->ID == Obj::HERO)
			{
				hero->appearance = LIBRARY->objtypeh
					->getHandlerFor(Obj::HERO, heroType->heroClass->getIndex())
					->getTemplates()
					.front();
			}

			if(hero->spellbookContainsSpell(SpellID::SPELLBOOK_PRESET))
			{
				hero->removeSpellFromSpellbook(SpellID::SPELLBOOK_PRESET);
				if(!hero->getArt(ArtifactPosition::SPELLBOOK) && heroType->haveSpellBook)
					hero->putArtifact(ArtifactPosition::SPELLBOOK, map->createArtifact(ArtifactID::SPELLBOOK));
			}
		}

		if(auto * town = dynamic_cast<CGTownInstance *>(object))
		{
			if(town->getTown())
			{
				for(const auto & building : town->getBuildings())
				{
					if(!town->getTown()->buildings.count(building))
						town->removeBuilding(building);
				}

				vstd::erase_if(town->forbiddenBuildings, [town](BuildingID building)
				{
					return !town->getTown()->buildings.count(building);
				});
			}
		}

		if(auto * artifact = dynamic_cast<CGArtifact *>(object))
		{
			if(artifact->ID == Obj::SPELL_SCROLL && !artifact->getArtifactInstance())
			{
				std::vector<SpellID> spells;
				for(const auto & spell : LIBRARY->spellh->objects)
				{
					if(spell)
						spells.push_back(spell->id);
				}

				auto scroll = map->createScroll(*RandomGeneratorUtil::nextItem(spells, CRandomGenerator::getDefault()));
				artifact->setArtifactInstance(scroll);
			}
		}

		if(auto * mine = dynamic_cast<CGMine *>(object))
		{
			if(!mine->isAbandoned())
			{
				if(mine->getResourceHandler()->getResourceType() == GameResID::NONE)
					mine->producedResource = GameResID(mine->subID);
				else
					mine->producedResource = mine->getResourceHandler()->getResourceType();

				mine->producedQuantity = mine->defaultResProduction();
			}
		}
	}
}

void convertMap(const boost::filesystem::path & inputPath, const boost::filesystem::path & outputPath)
{
	CMapService mapService;
	EditorCallback callback(nullptr);
	const auto inputBytes = readFile(inputPath);
	auto map = mapService.loadMap(inputBytes.data(), static_cast<int>(inputBytes.size()), inputPath.filename().string(), "core", "ASCII", &callback);
	callback.setMap(map.get());
	repairMapForSaving(map.get());
	mapService.saveMap(map, outputPath);
}

void initializeLibrary(const boost::filesystem::path & logPath)
{
	CBasicLogConfigurator logConfigurator(logPath, nullptr);
	logConfigurator.configureDefault();

	LIBRARY = new GameLibrary;
	LIBRARY->initializeFilesystem(false);
	logConfigurator.configure();
	LIBRARY->initializeLibrary();
}

void cleanupLibrary()
{
	delete LIBRARY;
	LIBRARY = nullptr;
	CResourceHandler::destroy();
}
}

int main(int argc, char ** argv)
{
	po::options_description options("Allowed options");
	options.add_options()
		("help,h", "display help and exit")
		("input,i", po::value<std::string>()->required(), "input .h3m or .vmap map")
		("output,o", po::value<std::string>()->required(), "output .vmap map");

	po::variables_map vm;

	try
	{
		po::store(po::parse_command_line(argc, argv, options), vm);

		if(vm.count("help"))
		{
			std::cout << options << std::endl;
			return 0;
		}

		po::notify(vm);

		const auto inputPath = boost::filesystem::path(vm["input"].as<std::string>());
		const auto outputPath = boost::filesystem::path(vm["output"].as<std::string>());
		const auto logPath = VCMIDirs::get().userLogsPath() / "VCMI_MapConverter_log.txt";

		initializeLibrary(logPath);
		auto cleanup = vstd::makeScopeGuard(cleanupLibrary);

		convertMap(inputPath, outputPath);

		std::cout << "Converted " << inputPath.string() << " to " << outputPath.string() << std::endl;
		return 0;
	}
	catch(const std::exception & e)
	{
		std::cerr << "Map conversion failed: " << e.what() << std::endl;
		return 1;
	}
}
