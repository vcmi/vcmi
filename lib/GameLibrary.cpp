/*
 * GameLibrary.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "GameLibrary.h"

#include "CBonusTypeHandler.h"
#include "CCreatureHandler.h"
#include "CConfigHandler.h"
#include "RoadHandler.h"
#include "RiverHandler.h"
#include "TerrainHandler.h"
#include "MapLayerHandler.h"
#include "spells/SpellSchoolHandler.h"
#include "CSkillHandler.h"
#include "../luascript/LuaModule.h"
#include "entities/artifact/CArtHandler.h"
#include "entities/faction/CTownHandler.h"
#include "entities/hero/CHeroClassHandler.h"
#include "entities/hero/CHeroHandler.h"
#include "entities/ResourceTypeHandler.h"
#include "texts/CGeneralTextHandler.h"
#include "campaign/CampaignRegionsHandler.h"
#include "mapping/MapFormatSettings.h"
#include "modding/CModHandler.h"
#include "modding/IdentifierStorage.h"
#include "modding/CModVersion.h"
#include "CStopWatch.h"
#include "VCMIDirs.h"
#include "filesystem/Filesystem.h"
#include "filesystem/CFilesystemLoader.h"
#include "filesystem/AdapterLoaders.h"
#include "rmg/CRmgTemplateStorage.h"
#include "mapObjectConstructors/CObjectClassesHandler.h"
#include "mapObjects/ObstacleSetHandler.h"
#include "mapping/CMapEditManager.h"
#include "spells/CSpellHandler.h"
#include "scripting/ScriptHandler.h"
#include "BattleFieldHandler.h"
#include "ObstacleHandler.h"
#include "GameSettings.h"

#include <vcmi/scripting/Service.h>

GameLibrary * LIBRARY = nullptr;



const ArtifactService * GameLibrary::artifacts() const
{
	return arth.get();
}

const CreatureService * GameLibrary::creatures() const
{
	return creh.get();
}

const FactionService * GameLibrary::factions() const
{
	return townh.get();
}

const HeroClassService * GameLibrary::heroClasses() const
{
	return heroclassesh.get();
}

const HeroTypeService * GameLibrary::heroTypes() const
{
	return heroh.get();
}

const ResourceTypeService * GameLibrary::resources() const
{
	return resourceTypeHandler.get();
}

const scripting::Service * GameLibrary::scripts() const
{
	return scriptHandler.get();
}

const spells::Service * GameLibrary::spells() const
{
	return spellh.get();
}

const SkillService * GameLibrary::skills() const
{
	return skillh.get();
}

const ITranslator * GameLibrary::staticTexts() const
{
	return generaltexth.get();
}

const IBonusTypeHandler * GameLibrary::getBth() const
{
	return bth.get();
}

const CIdentifierStorage * GameLibrary::identifiers() const
{
	return identifiersHandler.get();
}

const ScriptService * GameLibrary::scriptTypes() const
{
	return scriptTypeHandler.get();
}

const BattleFieldService * GameLibrary::battlefields() const
{
	return battlefieldsHandler.get();
}

const ObstacleService * GameLibrary::obstacles() const
{
	return obstacleHandler.get();
}

const IGameSettings * GameLibrary::engineSettings() const
{
	return settingsHandler.get();
}

const spells::SchoolService * GameLibrary::spellSchools() const
{
	return spellSchoolHandler.get();
}

void GameLibrary::loadFilesystem(bool extractArchives)
{
	CStopWatch loadTime;

	CResourceHandler::initialize();
	logGlobal->info("\tInitialization: %d ms", loadTime.getDiff());

	CResourceHandler::load("config/filesystem.json", extractArchives);
	logGlobal->info("\tData loading: %d ms", loadTime.getDiff());
}

void GameLibrary::loadModFilesystem(bool useTestPreset)
{
	CStopWatch loadTime;
	// Test preset discovers the vcmi-test fixtures mod from test/testdata/ instead of the
	// shipped Mods/ directory, so it is never scanned or shipped by the game itself.
	if(useTestPreset)
	{
		auto loader = std::make_unique<CFilesystemLoader>("MODS/", "test/testdata/", 64);
		dynamic_cast<CFilesystemList*>(CResourceHandler::get("initial"))->addLoader(std::move(loader), false);
	}
	modh = std::make_unique<CModHandler>(useTestPreset);
	identifiersHandler = std::make_unique<CIdentifierStorage>();
	logGlobal->info("\tMod handler: %d ms", loadTime.getDiff());

	modh->loadModFilesystems();
	logGlobal->info("\tMod filesystems: %d ms", loadTime.getDiff());
}

template <class Handler>
void createHandler(std::unique_ptr<Handler> & handler)
{
	handler = std::make_unique<Handler>();
}

void GameLibrary::initializeFilesystem(bool extractArchives, bool useTestPreset)
{
	loadFilesystem(extractArchives);
	settings.init("config/settings.json", "vcmi:settings");
	persistentStorage.init("config/persistentStorage.json", "");
	keyBindingsConfig.init("config/keyBindingsConfig.json", "");
	loadModFilesystem(useTestPreset);

	// Detect game data mode after filesystem is loaded
	gameDataMode = GameDataMode::SOD;
	if(CGeneralTextHandler::isRoEData())
	{
		if(CResourceHandler::get()->existsResource(ResourcePath("MAPS/H3DEMO.H3M")))
			gameDataMode = GameDataMode::DEMO_ROE;
		else
			gameDataMode = GameDataMode::ROE;
	}
	else if(CResourceHandler::get()->existsResource(ResourcePath("MAPS/H3DEMO.H3M")))
		gameDataMode = GameDataMode::DEMO_SOD;

	if(gameDataMode == GameDataMode::DEMO_ROE || gameDataMode == GameDataMode::DEMO_SOD)
		logGlobal->info("Game started with demo data");
	if(gameDataMode == GameDataMode::ROE || gameDataMode == GameDataMode::DEMO_ROE)
		logGlobal->info("Game started with RoE data");
}

GameLibrary::GameDataMode GameLibrary::getGameDataMode() const
{
	return gameDataMode;
}

bool GameLibrary::isRoeData() const
{
	return gameDataMode == GameDataMode::ROE || gameDataMode == GameDataMode::DEMO_ROE;
}

bool GameLibrary::isDemoData() const
{
	return gameDataMode == GameDataMode::DEMO_ROE || gameDataMode == GameDataMode::DEMO_SOD;
}

void GameLibrary::initializeLibrary()
{
	createHandler(settingsHandler);
	modh->initializeConfig();

	createHandler(generaltexth);
	createHandler(bth);
	createHandler(resourceTypeHandler);
	createHandler(roadTypeHandler);
	createHandler(riverTypeHandler);
	createHandler(terrainTypeHandler);
	createHandler(heroh);
	createHandler(heroclassesh);
	createHandler(arth);
	createHandler(creh);
	createHandler(townh);
	createHandler(biomeHandler);
	createHandler(objtypeh);
	createHandler(spellSchoolHandler);
	createHandler(spellh);
	createHandler(scriptTypeHandler);
	createHandler(skillh);
	createHandler(terviewh);
	createHandler(campaignRegions);
	createHandler(tplh); //templates need already resolved identifiers (refactor?)
	createHandler(battlefieldsHandler);
	createHandler(obstacleHandler);
	createHandler(mapLayerHandler);

	scriptHandler = std::make_unique<scripting::LuaModule>();
	scriptHandler->installScripting(*scriptTypeHandler);
	modh->load();
	modh->afterLoad();

	createHandler(mapFormat);
}

GameLibrary::GameLibrary() = default;
GameLibrary::~GameLibrary() = default;
