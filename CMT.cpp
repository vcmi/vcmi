// CMT.cpp : Defines the entry point for the console application.
//
#include "stdafx.h"
#include "SDL.h"
#include "SDL_TTF.h"
#include "hch\CVideoHandler.h"
#include "SDL_mixer.h"
#include "hch\CBuildingHandler.h"
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
#include "hch\CArtHandler.h"
#include "hch\CHeroHandler.h"
#include "hch\CCreatureHandler.h"
#include "hch\CAbilityHandler.h"
#include "hch\CSpellHandler.h"
#include "hch\CBuildingHandler.h"
#include "hch\CObjectHandler.h"
#include "CGameInfo.h"
#include "hch\CMusicHandler.h"
#include "hch\CSemiLodHandler.h"
#include "hch\CLodHandler.h"
#include "hch\CDefHandler.h"
#include "hch\CSndHandler.h"
#include "hch\CTownHandler.h"
#include "hch\CDefObjInfoHandler.h"
#include "hch\CAmbarCendamo.h"
#include "mapHandler.h"
#include "global.h"
#include "CPreGame.h"
#include "hch\CGeneralTextHandler.h"
#include "CConsoleHandler.h"
#include "CCursorHandler.h"
#include "CScreenHandler.h"
#include "CPathfinder.h"
#include "CGameState.h"
#include "CCallback.h"
#include "CPlayerInterface.h"
#include "CLuaHandler.h"
#include "CLua.h"
#if defined(MSDOS) || defined(OS2) || defined(WIN32) || defined(__CYGWIN__)
#  include <fcntl.h>
#  include <io.h>
#  define SET_BINARY_MODE(file) setmode(fileno(file), O_BINARY)
#else
#  define SET_BINARY_MODE(file)
#endif

#define CHUNK 16384
const char * NAME = "VCMI 0.4 \"Vingilot\"";

SDL_Surface * ekran, * screen, * screen2;
TTF_Font * TNRB16, *TNR, *GEOR13, *GEORXX, *GEORM;
void handleCPPObjS(std::map<int,CCPPObjectScript*> * mapa, CCPPObjectScript * script)
{
	std::vector<int> tempv = script->yourObjects();
	for (int i=0;i<tempv.size();i++)
	{
		(*mapa)[tempv[i]]=script;
	}
	CGI->state->cppscripts.insert(script);
}
void initGameState(CGameInfo * cgi)
{
	cgi->state->day=0;
	/*********creating players entries in gs****************************************/
	for (int i=0; i<cgi->scenarioOps.playerInfos.size();i++)
	{
		std::pair<int,PlayerState> ins(cgi->scenarioOps.playerInfos[i].color,PlayerState());
		ins.second.color=ins.first;
		ins.second.serial=i;
		cgi->state->players.insert(ins);
	}
	/******************RESOURCES****************************************************/
	//TODO: zeby komputer dostawal inaczej niz gracz
	std::vector<int> startres;
	std::ifstream tis("config/startres.txt");
	int k;
	for (int j=0;j<cgi->scenarioOps.difficulty;j++)
	{
		tis >> k;
		for (int z=0;z<RESOURCE_QUANTITY;z++)
			tis>>k;
	}
	tis >> k;
	for (int i=0;i<RESOURCE_QUANTITY;i++)
	{
		tis >> k;
		startres.push_back(k);
	}
	tis.close();
	for (std::map<int,PlayerState>::iterator i = cgi->state->players.begin(); i!=cgi->state->players.end(); i++)
	{
		(*i).second.resources.resize(RESOURCE_QUANTITY);
		for (int x=0;x<RESOURCE_QUANTITY;x++)
			(*i).second.resources[x] = startres[x];

	}

	/*************************HEROES************************************************/
	for (int i=0; i<cgi->heroh->heroInstances.size();i++) //heroes instances
	{
		if (!cgi->heroh->heroInstances[i]->type || cgi->heroh->heroInstances[i]->getOwner()<0)
			continue;
		//CGHeroInstance * vhi = new CGHeroInstance();
		//*vhi=*(cgi->heroh->heroInstances[i]);
		CGHeroInstance * vhi = (cgi->heroh->heroInstances[i]);
		vhi->subID = vhi->type->ID;
		if (!vhi->level)
		{
			vhi->exp=40+rand()%50;
			vhi->level = 1;
		}
		if (vhi->level>1) ;//TODO dodac um dr, ale potrzebne los
		if ((!vhi->primSkills.size()) || (vhi->primSkills[0]<0))
		{
			if (vhi->primSkills.size()<PRIMARY_SKILLS)
				vhi->primSkills.resize(PRIMARY_SKILLS);
			vhi->primSkills[0] = vhi->type->heroClass->initialAttack;
			vhi->primSkills[1] = vhi->type->heroClass->initialDefence;
			vhi->primSkills[2] = vhi->type->heroClass->initialPower;
			vhi->primSkills[3] = vhi->type->heroClass->initialKnowledge;
			vhi->mana = vhi->primSkills[3]*10;
		}
		if (!vhi->name.length())
		{
			vhi->name = vhi->type->name;
		}
		if (!vhi->biography.length())
		{
			vhi->biography = vhi->type->biography;
		}
		if (vhi->portrait < 0)
			vhi->portrait = vhi->type->ID;

		//initial army
		if (!vhi->army.slots.size())
		{
			vhi->army.slots[0].first = &(cgi->creh->creatures[(cgi->creh->nameToID[vhi->type->refType1stack])]);
			vhi->army.slots[0].second = (rand()%(vhi->type->high1stack-vhi->type->low1stack))+vhi->type->low1stack;
			vhi->army.slots[1].first = &(cgi->creh->creatures[(cgi->creh->nameToID[vhi->type->refType2stack])]);
			vhi->army.slots[1].second = (rand()%(vhi->type->high2stack-vhi->type->low2stack))+vhi->type->low2stack;
			vhi->army.slots[2].first = &(cgi->creh->creatures[(cgi->creh->nameToID[vhi->type->refType3stack])]);
			vhi->army.slots[2].second = (rand()%(vhi->type->high3stack-vhi->type->low3stack))+vhi->type->low3stack;
		}

		cgi->state->players[vhi->getOwner()].heroes.push_back(vhi);

	}
	/*************************FOG**OF**WAR******************************************/		
	for(std::map<int, PlayerState>::iterator k=cgi->state->players.begin(); k!=cgi->state->players.end(); ++k)
	{
		k->second.fogOfWarMap.resize(cgi->ac->map.width, Woff);
		for(int g=-Woff; g<cgi->ac->map.width+Woff; ++g)
			k->second.fogOfWarMap[g].resize(cgi->ac->map.height, Hoff);

		for(int g=-Woff; g<cgi->ac->map.width+Woff; ++g)
			for(int h=-Hoff; h<cgi->ac->map.height+Hoff; ++h)
				k->second.fogOfWarMap[g][h].resize(cgi->ac->map.twoLevel+1, 0);

		for(int g=-Woff; g<cgi->ac->map.width+Woff; ++g)
			for(int h=-Hoff; h<cgi->ac->map.height+Hoff; ++h)
				for(int v=0; v<cgi->ac->map.twoLevel+1; ++v)
					k->second.fogOfWarMap[g][h][v] = 0;
		for(int xd=0; xd<cgi->ac->map.width; ++xd) //revealing part of map around heroes
		{
			for(int yd=0; yd<cgi->ac->map.height; ++yd)
			{
				for(int ch=0; ch<k->second.heroes.size(); ++ch)
				{
					int deltaX = (k->second.heroes[ch]->getPosition(false).x-xd)*(k->second.heroes[ch]->getPosition(false).x-xd);
					int deltaY = (k->second.heroes[ch]->getPosition(false).y-yd)*(k->second.heroes[ch]->getPosition(false).y-yd);
					if(deltaX+deltaY<k->second.heroes[ch]->getSightDistance()*k->second.heroes[ch]->getSightDistance())
						k->second.fogOfWarMap[xd][yd][k->second.heroes[ch]->getPosition(false).z] = 1;
				}
			}
		}
	}
	/****************************TOWNS************************************************/
	for (int i=0;i<cgi->townh->townInstances.size();i++)
	{
		CGTownInstance * vti = new CGTownInstance();
		(*vti)=*(cgi->townh->townInstances[i]);
		if (vti->name.length()==0) // if town hasn't name we draw it
			vti->name=vti->town->names[rand()%vti->town->names.size()];
		
		cgi->state->players[vti->getOwner()].towns.push_back(vti);
	}

	for(std::map<int, PlayerState>::iterator k=cgi->state->players.begin(); k!=cgi->state->players.end(); ++k)
	{
		if(k->first==-1 || k->first==255)
			continue;
		for(int xd=0; xd<cgi->ac->map.width; ++xd) //revealing part of map around towns
		{
			for(int yd=0; yd<cgi->ac->map.height; ++yd)
			{
				for(int ch=0; ch<k->second.towns.size(); ++ch)
				{
					int deltaX = (k->second.towns[ch]->pos.x-xd)*(k->second.towns[ch]->pos.x-xd);
					int deltaY = (k->second.towns[ch]->pos.y-yd)*(k->second.towns[ch]->pos.y-yd);
					if(deltaX+deltaY<k->second.towns[ch]->getSightDistance()*k->second.towns[ch]->getSightDistance())
						k->second.fogOfWarMap[xd][yd][k->second.towns[ch]->pos.z] = 1;
				}
			}
		}
	}

	/****************************SCRIPTS************************************************/
	std::map<int, std::map<std::string, CObjectScript*> > * skrypty = &cgi->state->objscr; //alias for easier access
	/****************************C++ OBJECT SCRIPTS************************************************/
	std::map<int,CCPPObjectScript*> scripts;
	CScriptCallback * csc = new CScriptCallback();
	csc->gs = cgi->state;
	handleCPPObjS(&scripts,new CVisitableOPH(csc));
	handleCPPObjS(&scripts,new CVisitableOPW(csc));
	handleCPPObjS(&scripts,new CPickable(csc));
	handleCPPObjS(&scripts,new CMines(csc));
	//created map

	/****************************LUA OBJECT SCRIPTS************************************************/
	std::vector<std::string> * lf = CLuaHandler::searchForScripts("scripts/lua/objects"); //files
	for (int i=0; i<lf->size(); i++)
	{
		try
		{
			std::vector<std::string> * temp =  CLuaHandler::functionList((*lf)[i]);
			CLuaObjectScript * objs = new CLuaObjectScript((*lf)[i]);
			CLuaCallback::registerFuncs(objs->is);
			//objs
			for (int j=0; j<temp->size(); j++)
			{
				int obid ; //obj ID
				int dspos = (*temp)[j].find_first_of('_');
				obid = atoi((*temp)[j].substr(dspos+1,(*temp)[j].size()-dspos-1).c_str());
				std::string fname = (*temp)[j].substr(0,dspos);
				if (skrypty->find(obid)==skrypty->end())
					skrypty->insert(std::pair<int, std::map<std::string, CObjectScript*> >(obid,std::map<std::string,CObjectScript*>()));
				(*skrypty)[obid].insert(std::pair<std::string, CObjectScript*>(fname,objs));
			}
			delete temp;
		}HANDLE_EXCEPTION
	}
	/****************************INITIALIZING OBJECT SCRIPTS************************************************/
	std::string temps("newObject");
	for (int i=0; i<CGI->objh->objInstances.size(); i++)
	{
		//c++ scripts
		if (scripts.find(CGI->objh->objInstances[i]->ID) != scripts.end())
		{
			CGI->objh->objInstances[i]->state = scripts[CGI->objh->objInstances[i]->ID];
			CGI->objh->objInstances[i]->state->newObject(CGI->objh->objInstances[i]);
		}
		else 
		{
			CGI->objh->objInstances[i]->state = NULL;
		}

		// lua scripts
		if(cgi->state->checkFunc(CGI->objh->objInstances[i]->ID,temps))
			(*skrypty)[CGI->objh->objInstances[i]->ID][temps]->newObject(CGI->objh->objInstances[i]);
	}

	delete lf;
}

int _tmain(int argc, _TCHAR* argv[])
{ 
	//CLuaHandler luatest;
	//luatest.test();
	
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
		GEORM = TTF_OpenFont("Fonts\\georgia.ttf",10);

		CMusicHandler * mush = new CMusicHandler;  //initializing audio
		mush->initMusics();
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
		cgi->mush = mush;
		cgi->curh = new CCursorHandler;

		THC std::cout<<"Initializing screen, fonts and sound handling: "<<tmh.getDif()<<std::endl;
		cgi->spriteh = new CLodHandler;
		cgi->spriteh->init(std::string("Data\\H3sprite.lod"));
		cgi->bitmaph = new CLodHandler;
		cgi->bitmaph->init(std::string("Data\\H3bitmap.lod"));
		THC std::cout<<"Loading .lod files: "<<tmh.getDif()<<std::endl;

		cgi->curh->initCursor();
		cgi->curh->showGraphicCursor();

		cgi->screenh = new CScreenHandler;
		cgi->screenh->initScreen();

		THC std::cout<<"Preparing first handlers: "<<tmh.getDif()<<std::endl;

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
		THC std::cout<<"Preparing players' colours: "<<tmh.getDif()<<std::endl;
		CMessage::init();
		cgi->townh = new CTownHandler;
		cgi->townh->loadNames();
		CAbilityHandler * abilh = new CAbilityHandler;
		abilh->loadAbilities();
		cgi->abilh = abilh;
		CHeroHandler * heroh = new CHeroHandler;
		heroh->loadHeroes();
		heroh->loadPortraits();
		cgi->heroh = heroh;
		cgi->generaltexth = new CGeneralTextHandler;
		cgi->generaltexth->load();
		THC std::cout<<"Preparing more handlers: "<<tmh.getDif()<<std::endl;

		//initializing hero flags

		cgi->heroh->flags1.push_back(cgi->spriteh->giveDef("ABF01L.DEF")); //red
		cgi->heroh->flags1.push_back(cgi->spriteh->giveDef("ABF01G.DEF")); //blue
		cgi->heroh->flags1.push_back(cgi->spriteh->giveDef("ABF01R.DEF")); //tan
		cgi->heroh->flags1.push_back(cgi->spriteh->giveDef("ABF01D.DEF")); //green
		cgi->heroh->flags1.push_back(cgi->spriteh->giveDef("ABF01B.DEF")); //orange
		cgi->heroh->flags1.push_back(cgi->spriteh->giveDef("ABF01P.DEF")); //purple
		cgi->heroh->flags1.push_back(cgi->spriteh->giveDef("ABF01W.DEF")); //teal
		cgi->heroh->flags1.push_back(cgi->spriteh->giveDef("ABF01K.DEF")); //pink

		for(int q=0; q<8; ++q)
		{
			for(int o=0; o<cgi->heroh->flags1[q]->ourImages.size(); ++o)
			{
				if(cgi->heroh->flags1[q]->ourImages[o].groupNumber==6)
				{
					for(int e=0; e<8; ++e)
					{
						Cimage nci;
						nci.bitmap = CSDL_Ext::rotate01(cgi->heroh->flags1[q]->ourImages[o+e].bitmap);
						nci.groupNumber = 10;
						nci.imName = std::string();
						cgi->heroh->flags1[q]->ourImages.push_back(nci);
					}
					o+=8;
				}
				if(cgi->heroh->flags1[q]->ourImages[o].groupNumber==7)
				{
					for(int e=0; e<8; ++e)
					{
						Cimage nci;
						nci.bitmap = CSDL_Ext::rotate01(cgi->heroh->flags1[q]->ourImages[o+e].bitmap);
						nci.groupNumber = 11;
						nci.imName = std::string();
						cgi->heroh->flags1[q]->ourImages.push_back(nci);
					}
					o+=8;
				}
				if(cgi->heroh->flags1[q]->ourImages[o].groupNumber==8)
				{
					for(int e=0; e<8; ++e)
					{
						Cimage nci;
						nci.bitmap = CSDL_Ext::rotate01(cgi->heroh->flags1[q]->ourImages[o+e].bitmap);
						nci.groupNumber = 12;
						nci.imName = std::string();
						cgi->heroh->flags1[q]->ourImages.push_back(nci);
					}
					o+=8;
				}
			}

			for(int ff=80; ff<cgi->heroh->flags1[q]->ourImages.size(); ++ff)
			{
				CSDL_Ext::fullAlphaTransform(cgi->heroh->flags1[q]->ourImages[ff].bitmap);
			}
			cgi->heroh->flags1[q]->alphaTransformed = true;
		}

		cgi->heroh->flags2.push_back(cgi->spriteh->giveDef("ABF02L.DEF")); //red
		cgi->heroh->flags2.push_back(cgi->spriteh->giveDef("ABF02G.DEF")); //blue
		cgi->heroh->flags2.push_back(cgi->spriteh->giveDef("ABF02R.DEF")); //tan
		cgi->heroh->flags2.push_back(cgi->spriteh->giveDef("ABF02D.DEF")); //green
		cgi->heroh->flags2.push_back(cgi->spriteh->giveDef("ABF02B.DEF")); //orange
		cgi->heroh->flags2.push_back(cgi->spriteh->giveDef("ABF02P.DEF")); //purple
		cgi->heroh->flags2.push_back(cgi->spriteh->giveDef("ABF02W.DEF")); //teal
		cgi->heroh->flags2.push_back(cgi->spriteh->giveDef("ABF02K.DEF")); //pink

		for(int q=0; q<8; ++q)
		{
			for(int o=0; o<cgi->heroh->flags2[q]->ourImages.size(); ++o)
			{
				if(cgi->heroh->flags2[q]->ourImages[o].groupNumber==6)
				{
					for(int e=0; e<8; ++e)
					{
						Cimage nci;
						nci.bitmap = CSDL_Ext::rotate01(cgi->heroh->flags2[q]->ourImages[o+e].bitmap);
						nci.groupNumber = 10;
						nci.imName = std::string();
						cgi->heroh->flags2[q]->ourImages.push_back(nci);
					}
					o+=8;
				}
				if(cgi->heroh->flags2[q]->ourImages[o].groupNumber==7)
				{
					for(int e=0; e<8; ++e)
					{
						Cimage nci;
						nci.bitmap = CSDL_Ext::rotate01(cgi->heroh->flags2[q]->ourImages[o+e].bitmap);
						nci.groupNumber = 11;
						nci.imName = std::string();
						cgi->heroh->flags2[q]->ourImages.push_back(nci);
					}
					o+=8;
				}
				if(cgi->heroh->flags2[q]->ourImages[o].groupNumber==8)
				{
					for(int e=0; e<8; ++e)
					{
						Cimage nci;
						nci.bitmap = CSDL_Ext::rotate01(cgi->heroh->flags2[q]->ourImages[o+e].bitmap);
						nci.groupNumber = 12;
						nci.imName = std::string();
						cgi->heroh->flags2[q]->ourImages.push_back(nci);
					}
					o+=8;
				}
			}

			for(int ff=80; ff<cgi->heroh->flags2[q]->ourImages.size(); ++ff)
			{
				CSDL_Ext::fullAlphaTransform(cgi->heroh->flags2[q]->ourImages[ff].bitmap);
			}
			cgi->heroh->flags2[q]->alphaTransformed = true;
		}

		cgi->heroh->flags3.push_back(cgi->spriteh->giveDef("ABF03L.DEF")); //red
		cgi->heroh->flags3.push_back(cgi->spriteh->giveDef("ABF03G.DEF")); //blue
		cgi->heroh->flags3.push_back(cgi->spriteh->giveDef("ABF03R.DEF")); //tan
		cgi->heroh->flags3.push_back(cgi->spriteh->giveDef("ABF03D.DEF")); //green
		cgi->heroh->flags3.push_back(cgi->spriteh->giveDef("ABF03B.DEF")); //orange
		cgi->heroh->flags3.push_back(cgi->spriteh->giveDef("ABF03P.DEF")); //purple
		cgi->heroh->flags3.push_back(cgi->spriteh->giveDef("ABF03W.DEF")); //teal
		cgi->heroh->flags3.push_back(cgi->spriteh->giveDef("ABF03K.DEF")); //pink

		for(int q=0; q<8; ++q)
		{
			for(int o=0; o<cgi->heroh->flags3[q]->ourImages.size(); ++o)
			{
				if(cgi->heroh->flags3[q]->ourImages[o].groupNumber==6)
				{
					for(int e=0; e<8; ++e)
					{
						Cimage nci;
						nci.bitmap = CSDL_Ext::rotate01(cgi->heroh->flags3[q]->ourImages[o+e].bitmap);
						nci.groupNumber = 10;
						nci.imName = std::string();
						cgi->heroh->flags3[q]->ourImages.push_back(nci);
					}
					o+=8;
				}
				if(cgi->heroh->flags3[q]->ourImages[o].groupNumber==7)
				{
					for(int e=0; e<8; ++e)
					{
						Cimage nci;
						nci.bitmap = CSDL_Ext::rotate01(cgi->heroh->flags3[q]->ourImages[o+e].bitmap);
						nci.groupNumber = 11;
						nci.imName = std::string();
						cgi->heroh->flags3[q]->ourImages.push_back(nci);
					}
					o+=8;
				}
				if(cgi->heroh->flags3[q]->ourImages[o].groupNumber==8)
				{
					for(int e=0; e<8; ++e)
					{
						Cimage nci;
						nci.bitmap = CSDL_Ext::rotate01(cgi->heroh->flags3[q]->ourImages[o+e].bitmap);
						nci.groupNumber = 12;
						nci.imName = std::string();
						cgi->heroh->flags3[q]->ourImages.push_back(nci);
					}
					o+=8;
				}
			}

			for(int ff=80; ff<cgi->heroh->flags3[q]->ourImages.size(); ++ff)
			{
				CSDL_Ext::fullAlphaTransform(cgi->heroh->flags3[q]->ourImages[ff].bitmap);
			}
			cgi->heroh->flags3[q]->alphaTransformed = true;
		}

		cgi->heroh->flags4.push_back(cgi->spriteh->giveDef("AF00.DEF")); //red
		cgi->heroh->flags4.push_back(cgi->spriteh->giveDef("AF01.DEF")); //blue
		cgi->heroh->flags4.push_back(cgi->spriteh->giveDef("AF02.DEF")); //tan
		cgi->heroh->flags4.push_back(cgi->spriteh->giveDef("AF03.DEF")); //green
		cgi->heroh->flags4.push_back(cgi->spriteh->giveDef("AF04.DEF")); //orange
		cgi->heroh->flags4.push_back(cgi->spriteh->giveDef("AF05.DEF")); //purple
		cgi->heroh->flags4.push_back(cgi->spriteh->giveDef("AF06.DEF")); //teal
		cgi->heroh->flags4.push_back(cgi->spriteh->giveDef("AF07.DEF")); //pink

		for(int q=0; q<8; ++q)
		{
			for(int o=0; o<cgi->heroh->flags4[q]->ourImages.size(); ++o)
			{
				if(cgi->heroh->flags4[q]->ourImages[o].groupNumber==6)
				{
					for(int e=0; e<8; ++e)
					{
						Cimage nci;
						nci.bitmap = CSDL_Ext::rotate01(cgi->heroh->flags4[q]->ourImages[o+e].bitmap);
						nci.groupNumber = 10;
						nci.imName = std::string();
						cgi->heroh->flags4[q]->ourImages.push_back(nci);
					}
					o+=8;
				}
				if(cgi->heroh->flags4[q]->ourImages[o].groupNumber==7)
				{
					for(int e=0; e<8; ++e)
					{
						Cimage nci;
						nci.bitmap = CSDL_Ext::rotate01(cgi->heroh->flags4[q]->ourImages[o+e].bitmap);
						nci.groupNumber = 11;
						nci.imName = std::string();
						cgi->heroh->flags4[q]->ourImages.push_back(nci);
					}
					o+=8;
				}
				if(cgi->heroh->flags4[q]->ourImages[o].groupNumber==8)
				{
					for(int e=0; e<8; ++e)
					{
						Cimage nci;
						nci.bitmap = CSDL_Ext::rotate01(cgi->heroh->flags4[q]->ourImages[o+e].bitmap);
						nci.groupNumber = 12;
						nci.imName = std::string();
						cgi->heroh->flags4[q]->ourImages.push_back(nci);
					}
					o+=8;
				}
			}

			for(int o=0; o<cgi->heroh->flags4[q]->ourImages.size(); ++o)
			{
				if(cgi->heroh->flags4[q]->ourImages[o].groupNumber==1)
				{
					for(int e=0; e<8; ++e)
					{
						Cimage nci;
						nci.bitmap = CSDL_Ext::rotate01(cgi->heroh->flags4[q]->ourImages[o+e].bitmap);
						nci.groupNumber = 13;
						nci.imName = std::string();
						cgi->heroh->flags4[q]->ourImages.push_back(nci);
					}
					o+=8;
				}
				if(cgi->heroh->flags4[q]->ourImages[o].groupNumber==2)
				{
					for(int e=0; e<8; ++e)
					{
						Cimage nci;
						nci.bitmap = CSDL_Ext::rotate01(cgi->heroh->flags4[q]->ourImages[o+e].bitmap);
						nci.groupNumber = 14;
						nci.imName = std::string();
						cgi->heroh->flags4[q]->ourImages.push_back(nci);
					}
					o+=8;
				}
				if(cgi->heroh->flags4[q]->ourImages[o].groupNumber==3)
				{
					for(int e=0; e<8; ++e)
					{
						Cimage nci;
						nci.bitmap = CSDL_Ext::rotate01(cgi->heroh->flags4[q]->ourImages[o+e].bitmap);
						nci.groupNumber = 15;
						nci.imName = std::string();
						cgi->heroh->flags4[q]->ourImages.push_back(nci);
					}
					o+=8;
				}
			}

			for(int ff=80; ff<cgi->heroh->flags4[q]->ourImages.size(); ++ff)
			{
				CSDL_Ext::fullAlphaTransform(cgi->heroh->flags4[q]->ourImages[ff].bitmap);
			}
			cgi->heroh->flags4[q]->alphaTransformed = true;
		}

		//hero flags initialized

		THC std::cout<<"Initializing colours and flags: "<<tmh.getDif()<<std::endl;
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
		cgi->dobjinfo = new CDefObjInfoHandler;
		cgi->dobjinfo->load();
		cgi->state = new CGameState();
		cgi->state->players = std::map<int, PlayerState>();
		cgi->pathf = new CPathfinder();
		cgi->consoleh->cb = new CCallback(cgi->state,-1);
		cgi->consoleh->runConsole();
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

		initGameState(cgi);
		THC std::cout<<"Initializing GameState: "<<tmh.getDif()<<std::endl;

		/*for(int d=0; d<PLAYER_LIMIT; ++d)
		{
			cgi->playerint.push_back(NULL);
		}*/
		for (int i=0; i<cgi->scenarioOps.playerInfos.size();i++) //initializing interfaces
		{ 

			if(cgi->scenarioOps.playerInfos[i].name=="Computer")
				cgi->playerint.push_back(static_cast<CGameInterface*>(CAIHandler::getNewAI(new CCallback(cgi->state,cgi->scenarioOps.playerInfos[i].color),"EmptyAI.dll")));
			else 
			{
				cgi->state->currentPlayer=cgi->scenarioOps.playerInfos[i].color;
				cgi->playerint.push_back(new CPlayerInterface(cgi->scenarioOps.playerInfos[i].color,i));
				((CPlayerInterface*)(cgi->playerint[i]))->init(new CCallback(cgi->state,cgi->scenarioOps.playerInfos[i].color));
			}
		}

		while(1) //main game loop, one execution per turn
		{
			cgi->consoleh->cb->newTurn();
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
