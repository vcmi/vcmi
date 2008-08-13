#define VCMI_DLL
#include <algorithm>
#include <queue>
#include <fstream>
#include "CGameState.h"
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
#include "lib/NetPacks.h"
#include <boost/foreach.hpp>
#include <boost/thread.hpp>
#include <boost/thread/shared_mutex.hpp>
boost::rand48 ran;

CGObjectInstance * createObject(int id, int subid, int3 pos, int owner)
{
	CGObjectInstance * nobj;
	switch(id)
	{
	case 34: //hero
		{
			CGHeroInstance * nobj;
			nobj = new CGHeroInstance();
			nobj->pos = pos;
			nobj->tempOwner = owner;
			nobj->defInfo = new CGDefInfo();
			nobj->defInfo->id = 34;
			nobj->defInfo->subid = subid;
			nobj->defInfo->printPriority = 0;
			nobj->type = VLC->heroh->heroes[subid];
			for(int i=0;i<6;i++)
			{
				nobj->defInfo->blockMap[i]=255;
				nobj->defInfo->visitMap[i]=0;
			}
			nobj->ID = id;
			nobj->subID = subid;
			nobj->defInfo->handler=NULL;
			nobj->defInfo->blockMap[5] = 253;
			nobj->defInfo->visitMap[5] = 2;
			nobj->artifacts.resize(20);
			nobj->artifWorn[16] = 3;
			nobj->portrait = subid;
			nobj->primSkills.resize(4);
			nobj->primSkills[0] = nobj->type->heroClass->initialAttack;
			nobj->primSkills[1] = nobj->type->heroClass->initialDefence;
			nobj->primSkills[2] = nobj->type->heroClass->initialPower;
			nobj->primSkills[3] = nobj->type->heroClass->initialKnowledge;
			nobj->mana = 10 * nobj->primSkills[3];
			return nobj;
		}
	case 98: //town
		nobj = new CGTownInstance;
		break;
	default: //rest of objects
		nobj = new CGObjectInstance;
		nobj->defInfo = VLC->dobjinfo->gobjs[id][subid];
		break;
	}
	nobj->ID = id;
	nobj->subID = subid;
	if(!nobj->defInfo)
		std::cout <<"No def declaration for " <<id <<" "<<subid<<std::endl;
	nobj->pos = pos;
	//nobj->state = NULL;//new CLuaObjectScript();
	nobj->tempOwner = owner;
	nobj->info = NULL;
	nobj->defInfo->id = id;
	nobj->defInfo->subid = subid;

	//assigning defhandler
	if(nobj->ID==34 || nobj->ID==98)
		return nobj;
	nobj->defInfo = VLC->dobjinfo->gobjs[id][subid];
	//if(!nobj->defInfo->handler)
	//{
	//	nobj->defInfo->handler = CDefHandler::giveDef(nobj->defInfo->name);
	//	nobj->defInfo->width = nobj->defInfo->handler->ourImages[0].bitmap->w/32;
	//	nobj->defInfo->height = nobj->defInfo->handler->ourImages[0].bitmap->h/32;
	//}
	return nobj;
}
CStack * BattleInfo::getStack(int stackID)
{
	for(int g=0; g<stacks.size(); ++g)
	{
		if(stacks[g]->ID == stackID)
			return stacks[g];
	}
	return NULL;
}
CStack * BattleInfo::getStackT(int tileID)
{
	for(int g=0; g<stacks.size(); ++g)
	{
		if(stacks[g]->position == tileID 
			|| (stacks[g]->creature->isDoubleWide() && stacks[g]->attackerOwned && stacks[g]->position-1 == tileID)
			|| (stacks[g]->creature->isDoubleWide() && !stacks[g]->attackerOwned && stacks[g]->position+1 == tileID))
		{
			if(stacks[g]->alive)
			{
				return stacks[g];
			}
		}
	}
	return NULL;
}
void BattleInfo::getAccessibilityMap(bool *accessibility, int stackToOmmit)
{
	memset(accessibility,1,187); //initialize array with trues
	for(int g=0; g<stacks.size(); ++g)
	{
		if(!stacks[g]->alive || stacks[g]->ID==stackToOmmit) //we don't want to lock position of this stack
			continue;

		accessibility[stacks[g]->position] = false;
		if(stacks[g]->creature->isDoubleWide()) //if it's a double hex creature
		{
			if(stacks[g]->attackerOwned)
				accessibility[stacks[g]->position-1] = false;
			else
				accessibility[stacks[g]->position+1] = false;
		}
	}
	//TODO: obstacles
}
void BattleInfo::getAccessibilityMapForTwoHex(bool *accessibility, bool atackerSide, int stackToOmmit) //send pointer to at least 187 allocated bytes
{	
	bool mac[187];
	getAccessibilityMap(mac,stackToOmmit);
	memcpy(accessibility,mac,187);

	for(int b=0; b<187; ++b)
	{
		if( mac[b] && !(atackerSide ? mac[b-1] : mac[b+1]))
		{
			accessibility[b] = false;
		}
	}

	//removing accessibility for side hexes
	for(int v=0; v<187; ++v)
		if(atackerSide ? (v%17)==1 : (v%17)==15)
			accessibility[v] = false;
}
void BattleInfo::makeBFS(int start, bool*accessibility, int *predecessor, int *dists) //both pointers must point to the at least 187-elements int arrays
{
	//inits
	for(int b=0; b<187; ++b)
		predecessor[b] = -1;
	for(int g=0; g<187; ++g)
		dists[g] = 100000000;	
	
	std::queue<int> hexq; //bfs queue
	hexq.push(start);
	dists[hexq.front()] = 0;
	int curNext = -1; //for bfs loop only (helper var)
	while(!hexq.empty()) //bfs loop
	{
		int curHex = hexq.front();
		std::vector<int> neighbours = neighbouringTiles(curHex);
		hexq.pop();
		for(int nr=0; nr<neighbours.size(); nr++)
		{
			curNext = neighbours[nr];
			if(!accessibility[curNext] || (dists[curHex]+1)>=dists[curNext])
				continue;
			hexq.push(curNext);
			dists[curNext] = dists[curHex] + 1;
			predecessor[curNext] = curHex;
		}
	}
};

std::vector<int> BattleInfo::getAccessibility(int stackID)
{
	std::vector<int> ret;
	bool ac[187];
	CStack *s = getStack(stackID);
	if(s->creature->isDoubleWide())
		getAccessibilityMapForTwoHex(ac,s->attackerOwned,stackID);
	else
		getAccessibilityMap(ac,stackID);

	int pr[187], dist[187];
	makeBFS(s->position,ac,pr,dist);
	
	for(int i=0;i<187;i++)
		if(dist[i] <= s->creature->speed)
			ret.push_back(i);

	return ret;
}
signed char BattleInfo::mutualPosition(int hex1, int hex2)
{
	if(hex2 == hex1 - ( (hex1/17)%2 ? 18 : 17 )) //top left
		return 0;
	if(hex2 == hex1 - ( (hex1/17)%2 ? 17 : 16 )) //top right
		return 1;
	if(hex2 == hex1 - 1 && hex1%17 != 0) //left
		return 5;
	if(hex2 == hex1 + 1 && hex1%17 != 16) //right
		return 2;
	if(hex2 == hex1 + ( (hex1/17)%2 ? 16 : 17 )) //bottom left
		return 4;
	if(hex2 == hex1 + ( (hex1/17)%2 ? 17 : 18 )) //bottom right
		return 3;
	return -1;
}
std::vector<int> BattleInfo::neighbouringTiles(int hex)
{
#define CHECK_AND_PUSH(tile) {int hlp = (tile); if(hlp>=0 && hlp<187 && (hlp%17!=16) && hlp%17) ret.push_back(hlp);}
	std::vector<int> ret;
	CHECK_AND_PUSH(hex - ( (hex/17)%2 ? 18 : 17 ));
	CHECK_AND_PUSH(hex - ( (hex/17)%2 ? 17 : 16 ));
	CHECK_AND_PUSH(hex - 1);
	CHECK_AND_PUSH(hex + 1);
	CHECK_AND_PUSH(hex + ( (hex/17)%2 ? 16 : 17 ));
	CHECK_AND_PUSH(hex + ( (hex/17)%2 ? 17 : 18 ));
#undef CHECK_AND_PUSH
	return ret;
}
std::vector<int> BattleInfo::getPath(int start, int dest, bool*accessibility)
{							
	int predecessor[187]; //for getting the Path
	int dist[187]; //calculated distances

	makeBFS(start,accessibility,predecessor,dist);

	//making the Path
	std::vector<int> path;
	int curElem = dest;
	while(curElem != start)
	{
		path.push_back(curElem);
		curElem = predecessor[curElem];
	}
	return path;
}

CStack::CStack(CCreature * C, int A, int O, int I, bool AO)
	:creature(C),amount(A),owner(O), alive(true), position(-1), ID(I), attackerOwned(AO), firstHPleft(C->hitPoints)
{
}
void CGameState::applyNL(IPack * pack)
{
	switch(pack->getType())
	{
	case 101://NewTurn
		{
			NewTurn * n = static_cast<NewTurn*>(pack);
			day = n->day;
			BOOST_FOREACH(NewTurn::Hero h, n->heroes) //give mana/movement point
			{
				static_cast<CGHeroInstance*>(map->objects[h.id])->movement = h.move;
				static_cast<CGHeroInstance*>(map->objects[h.id])->mana = h.mana;
			}
			BOOST_FOREACH(SetResources h, n->res) //give resources
				applyNL(&h);
			BOOST_FOREACH(SetAvailableCreatures h, n->cres) //set available creatures in towns
				applyNL(&h);
			if(n->resetBuilded) //reset amount of structures set in this turn in towns
				BOOST_FOREACH(CGTownInstance* t, map->towns)
					t->builded = 0;
			break;
		}
	case 102: //set resource amount
		{
			SetResource *sr = static_cast<SetResource*>(pack);
			players[sr->player].resources[sr->resid] = sr->val;
			break;
		}
	case 104:
		{
			SetResources *sr = static_cast<SetResources*>(pack);
			for(int i=0;i<sr->res.size();i++)
				players[sr->player].resources[i] = sr->res[i];
			break;
		}
	case 105:
		{
			SetPrimSkill *sr = static_cast<SetPrimSkill*>(pack);
			CGHeroInstance *hero = getHero(sr->id);
			if(sr->which <4)
			{
				if(sr->abs)
					hero->primSkills[sr->which] = sr->val;
				else
					hero->primSkills[sr->which] += sr->val;
			}
			else if(sr->which == 4) //XP
			{
				if(sr->abs)
					hero->exp = sr->val;
				else
					hero->exp += sr->val;
			}
			break;
		}
	case 106:
		{
			SetSecSkill *sr = static_cast<SetSecSkill*>(pack);
			CGHeroInstance *hero = getHero(sr->id);
			if(hero->getSecSkillLevel(sr->which) < 0)
			{
				hero->secSkills.push_back(std::pair<int,int>(sr->which, (sr->abs)? (sr->val) : (sr->val-1)));
			}
			else
			{
				for(unsigned i=0;i<hero->secSkills.size();i++)
				{
					if(hero->secSkills[i].first == sr->which)
					{
						if(sr->abs)
							hero->secSkills[i].second = sr->val;
						else
							hero->secSkills[i].second += sr->val;
					}
				}
			}
			break;
		}
	case 108:
		{
			HeroVisitCastle *vc = static_cast<HeroVisitCastle*>(pack);
			CGHeroInstance *h = getHero(vc->hid);
			CGTownInstance *t = getTown(vc->tid);
			if(vc->start())
			{
				if(vc->garrison())
				{
					t->garrisonHero = h;
					h->visitedTown = t;
					h->inTownGarrison = true;
				}
				else
				{
					t->visitingHero = h;
					h->visitedTown = t;
					h->inTownGarrison = false;
				}
			}
			else
			{
				if(vc->garrison())
				{
					t->garrisonHero = NULL;
					h->visitedTown = NULL;
					h->inTownGarrison = false;
				}
				else
				{
					t->visitingHero = NULL;
					h->visitedTown = NULL;
					h->inTownGarrison = false;
				}
			}
			break;
		}
	case 500:
		{
			RemoveObject *rh = static_cast<RemoveObject*>(pack);
			CGObjectInstance *obj = map->objects[rh->id];
			if(obj->ID==34)
			{
				CGHeroInstance *h = static_cast<CGHeroInstance*>(obj);
				std::vector<CGHeroInstance*>::iterator nitr = std::find(map->heroes.begin(), map->heroes.end(),h);
				map->heroes.erase(nitr);
				int player = h->tempOwner;
				nitr = std::find(players[player].heroes.begin(), players[player].heroes.end(), h);
				players[player].heroes.erase(nitr);
			}
			map->objects[rh->id] = NULL;	

			//unblock tiles
			if(obj->defInfo)
			{
				map->removeBlockVisTiles(obj);
			}


			break;
		}
	case 501://hero try-move
		{
			TryMoveHero * n = static_cast<TryMoveHero*>(pack);
			CGHeroInstance *h = static_cast<CGHeroInstance*>(map->objects[n->id]);
			h->movement = n->movePoints;
			if(n->start!=n->end && n->result)
			{
				map->removeBlockVisTiles(h);
				h->pos = n->end;
				map->addBlockVisTiles(h);
			}
			BOOST_FOREACH(int3 t, n->fowRevealed)
				players[h->getOwner()].fogOfWarMap[t.x][t.y][t.z] = 1;
			break;
		}
	case 502:
		{
			SetGarrisons * n = static_cast<SetGarrisons*>(pack);
			for(std::map<ui32,CCreatureSet>::iterator i = n->garrs.begin(); i!=n->garrs.end(); i++)
				static_cast<CArmedInstance*>(map->objects[i->first])->army = i->second;
			break;
		}
	case 503:
		{
			//SetStrInfo *ssi = static_cast<SetStrInfo*>(pack);
			//static_cast<CGTownInstance*>(map->objects[ssi->tid])->strInfo.creatures = ssi->cres;
			break;
		}
	case 504:
		{
			NewStructures *ns = static_cast<NewStructures*>(pack);
			CGTownInstance*t = static_cast<CGTownInstance*>(map->objects[ns->tid]);
			BOOST_FOREACH(si32 bid,ns->bid)
				t->builtBuildings.insert(bid);
			t->builded = ns->builded;
			break;
		}
	case 506:
		{
			SetAvailableCreatures *sac = static_cast<SetAvailableCreatures*>(pack);
			static_cast<CGTownInstance*>(map->objects[sac->tid])->strInfo.creatures = sac->creatures;
			break;
		}
	case 1001://set object property
		{
			SetObjectProperty *p = static_cast<SetObjectProperty*>(pack);
			ui8 CGObjectInstance::*point;
			switch(p->what)
			{
			case 1:
				point = &CGObjectInstance::tempOwner;
				break;
			case 2:
				point = &CGObjectInstance::blockVisit;
				break;
			}
			map->objects[p->id]->*point = p->val;
			break;
		}
	case 2000:
		{
			HeroLevelUp * bs = static_cast<HeroLevelUp*>(pack);
			getHero(bs->heroid)->level = bs->level;
			break;
		}
	case 3000:
		{
			BattleStart * bs = static_cast<BattleStart*>(pack);
			curB = bs->info;
			break;
		}
	case 3001:
		{
			BattleNextRound *ns = static_cast<BattleNextRound*>(pack);
			curB->round = ns->round;
			break;
		}
	case 3002:
		{
			BattleSetActiveStack *ns = static_cast<BattleSetActiveStack*>(pack);
			curB->activeStack = ns->stack;
			break;
		}
	case 3003:
		{
			BattleResult *br = static_cast<BattleResult*>(pack);

			//TODO: give exp, artifacts to winner, decrease armies (casualties)

			for(unsigned i=0;i<curB->stacks.size();i++)
				delete curB->stacks[i];
			delete curB;
			curB = NULL;
			break;
		}
	case 3004:
		{
			BattleStackMoved *br = static_cast<BattleStackMoved*>(pack);
			curB->getStack(br->stack)->position = br->tile;
			break;
		}
	case 3005:
		{
			BattleStackAttacked *br = static_cast<BattleStackAttacked*>(pack);
			CStack * at = curB->getStack(br->stackAttacked);
			at->amount = br->newAmount;
			at->firstHPleft = br->newHP;
			at->alive = !br->killed();
			break;
		}
	case 3006:
		{
			BattleAttack *br = static_cast<BattleAttack*>(pack);
			applyNL(&br->bsa);
			break;
		}
	}
}
void CGameState::apply(IPack * pack)
{
	mx->lock();
	applyNL(pack);
	mx->unlock();
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
CGHeroInstance *CGameState::getHero(int objid)
{
	if(objid<0 || objid>=map->objects.size())
		return NULL;
	return static_cast<CGHeroInstance *>(map->objects[objid]);
}
CGTownInstance *CGameState::getTown(int objid)
{
	if(objid<0 || objid>=map->objects.size())
		return NULL;
	return static_cast<CGTownInstance *>(map->objects[objid]);
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
		h->portrait = cur->subID = ran.second;
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
	if(!cur->defInfo)
	{
		std::cout<<"Missing def declaration for "<<cur->ID<<" "<<cur->subID<<std::endl;
		return;
	}
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
CGameState::CGameState()
{
	mx = new boost::shared_mutex();
}
CGameState::~CGameState()
{
	delete mx;
}
void CGameState::init(StartInfo * si, Mapa * map, int Seed)
{
	day = 0;
	seed = Seed;
	ran.seed((boost::int32_t)seed);
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
		map->objects[no]->hoverName = VLC->objh->names[map->objects[no]->ID];
	}
	//std::cout<<"\tRandomizing objects: "<<th.getDif()<<std::endl;

	//giving starting hero
	for(int i=0;i<PLAYER_LIMIT;i++)
	{
		if((map->players[i].generateHeroAtMainTown && map->players[i].hasMainTown) ||  (map->players[i].hasMainTown && map->version==RoE))
		{
			int3 hpos = map->players[i].posOfMainTown;
			hpos.x+=1;// hpos.y+=1;
			int j;
			for(j=0; j<scenarioOps->playerInfos.size(); j++)
				if(scenarioOps->playerInfos[j].color == i)
					break;
			if(j == scenarioOps->playerInfos.size())
				continue;
			int h=pickHero(i);
			CGHeroInstance * nnn =  static_cast<CGHeroInstance*>(createObject(34,h,hpos,i));
			nnn->id = map->objects.size();
			hpos = map->players[i].posOfMainTown;hpos.x+=2;
			for(int o=0;o<map->towns.size();o++) //find main town
			{
				if(map->towns[o]->pos == hpos)
				{
					map->towns[o]->visitingHero = nnn;
					nnn->visitedTown = map->towns[o];
					nnn->inTownGarrison = false;
					break;
				}
			}
			//nnn->defInfo->handler = graphics->flags1[0];
			map->heroes.push_back(nnn);
			map->objects.push_back(nnn);
		}
	}
	//std::cout<<"\tGiving starting heroes: "<<th.getDif()<<std::endl;

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
				vhi->army.slots[x-pom2].first = pom;
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
}

bool CGameState::battleShootCreatureStack(int ID, int dest)
{
	return true;
}

int CGameState::battleGetStack(int pos)
{
	for(int g=0; g<curB->stacks.size(); ++g)
	{
		if(curB->stacks[g]->position == pos ||
				( curB->stacks[g]->creature->isDoubleWide() &&
					( (curB->stacks[g]->attackerOwned && curB->stacks[g]->position-1 == pos) ||
						(!curB->stacks[g]->attackerOwned && curB->stacks[g]->position-1 == pos)
					)
				)
			)
			return curB->stacks[g]->ID;
	}
	return -1;
}

int BattleInfo::calculateDmg(const CStack* attacker, const CStack* defender)
{
	int attackDefenseBonus = attacker->creature->attack - defender->creature->defence;
	int damageBase = 0;
	if(attacker->creature->damageMax == attacker->creature->damageMin) //constant damage
	{
		damageBase = attacker->creature->damageMin;
	}
	else
	{
		damageBase = rand()%(attacker->creature->damageMax - attacker->creature->damageMin) + attacker->creature->damageMin + 1;
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

	return (float)damageBase * (float)attacker->amount * dmgBonusMultiplier;
}