#define VCMI_DLL
#include <algorithm>
#include <queue>
#include <fstream>
#include "CGameState.h"
#include <boost/random/linear_congruential.hpp>
#include "hch/CDefObjInfoHandler.h"
#include "hch/CArtHandler.h"
#include "hch/CTownHandler.h"
#include "hch/CSpellHandler.h"
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


#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif


CGObjectInstance * createObject(int id, int subid, int3 pos, int owner)
{
	CGObjectInstance * nobj;
	switch(id)
	{
	case 34: //hero
		{
			CGHeroInstance * nobj = new CGHeroInstance();
			nobj->pos = pos;
			nobj->tempOwner = owner;
			nobj->subID = subid;
			//nobj->initHero(ran);
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
		tlog3 <<"No def declaration for " <<id <<" "<<subid<<std::endl;
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
			if(stacks[g]->alive())
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
		if(!stacks[g]->alive() || stacks[g]->ID==stackToOmmit) //we don't want to lock position of this stack
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
bool BattleInfo::isStackBlocked(int ID)
{
	CStack *our = getStack(ID);
	for(int i=0; i<stacks.size();i++)
	{
		if( !stacks[i]->alive()
			|| stacks[i]->owner==our->owner
		  )
			continue; //we ommit dead and allied stacks
		if( mutualPosition(stacks[i]->position,our->position) >= 0 )
			return true;
	}
	return false;
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

CStack::CStack(CCreature * C, int A, int O, int I, bool AO, int S)
	:creature(C),amount(A), baseAmount(A), owner(O), position(-1), ID(I), attackerOwned(AO), firstHPleft(C->hitPoints), slot(S), counterAttacks(1)
{
	abilities = C->abilities;
	state.insert(ALIVE);
}

CGHeroInstance* CGameState::HeroesPool::pickHeroFor(bool native, int player, const CTown *town, int notThatOne)
{
	if(player<0 || player>=PLAYER_LIMIT)
	{
		tlog1 << "Cannot pick hero for " << town->name << ". Wrong owner!\n";
		return NULL;
	}
	std::vector<CGHeroInstance *> pool;
	int sum=0, r;
	if(native)
	{
		for(std::map<ui32,CGHeroInstance *>::iterator i=heroesPool.begin(); i!=heroesPool.end(); i++)
		{
			if(pavailable[i->first] & 1<<player
			  && i->second->type->heroType/2 == town->typeID
			  && i->second->subID != notThatOne
			  )
			{
				pool.push_back(i->second);
			}
		}
		if(!pool.size())
			return pickHeroFor(false,player,town,notThatOne);
		else
			return pool[rand()%pool.size()];
	}
	else
	{
		for(std::map<ui32,CGHeroInstance *>::iterator i=heroesPool.begin(); i!=heroesPool.end(); i++)
		{
			if(pavailable[i->first] & 1<<player
				&& i->second->subID != notThatOne
			  )
			{
				pool.push_back(i->second);
				sum += i->second->type->heroClass->selectionProbability[town->typeID];
			}
		}
		if(!pool.size())
		{
			tlog1 << "There are no heroes available for player " << player<<"!\n";
			return NULL;
		}
		r = rand()%sum;
		for(int i=0; i<pool.size(); i++)
		{
			r -= pool[i]->type->heroClass->selectionProbability[town->typeID];
			if(r<0)
				return pool[i];
		}
		return pool[pool.size()-1];
	}
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
			if(hero->getSecSkillLevel(sr->which) == 0)
			{
				hero->secSkills.push_back(std::pair<int,int>(sr->which, sr->val));
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
	case 109:
		{
			ChangeSpells *rh = static_cast<ChangeSpells*>(pack);
			CGHeroInstance *hero = getHero(rh->hid);
			if(rh->learn)
				BOOST_FOREACH(ui32 sid, rh->spells)
					hero->spells.insert(sid);
			else
				BOOST_FOREACH(ui32 sid, rh->spells)
					hero->spells.erase(sid);
			break;
		}
	case 110:
		{
			SetMana *rh = static_cast<SetMana*>(pack);
			CGHeroInstance *hero = getHero(rh->hid);
			hero->mana = rh->val;
			break;
		}
	case 111:
		{
			SetMovePoints *rh = static_cast<SetMovePoints*>(pack);
			CGHeroInstance *hero = getHero(rh->hid);
			hero->movement = rh->val;
			break;
		}
	case 112:
		{
			FoWChange *rh = static_cast<FoWChange*>(pack);
			BOOST_FOREACH(int3 t, rh->tiles)
				players[rh->player].fogOfWarMap[t.x][t.y][t.z] = rh->mode;
			break;
		}
	case 113:
		{
			SetAvailableHeroes *rh = static_cast<SetAvailableHeroes*>(pack);
			players[rh->player].availableHeroes.clear();
			players[rh->player].availableHeroes.push_back(hpool.heroesPool[rh->hid1]);
			players[rh->player].availableHeroes.push_back(hpool.heroesPool[rh->hid2]);
			if(rh->flags & 1)
			{
				hpool.heroesPool[rh->hid1]->army.slots.clear();
				hpool.heroesPool[rh->hid1]->army.slots[0] = std::pair<ui32,si32>(VLC->creh->nameToID[hpool.heroesPool[rh->hid1]->type->refTypeStack[0]],1);
			}
			if(rh->flags & 2)
			{
				hpool.heroesPool[rh->hid2]->army.slots.clear();
				hpool.heroesPool[rh->hid2]->army.slots[0] = std::pair<ui32,si32>(VLC->creh->nameToID[hpool.heroesPool[rh->hid2]->type->refTypeStack[0]],1);
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
				if(h->visitedTown)
				{
					if(h->inTownGarrison)
						h->visitedTown->garrisonHero = NULL;
					else
						h->visitedTown->visitingHero = NULL;
					h->visitedTown = NULL;
				}
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
			{
				CArmedInstance *ai = static_cast<CArmedInstance*>(map->objects[i->first]);
				ai->army = i->second;
				if(ai->ID==98 && (static_cast<CGTownInstance*>(ai))->garrisonHero) //if there is a hero in garrison then we must update also his army
					const_cast<CGHeroInstance*>((static_cast<CGTownInstance*>(ai))->garrisonHero)->army = i->second;
				else if(ai->ID==34)
				{
					CGHeroInstance *h =  static_cast<CGHeroInstance*>(ai);
					if(h->visitedTown && h->inTownGarrison)
						h->visitedTown->army = i->second;
				}
			}
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
	case 508:
		{
			SetHeroesInTown *sac = static_cast<SetHeroesInTown*>(pack);
			CGTownInstance *t = getTown(sac->tid);
			CGHeroInstance *v  = getHero(sac->visiting), *g = getHero(sac->garrison);
			t->visitingHero = v;
			t->garrisonHero = g;
			if(v)
			{
				v->visitedTown = t;
				v->inTownGarrison = false;
				map->addBlockVisTiles(v);
			}
			if(g)
			{
				g->visitedTown = t;
				g->inTownGarrison = true;
				map->removeBlockVisTiles(g);
			}
			break;
		}
	case 509:
		{
			SetHeroArtifacts *sha = static_cast<SetHeroArtifacts*>(pack);
			CGHeroInstance *h = getHero(sha->hid);
			h->artifacts = sha->artifacts;
			h->artifWorn = sha->artifWorn;
			break;
		}
	case 515:
		{
			HeroRecruited *sha = static_cast<HeroRecruited*>(pack);
			CGHeroInstance *h = hpool.heroesPool[sha->hid];
			CGTownInstance *t = getTown(sha->tid);
			h->setOwner(sha->player);
			h->pos = sha->tile;
			h->movement =  h->maxMovePoints(true);

			hpool.heroesPool.erase(sha->hid);
			if(h->id < 0)
			{
				h->id = map->objects.size();
				map->objects.push_back(h);
			}
			else
				map->objects[h->id] = h;
			map->heroes.push_back(h);
			players[h->tempOwner].heroes.push_back(h);
			map->addBlockVisTiles(h);
			t->visitingHero = h;
			h->visitedTown = t;
			h->inTownGarrison = false;
			break;
		}
	case 1001://set object property
		{
			SetObjectProperty *p = static_cast<SetObjectProperty*>(pack);
			if(p->what == 3) //set creatures amount
			{
				tlog5 << "Setting creatures amount in " << p->id << std::endl;
				static_cast<CCreatureObjInfo*>(map->objects[p->id]->info)->number = p->val;
				break;
			}
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
			for(int i=0; i<curB->stacks.size();i++)
				curB->stacks[i]->counterAttacks = 1;
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
			if(br->killed())
				at->state -= ALIVE;
			break;
		}
	case 3006:
		{
			BattleAttack *br = static_cast<BattleAttack*>(pack);
			CStack *attacker = curB->getStack(br->stackAttacking);
			if(br->counter())
				attacker->counterAttacks--;
			if(br->shot())
				attacker->shots--;
			applyNL(&br->bsa);
			break;
		}
	case 3009:
		{
			SpellCasted *sc = static_cast<SpellCasted*>(pack);
			CGHeroInstance *h = (sc->side) ? getHero(curB->hero2) : getHero(curB->hero1);
			if(h)
				h->mana -= VLC->spellh->spells[sc->id].costs[sc->skill];
			//TODO: counter
			break;
		}
	}
}
void CGameState::apply(IPack * pack)
{
	while(!mx->try_lock())
		boost::this_thread::sleep(boost::posix_time::milliseconds(50)); //give other threads time to finish
	applyNL(pack);
	mx->unlock();
}
int CGameState::pickHero(int owner)
{
	int h=-1;
	if(!map->getHero(h = scenarioOps->getIthPlayersSettings(owner).hero,0)  &&  h>=0) //we haven't used selected hero
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
		tlog3 << "Warning: cannot find free hero - trying to get first available..."<<std::endl;
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
		{
			int r;
			do 
			{
				r = ran()%197;
			} while (vstd::contains(VLC->creh->notUsedMonsters,r));
			return std::pair<int,int>(54,r); 
		}
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
			tlog3 << "Cannot find a dwelling for creature "<<cid <<std::endl;
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
			tlog3 << "Cannot find a dwelling for creature "<<cid <<std::endl;
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
			tlog3 << "Cannot find a dwelling for creature "<<cid <<std::endl;
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
		if(!h) {tlog2<<"Wrong random hero at "<<cur->pos<<std::endl; return;}
		cur->ID = ran.first;
		h->portrait = cur->subID = ran.second;
		h->type = VLC->heroh->heroes[ran.second];
		map->heroes.push_back(h);
		return; //TODO: maybe we should do something with definfo?
	}
	else if(ran.first==98)//special code for town
	{
		CGTownInstance *t = dynamic_cast<CGTownInstance*>(cur);
		if(!t) {tlog2<<"Wrong random town at "<<cur->pos<<std::endl; return;}
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
		tlog1<<"*BIG* WARNING: Missing def declaration for "<<cur->ID<<" "<<cur->subID<<std::endl;
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
		{
			map->objects[no]->defInfo->handler=NULL;
			map->removeBlockVisTiles(map->objects[no]);
			map->objects[no]->defInfo->blockMap[5] = 255;
			map->addBlockVisTiles(map->objects[no]);
		}
		map->objects[no]->hoverName = VLC->objh->names[map->objects[no]->ID];
	}
	//std::cout<<"\tRandomizing objects: "<<th.getDif()<<std::endl;

	/*********give starting hero****************************************/
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
			nnn->initHero();
			map->heroes.push_back(nnn);
			map->objects.push_back(nnn);
			map->addBlockVisTiles(nnn);
		}
	}

	/*********creating players entries in gs****************************************/
	for (int i=0; i<scenarioOps->playerInfos.size();i++)
	{
		std::pair<int,PlayerState> ins(scenarioOps->playerInfos[i].color,PlayerState());
		ins.second.color=ins.first;
		ins.second.serial=i;
		players.insert(ins);
	}
	/******************RESOURCES****************************************************/
	//TODO: computer player should receive other amount of resource than computer (depending on difficulty)
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
	tis.clear();
	for (std::map<ui8,PlayerState>::iterator i = players.begin(); i!=players.end(); i++)
	{
		(*i).second.resources.resize(RESOURCE_QUANTITY);
		for (int x=0;x<RESOURCE_QUANTITY;x++)
			(*i).second.resources[x] = startres[x];
	}

	tis.open("config/resources.txt");
	tis >> k;
	int pom;
	for(int i=0;i<k;i++)
	{
		tis >> pom;
		resVals.push_back(pom);
	}

	/*************************HEROES************************************************/
	std::set<int> hids;
	for(int i=0; i<map->allowedHeroes.size(); i++) //add to hids all allowed heroes
		if(map->allowedHeroes[i])
			hids.insert(i);
	for (int i=0; i<map->heroes.size();i++) //heroes instances
	{
		if (map->heroes[i]->getOwner()<0)
		{
			tlog2 << "Warning - hero with uninitialized owner!\n";
			continue;
		}
		CGHeroInstance * vhi = (map->heroes[i]);
		vhi->initHero();
		players[vhi->getOwner()].heroes.push_back(vhi);
		hids.erase(vhi->subID);
	}
	for(int i=0; i<map->predefinedHeroes.size(); i++)
	{
		if(!vstd::contains(hids,map->predefinedHeroes[i]->subID))
			continue;
		map->predefinedHeroes[i]->initHero();
		hpool.heroesPool[map->predefinedHeroes[i]->subID] = map->predefinedHeroes[i];
		hpool.pavailable[map->predefinedHeroes[i]->subID] = 0xff;
		hids.erase(map->predefinedHeroes[i]->subID);
	}
	BOOST_FOREACH(int hid, hids) //all not used allowed heroes go into the pool
	{
		CGHeroInstance * vhi = new CGHeroInstance();
		vhi->initHero(hid);
		hpool.heroesPool[hid] = vhi;
		hpool.pavailable[hid] = 0xff;
	}
	for(int i=0; i<map->disposedHeroes.size(); i++)
	{
		hpool.pavailable[map->disposedHeroes[i].ID] = map->disposedHeroes[i].players;
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

		//starting bonus
		if(si->playerInfos[k->second.serial].bonus==brandom)
			si->playerInfos[k->second.serial].bonus = ran()%3;
		switch(si->playerInfos[k->second.serial].bonus)
		{
		case bgold:
			k->second.resources[6] += 500 + (ran()%6)*100;
			break;
		case bresource:
			{
				int res = VLC->townh->towns[si->playerInfos[k->second.serial].castle].primaryRes;
				if(res == 127)
				{
					k->second.resources[0] += 5 + ran()%6;
					k->second.resources[2] += 5 + ran()%6;
				}
				else
				{
					k->second.resources[res] += 3 + ran()%4;
				}
				break;
			}
		case bartifact:
			{
				if(!k->second.heroes.size())
				{
					tlog5 << "Cannot give starting artifact - no heroes!" << std::endl;
					break;
				}
				CArtifact *toGive;
				do 
				{
					toGive = VLC->arth->treasures[ran() % VLC->arth->treasures.size()];
				} while (!map->allowedArtifact[toGive->id]);
				CGHeroInstance *hero = k->second.heroes[0];
				std::vector<ui16>::iterator slot = vstd::findFirstNot(hero->artifWorn,toGive->possibleSlots);
				if(slot!=toGive->possibleSlots.end())
					hero->artifWorn[*slot] = toGive->id;
				else
					hero->artifacts.push_back(toGive->id);
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

		//init buildings
		if(vti->builtBuildings.find(-50)!=vti->builtBuildings.end()) //give standard set of buildings
		{
			vti->builtBuildings.erase(-50);
			vti->builtBuildings.insert(10);
			vti->builtBuildings.insert(5);
			vti->builtBuildings.insert(30);
			if(ran()%2)
				vti->builtBuildings.insert(31);
		}

		//init spells
		vti->spells.resize(SPELL_LEVELS);
		CSpell *s;
		for(int z=0; z<vti->obligatorySpells.size();z++)
		{
			s = &VLC->spellh->spells[vti->obligatorySpells[z]];
			vti->spells[s->level-1].push_back(s->id);
			vti->possibleSpells -= s->id;
		}
		while(vti->possibleSpells.size())
		{
			ui32 total=0, sel=-1;
			for(int ps=0;ps<vti->possibleSpells.size();ps++)
				total += VLC->spellh->spells[vti->possibleSpells[ps]].probabilities[vti->subID];
			int r = (total)? ran()%total : -1;
			for(int ps=0; ps<vti->possibleSpells.size();ps++)
			{
				r -= VLC->spellh->spells[vti->possibleSpells[ps]].probabilities[vti->subID];
				if(r<0)
				{
					sel = ps;
					break;
				}
			}
			if(sel<0)
				sel=0;

			CSpell *s = &VLC->spellh->spells[vti->possibleSpells[sel]];
			vti->spells[s->level-1].push_back(s->id);
			vti->possibleSpells -= s->id;
		}

		//init garrisons
		for (std::map<si32,std::pair<ui32,si32> >::iterator j=vti->army.slots.begin(); j!=vti->army.slots.end();j++)
		{
			if(j->second.first > 196 && j->second.first < 211)
			{
				if(j->second.first%2)
					j->second.first = vti->town->basicCreatures[ (j->second.first-197) / 2 ];
				else
					j->second.first = vti->town->upgradedCreatures[ (j->second.first-197) / 2 ];
			}
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

		//init visiting and garrisoned heroes
		for(int l=0; l<k->second.heroes.size();l++)
		{ 
			for(int m=0; m<k->second.towns.size();m++)
			{
				int3 vistile = k->second.towns[m]->pos; vistile.x--; //tile next to the entrance
				if(vistile == k->second.heroes[l]->pos || k->second.heroes[l]->pos==k->second.towns[m]->pos)
				{
					k->second.towns[m]->visitingHero = k->second.heroes[l];
					k->second.heroes[l]->visitedTown = k->second.towns[m];
					k->second.heroes[l]->inTownGarrison = false;
					if(k->second.heroes[l]->pos==k->second.towns[m]->pos)
						k->second.heroes[l]->pos.x -= 1;
					break;
				}
				//else if(k->second.heroes[l]->pos == k->second.towns[m]->pos)
				//{
				//	k->second.towns[m]->garrisonHero = k->second.heroes[l];
				//	k->second.towns[m]->army = k->second.heroes[l]->army;
				//	k->second.heroes[l]->visitedTown = k->second.towns[m];
				//	k->second.heroes[l]->inTownGarrison = true;
				//	k->second.heroes[l]->pos.x -= 1;
				//	goto mainplheloop;
				//}
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
	if(!curB)
		return -1;
	for(int g=0; g<curB->stacks.size(); ++g)
	{
		if(curB->stacks[g]->position == pos ||
				( curB->stacks[g]->creature->isDoubleWide() &&
					( (curB->stacks[g]->attackerOwned && curB->stacks[g]->position-1 == pos) ||
						(!curB->stacks[g]->attackerOwned && curB->stacks[g]->position+1 == pos)
					)
				)
			)
			return curB->stacks[g]->ID;
	}
	return -1;
}

UpgradeInfo CGameState::getUpgradeInfo(CArmedInstance *obj, int stackPos)
{
	UpgradeInfo ret;
	CCreature *base = &VLC->creh->creatures[obj->army.slots[stackPos].first];
	if((obj->ID == 98)  ||  ((obj->ID == 34) && static_cast<const CGHeroInstance*>(obj)->visitedTown))
	{
		CGTownInstance * t;
		if(obj->ID == 98)
			t = static_cast<CGTownInstance *>(const_cast<CArmedInstance *>(obj));
		else
			t = static_cast<const CGHeroInstance*>(obj)->visitedTown;
		for(std::set<si32>::iterator i=t->builtBuildings.begin();  i!=t->builtBuildings.end(); i++)
		{
			if( (*i) >= 37   &&   (*i) < 44 ) //upgraded creature dwelling
			{
				int nid = t->town->upgradedCreatures[(*i)-37]; //upgrade offered by that building
				if(base->upgrades.find(nid) != base->upgrades.end()) //possible upgrade
				{
					ret.newID.push_back(nid);
					ret.cost.push_back(std::set<std::pair<int,int> >());
					for(int j=0;j<RESOURCE_QUANTITY;j++)
					{
						int dif = VLC->creh->creatures[nid].cost[j] - base->cost[j];
						if(dif)
							ret.cost[ret.cost.size()-1].insert(std::make_pair(j,dif));
					}
				}
			}
		}//end for
	}
	//TODO: check if hero ability makes some upgrades possible

	if(ret.newID.size())
		ret.oldID = base->idNumber;

	return ret;
}

float CGameState::getMarketEfficiency( int player, int mode/*=0*/ )
{
	boost::shared_lock<boost::shared_mutex> lock(*mx);
	if(mode) return -1; //todo - support other modes
	int mcount = 0;
	for(int i=0;i<players[player].towns.size();i++)
		if(vstd::contains(players[player].towns[i]->builtBuildings,14))
			mcount++;
	float ret = std::min(((float)mcount+1.0f)/20.0f,0.5f);
	return ret;
}

std::set<int3> CGameState::tilesToReveal(int3 pos, int radious, int player)
{		
	std::set<int3> ret;
	int xbeg = pos.x - radious - 2;
	if(xbeg < 0)
		xbeg = 0;
	int xend = pos.x + radious + 2;
	if(xend >= map->width)
		xend = map->width;
	int ybeg = pos.y - radious - 2;
	if(ybeg < 0)
		ybeg = 0;
	int yend = pos.y + radious + 2;
	if(yend >= map->height)
		yend = map->height;
	for(int xd=xbeg; xd<xend; ++xd) //revealing part of map around heroes
	{
		for(int yd=ybeg; yd<yend; ++yd)
		{
			int deltaX = (pos.x-xd)*(pos.x-xd);
			int deltaY = (pos.y-yd)*(pos.y-yd);
			if(deltaX+deltaY<radious*radious)
			{
				if(player<0 || players[player].fogOfWarMap[xd][yd][pos.z]==0)
				{
					ret.insert(int3(xd,yd,pos.z));
				}
			}
		}
	}
	return ret;
}

int BattleInfo::calculateDmg(const CStack* attacker, const CStack* defender, const CGHeroInstance * attackerHero, const CGHeroInstance * defendingHero, bool shooting)
{
	int attackDefenseBonus = attacker->creature->attack + (attackerHero ? attackerHero->getPrimSkillLevel(0) : 0) - (defender->creature->defence + (defendingHero ? defendingHero->getPrimSkillLevel(1) : 0));
	int damageBase = 0;
	if(attacker->creature->damageMax == attacker->creature->damageMin) //constant damage
	{
		damageBase = attacker->creature->damageMin;
	}
	else
	{
		damageBase = rand()%(attacker->creature->damageMax - attacker->creature->damageMin) + attacker->creature->damageMin + 1;
	}

	float dmgBonusMultiplier = 1.0f;
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
	//handling secondary abilities
	if(attackerHero)
	{
		if(shooting)
		{
			switch(attackerHero->getSecSkillLevel(1)) //archery
			{
			case 1: //basic
				dmgBonusMultiplier *= 1.1f;
				break;
			case 2: //advanced
				dmgBonusMultiplier *= 1.25f;
				break;
			case 3: //expert
				dmgBonusMultiplier *= 1.5f;
				break;
			}
		}
		else
		{
			switch(attackerHero->getSecSkillLevel(22)) //offence
			{
			case 1: //basic
				dmgBonusMultiplier *= 1.1f;
				break;
			case 2: //advanced
				dmgBonusMultiplier *= 1.2f;
				break;
			case 3: //expert
				dmgBonusMultiplier *= 1.3f;
				break;
			}
		}
	}
	if(defendingHero)
	{
		switch(defendingHero->getSecSkillLevel(23)) //armourer
		{
		case 1: //basic
			dmgBonusMultiplier *= 0.95f;
			break;
		case 2: //advanced
			dmgBonusMultiplier *= 0.9f;
			break;
		case 3: //expert
			dmgBonusMultiplier *= 0.85f;
			break;
		}
	}

	return (float)damageBase * (float)attacker->amount * dmgBonusMultiplier;
}

void BattleInfo::calculateCasualties( std::set<std::pair<ui32,si32> > *casualties )
{
	for(int i=0; i<stacks.size();i++)//setting casualties
	{
		if(!stacks[i]->alive())
		{
			casualties[!stacks[i]->attackerOwned].insert(std::pair<ui32,si32>(stacks[i]->creature->idNumber,stacks[i]->baseAmount));
		}
		else if(stacks[i]->amount != stacks[i]->baseAmount)
		{
			casualties[!stacks[i]->attackerOwned].insert(std::pair<ui32,si32>(stacks[i]->creature->idNumber,stacks[i]->baseAmount - stacks[i]->amount));
		}
	}
}