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
#include "Filesystem/CResourceLoader.h"


LibClasses * VLC = NULL;

DLL_LINKAGE void preinitDLL(CConsoleHandler *Console, std::ostream *Logfile)
{
	console = Console;
	logfile = Logfile;
	VLC = new LibClasses;
	try
	{
		VLC->loadFilesystem();
	}
	HANDLE_EXCEPTION;
}

DLL_LINKAGE void loadDLLClasses()
{
	try
	{
		VLC->init();
	}
	HANDLE_EXCEPTION;
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
	tlog0<<"\t Initialization: "<<loadTime.getDiff()<<std::endl;

	CResourceHandler::loadFileSystem("", "ALL/config/filesystem.json");
	tlog0<<"\t Data loading: "<<loadTime.getDiff()<<std::endl;

	modh = new CModHandler;
	tlog0<<"\tMod handler: "<<loadTime.getDiff()<<std::endl;

	modh->initialize(CResourceHandler::getAvailableMods());
	CResourceHandler::setActiveMods(modh->getActiveMods());
	tlog0<<"\t Mod filesystems: "<<loadTime.getDiff()<<std::endl;

	tlog0<<"Basic initialization: "<<totalTime.getDiff()<<std::endl;
}

static void logHandlerLoaded(const std::string& name, CStopWatch &timer)
{
   tlog0<<"\t" << name << " handler: "<<timer.getDiff()<<std::endl;
};

template <class Handler> void createHandler(Handler *&handler, const std::string &name, CStopWatch &timer)
{
	handler = new Handler();
	handler->load();
	logHandlerLoaded(name, timer);
} 

void LibClasses::init()
{
	CStopWatch pomtime;

	createHandler(bth, "Bonus type", pomtime);
	
	createHandler(generaltexth, "General text", pomtime);

	createHandler(heroh, "Hero", pomtime);

	createHandler(arth, "Artifact", pomtime);

	createHandler(creh, "Creature", pomtime);

	createHandler(townh, "Town", pomtime);
	
	createHandler(objh, "Object", pomtime);
	
	createHandler(dobjinfo, "Def information", pomtime);

	createHandler(spellh, "Spell", pomtime);

	modh->loadActiveMods();
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
	generaltexth = new CGeneralTextHandler;
	generaltexth->load();
	arth->load(true);
	//modh->recreateHandlers();
	//modh->loadConfigFromFile ("defaultMods"); //TODO: remember last saved config
}

LibClasses::~LibClasses()
{
	clear();
}