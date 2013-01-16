#include "StdInc.h"
#include "VCMI_Lib.h"


#include "CArtHandler.h"
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

/*
 * VCMI_Lib.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

LibClasses * VLC = NULL;

DLL_LINKAGE VCMIDirs GVCMIDirs;

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

void LibClasses::init()
{
	CStopWatch pomtime;

	generaltexth = new CGeneralTextHandler;
	generaltexth->load();
	tlog0<<"\tGeneral text handler: "<<pomtime.getDiff()<<std::endl;

	heroh = new CHeroHandler;
	heroh->load();
	tlog0 <<"\tHero handler: "<<pomtime.getDiff()<<std::endl;

	arth = new CArtHandler;
	arth->loadArtifacts(false);
	tlog0<<"\tArtifact handler: "<<pomtime.getDiff()<<std::endl;

	creh = new CCreatureHandler();
	creh->loadCreatures();
	tlog0<<"\tCreature handler: "<<pomtime.getDiff()<<std::endl;

	townh = new CTownHandler;
	townh->load();
	tlog0<<"\tTown handler: "<<pomtime.getDiff()<<std::endl;

	objh = new CObjectHandler;
	objh->loadObjects();
	tlog0<<"\tObject handler: "<<pomtime.getDiff()<<std::endl;

	dobjinfo = new CDefObjInfoHandler;
	dobjinfo->load();
	tlog0<<"\tDef information handler: "<<pomtime.getDiff()<<std::endl;

	spellh = new CSpellHandler;
	spellh->loadSpells();
	tlog0<<"\tSpell handler: "<<pomtime.getDiff()<<std::endl;

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
	makeNull();
}

void LibClasses::makeNull()
{
	generaltexth = NULL;
	heroh = NULL;
	arth = NULL;
	creh = NULL;
	townh = NULL;
	objh = NULL;
	dobjinfo = NULL;
	spellh = NULL;
	modh = NULL;
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
	arth->loadArtifacts(true);
	//modh->recreateHandlers();
	//modh->loadConfigFromFile ("defaultMods"); //TODO: remember last saved config
}

LibClasses::~LibClasses()
{
	clear();
}