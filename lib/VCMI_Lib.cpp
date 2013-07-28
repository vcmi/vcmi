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
#include "CDefObjInfoHandler.h"
#include "CHeroHandler.h"
#include "CObjectHandler.h"
#include "CTownHandler.h"
#include "CBuildingHandler.h"
#include "CSpellHandler.h"
#include "CGeneralTextHandler.h"
#include "CModHandler.h"
#include "IGameEventsReceiver.h"
#include "CStopWatch.h"
#include "VCMIDirs.h"
#include "filesystem/Filesystem.h"
#include "CConsoleHandler.h"

LibClasses * VLC = nullptr;

DLL_LINKAGE void preinitDLL(CConsoleHandler *Console)
{
	console = Console;
	VLC = new LibClasses;
	//try
	{
		VLC->loadFilesystem();
	}
	//HANDLE_EXCEPTION;
}

DLL_LINKAGE void loadDLLClasses()
{
	//try
	{
		VLC->init();
	}
	//HANDLE_EXCEPTION;
}

const IBonusTypeHandler * LibClasses::getBth() const
{
	return bth;
}

void LibClasses::loadFilesystem()
{
	CStopWatch totalTime;
	CStopWatch loadTime;

	CResourceHandler::initialize();
    logGlobal->infoStream()<<"\t Initialization: "<<loadTime.getDiff();

	CResourceHandler::loadMainFileSystem("config/filesystem.json");
    logGlobal->infoStream()<<"\t Data loading: "<<loadTime.getDiff();

	modh = new CModHandler;
    logGlobal->infoStream()<<"\tMod handler: "<<loadTime.getDiff();

	modh->initialize(CResourceHandler::getAvailableMods());
	CResourceHandler::setActiveMods(modh->getActiveMods());
    logGlobal->infoStream()<<"\t Mod filesystems: "<<loadTime.getDiff();

    logGlobal->infoStream()<<"Basic initialization: "<<totalTime.getDiff();
}

static void logHandlerLoaded(const std::string& name, CStopWatch &timer)
{
   logGlobal->infoStream()<<"\t\t" << name << " handler: "<<timer.getDiff();
};

template <class Handler> void createHandler(Handler *&handler, const std::string &name, CStopWatch &timer)
{
	handler = new Handler();
	logHandlerLoaded(name, timer);
} 

void LibClasses::init()
{
	CStopWatch pomtime, totalTime;

	modh->beforeLoad();

	createHandler(bth, "Bonus type", pomtime);
	
	createHandler(generaltexth, "General text", pomtime);

	createHandler(heroh, "Hero", pomtime);

	createHandler(arth, "Artifact", pomtime);

	createHandler(creh, "Creature", pomtime);

	createHandler(townh, "Town", pomtime);
	
	createHandler(objh, "Object", pomtime);
	
	createHandler(dobjinfo, "Def information", pomtime);

	createHandler(spellh, "Spell", pomtime);

	logGlobal->infoStream()<<"\tInitializing handlers: "<< totalTime.getDiff();

	modh->loadGameContent();
	modh->reload();
	//FIXME: make sure that everything is ok after game restart
	//TODO: This should be done every time mod config changes

	IS_AI_ENABLED = false;
}

void LibClasses::clear()
{
	delete generaltexth;
	delete heroh;
	delete arth;
	delete creh;
	delete townh;
	delete objh;
	delete dobjinfo;
	delete spellh;
	delete modh;
	delete bth;
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
	dobjinfo = nullptr;
	spellh = nullptr;
	modh = nullptr;
	bth = nullptr;
}

LibClasses::LibClasses()
{
	//init pointers to handlers
	makeNull();
}

void LibClasses::callWhenDeserializing()
{
	// FIXME: check if any of these are needed
	//generaltexth = new CGeneralTextHandler;
	//generaltexth->load();
	//arth->load(true);
	//modh->recreateHandlers();
	//modh->loadConfigFromFile ("defaultMods"); //TODO: remember last saved config
}

LibClasses::~LibClasses()
{
	clear();
}
