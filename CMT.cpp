// CMT.cpp : Defines the entry point for the console application.
//
#include "stdafx.h"
#include "SDL.h"
#include "SDL_TTF.h"
#include "CVideoHandler.h"
#include "SDL_mixer.h"
#include "CBuildingHandler.h"
#include "SDL_Extensions.h"
#include "SDL_framerate.h"
#include <cmath>
#include <stdio.h>
#include <string.h>
#include <string>
#include <assert.h>
#include <vector>
#include "zlib.h"
#include <cmath>
#include <ctime>
#include "CArtHandler.h"
#include "CHeroHandler.h"
#include "CCreatureHandler.h"
#include "CAbilityHandler.h"
#include "CSpellHandler.h"
#include "CBuildingHandler.h"
#include "CObjectHandler.h"
#include "CGameInfo.h"
#include "CMusicHandler.h"
#include "CSemiLodHandler.h"
#include "CLodHandler.h"
#include "CDefHandler.h"
#include "CSndHandler.h"
#include "CTownHandler.h"
#include "CDefObjInfoHandler.h"
#include "CAmbarCendamo.h"
#include "mapHandler.h"
#include "global.h"
#include "CPreGame.h"
#include "CGeneralTextHandler.h"
#include "CConsoleHandler.h"
#include "CCursorHandler.h"
#include "CScreenHandler.h"

#if defined(MSDOS) || defined(OS2) || defined(WIN32) || defined(__CYGWIN__)
#  include <fcntl.h>
#  include <io.h>
#  define SET_BINARY_MODE(file) setmode(fileno(file), O_BINARY)
#else
#  define SET_BINARY_MODE(file)
#endif

#define CHUNK 16384
const char * NAME = "VCMI 0.3";

/* Compress from file source to file dest until EOF on source.
   def() returns Z_OK on success, Z_MEM_ERROR if memory could not be
   allocated for processing, Z_STREAM_ERROR if an invalid compression
   level is supplied, Z_VERSION_ERROR if the version of zlib.h and the
   version of the library linked do not match, or Z_ERRNO if there is
   an error reading or writing the files. */
SDL_Surface * ekran, * screen, * screen2;
TTF_Font * TNRB16, *TNR, *GEOR13, *GEORXX;

int _tmain(int argc, _TCHAR* argv[])
{ 

		//CBIKHandler cb;
		//cb.open("CSECRET.BIK");

	THC timeHandler tmh;
	THC tmh.getDif();
	int xx=0, yy=0, zz=0;
	SDL_Event sEvent;
	srand ( time(NULL) );
	SDL_Surface *temp;
	std::vector<SDL_Surface*> Sprites;
	float i;
	if(SDL_Init(SDL_INIT_VIDEO|SDL_INIT_TIMER|SDL_INIT_AUDIO/*|SDL_INIT_EVENTTHREAD*/)==0)
	{
		CPG=NULL;
		TTF_Init();
		atexit(TTF_Quit);
		atexit(SDL_Quit);
		//TNRB = TTF_OpenFont("Fonts\\tnrb.ttf",16);
		TNRB16 = TTF_OpenFont("Fonts\\tnrb.ttf",16);
		//TNR = TTF_OpenFont("Fonts\\tnr.ttf",10);
		GEOR13 = TTF_OpenFont("Fonts\\georgia.ttf",13);
		GEORXX = TTF_OpenFont("Fonts\\tnrb.ttf",22);

		//initializing audio
		CMusicHandler * mush = new CMusicHandler;
		mush->initMusics();
		//CSndHandler snd("Heroes3.snd");
		//snd.extract("AELMMOVE.wav","snddd.wav");
		//audio initialized

		/*if(Mix_PlayMusic(mush->mainMenuWoG, -1)==-1) //uncomment this fragment to have music
		{
			printf("Mix_PlayMusic: %s\n", Mix_GetError());
			// well, there's no music, but most games don't break without music...
		}*/

		screen2 = SDL_SetVideoMode(800,600,24,SDL_SWSURFACE|SDL_DOUBLEBUF/*|SDL_FULLSCREEN*/);
		screen = SDL_ConvertSurface(screen2, screen2->format, SDL_SWSURFACE);
		ekran = screen;

		SDL_WM_SetCaption(NAME,""); //set window title
		CGameInfo * cgi = new CGameInfo; //contains all global informations about game (texts, lodHandlers, map handler itp.)
		CGameInfo::mainObj = cgi;
		cgi->consoleh = new CConsoleHandler;
		cgi->consoleh->runConsole();
		cgi->mush = mush;
		cgi->curh = new CCursorHandler;

		THC std::cout<<"Initializing screen, fonts and sound handling: "<<tmh.getDif()<<std::endl;
		cgi->spriteh = new CLodHandler;
		cgi->spriteh->init(std::string("Data\\H3sprite.lod"));
		cgi->bitmaph = new CLodHandler;
		cgi->bitmaph->init(std::string("Data\\H3bitmap.lod"));

		cgi->curh->initCursor();
		cgi->curh->showGraphicCursor();

		cgi->screenh = new CScreenHandler;
		cgi->screenh->initScreen();

		//colors initialization
		SDL_Color p;
		p.unused = 0;
		p.r = 0xff; p.g = 0x0; p.b = 0x0; //red
		cgi->playerColors.push_back(p); //red
		p.r = 0x31; p.g = 0x52; p.b = 0xff; //blue
		cgi->playerColors.push_back(p); //blue
		p.r = 0x9c; p.g = 0x73; p.b = 0x52;//tan
		cgi->playerColors.push_back(p);//tan
		p.r = 0x42; p.g = 0x94; p.b = 0x29; //green
		cgi->playerColors.push_back(p); //green
		p.r = 0xff; p.g = 0x84; p.b = 0x0; //orange
		cgi->playerColors.push_back(p); //orange
		p.r = 0x8c; p.g = 0x29; p.b = 0xa5; //purple
		cgi->playerColors.push_back(p); //purple
		p.r = 0x09; p.g = 0x9c; p.b = 0xa5;//teal
		cgi->playerColors.push_back(p);//teal
		p.r = 0xc6; p.g = 0x7b; p.b = 0x8c;//pink
		cgi->playerColors.push_back(p);//pink
		p.r = 0x84; p.g = 0x84; p.b = 0x84;//gray
		cgi->neutralColor = p;//gray
		//colors initialized

		cgi->townh = new CTownHandler;
		cgi->townh->loadNames();

		CAbilityHandler * abilh = new CAbilityHandler;
		abilh->loadAbilities();
		cgi->abilh = abilh;
		CHeroHandler * heroh = new CHeroHandler;
		heroh->loadHeroes();
		heroh->loadPortraits();
		cgi->heroh = heroh;

		THC std::cout<<"Loading .lods: "<<tmh.getDif()<<std::endl;
		CPreGame * cpg = new CPreGame(); //main menu and submenus
		THC std::cout<<"Initialization CPreGame (together): "<<tmh.getDif()<<std::endl;
		cpg->mush = mush;
		cgi->scenarioOps = cpg->runLoop();
		THC tmh.getDif();

		cgi->sspriteh = new CSemiLodHandler();
		cgi->sspriteh->openLod("H3sprite.lod");
		CArtHandler * arth = new CArtHandler;
		arth->loadArtifacts();
		cgi->arth = arth;
		CCreatureHandler * creh = new CCreatureHandler;
		creh->loadCreatures();
		cgi->creh = creh;

		CSpellHandler * spellh = new CSpellHandler;
		spellh->loadSpells();
		cgi->spellh = spellh;
		CBuildingHandler * buildh = new CBuildingHandler;
		buildh->loadBuildings();
		cgi->buildh = buildh;
		CObjectHandler * objh = new CObjectHandler;
		objh->loadObjects();
		cgi->objh = objh;
		cgi->generaltexth = new CGeneralTextHandler;
		cgi->generaltexth->load();
		cgi->dobjinfo = new CDefObjInfoHandler;
		cgi->dobjinfo->load();
		cgi->state = new CGameState();
		cgi->pathf = new CPathfinder();
		THC std::cout<<"Handlers initailization: "<<tmh.getDif()<<std::endl;

		std::string mapname;
		if(CPG->ourScenSel->mapsel.selected==0) CPG->ourScenSel->mapsel.selected = 1; //only for tests
		if (CPG) mapname = CPG->ourScenSel->mapsel.ourMaps[CPG->ourScenSel->mapsel.selected].filename;
		gzFile map = gzopen(mapname.c_str(),"rb");
		std::string mapstr;int pom;
		while((pom=gzgetc(map))>=0)
		{
			mapstr+=pom;
		}
		gzclose(map);
		unsigned char *initTable = new unsigned char[mapstr.size()];
		for(int ss=0; ss<mapstr.size(); ++ss)
		{
			initTable[ss] = mapstr[ss];
		}
#define CHOOSE
#ifdef CHOOSE
		CAmbarCendamo * ac = new CAmbarCendamo(initTable); //4gryf
#else
		CAmbarCendamo * ac = new CAmbarCendamo("RoEtest"); //4gryf
#endif
		CMapHeader * mmhh = new CMapHeader(ac->bufor); //czytanie nag³ówka
		cgi->ac = ac;
		THC std::cout<<"Reading file: "<<tmh.getDif()<<std::endl;
		ac->deh3m();
		THC std::cout<<"Detecting file (together): "<<tmh.getDif()<<std::endl;
		ac->loadDefs();
		THC std::cout<<"Reading terrain defs: "<<tmh.getDif()<<std::endl;
		CMapHandler * mh = new CMapHandler();
		cgi->mh = mh;
		mh->reader = ac;
		THC std::cout<<"Creating mapHandler: "<<tmh.getDif()<<std::endl;
		mh->init();
		THC std::cout<<"Initializing mapHandler: "<<tmh.getDif()<<std::endl;
		//SDL_Rect * sr = new SDL_Rect(); sr->h=64;sr->w=64;sr->x=0;sr->y=0;
		//SDL_Surface * teren = mh->terrainRect(xx,yy,25,19);
		//THC std::cout<<"Preparing terrain to display: "<<tmh.getDif()<<std::endl;
		//SDL_BlitSurface(teren,NULL,ekran,NULL);
		//SDL_FreeSurface(teren);
		//SDL_UpdateRect(ekran, 0, 0, ekran->w, ekran->h);
		//THC std::cout<<"Displaying terrain: "<<tmh.getDif()<<std::endl;


		for (int i=0; i<cgi->scenarioOps.playerInfos.size();i++) //initializing interfaces
		{
			if(cgi->scenarioOps.playerInfos[i].name=="AI")
				cgi->playerint.push_back(new CGlobalAI());
			else 
			{
				cgi->playerint.push_back(new CPlayerInterface(cgi->scenarioOps.playerInfos[i].color,i));
				((CPlayerInterface*)(cgi->playerint[i]))->init();
			}
		}

		while(1) //main game loop, one execution per turn
		{
			for (int i=0;i<cgi->playerint.size();i++)
			{
				cgi->state->currentPlayer=cgi->playerint[i]->playerID;
				cgi->playerint[i]->yourTurn();
			}
		}
	}
	else
	{
		printf("Something was wrong: %s/n", SDL_GetError());
		return -1;
	}
}
