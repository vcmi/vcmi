/*
 * VCMI_Lib.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "VCMI_Lib.h"

#include "CArtHandler.h"
#include "CBonusTypeHandler.h"
#include "CCreatureHandler.h"
#include "CConfigHandler.h"
#include "RoadHandler.h"
#include "RiverHandler.h"
#include "TerrainHandler.h"
#include "spells/CSpellHandler.h"
#include "spells/effects/Registry.h"
#include "CSkillHandler.h"
#include "entities/faction/CTownHandler.h"
#include "entities/hero/CHeroClassHandler.h"
#include "entities/hero/CHeroHandler.h"
#include "texts/CGeneralTextHandler.h"
#include "modding/CModHandler.h"
#include "modding/IdentifierStorage.h"
#include "modding/CModVersion.h"
#include "IGameEventsReceiver.h"
#include "CStopWatch.h"
#include "VCMIDirs.h"
#include "filesystem/Filesystem.h"
#include "CConsoleHandler.h"
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

LibClasses * VLC = nullptr;

DLL_LINKAGE void preinitDLL(CConsoleHandler * Console, bool extractArchives)
{
	console = Console;
	VLC = new LibClasses();
	VLC->loadFilesystem(extractArchives);
	settings.init("config/settings.json", "vcmi:settings");
	persistentStorage.init("config/persistentStorage.json", "");
	VLC->loadModFilesystem();

}

DLL_LINKAGE void loadDLLClasses(bool onlyEssential)
{
	VLC->init(onlyEssential);
}

const ArtifactService * LibClasses::artifacts() const
{
	return arth.get();
}

const CreatureService * LibClasses::creatures() const
{
	return creh.get();
}

const FactionService * LibClasses::factions() const
{
	return townh.get();
}

const HeroClassService * LibClasses::heroClasses() const
{
	return heroclassesh.get();
}

const HeroTypeService * LibClasses::heroTypes() const
{
	return heroh.get();
}

#if SCRIPTING_ENABLED
const scripting::Service * LibClasses::scripts() const
{
	return scriptHandler.get();
}
#endif

const spells::Service * LibClasses::spells() const
{
	return spellh.get();
}

const SkillService * LibClasses::skills() const
{
	return skillh.get();
}

const IBonusTypeHandler * LibClasses::getBth() const
{
	return bth.get();
}

const CIdentifierStorage * LibClasses::identifiers() const
{
	return identifiersHandler.get();
}

const spells::effects::Registry * LibClasses::spellEffects() const
{
	return spells::effects::GlobalRegistry::get();
}

spells::effects::Registry * LibClasses::spellEffects()
{
	return spells::effects::GlobalRegistry::get();
}

const BattleFieldService * LibClasses::battlefields() const
{
	return battlefieldsHandler.get();
}

const ObstacleService * LibClasses::obstacles() const
{
	return obstacleHandler.get();
}

const IGameSettings * LibClasses::engineSettings() const
{
	return settingsHandler.get();
}

void LibClasses::loadFilesystem(bool extractArchives)
{
	CStopWatch loadTime;

	CResourceHandler::initialize();
	logGlobal->info("\tInitialization: %d ms", loadTime.getDiff());

	CResourceHandler::load("config/filesystem.json", extractArchives);
	logGlobal->info("\tData loading: %d ms", loadTime.getDiff());
}

void LibClasses::loadModFilesystem()
{
	CStopWatch loadTime;
	modh = std::make_unique<CModHandler>();
	identifiersHandler = std::make_unique<CIdentifierStorage>();
	logGlobal->info("\tMod handler: %d ms", loadTime.getDiff());

	modh->loadModFilesystems();
	logGlobal->info("\tMod filesystems: %d ms", loadTime.getDiff());
}

template <class Handler> void createHandler(std::shared_ptr<Handler> & handler)
{
	handler = std::make_shared<Handler>();
}

void LibClasses::init(bool onlyEssential)
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
	createHandler(spellh);
	createHandler(skillh);
	createHandler(terviewh);
	createHandler(tplh); //templates need already resolved identifiers (refactor?)
#if SCRIPTING_ENABLED
	createHandler(scriptHandler);
#endif
	createHandler(battlefieldsHandler);
	createHandler(obstacleHandler);

	modh->load();
	modh->afterLoad(onlyEssential);
}

#if SCRIPTING_ENABLED
void LibClasses::scriptsLoaded()
{
	scriptHandler->performRegistration(this);
}
#endif

LibClasses::LibClasses() = default;
LibClasses::~LibClasses() = default;

std::shared_ptr<CContentHandler> LibClasses::getContent() const
{
	return modh->content;
}

void LibClasses::setContent(std::shared_ptr<CContentHandler> content)
{
	modh->content = std::move(content);
}

VCMI_LIB_NAMESPACE_END
