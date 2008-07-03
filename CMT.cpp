// CMT.cpp : Defines the entry point for the console application.
//
#include "stdafx.h"
#include "SDL.h"
#include "SDL_TTF.h"
#include "hch/CVideoHandler.h"
#include "SDL_mixer.h"
#include "hch/CBuildingHandler.h"
#include "SDL_Extensions.h"
#include "SDL_framerate.h"
#include <cmath>
#include <string>
#include <vector>
#include "zlib.h"
#include <cmath>
#include "hch/CArtHandler.h"
#include "hch/CHeroHandler.h"
#include "hch/CCreatureHandler.h"
#include "hch/CAbilityHandler.h"
#include "hch/CSpellHandler.h"
#include "hch/CBuildingHandler.h"
#include "hch/CObjectHandler.h"
#include "CGameInfo.h"
#include "hch/CMusicHandler.h"
#include "hch/CLodHandler.h"
#include "hch/CDefHandler.h"
#include "hch/CTownHandler.h"
#include "hch/CDefObjInfoHandler.h"
#include "hch/CAmbarCendamo.h"
#include "mapHandler.h"
#include "global.h"
#include "CPreGame.h"
#include "hch/CGeneralTextHandler.h"
#include "CConsoleHandler.h"
#include "CCursorHandler.h"
#include "CScreenHandler.h"
#include "CPathfinder.h"
#include "CGameState.h"
#include "CCallback.h"
#include "CPlayerInterface.h"
#include "CLuaHandler.h"
#include "CLua.h"
#include "CAdvmapInterface.h"
#include "client/Graphics.h"
#include <boost/thread.hpp>
#include "lib/Connection.h"
std::string NAME = NAME_VER + std::string(" (client)");
DLL_EXPORT void initDLL(CLodHandler *b);
SDL_Surface * screen, * screen2;
extern SDL_Surface * CSDL_Ext::std32bppSurface;
TTF_Font * TNRB16, *TNR, *GEOR13, *GEORXX, *GEORM, *GEOR16;
void handleCPPObjS(std::map<int,CCPPObjectScript*> * mapa, CCPPObjectScript * script)
{
	std::vector<int> tempv = script->yourObjects();
	for (int i=0;i<tempv.size();i++)
	{
		(*mapa)[tempv[i]]=script;
	}
	CGI->state->cppscripts.insert(script);
}
int _tmain(int argc, _TCHAR* argv[])
{ 
	//boost::thread servthr(boost::bind(system,"VCMI_server.exe")); //runs server executable; 
												//TODO: add versions for other platforms
	CConnection c("localhost","3030",NAME,std::cout);
	int r;
	c >> r;		

	std::cout << NAME << std::endl;
	srand ( time(NULL) );
	CPG=NULL;
	atexit(SDL_Quit);
	CGameInfo * cgi = CGI = new CGameInfo; //contains all global informations about game (texts, lodHandlers, map handler itp.)
	//CLuaHandler luatest;
	//luatest.test(); 
		//CBIKHandler cb;
		//cb.open("CSECRET.BIK");
	std::cout << "Starting... " << std::endl;
	THC timeHandler tmh, total, pomtime;
	if(SDL_Init(SDL_INIT_VIDEO|SDL_INIT_TIMER|SDL_INIT_AUDIO/*|SDL_INIT_EVENTTHREAD*/)==0)
	{
		screen = SDL_SetVideoMode(800,600,24,SDL_SWSURFACE|SDL_DOUBLEBUF/*|SDL_FULLSCREEN*/);  //initializing important global surface
		THC std::cout<<"\tInitializing screen: "<<pomtime.getDif()<<std::endl;
		SDL_WM_SetCaption(NAME.c_str(),""); //set window title
		#if SDL_BYTEORDER == SDL_BIG_ENDIAN
			int rmask = 0xff000000;int gmask = 0x00ff0000;int bmask = 0x0000ff00;int amask = 0x000000ff;
		#else
			int rmask = 0x000000ff;	int gmask = 0x0000ff00;	int bmask = 0x00ff0000;	int amask = 0xff000000;
		#endif
		CSDL_Ext::std32bppSurface = SDL_CreateRGBSurface(SDL_SWSURFACE, 1, 1, 32, rmask, gmask, bmask, amask);
		THC std::cout<<"\tInitializing minors: "<<pomtime.getDif()<<std::endl;
		TTF_Init();
		TNRB16 = TTF_OpenFont("Fonts\\tnrb.ttf",16);
		GEOR13 = TTF_OpenFont("Fonts\\georgia.ttf",13);
		GEOR16 = TTF_OpenFont("Fonts\\georgia.ttf",16);
		GEORXX = TTF_OpenFont("Fonts\\tnrb.ttf",22);
		GEORM = TTF_OpenFont("Fonts\\georgia.ttf",10);
		atexit(TTF_Quit);
		THC std::cout<<"\tInitializing fonts: "<<pomtime.getDif()<<std::endl;
		CMusicHandler * mush = new CMusicHandler;  //initializing audio
		mush->initMusics();
		//audio initialized 
		cgi->consoleh = new CConsoleHandler;
		cgi->mush = mush;
		cgi->curh = new CCursorHandler; 
		THC std::cout<<"\tInitializing sound and cursor: "<<pomtime.getDif()<<std::endl;
		THC std::cout<<"Initializing screen, fonts and sound handling: "<<tmh.getDif()<<std::endl;
		CDefHandler::Spriteh = cgi->spriteh = new CLodHandler();
		cgi->spriteh->init("Data\\H3sprite.lod","Sprites");
		BitmapHandler::bitmaph = cgi->bitmaph = new CLodHandler;
		cgi->bitmaph->init("Data\\H3bitmap.lod","Data");
		THC std::cout<<"Loading .lod files: "<<tmh.getDif()<<std::endl;
		initDLL(cgi->bitmaph);
		THC std::cout<<"Initializing VCMI_Lib: "<<tmh.getDif()<<std::endl;

		//cgi->curh->initCursor();
		//cgi->curh->showGraphicCursor();
		pomtime.getDif();
		cgi->screenh = new CScreenHandler;
		cgi->screenh->initScreen();
		THC std::cout<<"\tScreen handler: "<<pomtime.getDif()<<std::endl;
		cgi->townh = new CTownHandler;
		cgi->townh->loadNames();
		THC std::cout<<"\tTown handler: "<<pomtime.getDif()<<std::endl;
		CAbilityHandler * abilh = new CAbilityHandler;
		abilh->loadAbilities();
		cgi->abilh = abilh;
		THC std::cout<<"\tAbility handler: "<<pomtime.getDif()<<std::endl;
		CHeroHandler * heroh = new CHeroHandler;
		heroh->loadHeroes();
		heroh->loadPortraits();
		cgi->heroh = heroh;
		THC std::cout<<"\tHero handler: "<<pomtime.getDif()<<std::endl;
		THC std::cout<<"Preparing first handlers: "<<tmh.getDif()<<std::endl;
		pomtime.getDif();
		graphics = new Graphics();
		THC std::cout<<"\tMain graphics: "<<tmh.getDif()<<std::endl;
		std::vector<CDefHandler **> animacje;
		for(std::vector<CHeroClass *>::iterator i = cgi->heroh->heroClasses.begin();i!=cgi->heroh->heroClasses.end();i++)
			animacje.push_back(&((*i)->*(&CHeroClass::moveAnim)));
		graphics->loadHeroAnim(animacje);
		THC std::cout<<"\tHero animations: "<<tmh.getDif()<<std::endl;
		THC std::cout<<"Initializing game graphics: "<<tmh.getDif()<<std::endl;
		CMessage::init();
		cgi->generaltexth = new CGeneralTextHandler;
		cgi->generaltexth->load();
		THC std::cout<<"Preparing more handlers: "<<tmh.getDif()<<std::endl;
		CPreGame * cpg = new CPreGame(); //main menu and submenus
		THC std::cout<<"Initialization CPreGame (together): "<<tmh.getDif()<<std::endl;
		THC std::cout<<"Initialization of VCMI (togeter): "<<total.getDif()<<std::endl;
		cpg->mush = mush;
		StartInfo *options = new StartInfo(cpg->runLoop());

		cgi->dobjinfo = new CDefObjInfoHandler;
		cgi->dobjinfo->load();
		THC std::cout<<"\tDef information handler: "<<pomtime.getDif()<<std::endl;

		cgi->state = new CGameState();
		cgi->state->scenarioOps = options;
		THC std::cout<<"\tGamestate: "<<pomtime.getDif()<<std::endl;

		THC tmh.getDif();pomtime.getDif();//reset timers
		CArtHandler * arth = new CArtHandler;
		arth->loadArtifacts();
		cgi->arth = arth;
		THC std::cout<<"\tArtifact handler: "<<pomtime.getDif()<<std::endl;

		CCreatureHandler * creh = new CCreatureHandler();
		creh->loadCreatures();
		cgi->creh = creh;
		THC std::cout<<"\tCreature handler: "<<pomtime.getDif()<<std::endl;

		CSpellHandler * spellh = new CSpellHandler;
		spellh->loadSpells();
		cgi->spellh = spellh;		
		THC std::cout<<"\tSpell handler: "<<pomtime.getDif()<<std::endl;

		CBuildingHandler * buildh = new CBuildingHandler;
		buildh->loadBuildings();
		cgi->buildh = buildh;
		THC std::cout<<"\tBuilding handler: "<<pomtime.getDif()<<std::endl;

		CObjectHandler * objh = new CObjectHandler;
		objh->loadObjects();
		cgi->objh = objh;
		THC std::cout<<"\tObject handler: "<<pomtime.getDif()<<std::endl;


		cgi->pathf = new CPathfinder();
		THC std::cout<<"\tPathfinder: "<<pomtime.getDif()<<std::endl;
		cgi->consoleh->cb = new CCallback(cgi->state,-1);
		cgi->consoleh->runConsole();
		THC std::cout<<"\tCallback and console: "<<pomtime.getDif()<<std::endl;
		THC std::cout<<"Handlers initialization (together): "<<tmh.getDif()<<std::endl;

		std::string mapname = cpg->ourScenSel->mapsel.ourMaps[cpg->ourScenSel->mapsel.selected].filename;
		std::cout<<"Opening map file: "<<mapname<<"\t\t"<<std::flush;
		gzFile map = gzopen(mapname.c_str(),"rb");
		std::vector<unsigned char> mapstr; int pom;
		while((pom=gzgetc(map))>=0)
		{
			mapstr.push_back(pom);
		}
		gzclose(map);
		unsigned char *initTable = new unsigned char[mapstr.size()];
		for(int ss=0; ss<mapstr.size(); ++ss)
		{
			initTable[ss] = mapstr[ss];
		}
		std::cout<<"done."<<std::endl;
		Mapa * mapa = new Mapa(initTable);
		THC std::cout<<"Reading and detecting map file (together): "<<tmh.getDif()<<std::endl;

		cgi->state->init(options,mapa,8);

		CMapHandler * mh = cgi->mh = new CMapHandler();
		THC std::cout<<"Initializing GameState (together): "<<tmh.getDif()<<std::endl;
		mh->map = mapa;
		THC std::cout<<"Creating mapHandler: "<<tmh.getDif()<<std::endl;
		mh->loadDefs();
		THC std::cout<<"Reading terrain defs: "<<tmh.getDif()<<std::endl;
		mh->init();
		THC std::cout<<"Initializing mapHandler (together): "<<tmh.getDif()<<std::endl;

		for (int i=0; i<cgi->state->scenarioOps->playerInfos.size();i++) //initializing interfaces
		{ 

			if(!cgi->state->scenarioOps->playerInfos[i].human)
				cgi->playerint.push_back(static_cast<CGameInterface*>(CAIHandler::getNewAI(new CCallback(cgi->state,cgi->state->scenarioOps->playerInfos[i].color),"EmptyAI.dll")));
			else 
			{
				cgi->state->currentPlayer=cgi->state->scenarioOps->playerInfos[i].color;
				cgi->playerint.push_back(new CPlayerInterface(cgi->state->scenarioOps->playerInfos[i].color,i));
				((CPlayerInterface*)(cgi->playerint[i]))->init(new CCallback(cgi->state,cgi->state->scenarioOps->playerInfos[i].color));
			}
		}
		///claculating FoWs for minimap
		/****************************Minimaps' FoW******************************************/
		for(int g=0; g<cgi->playerint.size(); ++g)
		{
			if(!cgi->playerint[g]->human)
				continue;
			CMinimap & mm = ((CPlayerInterface*)cgi->playerint[g])->adventureInt->minimap;

			int mw = mm.map[0]->w, mh = mm.map[0]->h,
				wo = mw/CGI->mh->sizes.x, ho = mh/CGI->mh->sizes.y;

			for(int d=0; d<cgi->mh->map->twoLevel+1; ++d)
			{
				SDL_Surface * pt = CSDL_Ext::newSurface(mm.pos.w, mm.pos.h, CSDL_Ext::std32bppSurface);

				for (int i=0; i<mw; i++)
				{
					for (int j=0; j<mh; j++)
					{
						int3 pp( ((i*CGI->mh->sizes.x)/mw), ((j*CGI->mh->sizes.y)/mh), d );

						if ( !((CPlayerInterface*)cgi->playerint[g])->cb->isVisible(pp) )
						{
							CSDL_Ext::SDL_PutPixelWithoutRefresh(pt,i,j,0,0,0);
						}
					}
				}
				CSDL_Ext::update(pt);
				mm.FoW.push_back(pt);
			}

		}

		while(1) //main game loop, one execution per turn
		{
			cgi->consoleh->cb->newTurn();
			for (int i=0;i<cgi->playerint.size();i++)
			{
				cgi->state->currentPlayer=cgi->playerint[i]->playerID;
				try
				{
					cgi->playerint[i]->yourTurn();
				}HANDLE_EXCEPTION
			}
		}
	}
	else
	{
		printf("Something was wrong: %s/n", SDL_GetError());
		return -1;
	}
}
