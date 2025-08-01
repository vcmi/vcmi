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
#include "spells/CSpellHandler.h"
#include "spells/SpellSchoolHandler.h"
#include "spells/effects/Registry.h"
#include "CSkillHandler.h"
#include "entities/artifact/CArtHandler.h"
#include "entities/faction/CTownHandler.h"
#include "entities/hero/CHeroClassHandler.h"
#include "entities/hero/CHeroHandler.h"
#include "texts/CGeneralTextHandler.h"
#include "campaign/CampaignRegionsHandler.h"
#include "mapping/MapFormatSettings.h"
#include "modding/CModHandler.h"
#include "modding/IdentifierStorage.h"
#include "modding/CModVersion.h"
#include "CStopWatch.h"
#include "VCMIDirs.h"
#include "filesystem/Filesystem.h"
#include "rmg/CRmgTemplateStorage.h"
#include "mapObjectConstructors/CObjectClassesHandler.h"
#include "mapObjects/CObjectHandler.h"
#include "mapObjects/ObstacleSetHandler.h"
#include "mapping/CMapEditManager.h"
#include "ScriptHandler.h"
#include "BattleFieldHandler.h"
#include "ObstacleHandler.h"
#include "GameSettings.h"

VCMI_LIB_NAMESPACE_BEGIN

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

#if SCRIPTING_ENABLED
const scripting::Service * GameLibrary::scripts() const
{
	return scriptHandler.get();
}
#endif

const spells::Service * GameLibrary::spells() const
{
	return spellh.get();
}

const SkillService * GameLibrary::skills() const
{
	return skillh.get();
}

const IBonusTypeHandler * GameLibrary::getBth() const
{
	return bth.get();
}

const CIdentifierStorage * GameLibrary::identifiers() const
{
	return identifiersHandler.get();
}

const spells::effects::Registry * GameLibrary::spellEffects() const
{
	return spells::effects::GlobalRegistry::get();
}

spells::effects::Registry * GameLibrary::spellEffects()
{
	return spells::effects::GlobalRegistry::get();
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

void GameLibrary::loadFilesystem(bool extractArchives)
{
	CStopWatch loadTime;

	CResourceHandler::initialize();
	logGlobal->info("\tInitialization: %d ms", loadTime.getDiff());

	CResourceHandler::load("config/filesystem.json", extractArchives);
	logGlobal->info("\tData loading: %d ms", loadTime.getDiff());
}

void GameLibrary::loadModFilesystem()
{
	CStopWatch loadTime;
	modh = std::make_unique<CModHandler>();
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

void GameLibrary::initializeFilesystem(bool extractArchives)
{
	loadFilesystem(extractArchives);
	settings.init("config/settings.json", "vcmi:settings");
	persistentStorage.init("config/persistentStorage.json", "");
	keyBindingsConfig.init("config/keyBindingsConfig.json", "");
	loadModFilesystem();
}

void GameLibrary::initializeLibrary()
{
	createHandler(settingsHandler);
	modh->initializeConfig();

	createHandler(generaltexth);
	createHandler(bth);
	createHandler(roadTypeHandler);
	createHandler(riverTypeHandler);
	createHandler(terrainTypeHandler);
	createHandler(heroh);
	createHandler(heroclassesh);
	createHandler(arth);
	createHandler(creh);
	createHandler(townh);
	createHandler(biomeHandler);
	createHandler(objh);
	createHandler(objtypeh);
	createHandler(spellSchoolHandler);
	createHandler(spellh);
	createHandler(skillh);
	createHandler(terviewh);
	createHandler(campaignRegions);
	createHandler(tplh); //templates need already resolved identifiers (refactor?)
#if SCRIPTING_ENABLED
	createHandler(scriptHandler);
#endif
	createHandler(battlefieldsHandler);
	createHandler(obstacleHandler);

	modh->load();
	modh->afterLoad();

	createHandler(mapFormat);
}

#if SCRIPTING_ENABLED
void GameLibrary::scriptsLoaded()
{
	scriptHandler->performRegistration(this);
}
#endif

GameLibrary::GameLibrary() = default;
GameLibrary::~GameLibrary() = default;

VCMI_LIB_NAMESPACE_END
