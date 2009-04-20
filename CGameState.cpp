#define VCMI_DLL
#include <algorithm>
#include <queue>
#include <fstream>
#include "CGameState.h"
#include <boost/random/linear_congruential.hpp>
#include "hch/CDefObjInfoHandler.h"
#include "hch/CArtHandler.h"
#include "hch/CBuildingHandler.h"
#include "hch/CGeneralTextHandler.h"
#include "hch/CTownHandler.h"
#include "hch/CSpellHandler.h"
#include "hch/CHeroHandler.h"
#include "hch/CObjectHandler.h"
#include "hch/CCreatureHandler.h"
#include "lib/VCMI_Lib.h"
#include "lib/Connection.h"
#include "map.h"
#include "StartInfo.h"
#include "lib/NetPacks.h"
#include <boost/foreach.hpp>
#include <boost/thread.hpp>
#include <boost/thread/shared_mutex.hpp>

#include "lib/RegisterTypes.cpp"
boost::rand48 ran;

#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif

/*
 * CGameState.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

void foofoofoo()
{
	//never called function to force instantation of templates
	int *ccc = NULL;
	registerTypes((CISer<CConnection>&)*ccc);
	registerTypes((COSer<CConnection>&)*ccc);
	registerTypes((CSaveFile&)*ccc);
	registerTypes((CLoadFile&)*ccc);
	registerTypes((CTypeList&)*ccc);
}


class CBaseForGSApply
{
public:
	virtual void applyOnGS(CGameState *gs, void *pack) const =0; 
};
template <typename T> class CApplyOnGS : public CBaseForGSApply
{
public:
	void applyOnGS(CGameState *gs, void *pack) const
	{
		T *ptr = static_cast<T*>(pack);
		while(!gs->mx->try_lock())
			boost::this_thread::sleep(boost::posix_time::milliseconds(50)); //give other threads time to finish
		ptr->applyGs(gs);
		gs->mx->unlock();
	}
};

class CGSApplier
{
public:
	std::map<ui16,CBaseForGSApply*> apps; 

	CGSApplier()
	{
		registerTypes2(*this);
	}
	template<typename T> void registerType(const T * t=NULL)
	{
		ui16 ID = typeList.registerType(t);
		apps[ID] = new CApplyOnGS<T>;
	}

} *applierGs = NULL;

std::string DLL_EXPORT toString(MetaString &ms)
{
	std::string ret;
	for(size_t i=0;i<ms.message.size();++i)
	{
		if(ms.message[i]>0)
		{
			ret += ms.strings[ms.message[i]-1];
		}
		else
		{
			std::vector<std::string> *vec;
			int type = ms.texts[-ms.message[i]-1].first,
				ser = ms.texts[-ms.message[i]-1].second;
			if(type == 5)
			{
				ret += VLC->arth->artifacts[ser].Name();
				continue;
			}
			else if(type == 7)
			{
				ret += VLC->creh->creatures[ser].namePl;
				continue;
			}
			else if(type == 9)
			{
				ret += VLC->generaltexth->mines[ser].first;
				continue;
			}
			else if(type == 10)
			{
				ret += VLC->generaltexth->mines[ser].second;
				continue;
			}
			else if(type == MetaString::SPELL_NAME)
			{
				ret += VLC->spellh->spells[ser].name;
				continue;
			}
			else
			{
				switch(type)
				{
				case 1:
					vec = &VLC->generaltexth->allTexts;
					break;
				case 2:
					vec = &VLC->generaltexth->xtrainfo;
					break;
				case 3:
					vec = &VLC->generaltexth->names;
					break;
				case 4:
					vec = &VLC->generaltexth->restypes;
					break;
				case 6:
					vec = &VLC->generaltexth->arraytxt;
					break;
				case 8:
					vec = &VLC->generaltexth->creGens;
					break;
				case 11:
					vec = &VLC->generaltexth->advobtxt;
					break;
				case 12:
					vec = &VLC->generaltexth->artifEvents;
					break;
				}
				ret += (*vec)[ser];
			}
		}
	}
	for(size_t i=0; i < ms.replacements.size(); ++i)
	{
		ret.replace(ret.find("%s"),2,ms.replacements[i]);
	}
	return ret;
}

CGObjectInstance * createObject(int id, int subid, int3 pos, int owner)
{
	CGObjectInstance * nobj;
	switch(id)
	{
	case HEROI_TYPE: //hero
		{
			CGHeroInstance * nobj = new CGHeroInstance();
			nobj->pos = pos;
			nobj->tempOwner = owner;
			nobj->subID = subid;
			//nobj->initHero(ran);
			return nobj;
		}
	case TOWNI_TYPE: //town
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
	if(nobj->ID==HEROI_TYPE || nobj->ID==TOWNI_TYPE)
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
	memset(accessibility,1,BFIELD_SIZE); //initialize array with trues
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
	//obstacles
	for(int b=0; b<obstacles.size(); ++b)
	{
		std::vector<int> blocked = VLC->heroh->obstacles[obstacles[b].ID].getBlocked(obstacles[b].pos);
		for(int c=0; c<blocked.size(); ++c)
		{
			if(blocked[c] >=0 && blocked[c] < BFIELD_SIZE)
				accessibility[blocked[c]] = false;
		}
	}
}
void BattleInfo::getAccessibilityMapForTwoHex(bool *accessibility, bool atackerSide, int stackToOmmit, bool addOccupiable) //send pointer to at least 187 allocated bytes
{	
	bool mac[BFIELD_SIZE];
	getAccessibilityMap(mac,stackToOmmit);
	memcpy(accessibility,mac,BFIELD_SIZE);
}
void BattleInfo::makeBFS(int start, bool*accessibility, int *predecessor, int *dists) //both pointers must point to the at least 187-elements int arrays
{
	//inits
	for(int b=0; b<BFIELD_SIZE; ++b)
		predecessor[b] = -1;
	for(int g=0; g<BFIELD_SIZE; ++g)
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

std::vector<int> BattleInfo::getAccessibility(int stackID, bool addOccupiable)
{
	std::vector<int> ret;
	bool ac[BFIELD_SIZE];
	CStack *s = getStack(stackID);
	if(s->creature->isDoubleWide())
		getAccessibilityMapForTwoHex(ac,s->attackerOwned,stackID,addOccupiable);
	else
		getAccessibilityMap(ac,stackID);

	int pr[BFIELD_SIZE], dist[BFIELD_SIZE];
	makeBFS(s->position,ac,pr,dist);

	if(s->creature->isDoubleWide())
	{
		if(!addOccupiable)
		{
			std::vector<int> rem;
			for(int b=0; b<BFIELD_SIZE; ++b)
			{
				if( ac[b] && !(s->attackerOwned ? ac[b-1] : ac[b+1]))
				{
					rem.push_back(b);
				}
			}

			for(int g=0; g<rem.size(); ++g)
			{
				ac[rem[g]] = false;
			}

			//removing accessibility for side hexes
			for(int v=0; v<BFIELD_SIZE; ++v)
				if(s->attackerOwned ? (v%BFIELD_WIDTH)==1 : (v%BFIELD_WIDTH)==(BFIELD_WIDTH - 2))
					ac[v] = false;
		}
		else
		{
			std::vector<int> rem;
			for(int b=0; b<BFIELD_SIZE; ++b)
			{
				if( ac[b] && (!ac[b-1] || dist[b-1] > s->Speed() ) && ( !ac[b+1] || dist[b+1] > s->Speed() ) && b%BFIELD_WIDTH != 0 && b%BFIELD_WIDTH != (BFIELD_WIDTH-1))
				{
					rem.push_back(b);
				}
			}

			for(int g=0; g<rem.size(); ++g)
			{
				ac[rem[g]] = false;
			}
		}
	}
	
	for(int i=0;i<BFIELD_SIZE;i++)
		if(dist[i] <= s->Speed() && ac[i])
		{
			ret.push_back(i);
		}

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
			continue; //we omit dead and allied stacks
		if(stacks[i]->creature->isDoubleWide())
		{
			if( mutualPosition(stacks[i]->position, our->position) >= 0  
			  || mutualPosition(stacks[i]->position + (stacks[i]->attackerOwned ? -1 : 1), our->position) >= 0)
				return true;
		}
		else
		{
			if( mutualPosition(stacks[i]->position, our->position) >= 0 )
				return true;
		}
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
#define CHECK_AND_PUSH(tile) {int hlp = (tile); if(hlp>=0 && hlp<BFIELD_SIZE && (hlp%BFIELD_WIDTH!=16) && hlp%BFIELD_WIDTH) ret.push_back(hlp);}
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
std::pair< std::vector<int>, int > BattleInfo::getPath(int start, int dest, bool*accessibility, bool flyingCreature)
{							
	int predecessor[BFIELD_SIZE]; //for getting the Path
	int dist[BFIELD_SIZE]; //calculated distances

	if(flyingCreature)
	{
		bool acc[BFIELD_SIZE]; //full accessibility table
		for(int b=0; b<BFIELD_SIZE; ++b) //initialization of acc
		{
			acc[b] = true;
		}

		makeBFS(start, acc, predecessor, dist);
	}
	else
	{
		makeBFS(start, accessibility, predecessor, dist);
	}

	//making the Path
	std::vector<int> path;
	int curElem = dest;
	while(curElem != start)
	{
		path.push_back(curElem);
		curElem = predecessor[curElem];
	}

	return std::make_pair(path, dist[dest]);
}

CStack::CStack(CCreature * C, int A, int O, int I, bool AO, int S)
	:ID(I), creature(C), amount(A), baseAmount(A), firstHPleft(C->hitPoints), owner(O), slot(S), attackerOwned(AO), position(-1),   
	 counterAttacks(1), shots(C->shots), state(), effects()
{
	speed = creature->speed;
	abilities = C->abilities;
	state.insert(ALIVE);
}

ui32 CStack::Speed() const
{
	int premy=0;
	const StackEffect *effect = 0;
	//haste effect check
	effect = getEffect(53);
	if(effect)
		premy += VLC->spellh->spells[effect->id].powers[effect->level];
	//slow effect check
	effect = getEffect(54);
	if(effect)
		premy -= (creature->speed * VLC->spellh->spells[effect->id].powers[effect->level])/100;
	//prayer effect check
	effect = getEffect(48);
	if(effect)
		premy += VLC->spellh->spells[effect->id].powers[effect->level];
	//bind effect check
	effect = getEffect(72);
	if(effect) 
	{
		premy = creature->speed; //don't use '- creature->speed' - speed is unsigned!
		premy = -premy;
	}
	return speed + premy;
}

const CStack::StackEffect * CStack::getEffect(ui16 id) const
{
	for (int i=0; i< effects.size(); i++)
		if(effects[i].id == id)
			return &effects[i];
	return NULL;
}

si8 CStack::Morale() const
{
	si8 ret = morale;
	if(getEffect(49)) //mirth
	{
		ret += VLC->spellh->spells[49].powers[getEffect(49)->level];
	}
	if(getEffect(50)) //sorrow
	{
		ret -= VLC->spellh->spells[50].powers[getEffect(50)->level];
	}
	if(ret > 3) ret = 3;
	if(ret < -3) ret = -3;
	return ret;
}

si8 CStack::Luck() const
{
	si8 ret = luck;
	if(getEffect(51)) //fortune
	{
		ret += VLC->spellh->spells[51].powers[getEffect(51)->level];
	}
	if(getEffect(52)) //misfortune
	{
		ret -= VLC->spellh->spells[52].powers[getEffect(52)->level];
	}
	if(ret > 3) ret = 3;
	if(ret < -3) ret = -3;
	return ret;
}

bool CStack::willMove()
{
	return !vstd::contains(state,DEFENDING)
		&& !vstd::contains(state,MOVED)
		&& alive()
		&& !vstd::contains(abilities,NOT_ACTIVE); //eg. Ammo Cart
}

CGHeroInstance* CGameState::HeroesPool::pickHeroFor(bool native, int player, const CTown *town, int notThatOne)
{
	if(player<0 || player>=PLAYER_LIMIT)
	{
		tlog1 << "Cannot pick hero for " << town->Name() << ". Wrong owner!\n";
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

//void CGameState::apply(CPack * pack)
//{
//	while(!mx->try_lock())
//		boost::this_thread::sleep(boost::posix_time::milliseconds(50)); //give other threads time to finish
//	//applyNL(pack);
//	mx->unlock();
//}
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
	if(objid<0 || objid>=map->objects.size() || map->objects[objid]->ID!=34)
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
			return std::pair<int,int>(HEROI_TYPE,pickHero(obj->tempOwner));
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
			return std::pair<int,int>(TOWNI_TYPE,f); 
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
					else if(map->objects[i]->ID==TOWNI_TYPE && dynamic_cast<CGTownInstance*>(map->objects[i])->identifier == info->identifier)
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
					else if(map->objects[i]->ID==TOWNI_TYPE && dynamic_cast<CGTownInstance*>(map->objects[i])->identifier == info->identifier)
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
		if(cur->ID==TOWNI_TYPE) //town - set def
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
	else if(ran.first==HEROI_TYPE)//special code for hero
	{
		CGHeroInstance *h = dynamic_cast<CGHeroInstance *>(cur);
		if(!h) {tlog2<<"Wrong random hero at "<<cur->pos<<std::endl; return;}
		cur->ID = ran.first;
		h->portrait = cur->subID = ran.second;
		h->type = VLC->heroh->heroes[ran.second];
		map->heroes.push_back(h);
		return; //TODO: maybe we should do something with definfo?
	}
	else if(ran.first==TOWNI_TYPE)//special code for town
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
	map->removeBlockVisTiles(cur); //recalculate blockvis tiles - picked object might have different than random placeholder
	map->defy.push_back(cur->defInfo = VLC->dobjinfo->gobjs[ran.first][ran.second]);
	if(!cur->defInfo)
	{
		tlog1<<"*BIG* WARNING: Missing def declaration for "<<cur->ID<<" "<<cur->subID<<std::endl;
		return;
	}

	map->addBlockVisTiles(cur);
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
	map = NULL;
	curB = NULL;
	scenarioOps = NULL;
	applierGs = new CGSApplier;
}
CGameState::~CGameState()
{
	delete mx;
	delete map;
	delete curB;
	delete scenarioOps;
	delete applierGs;
}
void CGameState::init(StartInfo * si, Mapa * map, int Seed)
{
	day = 0;
	seed = Seed;
	ran.seed((boost::int32_t)seed);
	scenarioOps = si;
	this->map = map;
	loadTownDInfos();
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
		}
		map->objects[no]->hoverName = VLC->generaltexth->names[map->objects[no]->ID];
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
			CGHeroInstance * nnn =  static_cast<CGHeroInstance*>(createObject(HEROI_TYPE,h,hpos,i));
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
		ins.second.human = scenarioOps->playerInfos[i].human;
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
	for (int i=0; i<map->heroes.size();i++) //heroes instances initialization
	{
		if (map->heroes[i]->getOwner()<0)
		{
			tlog2 << "Warning - hero with uninitialized owner!\n";
			continue;
		}
		CGHeroInstance * vhi = (map->heroes[i]);
		vhi->initHero();
		players.find(vhi->getOwner())->second.heroes.push_back(vhi);
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

		BOOST_FOREACH(CGObjectInstance *obj, map->objects)
		{
			if(obj->tempOwner != k->first) continue; //not a flagged object

			int3 objCenter = obj->getSightCenter();
			int radious = obj->getSightRadious();

			for (int xd = std::max<int>(objCenter.x - radious , 0); xd <= std::min<int>(objCenter.x + radious, map->width - 1); xd++)
			{
				for (int yd = std::max<int>(objCenter.y - radious, 0); yd <= std::min<int>(objCenter.y + radious, map->height - 1); yd++)
				{
					double distance = objCenter.dist2d(int3(xd,yd,objCenter.z)) - 0.5;
					if(distance <= radious)
						k->second.fogOfWarMap[xd][yd][objCenter.z] = 1;
				}
			}
		}

		//for(int xd=0; xd<map->width; ++xd) //revealing part of map around heroes
		//{
		//	for(int yd=0; yd<map->height; ++yd)
		//	{
		//		for(int ch=0; ch<k->second.heroes.size(); ++ch)
		//		{
		//			int deltaX = (k->second.heroes[ch]->getPosition(false).x-xd)*(k->second.heroes[ch]->getPosition(false).x-xd);
		//			int deltaY = (k->second.heroes[ch]->getPosition(false).y-yd)*(k->second.heroes[ch]->getPosition(false).y-yd);
		//			if(deltaX+deltaY<k->second.heroes[ch]->getSightDistance()*k->second.heroes[ch]->getSightDistance())
		//				k->second.fogOfWarMap[xd][yd][k->second.heroes[ch]->getPosition(false).z] = 1;
		//		}
		//	}
		//}

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
			vti->name = vti->town->Names()[ran()%vti->town->Names().size()];

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
		if(vti->getOwner() != 255)
			getPlayer(vti->getOwner())->towns.push_back(vti);
	}

	for(std::map<ui8, PlayerState>::iterator k=players.begin(); k!=players.end(); ++k)
	{
		if(k->first==-1 || k->first==255)
			continue;
	//	for(int xd=0; xd<map->width; ++xd) //revealing part of map around towns
	//	{
	//		for(int yd=0; yd<map->height; ++yd)
	//		{
	//			for(int ch=0; ch<k->second.towns.size(); ++ch)
	//			{
	//				int deltaX = (k->second.towns[ch]->pos.x-xd)*(k->second.towns[ch]->pos.x-xd);
	//				int deltaY = (k->second.towns[ch]->pos.y-yd)*(k->second.towns[ch]->pos.y-yd);
	//				if(deltaX+deltaY<k->second.towns[ch]->getSightDistance()*k->second.towns[ch]->getSightDistance())
	//					k->second.fogOfWarMap[xd][yd][k->second.towns[ch]->pos.z] = 1;
	//			}
	//		}
	//	}

		//init visiting and garrisoned heroes
		for(unsigned int l=0; l<k->second.heroes.size();l++)
		{ 
			for(unsigned int m=0; m<k->second.towns.size();m++)
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
			}
		}
	}

	for(int i=0; i<map->defy.size(); i++)
	{
		map->defy[i]->serial = i;
	}

	for(int i=0; i<map->objects.size(); i++)
	{
		map->objects[i]->initObj();
		if(map->objects[i]->ID == 62) //prison also needs to initialize hero
			static_cast<CGHeroInstance*>(map->objects[i])->initHero();
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
		if((curB->stacks[g]->position == pos 
			  || (curB->stacks[g]->creature->isDoubleWide() 
					&&( (curB->stacks[g]->attackerOwned && curB->stacks[g]->position-1 == pos) 
					||	(!curB->stacks[g]->attackerOwned && curB->stacks[g]->position+1 == pos)	)
			 ))
		  && curB->stacks[g]->alive()
		  )
			return curB->stacks[g]->ID;
	}
	return -1;
}

int CGameState::battleGetBattlefieldType(int3 tile)
{
	if(tile==int3() && curB)
		tile = curB->tile;
	else if(tile==int3() && !curB)
		return -1;

	//std::vector < std::pair<const CGObjectInstance*,SDL_Rect> > & objs = CGI->mh->ttiles[tile.x][tile.y][tile.z].objects;
	//for(int g=0; g<objs.size(); ++g)
	//{
	//	switch(objs[g].first->ID)
	//	{
	//	case 222: //clover field
	//		return 19;
	//	case 223: //cursed ground
	//		return 22;
	//	case 224: //evil fog
	//		return 20;
	//	case 225: //favourable winds
	//		return 21;
	//	case 226: //fiery fields
	//		return 14;
	//	case 227: //holy ground
	//		return 18;
	//	case 228: //lucid pools
	//		return 17;
	//	case 229: //magic clouds
	//		return 16;
	//	case 230: //magic plains
	//		return 9;
	//	case 231: //rocklands
	//		return 15;
	//	}
	//}

	switch(map->terrain[tile.x][tile.y][tile.z].tertype)
	{
	case dirt:
		return rand()%3+3;
	case sand:
		return 2; //TODO: coast support
	case grass:
		return rand()%2+6;
	case snow:
		return rand()%2+10;
	case swamp:
		return 13;
	case rough:
		return 23;
	case subterranean:
		return 12;
	case lava:
		return 8;
	case water:
		return 25;
	case rock:
		return 15;
	default:
		return -1;
	}
}

UpgradeInfo CGameState::getUpgradeInfo(CArmedInstance *obj, int stackPos)
{
	UpgradeInfo ret;
	CCreature *base = &VLC->creh->creatures[obj->army.slots[stackPos].first];
	if((obj->ID == TOWNI_TYPE)  ||  ((obj->ID == HEROI_TYPE) && static_cast<const CGHeroInstance*>(obj)->visitedTown))
	{
		CGTownInstance * t;
		if(obj->ID == TOWNI_TYPE)
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
	for(int i=0;i<getPlayer(player)->towns.size();i++)
		if(vstd::contains(getPlayer(player)->towns[i]->builtBuildings,14))
			mcount++;
	float ret = std::min(((float)mcount+1.0f)/20.0f,0.5f);
	return ret;
}

void CGameState::loadTownDInfos()
{
	for(int i=0;i<F_NUMBER;i++)
	{
		villages[i] = new CGDefInfo(*VLC->dobjinfo->castles[i]);
		forts[i] = VLC->dobjinfo->castles[i];
		capitols[i] = new CGDefInfo(*VLC->dobjinfo->castles[i]);
	}
}

void CGameState::getNeighbours(int3 tile, std::vector<int3> &vec, const boost::logic::tribool &onLand)
{
	vec.clear();
	int3 hlp;
	bool weAreOnLand = (map->getTile(tile).tertype != 8);
	if(tile.x > 0)
	{
		hlp = int3(tile.x-1,tile.y,tile.z);
		if((weAreOnLand == (map->getTile(hlp).tertype!=8)) && map->getTile(hlp).tertype!=9) 
			vec.push_back(hlp);
	}
	if(tile.y > 0)
	{
		hlp = int3(tile.x,tile.y-1,tile.z);
		if((weAreOnLand == (map->getTile(hlp).tertype!=8)) && map->getTile(hlp).tertype!=9) 
			vec.push_back(hlp);
	}
	if(tile.x > 0   &&   tile.y > 0)
	{
		hlp = int3(tile.x-1,tile.y-1,tile.z);
		if((weAreOnLand == (map->getTile(hlp).tertype!=8)) && map->getTile(hlp).tertype!=9) 
			vec.push_back(hlp);
	}
	if(tile.x > 0   &&   tile.y < map->height-1)
	{
		hlp = int3(tile.x-1,tile.y+1,tile.z);
		if((weAreOnLand == (map->getTile(hlp).tertype!=8)) && map->getTile(hlp).tertype!=9) 
			vec.push_back(hlp);
	}
	if(tile.y < map->height-1)
	{
		hlp = int3(tile.x,tile.y+1,tile.z);
		if((weAreOnLand == (map->getTile(hlp).tertype!=8)) && map->getTile(hlp).tertype!=9) 
			vec.push_back(hlp);
	}
	if(tile.x < map->width-1)
	{
		hlp = int3(tile.x+1,tile.y,tile.z);
		if((weAreOnLand == (map->getTile(hlp).tertype!=8)) && map->getTile(hlp).tertype!=9) 
			vec.push_back(hlp);
	}
	if(tile.x < map->width-1   &&   tile.y > 0)
	{
		hlp = int3(tile.x+1,tile.y-1,tile.z);
		if((weAreOnLand == (map->getTile(hlp).tertype!=8)) && map->getTile(hlp).tertype!=9) 
			vec.push_back(hlp);
	}
	if(tile.x < map->width-1   &&   tile.y < map->height-1)
	{
		hlp = int3(tile.x+1,tile.y+1,tile.z);
		if((weAreOnLand == (map->getTile(hlp).tertype!=8)) && map->getTile(hlp).tertype!=9) 
			vec.push_back(hlp);
	}
}

int CGameState::getMovementCost(const CGHeroInstance *h, int3 src, int3 dest, int remainingMovePoints, bool checkLast)
{
	if(src == dest) //same tile
		return 0;

	TerrainTile &s = map->terrain[src.x][src.y][src.z],
		&d = map->terrain[dest.x][dest.y][dest.z];

	//get basic cost
	int ret = h->getTileCost(d,s);

	if(src.x!=dest.x && src.y!=dest.y) //it's diagonal move
	{
		int old = ret;
		ret *= 1.414;
		//diagonal move costs too much but normal move is possible - allow diagonal move
		if(ret > remainingMovePoints  &&  remainingMovePoints > old)
		{
			return remainingMovePoints;
		}
	}


	int left = remainingMovePoints-ret;
	if(checkLast  &&  left > 0  &&  remainingMovePoints-ret < 250) //it might be the last tile - if no further move possible we take all move points
	{
		std::vector<int3> vec;
		getNeighbours(dest,vec,true);
		for(size_t i=0; i < vec.size(); i++)
		{
			int fcost = getMovementCost(h,dest,vec[i],left,false);
			if(fcost <= left)
			{
				return ret;
			}
		}
		ret = remainingMovePoints;
	}
	return ret;
}

int CGameState::canBuildStructure( const CGTownInstance *t, int ID )
{
	int ret = 7; //allowed by default

	//checking resources
	CBuilding * pom = VLC->buildh->buildings[t->subID][ID];
	for(int res=0;res<7;res++) //TODO: support custom amount of resources
	{
		if(pom->resources[res] > getPlayer(t->tempOwner)->resources[res])
			ret = 6; //lack of res
	}

	//checking for requirements
	for( std::set<int>::iterator ri  =  VLC->townh->requirements[t->subID][ID].begin();
		ri != VLC->townh->requirements[t->subID][ID].end();
		ri++ )
	{
		if(t->builtBuildings.find(*ri)==t->builtBuildings.end())
			ret = 8; //lack of requirements - cannot build
	}

	//can we build it?
	if(t->forbiddenBuildings.find(ID)!=t->forbiddenBuildings.end())
		ret = 2; //forbidden
	else if(t->builded >= MAX_BUILDING_PER_TURN)
		ret = 5; //building limit

	if(ID == 13) //capitol
	{
		for(int in = 0; in < map->towns.size(); in++)
		{
			if(map->towns[in]->tempOwner==t->tempOwner  &&  vstd::contains(map->towns[in]->builtBuildings,13))
			{
				ret = 0; //no more than one capitol
				break;
			}
		}
	}
	else if(ID == 6) //shipyard
	{
		if(map->getTile(t->pos + int3(-1,3,0)).tertype != water  &&  map->getTile(t->pos + int3(-3,3,0)).tertype != water)
			ret = 1; //lack of water
	}

	return ret;
}

void CGameState::apply(CPack *pack)
{
	applierGs->apps[typeList.getTypeID(pack)]->applyOnGS(this,pack);
}

PlayerState * CGameState::getPlayer( ui8 color )
{
	if(vstd::contains(players,color))
	{
		return &players[color];
	}
	else 
	{
		tlog2 << "Warning: Cannot find info for player " << int(color) << std::endl;
		return NULL;
	}
}

CPath * CGameState::getPath(int3 src, int3 dest, const CGHeroInstance * hero)
{
	if(!map->isInTheMap(src) || !map->isInTheMap(dest)) //check input
		return NULL;

	int3 hpos = hero->getPosition(false);
	tribool blockLandSea; //true - blocks sea, false - blocks land, indeterminate - allows all

	if (!hero->canWalkOnSea())
		blockLandSea = (map->getTile(hpos).tertype != water); //block land if hero is on water and vice versa
	else
		blockLandSea = boost::logic::indeterminate;

	//graph initialization
	std::vector< std::vector<CPathNode> > graph;
	graph.resize(map->width);
	for(size_t i=0; i<graph.size(); ++i)
	{
		graph[i].resize(map->height);
		for(size_t j=0; j<graph[i].size(); ++j)
		{
			const TerrainTile *tinfo = &map->terrain[i][j][src.z];
			CPathNode &node = graph[i][j];

			node.accesible = !tinfo->blocked;
			if(i==dest.x && j==dest.y && tinfo->visitable)
			{
				node.accesible = true; //for allowing visiting objects
			}
			node.dist = -1;
			node.theNodeBefore = NULL;
			node.visited = false;
			node.coord.x = i;
			node.coord.y = j;
			node.coord.z = dest.z;

			if ((tinfo->tertype == rock) //it's rock
				|| ((blockLandSea) && (tinfo->tertype == water)) //it's sea and we cannot walk on sea
				|| ((!blockLandSea) && (tinfo->tertype != water)) //it's land and we cannot walk on land
				|| !getPlayer(hero->tempOwner)->fogOfWarMap[i][j][src.z] //tile is covered by the FoW
			)
			{
				node.accesible = false;
			}
		}
	}
	//graph initialized

	//initial tile - set cost on 0 and add to the queue
	graph[src.x][src.y].dist = 0; 
	std::queue<CPathNode> mq;
	mq.push(graph[src.x][src.y]);

	ui32 curDist = 0xffffffff; //total cost of path - init with max possible val

	std::vector<int3> neighbours;
	neighbours.reserve(8);

	while(!mq.empty())
	{
		CPathNode &cp = graph[mq.front().coord.x][mq.front().coord.y];
		mq.pop();
		if (cp.coord == dest) //it's destination tile
		{
			if (cp.dist < curDist) //that path is better than previous one
				curDist = cp.dist;
			continue;
		}
		else
		{
			if (cp.dist > curDist) //it's not dest and current length is greater than cost of already found path 
				continue;
		}

		//add accessible neighbouring nodes to the queue
		getNeighbours(cp.coord,neighbours,blockLandSea);
		for(int i=0; i < neighbours.size(); i++)
		{
			CPathNode & dp = graph[neighbours[i].x][neighbours[i].y];
			if(dp.accesible)
			{
				int cost = getMovementCost(hero,cp.coord,dp.coord,hero->movement - cp.dist);
				if((dp.dist==-1 || (dp.dist > cp.dist + cost)) && dp.accesible && checkForVisitableDir(cp.coord, dp.coord) && checkForVisitableDir(dp.coord, cp.coord))
				{
					dp.dist = cp.dist + cost;
					dp.theNodeBefore = &cp;
					mq.push(dp);
				}
			}
		}
	}

	CPathNode *curNode = &graph[dest.x][dest.y];
	if(!curNode->theNodeBefore) //destination is not accessible
		return NULL;

	CPath * ret = new CPath;

	while(curNode->coord != graph[src.x][src.y].coord)
	{
		ret->nodes.push_back(*curNode);
		curNode = curNode->theNodeBefore;
	}

	ret->nodes.push_back(graph[src.x][src.y]);

	return ret;
}

bool CGameState::checkForVisitableDir(const int3 & src, const int3 & dst) const
{
	const TerrainTile * pom = &map->getTile(dst);
	for(int b=0; b<pom->visitableObjects.size(); ++b) //checking destination tile
	{
		if(!vstd::contains(pom->blockingObjects, pom->visitableObjects[b])) //this visitable object is not blocking, ignore
			continue;

		CGDefInfo * di = pom->visitableObjects[b]->defInfo;
		if( (dst.x == src.x-1 && dst.y == src.y-1) && !(di->visitDir & (1<<4)) )
		{
			return false;
		}
		if( (dst.x == src.x && dst.y == src.y-1) && !(di->visitDir & (1<<5)) )
		{
			return false;
		}
		if( (dst.x == src.x+1 && dst.y == src.y-1) && !(di->visitDir & (1<<6)) )
		{
			return false;
		}
		if( (dst.x == src.x+1 && dst.y == src.y) && !(di->visitDir & (1<<7)) )
		{
			return false;
		}
		if( (dst.x == src.x+1 && dst.y == src.y+1) && !(di->visitDir & (1<<0)) )
		{
			return false;
		}
		if( (dst.x == src.x && dst.y == src.y+1) && !(di->visitDir & (1<<1)) )
		{
			return false;
		}
		if( (dst.x == src.x-1 && dst.y == src.y+1) && !(di->visitDir & (1<<2)) )
		{
			return false;
		}
		if( (dst.x == src.x-1 && dst.y == src.y) && !(di->visitDir & (1<<3)) )
		{
			return false;
		}
	}
	return true;
}

int BattleInfo::calculateDmg(const CStack* attacker, const CStack* defender, const CGHeroInstance * attackerHero, const CGHeroInstance * defendingHero, bool shooting)
{
	int attackerAttackBonus = attacker->creature->attack + (attackerHero ? attackerHero->getPrimSkillLevel(0) : 0),
		defenderDefenseBonus = defender->creature->defence + (defendingHero ? defendingHero->getPrimSkillLevel(1) : 0),
		attackDefenseBonus = 0,
		minDmg = attacker->creature->damageMin * attacker->amount, 
		maxDmg = attacker->creature->damageMax * attacker->amount;

	//calculating total attack/defense skills modifier
	if(attacker->getEffect(56)) //frenzy for attacker
	{
		attackerAttackBonus += (VLC->spellh->spells[attacker->getEffect(56)->id].powers[attacker->getEffect(56)->level]/100.0) *(attacker->creature->defence + (attackerHero ? attackerHero->getPrimSkillLevel(1) : 0));
	}
	if(defender->getEffect(56)) //frenzy for defender
	{
		defenderDefenseBonus = 0;
	}
	attackDefenseBonus = attackerAttackBonus - defenderDefenseBonus;
	if(defender->getEffect(48)) //defender's prayer handling
	{
		attackDefenseBonus -= VLC->spellh->spells[defender->getEffect(48)->id].powers[defender->getEffect(48)->level];
	}
	if(attacker->getEffect(48)) //attacker's prayer handling
	{
		attackDefenseBonus += VLC->spellh->spells[attacker->getEffect(48)->id].powers[attacker->getEffect(48)->level];
	}
	if(defender->getEffect(46)) //stone skin handling
	{
		attackDefenseBonus -= VLC->spellh->spells[defender->getEffect(46)->id].powers[defender->getEffect(46)->level];
	}
	if(attacker->getEffect(45)) //weakness handling
	{
		attackDefenseBonus -= VLC->spellh->spells[attacker->getEffect(45)->id].powers[attacker->getEffect(45)->level];
	}
	if(!shooting && attacker->getEffect(43)) //bloodlust handling
	{
		attackDefenseBonus += VLC->spellh->spells[attacker->getEffect(43)->id].powers[attacker->getEffect(43)->level];
	}

	float dmgBonusMultiplier = 1.0f;

	//bonus from attack/defense skills
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
			switch(attackerHero->getSecSkillLevel(22)) //offense
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
		switch(defendingHero->getSecSkillLevel(23)) //armorer
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
	//handling spell effects
	if(!shooting && defender->getEffect(27)) //shield
	{
		if(defender->getEffect(27)->level<=1) //none or basic
			dmgBonusMultiplier *= 0.85f;
		else //adv or expert
			dmgBonusMultiplier *= 0.7f;
	}
	else if(shooting && defender->getEffect(28)) //air shield
	{
		if(defender->getEffect(28)->level<=1) //none or basic
			dmgBonusMultiplier *= 0.75f;
		else //adv or expert
			dmgBonusMultiplier *= 0.5f;
	}
	if(attacker->getEffect(42)) //curse handling (partial, the rest is below)
	{
		if(attacker->getEffect(42)->level>=2) //adv or expert
			dmgBonusMultiplier *= 0.8f;
	}



	minDmg *= dmgBonusMultiplier;
	maxDmg *= dmgBonusMultiplier;

	if(attacker->getEffect(42)) //curse handling (rest)
	{
		minDmg -= VLC->spellh->spells[attacker->getEffect(42)->id].powers[attacker->getEffect(42)->level];
		return minDmg;
	}
	else if(attacker->getEffect(41)) //bless handling
	{
		maxDmg += VLC->spellh->spells[attacker->getEffect(41)->id].powers[attacker->getEffect(41)->level];
		return maxDmg;
	}
	else
	{
		if(minDmg != maxDmg)
			return minDmg  +  rand() % (maxDmg - minDmg + 1);
		else
			return minDmg;
	}

	tlog1 << "We are too far in calculateDmg...\n";
	return -1;
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

CStack * BattleInfo::getNextStack()
{
	CStack *current = getStack(activeStack);
	for (int i = 0; i <  stacks.size(); i++)  //find fastest not moved/waited stack (stacks vector is sorted by speed)
	{
		if(stacks[i]->willMove()  &&  !vstd::contains(stacks[i]->state,WAITING))
			return stacks[i];
	}
	for (int i = stacks.size() - 1; i >= 0 ; i--) //find slowest waiting stack
	{
		if(stacks[i]->willMove())
			return stacks[i];
	}
	return NULL; //all stacks moved or defending!
}

std::vector<CStack> BattleInfo::getStackQueue()
{
	std::vector<CStack> ret;
	std::vector<int> taken; //if non-zero value, corresponding stack has been placed in ret
	taken.resize(stacks.size());
	for(int g=0; g<taken.size(); ++g)
	{
		taken[g] = 0;
	}

	for(int moved=0; moved<2; ++moved) //in first cycle we add stacks that can act in current turn, in second one the rest of them
	{
		for(int gc=0; gc<stacks.size(); ++gc)
		{
			int id = -1, speed = -1;
			for(int i=0; i<stacks.size(); ++i) //find not waited stacks only
			{
				if((moved == 1 ||!vstd::contains(stacks[i]->state,DEFENDING))
					&& stacks[i]->alive()
					&& (moved == 1 || !vstd::contains(stacks[i]->state,MOVED))
					&& !vstd::contains(stacks[i]->state,WAITING)
					&& taken[i]==0
					&& !vstd::contains(stacks[i]->abilities,NOT_ACTIVE)) //eg. Ammo Cart
				{
					if(speed == -1 || stacks[i]->Speed() > speed)
					{
						id = i;
						speed = stacks[i]->Speed();
					}
				}
			}
			if(id != -1)
			{
				ret.push_back(*stacks[id]);
				taken[id] = 1;
			}
			else //choose something from not moved stacks
			{
				int id = -1, speed = 10000; //infinite speed
				for(int i=0; i<stacks.size(); ++i) //find waited stacks only
				{
					if((moved == 1 ||!vstd::contains(stacks[i]->state,DEFENDING))
						&& stacks[i]->alive()
						&& (moved == 1 || !vstd::contains(stacks[i]->state,MOVED))
						&& vstd::contains(stacks[i]->state,WAITING)
						&& taken[i]==0
						&& !vstd::contains(stacks[i]->abilities,NOT_ACTIVE)) //eg. Ammo Cart
					{
						if(stacks[i]->Speed() < speed) //slowest one
						{
							id = i;
							speed = stacks[i]->Speed();
						}
					}
				}
				if(id != -1)
				{
					ret.push_back(*stacks[id]);
					taken[id] = 1;
				}
				else
				{
					break; //no stacks have been found, so none of them will be found in next iterations
				}
			}
		}
	}
	return ret;
}

int3 CPath::startPos() const
{
	return nodes[nodes.size()-1].coord;
}
void CPath::convert(ui8 mode) //mode=0 -> from 'manifest' to 'object'
{
	if (mode==0)
	{
		for (int i=0;i<nodes.size();i++)
		{
			nodes[i].coord = CGHeroInstance::convertPosition(nodes[i].coord,true);
		}
	}
}

int3 CPath::endPos() const
{
	return nodes[0].coord;
}
