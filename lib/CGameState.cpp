#define VCMI_DLL
#include "CCampaignHandler.h"
#include <algorithm>
#include <queue>
#include <fstream>
#include "CGameState.h"
#include <boost/random/linear_congruential.hpp>
#include "CDefObjInfoHandler.h"
#include "CArtHandler.h"
#include "CBuildingHandler.h"
#include "CGeneralTextHandler.h"
#include "CTownHandler.h"
#include "CSpellHandler.h"
#include "CHeroHandler.h"
#include "CObjectHandler.h"
#include "CCreatureHandler.h"
//#include "../lib/IGameCallback.h"
#include "VCMI_Lib.h"
#include "Connection.h"
#include "map.h"
#include "../StartInfo.h"
#include "NetPacks.h"
#include <boost/foreach.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/thread.hpp>
#include <boost/thread/shared_mutex.hpp>
#include <boost/assign/list_of.hpp>
#include "RegisterTypes.cpp"
#include <algorithm>
#include <numeric>
#include "CMapInfo.h"
#include "BattleState.h"

boost::rand48 ran;
class CGObjectInstance;

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

template <typename T> class CApplyOnGS;

class CBaseForGSApply
{
public:
	virtual void applyOnGS(CGameState *gs, void *pack) const =0; 
	virtual ~CBaseForGSApply(){};
	template<typename U> static CBaseForGSApply *getApplier(const U * t=NULL)
	{
		return new CApplyOnGS<U>;
	}
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

static CApplier<CBaseForGSApply> *applierGs = NULL;

class IObjectCaller
{
public:
	virtual void preInit()=0;
	virtual void postInit()=0;
};

template <typename T>
class CObjectCaller : public IObjectCaller
{
public:
	void preInit()
	{
		//T::preInit();
	}
	void postInit()
	{
		//T::postInit();
	}
};

class CObjectCallersHandler
{
public:
	std::vector<IObjectCaller*> apps; 

	template<typename T> void registerType(const T * t=NULL)
	{
		apps.push_back(new CObjectCaller<T>);
	}

	CObjectCallersHandler()
	{
		registerTypes1(*this);
	}

	~CObjectCallersHandler()
	{
		for (size_t i = 0; i < apps.size(); i++)
			delete apps[i];
	}

	void preInit()
	{
// 		for (size_t i = 0; i < apps.size(); i++)
// 			apps[i]->preInit();
	}

	void postInit()
	{
	//for (size_t i = 0; i < apps.size(); i++)
	//apps[i]->postInit();
	}
} *objCaller = NULL;

InfoAboutTown::InfoAboutTown()
{
	tType = NULL;
	details = NULL;
	fortLevel = 0;
	owner = -1;
}

InfoAboutTown::~InfoAboutTown()
{
	delete details;
}

void InfoAboutTown::initFromTown( const CGTownInstance *t, bool detailed )
{
	obj = t;
	army = ArmyDescriptor(t->getUpperArmy(), detailed);
	built = t->builded;
	fortLevel = t->fortLevel();
	name = t->name;
	tType = t->town;
	owner = t->tempOwner;

	if(detailed) 
	{
		//include details about hero
		details = new Details;
		details->goldIncome = t->dailyIncome();
		details->customRes = vstd::contains(t->builtBuildings, 15);
		details->hallLevel = t->hallLevel();
		details->garrisonedHero = t->garrisonHero;
	}
	//TODO: adjust undetailed info about army to our count of thieves guilds
}

void InfoAboutTown::initFromGarrison(const CGGarrison *garr, bool detailed)
{
	obj = garr;
	fortLevel = 0;
	army = ArmyDescriptor(garr, detailed);
	name = VLC->generaltexth->names[33]; // "Garrison"
	owner = garr->tempOwner;
	built = false;
	tType = NULL;

	// Show detailed info only to owning player.
	if(detailed)
	{
		details = new InfoAboutTown::Details;
		details->customRes = false;
		details->garrisonedHero = false;
		details->goldIncome = -1;
		details->hallLevel = -1;
	}
}

void MetaString::getLocalString(const std::pair<ui8,ui32> &txt, std::string &dst) const
{
	int type = txt.first, ser = txt.second;

	if(type == ART_NAMES)
	{
		dst = VLC->arth->artifacts[ser]->Name();
	}
	else if(type == CRE_PL_NAMES)
	{
		dst = VLC->creh->creatures[ser]->namePl;
	}
	else if(type == MINE_NAMES)
	{
		dst = VLC->generaltexth->mines[ser].first;
	}
	else if(type == MINE_EVNTS)
	{
		dst = VLC->generaltexth->mines[ser].second;
	}
	else if(type == SPELL_NAME)
	{
		dst = VLC->spellh->spells[ser]->name;
	}
	else if(type == CRE_SING_NAMES)
	{
		dst = VLC->creh->creatures[ser]->nameSing;
	}
	else if(type == ART_DESCR)
	{
		dst = VLC->arth->artifacts[ser]->Description();
	}
	else
	{
		std::vector<std::string> *vec;
		switch(type)
		{
		case GENERAL_TXT:
			vec = &VLC->generaltexth->allTexts;
			break;
		case XTRAINFO_TXT:
			vec = &VLC->generaltexth->xtrainfo;
			break;
		case OBJ_NAMES:
			vec = &VLC->generaltexth->names;
			break;
		case RES_NAMES:
			vec = &VLC->generaltexth->restypes;
			break;
		case ARRAY_TXT:
			vec = &VLC->generaltexth->arraytxt;
			break;
		case CREGENS:
			vec = &VLC->generaltexth->creGens;
			break;
		case CREGENS4:
			vec = &VLC->generaltexth->creGens4;
			break;
		case ADVOB_TXT:
			vec = &VLC->generaltexth->advobtxt;
			break;
		case ART_EVNTS:
			vec = &VLC->generaltexth->artifEvents;
			break;
		case SEC_SKILL_NAME:
			vec = &VLC->generaltexth->skillName;
			break;
		case COLOR:
			vec = &VLC->generaltexth->capColors;
			break;
		}
		dst = (*vec)[ser];
	}
}

DLL_EXPORT void MetaString::toString(std::string &dst) const
{
	size_t exSt = 0, loSt = 0, nums = 0;
	dst.clear();

	for(size_t i=0;i<message.size();++i)
	{//TEXACT_STRING, TLOCAL_STRING, TNUMBER, TREPLACE_ESTRING, TREPLACE_LSTRING, TREPLACE_NUMBER
		switch(message[i])
		{
		case TEXACT_STRING:
			dst += exactStrings[exSt++];
			break;
		case TLOCAL_STRING:
			{
				std::string hlp;
				getLocalString(localStrings[loSt++], hlp);
				dst += hlp;
			}
			break;
		case TNUMBER:
			dst += boost::lexical_cast<std::string>(numbers[nums++]);
			break;
		case TREPLACE_ESTRING:
			dst.replace (dst.find("%s"), 2, exactStrings[exSt++]);
			break;
		case TREPLACE_LSTRING:
			{
				std::string hlp;
				getLocalString(localStrings[loSt++], hlp);
				dst.replace (dst.find("%s"), 2, hlp);
			}
			break;
		case TREPLACE_NUMBER:
			dst.replace (dst.find("%d"), 2, boost::lexical_cast<std::string>(numbers[nums++]));
			break;
		case TREPLACE_PLUSNUMBER:
			dst.replace (dst.find("%+d"), 3, '+' + boost::lexical_cast<std::string>(numbers[nums++]));
			break;
		default:
			tlog1 << "MetaString processing error!\n";
			break;
		}
	}
}

DLL_EXPORT std::string MetaString::toString() const
{
	std::string ret;
	toString(ret);
	return ret;
}

DLL_EXPORT std::string MetaString::buildList () const
///used to handle loot from creature bank
{

	size_t exSt = 0, loSt = 0, nums = 0;
	std::string lista;		
	for (int i = 0; i < message.size(); ++i)
	{
		if (i > 0 && message[i] == TEXACT_STRING || message[i] == TLOCAL_STRING)
		{
			if (exSt == exactStrings.size() - 1)
				lista += VLC->generaltexth->allTexts[141]; //" and "
			else
				lista += ", ";
		}
		switch (message[i])
		{
			case TEXACT_STRING:
				lista += exactStrings[exSt++];
				break;
			case TLOCAL_STRING:
			{
				std::string hlp;
				getLocalString (localStrings[loSt++], hlp);
				lista += hlp;
			}
				break;
			case TNUMBER:
				lista += boost::lexical_cast<std::string>(numbers[nums++]);
				break;
			case TREPLACE_ESTRING:
				lista.replace (lista.find("%s"), 2, exactStrings[exSt++]);
				break;
			case TREPLACE_LSTRING:
			{
				std::string hlp;
				getLocalString (localStrings[loSt++], hlp);
				lista.replace (lista.find("%s"), 2, hlp);
			}
				break;
			case TREPLACE_NUMBER:
				lista.replace (lista.find("%d"), 2, boost::lexical_cast<std::string>(numbers[nums++]));
				break;
			default:
				tlog1 << "MetaString processing error!\n";
		}

	}
	return lista;
}


void  MetaString::addCreReplacement(TCreature id, TQuantity count) //adds sing or plural name;
{
	assert(count);
	if (count == 1)
		addReplacement (CRE_SING_NAMES, id);
	else
		addReplacement (CRE_PL_NAMES, id);
}

void MetaString::addReplacement(const CStackBasicDescriptor &stack)
{
	assert(stack.count); //valid count
	assert(stack.type); //valid type
	addCreReplacement(stack.type->idNumber, stack.count);
}

static CGObjectInstance * createObject(int id, int subid, int3 pos, int owner)
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
	nobj->defInfo->id = id;
	nobj->defInfo->subid = subid;

	//assigning defhandler
	if(nobj->ID==HEROI_TYPE || nobj->ID==TOWNI_TYPE)
		return nobj;
	nobj->defInfo = VLC->dobjinfo->gobjs[id][subid];
	return nobj;
}

CGHeroInstance * CGameState::HeroesPool::pickHeroFor(bool native, int player, const CTown *town, bmap<ui32, ConstTransitivePtr<CGHeroInstance> > &available, const CHeroClass *bannedClass /*= NULL*/) const
{
	CGHeroInstance *ret = NULL;

	if(player<0 || player>=PLAYER_LIMIT)
	{
		tlog1 << "Cannot pick hero for " << town->Name() << ". Wrong owner!\n";
		return NULL;
	}

	std::vector<CGHeroInstance *> pool;

	if(native)
	{
		for(bmap<ui32, ConstTransitivePtr<CGHeroInstance> >::iterator i=available.begin(); i!=available.end(); i++)
		{
			if(pavailable.find(i->first)->second & 1<<player
				&& i->second->type->heroType/2 == town->typeID)
			{
				pool.push_back(i->second); //get all avaliable heroes
			}
		}
		if(!pool.size())
		{
			tlog1 << "Cannot pick native hero for " << player << ". Picking any...\n";
			return pickHeroFor(false, player, town, available);
		}
		else
		{
			ret = pool[rand()%pool.size()];
		}
	}
	else
	{
		int sum=0, r;

		for(bmap<ui32, ConstTransitivePtr<CGHeroInstance> >::iterator i=available.begin(); i!=available.end(); i++)
		{
			if(pavailable.find(i->first)->second & 1<<player
				&& !bannedClass || i->second->type->heroClass != bannedClass)
			{
				pool.push_back(i->second);
				sum += i->second->type->heroClass->selectionProbability[town->typeID]; //total weight
			}
		}
		if(!pool.size())
		{
			tlog1 << "There are no heroes available for player " << player<<"!\n";
			return NULL;
		}

		r = rand()%sum;
		for (unsigned int i=0; i<pool.size(); i++)
		{
			r -= pool[i]->type->heroClass->selectionProbability[town->typeID];
			if(r < 0)
			{
				ret = pool[i];
				break;
			}
		}
		if(!ret)
			ret = pool.back();
	}

	available.erase(ret->subID);
	return ret;
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
	const PlayerSettings &ps = scenarioOps->getIthPlayersSettings(owner);
	if(!map->getHero(h = ps.hero,0)  &&  h>=0) //we haven't used selected hero
		return h;
	int i=0;

	do //try to find free hero of our faction
	{
		i++;
		h = ps.castle*HEROES_PER_TYPE*2+(ran()%(HEROES_PER_TYPE*2));//->scenarioOps->playerInfos[pru].hero = VLC->
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
	if(objid<0 || objid>=map->objects.size() || map->objects[objid]->ID!=HEROI_TYPE)
		return NULL;
	return static_cast<CGHeroInstance *>(map->objects[objid].get());
}

const CGHeroInstance * CGameState::getHero( int objid ) const
{
	return (const_cast<CGameState *>(this))->getHero(objid);
}

CGTownInstance *CGameState::getTown(int objid)
{
	if(objid<0 || objid>=map->objects.size())
		return NULL;
	CGObjectInstance *obj = map->objects[objid];

	if(obj->ID != TOWNI_TYPE)
		return NULL;

	return static_cast<CGTownInstance *>(obj);
}

const CGTownInstance * CGameState::getTown( int objid ) const
{
	return (const_cast<CGameState *>(this))->getTown(objid);
}

std::pair<int,int> CGameState::pickObject (CGObjectInstance *obj)
{
	switch(obj->ID)
	{
	case 65:
		return std::pair<int,int>(5, VLC->arth->getRandomArt (CArtifact::ART_TREASURE | CArtifact::ART_MINOR | CArtifact::ART_MAJOR | CArtifact::ART_RELIC));
	case 66: //random treasure artifact
		return std::pair<int,int>(5, VLC->arth->getRandomArt (CArtifact::ART_TREASURE));
	case 67: //random minor artifact
		return std::pair<int,int>(5, VLC->arth->getRandomArt (CArtifact::ART_MINOR));
	case 68: //random major artifact
		return std::pair<int,int>(5, VLC->arth->getRandomArt (CArtifact::ART_MAJOR));
	case 69: //random relic artifact
		return std::pair<int,int>(5, VLC->arth->getRandomArt (CArtifact::ART_RELIC));
	case 70: //random hero
		return std::pair<int,int>(HEROI_TYPE,pickHero(obj->tempOwner));
	case 71: //random monster
		return std::pair<int,int>(54,VLC->creh->pickRandomMonster(boost::ref(ran))); 
	case 72: //random monster lvl1
		return std::pair<int,int>(54,VLC->creh->pickRandomMonster(boost::ref(ran), 1)); 
	case 73: //random monster lvl2
		return std::pair<int,int>(54,VLC->creh->pickRandomMonster(boost::ref(ran), 2));
	case 74: //random monster lvl3
		return std::pair<int,int>(54,VLC->creh->pickRandomMonster(boost::ref(ran), 3));
	case 75: //random monster lvl4
		return std::pair<int,int>(54, VLC->creh->pickRandomMonster(boost::ref(ran), 4));
	case 76: //random resource
		return std::pair<int,int>(79,ran()%7); //now it's OH3 style, use %8 for mithril 
	case 77: //random town
		{
			int align = (static_cast<CGTownInstance*>(obj))->alignment,
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
		return std::pair<int,int>(54, VLC->creh->pickRandomMonster(boost::ref(ran), 5));
	case 163: //random monster lvl6
		return std::pair<int,int>(54, VLC->creh->pickRandomMonster(boost::ref(ran), 6));
	case 164: //random monster lvl7
		return std::pair<int,int>(54, VLC->creh->pickRandomMonster(boost::ref(ran), 7));
	case 216: //random dwelling
		{
			int faction = ran()%F_NUMBER;
			CGDwelling * dwl = static_cast<CGDwelling*>(obj);
			CCreGen2ObjInfo* info = static_cast<CCreGen2ObjInfo*>(dwl->info);
			if (info->asCastle)
			{
				for(unsigned int i=0;i<map->objects.size();i++)
				{
					if(map->objects[i]->ID==77 && dynamic_cast<CGTownInstance*>(map->objects[i].get())->identifier == info->identifier)
					{
						randomizeObject(map->objects[i]); //we have to randomize the castle first
						faction = map->objects[i]->subID;
						break;
					}
					else if(map->objects[i]->ID==TOWNI_TYPE && dynamic_cast<CGTownInstance*>(map->objects[i].get())->identifier == info->identifier)
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
			for(unsigned int i=0;i<VLC->objh->cregens.size();i++)
				if(VLC->objh->cregens[i]==cid)
					return std::pair<int,int>(17,i); 
			tlog3 << "Cannot find a dwelling for creature "<< cid << std::endl;
			return std::pair<int,int>(17,0);
			delete dwl->info;
			dwl->info = NULL;
		}
	case 217:
		{
			int faction = ran()%F_NUMBER;
			CGDwelling * dwl = static_cast<CGDwelling*>(obj);
			CCreGenObjInfo* info = static_cast<CCreGenObjInfo*>(dwl->info);
			if (info->asCastle)
			{
				for(unsigned int i=0;i<map->objects.size();i++)
				{
					if(map->objects[i]->ID==77 && dynamic_cast<CGTownInstance*>(map->objects[i].get())->identifier == info->identifier)
					{
						randomizeObject(map->objects[i]); //we have to randomize the castle first
						faction = map->objects[i]->subID;
						break;
					}
					else if(map->objects[i]->ID==TOWNI_TYPE && dynamic_cast<CGTownInstance*>(map->objects[i].get())->identifier == info->identifier)
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
			for(unsigned int i=0;i<VLC->objh->cregens.size();i++)
				if(VLC->objh->cregens[i]==cid)
					return std::pair<int,int>(17,i); 
			tlog3 << "Cannot find a dwelling for creature "<<cid <<std::endl;
			return std::pair<int,int>(17,0);
			delete dwl->info;
			dwl->info = NULL;
		}
	case 218:
		{
			CGDwelling * dwl = static_cast<CGDwelling*>(obj);
			CCreGen3ObjInfo* info = static_cast<CCreGen3ObjInfo*>(dwl->info);
			int level = ((info->maxLevel-info->minLevel) ? (ran()%(info->maxLevel-info->minLevel)+info->minLevel) : (info->minLevel));
			int cid = VLC->townh->towns[obj->subID].basicCreatures[level];
			for(unsigned int i=0;i<VLC->objh->cregens.size();i++)
				if(VLC->objh->cregens[i]==cid)
					return std::pair<int,int>(17,i); 
			tlog3 << "Cannot find a dwelling for creature "<<cid <<std::endl;
			return std::pair<int,int>(17,0); 
			delete dwl->info;
			dwl->info = NULL;
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
			t->town = &VLC->townh->towns[t->subID];
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
		h->randomizeArmy(h->type->heroType/2);
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
		t->randomizeArmy(t->subID);
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
	case 0: //day number
		return day;
		break;
	case 1: //day of week
		temp = (day)%7; // 1 - Monday, 7 - Sunday
		if (temp)
			return temp;
		else return 7;
		break;
	case 2:  //current week
		temp = ((day-1)/7)+1;
		if (!(temp%4))
			return 4;
		else 
			return (temp%4);
		break;
	case 3: //current month
		return ((day-1)/28)+1; 
		break;
	case 4: //day of month
		temp = (day)%28;
		if (temp)
			return temp;
		else return 28;
		break;
	}
	return 0;
}
CGameState::CGameState()
{
	mx = new boost::shared_mutex();
	applierGs = new CApplier<CBaseForGSApply>;
	registerTypes2(*applierGs);
	objCaller = new CObjectCallersHandler;
	globalEffects.description = "Global effects";
}
CGameState::~CGameState()
{
	delete mx;
	map.dellNull();
	curB.dellNull();
	//delete scenarioOps; //TODO: fix for loading ind delete
	//delete initialOpts;
	delete applierGs;
	delete objCaller;

	//TODO: delete properly that definfos
	villages.clear();
	capitols.clear();
}

BattleInfo * CGameState::setupBattle(int3 tile, const CArmedInstance *armies[2], const CGHeroInstance * heroes[2], bool creatureBank, const CGTownInstance *town)
{
	int terrain = map->getTile(tile).tertype;
	int terType = battleGetBattlefieldType(tile);
	return BattleInfo::setupBattle(tile, terrain, terType, armies, heroes, creatureBank, town);
}

void CGameState::init( StartInfo * si, ui32 checksum, int Seed )
{
	struct HLP
	{
		//it's assumed that given hero should receive the bonus
		static void giveCampaignBonusToHero(CGHeroInstance * hero, const StartInfo * si, const CScenarioTravel & st )
		{
			const CScenarioTravel::STravelBonus & curBonus = st.bonusesToChoose[si->choosenCampaignBonus];
			if(curBonus.isBonusForHero())
			{
				//apply bonus
				switch (curBonus.type)
				{
				case 0: //spell
					hero->spells.insert(curBonus.info2);
					break;
				case 1: //monster
					{
						for(int i=0; i<ARMY_SIZE; i++)
						{
							if(hero->slotEmpty(i))
							{
								hero->addToSlot(i, curBonus.info2, curBonus.info3);
								break;
							}
						}
					}
					break;
				case 3: //artifact
					hero->giveArtifact(curBonus.info2);
					break;
				case 4: //spell scroll
					{
						CArtifactInstance * scroll = CArtifactInstance::createScroll(VLC->spellh->spells[curBonus.info2]);
						scroll->putAt(hero, scroll->firstAvailableSlot(hero));
					}
					break;
				case 5: //prim skill
					{
						const ui8* ptr = reinterpret_cast<const ui8*>(&curBonus.info2);
						for (int g=0; g<PRIMARY_SKILLS; ++g)
						{
							int val = ptr[g];
							if (val == 0)
							{
								continue;
							}
							Bonus *bb = new Bonus(Bonus::PERMANENT, Bonus::PRIMARY_SKILL, Bonus::CAMPAIGN_BONUS, val, si->whichMapInCampaign, g);
							hero->addNewBonus(bb);
						}
					}
					break;
				case 6: //sec skills
					hero->setSecSkillLevel(static_cast<CGHeroInstance::SecondarySkill>(curBonus.info2), curBonus.info3, true);
					break;
				}
			}
		}

		static std::vector<const PlayerSettings *> getHumanPlayerInfo(const StartInfo * si)
		{
			std::vector<const PlayerSettings *> ret;
			for(std::map<int, PlayerSettings>::const_iterator it = si->playerInfos.begin(); 
				it != si->playerInfos.end(); ++it)
			{
				if(it->second.human)
					ret.push_back(&it->second);
			}

			return ret;
		}

		static void replaceHero( CGameState * gs, int objId, CGHeroInstance * ghi ) 
		{
			ghi->id = objId;
			gs->map->objects[objId] = ghi;
			gs->map->heroes.push_back(ghi);
		}

		//sort in descending order by power
		static bool heroPowerSorter(const CGHeroInstance * a, const CGHeroInstance * b)
		{
			return a->getHeroStrength() > b->getHeroStrength();
		}
	};


	seed = Seed;
	ran.seed((boost::int32_t)seed);
	scenarioOps = new StartInfo(*si);
	initialOpts = new StartInfo(*si);
	si = NULL;

	switch(scenarioOps->mode)
	{
	case StartInfo::NEW_GAME:
		map = new Mapa(scenarioOps->mapname);
		break;
	case StartInfo::CAMPAIGN:
		{
			campaign = new CCampaignState();
			campaign->initNewCampaign(*scenarioOps);
			assert(vstd::contains(campaign->camp->mapPieces, scenarioOps->whichMapInCampaign));

			std::string &mapContent = campaign->camp->mapPieces[scenarioOps->whichMapInCampaign];
			map = new Mapa();
			map->initFromBytes((const unsigned char*)mapContent.c_str());
		}
		break;
	case StartInfo::DUEL:
		{
			bool success = false;
			DuelParameters dp;
			try
			{
				CLoadFile lf(scenarioOps->mapname);
				lf >> dp;
				success = true;
			}
			catch(...)
			{}


			const CArmedInstance *armies[2] = {0};
			const CGHeroInstance *heroes[2] = {0};
			CGTownInstance *town = NULL;

			for(int i = 0; i < 2; i++)
			{
				CArmedInstance *obj = NULL;
				if(dp.sides[i].heroId >= 0)
				{
					CGHeroInstance *h = new CGHeroInstance();
					armies[i] = heroes[i] = h;
					obj = h;
					h->subID = dp.sides[i].heroId;
					h->initHero(h->subID);
				}
				else
				{
					CGCreature *c = new CGCreature();
					armies[i] = obj = c;
					c->subID = 34;
					
				}

				obj->initObj();
				obj->setOwner(i);

				for(int j = 0; j < ARRAY_COUNT(dp.sides[i].stacks); j++)
				{
					int cre = dp.sides[i].stacks[j].type, count = dp.sides[i].stacks[j].count;
					if(!count && obj->hasStackAtSlot(j)
						|| count)
						obj->setCreature(j, cre, count);
				}
			}

			curB = BattleInfo::setupBattle(int3(), dp.bfieldType, dp.terType, armies, heroes, false, town);
			curB->localInit();
			return;
		}
		break;
	default:
		tlog1 << "Wrong mode: " << (int)scenarioOps->mode << std::endl;
		return;
	}
	VLC->arth->initAllowedArtifactsList(map->allowedArtifact);
	tlog0 << "Map loaded!" << std::endl;


	//tlog0 <<"Reading and detecting map file (together): "<<tmh.getDif()<<std::endl;
	if(checksum)
	{
		tlog0 << "\tServer checksum for " << scenarioOps->mapname <<": "<< checksum << std::endl;
		tlog0 << "\tOur checksum for the map: "<< map->checksum << std::endl;
		if(map->checksum != checksum)
		{
			tlog1 << "Wrong map checksum!!!" << std::endl;
			throw std::string("Wrong checksum");
		}
	}

	day = 0;
	loadTownDInfos();

 	//pick grail location
 	if(map->grailPos.x < 0 || map->grailRadious) //grail not set or set within a radius around some place
 	{
		if(!map->grailRadious) //radius not given -> anywhere on map
			map->grailRadious = map->width * 2;


 		std::vector<int3> allowedPos;
 
		// add all not blocked tiles in range
 		for (int i = 0; i < map->width ; i++)
 		{
 			for (int j = 0; j < map->height ; j++)
 			{
 				for (int k = 0; k <= map->twoLevel ; k++)
 				{
 					const TerrainTile &t = map->terrain[i][j][k];
 					if(!t.blocked 
						&& !t.visitable 
						&& t.tertype != TerrainTile::water 
						&& t.tertype != TerrainTile::rock
						&& map->grailPos.dist2d(int3(i,j,k)) <= map->grailRadious)
 						allowedPos.push_back(int3(i,j,k));
 				}
 			}
 		}
 
		//remove tiles with holes
		for(unsigned int no=0; no<map->objects.size(); ++no)
			if(map->objects[no]->ID == 124)
				allowedPos -= map->objects[no]->pos;

		if(allowedPos.size())
			map->grailPos = allowedPos[ran() % allowedPos.size()];
		else
			tlog2 << "Warning: Grail cannot be placed, no appropriate tile found!\n";
 	}

	//picking random factions for players
	for(std::map<int, PlayerSettings>::iterator it = scenarioOps->playerInfos.begin(); 
		it != scenarioOps->playerInfos.end(); ++it)
	{
		if(it->second.castle==-1)
		{
			int f;
			do
			{
				f = ran()%F_NUMBER;
			}while(!(map->players[it->first].allowedFactions  &  1<<f));
			it->second.castle = f;
		}
	}

	//randomizing objects
	BOOST_FOREACH(CGObjectInstance *obj, map->objects)
	{
		randomizeObject(obj);
		obj->hoverName = VLC->generaltexth->names[obj->ID];

		//handle Favouring Winds - mark tiles under it
		if(obj->ID == 225)
			for (int i = 0; i < obj->getWidth() ; i++)
				for (int j = 0; j < obj->getHeight() ; j++)
				{
					int3 pos = obj->pos - int3(i,j,0);
					if(map->isInTheMap(pos))
						map->getTile(pos).siodmyTajemniczyBajt |= 128;
				}
	}
	//std::cout<<"\tRandomizing objects: "<<th.getDif()<<std::endl;

	/*********creating players entries in gs****************************************/
	for(std::map<int, PlayerSettings>::iterator it = scenarioOps->playerInfos.begin(); 
		it != scenarioOps->playerInfos.end(); ++it)
	{
		std::pair<int,PlayerState> ins(it->first,PlayerState());
		ins.second.color=ins.first;
		ins.second.human = it->second.human;
		ins.second.team = map->players[ins.first].team;
		teams[ins.second.team].id = ins.second.team;//init team
		teams[ins.second.team].players.insert(ins.first);//add player to team
		players.insert(ins);
	}

	/*********give starting hero****************************************/
	for(int i=0;i<PLAYER_LIMIT;i++)
	{
		const PlayerInfo &p = map->players[i];
		bool generateHero = (p.generateHeroAtMainTown && p.hasMainTown);
		if(generateHero && vstd::contains(scenarioOps->playerInfos, i))
		{
			int3 hpos = p.posOfMainTown;
			hpos.x+=1;

			int h = pickHero(i);
			if(scenarioOps->playerInfos[i].hero == -1)
				scenarioOps->playerInfos[i].hero = h;

			CGHeroInstance * nnn =  static_cast<CGHeroInstance*>(createObject(HEROI_TYPE,h,hpos,i));
			nnn->id = map->objects.size();
			nnn->initHero();
			map->heroes.push_back(nnn);
			map->objects.push_back(nnn);
			map->addBlockVisTiles(nnn);
		}
	}

	/*************************replace hero placeholders*****************************/

	if (campaign)
	{

		CScenarioTravel::STravelBonus bonus =
			campaign->camp->scenarios[scenarioOps->whichMapInCampaign].travelOptions.bonusesToChoose[scenarioOps->choosenCampaignBonus];


		std::vector<CGHeroInstance *> Xheroes;
		if (bonus.type == 8) //hero crossover
		{
			Xheroes = campaign->camp->scenarios[bonus.info2].crossoverHeroes;
		}

		//selecting heroes by type
		for(int g=0; g<map->objects.size(); ++g)
		{
			CGObjectInstance * obj = map->objects[g];
			if (obj->ID != 214) //not a placeholder
			{
				continue;
			}
			CGHeroPlaceholder * hp = static_cast<CGHeroPlaceholder*>(obj);

			if(hp->subID != 0xFF) //select by type
			{
				bool found = false;
				BOOST_FOREACH(CGHeroInstance * ghi, Xheroes)
				{
					if (ghi->subID == hp->subID)
					{
						found = true;
						HLP::replaceHero(this, g, ghi);
						Xheroes -= ghi;
						break;
					}
				}
				if (!found)
				{
					//TODO: create new hero of this type
				}
			}
		}

		//selecting heroes by power

		std::sort(Xheroes.begin(), Xheroes.end(), HLP::heroPowerSorter);
		for(int g=0; g<map->objects.size(); ++g)
		{
			CGObjectInstance * obj = map->objects[g];
			if (obj->ID != 214) //not a placeholder
			{
				continue;
			}
			CGHeroPlaceholder * hp = static_cast<CGHeroPlaceholder*>(obj);

			if (hp->subID == 0xFF) //select by power
			{
				if(Xheroes.size() > hp->power - 1)
					HLP::replaceHero(this, g, Xheroes[hp->power - 1]);
				else
					tlog2 << "Warning, to hero to replace!\n";
				//we don't have to remove hero from Xheroes because it would destroy the order and duplicates shouldn't happen
			}
		}
	}

	/******************RESOURCES****************************************************/
	std::vector<int> startresAI, startresHuman;
	std::ifstream tis(DATA_DIR "/config/startres.txt");
	int k;
	for (int j=0; j<scenarioOps->difficulty * 2; j++)
	{
		tis >> k;
		for (int z=0;z<RESOURCE_QUANTITY;z++)
			tis>>k;
	}
	tis >> k;
	for (int i=0; i<RESOURCE_QUANTITY; i++)
	{
		tis >> k;
		startresHuman.push_back(k);
	}
	tis >> k;
	for (int i=0; i<RESOURCE_QUANTITY; i++)
	{
		tis >> k;
		startresAI.push_back(k);
	}
	tis.close();
	tis.clear();
	for (std::map<ui8,PlayerState>::iterator i = players.begin(); i!=players.end(); i++)
	{
		(*i).second.resources.resize(RESOURCE_QUANTITY);
		for (int x=0;x<RESOURCE_QUANTITY;x++)
		{
			if (i->second.human)
			{
				(*i).second.resources[x] = startresHuman[x];
			}
			else
			{
				(*i).second.resources[x] = startresAI[x];
			}
		}
	}

	//give start resource bonus in case of campaign
	if (scenarioOps->mode == StartInfo::CAMPAIGN)
	{
		CScenarioTravel::STravelBonus chosenBonus = 
			campaign->camp->scenarios[scenarioOps->whichMapInCampaign].travelOptions.bonusesToChoose[scenarioOps->choosenCampaignBonus];
		if(chosenBonus.type == 7) //resource
		{
			std::vector<const PlayerSettings *> people = HLP::getHumanPlayerInfo(scenarioOps); //players we will give resource bonus
			for (int b=0; b<people.size(); ++b)
			{
				std::vector<int> res; //resources we will give
				switch (chosenBonus.info1)
				{
				case 0: case 1: case 2: case 3: case 4: case 5: case 6:
					res.push_back(chosenBonus.info1);
					break;
				case 0xFD: //wood+ore
					res.push_back(0); res.push_back(2);
					break;
				case 0xFE:  //rare
					res.push_back(1); res.push_back(3); res.push_back(4); res.push_back(5);
					break;
				default:
					assert(0);
					break;
				}
				//increasing resource quantity
				for (int n=0; n<res.size(); ++n)
				{
					players[people[b]->color].resources[res[n]] += chosenBonus.info2;
				}
			}
		}
	}


	/*************************HEROES************************************************/
	std::set<int> hids; //hero ids to create pool

	for(unsigned int i=0; i<map->allowedHeroes.size(); i++) //add to hids all allowed heroes
		if(map->allowedHeroes[i])
			hids.insert(i);

	for (unsigned int i=0; i<map->heroes.size();i++) //heroes instances initialization
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

	for (unsigned int i=0; i<map->objects.size();i++) //prisons
	{
		if (map->objects[i]->ID == 62)
			hids.erase(map->objects[i]->subID);
	}

	for(unsigned int i=0; i<map->predefinedHeroes.size(); i++)
	{
		if(!vstd::contains(hids,map->predefinedHeroes[i]->subID))
			continue;
		map->predefinedHeroes[i]->initHero();
		hpool.heroesPool[map->predefinedHeroes[i]->subID] = map->predefinedHeroes[i];
		hpool.pavailable[map->predefinedHeroes[i]->subID] = 0xff;
		hids.erase(map->predefinedHeroes[i]->subID);
	}

	BOOST_FOREACH(int hid, hids) //all not used allowed heroes go with default state into the pool
	{
		CGHeroInstance * vhi = new CGHeroInstance();
		vhi->initHero(hid);
		hpool.heroesPool[hid] = vhi;
		hpool.pavailable[hid] = 0xff;
	}

	for(unsigned int i=0; i<map->disposedHeroes.size(); i++)
	{
		hpool.pavailable[map->disposedHeroes[i].ID] = map->disposedHeroes[i].players;
	}

	if (scenarioOps->mode == StartInfo::CAMPAIGN) //give campaign bonuses for specific / best hero
	{

		CScenarioTravel::STravelBonus chosenBonus = 
			campaign->camp->scenarios[scenarioOps->whichMapInCampaign].travelOptions.bonusesToChoose[scenarioOps->choosenCampaignBonus];
		if (chosenBonus.isBonusForHero() && chosenBonus.info1 != 0xFFFE) //exclude generated heroes
		{
			//find human player
			int humanPlayer;
			for (std::map<ui8, PlayerState>::iterator it=players.begin(); it != players.end(); ++it)
			{
				if(it->second.human)
				{
					humanPlayer = it->first;
					break;
				}
			}
			std::vector<ConstTransitivePtr<CGHeroInstance> > & heroes = players[humanPlayer].heroes;

			if (chosenBonus.info1 == 0xFFFD) //most powerful
			{
				int maxB = -1;
				for (int b=0; b<heroes.size(); ++b)
				{
					if (maxB == -1 || heroes[b]->getTotalStrength() > heroes[maxB]->getTotalStrength())
					{
						maxB = b;
					}
				}
				if(maxB < 0)
					tlog2 << "Warning - cannot give bonus to hero cause there are no heroes!\n";
				else
					HLP::giveCampaignBonusToHero(heroes[maxB], scenarioOps, campaign->camp->scenarios[scenarioOps->whichMapInCampaign].travelOptions);
			}
			else //specific hero
			{
				for (int b=0; b<heroes.size(); ++b)
				{
					if (heroes[b]->subID == chosenBonus.info1)
					{
						HLP::giveCampaignBonusToHero(heroes[b], scenarioOps, campaign->camp->scenarios[scenarioOps->whichMapInCampaign].travelOptions);
						break;
					}
				}
			}
		}
	}

	/*************************FOG**OF**WAR******************************************/
	for(std::map<ui8, TeamState>::iterator k=teams.begin(); k!=teams.end(); ++k)
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
			if( !vstd::contains(k->second.players, obj->tempOwner)) continue; //not a flagged object

			boost::unordered_set<int3, ShashInt3> tiles;
			obj->getSightTiles(tiles);
			BOOST_FOREACH(int3 tile, tiles)
			{
				k->second.fogOfWarMap[tile.x][tile.y][tile.z] = 1;
			}
		}
	}
	
	for(std::map<ui8, PlayerState>::iterator k=players.begin(); k!=players.end(); ++k)
	{
		//starting bonus
		if(scenarioOps->playerInfos[k->first].bonus==PlayerSettings::brandom)
			scenarioOps->playerInfos[k->first].bonus = ran()%3;
		switch(scenarioOps->playerInfos[k->first].bonus)
		{
		case PlayerSettings::bgold:
			k->second.resources[6] += 500 + (ran()%6)*100;
			break;
		case PlayerSettings::bresource:
			{
				int res = VLC->townh->towns[scenarioOps->playerInfos[k->first].castle].primaryRes;
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
		case PlayerSettings::bartifact:
			{
 				if(!k->second.heroes.size())
				{
					tlog5 << "Cannot give starting artifact - no heroes!" << std::endl;
					break;
				}
 				CArtifact *toGive;
 				toGive = VLC->arth->artifacts[VLC->arth->getRandomArt (CArtifact::ART_TREASURE)];

				CGHeroInstance *hero = k->second.heroes[0];
 				hero->giveArtifact(toGive->id);
			}
		}
	}
	/****************************TOWNS************************************************/
	for ( int i=0; i<4; i++)
		CGTownInstance::universitySkills.push_back(14+i);//skills for university
	
	for (unsigned int i=0;i<map->towns.size();i++)
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
		
		if (vstd::contains(vti->builtBuildings,(6)) && vti->state()==2)
			vti->builtBuildings.erase(6);//if we have harbor without water - erase it (this is H3 behaviour)
	
		//init hordes
		for (int i = 0; i<CREATURES_PER_TOWN; i++)
			if (vstd::contains(vti->builtBuildings,(-31-i))) //if we have horde for this level
			{
				vti->builtBuildings.erase(-31-i);//remove old ID
				if (vti->town->hordeLvl[0] == i)//if town first horde is this one
				{
					vti->builtBuildings.insert(18);//add it
					if (vstd::contains(vti->builtBuildings,(37+i)))//if we have upgraded dwelling as well
						vti->builtBuildings.insert(19);//add it as well
				}
				if (vti->town->hordeLvl[1] == i)//if town second horde is this one
				{
					vti->builtBuildings.insert(24);
					if (vstd::contains(vti->builtBuildings,(37+i)))
						vti->builtBuildings.insert(25);
				}
			}
			
		for (std::list<CCastleEvent*>::iterator ev=vti->events.begin(); ev!=vti->events.end(); ev++)
			for (int i = 0; i<CREATURES_PER_TOWN; i++)
				if (vstd::contains((*ev)->buildings,(-31-i))) //if we have horde for this level
				{
					(*ev)->buildings.erase(-31-i);
					if (vti->town->hordeLvl[0] == i)
						(*ev)->buildings.insert(18);
					if (vti->town->hordeLvl[1] == i)
						(*ev)->buildings.insert(24);
				}
		//init spells
		vti->spells.resize(SPELL_LEVELS);
		CSpell *s;
		for(unsigned int z=0; z<vti->obligatorySpells.size();z++)
		{
			s = VLC->spellh->spells[vti->obligatorySpells[z]];
			vti->spells[s->level-1].push_back(s->id);
			vti->possibleSpells -= s->id;
		}
		while(vti->possibleSpells.size())
		{
			ui32 total=0;
			int sel = -1;

			for(unsigned int ps=0;ps<vti->possibleSpells.size();ps++)
				total += VLC->spellh->spells[vti->possibleSpells[ps]]->probabilities[vti->subID];
			int r = (total)? ran()%total : -1;
			for(unsigned int ps=0; ps<vti->possibleSpells.size();ps++)
			{
				r -= VLC->spellh->spells[vti->possibleSpells[ps]]->probabilities[vti->subID];
				if(r<0)
				{
					sel = ps;
					break;
				}
			}
			if(sel<0)
				sel=0;

			CSpell *s = VLC->spellh->spells[vti->possibleSpells[sel]];
			vti->spells[s->level-1].push_back(s->id);
			vti->possibleSpells -= s->id;
		}
		if(vti->getOwner() != 255)
			getPlayer(vti->getOwner())->towns.push_back(vti);
	}

	//campaign bonuses for towns
	if (scenarioOps->mode == StartInfo::CAMPAIGN)
	{
		CScenarioTravel::STravelBonus chosenBonus = 
			campaign->camp->scenarios[scenarioOps->whichMapInCampaign].travelOptions.bonusesToChoose[scenarioOps->choosenCampaignBonus];

		if (chosenBonus.type == 2)
		{
			for (int g=0; g<map->towns.size(); ++g)
			{
				PlayerState * owner = getPlayer(map->towns[g]->getOwner());
				PlayerInfo & pi = map->players[owner->color];
				
				if (owner->human && //human-owned
					map->towns[g]->pos == pi.posOfMainTown + int3(2, 0, 0)) 
				{
					map->towns[g]->builtBuildings.insert(
						CBuildingHandler::campToERMU(chosenBonus.info1, map->towns[g]->town->typeID, map->towns[g]->builtBuildings));
					break;
				}
			}
		}

	}

	objCaller->preInit();
	BOOST_FOREACH(CGObjectInstance *obj, map->objects)
	{
		obj->initObj();
		if(obj->ID == 62) //prison also needs to initialize hero
			static_cast<CGHeroInstance*>(obj)->initHero();
	}
	CGTeleport::postInit(); //pairing subterranean gates

	buildBonusSystemTree();

	for(std::map<ui8, PlayerState>::iterator k=players.begin(); k!=players.end(); ++k)
	{
		if(k->first==-1 || k->first==255)
			continue;

		//init visiting and garrisoned heroes
		BOOST_FOREACH(CGHeroInstance *h, k->second.heroes)
		{ 
			BOOST_FOREACH(CGTownInstance *t, k->second.towns)
			{
				int3 vistile = t->pos; vistile.x--; //tile next to the entrance
				if(vistile == h->pos || h->pos==t->pos)
				{
					t->setVisitingHero(h);
					if(h->pos == t->pos) //visiting hero placed in the editor has same pos as the town - we need to correct it
					{
						map->removeBlockVisTiles(h);
						h->pos.x -= 1;
						map->addBlockVisTiles(h);
					}
					break;
				}
			}
		}
	}
}

int CGameState::battleGetBattlefieldType(int3 tile)
{
	if(tile==int3() && curB)
		tile = curB->tile;
	else if(tile==int3() && !curB)
		return -1;

	const TerrainTile &t = map->getTile(tile);
	//fight in mine -> subterranean
	if(dynamic_cast<const CGMine *>(t.visitableObjects.front()))
		return 12;

	const std::vector <ConstTransitivePtr<CGObjectInstance> > & objs = map->objects;
	for(int g=0; g<objs.size(); ++g)
	{
		if( !objs[g] || objs[g]->pos.x - tile.x < 0  ||  objs[g]->pos.x - tile.x >= 8  
			||  tile.y - objs[g]->pos.y + 5 < 0  ||  tile.y - objs[g]->pos.y + 5 >=6 
			|| !objs[g]->coveringAt(objs[g]->pos.x - tile.x, tile.y - objs[g]->pos.y + 5)
			) //look only for objects covering given tile
			continue;
		switch(objs[g]->ID)
		{
		case 222: //clover field
			return 19;
		case 21: case 223: //cursed ground
			return 22;
		case 224: //evil fog
			return 20;
		case 225: //favourable winds
			return 21;
		case 226: //fiery fields
			return 14;
		case 227: //holy ground
			return 18;
		case 228: //lucid pools
			return 17;
		case 229: //magic clouds
			return 16;
		case 46: case 230: //magic plains
			return 9;
		case 231: //rocklands
			return 15;
		}
	}

	switch(t.tertype)
	{
	case TerrainTile::dirt:
		return rand()%3+3;
	case TerrainTile::sand:
		return 2; //TODO: coast support
	case TerrainTile::grass:
		return rand()%2+6;
	case TerrainTile::snow:
		return rand()%2+10;
	case TerrainTile::swamp:
		return 13;
	case TerrainTile::rough:
		return 23;
	case TerrainTile::subterranean:
		return 12;
	case TerrainTile::lava:
		return 8;
	case TerrainTile::water:
		return 25;
	case TerrainTile::rock:
		return 15;
	default:
		return -1;
	}
}

std::set<std::pair<int, int> > costDiff(const std::vector<ui32> &a, const std::vector<ui32> &b, const int modifier = 100) //modifer %
{
	std::set<std::pair<int, int> > ret;
	for(int j=0;j<RESOURCE_QUANTITY;j++)
	{
		assert(a[j] >= b[j]);
		if(int dif = modifier * (a[j] - b[j]) / 100)
			ret.insert(std::make_pair(j,dif));
	}
	return ret;
}

UpgradeInfo CGameState::getUpgradeInfo(const CStackInstance &stack)
{
	UpgradeInfo ret;
	const CCreature *base = stack.type;

	const CGHeroInstance *h = stack.armyObj->ID == HEROI_TYPE ? static_cast<const CGHeroInstance*>(stack.armyObj) : NULL;
	const CGTownInstance *t = NULL;

	if(stack.armyObj->ID == TOWNI_TYPE)
		t = static_cast<const CGTownInstance *>(stack.armyObj);
	else if(h)
	{	//hero speciality
		BonusList lista = h->speciality.getBonuses(Selector::typeSybtype(Bonus::SPECIAL_UPGRADE, base->idNumber));
		BOOST_FOREACH(const Bonus *it, lista)
		{
			ui16 nid = it->additionalInfo;
			if (nid != base->idNumber) //in very specific case the upgrade is avaliable by default (?)
			{
				ret.newID.push_back(nid);
				ret.cost.push_back(costDiff(VLC->creh->creatures[nid]->cost, base->cost));
			}
		}
		t = h->visitedTown;
	}
	if(t)
	{
		BOOST_FOREACH(si32 bid, t->builtBuildings)
		{
			if( bid >= 37   &&   bid < 44 ) //upgraded creature dwelling
			{
				int nid = t->town->upgradedCreatures[bid-37]; //upgrade offered by that building
				if(vstd::contains(base->upgrades, nid)) //possible upgrade
				{
					ret.newID.push_back(nid);
					ret.cost.push_back(costDiff(VLC->creh->creatures[nid]->cost, base->cost));
				}
			}
		}
	}

	//hero is visiting Hill Fort
	if(h && map->getTile(h->visitablePos()).visitableObjects.front()->ID == 35) 
	{
		static const int costModifiers[] = {0, 25, 50, 75, 100}; //we get cheaper upgrades depending on level
		const int costModifier = costModifiers[std::min<int>(std::max((int)base->level - 1, 0), ARRAY_COUNT(costModifiers) - 1)];

		BOOST_FOREACH(si32 nid, base->upgrades)
		{
			ret.newID.push_back(nid);
			ret.cost.push_back(costDiff(VLC->creh->creatures[nid]->cost, base->cost, costModifier));
		}
	}

	if(ret.newID.size())
		ret.oldID = base->idNumber;

	return ret;
}

int CGameState::getPlayerRelations( ui8 color1, ui8 color2 )
{
	if ( color1 == color2 )
		return 2;
	const TeamState * ts = getPlayerTeam(color1);
	if (ts && vstd::contains(ts->players, color2))
		return 1;
	return 0;
}

void CGameState::loadTownDInfos()
{
	for(int i=0;i<F_NUMBER;i++)
	{
		villages[i] = new CGDefInfo(*VLC->dobjinfo->castles[i]);
		forts[i] = VLC->dobjinfo->castles[i];
		map->defy.push_back(forts[i]);
		capitols[i] = new CGDefInfo(*VLC->dobjinfo->castles[i]);
	}

	std::ifstream ifs(DATA_DIR "/config/townsDefs.txt");
	int ccc;
	ifs>>ccc;
	for(int i=0; i < ccc*2; i++)
	{
		CGDefInfo *n;
		if(i<ccc)
			n = villages[i];
		else 
			n = capitols[i%ccc];

		ifs >> n->name;
		map->defy.push_back(n);
	}
}

void CGameState::getNeighbours(const TerrainTile &srct, int3 tile, std::vector<int3> &vec, const boost::logic::tribool &onLand, bool limitCoastSailing)
{
	static int3 dirs[] = { int3(0,1,0),int3(0,-1,0),int3(-1,0,0),int3(+1,0,0),
					int3(1,1,0),int3(-1,1,0),int3(1,-1,0),int3(-1,-1,0) };

	vec.clear();
	for (size_t i = 0; i < ARRAY_COUNT(dirs); i++)
	{
		const int3 hlp = tile + dirs[i];
		if(!map->isInTheMap(hlp)) 
			continue;

		const TerrainTile &hlpt = map->getTile(hlp);

		//we cannot visit things from blocked tiles
		if(srct.blocked && !srct.visitable && hlpt.visitable && srct.blockingObjects.front()->ID != HEROI_TYPE)
		{
			continue;
		}

		if(srct.tertype == TerrainTile::water && limitCoastSailing && hlpt.tertype == TerrainTile::water && dirs[i].x && dirs[i].y) //diagonal move through water
		{
			int3 hlp1 = tile,
				hlp2 = tile;
			hlp1.x += dirs[i].x;
			hlp2.y += dirs[i].y;

			if(map->getTile(hlp1).tertype != TerrainTile::water || map->getTile(hlp2).tertype != TerrainTile::water)
				continue;
		}

		if((indeterminate(onLand)  ||  onLand == (hlpt.tertype!=TerrainTile::water) ) 
			&& hlpt.tertype != TerrainTile::rock) 
		{
			vec.push_back(hlp);
		}
	}
}

int CGameState::getMovementCost(const CGHeroInstance *h, const int3 &src, const int3 &dest, int remainingMovePoints, bool checkLast)
{
	if(src == dest) //same tile
		return 0;

	TerrainTile &s = map->terrain[src.x][src.y][src.z],
		&d = map->terrain[dest.x][dest.y][dest.z];

	//get basic cost
	int ret = h->getTileCost(d,s);

	if(d.blocked && h->hasBonusOfType(Bonus::FLYING_MOVEMENT))
	{
		bool freeFlying = h->getBonusesCount(Selector::typeSybtype(Bonus::FLYING_MOVEMENT, 1)) > 0;

		if(!freeFlying)
		{
			ret *= 1.4f; //40% penalty for movement over blocked tile
		}
	}
	else if (d.tertype == TerrainTile::water)
	{
		if(h->boat && s.siodmyTajemniczyBajt & 128 && d.siodmyTajemniczyBajt & 128) //Favourable Winds
			ret *= 0.666f;
		else if (!h->boat && h->getBonusesCount(Selector::typeSybtype(Bonus::WATER_WALKING, 1)) > 0)
			ret *= 1.4f; //40% penalty for water walking
	}

	if(src.x != dest.x  &&  src.y != dest.y) //it's diagonal move
	{
		int old = ret;
		ret *= 1.414213;
		//diagonal move costs too much but normal move is possible - allow diagonal move for remaining move points
		if(ret > remainingMovePoints  &&  remainingMovePoints >= old)
		{
			return remainingMovePoints;
		}
	}


	int left = remainingMovePoints-ret;
	if(checkLast  &&  left > 0  &&  remainingMovePoints-ret < 250) //it might be the last tile - if no further move possible we take all move points
	{
		std::vector<int3> vec;
		getNeighbours(d, dest, vec, s.tertype != TerrainTile::water, true);
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

std::set<int> CGameState::getBuildingRequiments(const CGTownInstance *t, int ID)
{
	std::set<int> used;
	used.insert(ID);
	std::set<int> reqs = VLC->townh->requirements[t->subID][ID];

	while(true)
	{
		size_t noloop=0;
		for(std::set<int>::iterator i=reqs.begin();i!=reqs.end();i++)
		{
			if(used.find(*i)==used.end()) //we haven't added requirements for this building
			{
				used.insert(*i);
				for(
					std::set<int>::iterator j=VLC->townh->requirements[t->subID][*i].begin();
					j!=VLC->townh->requirements[t->subID][*i].end();
					j++)
					{
						reqs.insert(*j);//creating full list of requirements
					}
			}
			else
			{
				noloop++;
			}
		}
		if(noloop==reqs.size())
			break;
	}
	return reqs;
}

int CGameState::canBuildStructure( const CGTownInstance *t, int ID )
{
	int ret = Buildings::ALLOWED;

	if(t->builded >= MAX_BUILDING_PER_TURN)
		ret = Buildings::CANT_BUILD_TODAY; //building limit

	CBuilding * pom = VLC->buildh->buildings[t->subID][ID];
	
	if(!pom)
		return Buildings::ERROR;

	//checking resources
	for(int res=0; res<RESOURCE_QUANTITY; res++)
	{
		if(pom->resources[res] > getPlayer(t->tempOwner)->resources[res])
			ret = Buildings::NO_RESOURCES; //lack of res
	}

	//checking for requirements
	std::set<int> reqs = getBuildingRequiments(t, ID);//getting all requiments

	for( std::set<int>::iterator ri  =  reqs.begin(); ri != reqs.end(); ri++ )
	{
		if(t->builtBuildings.find(*ri)==t->builtBuildings.end())
			ret = Buildings::PREREQUIRES; //lack of requirements - cannot build
	}

	//can we build it?
	if(t->forbiddenBuildings.find(ID)!=t->forbiddenBuildings.end())
		ret = Buildings::FORBIDDEN; //forbidden

	if(ID == 13) //capitol
	{
		for(unsigned int in = 0; in < map->towns.size(); in++)
		{
			if(map->towns[in]->tempOwner==t->tempOwner  &&  vstd::contains(map->towns[in]->builtBuildings,13))
			{
				ret = Buildings::HAVE_CAPITAL; //no more than one capitol
				break;
			}
		}
	}
	else if(ID == 6) //shipyard
	{
		int3 tile = t->bestLocation();
		if( !map->isInTheMap(tile) || map->getTile(tile).tertype != TerrainTile::water )
			ret = Buildings::NO_WATER; //lack of water
	}

	if(t->builtBuildings.find(ID)!=t->builtBuildings.end())	//already built
		ret = Buildings::ALREADY_PRESENT;
	return ret;
}

void CGameState::apply(CPack *pack)
{
	ui16 typ = typeList.getTypeID(pack);
	applierGs->apps[typ]->applyOnGS(this,pack);
}

TeamState *CGameState::getTeam(ui8 teamID)
{
	if(vstd::contains(teams,teamID))
	{
		return &teams[teamID];
	}
	else 
	{
		tlog2 << "Warning: Cannot find info for team " << int(teamID) << std::endl;
		return NULL;
	}
	
}

TeamState *CGameState::getPlayerTeam(ui8 color)
{
	PlayerState * ps = getPlayer(color);
	if (ps)
		return getTeam(ps->team);
	return NULL;
}

PlayerState * CGameState::getPlayer( ui8 color, bool verbose )
{
	if(vstd::contains(players,color))
	{
		return &players[color];
	}
	else 
	{
		if(verbose)
			tlog2 << "Warning: Cannot find info for player " << int(color) << std::endl;
		return NULL;
	}
}

const PlayerState * CGameState::getPlayer( ui8 color, bool verbose ) const
{
	return (const_cast<CGameState *>(this))->getPlayer(color, verbose);
}


const TeamState * CGameState::getTeam( ui8 teamID ) const
{
	return (const_cast<CGameState *>(this))->getTeam(teamID);
}

const TeamState * CGameState::getPlayerTeam( ui8 teamID ) const
{
	return (const_cast<CGameState *>(this))->getPlayerTeam(teamID);
}

bool CGameState::getPath(int3 src, int3 dest, const CGHeroInstance * hero, CPath &ret)
{
	if(!map->isInTheMap(src) || !map->isInTheMap(dest)) //check input
		return false;

	int3 hpos = hero->getPosition(false);

	bool flying = false; //hero is under flying effect	TODO
	bool waterWalking = false; //hero is on land and can walk on water TODO
	bool onLand = map->getTile(hpos).tertype != TerrainTile::water;
// 	tribool blockLandSea; //true - blocks sea, false - blocks land, indeterminate - allows all
// 
// 	if (!hero->canWalkOnSea())
// 		blockLandSea = (map->getTile(hpos).tertype != TerrainTile::water); //block land if hero is on water and vice versa
// 	else
// 		blockLandSea = boost::logic::indeterminate;

	const std::vector<std::vector<std::vector<ui8> > > &FoW = getPlayerTeam(hero->tempOwner)->fogOfWarMap;

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

			node.accessible = !tinfo->blocked;
			node.dist = -1;
			node.theNodeBefore = NULL;
			node.visited = false;
			node.coord.x = i;
			node.coord.y = j;
			node.coord.z = dest.z;

			if(!tinfo->entrableTerrain(onLand, flying || waterWalking)
				|| !FoW[i][j][src.z] //tile is covered by the FoW
			)
			{
				node.accessible = false;
			}
		}
	}


	//Special rules for the destination tile
	{
		const TerrainTile *t = &map->terrain[dest.x][dest.y][dest.z];
		CPathNode &d = graph[dest.x][dest.y];

		//tile may be blocked by blockvis / normal vis obj but it still must be accessible
		if(t->visitable) 
		{
			d.accessible = true; //for allowing visiting objects
		}

		if(onLand && t->tertype == TerrainTile::water) //hero can walk only on land and dst lays on the water
		{
			size_t i = 0;
			for(; i < t->visitableObjects.size(); i++)
				if(t->visitableObjects[i]->ID == 8  ||  t->visitableObjects[i]->ID == HEROI_TYPE) //it's a Boat
					break;

			d.accessible = (i < t->visitableObjects.size()); //dest is accessible only if there is boat/hero
		}
		else if(!onLand && t->tertype != TerrainTile::water) //hero is moving by water
		{
			d.accessible = (t->siodmyTajemniczyBajt & 64) && !t->blocked; //tile is accessible if it's coastal and not blocked
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
		getNeighbours(map->getTile(cp.coord), cp.coord, neighbours, boost::logic::indeterminate, true);
		for(unsigned int i=0; i < neighbours.size(); i++)
		{
			CPathNode & dp = graph[neighbours[i].x][neighbours[i].y];
			if(dp.accessible)
			{
				int cost = getMovementCost(hero,cp.coord,dp.coord,hero->movement - cp.dist);
				if((dp.dist==-1 || (dp.dist > cp.dist + cost)) && dp.accessible && checkForVisitableDir(cp.coord, dp.coord) && checkForVisitableDir(dp.coord, cp.coord))
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
		return false;


	//fill ret with found path
	ret.nodes.clear();
	while(curNode->coord != graph[src.x][src.y].coord)
	{
		ret.nodes.push_back(*curNode);
		curNode = curNode->theNodeBefore;
	}
	ret.nodes.push_back(graph[src.x][src.y]);

	return true;
}

void CGameState::calculatePaths(const CGHeroInstance *hero, CPathsInfo &out, int3 src, int movement)
{
	assert(hero);
	boost::shared_lock<boost::shared_mutex> lock(*mx);
	if(src.x < 0)
		src = hero->getPosition(false);
	if(movement < 0)
		movement = hero->movement;

	out.hero = hero;
	out.hpos = src;

	if(!map->isInTheMap(src)/* || !map->isInTheMap(dest)*/) //check input
	{
		tlog1 << "CGameState::calculatePaths: Hero outside the map? How dare you...\n";
		return;
	}

	tribool onLand; //true - blocks sea, false - blocks land, indeterminate - allows all

	if (!hero->canWalkOnSea())
		onLand = (map->getTile(src).tertype != TerrainTile::water); //block land if hero is on water and vice versa
	else
		onLand = boost::logic::indeterminate;

	const std::vector<std::vector<std::vector<ui8> > > &FoW = getPlayerTeam(hero->tempOwner)->fogOfWarMap;

	bool flying = hero->hasBonusOfType(Bonus::FLYING_MOVEMENT);
	bool waterWalk = hero->hasBonusOfType(Bonus::WATER_WALKING);

	//graph initialization
	CGPathNode ***graph = out.nodes;
	for(size_t i=0; i < out.sizes.x; ++i)
	{
		for(size_t j=0; j < out.sizes.y; ++j)
		{
			for(size_t k=0; k < out.sizes.z; ++k)
			{
				const TerrainTile *tinfo = &map->terrain[i][j][k];
				CGPathNode &node = graph[i][j][k];

				node.accessible = (tinfo->blocked ? CGPathNode::FLYABLE : CGPathNode::ACCESSIBLE);
				if(!flying && node.accessible == CGPathNode::FLYABLE)
				{
					node.accessible = CGPathNode::BLOCKED;
				}
				node.turns = 0xff;
				node.moveRemains = 0;
				node.coord.x = i;
				node.coord.y = j;
				node.coord.z = k;
				node.land = tinfo->tertype != TerrainTile::water;
				node.theNodeBefore = NULL;

				bool leaveAsBlocked = false;

				if((onLand || indeterminate(onLand)) && !node.land)//it's sea and we cannot walk on sea
				{
					if (waterWalk || flying)
					{
						node.accessible = CGPathNode::FLYABLE;
					}
					else
					{
						node.accessible = CGPathNode::BLOCKED;
						leaveAsBlocked = true;
					}
				}

				if ( tinfo->tertype == TerrainTile::rock//it's rock
					|| (!onLand && node.land)		//it's land and we cannot walk on land (complementary condition is handled above)
					|| !FoW[i][j][k]					//tile is covered by the FoW
					|| leaveAsBlocked
				)
				{
					node.accessible = CGPathNode::BLOCKED;
				}
				else if(tinfo->visitable)
				{
					//hero is protected in Sanctuary
					if(tinfo->visitableObjects.front()->ID == 80 && tinfo->visitableObjects.back()->ID == HEROI_TYPE && tinfo->visitableObjects.back()->tempOwner != hero->tempOwner)
						node.accessible = CGPathNode::BLOCKED;
					else
					{
						for(size_t ii = 0; ii < tinfo->visitableObjects.size(); ii++)
						{
							const CGObjectInstance * const obj = tinfo->visitableObjects[ii];
							if(obj->getPassableness() & 1<<hero->tempOwner) //special object instance specific passableness flag - overwrites other accessibility flags
							{
								node.accessible = CGPathNode::ACCESSIBLE;
							}
							else if(obj->blockVisit)
							{
								node.accessible = CGPathNode::BLOCKVIS;
								break;
							}
							else if(obj->ID != EVENTI_TYPE) //pathfinder should ignore placed events
							{
								node.accessible = CGPathNode::VISITABLE;
							}
						}
					}
				}
				else if (map->isInTheMap(guardingCreaturePosition(int3(i, j, k)))
					&& tinfo->blockingObjects.size() == 0)
				{
					// Monster close by; blocked visit for battle.
					node.accessible = CGPathNode::BLOCKVIS;
				}

				if(onLand && !node.land) //hero can walk only on land and tile lays on the water
				{
					size_t i = 0;
					for(; i < tinfo->visitableObjects.size(); i++)
						if(tinfo->visitableObjects[i]->ID == 8  ||  tinfo->visitableObjects[i]->ID == HEROI_TYPE) //it's a Boat
							break;
					if(i < tinfo->visitableObjects.size())
						node.accessible = CGPathNode::BLOCKVIS; //dest is accessible only if there is boat/hero
				}
				else if(!onLand && tinfo->tertype != TerrainTile::water) //hero is moving by water
				{
					if((tinfo->siodmyTajemniczyBajt & 64) && !tinfo->blocked)
						node.accessible = CGPathNode::VISITABLE; //tile is accessible if it's coastal and not blocked
				}
			}
		}
	}
	//graph initialized


	//initial tile - set cost on 0 and add to the queue
	graph[src.x][src.y][src.z].turns = 0; 
	graph[src.x][src.y][src.z].moveRemains = movement;
	std::queue<CGPathNode*> mq;
	mq.push(&graph[src.x][src.y][src.z]);

	//ui32 curDist = 0xffffffff; //total cost of path - init with max possible val

	std::vector<int3> neighbours;
	neighbours.reserve(8);

	while(!mq.empty())
	{
		CGPathNode *cp = mq.front();
		mq.pop();

		const int3 guardPosition = guardingCreaturePosition(cp->coord);
		const bool guardedPosition = (guardPosition != int3(-1, -1, -1) && cp->coord != src);
		const TerrainTile &ct = map->getTile(cp->coord);
		int movement = cp->moveRemains, turn = cp->turns;
		if(!movement)
		{
			movement = hero->maxMovePoints(ct.tertype != TerrainTile::water);
			turn++;
		}

		//add accessible neighbouring nodes to the queue
		getNeighbours(ct, cp->coord, neighbours, boost::logic::indeterminate, !onLand);
		for(unsigned int i=0; i < neighbours.size(); i++)
		{
			int moveAtNextTile = movement;
			int turnAtNextTile = turn;

			const int3 &n = neighbours[i]; //current neighbor
			CGPathNode & dp = graph[n.x][n.y][n.z];
			if( !checkForVisitableDir(cp->coord, dp.coord) 
				|| !checkForVisitableDir(dp.coord, cp->coord)
				|| dp.accessible == CGPathNode::BLOCKED )
			{
				continue;
			}

			int cost = getMovementCost(hero, cp->coord, dp.coord, movement);
			int remains = movement - cost;

			if(remains < 0)
			{
				//occurs rarely, when hero with low movepoints tries to go leave the road
				turnAtNextTile++;
				moveAtNextTile = hero->maxMovePoints(ct.tertype != TerrainTile::water);
				cost = getMovementCost(hero, cp->coord, dp.coord, moveAtNextTile); //cost must be updated, movement points changed :(
				remains = moveAtNextTile - cost;
			}

			const bool neighborIsGuard = guardingCreaturePosition(cp->coord) == dp.coord;

			if((dp.turns==0xff		//we haven't been here before
				|| dp.turns > turnAtNextTile
				|| (dp.turns >= turnAtNextTile  &&  dp.moveRemains < remains)) //this route is faster
				&& (!guardedPosition || neighborIsGuard)) // Can step into tile of guard
			{
				
				assert(&dp != cp->theNodeBefore); //two tiles can't point to each other
				dp.moveRemains = remains;
				dp.turns = turnAtNextTile;
				dp.theNodeBefore = cp;

				const bool guardedNeighbor = guardingCreaturePosition(dp.coord) != int3(-1, -1, -1);
				//const bool positionIsGuard = guardingCreaturePosition(cp->coord) == cp->coord; //can this be true? hero never passes from monster tile...

				if (dp.accessible == CGPathNode::ACCESSIBLE || dp.accessible == CGPathNode::FLYABLE
					|| (guardedNeighbor && !guardedPosition)) // Can step into a hostile tile once.
				{
					mq.push(&dp);
				}
			}
		} //neighbours loop
	} //queue loop
}

/**
 * Tells if the tile is guarded by a monster as well as the position
 * of the monster that will attack on it.
 *
 * @return int3(-1, -1, -1) if the tile is unguarded, or the position of
 * the monster guarding the tile.
 */
int3 CGameState::guardingCreaturePosition (int3 pos) const
{
	const int3 originalPos = pos;
	// Give monster at position priority.
	if (!map->isInTheMap(pos))
		return int3(-1, -1, -1);
	const TerrainTile &posTile = map->terrain[pos.x][pos.y][pos.z];
	if (posTile.visitable) 
	{
		BOOST_FOREACH (CGObjectInstance* obj, posTile.visitableObjects) 
		{
			if(obj->blockVisit)
			{
				if (obj->ID == 54) // Monster
					return pos;
				else
					return int3(-1, -1, -1); //blockvis objects are not guarded by neighbouring creatures
			}
		}
	}

	// See if there are any monsters adjacent.
	pos -= int3(1, 1, 0); // Start with top left.
	for (int dx = 0; dx < 3; dx++) 
	{
		for (int dy = 0; dy < 3; dy++) 
		{
			if (map->isInTheMap(pos)) 
			{
				TerrainTile &tile = map->terrain[pos.x][pos.y][pos.z];
				if (tile.visitable && (tile.tertype == TerrainTile::water) == (posTile.tertype == TerrainTile::water)) 
				{
					BOOST_FOREACH (CGObjectInstance* obj, tile.visitableObjects) 
					{
						if (obj->ID == 54  &&  checkForVisitableDir(pos, &map->getTile(originalPos), originalPos)) // Monster being able to attack investigated tile
						{ 
							return pos;
						}
					}
				}
			}

			pos.y++;
		}
		pos.y -= 3;
		pos.x++;
	}

	return int3(-1, -1, -1);
}

bool CGameState::isVisible(int3 pos, int player)
{
	if(player == 255) //neutral player
		return false;
	return getPlayerTeam(player)->fogOfWarMap[pos.x][pos.y][pos.z];
}

bool CGameState::isVisible( const CGObjectInstance *obj, int player )
{
	if(player == -1)
		return true;

	if(player == 255) //neutral player  -> TODO ??? needed?
		return false;
	//object is visible when at least one blocked tile is visible
	for(int fx=0; fx<8; ++fx)
	{
		for(int fy=0; fy<6; ++fy)
		{
			int3 pos = obj->pos + int3(fx-7,fy-5,0);
			if(map->isInTheMap(pos) 
				&& !((obj->defInfo->blockMap[fy] >> (7 - fx)) & 1) 
				&& isVisible(pos, player)  )
				return true;
		}
	}
	return false;
}

bool CGameState::checkForVisitableDir(const int3 & src, const int3 & dst) const
{
	const TerrainTile * pom = &map->getTile(dst);
	return checkForVisitableDir(src, pom, dst);
}

bool CGameState::checkForVisitableDir( const int3 & src, const TerrainTile *pom, const int3 & dst ) const
{
	for(unsigned int b=0; b<pom->visitableObjects.size(); ++b) //checking destination tile
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


int CGameState::victoryCheck( ui8 player ) const
{
	const PlayerState *p = getPlayer(player);
	if(map->victoryCondition.condition == winStandard  ||  map->victoryCondition.allowNormalVictory)
		if(player == checkForStandardWin())
			return -1;

	if (p->enteredWinningCheatCode)
	{ //cheater or tester, but has entered the code...
		return 1;
	}

	if(p->human || map->victoryCondition.appliesToAI)
	{
 		switch(map->victoryCondition.condition)
		{
		case artifact:
			//check if any hero has winning artifact
			for(size_t i = 0; i < p->heroes.size(); i++)
				if(p->heroes[i]->hasArt(map->victoryCondition.ID))
					return 1;

			break;

		case gatherTroop:
			{
				//check if in players armies there is enough creatures
				int total = 0; //creature counter
				for(size_t i = 0; i < map->objects.size(); i++)
				{
					const CArmedInstance *ai = NULL;
					if(map->objects[i] 
						&& map->objects[i]->tempOwner == player //object controlled by player
						&&  (ai = dynamic_cast<const CArmedInstance*>(map->objects[i].get()))) //contains army
					{
						for(TSlots::const_iterator i=ai->Slots().begin(); i!=ai->Slots().end(); ++i) //iterate through army
							if(i->second->type->idNumber == map->victoryCondition.ID) //it's searched creature
								total += i->second->count;
					}
				}

				if(total >= map->victoryCondition.count)
					return 1;
			}
			break;

		case gatherResource:
			if(p->resources[map->victoryCondition.ID] >= map->victoryCondition.count)
				return 1;

			break;

		case buildCity:
			{
				const CGTownInstance *t = static_cast<const CGTownInstance *>(map->victoryCondition.obj);
				if(t->tempOwner == player && t->fortLevel()-1 >= map->victoryCondition.ID && t->hallLevel()-1 >= map->victoryCondition.count)
					return 1;
			}
			break;

		case buildGrail:
			BOOST_FOREACH(const CGTownInstance *t, map->towns)
				if((t == map->victoryCondition.obj || !map->victoryCondition.obj)
					&& t->tempOwner == player 
					&& vstd::contains(t->builtBuildings, 26))
					return 1;
			break;

		case beatHero:
			if(map->victoryCondition.obj->tempOwner >= PLAYER_LIMIT) //target hero not present on map
				return 1;
			break;
		case captureCity:
			{
				if(map->victoryCondition.obj->tempOwner == player)
					return 1;
			}
			break;
		case beatMonster:
			if(!map->objects[map->victoryCondition.obj->id]) //target monster not present on map
				return 1;
			break;
		case takeDwellings:
			for(size_t i = 0; i < map->objects.size(); i++)
			{
				if(map->objects[i] && map->objects[i]->tempOwner != player) //check not flagged objs
				{
					switch(map->objects[i]->ID)
					{
					case 17: case 18: case 19: case 20: //dwellings
					case 216: case 217: case 218:
						return 0; //found not flagged dwelling - player not won
					}
				}
			}
			return 1;
			break;
		case takeMines:
			for(size_t i = 0; i < map->objects.size(); i++)
			{
				if(map->objects[i] && map->objects[i]->tempOwner != player) //check not flagged objs
				{
					switch(map->objects[i]->ID)
					{
					case 53: case 220:
						return 0; //found not flagged mine - player not won
					}
				}
			}
			return 1;
			break;
		case transportItem:
			{
				const CGTownInstance *t = static_cast<const CGTownInstance *>(map->victoryCondition.obj);
				if((t->visitingHero && t->visitingHero->hasArt(map->victoryCondition.ID))
					|| (t->garrisonHero && t->garrisonHero->hasArt(map->victoryCondition.ID)))
				{
					return 1;
				}
			}
			break;
 		}
	}

	return 0;
}

ui8 CGameState::checkForStandardWin() const
{
	//std victory condition is:
	//all enemies lost
	ui8 supposedWinner = 255, winnerTeam = 255;
	for(std::map<ui8,PlayerState>::const_iterator i = players.begin(); i != players.end(); i++)
	{
		if(i->second.status == PlayerState::INGAME && i->first < PLAYER_LIMIT)
		{
			if(supposedWinner == 255)		
			{
				//first player remaining ingame - candidate for victory
				supposedWinner = i->second.color;
				winnerTeam = i->second.team;
			}
			else if(winnerTeam != i->second.team)
			{
				//current candidate has enemy remaining in game -> no vicotry
				return 255;
			}
		}
	}

	return supposedWinner;
}

bool CGameState::checkForStandardLoss( ui8 player ) const
{
	//std loss condition is: player lost all towns and heroes
	const PlayerState &p = *getPlayer(player);
	return !p.heroes.size() && !p.towns.size();
}

struct statsHLP
{
	typedef std::pair< ui8, si64 > TStat;
	//converts [<player's color, value>] to vec[place] -> platers
	static std::vector< std::list< ui8 > > getRank( std::vector<TStat> stats )
	{
		std::sort(stats.begin(), stats.end(), statsHLP());

		//put first element
		std::vector< std::list<ui8> > ret;
		std::list<ui8> tmp;
		tmp.push_back( stats[0].first );
		ret.push_back( tmp );

		//the rest of elements
		for(int g=1; g<stats.size(); ++g)
		{
			if(stats[g].second == stats[g-1].second)
			{
				(ret.end()-1)->push_back( stats[g].first );
			}
			else
			{
				//create next occupied rank
				std::list<ui8> tmp;
				tmp.push_back(stats[g].first);
				ret.push_back(tmp);
			}
		}

		return ret;
	}

	bool operator()(const TStat & a, const TStat & b) const
	{
		return a.second > b.second;
	}

	static const CGHeroInstance * findBestHero(CGameState * gs, int color)
	{
		std::vector<ConstTransitivePtr<CGHeroInstance> > &h = gs->players[color].heroes;
		if(!h.size())
			return NULL;
		//best hero will be that with highest exp
		int best = 0;
		for(int b=1; b<h.size(); ++b)
		{
			if(h[b]->exp > h[best]->exp)
			{
				best = b;
			}
		}
		return h[best];
	}

	//calculates total number of artifacts that belong to given player
	static int getNumberOfArts(const PlayerState * ps)
	{
		int ret = 0;
		for(int g=0; g<ps->heroes.size(); ++g)
		{
			ret += ps->heroes[g]->artifactsInBackpack.size() + ps->heroes[g]->artifactsWorn.size();
		}
		return ret;
	}
};

void CGameState::obtainPlayersStats(SThievesGuildInfo & tgi, int level)
{
#define FILL_FIELD(FIELD, VAL_GETTER) \
	{ \
		std::vector< std::pair< ui8, si64 > > stats; \
		for(std::map<ui8, PlayerState>::const_iterator g = players.begin(); g != players.end(); ++g) \
		{ \
			if(g->second.color == 255) \
				continue; \
			std::pair< ui8, si64 > stat; \
			stat.first = g->second.color; \
			stat.second = VAL_GETTER; \
			stats.push_back(stat); \
		} \
		tgi.FIELD = statsHLP::getRank(stats); \
	}

	for(std::map<ui8, PlayerState>::const_iterator g = players.begin(); g != players.end(); ++g)
	{
		if(g->second.color != 255)
			tgi.playerColors.push_back(g->second.color);
	}
	
	if(level >= 1) //num of towns & num of heroes
	{
		//num of towns
		FILL_FIELD(numOfTowns, g->second.towns.size())
		//num of heroes
		FILL_FIELD(numOfHeroes, g->second.heroes.size())
		//best hero's portrait
		for(std::map<ui8, PlayerState>::const_iterator g = players.begin(); g != players.end(); ++g)
		{
			if(g->second.color == 255)
				continue;
			const CGHeroInstance * best = statsHLP::findBestHero(this, g->second.color);
			InfoAboutHero iah;
			iah.initFromHero(best, level >= 8);
			iah.army.clear();
			tgi.colorToBestHero[g->second.color] = iah;
		}
	}
	if(level >= 2) //gold
	{
		FILL_FIELD(gold, g->second.resources[6])
	}
	if(level >= 2) //wood & ore
	{
		FILL_FIELD(woodOre, g->second.resources[0] + g->second.resources[1])
	}
	if(level >= 3) //mercury, sulfur, crystal, gems
	{
		FILL_FIELD(mercSulfCrystGems, g->second.resources[2] + g->second.resources[3] + g->second.resources[4] + g->second.resources[5])
	}
	if(level >= 4) //obelisks found
	{
		//TODO
	}
	if(level >= 5) //artifacts
	{
		FILL_FIELD(artifacts, statsHLP::getNumberOfArts(&g->second))
	}
	if(level >= 6) //army strength
	{
		//TODO
	}
	if(level >= 7) //income
	{
		//TODO
	}
	if(level >= 8) //best hero's stats
	{
		//already set in  lvl 1 handling
	}
	if(level >= 9) //personality
	{
		for(std::map<ui8, PlayerState>::const_iterator g = players.begin(); g != players.end(); ++g)
		{
			if(g->second.color == 255) //do nothing for neutral player
				continue;
			if(g->second.human)
			{
				tgi.personality[g->second.color] = -1;
			}
			else //AI
			{
				tgi.personality[g->second.color] = map->players[g->second.color].AITactic;
			}
			
		}
	}
	if(level >= 10) //best creature
	{
		//best creatures belonging to player (highest AI value)
		for(std::map<ui8, PlayerState>::const_iterator g = players.begin(); g != players.end(); ++g)
		{
			if(g->second.color == 255) //do nothing for neutral player
				continue;
			int bestCre = -1; //best creature's ID
			for(int b=0; b<g->second.heroes.size(); ++b)
			{
				for(TSlots::const_iterator it = g->second.heroes[b]->Slots().begin(); it != g->second.heroes[b]->Slots().end(); ++it)
				{
					int toCmp = it->second->type->idNumber; //ID of creature we should compare with the best one
					if(bestCre == -1 || VLC->creh->creatures[bestCre]->AIValue < VLC->creh->creatures[toCmp]->AIValue)
					{
						bestCre = toCmp;
					}
				}
			}
			tgi.bestCreature[g->second.color] = bestCre;
		}
	}

#undef FILL_FIELD
}

int CGameState::lossCheck( ui8 player ) const
{
	const PlayerState *p = getPlayer(player);
	//if(map->lossCondition.typeOfLossCon == lossStandard)
		if(checkForStandardLoss(player))
			return -1;

	if (p->enteredLosingCheatCode)
	{
		return 1;
	}

	if(p->human) //special loss condition applies only to human player
	{
		switch(map->lossCondition.typeOfLossCon)
		{
		case lossCastle:
			{
				const CGTownInstance *t = dynamic_cast<const CGTownInstance *>(map->lossCondition.obj);
				assert(t);
				if(t->tempOwner != player)
					return 1;
			}
			break;
		case lossHero:
			{
				const CGHeroInstance *h = dynamic_cast<const CGHeroInstance *>(map->lossCondition.obj);
				assert(h);
				if(h->tempOwner != player)
					return 1;
			}
			break;
		case timeExpires:
			if(map->lossCondition.timeLimit < day)
				return 1;
			break;
		}
	}

	if(!p->towns.size() && p->daysWithoutCastle >= 7)
		return 2;

	return false;
}

bmap<ui32, ConstTransitivePtr<CGHeroInstance> > CGameState::unusedHeroesFromPool()
{
	bmap<ui32, ConstTransitivePtr<CGHeroInstance> > pool = hpool.heroesPool;
	for ( std::map<ui8, PlayerState>::iterator i = players.begin() ; i != players.end();i++)
		for(std::vector< ConstTransitivePtr<CGHeroInstance> >::iterator j = i->second.availableHeroes.begin(); j != i->second.availableHeroes.end(); j++)
			if(*j)
				pool.erase((**j).subID);

	return pool;
}

void CGameState::buildBonusSystemTree()
{
	buildGlobalTeamPlayerTree();
	attachArmedObjects();

	BOOST_FOREACH(CGTownInstance *t, map->towns)
	{
		t->deserializationFix();
	}
	// CStackInstance <-> CCreature, CStackInstance <-> CArmedInstance, CArtifactInstance <-> CArtifact 
	// are provided on initializing / deserializing
}

void CGameState::deserializationFix()
{
	buildGlobalTeamPlayerTree();
	attachArmedObjects();
}

void CGameState::buildGlobalTeamPlayerTree()
{
	for(std::map<ui8, TeamState>::iterator k=teams.begin(); k!=teams.end(); ++k)
	{
		TeamState *t = &k->second;
		t->attachTo(&globalEffects);

		BOOST_FOREACH(ui8 teamMember, k->second.players)
		{
			PlayerState *p = getPlayer(teamMember);
			assert(p);
			p->attachTo(t);
		}
	}
}

void CGameState::attachArmedObjects()
{
	BOOST_FOREACH(CGObjectInstance *obj, map->objects)
	{
		if(CArmedInstance *armed = dynamic_cast<CArmedInstance*>(obj))
			armed->whatShouldBeAttached()->attachTo(armed->whereShouldBeAttached(this));
	}
}

int3 CPath::startPos() const
{
	return nodes[nodes.size()-1].coord;
}
void CPath::convert(ui8 mode) //mode=0 -> from 'manifest' to 'object'
{
	if (mode==0)
	{
		for (unsigned int i=0;i<nodes.size();i++)
		{
			nodes[i].coord = CGHeroInstance::convertPosition(nodes[i].coord,true);
		}
	}
}

int3 CPath::endPos() const
{
	return nodes[0].coord;
}

CGPathNode::CGPathNode()
:coord(-1,-1,-1)
{
	accessible = 0;
	land = 0;
	moveRemains = 0;
	turns = 255;
	theNodeBefore = NULL;
}

bool CPathsInfo::getPath( const int3 &dst, CGPath &out )
{
	out.nodes.clear();
	const CGPathNode *curnode = &nodes[dst.x][dst.y][dst.z];
	if(!curnode->theNodeBefore || curnode->accessible == CGPathNode::FLYABLE)
		return false;


	//we'll transform number of turns to conform the rule that hero cannot stop on blocked tile
	bool transition01 = false;
	while(curnode)
	{
		CGPathNode cpn = *curnode;
		if(transition01)
		{
			if (curnode->accessible == CGPathNode::ACCESSIBLE)
			{
				transition01 = false;
			}
			else if (curnode->accessible == CGPathNode::FLYABLE)
			{
				cpn.turns = 1;
			}
		}
		if(curnode->turns == 1 && curnode->theNodeBefore->turns == 0)
		{
			transition01 = true;
		}
		curnode = curnode->theNodeBefore;

		out.nodes.push_back(cpn);
	}
	return true;
}

CPathsInfo::CPathsInfo( const int3 &Sizes )
:sizes(Sizes)
{
	hero = NULL;
	nodes = new CGPathNode**[sizes.x];
	for(int i = 0; i < sizes.x; i++)
	{
		nodes[i] = new CGPathNode*[sizes.y];
		for (int j = 0; j < sizes.y; j++)
		{
			nodes[i][j] = new CGPathNode[sizes.z];
		}
	}
}

CPathsInfo::~CPathsInfo()
{
	for(int i = 0; i < sizes.x; i++)
	{
		for (int j = 0; j < sizes.y; j++)
		{
			delete [] nodes[i][j];
		}
		delete [] nodes[i];
	}
	delete [] nodes;
}

int3 CGPath::startPos() const
{
	return nodes[nodes.size()-1].coord;
}

int3 CGPath::endPos() const
{
	return nodes[0].coord;
}

void CGPath::convert( ui8 mode )
{
	if(mode==0)
	{
		for(unsigned int i=0;i<nodes.size();i++)
		{
			nodes[i].coord = CGHeroInstance::convertPosition(nodes[i].coord,true);
		}
	}
}

PlayerState::PlayerState() 
 : color(-1), currentSelection(0xffffffff), enteredWinningCheatCode(0), 
   enteredLosingCheatCode(0), status(INGAME), daysWithoutCastle(0)
{
	nodeType = PLAYER;
}

std::string PlayerState::nodeName() const
{
	return "Player " + (color < VLC->generaltexth->capColors.size() ? VLC->generaltexth->capColors[color] : boost::lexical_cast<std::string>(color));
}

// void PlayerState::getParents(TCNodes &out, const CBonusSystemNode *root /*= NULL*/) const
// {
// 	return; //no loops possible
// }
// 
// void PlayerState::getBonuses(BonusList &out, const CSelector &selector, const CBonusSystemNode *root /*= NULL*/) const
// {
// 	for (std::vector<CGHeroInstance *>::const_iterator it = heroes.begin(); it != heroes.end(); it++)
// 	{
// 		if (*it != root)
// 			(*it)->getBonuses(out, selector, this);
// 	}
// }

InfoAboutHero::InfoAboutHero()
{
	details = NULL;
	hclass = NULL;
	portrait = -1;
}

InfoAboutHero::InfoAboutHero( const InfoAboutHero & iah )
{
	assign(iah);
}

InfoAboutHero::~InfoAboutHero()
{
	delete details;
}

void InfoAboutHero::initFromHero( const CGHeroInstance *h, bool detailed )
{
	if(!h) return;

	owner = h->tempOwner;
	hclass = h->type->heroClass;
	name = h->name;
	portrait = h->portrait;
	army = ArmyDescriptor(h, detailed);

	if(detailed) 
	{
		//include details about hero
		details = new Details;
		details->luck = h->LuckVal();
		details->morale = h->MoraleVal();
		details->mana = h->mana;
		details->primskills.resize(PRIMARY_SKILLS);

		for (int i = 0; i < PRIMARY_SKILLS ; i++)
		{
			details->primskills[i] = h->getPrimSkillLevel(i);
		}
	}
}

void InfoAboutHero::assign( const InfoAboutHero & iah )
{
	army = iah.army;
	details = (iah.details ? new Details(*iah.details) : NULL);
	hclass = iah.hclass;
	name = iah.name;
	owner = iah.owner;
	portrait = iah.portrait;
}

InfoAboutHero & InfoAboutHero::operator=( const InfoAboutHero & iah )
{
	assign(iah);
	return *this;
}



ArmyDescriptor::ArmyDescriptor(const CArmedInstance *army, bool detailed)
{
	for(TSlots::const_iterator i = army->Slots().begin(); i != army->Slots().end(); i++)
	{
		if(detailed)
			(*this)[i->first] = *i->second;
		else
			(*this)[i->first] = CStackBasicDescriptor(i->second->type, i->second->getQuantityID());
	}
}

ArmyDescriptor::ArmyDescriptor()
{

}

DuelParameters::SideSettings::StackSettings::StackSettings()
	: type(-1), count(0)
{
}

DuelParameters::SideSettings::StackSettings::StackSettings(si32 Type, si32 Count)
	: type(Type), count(Count)
{
}

DuelParameters::SideSettings::SideSettings()
{
	heroId = -1;
}

DuelParameters::DuelParameters()
{
	terType = TerrainTile::dirt;
	bfieldType = 15;
}

TeamState::TeamState()
{
	nodeType = TEAM;
}