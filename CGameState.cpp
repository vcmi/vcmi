#define VCMI_DLL
#include <algorithm>
#include <queue>
#include <fstream>
#include "CGameState.h"
#include "CGameInterface.h"
#include "CPlayerInterface.h"
#include "SDL_Extensions.h"
#include "CBattleInterface.h" //for CBattleHex
#include <boost/random/linear_congruential.hpp>
#include "hch/CDefObjInfoHandler.h"
#include "hch/CArtHandler.h"
#include "hch/CTownHandler.h"
#include "hch/CHeroHandler.h"
#include "hch/CObjectHandler.h"
#include "hch/CCreatureHandler.h"
#include "lib/VCMI_Lib.h"
#include "map.h"
#include "StartInfo.h"
#include "CLua.h"
#include "CCallback.h"
#include "CLuaHandler.h"


boost::rand48 ran;
class CMP_stack
{
public:
	bool operator ()(const CStack* a, const CStack* b)
	{
		return (a->creature->speed)>(b->creature->speed);
	}
} cmpst ;

CStack::CStack(CCreature * C, int A, int O, int I, bool AO)
	:creature(C),amount(A),owner(O), alive(true), position(-1), ID(I), attackerOwned(AO), firstHPleft(C->hitPoints)
{
}
int CGameState::pickHero(int owner)
{
	int h=-1;
	if(map->getHero(h = scenarioOps->getIthPlayersSettings(owner).hero,0)  &&  h>=0) //we haven't used selected hero
		return h;
	int f = scenarioOps->getIthPlayersSettings(owner).castle;
	int i=0;
	do //try to find free hero of our faction
	{
		i++;
		h = scenarioOps->getIthPlayersSettings(owner).castle*HEROES_PER_TYPE*2+(ran()%(HEROES_PER_TYPE*2));//->scenarioOps->playerInfos[pru].hero = VLC->
	} while( map->getHero(h)  &&  i<175);
	if(i>174) //probably no free heroes - there's no point in further search, we'll take first free
	{
		std::cout << "Warning: cannot find free hero - trying to get first available..."<<std::endl;
		for(int j=0; j<HEROES_PER_TYPE * 2 * F_NUMBER; j++)
			if(!map->getHero(j))
				h=j;
	}
	return h;
}
std::pair<int,int> CGameState::pickObject(CGObjectInstance *obj)
{
	switch(obj->ID)
	{
	case 65: //random artifact
		return std::pair<int,int>(5,(ran()%136)+7); //tylko sensowny zakres - na poczatku sa katapulty itp, na koncu specjalne i blanki
	case 66: //random treasure artifact
		return std::pair<int,int>(5,VLC->arth->treasures[ran()%VLC->arth->treasures.size()]->id);
	case 67: //random minor artifact
		return std::pair<int,int>(5,VLC->arth->minors[ran()%VLC->arth->minors.size()]->id);
	case 68: //random major artifact
		return std::pair<int,int>(5,VLC->arth->majors[ran()%VLC->arth->majors.size()]->id);
	case 69: //random relic artifact
		return std::pair<int,int>(5,VLC->arth->relics[ran()%VLC->arth->relics.size()]->id);
	case 70: //random hero
		{
			return std::pair<int,int>(34,pickHero(obj->tempOwner));
		}
	case 71: //random monster
		return std::pair<int,int>(54,ran()%(VLC->creh->creatures.size())); 
	case 72: //random monster lvl1
		return std::pair<int,int>(54,VLC->creh->levelCreatures[1][ran()%VLC->creh->levelCreatures[1].size()]->idNumber); 
	case 73: //random monster lvl2
		return std::pair<int,int>(54,VLC->creh->levelCreatures[2][ran()%VLC->creh->levelCreatures[2].size()]->idNumber);
	case 74: //random monster lvl3
		return std::pair<int,int>(54,VLC->creh->levelCreatures[3][ran()%VLC->creh->levelCreatures[3].size()]->idNumber);
	case 75: //random monster lvl4
		return std::pair<int,int>(54,VLC->creh->levelCreatures[4][ran()%VLC->creh->levelCreatures[4].size()]->idNumber);
	case 76: //random resource
		return std::pair<int,int>(79,ran()%7); //now it's OH3 style, use %8 for mithril 
	case 77: //random town
		{
			int align = ((CGTownInstance*)obj)->alignment,
				f;
			if(align>PLAYER_LIMIT-1)//same as owner / random
			{
				if(obj->tempOwner > PLAYER_LIMIT-1)
					f = -1; //random
				else
					f = scenarioOps->getIthPlayersSettings(obj->tempOwner).castle;
			}
			else
			{
				f = scenarioOps->getIthPlayersSettings(align).castle;
			}
			if(f<0) f = ran()%VLC->townh->towns.size();
			return std::pair<int,int>(98,f); 
		}
	case 162: //random monster lvl5
		return std::pair<int,int>(54,VLC->creh->levelCreatures[5][ran()%VLC->creh->levelCreatures[5].size()]->idNumber);
	case 163: //random monster lvl6
		return std::pair<int,int>(54,VLC->creh->levelCreatures[6][ran()%VLC->creh->levelCreatures[6].size()]->idNumber);
	case 164: //random monster lvl7
		return std::pair<int,int>(54,VLC->creh->levelCreatures[7][ran()%VLC->creh->levelCreatures[7].size()]->idNumber); 
	case 216: //random dwelling
		{
			int faction = ran()%F_NUMBER;
			CCreGen2ObjInfo* info =(CCreGen2ObjInfo*)obj->info;
			if (info->asCastle)
			{
				for(int i=0;i<map->objects.size();i++)
				{
					if(map->objects[i]->ID==77 && dynamic_cast<CGTownInstance*>(map->objects[i])->identifier == info->identifier)
					{
						randomizeObject(map->objects[i]); //we have to randomize the castle first
						faction = map->objects[i]->subID;
						break;
					}
					else if(map->objects[i]->ID==98 && dynamic_cast<CGTownInstance*>(map->objects[i])->identifier == info->identifier)
					{
						faction = map->objects[i]->subID;
						break;
					}
				}
			}
			else
			{
				while((!(info->castles[0]&(1<<faction))))
				{
					if((faction>7) && (info->castles[1]&(1<<(faction-8))))
						break;
					faction = ran()%F_NUMBER;
				}
			}
			int level = ((info->maxLevel-info->minLevel) ? (ran()%(info->maxLevel-info->minLevel)+info->minLevel) : (info->minLevel));
			int cid = VLC->townh->towns[faction].basicCreatures[level];
			for(int i=0;i<VLC->objh->cregens.size();i++)
				if(VLC->objh->cregens[i]==cid)
					return std::pair<int,int>(17,i); 
			std::cout << "Cannot find a dwelling for creature "<<cid <<std::endl;
			return std::pair<int,int>(17,0); 
		}
	case 217:
		{
			int faction = ran()%F_NUMBER;
			CCreGenObjInfo* info =(CCreGenObjInfo*)obj->info;
			if (info->asCastle)
			{
				for(int i=0;i<map->objects.size();i++)
				{
					if(map->objects[i]->ID==77 && dynamic_cast<CGTownInstance*>(map->objects[i])->identifier == info->identifier)
					{
						randomizeObject(map->objects[i]); //we have to randomize the castle first
						faction = map->objects[i]->subID;
						break;
					}
					else if(map->objects[i]->ID==98 && dynamic_cast<CGTownInstance*>(map->objects[i])->identifier == info->identifier)
					{
						faction = map->objects[i]->subID;
						break;
					}
				}
			}
			else
			{
				while((!(info->castles[0]&(1<<faction))))
				{
					if((faction>7) && (info->castles[1]&(1<<(faction-8))))
						break;
					faction = ran()%F_NUMBER;
				}
			}
			int cid = VLC->townh->towns[faction].basicCreatures[obj->subID];
			for(int i=0;i<VLC->objh->cregens.size();i++)
				if(VLC->objh->cregens[i]==cid)
					return std::pair<int,int>(17,i); 
			std::cout << "Cannot find a dwelling for creature "<<cid <<std::endl;
			return std::pair<int,int>(17,0); 
		}
	case 218:
		{
			CCreGen3ObjInfo* info =(CCreGen3ObjInfo*)obj->info;
			int level = ((info->maxLevel-info->minLevel) ? (ran()%(info->maxLevel-info->minLevel)+info->minLevel) : (info->minLevel));
			int cid = VLC->townh->towns[obj->subID].basicCreatures[level];
			for(int i=0;i<VLC->objh->cregens.size();i++)
				if(VLC->objh->cregens[i]==cid)
					return std::pair<int,int>(17,i); 
			std::cout << "Cannot find a dwelling for creature "<<cid <<std::endl;
			return std::pair<int,int>(17,0); 
		}
	}
	return std::pair<int,int>(-1,-1);
}
void CGameState::randomizeObject(CGObjectInstance *cur)
{		
	std::pair<int,int> ran = pickObject(cur);
	if(ran.first<0 || ran.second<0) //this is not a random object, or we couldn't find anything
	{
		if(cur->ID==98) //town - set def
		{
			CGTownInstance *t = dynamic_cast<CGTownInstance*>(cur);
			if(t->hasCapitol())
				t->defInfo = capitols[t->subID];
			else if(t->hasFort())
				t->defInfo = forts[t->subID];
			else
				t->defInfo = villages[t->subID]; 
		}
		return;
	}
	else if(ran.first==34)//special code for hero
	{
		CGHeroInstance *h = dynamic_cast<CGHeroInstance *>(cur);
		if(!h) {std::cout<<"Wrong random hero at "<<cur->pos<<std::endl; return;}
		cur->ID = ran.first;
		cur->subID = ran.second;
		h->type = VLC->heroh->heroes[ran.second];
		map->heroes.push_back(h);
		return; //TODO: maybe we should do something with definfo?
	}
	else if(ran.first==98)//special code for town
	{
		CGTownInstance *t = dynamic_cast<CGTownInstance*>(cur);
		if(!t) {std::cout<<"Wrong random town at "<<cur->pos<<std::endl; return;}
		cur->ID = ran.first;
		cur->subID = ran.second;
		t->town = &VLC->townh->towns[ran.second];
		if(t->hasCapitol())
			t->defInfo = capitols[t->subID];
		else if(t->hasFort())
			t->defInfo = forts[t->subID];
		else
			t->defInfo = villages[t->subID]; 
		map->towns.push_back(t);
		return;
	}
	//we have to replace normal random object
	cur->ID = ran.first;
	cur->subID = ran.second;
	map->defs.insert(cur->defInfo = VLC->dobjinfo->gobjs[ran.first][ran.second]);
	if(!cur->defInfo){std::cout<<"Missing def declaration for "<<cur->ID<<" "<<cur->subID<<std::endl;return;}
}

int CGameState::getDate(int mode) const
{
	int temp;
	switch (mode)
	{
	case 0:
		return day;
		break;
	case 1:
		temp = (day)%7;
		if (temp)
			return temp;
		else return 7;
		break;
	case 2:
		temp = ((day-1)/7)+1;
		if (!(temp%4))
			return 4;
		else 
			return (temp%4);
		break;
	case 3:
		return ((day-1)/28)+1;
		break;
	}
	return 0;
}
void CGameState::init(StartInfo * si, Mapa * map, int Seed)
{
	seed = Seed;
	ran.seed((long)seed);
	scenarioOps = si;
	this->map = map;

	for(int i=0;i<F_NUMBER;i++)
	{
		villages[i] = new CGDefInfo(*VLC->dobjinfo->castles[i]);
		forts[i] = VLC->dobjinfo->castles[i];
		capitols[i] = new CGDefInfo(*VLC->dobjinfo->castles[i]);
	}

	//picking random factions for players
	for(int i=0;i<scenarioOps->playerInfos.size();i++)
	{
		if(scenarioOps->playerInfos[i].castle==-1)
		{
			int f;
			do
			{
				f = ran()%F_NUMBER;
			}while(!(map->players[scenarioOps->playerInfos[i].color].allowedFactions  &  1<<f));
			scenarioOps->playerInfos[i].castle = f;
		}
	}
	//randomizing objects
	for(int no=0; no<map->objects.size(); ++no)
	{
		randomizeObject(map->objects[no]);
		if(map->objects[no]->ID==26)
			map->objects[no]->defInfo->handler=NULL;
	}
	//std::cout<<"\tRandomizing objects: "<<th.getDif()<<std::endl;



		day=0;
	/*********creating players entries in gs****************************************/
	for (int i=0; i<scenarioOps->playerInfos.size();i++)
	{
		std::pair<int,PlayerState> ins(scenarioOps->playerInfos[i].color,PlayerState());
		ins.second.color=ins.first;
		ins.second.serial=i;
		players.insert(ins);
	}
	/******************RESOURCES****************************************************/
	//TODO: zeby komputer dostawal inaczej niz gracz 
	std::vector<int> startres;
	std::ifstream tis("config/startres.txt");
	int k;
	for (int j=0;j<scenarioOps->difficulty;j++)
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
	for (std::map<ui8,PlayerState>::iterator i = players.begin(); i!=players.end(); i++)
	{
		(*i).second.resources.resize(RESOURCE_QUANTITY);
		for (int x=0;x<RESOURCE_QUANTITY;x++)
			(*i).second.resources[x] = startres[x];

	}

	/*************************HEROES************************************************/
	for (int i=0; i<map->heroes.size();i++) //heroes instances
	{
		if (map->heroes[i]->getOwner()<0)
			continue;
		CGHeroInstance * vhi = (map->heroes[i]);
		if(!vhi->type)
			vhi->type = VLC->heroh->heroes[vhi->subID];
		//vhi->subID = vhi->type->ID;
		if (vhi->level<1)
		{
			vhi->exp=40+ran()%50;
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
		}
		vhi->mana = vhi->primSkills[3]*10;
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
		if (!vhi->army.slots.size()) //standard army
		{
			int pom, pom2=0;
			for(int x=0;x<3;x++)
			{
				pom = (VLC->creh->nameToID[vhi->type->refTypeStack[x]]);
				if(pom>=145 && pom<=149) //war machine
				{
					pom2++;
					switch (pom)
					{
					case 145: //catapult
						vhi->artifWorn[16] = 3;
						break;
					default:
						pom-=145;
						vhi->artifWorn[13+pom] = 4+pom;
						break;
					}
					continue;
				}
				vhi->army.slots[x-pom2].first = &(VLC->creh->creatures[pom]);
				if((pom = (vhi->type->highStack[x]-vhi->type->lowStack[x])) > 0)
					vhi->army.slots[x-pom2].second = (ran()%pom)+vhi->type->lowStack[x];
				else 
					vhi->army.slots[x-pom2].second = +vhi->type->lowStack[x];
			}
		}

		players[vhi->getOwner()].heroes.push_back(vhi);

	}
	/*************************FOG**OF**WAR******************************************/		
	for(std::map<ui8, PlayerState>::iterator k=players.begin(); k!=players.end(); ++k)
	{
		k->second.fogOfWarMap.resize(map->width);
		for(int g=0; g<map->width; ++g)
			k->second.fogOfWarMap[g].resize(map->height);

		for(int g=-0; g<map->width; ++g)
			for(int h=0; h<map->height; ++h)
				k->second.fogOfWarMap[g][h].resize(map->twoLevel+1, 0);

		for(int g=0; g<map->width; ++g)
			for(int h=0; h<map->height; ++h)
				for(int v=0; v<map->twoLevel+1; ++v)
					k->second.fogOfWarMap[g][h][v] = 0;
		for(int xd=0; xd<map->width; ++xd) //revealing part of map around heroes
		{
			for(int yd=0; yd<map->height; ++yd)
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
	for (int i=0;i<map->towns.size();i++)
	{
		CGTownInstance * vti =(map->towns[i]);
		if(!vti->town)
			vti->town = &VLC->townh->towns[vti->subID];
		if (vti->name.length()==0) // if town hasn't name we draw it
			vti->name=vti->town->names[ran()%vti->town->names.size()];
		if(vti->builtBuildings.find(-50)!=vti->builtBuildings.end()) //give standard set of buildings
		{
			vti->builtBuildings.erase(-50);
			vti->builtBuildings.insert(10);
			vti->builtBuildings.insert(5);
			vti->builtBuildings.insert(30);
			if(ran()%2)
				vti->builtBuildings.insert(31);
		}
		players[vti->getOwner()].towns.push_back(vti);
	}

	for(std::map<ui8, PlayerState>::iterator k=players.begin(); k!=players.end(); ++k)
	{
		if(k->first==-1 || k->first==255)
			continue;
		for(int xd=0; xd<map->width; ++xd) //revealing part of map around towns
		{
			for(int yd=0; yd<map->height; ++yd)
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

		//init visiting heroes
		for(int l=0; l<k->second.heroes.size();l++)
		{ 
			for(int m=0; m<k->second.towns.size();m++)
			{
				int3 vistile = k->second.towns[m]->pos; vistile.x--; //tile next to the entrance
				if(vistile == k->second.heroes[l]->pos)
				{
					k->second.towns[m]->visitingHero = k->second.heroes[l];
					break;
				}
			}
		}
	}

	///****************************SCRIPTS************************************************/
	//std::map<int, std::map<std::string, CObjectScript*> > * skrypty = &objscr; //alias for easier access
	///****************************C++ OBJECT SCRIPTS************************************************/
	//std::map<int,CCPPObjectScript*> scripts;
	//CScriptCallback * csc = new CScriptCallback();
	//csc->gs = this;
	//handleCPPObjS(&scripts,new CVisitableOPH(csc));
	//handleCPPObjS(&scripts,new CVisitableOPW(csc));
	//handleCPPObjS(&scripts,new CPickable(csc));
	//handleCPPObjS(&scripts,new CMines(csc));
	//handleCPPObjS(&scripts,new CTownScript(csc));
	//handleCPPObjS(&scripts,new CHeroScript(csc));
	//handleCPPObjS(&scripts,new CMonsterS(csc));
	//handleCPPObjS(&scripts,new CCreatureGen(csc));
	////created map

	///****************************LUA OBJECT SCRIPTS************************************************/
	//std::vector<std::string> * lf = CLuaHandler::searchForScripts("scripts/lua/objects"); //files
	//for (int i=0; i<lf->size(); i++)
	//{
	//	try
	//	{
	//		std::vector<std::string> * temp =  CLuaHandler::functionList((*lf)[i]);
	//		CLuaObjectScript * objs = new CLuaObjectScript((*lf)[i]);
	//		CLuaCallback::registerFuncs(objs->is);
	//		//objs
	//		for (int j=0; j<temp->size(); j++)
	//		{
	//			int obid ; //obj ID
	//			int dspos = (*temp)[j].find_first_of('_');
	//			obid = atoi((*temp)[j].substr(dspos+1,(*temp)[j].size()-dspos-1).c_str());
	//			std::string fname = (*temp)[j].substr(0,dspos);
	//			if (skrypty->find(obid)==skrypty->end())
	//				skrypty->insert(std::pair<int, std::map<std::string, CObjectScript*> >(obid,std::map<std::string,CObjectScript*>()));
	//			(*skrypty)[obid].insert(std::pair<std::string, CObjectScript*>(fname,objs));
	//		}
	//		delete temp;
	//	}HANDLE_EXCEPTION
	//}
	///****************************INITIALIZING OBJECT SCRIPTS************************************************/
	//std::string temps("newObject");
	//for (int i=0; i<map->objects.size(); i++)
	//{
	//	//c++ scripts
	//	if (scripts.find(map->objects[i]->ID) != scripts.end())
	//	{
	//		map->objects[i]->state = scripts[map->objects[i]->ID];
	//		map->objects[i]->state->newObject(map->objects[i]);
	//	}
	//	else 
	//	{
	//		map->objects[i]->state = NULL;
	//	}

	//	// lua scripts
	//	if(checkFunc(map->objects[i]->ID,temps))
	//		(*skrypty)[map->objects[i]->ID][temps]->newObject(map->objects[i]);
	//}

	//delete lf;
}
void CGameState::battle(CCreatureSet * army1, CCreatureSet * army2, int3 tile, CArmedInstance *hero1, CArmedInstance *hero2)
{/*
	curB = new BattleInfo();
	std::vector<CStack*> & stacks = (curB->stacks);

	curB->army1=army1;
	curB->army2=army2;
	curB->hero1=dynamic_cast<CGHeroInstance*>(hero1);
	curB->hero2=dynamic_cast<CGHeroInstance*>(hero2);
	curB->side1=(hero1)?(hero1->tempOwner):(-1);
	curB->side2=(hero2)?(hero2->tempOwner):(-1);
	curB->round = -2;
	curB->stackActionPerformed = false;
	for(std::map<int,std::pair<CCreature*,int> >::iterator i = army1->slots.begin(); i!=army1->slots.end(); i++)
	{
		stacks.push_back(new CStack(i->second.first,i->second.second,0, stacks.size(), true));
		stacks[stacks.size()-1]->ID = stacks.size()-1;
	}
	//initialization of positions
	switch(army1->slots.size()) //for attacker
	{
	case 0:
		break;
	case 1:
		stacks[0]->position = 86; //6
		break;
	case 2:
		stacks[0]->position = 35; //3
		stacks[1]->position = 137; //9
		break;
	case 3:
		stacks[0]->position = 35; //3
		stacks[1]->position = 86; //6
		stacks[2]->position = 137; //9
		break;
	case 4:
		stacks[0]->position = 1; //1
		stacks[1]->position = 69; //5
		stacks[2]->position = 103; //7
		stacks[3]->position = 171; //11
		break;
	case 5:
		stacks[0]->position = 1; //1
		stacks[1]->position = 35; //3
		stacks[2]->position = 86; //6
		stacks[3]->position = 137; //9
		stacks[4]->position = 171; //11
		break;
	case 6:
		stacks[0]->position = 1; //1
		stacks[1]->position = 35; //3
		stacks[2]->position = 69; //5
		stacks[3]->position = 103; //7
		stacks[4]->position = 137; //9
		stacks[5]->position = 171; //11
		break;
	case 7:
		stacks[0]->position = 1; //1
		stacks[1]->position = 35; //3
		stacks[2]->position = 69; //5
		stacks[3]->position = 86; //6
		stacks[4]->position = 103; //7
		stacks[5]->position = 137; //9
		stacks[6]->position = 171; //11
		break;
	default: //fault
		break;
	}
	for(std::map<int,std::pair<CCreature*,int> >::iterator i = army2->slots.begin(); i!=army2->slots.end(); i++)
		stacks.push_back(new CStack(i->second.first,i->second.second,1, stacks.size(), false));
	switch(army2->slots.size()) //for defender
	{
	case 0:
		break;
	case 1:
		stacks[0+army1->slots.size()]->position = 100; //6
		break;
	case 2:
		stacks[0+army1->slots.size()]->position = 49; //3
		stacks[1+army1->slots.size()]->position = 151; //9
		break;
	case 3:
		stacks[0+army1->slots.size()]->position = 49; //3
		stacks[1+army1->slots.size()]->position = 100; //6
		stacks[2+army1->slots.size()]->position = 151; //9
		break;
	case 4:
		stacks[0+army1->slots.size()]->position = 15; //1
		stacks[1+army1->slots.size()]->position = 83; //5
		stacks[2+army1->slots.size()]->position = 117; //7
		stacks[3+army1->slots.size()]->position = 185; //11
		break;
	case 5:
		stacks[0+army1->slots.size()]->position = 15; //1
		stacks[1+army1->slots.size()]->position = 49; //3
		stacks[2+army1->slots.size()]->position = 100; //6
		stacks[3+army1->slots.size()]->position = 151; //9
		stacks[4+army1->slots.size()]->position = 185; //11
		break;
	case 6:
		stacks[0+army1->slots.size()]->position = 15; //1
		stacks[1+army1->slots.size()]->position = 49; //3
		stacks[2+army1->slots.size()]->position = 83; //5
		stacks[3+army1->slots.size()]->position = 117; //7
		stacks[4+army1->slots.size()]->position = 151; //9
		stacks[5+army1->slots.size()]->position = 185; //11
		break;
	case 7:
		stacks[0+army1->slots.size()]->position = 15; //1
		stacks[1+army1->slots.size()]->position = 49; //3
		stacks[2+army1->slots.size()]->position = 83; //5
		stacks[3+army1->slots.size()]->position = 100; //6
		stacks[4+army1->slots.size()]->position = 117; //7
		stacks[5+army1->slots.size()]->position = 151; //9
		stacks[6+army1->slots.size()]->position = 185; //11
		break;
	default: //fault
		break;
	}
	for(int g=0; g<stacks.size(); ++g) //shifting positions of two-hex creatures
	{
		if((stacks[g]->position%17)==1 && stacks[g]->creature->isDoubleWide())
		{
			stacks[g]->position += 1;
		}
		else if((stacks[g]->position%17)==15 && stacks[g]->creature->isDoubleWide())
		{
			stacks[g]->position -= 1;
		}
	}
	std::stable_sort(stacks.begin(),stacks.end(),cmpst);

	//for start inform players about battle
	for(std::map<int, PlayerState>::iterator j=players.begin(); j!=players.end(); ++j)//->players.size(); ++j) //for testing
	{
		if (j->first > PLAYER_LIMIT)
			break;
		if(j->second.fogOfWarMap[tile.x][tile.y][tile.z])
		{ //player should be notified
			tribool side = tribool::indeterminate_value;
			if(j->first == curB->side1) //player is attacker
				side = false;
			else if(j->first == curB->side2) //player is defender
				side = true;
			else 
				continue; //no witnesses
			if(VLC->playerint[j->second.serial]->human)
			{
				((CPlayerInterface*)( VLC->playerint[j->second.serial] ))->battleStart(army1, army2, tile, curB->hero1, curB->hero2, side);
			}
			else
			{
				//VLC->playerint[j->second.serial]->battleStart(army1, army2, tile, curB->hero1, curB->hero2, side);
			}
		}
	}

	curB->round++;
	if( (curB->hero1 && curB->hero1->getSecSkillLevel(19)>=0) || ( curB->hero2 && curB->hero2->getSecSkillLevel(19)>=0)  )//someone has tactics
	{
		//TODO: wywolania dla rundy -1, ograniczenie pola ruchu, etc
	}

	curB->round++;

	//SDL_Thread * eventh = SDL_CreateThread(battleEventThread, NULL);

	while(true) //till the end of the battle ;]
	{
		bool battleEnd = false;
		//tell players about next round
		for(int v=0; v<VLC->playerint.size(); ++v)
			VLC->playerint[v]->battleNewRound(curB->round);

		//stack loop
		for(int i=0;i<stacks.size();i++)
		{
			curB->activeStack = i;
			curB->stackActionPerformed = false;
			if(stacks[i]->alive) //indicate posiibility of making action for this unit
			{
				unsigned char owner = (stacks[i]->owner)?(hero2 ? hero2->tempOwner : 255):(hero1->tempOwner);
				unsigned char serialOwner = -1;
				for(int g=0; g<VLC->playerint.size(); ++g)
				{
					if(VLC->playerint[g]->playerID == owner)
					{
						serialOwner = g;
						break;
					}
				}
				if(serialOwner==255) //neutral unit
				{
				}
				else if(VLC->playerint[serialOwner]->human)
				{
					BattleAction ba = ((CPlayerInterface*)VLC->playerint[serialOwner])->activeStack(stacks[i]->ID);
					switch(ba.actionType)
					{
					case 2: //walk
						{
							battleMoveCreatureStack(ba.stackNumber, ba.destinationTile);
						}
					case 3: //defend
						{
							break;
						}
					case 4: //retreat/flee
						{
							for(int v=0; v<VLC->playerint.size(); ++v) //tell about the end of this battle to interfaces
								VLC->playerint[v]->battleEnd(army1, army2, hero1, hero2, std::vector<int>(), 0, false);
							battleEnd = true;
							break;
						}
					case 6: //walk or attack
						{
							battleMoveCreatureStack(ba.stackNumber, ba.destinationTile);
							battleAttackCreatureStack(ba.stackNumber, ba.destinationTile);
							break;
						}
					}
				}
				else
				{
					//VLC->playerint[serialOwner]->activeStack(stacks[i]->ID);
				}
			}
			if(battleEnd)
				break;
			//sprawdzic czy po tej akcji ktoras strona nie wygrala bitwy
		}
		if(battleEnd)
			break;
		curB->round++;
		SDL_Delay(50);
	}

	for(int i=0;i<stacks.size();i++)
		delete stacks[i];
	delete curB;
	curB = NULL;*/
}

bool CGameState::battleMoveCreatureStack(int ID, int dest)
{/*
	//first checks
	if(curB->stackActionPerformed) //because unit cannot be moved more than once
		return false;

	unsigned char owner = -1; //owner moved of unit
	for(int g=0; g<curB->stacks.size(); ++g)
	{
		if(curB->stacks[g]->ID == ID)
		{
			owner = curB->stacks[g]->owner;
			break;
		}
	}

	bool stackAtEnd = false; //true if there is a stack at the end of the path (we should attack it)
	int numberOfStackAtEnd = -1;
	for(int g=0; g<curB->stacks.size(); ++g)
	{
		if(curB->stacks[g]->position == dest 
			|| (curB->stacks[g]->creature->isDoubleWide() && curB->stacks[g]->attackerOwned && curB->stacks[g]->position-1 == dest)
			|| (curB->stacks[g]->creature->isDoubleWide() && !curB->stacks[g]->attackerOwned && curB->stacks[g]->position+1 == dest))
		{
			if(curB->stacks[g]->alive)
			{
				stackAtEnd = true;
				numberOfStackAtEnd = g;
				break;
			}
		}
	}

	//selecting moved stack
	CStack * curStack = NULL;
	for(int y=0; y<curB->stacks.size(); ++y)
	{
		if(curB->stacks[y]->ID == ID)
		{
			curStack = curB->stacks[y];
			break;
		}
	}
	if(!curStack)
		return false;
	//initing necessary tables
	bool accessibility[187]; //accesibility of hexes
	for(int k=0; k<187; k++)
		accessibility[k] = true;
	for(int g=0; g<curB->stacks.size(); ++g)
	{
		if(curB->stacks[g]->ID != ID && curB->stacks[g]->alive) //we don't want to lock enemy's positions and this units' position
		{
			accessibility[curB->stacks[g]->position] = false;
			if(curB->stacks[g]->creature->isDoubleWide()) //if it's a double hex creature
			{
				if(curB->stacks[g]->attackerOwned)
					accessibility[curB->stacks[g]->position-1] = false;
				else
					accessibility[curB->stacks[g]->position+1] = false;
			}
		}
	}
	accessibility[dest] = true;
	if(curStack->creature->isDoubleWide()) //locking positions unreachable by two-hex creatures
	{
		bool mac[187];
		for(int b=0; b<187; ++b)
		{
			//
			//	&& (  ? (curStack->attackerOwned ? accessibility[curNext-1] : accessibility[curNext+1]) : true )
			mac[b] = accessibility[b];
			if( accessibility[b] && !(curStack->attackerOwned ? accessibility[b-1] : accessibility[b+1]))
			{
				mac[b] = false;
			}
		}
		mac[curStack->attackerOwned ? curStack->position+1 : curStack->position-1]=true;
		for(int v=0; v<187; ++v)
			accessibility[v] = mac[v];
		//removing accessibility for side hexes
		for(int v=0; v<187; ++v)
			if(curStack->attackerOwned ? (v%17)==1 : (v%17)==15)
				accessibility[v] = false;
	}
	if(!accessibility[dest])
		return false;
	int predecessor[187]; //for getting the Path
	for(int b=0; b<187; ++b)
		predecessor[b] = -1;
	//bfsing
	int dists[187]; //calculated distances
	std::queue<int> hexq; //bfs queue
	hexq.push(curStack->position);
	for(int g=0; g<187; ++g)
		dists[g] = 100000000;
	dists[hexq.front()] = 0;
	int curNext = -1; //for bfs loop only (helper var)
	while(!hexq.empty()) //bfs loop
	{
		int curHex = hexq.front();
		hexq.pop();
		curNext = curHex - ( (curHex/17)%2 ? 18 : 17 );
		if((curNext > 0) && (accessibility[curNext] || curNext==dest) && (dists[curHex] + 1 < dists[curNext]) && (curNext)%17!=0 && (curNext)%17!=16) //top left
		{
			hexq.push(curNext);
			dists[curNext] = dists[curHex] + 1;
			predecessor[curNext] = curHex;
		}
		curNext = curHex - ( (curHex/17)%2 ? 17 : 16 );
		if((curNext > 0) && (accessibility[curNext] || curNext==dest)  && (dists[curHex] + 1 < dists[curNext]) && (curNext)%17!=0 && (curNext)%17!=16) //top right
		{
			hexq.push(curNext);
			dists[curNext] = dists[curHex] + 1;
			predecessor[curNext] = curHex;
		}
		curNext = curHex - 1;
		if((curNext > 0) && (accessibility[curNext] || curNext==dest)  && (dists[curHex] + 1 < dists[curNext]) && (curNext)%17!=0 && (curNext)%17!=16) //left
		{
			hexq.push(curNext);
			dists[curNext] = dists[curHex] + 1;
			predecessor[curNext] = curHex;
		}
		curNext = curHex + 1;
		if((curNext < 187) && (accessibility[curNext] || curNext==dest)  && (dists[curHex] + 1 < dists[curNext]) && (curNext)%17!=0 && (curNext)%17!=16) //right
		{
			hexq.push(curNext);
			dists[curNext] = dists[curHex] + 1;
			predecessor[curNext] = curHex;
		}
		curNext = curHex + ( (curHex/17)%2 ? 16 : 17 );
		if((curNext < 187) && (accessibility[curNext] || curNext==dest)  && (dists[curHex] + 1 < dists[curNext]) && (curNext)%17!=0 && (curNext)%17!=16) //bottom left
		{
			hexq.push(curNext);
			dists[curNext] = dists[curHex] + 1;
			predecessor[curNext] = curHex;
		}
		curNext = curHex + ( (curHex/17)%2 ? 17 : 18 );
		if((curNext < 187) && (accessibility[curNext] || curNext==dest)  && (dists[curHex] + 1 < dists[curNext]) && (curNext)%17!=0 && (curNext)%17!=16) //bottom right
		{
			hexq.push(curNext);
			dists[curNext] = dists[curHex] + 1;
			predecessor[curNext] = curHex;
		}
	}
	//following the Path
	if(dists[dest] > curStack->creature->speed && !(stackAtEnd && dists[dest] == curStack->creature->speed+1)) //we can attack a stack if we can go to adjacent hex
		return false;
	std::vector<int> path;
	int curElem = dest;
	while(curElem!=curStack->position)
	{
		path.push_back(curElem);
		curElem = predecessor[curElem];
	}
	for(int v=path.size()-1; v>=0; --v)
	{
		if(v!=0 || !stackAtEnd) //it's not the last step
		{
			LOCPLINT->battleStackMoved(ID, path[v], v==path.size()-1, v==0 || (stackAtEnd && v==1) );
			curStack->position = path[v];
		}
		else //if it's last step and we should attack unit at the end
		{
			LOCPLINT->battleStackAttacking(ID, path[v]);
			//counting dealt damage
			int numberOfCres = curStack->amount; //number of attacking creatures
			int attackDefenseBonus = curStack->creature->attack - curB->stacks[numberOfStackAtEnd]->creature->defence;
			int damageBase = 0;
			if(curStack->creature->damageMax == curStack->creature->damageMin) //constant damage
			{
				damageBase = curStack->creature->damageMin;
			}
			else
			{
				damageBase = ran()%(curStack->creature->damageMax - curStack->creature->damageMin) + curStack->creature->damageMin + 1;
			}

			float dmgBonusMultiplier = 1.0;
			if(attackDefenseBonus < 0) //decreasing dmg
			{
				if(0.02f * (-attackDefenseBonus) > 0.3f)
				{
					dmgBonusMultiplier += -0.3f;
				}
				else
				{
					dmgBonusMultiplier += 0.02f * attackDefenseBonus;
				}
			}
			else //increasing dmg
			{
				if(0.05f * attackDefenseBonus > 4.0f)
				{
					dmgBonusMultiplier += 4.0f;
				}
				else
				{
					dmgBonusMultiplier += 0.05f * attackDefenseBonus;
				}
			}

			int finalDmg = (float)damageBase * (float)curStack->amount * dmgBonusMultiplier;

			//applying damages
			int cresKilled = finalDmg / curB->stacks[numberOfStackAtEnd]->creature->hitPoints;
			int damageFirst = finalDmg % curB->stacks[numberOfStackAtEnd]->creature->hitPoints;

			if( curB->stacks[numberOfStackAtEnd]->firstHPleft <= damageFirst )
			{
				curB->stacks[numberOfStackAtEnd]->amount -= 1;
				curB->stacks[numberOfStackAtEnd]->firstHPleft += curB->stacks[numberOfStackAtEnd]->creature->hitPoints - damageFirst;
			}
			else
			{
				curB->stacks[numberOfStackAtEnd]->firstHPleft -= damageFirst;
			}

			int cresInstackBefore = curB->stacks[numberOfStackAtEnd]->amount; 
			curB->stacks[numberOfStackAtEnd]->amount -= cresKilled;
			if(curB->stacks[numberOfStackAtEnd]->amount<=0) //stack killed
			{
				curB->stacks[numberOfStackAtEnd]->amount = 0;
				LOCPLINT->battleStackKilled(curB->stacks[numberOfStackAtEnd]->ID, finalDmg, std::min(cresKilled, cresInstackBefore) , ID);
				curB->stacks[numberOfStackAtEnd]->alive = false;
			}
			else
			{
				LOCPLINT->battleStackIsAttacked(curB->stacks[numberOfStackAtEnd]->ID, finalDmg, std::min(cresKilled, cresInstackBefore), ID);
			}

			//damage applied
		}
	}
	curB->stackActionPerformed = true;
	LOCPLINT->actionFinished(BattleAction());*/
	return true;
}

bool CGameState::battleAttackCreatureStack(int ID, int dest)
{
	int attackedCreaure = -1; //-1 - there is no attacked creature
	for(int b=0; b<curB->stacks.size(); ++b) //TODO: make upgrades for two-hex cres.
	{
		if(curB->stacks[b]->position == dest)
		{
			attackedCreaure = curB->stacks[b]->ID;
			break;
		}
	}
	if(attackedCreaure == -1)
		return false;
	//LOCPLINT->cb->
	return true;
}

std::vector<int> CGameState::battleGetRange(int ID)
{/*
	int initialPlace=-1; //position of unit
	int radius=-1; //range of unit
	unsigned char owner = -1; //owner of unit
	//selecting stack
	CStack * curStack = NULL;
	for(int y=0; y<curB->stacks.size(); ++y)
	{
		if(curB->stacks[y]->ID == ID)
		{
			curStack = curB->stacks[y];
			break;
		}
	}

	for(int g=0; g<curB->stacks.size(); ++g)
	{
		if(curB->stacks[g]->ID == ID)
		{
			initialPlace = curB->stacks[g]->position;
			radius = curB->stacks[g]->creature->speed;
			owner = curB->stacks[g]->owner;
			break;
		}
	}

	bool accessibility[187]; //accesibility of hexes
	for(int k=0; k<187; k++)
		accessibility[k] = true;
	for(int g=0; g<curB->stacks.size(); ++g)
	{
		if(curB->stacks[g]->ID != ID && curB->stacks[g]->alive) //we don't want to lock current unit's position
		{
			accessibility[curB->stacks[g]->position] = false;
			if(curB->stacks[g]->creature->isDoubleWide()) //if it's a double hex creature
			{
				if(curB->stacks[g]->attackerOwned)
					accessibility[curB->stacks[g]->position-1] = false;
				else
					accessibility[curB->stacks[g]->position+1] = false;
			}
		}
	}

	if(curStack->creature->isDoubleWide()) //locking positions unreachable by two-hex creatures
	{
		bool mac[187];
		for(int b=0; b<187; ++b)
		{
			//
			//	&& (  ? (curStack->attackerOwned ? accessibility[curNext-1] : accessibility[curNext+1]) : true )
			mac[b] = accessibility[b];
			if( accessibility[b] && !(curStack->attackerOwned ? accessibility[b-1] : accessibility[b+1]))
			{
				mac[b] = false;
			}
		}
		mac[curStack->attackerOwned ? curStack->position+1 : curStack->position-1]=true;
		for(int v=0; v<187; ++v)
			accessibility[v] = mac[v];
		//removing accessibility for side hexes
		for(int v=0; v<187; ++v)
			if(curStack->attackerOwned ? (v%17)==1 : (v%17)==15)
				accessibility[v] = false;
	}

	int dists[187]; //calculated distances
	std::queue<int> hexq; //bfs queue
	hexq.push(initialPlace);
	for(int g=0; g<187; ++g)
		dists[g] = 100000000;
	dists[initialPlace] = 0;
	int curNext = -1; //for bfs loop only (helper var)
	while(!hexq.empty()) //bfs loop
	{
		int curHex = hexq.front();
		hexq.pop();
		curNext = curHex - ( (curHex/17)%2 ? 18 : 17 );
		if((curNext > 0) && accessibility[curNext]  && (dists[curHex] + 1 < dists[curNext]) && (curNext)%17!=0 && (curNext)%17!=16) //top left
		{
			hexq.push(curNext);
			dists[curNext] = dists[curHex] + 1;
		}
		curNext = curHex - ( (curHex/17)%2 ? 17 : 16 );
		if((curNext > 0) && accessibility[curNext]  && (dists[curHex] + 1 < dists[curNext]) && (curNext)%17!=0 && (curNext)%17!=16) //top right
		{
			hexq.push(curNext);
			dists[curNext] = dists[curHex] + 1;
		}
		curNext = curHex - 1;
		if((curNext > 0) && accessibility[curNext]  && (dists[curHex] + 1 < dists[curNext]) && (curNext)%17!=0 && (curNext)%17!=16) //left
		{
			hexq.push(curNext);
			dists[curNext] = dists[curHex] + 1;
		}
		curNext = curHex + 1;
		if((curNext < 187) && accessibility[curNext]  && (dists[curHex] + 1 < dists[curNext]) && (curNext)%17!=0 && (curNext)%17!=16) //right
		{
			hexq.push(curNext);
			dists[curNext] = dists[curHex] + 1;
		}
		curNext = curHex + ( (curHex/17)%2 ? 16 : 17 );
		if((curNext < 187) && accessibility[curNext]  && (dists[curHex] + 1 < dists[curNext]) && (curNext)%17!=0 && (curNext)%17!=16) //bottom left
		{
			hexq.push(curNext);
			dists[curNext] = dists[curHex] + 1;
		}
		curNext = curHex + ( (curHex/17)%2 ? 17 : 18 );
		if((curNext < 187) && accessibility[curNext]  && (dists[curHex] + 1 < dists[curNext]) && (curNext)%17!=0 && (curNext)%17!=16) //bottom right
		{
			hexq.push(curNext);
			dists[curNext] = dists[curHex] + 1;
		}
	}

	std::vector<int> ret;

	for(int i=0; i<187; ++i)
	{
		if(dists[i]<=radius)
		{
			ret.push_back(i);
		}
	}

	std::vector<int> additionals;

	//adding enemies' positions
	for(int c=0; c<curB->stacks.size(); ++c)
	{
		if(curB->stacks[c]->alive && curB->stacks[c]->owner != owner)
		{
			for(int g=0; g<ret.size(); ++g)
			{
				if(CBattleHex::mutualPosition(ret[g], curB->stacks[c]->position) != -1)
				{
					additionals.push_back(curB->stacks[c]->position);
				}
				if(curB->stacks[c]->creature->isDoubleWide() && curB->stacks[c]->attackerOwned && CBattleHex::mutualPosition(ret[g], curB->stacks[c]->position-1) != -1)
				{
					additionals.push_back(curB->stacks[c]->position-1);
				}
				if(curB->stacks[c]->creature->isDoubleWide() && !curB->stacks[c]->attackerOwned && CBattleHex::mutualPosition(ret[g], curB->stacks[c]->position+1) != -1)
				{
					additionals.push_back(curB->stacks[c]->position+1);
				}
			}
		}
	}
	for(int g=0; g<additionals.size(); ++g)
	{
		ret.push_back(additionals[g]);
	}

	std::sort(ret.begin(), ret.end());
	std::vector<int>::iterator nend = std::unique(ret.begin(), ret.end());

	std::vector<int> ret2;

	for(std::vector<int>::iterator it = ret.begin(); it != nend; ++it)
	{
		ret2.push_back(*it);
	}

	return ret2;*/
	return std::vector<int>();
}

