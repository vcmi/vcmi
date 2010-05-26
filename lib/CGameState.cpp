#define VCMI_DLL
#include "../hch/CCampaignHandler.h"
#include <algorithm>
#include <queue>
#include <fstream>
#include "CGameState.h"
#include <boost/random/linear_congruential.hpp>
#include "../hch/CDefObjInfoHandler.h"
#include "../hch/CArtHandler.h"
#include "../hch/CBuildingHandler.h"
#include "../hch/CGeneralTextHandler.h"
#include "../hch/CTownHandler.h"
#include "../hch/CSpellHandler.h"
#include "../hch/CHeroHandler.h"
#include "../hch/CObjectHandler.h"
#include "../hch/CCreatureHandler.h"
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
	virtual ~CBaseForGSApply(){};
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
	~CGSApplier()
	{
		std::map<ui16,CBaseForGSApply*>::iterator iter;

		for(iter = apps.begin(); iter != apps.end(); iter++)
			delete iter->second;
	}
	template<typename T> void registerType(const T * t=NULL)
	{
		ui16 ID = typeList.registerType(t);
		apps[ID] = new CApplyOnGS<T>;
	}

} *applierGs = NULL;

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
		T::preInit();
	}
	void postInit()
	{
		T::postInit();
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
		for (size_t i = 0; i < apps.size(); i++)
			apps[i]->preInit();
	}

	void postInit()
	{
		for (size_t i = 0; i < apps.size(); i++)
			apps[i]->postInit();
	}
} *objCaller = NULL;

void MetaString::getLocalString(const std::pair<ui8,ui32> &txt, std::string &dst) const
{
	int type = txt.first, ser = txt.second;

	if(type == ART_NAMES)
	{
		dst = VLC->arth->artifacts[ser].Name();
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
		dst = VLC->spellh->spells[ser].name;
	}
	else if(type == CRE_SING_NAMES)
	{
		dst = VLC->creh->creatures[ser]->nameSing;
	}
	else if(type == ART_DESCR)
	{
		dst = VLC->arth->artifacts[ser].Description();
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
		default:
			tlog1 << "MetaString processing error!\n";
			break;
		}
	}
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

void MetaString::addReplacement(const CStackInstance &stack)
{
	assert(stack.count); //valid count
	assert(stack.type); //valid type

	if (stack.count == 1)
		addReplacement (CRE_SING_NAMES, stack.type->idNumber);
	else
		addReplacement (CRE_PL_NAMES, stack.type->idNumber);
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

CStack * BattleInfo::getStack(int stackID, bool onlyAlive)
{
	for(unsigned int g=0; g<stacks.size(); ++g)
	{
		if(stacks[g]->ID == stackID && (!onlyAlive || stacks[g]->alive()))
			return stacks[g];
	}
	return NULL;
}

const CStack * BattleInfo::getStack(int stackID, bool onlyAlive) const
{
	return const_cast<BattleInfo * const>(this)->getStack(stackID, onlyAlive);
}

CStack * BattleInfo::getStackT(int tileID, bool onlyAlive)
{
	for(unsigned int g=0; g<stacks.size(); ++g)
	{
		if(stacks[g]->position == tileID 
			|| (stacks[g]->doubleWide() && stacks[g]->attackerOwned && stacks[g]->position-1 == tileID)
			|| (stacks[g]->doubleWide() && !stacks[g]->attackerOwned && stacks[g]->position+1 == tileID))
		{
			if(!onlyAlive || stacks[g]->alive())
			{
				return stacks[g];
			}
		}
	}
	return NULL;
}

const CStack * BattleInfo::getStackT(int tileID, bool onlyAlive) const
{
	return const_cast<BattleInfo * const>(this)->getStackT(tileID, onlyAlive);
}

void BattleInfo::getAccessibilityMap(bool *accessibility, bool twoHex, bool attackerOwned, bool addOccupiable, std::set<int> & occupyable, bool flying, int stackToOmmit) const
{
	memset(accessibility, 1, BFIELD_SIZE); //initialize array with trues

	//removing accessibility for side columns of hexes
	for(int v = 0; v < BFIELD_SIZE; ++v)
	{
		if( v % BFIELD_WIDTH == 0 || v % BFIELD_WIDTH == (BFIELD_WIDTH - 1) )
			accessibility[v] = false;
	}

	for(unsigned int g=0; g<stacks.size(); ++g)
	{
		if(!stacks[g]->alive() || stacks[g]->ID==stackToOmmit || stacks[g]->position < 0) //we don't want to lock position of this stack (eg. if it's a turret)
			continue;

		accessibility[stacks[g]->position] = false;
		if(stacks[g]->doubleWide()) //if it's a double hex creature
		{
			if(stacks[g]->attackerOwned)
				accessibility[stacks[g]->position-1] = false;
			else
				accessibility[stacks[g]->position+1] = false;
		}
	}
	//obstacles
	for(unsigned int b=0; b<obstacles.size(); ++b)
	{
		std::vector<int> blocked = VLC->heroh->obstacles[obstacles[b].ID].getBlocked(obstacles[b].pos);
		for(unsigned int c=0; c<blocked.size(); ++c)
		{
			if(blocked[c] >=0 && blocked[c] < BFIELD_SIZE)
				accessibility[blocked[c]] = false;
		}
	}

	//walls
	if(siege > 0)
	{
		static const int permanentlyLocked[] = {12, 45, 78, 112, 147, 165};
		for(int b=0; b<ARRAY_COUNT(permanentlyLocked); ++b)
		{
			accessibility[permanentlyLocked[b]] = false;
		}

		static const std::pair<int, int> lockedIfNotDestroyed[] = //(which part of wall, which hex is blocked if this part of wall is not destroyed
			{std::make_pair(2, 182), std::make_pair(3, 130), std::make_pair(4, 62), std::make_pair(5, 29)};
		for(int b=0; b<ARRAY_COUNT(lockedIfNotDestroyed); ++b)
		{
			if(si.wallState[lockedIfNotDestroyed[b].first] < 3)
			{
				accessibility[lockedIfNotDestroyed[b].second] = false;
			}
		}

		//gate
		if(attackerOwned && si.wallState[7] < 3) //if it attacker's unit and gate is not destroyed
		{
			accessibility[95] = accessibility[96] = false; //block gate's hexes
		}
	}

	//occupyability
	if(addOccupiable && twoHex)
	{
		std::set<int> rem; //tiles to unlock
		for(int h=0; h<BFIELD_HEIGHT; ++h)
		{
			for(int w=1; w<BFIELD_WIDTH-1; ++w)
			{
				int hex = h * BFIELD_WIDTH + w;
				if(!isAccessible(hex, accessibility, twoHex, attackerOwned, flying, true)
					&& (attackerOwned ? isAccessible(hex+1, accessibility, twoHex, attackerOwned, flying, true) : isAccessible(hex-1, accessibility, twoHex, attackerOwned, flying, true) )
					)
					rem.insert(hex);
			}
		}
		occupyable = rem;
		/*for(std::set<int>::const_iterator it = rem.begin(); it != rem.end(); ++it)
		{
			accessibility[*it] = true;
		}*/
	}
}

bool BattleInfo::isAccessible(int hex, bool * accessibility, bool twoHex, bool attackerOwned, bool flying, bool lastPos)
{
	if(flying && !lastPos)
		return true;

	if(twoHex)
	{
		//if given hex is accessible and appropriate adjacent one is free too
		return accessibility[hex] && accessibility[hex + (attackerOwned ? -1 : 1 )];
	}
	else
	{
		return accessibility[hex];
	}
}

void BattleInfo::makeBFS(int start, bool *accessibility, int *predecessor, int *dists, bool twoHex, bool attackerOwned, bool flying, bool fillPredecessors) const //both pointers must point to the at least 187-elements int arrays
{
	//inits
	for(int b=0; b<BFIELD_SIZE; ++b)
		predecessor[b] = -1;
	for(int g=0; g<BFIELD_SIZE; ++g)
		dists[g] = 100000000;	
	
	std::queue< std::pair<int, bool> > hexq; //bfs queue <hex, accessible> (second filed used only if fillPredecessors is true)
	hexq.push(std::make_pair(start, true));
	dists[hexq.front().first] = 0;
	int curNext = -1; //for bfs loop only (helper var)
	while(!hexq.empty()) //bfs loop
	{
		std::pair<int, bool> curHex = hexq.front();
		std::vector<int> neighbours = neighbouringTiles(curHex.first);
		hexq.pop();
		for(unsigned int nr=0; nr<neighbours.size(); nr++)
		{
			curNext = neighbours[nr]; //if(!accessibility[curNext] || (dists[curHex]+1)>=dists[curNext])
			bool accessible = isAccessible(curNext, accessibility, twoHex, attackerOwned, flying, dists[curHex.first]+1 == dists[curNext]);
			if( dists[curHex.first]+1 >= dists[curNext] )
				continue;
			if(accessible && curHex.second)
			{
				hexq.push(std::make_pair(curNext, true));
				dists[curNext] = dists[curHex.first] + 1;
			}
			else if(fillPredecessors && !(accessible && !curHex.second))
			{
				hexq.push(std::make_pair(curNext, false));
				dists[curNext] = dists[curHex.first] + 1;
			}
			predecessor[curNext] = curHex.first;
		}
	}
};

std::vector<int> BattleInfo::getAccessibility(int stackID, bool addOccupiable) const
{
	std::vector<int> ret;
	bool ac[BFIELD_SIZE];
	const CStack *s = getStack(stackID, false); //this function is called from healedOrResurrected, so our stack can be dead

	if(s->position < 0) //turrets
		return std::vector<int>();

	std::set<int> occupyable;

	getAccessibilityMap(ac, s->doubleWide(), s->attackerOwned, addOccupiable, occupyable, s->hasBonusOfType(Bonus::FLYING), stackID);

	int pr[BFIELD_SIZE], dist[BFIELD_SIZE];
	makeBFS(s->position, ac, pr, dist, s->doubleWide(), s->attackerOwned, s->hasBonusOfType(Bonus::FLYING), false);

	if(s->doubleWide())
	{
		if(!addOccupiable)
		{
			std::vector<int> rem;
			for(int b=0; b<BFIELD_SIZE; ++b)
			{
				//don't take into account most left and most right columns of hexes
				if( b % BFIELD_WIDTH == 0 || b % BFIELD_WIDTH == BFIELD_WIDTH - 1 )
					continue;

				if( ac[b] && !(s->attackerOwned ? ac[b-1] : ac[b+1]) )
				{
					rem.push_back(b);
				}
			}

			for(unsigned int g=0; g<rem.size(); ++g)
			{
				ac[rem[g]] = false;
			}

			//removing accessibility for side hexes
			for(int v=0; v<BFIELD_SIZE; ++v)
				if(s->attackerOwned ? (v%BFIELD_WIDTH)==1 : (v%BFIELD_WIDTH)==(BFIELD_WIDTH - 2))
					ac[v] = false;
		}
	}
	
	for (int i=0; i < BFIELD_SIZE ; ++i) {
		if(
			( ( !addOccupiable && dist[i] <= s->Speed() && ac[i] ) || ( addOccupiable && dist[i] <= s->Speed() && isAccessible(i, ac, s->doubleWide(), s->attackerOwned, s->hasBonusOfType(Bonus::FLYING), true) ) )//we can reach it
			|| (vstd::contains(occupyable, i) && ( dist[ i + (s->attackerOwned ? 1 : -1 ) ] <= s->Speed() ) &&
				ac[i + (s->attackerOwned ? 1 : -1 )] ) //it's occupyable and we can reach adjacent hex
			)
		{
			ret.push_back(i);
		}
	}

	return ret;
}
bool BattleInfo::isStackBlocked(int ID)
{
	CStack *our = getStack(ID);
	if(our->hasBonusOfType(Bonus::SIEGE_WEAPON)) //siege weapons cannot be blocked
		return false;

	for(unsigned int i=0; i<stacks.size();i++)
	{
		if( !stacks[i]->alive()
			|| stacks[i]->owner==our->owner
		  )
			continue; //we omit dead and allied stacks
		if(stacks[i]->doubleWide())
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
std::pair< std::vector<int>, int > BattleInfo::getPath(int start, int dest, bool*accessibility, bool flyingCreature, bool twoHex, bool attackerOwned)
{							
	int predecessor[BFIELD_SIZE]; //for getting the Path
	int dist[BFIELD_SIZE]; //calculated distances

	makeBFS(start, accessibility, predecessor, dist, twoHex, attackerOwned, flyingCreature, false);
	
	if(predecessor[dest] == -1) //cannot reach destination
	{
		return std::make_pair(std::vector<int>(), 0);
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

CStack::CStack(const CStackInstance *base, int O, int I, bool AO, int S)
	: CStackInstance(*base), ID(I), owner(O), slot(S), attackerOwned(AO), position(-1),   
	counterAttacks(1)
{
	baseAmount = base->count;
	firstHPleft = valOfBonuses(Bonus::STACK_HEALTH);
	shots = type->shots;
	counterAttacks += valOfBonuses(Bonus::ADDITIONAL_RETALIATION);

	//alive state indication
	state.insert(ALIVE);
}

ui32 CStack::Speed( int turn /*= 0*/ ) const
{
	if(hasBonus(Selector::type(Bonus::SIEGE_WEAPON) && Selector::turns(turn))) //war machines cannot move
		return 0;

	int speed = valOfBonuses(Selector::type(Bonus::STACKS_SPEED) && Selector::turns(turn));

	int percentBonus = 0;
	BOOST_FOREACH(const Bonus &b, bonuses)
	{
		if(b.type == Bonus::STACKS_SPEED)
		{
			percentBonus += b.additionalInfo;
		}
	}

	speed = ((100 + percentBonus) * speed)/100;

	//bind effect check
	if(getEffect(72)) 
	{
		return 0;
	}

	return speed;
}

const CStack::StackEffect * CStack::getEffect( ui16 id, int turn /*= 0*/ ) const
{
	for (unsigned int i=0; i< effects.size(); i++)
		if(effects[i].id == id)
			if(!turn || effects[i].turnsRemain > turn)
				return &effects[i];
	return NULL;
}

ui8 CStack::howManyEffectsSet(ui16 id) const
{
	ui8 ret = 0;
	for (unsigned int i=0; i< effects.size(); i++)
		if(effects[i].id == id) //effect found
		{
			++ret;
		}
	return ret;
}

bool CStack::willMove(int turn /*= 0*/) const
{
	return ( turn ? true : !vstd::contains(state, DEFENDING) )
		&& !moved(turn)
		&& canMove(turn);
}

bool CStack::canMove( int turn /*= 0*/ ) const
{
	return alive()
		&& !hasBonus(Selector::type(Bonus::NOT_ACTIVE) && Selector::turns(turn)); //eg. Ammo Cart or blinded creature
}

bool CStack::moved( int turn /*= 0*/ ) const
{
	if(!turn)
		return vstd::contains(state, MOVED);
	else
		return false;
}

bool CStack::doubleWide() const
{
	return type->doubleWide;
}

CGHeroInstance * CGameState::HeroesPool::pickHeroFor(bool native, int player, const CTown *town, std::map<ui32,CGHeroInstance *> &available) const
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
		for(std::map<ui32,CGHeroInstance *>::iterator i=available.begin(); i!=available.end(); i++)
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

		for(std::map<ui32,CGHeroInstance *>::iterator i=available.begin(); i!=available.end(); i++)
		{
			if(pavailable.find(i->first)->second & 1<<player)
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
	if(objid<0 || objid>=map->objects.size() || map->objects[objid]->ID!=HEROI_TYPE)
		return NULL;
	return static_cast<CGHeroInstance *>(map->objects[objid]);
}

const CGHeroInstance * CGameState::getHero( int objid ) const
{
	return (const_cast<CGameState *>(this))->getHero(objid);
}

CGTownInstance *CGameState::getTown(int objid)
{
	if(objid<0 || objid>=map->objects.size())
		return NULL;
	return static_cast<CGTownInstance *>(map->objects[objid]);
}

const CGTownInstance * CGameState::getTown( int objid ) const
{
	return (const_cast<CGameState *>(this))->getTown(objid);
}

std::pair<int,int> CGameState::pickObject (CGObjectInstance *obj)
{
	switch(obj->ID)
	{
	case 65: //random artifact
		return std::pair<int,int>(5,(ran()%136)+7); //the only reasonable range - there are siege weapons and blanks we must ommit
	case 66: //random treasure artifact
 		return std::pair<int,int>(5,VLC->arth->treasures[ran()%VLC->arth->treasures.size()]->id);
	case 67: //random minor artifact
 		return std::pair<int,int>(5,VLC->arth->minors[ran()%VLC->arth->minors.size()]->id);
	case 68: //random major artifact
		return std::pair<int,int>(5,VLC->arth->majors[ran()%VLC->arth->majors.size()]->id);
	case 69: //random relic artifact
		return std::pair<int,int>(5,VLC->arth->relics[ran()%VLC->arth->relics.size()]->id);
	/*case 65: //random artifact //TODO: apply new randndomization system
		return std::pair<int,int>(5, cb.getRandomArt (CArtifact::ART_TREASURE | CArtifact::ART_MINOR | CArtifact::ART_MAJOR | CArtifact::ART_RELIC));
	case 66: //random treasure artifact
		return std::pair<int,int>(5, cb.getRandomArt (CArtifact::ART_TREASURE));
	case 67: //random minor artifact
		return std::pair<int,int>(5, cb.getRandomArt (CArtifact::ART_MINOR));
	case 68: //random major artifact
		return std::pair<int,int>(5, cb.getRandomArt (CArtifact::ART_MAJOR));
	case 69: //random relic artifact
		return std::pair<int,int>(5, cb.getRandomArt (CArtifact::ART_RELIC));*/
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
		return std::pair<int,int>(54,VLC->creh->levelCreatures[5][ran()%VLC->creh->levelCreatures[5].size()]->idNumber);
	case 163: //random monster lvl6
		return std::pair<int,int>(54,VLC->creh->levelCreatures[6][ran()%VLC->creh->levelCreatures[6].size()]->idNumber);
	case 164: //random monster lvl7
		return std::pair<int,int>(54,VLC->creh->levelCreatures[7][ran()%VLC->creh->levelCreatures[7].size()]->idNumber); 
	case 216: //random dwelling
		{
			int faction = ran()%F_NUMBER;
			CGDwelling * dwl = static_cast<CGDwelling*>(obj);
			CCreGen2ObjInfo* info = static_cast<CCreGen2ObjInfo*>(dwl->info);
			if (info->asCastle)
			{
				for(unsigned int i=0;i<map->objects.size();i++)
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
	objCaller = new CObjectCallersHandler;
	campaign = NULL;
}
CGameState::~CGameState()
{
	delete mx;
	delete map;
	delete curB;
	delete scenarioOps;
	delete applierGs;
	delete objCaller;
}
void CGameState::init( StartInfo * si, ui32 checksum, int Seed )
{

	switch(si->mode)
	{
	case 0:
		map = new Mapa(si->mapname);
		break;
	case 2:
		campaign = new CCampaignState();
		campaign->initNewCampaign(*si);
		std::string &mapContent = campaign->camp->mapPieces[si->whichMapInCampaign];
		map = new Mapa();
		map->initFromBytes((const unsigned char*)mapContent.c_str());
		break;
	}
	tlog0 << "Map loaded!" << std::endl;

	//tlog0 <<"Reading and detecting map file (together): "<<tmh.getDif()<<std::endl;
	if(checksum)
	{
		tlog0 << "\tServer checksum for " << si->mapname <<": "<< checksum << std::endl;
		tlog0 << "\tOur checksum for the map: "<< map->checksum << std::endl;
		if(map->checksum != checksum)
		{
			tlog1 << "Wrong map checksum!!!" << std::endl;
			throw std::string("Wrong checksum");
		}
	}

	day = 0;
	seed = Seed;
	ran.seed((boost::int32_t)seed);
	scenarioOps = si;
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
	for(unsigned int i=0;i<scenarioOps->playerInfos.size();i++)
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
	for(unsigned int no=0; no<map->objects.size(); ++no)
	{
		randomizeObject(map->objects[no]);
		if(map->objects[no]->ID==EVENTI_TYPE)
		{
			map->objects[no]->defInfo->handler=NULL;
		}
		map->objects[no]->hoverName = VLC->generaltexth->names[map->objects[no]->ID];
	}
	//std::cout<<"\tRandomizing objects: "<<th.getDif()<<std::endl;

	/*********give starting hero****************************************/
	for(int i=0;i<PLAYER_LIMIT;i++)
	{
		if((map->players[i].generateHeroAtMainTown && map->players[i].hasMainTown) ||  (map->players[i].hasMainTown && map->version==CMapHeader::RoE))
		{
			int3 hpos = map->players[i].posOfMainTown;
			hpos.x+=1;// hpos.y+=1;
			int j;
			for(j=0; j<scenarioOps->playerInfos.size(); j++) //don't add unsigned here - we are refering to the variable above
				if(scenarioOps->playerInfos[j].color == i)
					break;
			if(j == scenarioOps->playerInfos.size())
				continue;

			int h=pickHero(i);
			if(scenarioOps->playerInfos[j].hero == -1)
				scenarioOps->playerInfos[j].hero = h;

			CGHeroInstance * nnn =  static_cast<CGHeroInstance*>(createObject(HEROI_TYPE,h,hpos,i));
			nnn->id = map->objects.size();
			hpos = map->players[i].posOfMainTown;hpos.x+=2;
			for(unsigned int o=0;o<map->towns.size();o++) //find main town
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
	for (unsigned int i=0; i<scenarioOps->playerInfos.size();i++)
	{
		std::pair<int,PlayerState> ins(scenarioOps->playerInfos[i].color,PlayerState());
		ins.second.color=ins.first;
		ins.second.serial=i;
		ins.second.human = scenarioOps->playerInfos[i].human;
		players.insert(ins);
	}
	/******************RESOURCES****************************************************/
	//TODO: computer player should receive other amount of resource than player (depending on difficulty)
	std::vector<int> startres;
	std::ifstream tis(DATA_DIR "/config/startres.txt");
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


	/*************************HEROES************************************************/
	std::set<int> hids;
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
	for (unsigned int i=0; i<map->objects.size();i++) //heroes instances initialization
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
	BOOST_FOREACH(int hid, hids) //all not used allowed heroes go into the pool
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

			std::set<int3> tiles;
			obj->getSightTiles(tiles);
			BOOST_FOREACH(int3 tile, tiles)
			{
				k->second.fogOfWarMap[tile.x][tile.y][tile.z] = 1;
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
				{
					VLC->arth->equipArtifact(hero->artifWorn, *slot, toGive->id, &hero->bonuses);
				}
				else
					hero->giveArtifact(toGive->id);
			}
		}
	}
	/****************************TOWNS************************************************/
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
		}//init hordes
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
		//init spells
		vti->spells.resize(SPELL_LEVELS);
		CSpell *s;
		for(unsigned int z=0; z<vti->obligatorySpells.size();z++)
		{
			s = &VLC->spellh->spells[vti->obligatorySpells[z]];
			vti->spells[s->level-1].push_back(s->id);
			vti->possibleSpells -= s->id;
		}
		while(vti->possibleSpells.size())
		{
			ui32 total=0, sel=-1;
			for(unsigned int ps=0;ps<vti->possibleSpells.size();ps++)
				total += VLC->spellh->spells[vti->possibleSpells[ps]].probabilities[vti->subID];
			int r = (total)? ran()%total : -1;
			for(unsigned int ps=0; ps<vti->possibleSpells.size();ps++)
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
		if(vti->getOwner() != 255)
			getPlayer(vti->getOwner())->towns.push_back(vti);
	}

	for(std::map<ui8, PlayerState>::iterator k=players.begin(); k!=players.end(); ++k)
	{
		if(k->first==-1 || k->first==255)
			continue;

		//init visiting and garrisoned heroes
		for(unsigned int l=0; l<k->second.heroes.size();l++)
		{ 
			CGHeroInstance *h = k->second.heroes[l];
			for(unsigned int m=0; m<k->second.towns.size();m++)
			{
				CGTownInstance *t = k->second.towns[m];
				int3 vistile = t->pos; vistile.x--; //tile next to the entrance
				if(vistile == h->pos || h->pos==t->pos)
				{
					t->visitingHero = h;
					h->visitedTown = t;
					h->inTownGarrison = false;
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

	for(unsigned int i=0; i<map->defy.size(); i++)
	{
		map->defy[i]->serial = i;
	}

	objCaller->preInit();
	for(unsigned int i=0; i<map->objects.size(); i++)
	{
		map->objects[i]->initObj();
		if(map->objects[i]->ID == 62) //prison also needs to initialize hero
			static_cast<CGHeroInstance*>(map->objects[i])->initHero();
	}
	objCaller->postInit();
}

bool CGameState::battleShootCreatureStack(int ID, int dest)
{
	return true;
}

bool CGameState::battleCanFlee(int player)
{
	if(!curB) //there is no battle
		return false;

	if(curB->heroes[0]->hasBonusOfType(Bonus::ENEMY_CANT_ESCAPE) //eg. one of heroes is wearing shakles of war
		|| curB->heroes[0]->hasBonusOfType(Bonus::ENEMY_CANT_ESCAPE))
		return false;

	return true;
}

int CGameState::battleGetStack(int pos, bool onlyAlive)
{
	if(!curB)
		return -1;
	for(unsigned int g=0; g<curB->stacks.size(); ++g)
	{
		if((curB->stacks[g]->position == pos 
			  || (curB->stacks[g]->doubleWide() 
					&&( (curB->stacks[g]->attackerOwned && curB->stacks[g]->position-1 == pos) 
					||	(!curB->stacks[g]->attackerOwned && curB->stacks[g]->position+1 == pos)	)
			 ))
			 && (!onlyAlive || curB->stacks[g]->alive())
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

	const std::vector <CGObjectInstance*> & objs = map->objects;
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

	switch(map->terrain[tile.x][tile.y][tile.z].tertype)
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

const CGHeroInstance * CGameState::battleGetOwner(int stackID)
{
	if(!curB)
		return NULL;

	return curB->heroes[!curB->getStack(stackID)->attackerOwned];
}

UpgradeInfo CGameState::getUpgradeInfo(const CArmedInstance *obj, int stackPos)
{
	UpgradeInfo ret;
	const CCreature *base = obj->getCreature(stackPos);
	if((obj->ID == TOWNI_TYPE)  ||  ((obj->ID == HEROI_TYPE) && static_cast<const CGHeroInstance*>(obj)->visitedTown))
	{
		const CGTownInstance * t;
		if(obj->ID == TOWNI_TYPE)
			t = static_cast<const CGTownInstance *>(obj);
		else
			t = static_cast<const CGHeroInstance*>(obj)->visitedTown;
		for(std::set<si32>::const_iterator i=t->builtBuildings.begin();  i!=t->builtBuildings.end(); i++)
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
						int dif = VLC->creh->creatures[nid]->cost[j] - base->cost[j];
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

void CGameState::loadTownDInfos()
{
	for(int i=0;i<F_NUMBER;i++)
	{
		villages[i] = new CGDefInfo(*VLC->dobjinfo->castles[i]);
		forts[i] = VLC->dobjinfo->castles[i];
		capitols[i] = new CGDefInfo(*VLC->dobjinfo->castles[i]);
	}
}

void CGameState::getNeighbours( const TerrainTile &srct, int3 tile, std::vector<int3> &vec, const boost::logic::tribool &onLand )
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
		if(srct.blocked && hlpt.visitable && srct.blockingObjects.front()->ID != HEROI_TYPE)
		{
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

	if(d.blocked)
	{
		bool freeFlying = h->getBonusesCount(Selector::typeSybtype(Bonus::FLYING_MOVEMENT, 1)) > 0;

		if(!freeFlying)
		{
			ret *= 1.4f; //40% penalty for movement over blocked tile
		}
	}
	else if (d.tertype == TerrainTile::water)
	{
		if (!h->boat && h->getBonusesCount(Selector::typeSybtype(Bonus::WATER_WALKING, 1)) > 0)
		{
			ret *= 1.4f; //40% penalty for water walking
		}
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
		getNeighbours(d, dest, vec, s.tertype != TerrainTile::water);
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
	int ret = 7; //allowed by default

	if(t->builded >= MAX_BUILDING_PER_TURN)
		ret = 5; //building limit

	//checking resources
	CBuilding * pom = VLC->buildh->buildings[t->subID][ID];
	
	if(!pom)
		return 8;

// 	if(pom->Name().size()==0||pom->resources.size()==0)
// 		return 2;//TODO: why does this happen?

	for(int res=0;res<pom->resources.size();res++) //TODO: support custom amount of resources
	{
		if(pom->resources[res] > getPlayer(t->tempOwner)->resources[res])
			ret = 6; //lack of res
	}

	//checking for requirements
	std::set<int> reqs = getBuildingRequiments(t, ID);//getting all requiments

	for( std::set<int>::iterator ri  =  reqs.begin(); ri != reqs.end(); ri++ )
	{
		if(t->builtBuildings.find(*ri)==t->builtBuildings.end())
			ret = 8; //lack of requirements - cannot build
	}

	//can we build it?
	if(t->forbiddenBuildings.find(ID)!=t->forbiddenBuildings.end())
		ret = 2; //forbidden

	if(ID == 13) //capitol
	{
		for(unsigned int in = 0; in < map->towns.size(); in++)
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
		int3 t1(t->pos + int3(-1,3,0)),
			t2(t->pos + int3(-3,3,0));
		if(map->isInTheMap(t1) && map->getTile(t1).tertype != TerrainTile::water  
			&&  (map->isInTheMap(t2) && map->getTile(t2).tertype != TerrainTile::water))
			ret = 1; //lack of water
	}

	if(t->builtBuildings.find(ID)!=t->builtBuildings.end())	//already built
		ret = 4;
	return ret;
}

void CGameState::apply(CPack *pack)
{
	ui16 typ = typeList.getTypeID(pack);
	assert(typ >= 0);
	applierGs->apps[typ]->applyOnGS(this,pack);
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

	const std::vector<std::vector<std::vector<ui8> > > &FoW = getPlayer(hero->tempOwner)->fogOfWarMap;

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
		getNeighbours(map->getTile(cp.coord), cp.coord, neighbours, boost::logic::indeterminate);
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

	const std::vector<std::vector<std::vector<ui8> > > &FoW = getPlayer(hero->tempOwner)->fogOfWarMap;

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

				if((onLand || indeterminate(onLand)) && !node.land)//it's sea and we cannot walk on sea
				{
					if (waterWalk || flying)
					{
						node.accessible = CGPathNode::FLYABLE;
					}
					else
					{
						node.accessible = CGPathNode::BLOCKED;
					}
				}

				if ( tinfo->tertype == TerrainTile::rock//it's rock
					|| !onLand && node.land		//it's land and we cannot walk on land (complementary condition is handled above)
					|| !FoW[i][j][k]					//tile is covered by the FoW
				)
				{
					node.accessible = CGPathNode::BLOCKED;
				}
				else if(tinfo->visitable)
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
		getNeighbours(ct, cp->coord, neighbours, boost::logic::indeterminate);
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
	// Give monster at position priority.
	if (!map->isInTheMap(pos))
		return int3(-1, -1, -1);
	const TerrainTile &posTile = map->terrain[pos.x][pos.y][pos.z];
	if (posTile.visitable) {
		BOOST_FOREACH (CGObjectInstance* obj, posTile.visitableObjects) {
			if (obj->ID == 54) { // Monster
				return pos;
			}
		}
	}

	// See if there are any monsters adjacent.
	pos -= int3(1, 1, 0); // Start with top left.
	for (int dx = 0; dx < 3; dx++) {
		for (int dy = 0; dy < 3; dy++) {
			if (map->isInTheMap(pos)) {
				TerrainTile &tile = map->terrain[pos.x][pos.y][pos.z];
				if (tile.visitable) {
					BOOST_FOREACH (CGObjectInstance* obj, tile.visitableObjects) {
						if (obj->ID == 54) { // Monster
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
	return players[player].fogOfWarMap[pos.x][pos.y][pos.z];
}

bool CGameState::isVisible( const CGObjectInstance *obj, int player )
{
	if(player == 255) //neutral player
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
std::pair<ui32, ui32> BattleInfo::calculateDmgRange( const CStack* attacker, const CStack* defender, const CGHeroInstance * attackerHero, const CGHeroInstance * defendingHero, bool shooting, ui8 charge, bool lucky )
{
	float additiveBonus=1.0f, multBonus=1.0f,
		minDmg = attacker->type->damageMin * attacker->count, 
		maxDmg = attacker->type->damageMax * attacker->count;

	if(attacker->type->idNumber == 149) //arrow turret
	{
		switch(attacker->position)
		{
		case -2: //keep
			minDmg = 15;
			maxDmg = 15;
			break;
		case -3: case -4: //turrets
			minDmg = 7.5f;
			maxDmg = 7.5f;
			break;
		}
	}

	if(attacker->hasBonusOfType(Bonus::SIEGE_WEAPON) && attacker->type->idNumber != 149) //any siege weapon, but only ballista can attack (second condition - not arrow turret)
	{ //minDmg and maxDmg are multiplied by hero attack + 1
		minDmg *= attackerHero->getPrimSkillLevel(0) + 1; 
		maxDmg *= attackerHero->getPrimSkillLevel(0) + 1; 
	}

	int attackDefenceDifference = 0;
	if(attacker->hasBonusOfType(Bonus::GENERAL_ATTACK_REDUCTION))
	{
		float multAttackReduction = attacker->valOfBonuses(Bonus::GENERAL_ATTACK_REDUCTION, -1024) / 100.0f;
		attackDefenceDifference = attacker->Attack() * multAttackReduction;
	}
	else
	{
		attackDefenceDifference = attacker->Attack();
	}

	if(attacker->hasBonusOfType(Bonus::ENEMY_DEFENCE_REDUCTION))
	{
		float multDefenceReduction = (100.0f - attacker->valOfBonuses(Bonus::ENEMY_DEFENCE_REDUCTION, -1024)) / 100.0f;
		attackDefenceDifference -= defender->Defense() * multDefenceReduction;
	}
	else
	{
		attackDefenceDifference -= defender->Defense();
	}

	//calculating total attack/defense skills modifier

	if(shooting) //precision handling (etc.)
		attackDefenceDifference += attacker->getBonuses(Selector::typeSybtype(Bonus::PRIMARY_SKILL, PrimarySkill::ATTACK), Selector::effectRange(Bonus::ONLY_DISTANCE_FIGHT)).totalValue();
	else //bloodlust handling (etc.)
		attackDefenceDifference += attacker->getBonuses(Selector::typeSybtype(Bonus::PRIMARY_SKILL, PrimarySkill::ATTACK), Selector::effectRange(Bonus::ONLY_MELEE_FIGHT)).totalValue();


	if(attacker->getEffect(55)) //slayer handling
	{
		std::vector<int> affectedIds;
		int spLevel = attacker->getEffect(55)->level;

		for(int g = 0; g < VLC->creh->creatures.size(); ++g)
		{
			BOOST_FOREACH(const Bonus &b, VLC->creh->creatures[g]->bonuses)
			{
				if ( (b.type == Bonus::KING3 && spLevel >= 3) || //expert
					(b.type == Bonus::KING2 && spLevel >= 2) || //adv +
					(b.type == Bonus::KING1 && spLevel >= 0) ) //none or basic +
				{
					affectedIds.push_back(g);
					break;
				}
			}
		}

		for(unsigned int g=0; g<affectedIds.size(); ++g)
		{
			if(defender->type->idNumber == affectedIds[g])
			{
				attackDefenceDifference += VLC->spellh->spells[55].powers[attacker->getEffect(55)->level];
				break;
			}
		}
	}

	//bonus from attack/defense skills
	if(attackDefenceDifference < 0) //decreasing dmg
	{
		float dec = 0.025f * (-attackDefenceDifference);
		if(dec > 0.7f)
		{
			multBonus *= 0.3f; //1.0 - 0.7
		}
		else
		{
			multBonus *= 1.0f - dec;
		}
	}
	else //increasing dmg
	{
		float inc = 0.05f * attackDefenceDifference;
		if(inc > 4.0f)
		{
			additiveBonus += 4.0f;
		}
		else
		{
			additiveBonus += inc;
		}
	}
	

	//applying jousting bonus
	if( attacker->hasBonusOfType(Bonus::JOUSTING) && !defender->hasBonusOfType(Bonus::CHARGE_IMMUNITY) )
		additiveBonus += charge * 0.05f;

	
	//handling secondary abilities and artifacts giving premies to them
	if(attackerHero)
	{
		if(shooting)
		{
			switch(attackerHero->getSecSkillLevel(1)) //archery
			{
			case 1: //basic
				additiveBonus += 0.1f;
				break;
			case 2: //advanced
				additiveBonus += 0.25f;
				break;
			case 3: //expert
				additiveBonus += 0.5f;
				break;
			}

			if(attackerHero->getSecSkillLevel(1) > 0) //non-none level
			{
				//apply artifact premy to archery
				additiveBonus += attackerHero->valOfBonuses(Bonus::SECONDARY_SKILL_PREMY, 1) / 100.0f;
			}
		}
		else
		{
			switch(attackerHero->getSecSkillLevel(22)) //offense
			{
			case 1: //basic
				additiveBonus += 0.1f;
				break;
			case 2: //advanced
				additiveBonus += 0.2f;
				break;
			case 3: //expert
				additiveBonus += 0.3f;
				break;
			}
		}
	}

	if(defendingHero)
	{
		switch(defendingHero->getSecSkillLevel(23)) //armorer
		{
		case 1: //basic
			multBonus *= 0.95f;
			break;
		case 2: //advanced
			multBonus *= 0.9f;
			break;
		case 3: //expert
			multBonus *= 0.85f;
			break;
		}
	}

	//handling hate effect
	if( attacker->hasBonusOfType(Bonus::HATE, defender->type->idNumber) )
		additiveBonus += 0.5f;

	//luck bonus
	if (lucky)
	{
		additiveBonus += 1.0f;
	}

	//handling spell effects
	if(!shooting && defender->hasBonusOfType(Bonus::GENERAL_DAMAGE_REDUCTION, 0)) //eg. shield
	{
		multBonus *= float(defender->valOfBonuses(Bonus::GENERAL_DAMAGE_REDUCTION, 0)) / 100.0f;
	}
	else if(shooting && defender->hasBonusOfType(Bonus::GENERAL_DAMAGE_REDUCTION, 1)) //eg. air shield
	{
		multBonus *= float(defender->valOfBonuses(Bonus::GENERAL_DAMAGE_REDUCTION, 1)) / 100.0f;
	}
	if(attacker->getEffect(42)) //curse handling (partial, the rest is below)
	{
		multBonus *= 0.8f * float(VLC->spellh->spells[42].powers[attacker->getEffect(42)->level]); //the second factor is 1 or 0
	}

	class HLP
	{
	public:
		static bool hasAdvancedAirShield(const CStack * stack)
		{
			for(int g=0; g<stack->effects.size(); ++g)
			{
				if (stack->effects[g].id == 28 && stack->effects[g].level >= 2)
				{
					return true;
				}
			}
			return false;
		}
	};

	//wall / distance penalty + advanced air shield
	if (shooting && !NBonus::hasOfType(attackerHero, Bonus::NO_SHOTING_PENALTY) && (
		hasDistancePenalty(attacker->ID, defender->position) || hasWallPenalty(attacker->ID, defender->position) ||
		HLP::hasAdvancedAirShield(defender) )
		)
	{
		multBonus *= 0.5;
	}
	if (!shooting && attacker->hasBonusOfType(Bonus::SHOOTER) && !attacker->hasBonusOfType(Bonus::NO_MELEE_PENALTY))
	{
		multBonus *= 0.5;
	}

	minDmg *= additiveBonus * multBonus;
	maxDmg *= additiveBonus * multBonus;

	std::pair<ui32, ui32> returnedVal;

	if(attacker->getEffect(42)) //curse handling (rest)
	{
		minDmg -= VLC->spellh->spells[42].powers[attacker->getEffect(42)->level];
		returnedVal = std::make_pair(int(minDmg), int(minDmg));
	}
	else if(attacker->getEffect(41)) //bless handling
	{
		maxDmg += VLC->spellh->spells[41].powers[attacker->getEffect(41)->level];
		returnedVal =  std::make_pair(int(maxDmg), int(maxDmg));
	}
	else
	{
		returnedVal =  std::make_pair(int(minDmg), int(maxDmg));
	}

	//damage cannot be less than 1
	amax(returnedVal.first, 1);
	amax(returnedVal.second, 1);

	return returnedVal;
}

ui32 BattleInfo::calculateDmg( const CStack* attacker, const CStack* defender, const CGHeroInstance * attackerHero, const CGHeroInstance * defendingHero, bool shooting, ui8 charge, bool lucky )
{
	std::pair<ui32, ui32> range = calculateDmgRange(attacker, defender, attackerHero, defendingHero, shooting, charge, lucky);

	if(range.first != range.second)
	{
		int valuesToAverage[10];
		int howManyToAv = std::min<ui32>(10, attacker->count);
		for (int g=0; g<howManyToAv; ++g)
		{
			valuesToAverage[g] = range.first  +  rand() % (range.second - range.first + 1);
		}

		return std::accumulate(valuesToAverage, valuesToAverage + howManyToAv, 0) / howManyToAv;
	}
	else
		return range.first;
}

void BattleInfo::calculateCasualties( std::map<ui32,si32> *casualties ) const
{
	for(unsigned int i=0; i<stacks.size();i++)//setting casualties
	{
		const CStack * const st = stacks[i];
		si32 killed = (st->alive() ? st->baseAmount - st->count : st->baseAmount);
		amax(killed, 0);
		if(killed)
			casualties[!st->attackerOwned][st->type->idNumber] += killed;
	}
}

si8 CGameState::battleMaxSpellLevel()
{
	if(!curB) //there is not battle
	{
		tlog1 << "si8 CGameState::maxSpellLevel() call when there is no battle!" << std::endl;
		throw "si8 CGameState::maxSpellLevel() call when there is no battle!";
	}

	si8 levelLimit = SPELL_LEVELS;

	const CGHeroInstance *h1 =  curB->heroes[0];
	if(h1)
	{
		for(std::list<Bonus>::const_iterator i = h1->bonuses.begin(); i != h1->bonuses.end(); i++)
			if(i->type == Bonus::BLOCK_SPELLS_ABOVE_LEVEL)
				amin(levelLimit, i->val);
	}

	const CGHeroInstance *h2 = curB->heroes[1];
	if(h2)
	{
		for(std::list<Bonus>::const_iterator i = h2->bonuses.begin(); i != h2->bonuses.end(); i++)
			if(i->type == Bonus::BLOCK_SPELLS_ABOVE_LEVEL)
				amin(levelLimit, i->val);
	}

	return levelLimit;
}

std::set<CStack*> BattleInfo::getAttackedCreatures( const CSpell * s, int skillLevel, ui8 attackerOwner, int destinationTile )
{
	std::set<ui16> attackedHexes = s->rangeInHexes(destinationTile, skillLevel);
	std::set<CStack*> attackedCres; /*std::set to exclude multiple occurrences of two hex creatures*/

	bool onlyAlive = s->id != 38 && s->id != 39; //when casting resurrection or animate dead we should be allow to select dead stack

	if(s->id == 24 || s->id == 25 || s->id == 26) //death ripple, destroy undead and Armageddon
	{
		for(int it=0; it<stacks.size(); ++it)
		{
			if((s->id == 24 && !stacks[it]->type->isUndead()) //death ripple
				|| (s->id == 25 && stacks[it]->type->isUndead()) //destroy undead
				|| (s->id == 26) //Armageddon
				)
			{
				if(stacks[it]->alive())
					attackedCres.insert(stacks[it]);
			}
		}
	}
	else if(VLC->spellh->spells[s->id].attributes.find("CREATURE_TARGET_1") != std::string::npos
		|| VLC->spellh->spells[s->id].attributes.find("CREATURE_TARGET_2") != std::string::npos) //spell to be cast on a specific creature but massive on expert
	{
		if(skillLevel < 3)  /*not expert */
		{
			CStack * st = getStackT(destinationTile, onlyAlive);
			if(st)
				attackedCres.insert(st);
		}
		else
		{
			for(int it=0; it<stacks.size(); ++it)
			{
				/*if it's non negative spell and our unit or non positive spell and hostile unit */
				if((VLC->spellh->spells[s->id].positiveness >= 0 && stacks[it]->owner == attackerOwner)
					||(VLC->spellh->spells[s->id].positiveness <= 0 && stacks[it]->owner != attackerOwner )
					)
				{
					if(!onlyAlive || stacks[it]->alive())
						attackedCres.insert(stacks[it]);
				}
			}
		} //if(caster->getSpellSchoolLevel(s) < 3)
	}
	else if(VLC->spellh->spells[s->id].attributes.find("CREATURE_TARGET") != std::string::npos) //spell to be cast on one specific creature
	{
		CStack * st = getStackT(destinationTile, onlyAlive);
		if(st)
			attackedCres.insert(st);
	}
	else //custom range from attackedHexes
	{
		for(std::set<ui16>::iterator it = attackedHexes.begin(); it != attackedHexes.end(); ++it)
		{
			CStack * st = getStackT(*it, onlyAlive);
			if(st)
				attackedCres.insert(st);
		}
	}
	return attackedCres;
}

int BattleInfo::calculateSpellDuration( const CSpell * spell, const CGHeroInstance * caster, int usedSpellPower )
{
	if(!caster) //TODO: something better
		return std::max(5, usedSpellPower);
	switch(spell->id)
	{
	case 56: //frenzy
		return 1;
	default: //other spells
		return caster->getPrimSkillLevel(2) + caster->valOfBonuses(Bonus::SPELL_DURATION);
	}
}

CStack * BattleInfo::generateNewStack(const CStackInstance &base, int stackID, bool attackerOwned, int slot, int /*TerrainTile::EterrainType*/ terrain, int position) const
{
	CStack * ret = new CStack(&base, attackerOwned ? side1 : side2, stackID, attackerOwned, slot);

	//native terrain bonuses
	int faction = ret->type->faction;
	if(faction >= 0 && VLC->heroh->nativeTerrains[faction] == terrain)
	{
		ret->bonuses.push_back(makeFeature(Bonus::STACKS_SPEED, Bonus::ONE_BATTLE, 0, 1, Bonus::TERRAIN_NATIVE));
		ret->bonuses.push_back(makeFeature(Bonus::PRIMARY_SKILL, Bonus::ONE_BATTLE, PrimarySkill::ATTACK, 1, Bonus::TERRAIN_NATIVE));
		ret->bonuses.push_back(makeFeature(Bonus::PRIMARY_SKILL, Bonus::ONE_BATTLE, PrimarySkill::DEFENSE, 1, Bonus::TERRAIN_NATIVE));
	}

	ret->position = position;

	return ret;
}

ui32 BattleInfo::getSpellCost(const CSpell * sp, const CGHeroInstance * caster) const
{
	ui32 ret = caster->getSpellCost(sp);

	//checking for friendly stacks reducing cost of the spell and
	//enemy stacks increasing it
	si32 manaReduction = 0;
	si32 manaIncrease = 0;
	for(int g=0; g<stacks.size(); ++g)
	{
		if( stacks[g]->owner == caster->tempOwner && stacks[g]->hasBonusOfType(Bonus::CHANGES_SPELL_COST_FOR_ALLY) )
		{
			amin(manaReduction, stacks[g]->valOfBonuses(Bonus::CHANGES_SPELL_COST_FOR_ALLY));
		}
		if( stacks[g]->owner != caster->tempOwner && stacks[g]->hasBonusOfType(Bonus::CHANGES_SPELL_COST_FOR_ENEMY) )
		{
			amax(manaIncrease, stacks[g]->valOfBonuses(Bonus::CHANGES_SPELL_COST_FOR_ENEMY));
		}
	}

	return ret + manaReduction + manaIncrease;
}

int BattleInfo::hexToWallPart(int hex) const
{
	if(siege == 0) //there is no battle!
		return -1;

	static const std::pair<int, int> attackable[] = //potentially attackable parts of wall
	{std::make_pair(50, 0), std::make_pair(183, 1), std::make_pair(182, 2), std::make_pair(130, 3),
	std::make_pair(62, 4), std::make_pair(29, 5), std::make_pair(12, 6), std::make_pair(95, 7), std::make_pair(96, 7)};

	for(int g = 0; g < ARRAY_COUNT(attackable); ++g)
	{
		if(attackable[g].first == hex)
			return attackable[g].second;
	}

	return -1; //not found!
}

int BattleInfo::lineToWallHex( int line ) const
{
	static const int lineToHex[] = {12, 29, 45, 62, 78, 95, 112, 130, 147, 165, 182};

	return lineToHex[line];
}

std::pair<const CStack *, int> BattleInfo::getNearestStack(const CStack * closest, boost::logic::tribool attackerOwned) const
{	
	bool ac[BFIELD_SIZE];
	std::set<int> occupyable;

	getAccessibilityMap(ac, closest->doubleWide(), closest->attackerOwned, false, occupyable, closest->hasBonusOfType(Bonus::FLYING), closest->ID);

	int predecessor[BFIELD_SIZE], dist[BFIELD_SIZE];
	makeBFS(closest->position, ac, predecessor, dist, closest->doubleWide(), closest->attackerOwned, closest->hasBonusOfType(Bonus::FLYING), true);

	std::vector< std::pair< std::pair<int, int>, const CStack *> > stackPairs; //pairs <<distance, hex>, stack>
	for(int g=0; g<BFIELD_SIZE; ++g)
	{
		const CStack * atG = getStackT(g);
		if(!atG || atG->ID == closest->ID) //if there is not stack or we are the closest one
			continue;
		if(boost::logic::indeterminate(attackerOwned) || atG->attackerOwned == attackerOwned)
		{
			if(predecessor[g] == -1) //TODO: is it really the best solution?
				continue;
			stackPairs.push_back( std::make_pair( std::make_pair(dist[predecessor[g]], g), atG) );
		}
	}

	if(stackPairs.size() > 0)
	{
		std::vector< std::pair< std::pair<int, int>, const CStack *> > minimalPairs;
		minimalPairs.push_back(stackPairs[0]);
	
		for(int b=1; b<stackPairs.size(); ++b)
		{
			if(stackPairs[b].first.first < minimalPairs[0].first.first)
			{
				minimalPairs.clear();
				minimalPairs.push_back(stackPairs[b]);
			}
			else if(stackPairs[b].first.first == minimalPairs[0].first.first)
			{
				minimalPairs.push_back(stackPairs[b]);
			}
		}

		std::pair< std::pair<int, int>, const CStack *> minPair = minimalPairs[minimalPairs.size()/2];

		return std::make_pair(minPair.second, predecessor[minPair.first.second]);
	}

	return std::make_pair<const CStack * , int>(NULL, -1);
}

ui32 BattleInfo::calculateSpellDmg( const CSpell * sp, const CGHeroInstance * caster, const CStack * affectedCreature, int spellSchoolLevel, int usedSpellPower ) const
{
	ui32 ret = 0; //value to return

	//15 - magic arrows, 16 - ice bolt, 17 - lightning bolt, 18 - implosion, 20 - frost ring, 21 - fireball, 22 - inferno, 23 - meteor shower,
	//24 - death ripple, 25 - destroy undead, 26 - armageddon, 77 - thunderbolt
	static std::map <int, int> dmgMultipliers = boost::assign::map_list_of(15, 10)(16, 20)(17, 25)(18, 75)(20, 10)(21, 10)(22, 10)(23, 10)(24, 5)(25, 10)(26, 50)(77, 10);

	//check if spell really does damage - if not, return 0
	if(dmgMultipliers.find(sp->id) == dmgMultipliers.end())
		return 0;


	ret = usedSpellPower * dmgMultipliers[sp->id];
	ret += sp->powers[spellSchoolLevel];
	
	//applying sorcerery secondary skill
	if(caster)
	{
		switch(caster->getSecSkillLevel(25))
		{
		case 1: //basic
			ret *= 1.05f;
			break;
		case 2: //advanced
			ret *= 1.1f;
			break;
		case 3: //expert
			ret *= 1.15f;
			break;
		}
	}
	//applying hero bonuses
	if(sp->air && caster && caster->valOfBonuses(Bonus::AIR_SPELL_DMG_PREMY) != 0)
	{
		ret *= (100.0f + caster->valOfBonuses(Bonus::AIR_SPELL_DMG_PREMY)) / 100.0f;
	}
	else if(sp->fire && caster && caster->valOfBonuses(Bonus::FIRE_SPELL_DMG_PREMY) != 0)
	{
		ret *= (100.0f + caster->valOfBonuses(Bonus::FIRE_SPELL_DMG_PREMY)) / 100.0f;
	}
	else if(sp->water && caster && caster->valOfBonuses(Bonus::WATER_SPELL_DMG_PREMY) != 0)
	{
		ret *= (100.0f + caster->valOfBonuses(Bonus::WATER_SPELL_DMG_PREMY)) / 100.0f;
	}
	else if(sp->earth && caster && caster->valOfBonuses(Bonus::EARTH_SPELL_DMG_PREMY) != 0)
	{
		ret *= (100.0f + caster->valOfBonuses(Bonus::EARTH_SPELL_DMG_PREMY)) / 100.0f;
	}

	//affected creature-specific part
	if(affectedCreature)
	{
		//applying protections - when spell has more then one elements, only one protection should be applied (I think)
		if(sp->air && affectedCreature->hasBonusOfType(Bonus::SPELL_DAMAGE_REDUCTION, 0)) //air spell & protection from air
		{
			ret *= affectedCreature->valOfBonuses(Bonus::SPELL_DAMAGE_REDUCTION, 0);
			ret /= 100;
		}
		else if(sp->fire && affectedCreature->hasBonusOfType(Bonus::SPELL_DAMAGE_REDUCTION, 1)) //fire spell & protection from fire
		{
			ret *= affectedCreature->valOfBonuses(Bonus::SPELL_DAMAGE_REDUCTION, 1);
			ret /= 100;
		}
		else if(sp->water && affectedCreature->hasBonusOfType(Bonus::SPELL_DAMAGE_REDUCTION, 2)) //water spell & protection from water
		{
			ret *= affectedCreature->valOfBonuses(Bonus::SPELL_DAMAGE_REDUCTION, 2);
			ret /= 100;
		}
		else if (sp->earth && affectedCreature->hasBonusOfType(Bonus::SPELL_DAMAGE_REDUCTION, 3)) //earth spell & protection from earth
		{
			ret *= affectedCreature->valOfBonuses(Bonus::SPELL_DAMAGE_REDUCTION, 3);
			ret /= 100;
		}

		//general spell dmg reduction
		if(sp->air && affectedCreature->hasBonusOfType(Bonus::SPELL_DAMAGE_REDUCTION, -1)) //air spell & protection from air
		{
			ret *= affectedCreature->valOfBonuses(Bonus::SPELL_DAMAGE_REDUCTION, -1);
			ret /= 100;
		}

		//dmg increasing
		if( affectedCreature->hasBonusOfType(Bonus::MORE_DAMAGE_FROM_SPELL, sp->id) )
		{
			ret *= 100 + affectedCreature->valOfBonuses(Bonus::MORE_DAMAGE_FROM_SPELL, sp->id);
			ret /= 100;
		}
	}

	return ret;
}

bool CGameState::battleCanShoot(int ID, int dest)
{
	if(!curB)
		return false;

	const CStack *our = curB->getStack(ID),
		*dst = curB->getStackT(dest);

	if(!our || !dst) return false;

	const CGHeroInstance * ourHero = battleGetOwner(our->ID);

	if(our->hasBonusOfType(Bonus::FORGETFULL)) //forgetfulness
		return false;

	if(our->type->idNumber == 145 && dst) //catapult cannot attack creatures
		return false;

	if(our->hasBonusOfType(Bonus::SHOOTER)//it's shooter
		&& our->owner != dst->owner
		&& dst->alive()
		&& (!curB->isStackBlocked(ID)  ||  NBonus::hasOfType(ourHero, Bonus::FREE_SHOOTING))
		&& our->shots
		)
		return true;
	return false;
}

int CGameState::victoryCheck( ui8 player ) const
{
	const PlayerState *p = getPlayer(player);
	if(map->victoryCondition.condition == winStandard  ||  map->victoryCondition.allowNormalVictory)
		if(player == checkForStandardWin())
			return -1;

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
						&&  (ai = dynamic_cast<const CArmedInstance*>(map->objects[i]))) //contains army
					{
						for(TSlots::const_iterator i=ai->Slots().begin(); i!=ai->Slots().end(); ++i) //iterate through army
							if(i->second.type->idNumber == map->victoryCondition.ID) //it's searched creature
								total += i->second.count;
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
				if(t->visitingHero && t->visitingHero->hasArt(map->victoryCondition.ID)
					|| t->garrisonHero && t->garrisonHero->hasArt(map->victoryCondition.ID))
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
				winnerTeam = map->players[supposedWinner].team;
			}
			else if(winnerTeam != map->players[i->second.color].team)
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
		std::vector<CGHeroInstance *> &h = gs->players[color].heroes;
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
			ret += ps->heroes[g]->artifacts.size() + ps->heroes[g]->artifWorn.size();
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
				tgi.personality[g->second.color] = map->players[g->second.serial].AITactic;
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
					int toCmp = it->second.type->idNumber; //ID of creature we should compare with the best one
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

const CStack * BattleInfo::getNextStack() const
{
	std::vector<const CStack *> hlp;
	getStackQueue(hlp, 1, -1);

	if(hlp.size())
		return hlp[0];
	else
		return NULL;
}

static const CStack *takeStack(std::vector<const CStack *> &st, int &curside, int turn)
{
	const CStack *ret = NULL;
	unsigned i, //fastest stack
		j; //fastest stack of the other side
	for(i = 0; i < st.size(); i++)
		if(st[i])
			break;

	//no stacks left
	if(i == st.size())
		return NULL;

	const CStack *fastest = st[i], *other = NULL;
	int bestSpeed = fastest->Speed(turn);

	if(fastest->attackerOwned != curside)
	{
		ret = fastest;
	}
	else
	{
		for(j = i + 1; j < st.size(); j++)
		{
			if(!st[j]) continue;
			if(st[j]->attackerOwned != curside || st[j]->Speed(turn) != bestSpeed)
				break;
		}

		if(j >= st.size())
		{
			ret = fastest;
		}
		else
		{
			other = st[j];
			if(other->Speed(turn) != bestSpeed)
				ret = fastest;
			else
				ret = other;
		}
	}

	assert(ret);
	if(ret == fastest)
		st[i] = NULL;
	else
		st[j] = NULL;

	curside = ret->attackerOwned;
	return ret;
}

void BattleInfo::getStackQueue( std::vector<const CStack *> &out, int howMany, int turn /*= 0*/, int lastMoved /*= -1*/ ) const
{
	//we'll split creatures with remaining movement to 4 parts
	std::vector<const CStack *> phase[4]; //0 - turrets/catapult, 1 - normal (unmoved) creatures, other war machines, 2 - waited cres that had morale, 3 - rest of waited cres
	int toMove = 0; //how many stacks still has move
	const CStack *active = getStack(activeStack);

	//active stack hasn't taken any action yet - must be placed at the beginning of queue, no matter what
	if(!turn && active && active->willMove() && !vstd::contains(active->state, WAITING))
	{
		out.push_back(active);
		if(out.size() == howMany)
			return;
	}


	for(unsigned int i=0; i<stacks.size(); ++i)
	{
		const CStack * const s = stacks[i];
		if(turn <= 0 && !s->willMove() //we are considering current round and stack won't move
			|| turn > 0 && !s->canMove(turn) //stack won't be able to move in later rounds
			|| turn <= 0 && s == active && out.size() && s == out.front()) //it's active stack already added at the beginning of queue
		{
			continue;
		}

		int p = -1; //in which phase this tack will move?
		if(turn <= 0 && vstd::contains(s->state, WAITING)) //consider waiting state only for ongoing round
		{
			if(vstd::contains(s->state, HAD_MORALE))
				p = 2;
			else
				p = 3;
		}
		else if(s->type->idNumber == 145  ||  s->type->idNumber == 149) //catapult and turrets are first
		{
			p = 0;
		}
		else
		{
			p = 1;
		}

		phase[p].push_back(s);
		toMove++;
	}

	for(int i = 0; i < 4; i++)
		std::sort(phase[i].begin(), phase[i].end(), CMP_stack(i, turn > 0 ? turn : 0));

	for(size_t i = 0; i < phase[0].size() && i < howMany; i++)
		out.push_back(phase[0][i]);

	if(out.size() == howMany)
		return;

	if(lastMoved == -1)
	{
		if(active)
		{
			if(out.size() && out.front() == active)
				lastMoved = active->attackerOwned;
			else
				lastMoved = active->attackerOwned;
		}
		else
		{
			lastMoved = 0;
		}
	}

	int pi = 1;
	while(out.size() < howMany)
	{
		const CStack *hlp = takeStack(phase[pi], lastMoved, turn);
		if(!hlp)
		{
			pi++;
			if(pi > 3)
			{
				//if(turn != 2)
					getStackQueue(out, howMany, turn + 1, lastMoved);
				return;
			}
		}
		else
		{
			out.push_back(hlp);
		}
	}
}

si8 BattleInfo::hasDistancePenalty( int stackID, int destHex )
{
	const CStack * stack = getStack(stackID);

	int distance = std::abs(destHex % BFIELD_WIDTH - stack->position % BFIELD_WIDTH);

	//I hope it's approximately correct
	return distance > 8 && !stack->hasBonusOfType(Bonus::NO_DISTANCE_PENALTY);
}

si8 BattleInfo::sameSideOfWall(int pos1, int pos2)
{
	int wallInStackLine = lineToWallHex(pos1/BFIELD_WIDTH);
	int wallInDestLine = lineToWallHex(pos2/BFIELD_WIDTH);

	bool stackLeft = pos1 < wallInStackLine;
	bool destLeft = pos2 < wallInDestLine;

	return stackLeft != destLeft;
}

si8 BattleInfo::hasWallPenalty( int stackID, int destHex )
{
	if (siege == 0)
	{
		return false;
	}
	const CStack * stack = getStack(stackID);
	if (stack->hasBonusOfType(Bonus::NO_WALL_PENALTY));
	{
		return false;
	}

	return !sameSideOfWall(stack->position, destHex);
}

si8 BattleInfo::canTeleportTo(int stackID, int destHex, int telportLevel)
{
	bool ac[BFIELD_SIZE];
	const CStack *s = getStack(stackID, false); //this function is called from healedOrResurrected, so our stack can be dead

	std::set<int> occupyable;

	getAccessibilityMap(ac, s->doubleWide(), s->attackerOwned, false, occupyable, s->hasBonusOfType(Bonus::FLYING), stackID);

	if (siege && telportLevel < 2) //check for wall
	{
		return ac[destHex] && sameSideOfWall(s->position, destHex);
	}
	else
	{
		return ac[destHex];
	}

}

void BattleInfo::getBonuses(BonusList &out, const CSelector &selector, const CBonusSystemNode *root /*= NULL*/) const
{
	CBonusSystemNode::getBonuses(out, selector, root);

	const CStack *dest = dynamic_cast<const CStack*>(root);
	if (!dest)
		return;


	//TODO: make it in clean way
	if(Selector::matchesType(selector, Bonus::MORALE) || Selector::matchesType(selector, Bonus::LUCK))
	{
		BOOST_FOREACH(const CStack *s, stacks)
		{
			if(s->owner == dest->owner)
				s->getBonuses(out, selector, Selector::effectRange(Bonus::ONLY_ALLIED_ARMY), this);
			else
				s->getBonuses(out, selector, Selector::effectRange(Bonus::ONLY_ENEMY_ARMY), this);
		}
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

bool CMP_stack::operator()( const CStack* a, const CStack* b )
{
	switch(phase)
	{
	case 0: //catapult moves after turrets
		return a->type->idNumber < b->type->idNumber; //catapult is 145 and turrets are 149
		//TODO? turrets order
	case 1: //fastest first, upper slot first
		{
			int as = a->Speed(turn), bs = b->Speed(turn);
			if(as != bs)
				return as > bs;
			else
				return a->slot < b->slot;
		}
	case 2: //fastest last, upper slot first
		//TODO: should be replaced with order of receiving morale!
	case 3: //fastest last, upper slot first
		{
			int as = a->Speed(turn), bs = b->Speed(turn);
			if(as != bs)
				return as < bs;
			else
				return a->slot < b->slot;
		}
	default:
		assert(0);
		return false;
	}

}

CMP_stack::CMP_stack( int Phase /*= 1*/, int Turn )
{
	phase = Phase;
	turn = Turn;
}

PlayerState::PlayerState() 
 : color(-1), currentSelection(0xffffffff), status(INGAME), daysWithoutCastle(0)
{

}

void PlayerState::getParents(TCNodes &out, const CBonusSystemNode *root /*= NULL*/) const
{
	//TODO: global effects
}

void PlayerState::getBonuses(BonusList &out, const CSelector &selector, const CBonusSystemNode *root /*= NULL*/) const
{
}

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
	army = h->getArmy(); 

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
	else
	{
		//hide info about hero stacks counts using descriptives names ids
		for(TSlots::const_iterator i = army.Slots().begin(); i != army.Slots().end(); ++i)
		{
			army.setStackCount(i->first, i->second.getQuantityID()+1);
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

void CCampaignState::initNewCampaign( const StartInfo &si )
{
	assert(si.mode == 2);
	campaignName = si.mapname;
	currentMap = si.whichMapInCampaign;
	
	camp = CCampaignHandler::getCampaign(campaignName, true); //TODO lod???
	for (ui8 i = 0; i < camp->mapPieces.size(); i++)
		mapsRemaining.push_back(i);
}
