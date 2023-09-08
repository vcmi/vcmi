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
#include "CHeroHandler.h"
#include "CTownHandler.h"
#include "CConfigHandler.h"
#include "RoadHandler.h"
#include "RiverHandler.h"
#include "TerrainHandler.h"
#include "CBuildingHandler.h"
#include "spells/CSpellHandler.h"
#include "spells/effects/Registry.h"
#include "CSkillHandler.h"
#include "CGeneralTextHandler.h"
#include "modding/CModHandler.h"
#include "modding/CModInfo.h"
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
#include "mapping/CMapEditManager.h"
#include "ScriptHandler.h"
#include "BattleFieldHandler.h"
#include "ObstacleHandler.h"
#include "GameSettings.h"

VCMI_LIB_NAMESPACE_BEGIN

LibClasses * VLC = nullptr;

DLL_LINKAGE void preinitDLL(CConsoleHandler * Console, bool onlyEssential, bool extractArchives)
{
	console = Console;
	VLC = new LibClasses();
	VLC->loadFilesystem(extractArchives);
	settings.init();
	VLC->loadModFilesystem(onlyEssential);

}

DLL_LINKAGE void loadDLLClasses(bool onlyEssential)
{
	VLC->init(onlyEssential);
}

const ArtifactService * LibClasses::artifacts() const
{
	return arth;
}

const CreatureService * LibClasses::creatures() const
{
	return creh;
}

const FactionService * LibClasses::factions() const
{
	return townh;
}

const HeroClassService * LibClasses::heroClasses() const
{
	return &heroh->classes;
}

const HeroTypeService * LibClasses::heroTypes() const
{
	return heroh;
}

#if SCRIPTING_ENABLED
const scripting::Service * LibClasses::scripts() const
{
	return scriptHandler;
}
#endif

const spells::Service * LibClasses::spells() const
{
	return spellh;
}

const SkillService * LibClasses::skills() const
{
	return skillh;
}

const IBonusTypeHandler * LibClasses::getBth() const
{
	return bth;
}

const CIdentifierStorage * LibClasses::identifiers() const
{
	return identifiersHandler;
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
	return battlefieldsHandler;
}

const ObstacleService * LibClasses::obstacles() const
{
	return obstacleHandler;
}

const IGameSettings * LibClasses::settings() const
{
	return settingsHandler;
}

void LibClasses::updateEntity(Metatype metatype, int32_t index, const JsonNode & data)
{
	switch(metatype)
	{
	case Metatype::ARTIFACT:
		arth->updateEntity(index, data);
		break;
	case Metatype::CREATURE:
		creh->updateEntity(index, data);
		break;
	case Metatype::FACTION:
		townh->updateEntity(index, data);
		break;
	case Metatype::HERO_CLASS:
		heroh->classes.updateEntity(index, data);
		break;
	case Metatype::HERO_TYPE:
		heroh->updateEntity(index, data);
		break;
	case Metatype::SKILL:
		skillh->updateEntity(index, data);
		break;
	case Metatype::SPELL:
		spellh->updateEntity(index, data);
		break;
	default:
		logGlobal->error("Invalid Metatype id %d", static_cast<int32_t>(metatype));
		break;
	}
}

void LibClasses::loadFilesystem(bool extractArchives)
{
	CStopWatch loadTime;

	CResourceHandler::initialize();
	logGlobal->info("\tInitialization: %d ms", loadTime.getDiff());

	CResourceHandler::load("config/filesystem.json", extractArchives);
	logGlobal->info("\tData loading: %d ms", loadTime.getDiff());
}

void LibClasses::loadModFilesystem(bool onlyEssential)
{
	CStopWatch loadTime;
	modh = new CModHandler();
	identifiersHandler = new CIdentifierStorage();
	modh->loadMods(onlyEssential);
	logGlobal->info("\tMod handler: %d ms", loadTime.getDiff());

	modh->loadModFilesystems();
	logGlobal->info("\tMod filesystems: %d ms", loadTime.getDiff());
}

static void logHandlerLoaded(const std::string & name, CStopWatch & timer)
{
	logGlobal->info("\t\t %s handler: %d ms", name, timer.getDiff());
}

template <class Handler> void createHandler(Handler *&handler, const std::string &name, CStopWatch &timer)
{
	handler = new Handler();
	logHandlerLoaded(name, timer);
}

void LibClasses::init(bool onlyEssential)
{
	CStopWatch pomtime;
	CStopWatch totalTime;

	createHandler(settingsHandler, "Game Settings", pomtime);
	modh->initializeConfig();

	createHandler(generaltexth, "General text", pomtime);
	createHandler(bth, "Bonus type", pomtime);
	createHandler(roadTypeHandler, "Road", pomtime);
	createHandler(riverTypeHandler, "River", pomtime);
	createHandler(terrainTypeHandler, "Terrain", pomtime);
	createHandler(heroh, "Hero", pomtime);
	createHandler(arth, "Artifact", pomtime);
	createHandler(creh, "Creature", pomtime);
	createHandler(townh, "Town", pomtime);
	createHandler(objh, "Object", pomtime);
	createHandler(objtypeh, "Object types information", pomtime);
	createHandler(spellh, "Spell", pomtime);
	createHandler(skillh, "Skill", pomtime);
	createHandler(terviewh, "Terrain view pattern", pomtime);
	createHandler(tplh, "Template", pomtime); //templates need already resolved identifiers (refactor?)
#if SCRIPTING_ENABLED
	createHandler(scriptHandler, "Script", pomtime);
#endif
	createHandler(battlefieldsHandler, "Battlefields", pomtime);
	createHandler(obstacleHandler, "Obstacles", pomtime);
	logGlobal->info("\tInitializing handlers: %d ms", totalTime.getDiff());

	modh->load();
	modh->afterLoad(onlyEssential);
}

void LibClasses::clear()
{
	delete heroh;
	delete arth;
	delete creh;
	delete townh;
	delete objh;
	delete objtypeh;
	delete spellh;
	delete skillh;
	delete modh;
	delete bth;
	delete tplh;
	delete terviewh;
#if SCRIPTING_ENABLED
	delete scriptHandler;
#endif
	delete battlefieldsHandler;
	delete generaltexth;
	delete identifiersHandler;
	makeNull();
}

void LibClasses::makeNull()
{
	generaltexth = nullptr;
	heroh = nullptr;
	arth = nullptr;
	creh = nullptr;
	townh = nullptr;
	objh = nullptr;
	objtypeh = nullptr;
	spellh = nullptr;
	skillh = nullptr;
	modh = nullptr;
	bth = nullptr;
	tplh = nullptr;
	terviewh = nullptr;
#if SCRIPTING_ENABLED
	scriptHandler = nullptr;
#endif
	battlefieldsHandler = nullptr;
	identifiersHandler = nullptr;
}

LibClasses::LibClasses()
{
	//init pointers to handlers
	makeNull();
}

void LibClasses::callWhenDeserializing()
{
	//FIXME: check if any of these are needed
	//generaltexth = new CGeneralTextHandler();
	//generaltexth->load();
	//arth->load(true);
	//modh->recreateHandlers();
	//modh->loadConfigFromFile ("defaultMods"); //TODO: remember last saved config
}

#if SCRIPTING_ENABLED
void LibClasses::scriptsLoaded()
{
	scriptHandler->performRegistration(this);
}
#endif

LibClasses::~LibClasses()
{
	clear();
}

std::shared_ptr<CContentHandler> LibClasses::getContent() const
{
	return modh->content;
}

void LibClasses::setContent(std::shared_ptr<CContentHandler> content)
{
	modh->content = std::move(content);
}

VCMI_LIB_NAMESPACE_END
