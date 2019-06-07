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
#include "mapObjects/CObjectClassesHandler.h"
#include "CHeroHandler.h"
#include "mapObjects/CObjectHandler.h"
#include "CTownHandler.h"
#include "CBuildingHandler.h"
#include "spells/CSpellHandler.h"
#include "spells/effects/Registry.h"
#include "CSkillHandler.h"
#include "CGeneralTextHandler.h"
#include "CModHandler.h"
#include "IGameEventsReceiver.h"
#include "CStopWatch.h"
#include "VCMIDirs.h"
#include "filesystem/Filesystem.h"
#include "CConsoleHandler.h"
#include "rmg/CRmgTemplateStorage.h"
#include "mapping/CMapEditManager.h"
#include "ScriptHandler.h"

LibClasses * VLC = nullptr;

DLL_LINKAGE void preinitDLL(CConsoleHandler * Console, bool onlyEssential)
{
	console = Console;
	VLC = new LibClasses();
	try
	{
		VLC->loadFilesystem(onlyEssential);
	}
	catch(...)
	{
		handleException();
		throw;
	}
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

const scripting::Service * LibClasses::scripts() const
{
	return scriptHandler;
}

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

const spells::effects::Registry * LibClasses::spellEffects() const
{
	return spells::effects::GlobalRegistry::get();
}

spells::effects::Registry * LibClasses::spellEffects()
{
	return spells::effects::GlobalRegistry::get();
}

void LibClasses::loadFilesystem(bool onlyEssential)
{
	CStopWatch totalTime;
	CStopWatch loadTime;

	CResourceHandler::initialize();
	logGlobal->info("\tInitialization: %d ms", loadTime.getDiff());

	CResourceHandler::load("config/filesystem.json");
	logGlobal->info("\tData loading: %d ms", loadTime.getDiff());

	modh = new CModHandler();
	logGlobal->info("\tMod handler: %d ms", loadTime.getDiff());

	modh->loadMods(onlyEssential);
	modh->loadModFilesystems();
	logGlobal->info("\tMod filesystems: %d ms", loadTime.getDiff());

	logGlobal->info("Basic initialization: %d ms", totalTime.getDiff());
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
	CStopWatch pomtime, totalTime;

	modh->initializeConfig();

	createHandler(bth, "Bonus type", pomtime);

	createHandler(generaltexth, "General text", pomtime);

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

	createHandler(scriptHandler, "Script", pomtime);

	logGlobal->info("\tInitializing handlers: %d ms", totalTime.getDiff());

	modh->load();

	modh->afterLoad(onlyEssential);

	//FIXME: make sure that everything is ok after game restart
	//TODO: This should be done every time mod config changes
}

void LibClasses::clear()
{
	delete generaltexth;
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
	delete scriptHandler;
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
	scriptHandler = nullptr;
}

LibClasses::LibClasses()
{
	IS_AI_ENABLED = false;
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

void LibClasses::scriptsLoaded()
{
	scriptHandler->performRegistration(this);
}

LibClasses::~LibClasses()
{
	clear();
}

void LibClasses::update800()
{
	vstd::clear_pointer(scriptHandler);
	scriptHandler = new scripting::ScriptHandler();
}

