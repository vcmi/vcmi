#include "StdInc.h"
#include "CGameState.h"

#include "mapping/CCampaignHandler.h"
#include "mapObjects/CObjectClassesHandler.h"
#include "CArtHandler.h"
#include "CBuildingHandler.h"
#include "CGeneralTextHandler.h"
#include "CTownHandler.h"
#include "spells/CSpellHandler.h"
#include "CHeroHandler.h"
#include "mapObjects/CObjectHandler.h"
#include "CCreatureHandler.h"
#include "CModHandler.h"
#include "VCMI_Lib.h"
#include "Connection.h"
#include "mapping/CMap.h"
#include "mapping/CMapService.h"
#include "StartInfo.h"
#include "NetPacks.h"
#include "registerTypes/RegisterTypes.h"
#include "mapping/CMapInfo.h"
#include "BattleState.h"
#include "JsonNode.h"
#include "filesystem/Filesystem.h"
#include "GameConstants.h"
#include "rmg/CMapGenerator.h"
#include "CStopWatch.h"
#include "mapping/CMapEditManager.h"

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

template <typename T> class CApplyOnGS;

class CBaseForGSApply
{
public:
	virtual void applyOnGS(CGameState *gs, void *pack) const =0;
	virtual ~CBaseForGSApply(){};
	template<typename U> static CBaseForGSApply *getApplier(const U * t=nullptr)
	{
		return new CApplyOnGS<U>;
	}
};

template <typename T> class CApplyOnGS : public CBaseForGSApply
{
public:
	void applyOnGS(CGameState *gs, void *pack) const override
	{
		T *ptr = static_cast<T*>(pack);

		boost::unique_lock<boost::shared_mutex> lock(*gs->mx);
		ptr->applyGs(gs);
	}
};

static CApplier<CBaseForGSApply> *applierGs = nullptr;

// class IObjectCaller
// {
// public:
// 	virtual ~IObjectCaller(){};
// 	virtual void preInit()=0;
// 	virtual void postInit()=0;
// };
//
// template <typename T>
// class CObjectCaller : public IObjectCaller
// {
// public:
// 	void preInit()
// 	{
// 		//T::preInit();
// 	}
// 	void postInit()
// 	{
// 		//T::postInit();
// 	}
// };

// class CObjectCallersHandler
// {
// public:
// 	std::vector<IObjectCaller*> apps;
//
// 	template<typename T> void registerType(const T * t=nullptr)
// 	{
// 		apps.push_back(new CObjectCaller<T>);
// 	}
//
// 	CObjectCallersHandler()
// 	{
// 		registerTypesMapObjects(*this);
// 	}
//
// 	~CObjectCallersHandler()
// 	{
// 		for (auto & elem : apps)
// 			delete elem;
// 	}
//
// 	void preInit()
// 	{
// // 		for (size_t i = 0; i < apps.size(); i++)
// // 			apps[i]->preInit();
// 	}
//
// 	void postInit()
// 	{
// 	//for (size_t i = 0; i < apps.size(); i++)
// 	//apps[i]->postInit();
// 	}
// } *objCaller = nullptr;

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
		dst = SpellID(ser).toSpell()->name;
	}
	else if(type == CRE_SING_NAMES)
	{
		dst = VLC->creh->creatures[ser]->nameSing;
	}
	else if(type == ART_DESCR)
	{
		dst = VLC->arth->artifacts[ser]->Description();
	}
	else if (type == ART_EVNTS)
	{
		dst = VLC->arth->artifacts[ser]->EventText();
	}
	else if (type == OBJ_NAMES)
	{
		dst = VLC->objtypeh->getObjectName(ser);
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
		case SEC_SKILL_NAME:
			vec = &VLC->generaltexth->skillName;
			break;
		case COLOR:
			vec = &VLC->generaltexth->capColors;
			break;
		default:
            logGlobal->errorStream() << "Failed string substitution because type is " << type;
			dst = "#@#";
			return;
		}
		if(vec->size() <= ser)
		{
            logGlobal->errorStream() << "Failed string substitution with type " << type << " because index " << ser << " is out of bounds!";
			dst = "#!#";
		}
		else
			dst = (*vec)[ser];
	}
}

DLL_LINKAGE void MetaString::toString(std::string &dst) const
{
	size_t exSt = 0, loSt = 0, nums = 0;
	dst.clear();

	for(auto & elem : message)
	{//TEXACT_STRING, TLOCAL_STRING, TNUMBER, TREPLACE_ESTRING, TREPLACE_LSTRING, TREPLACE_NUMBER
		switch(elem)
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
			boost::replace_first(dst, "%s", exactStrings[exSt++]);
			break;
		case TREPLACE_LSTRING:
			{
				std::string hlp;
				getLocalString(localStrings[loSt++], hlp);
				boost::replace_first(dst, "%s", hlp);
			}
			break;
		case TREPLACE_NUMBER:
			boost::replace_first(dst, "%d", boost::lexical_cast<std::string>(numbers[nums++]));
			break;
		case TREPLACE_PLUSNUMBER:
			boost::replace_first(dst, "%+d", '+' + boost::lexical_cast<std::string>(numbers[nums++]));
			break;
		default:
			logGlobal->errorStream() << "MetaString processing error! Received message of type " << int(elem);
			break;
		}
	}
}

DLL_LINKAGE std::string MetaString::toString() const
{
	std::string ret;
	toString(ret);
	return ret;
}

DLL_LINKAGE std::string MetaString::buildList () const
///used to handle loot from creature bank
{

	size_t exSt = 0, loSt = 0, nums = 0;
	std::string lista;
	for (int i = 0; i < message.size(); ++i)
	{
		if (i > 0 && (message[i] == TEXACT_STRING || message[i] == TLOCAL_STRING))
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
				logGlobal->errorStream() << "MetaString processing error! Received message of type " << int(message[i]);
		}

	}
	return lista;
}


void  MetaString::addCreReplacement(CreatureID id, TQuantity count) //adds sing or plural name;
{
	if (!count)
		addReplacement (CRE_PL_NAMES, id); //no creatures - just empty name (eg. defeat Angels)
	else if (count == 1)
		addReplacement (CRE_SING_NAMES, id);
	else
		addReplacement (CRE_PL_NAMES, id);
}

void MetaString::addReplacement(const CStackBasicDescriptor &stack)
{
	assert(stack.type); //valid type
	addCreReplacement(stack.type->idNumber, stack.count);
}

static CGObjectInstance * createObject(Obj id, int subid, int3 pos, PlayerColor owner)
{
	CGObjectInstance * nobj;
	switch(id)
	{
	case Obj::HERO:
		{
			auto handler = VLC->objtypeh->getHandlerFor(id, VLC->heroh->heroes[subid]->heroClass->id);
			nobj = handler->create(handler->getTemplates().front());
			break;
		}
	case Obj::TOWN:
		nobj = new CGTownInstance;
		break;
	default: //rest of objects
		nobj = new CGObjectInstance;
		break;
	}
	nobj->ID = id;
	nobj->subID = subid;
	nobj->pos = pos;
	nobj->tempOwner = owner;
	if (id != Obj::HERO)
		nobj->appearance = VLC->objtypeh->getHandlerFor(id, subid)->getTemplates().front();

	return nobj;
}

CGHeroInstance * CGameState::HeroesPool::pickHeroFor(bool native, PlayerColor player, const CTown *town,
	std::map<ui32, ConstTransitivePtr<CGHeroInstance> > &available, CRandomGenerator & rand, const CHeroClass * bannedClass /*= nullptr*/) const
{
	CGHeroInstance *ret = nullptr;

	if(player>=PlayerColor::PLAYER_LIMIT)
	{
        logGlobal->errorStream() << "Cannot pick hero for " << town->faction->index << ". Wrong owner!";
		return nullptr;
	}

	std::vector<CGHeroInstance *> pool;

	if(native)
	{
		for(auto & elem : available)
		{
			if(pavailable.find(elem.first)->second & 1<<player.getNum()
				&& elem.second->type->heroClass->faction == town->faction->index)
			{
				pool.push_back(elem.second); //get all available heroes
			}
		}
		if(!pool.size())
		{
            logGlobal->errorStream() << "Cannot pick native hero for " << player << ". Picking any...";
			return pickHeroFor(false, player, town, available, rand);
		}
		else
		{
			ret = *RandomGeneratorUtil::nextItem(pool, rand);
		}
	}
	else
	{
		int sum=0, r;

		for(auto & elem : available)
		{
			if (pavailable.find(elem.first)->second & (1<<player.getNum()) &&    // hero is available
			    ( !bannedClass || elem.second->type->heroClass != bannedClass) ) // and his class is not same as other hero
			{
				pool.push_back(elem.second);
				sum += elem.second->type->heroClass->selectionProbability[town->faction->index]; //total weight
			}
		}
		if(!pool.size() || sum == 0)
		{
            logGlobal->errorStream() << "There are no heroes available for player " << player<<"!";
			return nullptr;
		}

		r = rand.nextInt(sum - 1);
		for (auto & elem : pool)
		{
			r -= elem->type->heroClass->selectionProbability[town->faction->index];
			if(r < 0)
			{
				ret = elem;
				break;
			}
		}
		if(!ret)
			ret = pool.back();
	}

	available.erase(ret->subID);
	return ret;
}

void CGameState::CrossoverHeroesList::addHeroToBothLists(CGHeroInstance * hero)
{
	heroesFromPreviousScenario.push_back(hero);
	heroesFromAnyPreviousScenarios.push_back(hero);
}

void CGameState::CrossoverHeroesList::removeHeroFromBothLists(CGHeroInstance * hero)
{
	heroesFromPreviousScenario -= hero;
	heroesFromAnyPreviousScenarios -= hero;
}

CGameState::CampaignHeroReplacement::CampaignHeroReplacement(CGHeroInstance * hero, ObjectInstanceID heroPlaceholderId) : hero(hero), heroPlaceholderId(heroPlaceholderId)
{

}

int CGameState::pickNextHeroType(PlayerColor owner)
{
	const PlayerSettings &ps = scenarioOps->getIthPlayersSettings(owner);
	if(ps.hero >= 0 && !isUsedHero(HeroTypeID(ps.hero))) //we haven't used selected hero
	{
		return ps.hero;
	}

	return pickUnusedHeroTypeRandomly(owner);
}

int CGameState::pickUnusedHeroTypeRandomly(PlayerColor owner)
{
	//list of available heroes for this faction and others
	std::vector<HeroTypeID> factionHeroes, otherHeroes;

	const PlayerSettings &ps = scenarioOps->getIthPlayersSettings(owner);
	for(HeroTypeID hid : getUnusedAllowedHeroes())
	{
		if(VLC->heroh->heroes[hid.getNum()]->heroClass->faction == ps.castle)
			factionHeroes.push_back(hid);
		else
			otherHeroes.push_back(hid);
	}

	// select random hero native to "our" faction
	if(!factionHeroes.empty())
	{
		return RandomGeneratorUtil::nextItem(factionHeroes, rand)->getNum();
	}

	logGlobal->warnStream() << "Cannot find free hero of appropriate faction for player " << owner << " - trying to get first available...";
	if(!otherHeroes.empty())
	{
		return RandomGeneratorUtil::nextItem(otherHeroes, rand)->getNum();
	}

	logGlobal->errorStream() << "No free allowed heroes!";
	auto notAllowedHeroesButStillBetterThanCrash = getUnusedAllowedHeroes(true);
	if(notAllowedHeroesButStillBetterThanCrash.size())
		return notAllowedHeroesButStillBetterThanCrash.begin()->getNum();

	logGlobal->errorStream() << "No free heroes at all!";
	assert(0); //current code can't handle this situation
	return -1; // no available heroes at all
}

std::pair<Obj,int> CGameState::pickObject (CGObjectInstance *obj)
{
	switch(obj->ID)
	{
	case Obj::RANDOM_ART:
		return std::make_pair(Obj::ARTIFACT, VLC->arth->pickRandomArtifact(rand, CArtifact::ART_TREASURE | CArtifact::ART_MINOR | CArtifact::ART_MAJOR | CArtifact::ART_RELIC));
	case Obj::RANDOM_TREASURE_ART:
		return std::make_pair(Obj::ARTIFACT, VLC->arth->pickRandomArtifact(rand, CArtifact::ART_TREASURE));
	case Obj::RANDOM_MINOR_ART:
		return std::make_pair(Obj::ARTIFACT, VLC->arth->pickRandomArtifact(rand, CArtifact::ART_MINOR));
	case Obj::RANDOM_MAJOR_ART:
		return std::make_pair(Obj::ARTIFACT, VLC->arth->pickRandomArtifact(rand, CArtifact::ART_MAJOR));
	case Obj::RANDOM_RELIC_ART:
		return std::make_pair(Obj::ARTIFACT, VLC->arth->pickRandomArtifact(rand, CArtifact::ART_RELIC));
	case Obj::RANDOM_HERO:
		return std::make_pair(Obj::HERO, pickNextHeroType(obj->tempOwner));
	case Obj::RANDOM_MONSTER:
		return std::make_pair(Obj::MONSTER, VLC->creh->pickRandomMonster(rand));
	case Obj::RANDOM_MONSTER_L1:
		return std::make_pair(Obj::MONSTER, VLC->creh->pickRandomMonster(rand, 1));
	case Obj::RANDOM_MONSTER_L2:
		return std::make_pair(Obj::MONSTER, VLC->creh->pickRandomMonster(rand, 2));
	case Obj::RANDOM_MONSTER_L3:
		return std::make_pair(Obj::MONSTER, VLC->creh->pickRandomMonster(rand, 3));
	case Obj::RANDOM_MONSTER_L4:
		return std::make_pair(Obj::MONSTER, VLC->creh->pickRandomMonster(rand, 4));
	case Obj::RANDOM_RESOURCE:
		return std::make_pair(Obj::RESOURCE,rand.nextInt(6)); //now it's OH3 style, use %8 for mithril
	case Obj::RANDOM_TOWN:
		{
			PlayerColor align = PlayerColor((static_cast<CGTownInstance*>(obj))->alignment);
			si32 f; // can be negative (for random)
			if(align >= PlayerColor::PLAYER_LIMIT)//same as owner / random
			{
				if(obj->tempOwner >= PlayerColor::PLAYER_LIMIT)
					f = -1; //random
				else
					f = scenarioOps->getIthPlayersSettings(obj->tempOwner).castle;
			}
			else
			{
				f = scenarioOps->getIthPlayersSettings(align).castle;
			}
			if(f<0)
			{
				do
				{
					f = rand.nextInt(VLC->townh->factions.size() - 1);
				}
				while (VLC->townh->factions[f]->town == nullptr); // find playable faction
			}
			return std::make_pair(Obj::TOWN,f);
		}
	case Obj::RANDOM_MONSTER_L5:
		return std::make_pair(Obj::MONSTER, VLC->creh->pickRandomMonster(rand, 5));
	case Obj::RANDOM_MONSTER_L6:
		return std::make_pair(Obj::MONSTER, VLC->creh->pickRandomMonster(rand, 6));
	case Obj::RANDOM_MONSTER_L7:
		return std::make_pair(Obj::MONSTER, VLC->creh->pickRandomMonster(rand, 7));
	case Obj::RANDOM_DWELLING:
	case Obj::RANDOM_DWELLING_LVL:
	case Obj::RANDOM_DWELLING_FACTION:
		{
			CGDwelling * dwl = static_cast<CGDwelling*>(obj);
			int faction;

			//if castle alignment available
			if (auto info = dynamic_cast<CCreGenAsCastleInfo*>(dwl->info))
			{
				faction = rand.nextInt(VLC->townh->factions.size() - 1);
				if (info->asCastle)
				{
					for(auto & elem : map->objects)
					{
						if(!elem)
							continue;

						if(elem->ID==Obj::RANDOM_TOWN
							&& dynamic_cast<CGTownInstance*>(elem.get())->identifier == info->identifier)
						{
							randomizeObject(elem); //we have to randomize the castle first
							faction = elem->subID;
							break;
						}
						else if(elem->ID==Obj::TOWN
							&& dynamic_cast<CGTownInstance*>(elem.get())->identifier == info->identifier)
						{
							faction = elem->subID;
							break;
						}
					}
				}
				else
				{
					while(!(info->castles[0]&(1<<faction)))
					{
						if((faction>7) && (info->castles[1]&(1<<(faction-8))))
							break;
						faction = rand.nextInt(GameConstants::F_NUMBER - 1);
					}
				}
			}
			else // castle alignment fixed
				faction = obj->subID;

			int level;

			//if level set to range
			if (auto info = dynamic_cast<CCreGenLeveledInfo*>(dwl->info))
			{
				level = rand.nextInt(info->minLevel, info->maxLevel);
			}
			else // fixed level
			{
				level = obj->subID;
			}

			delete dwl->info;
			dwl->info = nullptr;

			std::pair<Obj, int> result(Obj::NO_OBJ, -1);
			CreatureID cid = VLC->townh->factions[faction]->town->creatures[level][0];

			//NOTE: this will pick last dwelling with this creature (Mantis #900)
			//check for block map equality is better but more complex solution
			auto testID = [&](Obj primaryID) -> void
			{
				auto dwellingIDs = VLC->objtypeh->knownSubObjects(primaryID);
				for (si32 entry : dwellingIDs)
				{
					auto handler = dynamic_cast<const CDwellingInstanceConstructor*>(VLC->objtypeh->getHandlerFor(primaryID, entry).get());

					if (handler->producesCreature(VLC->creh->creatures[cid]))
						result = std::make_pair(primaryID, entry);
				}
			};

			testID(Obj::CREATURE_GENERATOR1);
			if (result.first == Obj::NO_OBJ)
				testID(Obj::CREATURE_GENERATOR4);

			if (result.first == Obj::NO_OBJ)
			{
				logGlobal->errorStream() << "Error: failed to find dwelling for "<< VLC->townh->factions[faction]->name << " of level " << int(level);
				result = std::make_pair(Obj::CREATURE_GENERATOR1, *RandomGeneratorUtil::nextItem(VLC->objtypeh->knownSubObjects(Obj::CREATURE_GENERATOR1), rand));
			}

			return result;
		}
	}
	return std::make_pair(Obj::NO_OBJ,-1);
}

void CGameState::randomizeObject(CGObjectInstance *cur)
{
	std::pair<Obj,int> ran = pickObject(cur);
	if(ran.first == Obj::NO_OBJ || ran.second<0) //this is not a random object, or we couldn't find anything
	{
		if(cur->ID==Obj::TOWN)
			cur->setType(cur->ID, cur->subID); // update def, if necessary
	}
	else if(ran.first==Obj::HERO)//special code for hero
	{
		CGHeroInstance *h = dynamic_cast<CGHeroInstance *>(cur);
		cur->setType(ran.first, ran.second);
		map->heroesOnMap.push_back(h);
	}
	else if(ran.first==Obj::TOWN)//special code for town
	{
		CGTownInstance *t = dynamic_cast<CGTownInstance*>(cur);
		cur->setType(ran.first, ran.second);
		map->towns.push_back(t);
	}
	else
	{
		cur->setType(ran.first, ran.second);
	}
}

int CGameState::getDate(Date::EDateType mode) const
{
	int temp;
	switch (mode)
	{
	case Date::DAY:
		return day;
	case Date::DAY_OF_WEEK: //day of week
		temp = (day)%7; // 1 - Monday, 7 - Sunday
		return temp ? temp : 7;
	case Date::WEEK:  //current week
		temp = ((day-1)/7)+1;
		if (!(temp%4))
			return 4;
		else
			return (temp%4);
	case Date::MONTH: //current month
		return ((day-1)/28)+1;
	case Date::DAY_OF_MONTH: //day of month
		temp = (day)%28;
		if (temp)
			return temp;
		else return 28;
	}
	return 0;
}

CGameState::CGameState()
{
	gs = this;
	mx = new boost::shared_mutex();
	applierGs = new CApplier<CBaseForGSApply>;
	registerTypesClientPacks1(*applierGs);
	registerTypesClientPacks2(*applierGs);
	//objCaller = new CObjectCallersHandler;
	globalEffects.setDescription("Global effects");
	globalEffects.setNodeType(CBonusSystemNode::GLOBAL_EFFECTS);
}

CGameState::~CGameState()
{
	//delete mx;//TODO: crash on Linux (mutex must be unlocked before destruction)
	map.dellNull();
	curB.dellNull();
	//delete scenarioOps; //TODO: fix for loading ind delete
	//delete initialOpts;
	delete applierGs;
	//delete objCaller;

	for(auto ptr : hpool.heroesPool) // clean hero pool
		ptr.second.dellNull();
}

BattleInfo * CGameState::setupBattle(int3 tile, const CArmedInstance *armies[2], const CGHeroInstance * heroes[2], bool creatureBank, const CGTownInstance *town)
{
	const TerrainTile &t = map->getTile(tile);
	ETerrainType terrain = t.terType;
	if(t.isCoastal() && !t.isWater())
		terrain = ETerrainType::SAND;

	BFieldType terType = battleGetBattlefieldType(tile);
	if (heroes[0] && heroes[0]->boat && heroes[1] && heroes[1]->boat)
		terType = BFieldType::SHIP_TO_SHIP;
	return BattleInfo::setupBattle(tile, terrain, terType, armies, heroes, creatureBank, town);
}

void CGameState::init(StartInfo * si)
{
    logGlobal->infoStream() << "\tUsing random seed: "<< si->seedToBeUsed;
	rand.setSeed(si->seedToBeUsed);
	scenarioOps = CMemorySerializer::deepCopy(*si).release();
	initialOpts = CMemorySerializer::deepCopy(*si).release();
	si = nullptr;

	switch(scenarioOps->mode)
	{
	case StartInfo::NEW_GAME:
		initNewGame();
		break;
	case StartInfo::CAMPAIGN:
		initCampaign();
		break;
	case StartInfo::DUEL:
		initDuel();
		return;
	default:
        logGlobal->errorStream() << "Wrong mode: " << (int)scenarioOps->mode;
		return;
	}
	VLC->arth->initAllowedArtifactsList(map->allowedArtifact);
    logGlobal->infoStream() << "Map loaded!";

	checkMapChecksum();

	day = 0;

    logGlobal->debugStream() << "Initialization:";

	initPlayerStates();
	placeCampaignHeroes();
	initGrailPosition();
	initRandomFactionsForPlayers();
	randomizeMapObjects();
	placeStartingHeroes();
	initStartingResources();
	initHeroes();
	initStartingBonus();
	initTowns();
	initMapObjects();
	buildBonusSystemTree();
	initVisitingAndGarrisonedHeroes();
	initFogOfWar();

    logGlobal->debugStream() << "\tChecking objectives";
	map->checkForObjectives(); //needs to be run when all objects are properly placed

	auto seedAfterInit = rand.nextInt();
    logGlobal->infoStream() << "Seed after init is " << seedAfterInit << " (before was " << scenarioOps->seedToBeUsed << ")";
	if(scenarioOps->seedPostInit > 0)
	{
		//RNG must be in the same state on all machines when initialization is done (otherwise we have desync)
		assert(scenarioOps->seedPostInit == seedAfterInit);
	}
	else
	{
		scenarioOps->seedPostInit = seedAfterInit; //store the post init "seed"
	}
}

void CGameState::initNewGame()
{
	if(scenarioOps->createRandomMap())
	{
		logGlobal->infoStream() << "Create random map.";
		CStopWatch sw;

		// Gen map
		CMapGenerator mapGenerator;
		map = mapGenerator.generate(scenarioOps->mapGenOptions.get(), scenarioOps->seedToBeUsed).release();

		// Update starting options
		for(int i = 0; i < map->players.size(); ++i)
		{
			const auto & playerInfo = map->players[i];
			if(playerInfo.canAnyonePlay())
			{
				PlayerSettings & playerSettings = scenarioOps->playerInfos[PlayerColor(i)];
				playerSettings.compOnly = !playerInfo.canHumanPlay;
				playerSettings.team = playerInfo.team;
				playerSettings.castle = playerInfo.defaultCastle();
				if(playerSettings.playerID == PlayerSettings::PLAYER_AI && playerSettings.name.empty())
				{
					playerSettings.name = VLC->generaltexth->allTexts[468];
				}
				playerSettings.color = PlayerColor(i);
			}
			else
			{
				scenarioOps->playerInfos.erase(PlayerColor(i));
			}
		}

		logGlobal->infoStream() << boost::format("Generated random map in %i ms.") % sw.getDiff();
	}
	else
	{
		logGlobal->infoStream() << "Open map file: " << scenarioOps->mapname;
		map = CMapService::loadMap(scenarioOps->mapname).release();
	}
}

void CGameState::initCampaign()
{
	logGlobal->infoStream() << "Open campaign map file: " << scenarioOps->campState->currentMap;
	auto campaign = scenarioOps->campState;
	assert(vstd::contains(campaign->camp->mapPieces, *scenarioOps->campState->currentMap));

	std::string scenarioName = scenarioOps->mapname.substr(0, scenarioOps->mapname.find('.'));
	boost::to_lower(scenarioName);
	scenarioName += ':' + boost::lexical_cast<std::string>(*campaign->currentMap);

	std::string & mapContent = campaign->camp->mapPieces[*campaign->currentMap];
	auto buffer = reinterpret_cast<const ui8 *>(mapContent.data());
	map = CMapService::loadMap(buffer, mapContent.size(), scenarioName).release();
}

void CGameState::initDuel()
{
	DuelParameters dp;
	try //CLoadFile likes throwing
	{
		if(boost::algorithm::ends_with(scenarioOps->mapname, ".json"))
		{
            logGlobal->infoStream() << "Loading duel settings from JSON file: " << scenarioOps->mapname;
			dp = DuelParameters::fromJSON(scenarioOps->mapname);
            logGlobal->infoStream() << "JSON file has been successfully read!";
		}
		else
		{
			CLoadFile lf(scenarioOps->mapname);
			lf.serializer >> dp;
		}
	}
	catch(...)
	{
        logGlobal->errorStream() << "Cannot load duel settings from " << scenarioOps->mapname;
		throw;
	}

	const CArmedInstance *armies[2] = {nullptr};
	const CGHeroInstance *heroes[2] = {nullptr};
	CGTownInstance *town = nullptr;

	for(int i = 0; i < 2; i++)
	{
		CArmedInstance *obj = nullptr;
		if(dp.sides[i].heroId >= 0)
		{
			const DuelParameters::SideSettings &ss = dp.sides[i];
			auto h = new CGHeroInstance();
			armies[i] = heroes[i] = h;
			obj = h;
			h->subID = ss.heroId;
			for(int i = 0; i < ss.heroPrimSkills.size(); i++)
				h->pushPrimSkill(static_cast<PrimarySkill::PrimarySkill>(i), ss.heroPrimSkills[i]);

			if(!ss.spells.empty())
			{
				h->putArtifact(ArtifactPosition::SPELLBOOK, CArtifactInstance::createNewArtifactInstance(0));
				boost::copy(ss.spells, std::inserter(h->spells, h->spells.begin()));
			}

			for(auto &parka : ss.artifacts)
			{
				h->putArtifact(ArtifactPosition(parka.first), parka.second);
			}

			typedef const std::pair<si32, si8> &TSecSKill;
			for(TSecSKill secSkill : ss.heroSecSkills)
				h->setSecSkillLevel(SecondarySkill(secSkill.first), secSkill.second, 1);

			h->initHero(HeroTypeID(h->subID));
			obj->initObj();
		}
		else
		{
			auto c = new CGCreature();
			armies[i] = obj = c;
			//c->subID = 34;
		}

		obj->setOwner(PlayerColor(i));

		for(int j = 0; j < ARRAY_COUNT(dp.sides[i].stacks); j++)
		{
			CreatureID cre = dp.sides[i].stacks[j].type;
			TQuantity count = dp.sides[i].stacks[j].count;
			if(count || obj->hasStackAtSlot(SlotID(j)))
				obj->setCreature(SlotID(j), cre, count);
		}

		for(const DuelParameters::CusomCreature &cc : dp.creatures)
		{
			CCreature *c = VLC->creh->creatures[cc.id];
			if(cc.attack >= 0)
				c->getBonusLocalFirst(Selector::typeSubtype(Bonus::PRIMARY_SKILL, PrimarySkill::ATTACK))->val = cc.attack;
			if(cc.defense >= 0)
				c->getBonusLocalFirst(Selector::typeSubtype(Bonus::PRIMARY_SKILL, PrimarySkill::DEFENSE))->val = cc.defense;
			if(cc.speed >= 0)
				c->getBonusLocalFirst(Selector::type(Bonus::STACKS_SPEED))->val = cc.speed;
			if(cc.HP >= 0)
				c->getBonusLocalFirst(Selector::type(Bonus::STACK_HEALTH))->val = cc.HP;
			if(cc.dmg >= 0)
			{
				c->getBonusLocalFirst(Selector::typeSubtype(Bonus::CREATURE_DAMAGE, 1))->val = cc.dmg;
				c->getBonusLocalFirst(Selector::typeSubtype(Bonus::CREATURE_DAMAGE, 2))->val = cc.dmg;
			}
			if(cc.shoots >= 0)
				c->getBonusLocalFirst(Selector::type(Bonus::SHOTS))->val = cc.shoots;
		}
	}

	curB = BattleInfo::setupBattle(int3(), dp.terType, dp.bfieldType, armies, heroes, false, town);
	curB->obstacles = dp.obstacles;
	curB->localInit();
}

void CGameState::checkMapChecksum()
{
	logGlobal->infoStream() << "\tOur checksum for the map: "<< map->checksum;
	if(scenarioOps->mapfileChecksum)
	{
		logGlobal->infoStream() << "\tServer checksum for " << scenarioOps->mapname <<": "<< scenarioOps->mapfileChecksum;
		if(map->checksum != scenarioOps->mapfileChecksum)
		{
			logGlobal->errorStream() << "Wrong map checksum!!!";
			throw std::runtime_error("Wrong checksum");
		}
	}
	else
	{
		scenarioOps->mapfileChecksum = map->checksum;
	}
}

void CGameState::initGrailPosition()
{
	logGlobal->debugStream() << "\tPicking grail position";
	//pick grail location
	if(map->grailPos.x < 0 || map->grailRadious) //grail not set or set within a radius around some place
	{
		if(!map->grailRadious) //radius not given -> anywhere on map
			map->grailRadious = map->width * 2;

		std::vector<int3> allowedPos;
		static const int BORDER_WIDTH = 9; // grail must be at least 9 tiles away from border

		// add all not blocked tiles in range
		for (int i = BORDER_WIDTH; i < map->width - BORDER_WIDTH ; i++)
		{
			for (int j = BORDER_WIDTH; j < map->height - BORDER_WIDTH; j++)
			{
				for (int k = 0; k < (map->twoLevel ? 2 : 1); k++)
				{
					const TerrainTile &t = map->getTile(int3(i, j, k));
					if(!t.blocked
						&& !t.visitable
						&& t.terType != ETerrainType::WATER
						&& t.terType != ETerrainType::ROCK
						&& map->grailPos.dist2dSQ(int3(i, j, k)) <= (map->grailRadious * map->grailRadious))
						allowedPos.push_back(int3(i,j,k));
				}
			}
		}

		//remove tiles with holes
		for(auto & elem : map->objects)
			if(elem && elem->ID == Obj::HOLE)
				allowedPos -= elem->pos;

		if(!allowedPos.empty())
		{
			map->grailPos = *RandomGeneratorUtil::nextItem(allowedPos, rand);
		}
		else
		{
			logGlobal->warnStream() << "Warning: Grail cannot be placed, no appropriate tile found!";
		}
	}
}

void CGameState::initRandomFactionsForPlayers()
{
	logGlobal->debugStream() << "\tPicking random factions for players";
	for(auto & elem : scenarioOps->playerInfos)
	{
		if(elem.second.castle==-1)
		{
			auto randomID = rand.nextInt(map->players[elem.first.getNum()].allowedFactions.size() - 1);
			auto iter = map->players[elem.first.getNum()].allowedFactions.begin();
			std::advance(iter, randomID);

			elem.second.castle = *iter;
		}
	}
}

void CGameState::randomizeMapObjects()
{
	logGlobal->debugStream() << "\tRandomizing objects";
	for(CGObjectInstance *obj : map->objects)
	{
		if(!obj) continue;

		randomizeObject(obj);

		//handle Favouring Winds - mark tiles under it
		if(obj->ID == Obj::FAVORABLE_WINDS)
		{
			for (int i = 0; i < obj->getWidth() ; i++)
			{
				for (int j = 0; j < obj->getHeight() ; j++)
				{
					int3 pos = obj->pos - int3(i,j,0);
					if(map->isInTheMap(pos)) map->getTile(pos).extTileFlags |= 128;
				}
			}
		}
	}
}

void CGameState::initPlayerStates()
{
	logGlobal->debugStream() << "\tCreating player entries in gs";
	for(auto & elem : scenarioOps->playerInfos)
	{
		std::pair<PlayerColor, PlayerState> ins(elem.first,PlayerState());
		ins.second.color=ins.first;
		ins.second.human = elem.second.playerID;
		ins.second.team = map->players[ins.first.getNum()].team;
		teams[ins.second.team].id = ins.second.team;//init team
		teams[ins.second.team].players.insert(ins.first);//add player to team
		players.insert(ins);
	}
}

void CGameState::placeCampaignHeroes()
{
	if (scenarioOps->campState)
	{
		// place bonus hero
		auto campaignBonus = scenarioOps->campState->getBonusForCurrentMap();
		bool campaignGiveHero = campaignBonus && campaignBonus.get().type == CScenarioTravel::STravelBonus::HERO;

		if(campaignGiveHero)
		{
			auto playerColor = PlayerColor(campaignBonus->info1);
			auto it = scenarioOps->playerInfos.find(playerColor);
			if(it != scenarioOps->playerInfos.end())
			{
				auto heroTypeId = campaignBonus->info2;
				if(heroTypeId == 0xffff) // random bonus hero
				{
					heroTypeId = pickUnusedHeroTypeRandomly(playerColor);
				}

				placeStartingHero(playerColor, HeroTypeID(heroTypeId), map->players[playerColor.getNum()].posOfMainTown);
			}
		}

		// replace heroes placeholders
		auto crossoverHeroes = getCrossoverHeroesFromPreviousScenarios();

		if(!crossoverHeroes.heroesFromAnyPreviousScenarios.empty())
		{
			logGlobal->debugStream() << "\tGenerate list of hero placeholders";
			auto campaignHeroReplacements = generateCampaignHeroesToReplace(crossoverHeroes);

			logGlobal->debugStream() << "\tPrepare crossover heroes";
			prepareCrossoverHeroes(campaignHeroReplacements, scenarioOps->campState->getCurrentScenario().travelOptions);

			// remove same heroes on the map which will be added through crossover heroes
			// INFO: we will remove heroes because later it may be possible that the API doesn't allow having heroes
			// with the same hero type id
			std::vector<CGHeroInstance *> removedHeroes;

			for(auto & campaignHeroReplacement : campaignHeroReplacements)
			{
				auto hero = getUsedHero(HeroTypeID(campaignHeroReplacement.hero->subID));
				if(hero)
				{
					removedHeroes.push_back(hero);
					map->heroesOnMap -= hero;
					map->objects[hero->id.getNum()] = nullptr;
					map->removeBlockVisTiles(hero, true);
				}
			}

			logGlobal->debugStream() << "\tReplace placeholders with heroes";
			replaceHeroesPlaceholders(campaignHeroReplacements);

			// remove hero placeholders on map
			for(auto obj : map->objects)
			{
				if(obj && obj->ID == Obj::HERO_PLACEHOLDER)
				{
					auto heroPlaceholder = dynamic_cast<CGHeroPlaceholder *>(obj.get());
					map->removeBlockVisTiles(heroPlaceholder, true);
					map->objects[heroPlaceholder->id.getNum()] = nullptr;
					delete heroPlaceholder;
				}
			}

			// now add removed heroes again with unused type ID
			for(auto hero : removedHeroes)
			{
				si32 heroTypeId = 0;
				if(hero->ID == Obj::HERO)
				{
					heroTypeId = pickUnusedHeroTypeRandomly(hero->tempOwner);
				}
				else if(hero->ID == Obj::PRISON)
				{
					auto unusedHeroTypeIds = getUnusedAllowedHeroes();
					if(!unusedHeroTypeIds.empty())
					{
						heroTypeId = (*RandomGeneratorUtil::nextItem(unusedHeroTypeIds, rand)).getNum();
					}
					else
					{
						logGlobal->errorStream() << "No free hero type ID found to replace prison.";
						assert(0);
					}
				}
				else
				{
					assert(0); // should not happen
				}

				hero->subID = heroTypeId;
				hero->portrait = hero->subID;
				map->getEditManager()->insertObject(hero, hero->pos);
			}
		}
	}
}

void CGameState::placeStartingHero(PlayerColor playerColor, HeroTypeID heroTypeId, int3 townPos)
{
	townPos.x += 1;

	CGHeroInstance * hero =  static_cast<CGHeroInstance*>(createObject(Obj::HERO, heroTypeId.getNum(), townPos, playerColor));
	map->getEditManager()->insertObject(hero, townPos);
}

CGameState::CrossoverHeroesList CGameState::getCrossoverHeroesFromPreviousScenarios() const
{
	CrossoverHeroesList crossoverHeroes;

	auto campaignState = scenarioOps->campState;
	auto bonus = campaignState->getBonusForCurrentMap();
	if (bonus && bonus->type == CScenarioTravel::STravelBonus::HEROES_FROM_PREVIOUS_SCENARIO)
	{
		crossoverHeroes.heroesFromAnyPreviousScenarios = crossoverHeroes.heroesFromPreviousScenario = campaignState->camp->scenarios[bonus->info2].crossoverHeroes;
	}
	else
	{
		if(!campaignState->mapsConquered.empty())
		{
			crossoverHeroes.heroesFromPreviousScenario = campaignState->camp->scenarios[campaignState->mapsConquered.back()].crossoverHeroes;

			for(auto mapNr : campaignState->mapsConquered)
			{
				// create a list of deleted heroes
				auto & scenario = campaignState->camp->scenarios[mapNr];
				auto lostCrossoverHeroes = scenario.getLostCrossoverHeroes();

				// remove heroes which didn't reached the end of the scenario, but were available at the start
				for(auto hero : lostCrossoverHeroes)
				{
					crossoverHeroes.heroesFromAnyPreviousScenarios.erase(range::remove_if(crossoverHeroes.heroesFromAnyPreviousScenarios,
						CGObjectInstanceBySubIdFinder(hero)), crossoverHeroes.heroesFromAnyPreviousScenarios.end());
				}

				// now add heroes which completed the scenario
				for(auto hero : scenario.crossoverHeroes)
				{
					// add new heroes and replace old heroes with newer ones
					auto it = range::find_if(crossoverHeroes.heroesFromAnyPreviousScenarios, CGObjectInstanceBySubIdFinder(hero));
					if (it != crossoverHeroes.heroesFromAnyPreviousScenarios.end())
					{
						// replace old hero with newer one
						crossoverHeroes.heroesFromAnyPreviousScenarios[it - crossoverHeroes.heroesFromAnyPreviousScenarios.begin()] = hero;
					}
					else
					{
						// add new hero
						crossoverHeroes.heroesFromAnyPreviousScenarios.push_back(hero);
					}
				}
			}
		}
	}

	return crossoverHeroes;
}

void CGameState::prepareCrossoverHeroes(std::vector<CGameState::CampaignHeroReplacement> & campaignHeroReplacements, const CScenarioTravel & travelOptions) const
{
	// create heroes list for convenience iterating
	std::vector<CGHeroInstance *> crossoverHeroes;
	for(auto & campaignHeroReplacement : campaignHeroReplacements)
	{
		crossoverHeroes.push_back(campaignHeroReplacement.hero);
	}

	// TODO replace magic numbers with named constants
	// TODO this logic (what should be kept) should be part of CScenarioTravel and be exposed via some clean set of methods
	if(!(travelOptions.whatHeroKeeps & 1))
	{
		//trimming experience
		for(CGHeroInstance * cgh : crossoverHeroes)
		{
			cgh->initExp();
		}
	}

	if(!(travelOptions.whatHeroKeeps & 2))
	{
		//trimming prim skills
		for(CGHeroInstance * cgh : crossoverHeroes)
		{
			for(int g=0; g<GameConstants::PRIMARY_SKILLS; ++g)
			{
				auto sel = Selector::type(Bonus::PRIMARY_SKILL)
					.And(Selector::subtype(g))
					.And(Selector::sourceType(Bonus::HERO_BASE_SKILL));

				cgh->getBonusLocalFirst(sel)->val = cgh->type->heroClass->primarySkillInitial[g];
			}
		}
	}

	if(!(travelOptions.whatHeroKeeps & 4))
	{
		//trimming sec skills
		for(CGHeroInstance * cgh : crossoverHeroes)
		{
			cgh->secSkills = cgh->type->secSkillsInit;
			cgh->recreateSecondarySkillsBonuses();
		}
	}

	if(!(travelOptions.whatHeroKeeps & 8))
	{
		for(CGHeroInstance * cgh : crossoverHeroes)
		{
			// Trimming spells
			cgh->spells.clear();

			// Spellbook will also be removed
			if (cgh->hasSpellbook())
				ArtifactLocation(cgh, ArtifactPosition(ArtifactPosition::SPELLBOOK)).removeArtifact();
		}
	}

	if(!(travelOptions.whatHeroKeeps & 16))
	{
		//trimming artifacts
		for(CGHeroInstance * hero : crossoverHeroes)
		{
			size_t totalArts = GameConstants::BACKPACK_START + hero->artifactsInBackpack.size();
			for (size_t i = 0; i < totalArts; i++ )
			{
				auto artifactPosition = ArtifactPosition(i);
				if(artifactPosition == ArtifactPosition::SPELLBOOK) continue; // do not handle spellbook this way

				const ArtSlotInfo *info = hero->getSlot(artifactPosition);
				if(!info)
					continue;

				// TODO: why would there be nullptr artifacts?
				const CArtifactInstance *art = info->artifact;
				if(!art)
					continue;

				int id  = art->artType->id;
				assert( 8*18 > id );//number of arts that fits into h3m format
				bool takeable = travelOptions.artifsKeptByHero[id / 8] & ( 1 << (id%8) );

				ArtifactLocation al(hero, artifactPosition);
				if(!takeable  &&  !al.getSlot()->locked)  //don't try removing locked artifacts -> it crashes #1719
					al.removeArtifact();
			}
		}
	}

	//trimming creatures
	for(CGHeroInstance * cgh : crossoverHeroes)
	{
		auto shouldSlotBeErased = [&](const std::pair<SlotID, CStackInstance *> & j) -> bool
		{
			CreatureID::ECreatureID crid = j.second->getCreatureID().toEnum();
			return !(travelOptions.monstersKeptByHero[crid / 8] & (1 << (crid % 8)));
		};

		auto stacksCopy = cgh->stacks; //copy of the map, so we can iterate iover it and remove stacks
		for(auto &slotPair : stacksCopy)
			if(shouldSlotBeErased(slotPair))
				cgh->eraseStack(slotPair.first);
	}

	// Removing short-term bonuses
	for(CGHeroInstance * cgh : crossoverHeroes)
	{
		cgh->popBonuses(CSelector(Bonus::OneDay)
			.Or(CSelector(Bonus::OneWeek))
			.Or(CSelector(Bonus::NTurns))
			.Or(CSelector(Bonus::NDays))
			.Or(CSelector(Bonus::OneBattle)));
	}

}

void CGameState::placeStartingHeroes()
{
	logGlobal->debugStream() << "\tGiving starting hero";

	for(auto & playerSettingPair : scenarioOps->playerInfos)
	{
		auto playerColor = playerSettingPair.first;
		auto & playerInfo = map->players[playerColor.getNum()];
		if(playerInfo.generateHeroAtMainTown && playerInfo.hasMainTown)
		{
			// Do not place a starting hero if the hero was already placed due to a campaign bonus
			if(scenarioOps->campState)
			{
				if(auto campaignBonus = scenarioOps->campState->getBonusForCurrentMap())
				{
					if(campaignBonus->type == CScenarioTravel::STravelBonus::HERO && playerColor == PlayerColor(campaignBonus->info1)) continue;
				}
			}

			int heroTypeId = pickNextHeroType(playerColor);
			if(playerSettingPair.second.hero == -1) playerSettingPair.second.hero = heroTypeId;

			placeStartingHero(playerColor, HeroTypeID(heroTypeId), playerInfo.posOfMainTown);
		}
	}
}

void CGameState::initStartingResources()
{
	logGlobal->debugStream() << "\tSetting up resources";
	const JsonNode config(ResourceID("config/startres.json"));
	const JsonVector &vector = config["difficulty"].Vector();
	const JsonNode &level = vector[scenarioOps->difficulty];

	TResources startresAI(level["ai"]);
	TResources startresHuman(level["human"]);

	for (auto & elem : players)
	{
		PlayerState &p = elem.second;

		if (p.human)
			p.resources = startresHuman;
		else
			p.resources = startresAI;
	}

	auto getHumanPlayerInfo = [&]() -> std::vector<const PlayerSettings *>
	{
		std::vector<const PlayerSettings *> ret;
		for(auto it = scenarioOps->playerInfos.cbegin();
			it != scenarioOps->playerInfos.cend(); ++it)
		{
			if(it->second.playerID != PlayerSettings::PLAYER_AI)
				ret.push_back(&it->second);
		}

		return ret;
	};

	//give start resource bonus in case of campaign
	if (scenarioOps->mode == StartInfo::CAMPAIGN)
	{
		auto chosenBonus = scenarioOps->campState->getBonusForCurrentMap();
		if(chosenBonus && chosenBonus->type == CScenarioTravel::STravelBonus::RESOURCE)
		{
			std::vector<const PlayerSettings *> people = getHumanPlayerInfo(); //players we will give resource bonus
			for(const PlayerSettings *ps : people)
			{
				std::vector<int> res; //resources we will give
				switch (chosenBonus->info1)
				{
				case 0: case 1: case 2: case 3: case 4: case 5: case 6:
					res.push_back(chosenBonus->info1);
					break;
				case 0xFD: //wood+ore
					res.push_back(Res::WOOD); res.push_back(Res::ORE);
					break;
				case 0xFE:  //rare
					res.push_back(Res::MERCURY); res.push_back(Res::SULFUR); res.push_back(Res::CRYSTAL); res.push_back(Res::GEMS);
					break;
				default:
					assert(0);
					break;
				}
				//increasing resource quantity
				for (auto & re : res)
				{
					players[ps->color].resources[re] += chosenBonus->info2;
				}
			}
		}
	}
}

void CGameState::initHeroes()
{
	for(auto hero : map->heroesOnMap)  //heroes instances initialization
	{
		if (hero->getOwner() == PlayerColor::UNFLAGGABLE)
		{
			logGlobal->warnStream() << "Warning - hero with uninitialized owner!";
			continue;
		}

		hero->initHero();
		getPlayer(hero->getOwner())->heroes.push_back(hero);
		map->allHeroes[hero->type->ID.getNum()] = hero;
	}

	for(auto obj : map->objects) //prisons
	{
		if(obj && obj->ID == Obj::PRISON)
			map->allHeroes[obj->subID] = dynamic_cast<CGHeroInstance*>(obj.get());
	}

	std::set<HeroTypeID> heroesToCreate = getUnusedAllowedHeroes(); //ids of heroes to be created and put into the pool
	for(auto ph : map->predefinedHeroes)
	{
		if(!vstd::contains(heroesToCreate, HeroTypeID(ph->subID)))
			continue;
		ph->initHero();
		hpool.heroesPool[ph->subID] = ph;
		hpool.pavailable[ph->subID] = 0xff;
		heroesToCreate.erase(ph->type->ID);

		map->allHeroes[ph->subID] = ph;
	}

	for(HeroTypeID htype : heroesToCreate) //all not used allowed heroes go with default state into the pool
	{
		auto  vhi = new CGHeroInstance();
		vhi->initHero(htype);

		int typeID = htype.getNum();
		map->allHeroes[typeID] = vhi;
		hpool.heroesPool[typeID] = vhi;
		hpool.pavailable[typeID] = 0xff;
	}

	for(auto & elem : map->disposedHeroes)
	{
		hpool.pavailable[elem.heroId] = elem.players;
	}

	if (scenarioOps->mode == StartInfo::CAMPAIGN) //give campaign bonuses for specific / best hero
	{
		auto chosenBonus = scenarioOps->campState->getBonusForCurrentMap();
		if (chosenBonus && chosenBonus->isBonusForHero() && chosenBonus->info1 != 0xFFFE) //exclude generated heroes
		{
			//find human player
			PlayerColor humanPlayer=PlayerColor::NEUTRAL;
			for (auto & elem : players)
			{
				if(elem.second.human)
				{
					humanPlayer = elem.first;
					break;
				}
			}
			assert(humanPlayer != PlayerColor::NEUTRAL);

			std::vector<ConstTransitivePtr<CGHeroInstance> > & heroes = players[humanPlayer].heroes;

			if (chosenBonus->info1 == 0xFFFD) //most powerful
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
					logGlobal->warnStream() << "Warning - cannot give bonus to hero cause there are no heroes!";
				else
					giveCampaignBonusToHero(heroes[maxB]);
			}
			else //specific hero
			{
				for (auto & heroe : heroes)
				{
					if (heroe->subID == chosenBonus->info1)
					{
						giveCampaignBonusToHero(heroe);
						break;
					}
				}
			}
		}
	}
}

void CGameState::giveCampaignBonusToHero(CGHeroInstance * hero)
{
	const boost::optional<CScenarioTravel::STravelBonus> & curBonus = scenarioOps->campState->getBonusForCurrentMap();
	if(!curBonus)
		return;

	if(curBonus->isBonusForHero())
	{
		//apply bonus
		switch (curBonus->type)
		{
		case CScenarioTravel::STravelBonus::SPELL:
			hero->spells.insert(SpellID(curBonus->info2));
			break;
		case CScenarioTravel::STravelBonus::MONSTER:
			{
				for(int i=0; i<GameConstants::ARMY_SIZE; i++)
				{
					if(hero->slotEmpty(SlotID(i)))
					{
						hero->addToSlot(SlotID(i), CreatureID(curBonus->info2), curBonus->info3);
						break;
					}
				}
			}
			break;
		case CScenarioTravel::STravelBonus::ARTIFACT:
			gs->giveHeroArtifact(hero, static_cast<ArtifactID>(curBonus->info2));
			break;
		case CScenarioTravel::STravelBonus::SPELL_SCROLL:
			{
				CArtifactInstance * scroll = CArtifactInstance::createScroll(SpellID(curBonus->info2).toSpell());
				scroll->putAt(ArtifactLocation(hero, scroll->firstAvailableSlot(hero)));
			}
			break;
		case CScenarioTravel::STravelBonus::PRIMARY_SKILL:
			{
				const ui8* ptr = reinterpret_cast<const ui8*>(&curBonus->info2);
				for (int g=0; g<GameConstants::PRIMARY_SKILLS; ++g)
				{
					int val = ptr[g];
					if (val == 0)
					{
						continue;
					}
					auto bb = new Bonus(Bonus::PERMANENT, Bonus::PRIMARY_SKILL, Bonus::CAMPAIGN_BONUS, val, *scenarioOps->campState->currentMap, g);
					hero->addNewBonus(bb);
				}
			}
			break;
		case CScenarioTravel::STravelBonus::SECONDARY_SKILL:
			hero->setSecSkillLevel(SecondarySkill(curBonus->info2), curBonus->info3, true);
			break;
		}
	}
}

void CGameState::initFogOfWar()
{
	logGlobal->debugStream() << "\tFog of war"; //FIXME: should be initialized after all bonuses are set
	for(auto & elem : teams)
	{
		elem.second.fogOfWarMap.resize(map->width);
		for(int g=0; g<map->width; ++g)
			elem.second.fogOfWarMap[g].resize(map->height);

		for(int g=-0; g<map->width; ++g)
			for(int h=0; h<map->height; ++h)
				elem.second.fogOfWarMap[g][h].resize(map->twoLevel ? 2 : 1, 0);

		for(int g=0; g<map->width; ++g)
			for(int h=0; h<map->height; ++h)
				for(int v = 0; v < (map->twoLevel ? 2 : 1); ++v)
					elem.second.fogOfWarMap[g][h][v] = 0;

		for(CGObjectInstance *obj : map->objects)
		{
			if(!obj || !vstd::contains(elem.second.players, obj->tempOwner)) continue; //not a flagged object

			std::unordered_set<int3, ShashInt3> tiles;
			getTilesInRange(tiles, obj->getSightCenter(), obj->getSightRadious(), obj->tempOwner, 1);
			for(int3 tile : tiles)
			{
				elem.second.fogOfWarMap[tile.x][tile.y][tile.z] = 1;
			}
		}
	}
}

void CGameState::initStartingBonus()
{
	logGlobal->debugStream() << "\tStarting bonuses";
	for(auto & elem : players)
	{
		//starting bonus
		if(scenarioOps->playerInfos[elem.first].bonus==PlayerSettings::RANDOM)
			scenarioOps->playerInfos[elem.first].bonus = static_cast<PlayerSettings::Ebonus>(rand.nextInt(2));
		switch(scenarioOps->playerInfos[elem.first].bonus)
		{
		case PlayerSettings::GOLD:
			elem.second.resources[Res::GOLD] += rand.nextInt(500, 1000);
			break;
		case PlayerSettings::RESOURCE:
			{
				int res = VLC->townh->factions[scenarioOps->playerInfos[elem.first].castle]->town->primaryRes;
				if(res == Res::WOOD_AND_ORE)
				{
					elem.second.resources[Res::WOOD] += rand.nextInt(5, 10);
					elem.second.resources[Res::ORE] += rand.nextInt(5, 10);
				}
				else
				{
					elem.second.resources[res] += rand.nextInt(3, 6);
				}
				break;
			}
		case PlayerSettings::ARTIFACT:
			{
				if(!elem.second.heroes.size())
				{
					logGlobal->debugStream() << "Cannot give starting artifact - no heroes!";
					break;
				}
				CArtifact *toGive;
				toGive = VLC->arth->artifacts[VLC->arth->pickRandomArtifact(rand, CArtifact::ART_TREASURE)];

				CGHeroInstance *hero = elem.second.heroes[0];
				giveHeroArtifact(hero, toGive->id);
			}
			break;
		}
	}
}

void CGameState::initTowns()
{
	logGlobal->debugStream() << "\tTowns";

	//campaign bonuses for towns
	if (scenarioOps->mode == StartInfo::CAMPAIGN)
	{
		auto chosenBonus = scenarioOps->campState->getBonusForCurrentMap();

		if (chosenBonus && chosenBonus->type == CScenarioTravel::STravelBonus::BUILDING)
		{
			for (int g=0; g<map->towns.size(); ++g)
			{
				PlayerState * owner = getPlayer(map->towns[g]->getOwner());
				if (owner)
				{
					PlayerInfo & pi = map->players[owner->color.getNum()];

					if (owner->human && //human-owned
						map->towns[g]->pos == pi.posOfMainTown + int3(2, 0, 0))
					{
						map->towns[g]->builtBuildings.insert(
							CBuildingHandler::campToERMU(chosenBonus->info1, map->towns[g]->subID, map->towns[g]->builtBuildings));
						break;
					}
				}
			}
		}
	}

	CGTownInstance::universitySkills.clear();
	for ( int i=0; i<4; i++)
		CGTownInstance::universitySkills.push_back(14+i);//skills for university

	for (auto & elem : map->towns)
	{
		CGTownInstance * vti =(elem);
		if(!vti->town)
		{
			vti->town = VLC->townh->factions[vti->subID]->town;
		}
		if(vti->name.empty())
		{
			vti->name = *RandomGeneratorUtil::nextItem(vti->town->names, rand);
		}

		//init buildings
		if(vstd::contains(vti->builtBuildings, BuildingID::DEFAULT)) //give standard set of buildings
		{
			vti->builtBuildings.erase(BuildingID::DEFAULT);
			vti->builtBuildings.insert(BuildingID::VILLAGE_HALL);
			if(vti->tempOwner != PlayerColor::NEUTRAL)
				vti->builtBuildings.insert(BuildingID::TAVERN);

			vti->builtBuildings.insert(BuildingID::DWELL_FIRST);
			if(rand.nextInt(1) == 1)
			{
				vti->builtBuildings.insert(BuildingID::DWELL_LVL_2);
			}
		}

		//#1444 - remove entries that don't have buildings defined (like some unused extra town hall buildings)
		vstd::erase_if(vti->builtBuildings, [vti](BuildingID bid){
			return !vti->town->buildings.count(bid) || !vti->town->buildings.at(bid); });

		if (vstd::contains(vti->builtBuildings, BuildingID::SHIPYARD) && vti->shipyardStatus()==IBoatGenerator::TILE_BLOCKED)
			vti->builtBuildings.erase(BuildingID::SHIPYARD);//if we have harbor without water - erase it (this is H3 behaviour)

		//init hordes
		for (int i = 0; i<GameConstants::CREATURES_PER_TOWN; i++)
			if (vstd::contains(vti->builtBuildings,(-31-i))) //if we have horde for this level
			{
				vti->builtBuildings.erase(BuildingID(-31-i));//remove old ID
				if (vti->town->hordeLvl.at(0) == i)//if town first horde is this one
				{
					vti->builtBuildings.insert(BuildingID::HORDE_1);//add it
					if (vstd::contains(vti->builtBuildings,(BuildingID::DWELL_UP_FIRST+i)))//if we have upgraded dwelling as well
						vti->builtBuildings.insert(BuildingID::HORDE_1_UPGR);//add it as well
				}
				if (vti->town->hordeLvl.at(1) == i)//if town second horde is this one
				{
					vti->builtBuildings.insert(BuildingID::HORDE_2);
					if (vstd::contains(vti->builtBuildings,(BuildingID::DWELL_UP_FIRST+i)))
						vti->builtBuildings.insert(BuildingID::HORDE_2_UPGR);
				}
			}

		//Early check for #1444-like problems
		for(auto building : vti->builtBuildings)
		{
			assert(vti->town->buildings.at(building) != nullptr);
			UNUSED(building);
		}

		//town events
		for(CCastleEvent &ev : vti->events)
		{
			for (int i = 0; i<GameConstants::CREATURES_PER_TOWN; i++)
				if (vstd::contains(ev.buildings,(-31-i))) //if we have horde for this level
				{
					ev.buildings.erase(BuildingID(-31-i));
					if (vti->town->hordeLvl.at(0) == i)
						ev.buildings.insert(BuildingID::HORDE_1);
					if (vti->town->hordeLvl.at(1) == i)
						ev.buildings.insert(BuildingID::HORDE_2);
				}
		}
		//init spells
		vti->spells.resize(GameConstants::SPELL_LEVELS);

		for(ui32 z=0; z<vti->obligatorySpells.size();z++)
		{
			CSpell *s = vti->obligatorySpells[z].toSpell();
			vti->spells[s->level-1].push_back(s->id);
			vti->possibleSpells -= s->id;
		}
		while(vti->possibleSpells.size())
		{
			ui32 total=0;
			int sel = -1;

			for(ui32 ps=0;ps<vti->possibleSpells.size();ps++)
				total += vti->possibleSpells[ps].toSpell()->getProbability(vti->subID);

			if (total == 0) // remaining spells have 0 probability
				break;

			auto r = rand.nextInt(total - 1);
			for(ui32 ps=0; ps<vti->possibleSpells.size();ps++)
			{
				r -= vti->possibleSpells[ps].toSpell()->getProbability(vti->subID);
				if(r<0)
				{
					sel = ps;
					break;
				}
			}
			if(sel<0)
				sel=0;

			CSpell *s = vti->possibleSpells[sel].toSpell();
			vti->spells[s->level-1].push_back(s->id);
			vti->possibleSpells -= s->id;
		}
		vti->possibleSpells.clear();
		if(vti->getOwner() != PlayerColor::NEUTRAL)
			getPlayer(vti->getOwner())->towns.push_back(vti);

	}
}

void CGameState::initMapObjects()
{
	logGlobal->debugStream() << "\tObject initialization";
//	objCaller->preInit();
	for(CGObjectInstance *obj : map->objects)
	{
		if(obj)
		{
			//logGlobal->traceStream() << boost::format ("Calling Init for object %d, %d") % obj->ID % obj->subID;
			obj->initObj();
		}
	}
	for(CGObjectInstance *obj : map->objects)
	{
		if(!obj)
			continue;

		switch (obj->ID)
		{
			case Obj::QUEST_GUARD:
			case Obj::SEER_HUT:
			{
				auto q = static_cast<CGSeerHut*>(obj);
				assert (q);
				q->setObjToKill();
			}
		}
	}
	CGSubterraneanGate::postInit(); //pairing subterranean gates

	map->calculateGuardingGreaturePositions(); //calculate once again when all the guards are placed and initialized
}

void CGameState::initVisitingAndGarrisonedHeroes()
{
	for(auto k=players.begin(); k!=players.end(); ++k)
	{
		if(k->first==PlayerColor::NEUTRAL)
			continue;

		//init visiting and garrisoned heroes
		for(CGHeroInstance *h : k->second.heroes)
		{
			for(CGTownInstance *t : k->second.towns)
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
	for (auto hero : map->heroesOnMap)
	{
		if (hero->visitedTown)
		{
			assert (hero->visitedTown->visitingHero == hero);
		}
	}
}

BFieldType CGameState::battleGetBattlefieldType(int3 tile)
{
	if(tile==int3() && curB)
		tile = curB->tile;
	else if(tile==int3() && !curB)
		return BFieldType::NONE;

	const TerrainTile &t = map->getTile(tile);
	//fight in mine -> subterranean
	if(dynamic_cast<const CGMine *>(t.visitableObjects.front()))
		return BFieldType::SUBTERRANEAN;

	for(auto &obj : map->objects)
	{
		//look only for objects covering given tile
		if( !obj || obj->pos.z != tile.z || !obj->coveringAt(tile.x, tile.y))
			continue;

		switch(obj->ID)
		{
		case Obj::CLOVER_FIELD:
			return BFieldType::CLOVER_FIELD;
		case Obj::CURSED_GROUND1: case Obj::CURSED_GROUND2:
			return BFieldType::CURSED_GROUND;
		case Obj::EVIL_FOG:
			return BFieldType::EVIL_FOG;
		case Obj::FAVORABLE_WINDS:
			return BFieldType::FAVOURABLE_WINDS;
		case Obj::FIERY_FIELDS:
			return BFieldType::FIERY_FIELDS;
		case Obj::HOLY_GROUNDS:
			return BFieldType::HOLY_GROUND;
		case Obj::LUCID_POOLS:
			return BFieldType::LUCID_POOLS;
		case Obj::MAGIC_CLOUDS:
			return BFieldType::MAGIC_CLOUDS;
		case Obj::MAGIC_PLAINS1: case Obj::MAGIC_PLAINS2:
			return BFieldType::MAGIC_PLAINS;
		case Obj::ROCKLANDS:
			return BFieldType::ROCKLANDS;
		}
	}

	if(!t.isWater() && t.isCoastal())
		return BFieldType::SAND_SHORE;

	switch(t.terType)
	{
	case ETerrainType::DIRT:
		return BFieldType(rand.nextInt(3, 5));
	case ETerrainType::SAND:
		return BFieldType::SAND_MESAS; //TODO: coast support
	case ETerrainType::GRASS:
		return BFieldType(rand.nextInt(6, 7));
	case ETerrainType::SNOW:
		return BFieldType(rand.nextInt(10, 11));
	case ETerrainType::SWAMP:
		return BFieldType::SWAMP_TREES;
	case ETerrainType::ROUGH:
		return BFieldType::ROUGH;
	case ETerrainType::SUBTERRANEAN:
		return BFieldType::SUBTERRANEAN;
	case ETerrainType::LAVA:
		return BFieldType::LAVA;
	case ETerrainType::WATER:
		return BFieldType::SHIP;
	case ETerrainType::ROCK:
		return BFieldType::ROCKLANDS;
	default:
		return BFieldType::NONE;
	}
}

UpgradeInfo CGameState::getUpgradeInfo(const CStackInstance &stack)
{
	UpgradeInfo ret;
	const CCreature *base = stack.type;

	const CGHeroInstance *h = stack.armyObj->ID == Obj::HERO ? static_cast<const CGHeroInstance*>(stack.armyObj) : nullptr;
	const CGTownInstance *t = nullptr;

	if(stack.armyObj->ID == Obj::TOWN)
		t = static_cast<const CGTownInstance *>(stack.armyObj);
	else if(h)
	{	//hero specialty
		TBonusListPtr lista = h->getBonuses(Selector::typeSubtype(Bonus::SPECIAL_UPGRADE, base->idNumber));
		for(const Bonus *it : *lista)
		{
			auto nid = CreatureID(it->additionalInfo);
			if (nid != base->idNumber) //in very specific case the upgrade is available by default (?)
			{
				ret.newID.push_back(nid);
				ret.cost.push_back(VLC->creh->creatures[nid]->cost - base->cost);
			}
		}
		t = h->visitedTown;
	}
	if(t)
	{
		for(const CGTownInstance::TCreaturesSet::value_type & dwelling : t->creatures)
		{
			if (vstd::contains(dwelling.second, base->idNumber)) //Dwelling with our creature
			{
				for(auto upgrID : dwelling.second)
				{
					if(vstd::contains(base->upgrades, upgrID)) //possible upgrade
					{
						ret.newID.push_back(upgrID);
						ret.cost.push_back(VLC->creh->creatures[upgrID]->cost - base->cost);
					}
				}
			}
		}
	}

	//hero is visiting Hill Fort
	if(h && map->getTile(h->visitablePos()).visitableObjects.front()->ID == Obj::HILL_FORT)
	{
		static const int costModifiers[] = {0, 25, 50, 75, 100}; //we get cheaper upgrades depending on level
		const int costModifier = costModifiers[std::min<int>(std::max((int)base->level - 1, 0), ARRAY_COUNT(costModifiers) - 1)];

		for(auto nid : base->upgrades)
		{
			ret.newID.push_back(nid);
			ret.cost.push_back((VLC->creh->creatures[nid]->cost - base->cost) * costModifier / 100);
		}
	}

	if(ret.newID.size())
		ret.oldID = base->idNumber;

	for (Res::ResourceSet &cost : ret.cost)
		cost.positive(); //upgrade cost can't be negative, ignore missing resources

	return ret;
}

PlayerRelations::PlayerRelations CGameState::getPlayerRelations( PlayerColor color1, PlayerColor color2 )
{
	if ( color1 == color2 )
		return PlayerRelations::SAME_PLAYER;
	if(color1 == PlayerColor::NEUTRAL || color2 == PlayerColor::NEUTRAL) //neutral player has no friends
		return PlayerRelations::ENEMIES;

	const TeamState * ts = getPlayerTeam(color1);
	if (ts && vstd::contains(ts->players, color2))
		return PlayerRelations::ALLIES;
	return PlayerRelations::ENEMIES;
}

void CGameState::getNeighbours(const TerrainTile &srct, int3 tile, std::vector<int3> &vec, const boost::logic::tribool &onLand, bool limitCoastSailing)
{
	static const int3 dirs[] = { int3(0,1,0),int3(0,-1,0),int3(-1,0,0),int3(+1,0,0),
					int3(1,1,0),int3(-1,1,0),int3(1,-1,0),int3(-1,-1,0) };

	//vec.reserve(8); //optimization
	for (auto & dir : dirs)
	{
		const int3 hlp = tile + dir;
		if(!map->isInTheMap(hlp))
			continue;

		const TerrainTile &hlpt = map->getTile(hlp);

// 		//we cannot visit things from blocked tiles
// 		if(srct.blocked && !srct.visitable && hlpt.visitable && srct.blockingObjects.front()->ID != HEROI_TYPE)
// 		{
// 			continue;
// 		}

        if(srct.terType == ETerrainType::WATER && limitCoastSailing && hlpt.terType == ETerrainType::WATER && dir.x && dir.y) //diagonal move through water
		{
			int3 hlp1 = tile,
				hlp2 = tile;
			hlp1.x += dir.x;
			hlp2.y += dir.y;

            if(map->getTile(hlp1).terType != ETerrainType::WATER || map->getTile(hlp2).terType != ETerrainType::WATER)
				continue;
		}

        if((indeterminate(onLand)  ||  onLand == (hlpt.terType!=ETerrainType::WATER) )
            && hlpt.terType != ETerrainType::ROCK)
		{
			vec.push_back(hlp);
		}
	}
}

int CGameState::getMovementCost(const CGHeroInstance *h, const int3 &src, const int3 &dest, bool flying, int remainingMovePoints, bool checkLast)
{
	if(src == dest) //same tile
		return 0;

	TerrainTile &s = map->getTile(src),
		&d = map->getTile(dest);

	//get basic cost
	int ret = h->getTileCost(d,s);

	if(d.blocked && flying)
	{
		bool freeFlying = h->getBonusesCount(Selector::typeSubtype(Bonus::FLYING_MOVEMENT, 1)) > 0;

		if(!freeFlying)
		{
			ret *= 1.4; //40% penalty for movement over blocked tile
		}
	}
    else if (d.terType == ETerrainType::WATER)
	{
		if(h->boat && s.hasFavourableWinds() && d.hasFavourableWinds()) //Favourable Winds
			ret *= 0.666;
		else if (!h->boat && h->getBonusesCount(Selector::typeSubtype(Bonus::WATER_WALKING, 1)) > 0)
			ret *= 1.4; //40% penalty for water walking
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
		vec.reserve(8); //optimization
        getNeighbours(d, dest, vec, s.terType != ETerrainType::WATER, true);
		for(auto & elem : vec)
		{
			int fcost = getMovementCost(h,dest, elem, flying, left, false);
			if(fcost <= left)
			{
				return ret;
			}
		}
		ret = remainingMovePoints;
	}
	return ret;
}

void CGameState::apply(CPack *pack)
{
	ui16 typ = typeList.getTypeID(pack);
	applierGs->apps[typ]->applyOnGS(this,pack);
}

void CGameState::calculatePaths(const CGHeroInstance *hero, CPathsInfo &out)
{
	CPathfinder pathfinder(out, this, hero);
	pathfinder.calculatePaths();
}

/**
 * Tells if the tile is guarded by a monster as well as the position
 * of the monster that will attack on it.
 *
 * @return int3(-1, -1, -1) if the tile is unguarded, or the position of
 * the monster guarding the tile.
 */
std::vector<CGObjectInstance*> CGameState::guardingCreatures (int3 pos) const
{
	std::vector<CGObjectInstance*> guards;
	const int3 originalPos = pos;
	if (!map->isInTheMap(pos))
		return guards;

	const TerrainTile &posTile = map->getTile(pos);
	if (posTile.visitable)
	{
		for (CGObjectInstance* obj : posTile.visitableObjects)
		{
			if(obj->blockVisit)
			{
				if (obj->ID == Obj::MONSTER) // Monster
					guards.push_back(obj);
			}
		}
	}
	pos -= int3(1, 1, 0); // Start with top left.
	for (int dx = 0; dx < 3; dx++)
	{
		for (int dy = 0; dy < 3; dy++)
		{
			if (map->isInTheMap(pos))
			{
				const auto & tile = map->getTile(pos);
                if (tile.visitable && (tile.isWater() == posTile.isWater()))
				{
					for (CGObjectInstance* obj : tile.visitableObjects)
					{
						if (obj->ID == Obj::MONSTER  &&  map->checkForVisitableDir(pos, &map->getTile(originalPos), originalPos)) // Monster being able to attack investigated tile
						{
							guards.push_back(obj);
						}
					}
				}
			}

			pos.y++;
		}
		pos.y -= 3;
		pos.x++;
	}
	return guards;

}

int3 CGameState::guardingCreaturePosition (int3 pos) const
{
	return gs->map->guardingCreaturePositions[pos.x][pos.y][pos.z];
}

bool CGameState::isVisible(int3 pos, PlayerColor player)
{
	if(player == PlayerColor::NEUTRAL)
		return false;
	return getPlayerTeam(player)->fogOfWarMap[pos.x][pos.y][pos.z];
}

bool CGameState::isVisible( const CGObjectInstance *obj, boost::optional<PlayerColor> player )
{
	if(!player)
		return true;

	//we should always see our own heroes - but sometimes not visible heroes cause crash :?
	if (player == obj->tempOwner)
		return true;

	if(*player == PlayerColor::NEUTRAL) //-> TODO ??? needed?
		return false;
	//object is visible when at least one blocked tile is visible
	for(int fy=0; fy < obj->getHeight(); ++fy)
	{
		for(int fx=0; fx < obj->getWidth(); ++fx)
		{
			int3 pos = obj->pos + int3(-fx, -fy, 0);

			if ( map->isInTheMap(pos) &&
				 obj->coveringAt(pos.x, pos.y) &&
				 isVisible(pos, *player))
				return true;
		}
	}
	return false;
}

bool CGameState::checkForVisitableDir(const int3 & src, const int3 & dst) const
{
	const TerrainTile * pom = &map->getTile(dst);
	return map->checkForVisitableDir(src, pom, dst);
}

EVictoryLossCheckResult CGameState::checkForVictoryAndLoss(PlayerColor player) const
{
	const std::string & messageWonSelf = VLC->generaltexth->allTexts[659];
	const std::string & messageWonOther = VLC->generaltexth->allTexts[5];
	const std::string & messageLostSelf = VLC->generaltexth->allTexts[7];
	const std::string & messageLostOther = VLC->generaltexth->allTexts[8];

	auto evaluateEvent = [=](const EventCondition & condition)
	{
		return this->checkForVictory(player, condition);
	};

	const PlayerState *p = CGameInfoCallback::getPlayer(player);

	//cheater or tester, but has entered the code...
	if (p->enteredWinningCheatCode)
		return EVictoryLossCheckResult::victory(messageWonSelf, messageWonOther);

	if (p->enteredLosingCheatCode)
		return EVictoryLossCheckResult::defeat(messageLostSelf, messageLostOther);

	for (const TriggeredEvent & event : map->triggeredEvents)
	{
		if ((event.trigger.test(evaluateEvent)))
		{
			if (event.effect.type == EventEffect::VICTORY)
				return EVictoryLossCheckResult::victory(event.onFulfill, event.effect.toOtherMessage);

			if (event.effect.type == EventEffect::DEFEAT)
				return EVictoryLossCheckResult::defeat(event.onFulfill, event.effect.toOtherMessage);
		}
	}

	if (checkForStandardLoss(player))
	{
		return EVictoryLossCheckResult::defeat(messageLostSelf, messageLostOther);
	}
	return EVictoryLossCheckResult();
}

bool CGameState::checkForVictory( PlayerColor player, const EventCondition & condition ) const
{
	const PlayerState *p = CGameInfoCallback::getPlayer(player);
	switch (condition.condition)
	{
		case EventCondition::STANDARD_WIN:
		{
			return player == checkForStandardWin();
		}
		case EventCondition::HAVE_ARTIFACT: //check if any hero has winning artifact
		{
			for(auto & elem : p->heroes)
				if(elem->hasArt(condition.objectType))
					return true;
			return false;
		}
		case EventCondition::HAVE_CREATURES:
		{
			//check if in players armies there is enough creatures
			int total = 0; //creature counter
			for(size_t i = 0; i < map->objects.size(); i++)
			{
				const CArmedInstance *ai = nullptr;
				if(map->objects[i]
					&& map->objects[i]->tempOwner == player //object controlled by player
					&&  (ai = dynamic_cast<const CArmedInstance*>(map->objects[i].get()))) //contains army
				{
					for(auto & elem : ai->Slots()) //iterate through army
						if(elem.second->type->idNumber == condition.objectType) //it's searched creature
							total += elem.second->count;
				}
			}
			return total >= condition.value;
		}
		case EventCondition::HAVE_RESOURCES:
		{
			return p->resources[condition.objectType] >= condition.value;
		}
		case EventCondition::HAVE_BUILDING:
		{
			if (condition.object) // specific town
			{
				const CGTownInstance *t = static_cast<const CGTownInstance *>(condition.object);
				return (t->tempOwner == player && t->hasBuilt(BuildingID(condition.objectType)));
			}
			else // any town
			{
				for (const CGTownInstance * t : p->towns)
				{
					if (t->hasBuilt(BuildingID(condition.objectType)))
						return true;
				}
				return false;
			}
		}
		case EventCondition::DESTROY:
		{
			if (condition.object) // mode A - destroy specific object of this type
			{
				if (auto hero = dynamic_cast<const CGHeroInstance*>(condition.object))
					return boost::range::find(gs->map->heroesOnMap, hero) == gs->map->heroesOnMap.end();
				else
					return getObj(condition.object->id) == nullptr;
			}
			else
			{
				for(auto & elem : map->objects) // mode B - destroy all objects of this type
				{
					if(elem && elem->ID == condition.objectType)
						return false;
				}
				return true;
			}
		}
		case EventCondition::CONTROL:
		{
			// list of players that need to control object to fulfull condition
			// NOTE: cgameinfocallback specified explicitly in order to get const version
			auto & team = CGameInfoCallback::getPlayerTeam(player)->players;

			if (condition.object) // mode A - flag one specific object, like town
			{
				return team.count(condition.object->tempOwner) != 0;
			}
			else
			{
				for(auto & elem : map->objects) // mode B - flag all objects of this type
				{
					 //check not flagged objs
					if ( elem && elem->ID == condition.objectType && team.count(elem->tempOwner) == 0 )
						return false;
				}
				return true;
			}
		}
		case EventCondition::TRANSPORT:
		{
			const CGTownInstance *t = static_cast<const CGTownInstance *>(condition.object);
			if((t->visitingHero && t->visitingHero->hasArt(condition.objectType))
				|| (t->garrisonHero && t->garrisonHero->hasArt(condition.objectType)))
			{
				return true;
			}
			return false;
		}
		case EventCondition::DAYS_PASSED:
		{
			return gs->day > condition.value;
		}
		case EventCondition::IS_HUMAN:
		{
			return p->human ? condition.value == 1 : condition.value == 0;
		}
		case EventCondition::DAYS_WITHOUT_TOWN:
		{
			if (p->daysWithoutCastle)
				return p->daysWithoutCastle.get() >= condition.value;
			else
				return false;
		}
		case EventCondition::CONST_VALUE:
		{
			return condition.value; // just convert to bool
		}
	}
	assert(0);
	return false;
}

PlayerColor CGameState::checkForStandardWin() const
{
	//std victory condition is:
	//all enemies lost
	PlayerColor supposedWinner = PlayerColor::NEUTRAL;
	TeamID winnerTeam = TeamID::NO_TEAM;
	for(auto & elem : players)
	{
		if(elem.second.status == EPlayerStatus::INGAME && elem.first < PlayerColor::PLAYER_LIMIT)
		{
			if(supposedWinner == PlayerColor::NEUTRAL)
			{
				//first player remaining ingame - candidate for victory
				supposedWinner = elem.second.color;
				winnerTeam = elem.second.team;
			}
			else if(winnerTeam != elem.second.team)
			{
				//current candidate has enemy remaining in game -> no vicotry
				return PlayerColor::NEUTRAL;
			}
		}
	}

	return supposedWinner;
}

bool CGameState::checkForStandardLoss( PlayerColor player ) const
{
	//std loss condition is: player lost all towns and heroes
	const PlayerState &p = *CGameInfoCallback::getPlayer(player);
	return !p.heroes.size() && !p.towns.size();
}

struct statsHLP
{
	typedef std::pair< PlayerColor, si64 > TStat;
	//converts [<player's color, value>] to vec[place] -> platers
	static std::vector< std::vector< PlayerColor > > getRank( std::vector<TStat> stats )
	{
		std::sort(stats.begin(), stats.end(), statsHLP());

		//put first element
		std::vector< std::vector<PlayerColor> > ret;
		std::vector<PlayerColor> tmp;
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
				std::vector<PlayerColor> tmp;
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

	static const CGHeroInstance * findBestHero(CGameState * gs, PlayerColor color)
	{
		std::vector<ConstTransitivePtr<CGHeroInstance> > &h = gs->players[color].heroes;
		if(!h.size())
			return nullptr;
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
		for(auto h : ps->heroes)
		{
			ret += h->artifactsInBackpack.size() + h->artifactsWorn.size();
		}
		return ret;
	}

	// get total strength of player army
	static si64 getArmyStrength(const PlayerState * ps)
	{
		si64 str = 0;

		for(auto h : ps->heroes)
		{
			if(!h->inTownGarrison)		//original h3 behavior
				str += h->getArmyStrength();
		}
		return str;
	}
};

void CGameState::obtainPlayersStats(SThievesGuildInfo & tgi, int level)
{
	auto playerInactive = [&](PlayerColor color)
	{
		return color == PlayerColor::NEUTRAL || players.at(color).status != EPlayerStatus::INGAME;
	};

#define FILL_FIELD(FIELD, VAL_GETTER) \
	{ \
		std::vector< std::pair< PlayerColor, si64 > > stats; \
		for(auto g = players.begin(); g != players.end(); ++g) \
		{ \
			if(playerInactive(g->second.color)) \
				continue; \
			std::pair< PlayerColor, si64 > stat; \
			stat.first = g->second.color; \
			stat.second = VAL_GETTER; \
			stats.push_back(stat); \
		} \
		tgi.FIELD = statsHLP::getRank(stats); \
	}

	for(auto & elem : players)
	{
		if(!playerInactive(elem.second.color))
			tgi.playerColors.push_back(elem.second.color);
	}

	if(level >= 1) //num of towns & num of heroes
	{
		//num of towns
		FILL_FIELD(numOfTowns, g->second.towns.size())
		//num of heroes
		FILL_FIELD(numOfHeroes, g->second.heroes.size())
		//best hero's portrait
		for(auto g = players.cbegin(); g != players.cend(); ++g)
		{
			if(playerInactive(g->second.color))
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
		FILL_FIELD(gold, g->second.resources[Res::GOLD])
	}
	if(level >= 2) //wood & ore
	{
		FILL_FIELD(woodOre, g->second.resources[Res::WOOD] + g->second.resources[Res::ORE])
	}
	if(level >= 3) //mercury, sulfur, crystal, gems
	{
		FILL_FIELD(mercSulfCrystGems, g->second.resources[Res::MERCURY] + g->second.resources[Res::SULFUR] + g->second.resources[Res::CRYSTAL] + g->second.resources[Res::GEMS])
	}
	if(level >= 4) //obelisks found
	{
		FILL_FIELD(obelisks, CGObelisk::visited[gs->getPlayerTeam(g->second.color)->id])
	}
	if(level >= 5) //artifacts
	{
		FILL_FIELD(artifacts, statsHLP::getNumberOfArts(&g->second))
	}
	if(level >= 6) //army strength
	{
		FILL_FIELD(army, statsHLP::getArmyStrength(&g->second))
	}
	if(level >= 7) //income
	{
		//TODO:obtainPlayersStats - income
	}
	if(level >= 8) //best hero's stats
	{
		//already set in  lvl 1 handling
	}
	if(level >= 9) //personality
	{
		for(auto g = players.cbegin(); g != players.cend(); ++g)
		{
			if(playerInactive(g->second.color)) //do nothing for neutral player
				continue;
			if(g->second.human)
			{
                tgi.personality[g->second.color] = EAiTactic::NONE;
			}
			else //AI
			{
                tgi.personality[g->second.color] = map->players[g->second.color.getNum()].aiTactic;
			}

		}
	}
	if(level >= 10) //best creature
	{
		//best creatures belonging to player (highest AI value)
		for(auto g = players.cbegin(); g != players.cend(); ++g)
		{
			if(playerInactive(g->second.color)) //do nothing for neutral player
				continue;
			int bestCre = -1; //best creature's ID
			for(auto & elem : g->second.heroes)
			{
				for(auto it = elem->Slots().begin(); it != elem->Slots().end(); ++it)
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

std::map<ui32, ConstTransitivePtr<CGHeroInstance> > CGameState::unusedHeroesFromPool()
{
	std::map<ui32, ConstTransitivePtr<CGHeroInstance> > pool = hpool.heroesPool;
	for ( auto i = players.cbegin() ; i != players.cend();i++)
		for(auto j = i->second.availableHeroes.cbegin(); j != i->second.availableHeroes.cend(); j++)
			if(*j)
				pool.erase((**j).subID);

	return pool;
}

void CGameState::buildBonusSystemTree()
{
	buildGlobalTeamPlayerTree();
	attachArmedObjects();

	for(CGTownInstance *t : map->towns)
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
	for(auto k=teams.begin(); k!=teams.end(); ++k)
	{
		TeamState *t = &k->second;
		t->attachTo(&globalEffects);

		for(PlayerColor teamMember : k->second.players)
		{
			PlayerState *p = getPlayer(teamMember);
			assert(p);
			p->attachTo(t);
		}
	}
}

void CGameState::attachArmedObjects()
{
	for(CGObjectInstance *obj : map->objects)
	{
		if(CArmedInstance *armed = dynamic_cast<CArmedInstance*>(obj))
			armed->whatShouldBeAttached()->attachTo(armed->whereShouldBeAttached(this));
	}
}

void CGameState::giveHeroArtifact(CGHeroInstance *h, ArtifactID aid)
{
	 CArtifact * const artifact = VLC->arth->artifacts[aid]; //pointer to constant object
	 CArtifactInstance *ai = CArtifactInstance::createNewArtifactInstance(artifact);
	 map->addNewArtifactInstance(ai);
	 ai->putAt(ArtifactLocation(h, ai->firstAvailableSlot(h)));
}

std::set<HeroTypeID> CGameState::getUnusedAllowedHeroes(bool alsoIncludeNotAllowed /*= false*/) const
{
	std::set<HeroTypeID> ret;
	for(int i = 0; i < map->allowedHeroes.size(); i++)
		if(map->allowedHeroes[i] || alsoIncludeNotAllowed)
			ret.insert(HeroTypeID(i));

	for(auto hero : map->heroesOnMap)  //heroes instances initialization
	{
		if(hero->type)
			ret -= hero->type->ID;
		else
			ret -= HeroTypeID(hero->subID);
	}

	for(auto obj : map->objects) //prisons
		if(obj && obj->ID == Obj::PRISON)
			ret -= HeroTypeID(obj->subID);

	return ret;
}

std::vector<CGameState::CampaignHeroReplacement> CGameState::generateCampaignHeroesToReplace(CrossoverHeroesList & crossoverHeroes)
{
	std::vector<CampaignHeroReplacement> campaignHeroReplacements;

	//selecting heroes by type
	for(auto obj : map->objects)
	{
		if(obj && obj->ID == Obj::HERO_PLACEHOLDER)
		{
			auto heroPlaceholder = dynamic_cast<CGHeroPlaceholder *>(obj.get());
			if(heroPlaceholder->subID != 0xFF) //select by type
			{
				auto it = range::find_if(crossoverHeroes.heroesFromAnyPreviousScenarios, [heroPlaceholder](CGHeroInstance * hero)
				{
					return hero->subID == heroPlaceholder->subID;
				});

				if(it != crossoverHeroes.heroesFromAnyPreviousScenarios.end())
				{
					auto hero = *it;
					crossoverHeroes.removeHeroFromBothLists(hero);
                    campaignHeroReplacements.push_back(CampaignHeroReplacement(CMemorySerializer::deepCopy(*hero).release(), heroPlaceholder->id));
				}
			}
		}
	}

	//selecting heroes by power
	range::sort(crossoverHeroes.heroesFromPreviousScenario, [](const CGHeroInstance * a, const CGHeroInstance * b)
	{
		return a->getHeroStrength() > b->getHeroStrength();
	}); //sort, descending strength

	// sort hero placeholders descending power
	std::vector<CGHeroPlaceholder *> heroPlaceholders;
	for(auto obj : map->objects)
	{
		if(obj && obj->ID == Obj::HERO_PLACEHOLDER)
		{
			auto heroPlaceholder = dynamic_cast<CGHeroPlaceholder *>(obj.get());
			if(heroPlaceholder->subID == 0xFF) //select by power
			{
				heroPlaceholders.push_back(heroPlaceholder);
			}
		}
	}
	range::sort(heroPlaceholders, [](const CGHeroPlaceholder * a, const CGHeroPlaceholder * b)
	{
		return a->power > b->power;
	});

	for(int i = 0; i < heroPlaceholders.size(); ++i)
	{
		auto heroPlaceholder = heroPlaceholders[i];
		if(crossoverHeroes.heroesFromPreviousScenario.size() > i)
		{
            auto hero = crossoverHeroes.heroesFromPreviousScenario[i];
            campaignHeroReplacements.push_back(CampaignHeroReplacement(CMemorySerializer::deepCopy(*hero).release(), heroPlaceholder->id));
		}
	}

	return campaignHeroReplacements;
}

void CGameState::replaceHeroesPlaceholders(const std::vector<CGameState::CampaignHeroReplacement> & campaignHeroReplacements)
{
	for(auto campaignHeroReplacement : campaignHeroReplacements)
	{
		auto heroPlaceholder = dynamic_cast<CGHeroPlaceholder*>(getObjInstance(campaignHeroReplacement.heroPlaceholderId));

		CGHeroInstance *heroToPlace = campaignHeroReplacement.hero;
		heroToPlace->id = campaignHeroReplacement.heroPlaceholderId;
		heroToPlace->tempOwner = heroPlaceholder->tempOwner;
		heroToPlace->pos = heroPlaceholder->pos;
		heroToPlace->type = VLC->heroh->heroes[heroToPlace->subID];

		for(auto &&i : heroToPlace->stacks)
			i.second->type = VLC->creh->creatures[i.second->getCreatureID()];

		auto fixArtifact = [&](CArtifactInstance * art)
		{
			art->artType = VLC->arth->artifacts[art->artType->id];
			gs->map->artInstances.push_back(art);
			art->id = ArtifactInstanceID(gs->map->artInstances.size() - 1);
		};

		for(auto &&i : heroToPlace->artifactsWorn)
			fixArtifact(i.second.artifact);
		for(auto &&i : heroToPlace->artifactsInBackpack)
			fixArtifact(i.artifact);

		map->heroesOnMap.push_back(heroToPlace);
		map->objects[heroToPlace->id.getNum()] = heroToPlace;
		map->addBlockVisTiles(heroToPlace);

		scenarioOps->campState->getCurrentScenario().placedCrossoverHeroes.push_back(heroToPlace);
	}
}

bool CGameState::isUsedHero(HeroTypeID hid) const
{
	return getUsedHero(hid);
}

CGHeroInstance * CGameState::getUsedHero(HeroTypeID hid) const
{
	for(auto hero : map->heroesOnMap)  //heroes instances initialization
	{
		if(hero->subID == hid.getNum())
		{
			return hero;
		}
	}

	for(auto obj : map->objects) //prisons
	{
		if(obj && obj->ID == Obj::PRISON && obj->subID == hid.getNum())
		{
			return dynamic_cast<CGHeroInstance *>(obj.get());
		}
	}

	return nullptr;
}

CGPathNode::CGPathNode()
:coord(-1,-1,-1)
{
	accessible = NOT_SET;
	land = 0;
	moveRemains = 0;
	turns = 255;
	theNodeBefore = nullptr;
}

bool CGPathNode::reachable() const
{
	return turns < 255;
}

const CGPathNode * CPathsInfo::getPathInfo( const int3& tile ) const
{
	boost::unique_lock<boost::mutex> pathLock(pathMx);
	if (tile.x >= sizes.x || tile.y >= sizes.y || tile.z >= sizes.z ||
	    tile.x < 0 || tile.y < 0 || tile.z < 0)
		return nullptr;
	return &nodes[tile.x][tile.y][tile.z];
}

int CPathsInfo::getDistance( int3 tile ) const
{
	boost::unique_lock<boost::mutex> pathLock(pathMx);

	CGPath ret;
	if (getPath(tile, ret))
		return ret.nodes.size();
	else
		return 255;
}

bool CPathsInfo::getPath( const int3 &dst, CGPath &out ) const
{
	boost::unique_lock<boost::mutex> pathLock(pathMx);

	out.nodes.clear();
	const CGPathNode *curnode = &nodes[dst.x][dst.y][dst.z];
	if(!curnode->theNodeBefore)
		return false;


	while(curnode)
	{
		CGPathNode cpn = *curnode;
		curnode = curnode->theNodeBefore;
		out.nodes.push_back(cpn);
	}
	return true;
}

CPathsInfo::CPathsInfo( const int3 &Sizes )
:sizes(Sizes)
{
	hero = nullptr;
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
		for(auto & elem : nodes)
		{
			elem.coord = CGHeroInstance::convertPosition(elem.coord,true);
		}
	}
}

PlayerState::PlayerState()
 : color(-1), enteredWinningCheatCode(0),
   enteredLosingCheatCode(0), status(EPlayerStatus::INGAME)
{
	setNodeType(PLAYER);
}

std::string PlayerState::nodeName() const
{
	return "Player " + (color.getNum() < VLC->generaltexth->capColors.size() ? VLC->generaltexth->capColors[color.getNum()] : boost::lexical_cast<std::string>(color));
}

InfoAboutArmy::InfoAboutArmy():
    owner(PlayerColor::NEUTRAL)
{}

InfoAboutArmy::InfoAboutArmy(const CArmedInstance *Army, bool detailed)
{
	initFromArmy(Army, detailed);
}

void InfoAboutArmy::initFromArmy(const CArmedInstance *Army, bool detailed)
{
	army = ArmyDescriptor(Army, detailed);
	owner = Army->tempOwner;
	name = Army->getObjectName();
}

void InfoAboutHero::assign(const InfoAboutHero & iah)
{
	InfoAboutArmy::operator = (iah);

	details = (iah.details ? new Details(*iah.details) : nullptr);
	hclass = iah.hclass;
	portrait = iah.portrait;
}

InfoAboutHero::InfoAboutHero():
    details(nullptr),
    hclass(nullptr),
    portrait(-1)
{}

InfoAboutHero::InfoAboutHero(const InfoAboutHero & iah):
    InfoAboutArmy()
{
	assign(iah);
}

InfoAboutHero::InfoAboutHero(const CGHeroInstance *h, bool detailed)
	: details(nullptr),
	hclass(nullptr),
	portrait(-1)
{
	initFromHero(h, detailed);
}

InfoAboutHero::~InfoAboutHero()
{
	delete details;
}

InfoAboutHero & InfoAboutHero::operator=(const InfoAboutHero & iah)
{
	assign(iah);
	return *this;
}

void InfoAboutHero::initFromHero(const CGHeroInstance *h, bool detailed)
{
	if(!h)
		return;

	initFromArmy(h, detailed);

	hclass = h->type->heroClass;
	name = h->name;
	portrait = h->portrait;

	if(detailed)
	{
		//include details about hero
		details = new Details;
		details->luck = h->LuckVal();
		details->morale = h->MoraleVal();
		details->mana = h->mana;
		details->primskills.resize(GameConstants::PRIMARY_SKILLS);

		for (int i = 0; i < GameConstants::PRIMARY_SKILLS ; i++)
		{
			details->primskills[i] = h->getPrimSkillLevel(static_cast<PrimarySkill::PrimarySkill>(i));
		}
	}
}

InfoAboutTown::InfoAboutTown():
    details(nullptr),
    tType(nullptr),
    built(0),
    fortLevel(0)
{

}

InfoAboutTown::InfoAboutTown(const CGTownInstance *t, bool detailed)
{
	initFromTown(t, detailed);
}

InfoAboutTown::~InfoAboutTown()
{
	delete details;
}

void InfoAboutTown::initFromTown(const CGTownInstance *t, bool detailed)
{
	initFromArmy(t, detailed);
	army = ArmyDescriptor(t->getUpperArmy(), detailed);
	built = t->builded;
	fortLevel = t->fortLevel();
	name = t->name;
	tType = t->town;

	if(detailed)
	{
		//include details about hero
		details = new Details;
		TResources income = t->dailyIncome();
		details->goldIncome = income[Res::GOLD];
		details->customRes = t->hasBuilt(BuildingID::RESOURCE_SILO);
		details->hallLevel = t->hallLevel();
		details->garrisonedHero = t->garrisonHero;
	}
}

ArmyDescriptor::ArmyDescriptor(const CArmedInstance *army, bool detailed)
    : isDetailed(detailed)
{
	for(auto & elem : army->Slots())
	{
		if(detailed)
			(*this)[elem.first] = *elem.second;
		else
			(*this)[elem.first] = CStackBasicDescriptor(elem.second->type, elem.second->getQuantityID());
	}
}

ArmyDescriptor::ArmyDescriptor()
    : isDetailed(false)
{

}

int ArmyDescriptor::getStrength() const
{
	ui64 ret = 0;
	if(isDetailed)
	{
		for(auto & elem : *this)
			ret += elem.second.type->AIValue * elem.second.count;
	}
	else
	{
		for(auto & elem : *this)
			ret += elem.second.type->AIValue * CCreature::estimateCreatureCount(elem.second.count);
	}
	return ret;
}

DuelParameters::SideSettings::StackSettings::StackSettings()
	: type(CreatureID::NONE), count(0)
{
}

DuelParameters::SideSettings::StackSettings::StackSettings(CreatureID Type, si32 Count)
	: type(Type), count(Count)
{
}

DuelParameters::SideSettings::SideSettings()
{
	heroId = -1;
}

DuelParameters::DuelParameters():
    terType(ETerrainType::DIRT),
    bfieldType(BFieldType::ROCKLANDS)
{
}

DuelParameters DuelParameters::fromJSON(const std::string &fname)
{
	DuelParameters ret;

	const JsonNode duelData(ResourceID("DATA/" + fname, EResType::TEXT));
	ret.terType = ETerrainType((int)duelData["terType"].Float());
	ret.bfieldType = BFieldType((int)duelData["bfieldType"].Float());
	for(const JsonNode &n : duelData["sides"].Vector())
	{
		SideSettings &ss = ret.sides[(int)n["side"].Float()];
		int i = 0;
		for(const JsonNode &stackNode : n["army"].Vector())
		{
			ss.stacks[i].type = CreatureID((si32)stackNode.Vector()[0].Float());
			ss.stacks[i].count = stackNode.Vector()[1].Float();
			i++;
		}

		if(n["heroid"].getType() == JsonNode::DATA_FLOAT)
			ss.heroId = n["heroid"].Float();
		else
			ss.heroId = -1;

		for(const JsonNode &entry : n["heroPrimSkills"].Vector())
			ss.heroPrimSkills.push_back(entry.Float());

		for(const JsonNode &skillNode : n["heroSecSkills"].Vector())
		{
			std::pair<si32, si8> secSkill;
			secSkill.first = skillNode.Vector()[0].Float();
			secSkill.second = skillNode.Vector()[1].Float();
			ss.heroSecSkills.push_back(secSkill);
		}

		assert(ss.heroPrimSkills.empty() || ss.heroPrimSkills.size() == GameConstants::PRIMARY_SKILLS);

		if(ss.heroId != -1)
		{
			const JsonNode & spells = n["spells"];
			if(spells.getType() == JsonNode::DATA_STRING  &&  spells.String() == "all")
			{
				for(auto spell : VLC->spellh->objects)
					if(spell->id <= SpellID::SUMMON_AIR_ELEMENTAL)
						ss.spells.insert(spell->id);
			}
			else
				for(const JsonNode &spell : n["spells"].Vector())
					ss.spells.insert(SpellID(spell.Float()));
		}
	}

	for(const JsonNode &n : duelData["obstacles"].Vector())
	{
		auto oi = make_shared<CObstacleInstance>();
		if(n.getType() == JsonNode::DATA_VECTOR)
		{
			oi->ID = n.Vector()[0].Float();
			oi->pos = n.Vector()[1].Float();
		}
		else
		{
			assert(n.getType() == JsonNode::DATA_FLOAT);
			oi->ID = 21;
			oi->pos = n.Float();
		}
		oi->uniqueID = ret.obstacles.size();
		ret.obstacles.push_back(oi);
	}

	for(const JsonNode &n : duelData["creatures"].Vector())
	{
		CusomCreature cc;
		cc.id = n["id"].Float();

#define retrieve(name)								\
	if(n[ #name ].getType() == JsonNode::DATA_FLOAT)\
	cc.name = n[ #name ].Float();			\
	else											\
	cc.name = -1;

		retrieve(attack);
		retrieve(defense);
		retrieve(HP);
		retrieve(dmg);
		retrieve(shoots);
		retrieve(speed);
		ret.creatures.push_back(cc);
	}

	return ret;
}

TeamState::TeamState()
{
	setNodeType(TEAM);
}

void CPathfinder::initializeGraph()
{
	CGPathNode ***graph = out.nodes;
	for(size_t i=0; i < out.sizes.x; ++i)
	{
		for(size_t j=0; j < out.sizes.y; ++j)
		{
			for(size_t k=0; k < out.sizes.z; ++k)
			{
				curPos = int3(i,j,k);
				const TerrainTile *tinfo = &gs->map->getTile(int3(i, j, k));
				CGPathNode &node = graph[i][j][k];

				node.accessible = evaluateAccessibility(tinfo);
				node.turns = 0xff;
				node.moveRemains = 0;
				node.coord.x = i;
				node.coord.y = j;
				node.coord.z = k;
                node.land = tinfo->terType != ETerrainType::WATER;
				node.theNodeBefore = nullptr;
			}
		}
	}
}

void CPathfinder::calculatePaths()
{
	bool flying = hero->hasBonusOfType(Bonus::FLYING_MOVEMENT);
	int maxMovePointsLand = hero->maxMovePoints(true);
	int maxMovePointsWater = hero->maxMovePoints(false);
	int3 src = hero->getPosition(false);

	auto maxMovePoints = [&](CGPathNode *cp) -> int
	{
		return cp->land ? maxMovePointsLand : maxMovePointsWater;
	};

	out.hero = hero;
	out.hpos = hero->getPosition(false);

	if(!gs->map->isInTheMap(out.hpos)/* || !gs->map->isInTheMap(dest)*/) //check input
	{
		logGlobal->errorStream() << "CGameState::calculatePaths: Hero outside the gs->map? How dare you...";
		return;
	}

	//logGlobal->infoStream() << boost::format("Calculating paths for hero %s (adress  %d) of player %d") % hero->name % hero % hero->tempOwner;
	initializeGraph();

	//initial tile - set cost on 0 and add to the queue
	CGPathNode &initialNode = *getNode(out.hpos);
	initialNode.turns = 0;
	initialNode.moveRemains = hero->movement;
	mq.push_back(&initialNode);

	std::vector<int3> neighbours;
	neighbours.reserve(16);
	while(!mq.empty())
	{
		cp = mq.front();
		mq.pop_front();

		const int3 sourceGuardPosition = gs->map->guardingCreaturePositions[cp->coord.x][cp->coord.y][cp->coord.z];
		bool guardedSource = (sourceGuardPosition != int3(-1, -1, -1) && cp->coord != src);
		ct = &gs->map->getTile(cp->coord);

		int movement = cp->moveRemains, turn = cp->turns;
		if(!movement)
		{
			movement = maxMovePoints(cp);
			turn++;
		}

		//add accessible neighbouring nodes to the queue
		neighbours.clear();

		auto isAllowedTeleportEntrance = [&](const CGTeleport * obj) -> bool
		{
			if(!gs->isTeleportEntrancePassable(obj, hero->tempOwner))
				return false;

			auto whirlpool = dynamic_cast<const CGWhirlpool *>(obj);
			if(whirlpool)
			{
				if(addTeleportWhirlpool(whirlpool))
					return true;
			}
			else if(addTeleportTwoWay(obj) || addTeleportOneWay(obj) || addTeleportOneWayRandom(obj))
				return true;

			return false;
		};

		auto sObj = ct->topVisitableObj(cp->coord == CGHeroInstance::convertPosition(hero->pos, false));
		auto cObj = dynamic_cast<const CGTeleport *>(sObj);
		if(isAllowedTeleportEntrance(cObj))
		{
			for(auto objId : gs->getTeleportChannelExits(cObj->channel, hero->tempOwner))
			{
				auto obj = getObj(objId);
				if(CGTeleport::isExitPassable(gs, hero, obj))
					neighbours.push_back(obj->visitablePos());
			}
		}

		std::vector<int3> neighbour_tiles;
		gs->getNeighbours(*ct, cp->coord, neighbour_tiles, boost::logic::indeterminate, !cp->land);
		if(sObj)
		{
			for(int3 neighbour_tile: neighbour_tiles)
			{
				if(canMoveBetween(neighbour_tile, sObj->visitablePos()))
					neighbours.push_back(neighbour_tile);
			}
		}
		else
			vstd::concatenate(neighbours, neighbour_tiles);

		for(auto & neighbour : neighbours)
		{
			const int3 &n = neighbour; //current neighbor
			dp = getNode(n);
			dt = &gs->map->getTile(n);
			destTopVisObjID = dt->topVisitableId();

			useEmbarkCost = 0; //0 - usual movement; 1 - embark; 2 - disembark
			const bool destIsGuardian = sourceGuardPosition == n;

			auto dObj = dynamic_cast<const CGTeleport*>(dt->topVisitableObj());
			if(!goodForLandSeaTransition()
			   || (!canMoveBetween(cp->coord, dp->coord) && !CGTeleport::isConnected(cObj, dObj))
			   || dp->accessible == CGPathNode::BLOCKED)
			{
				continue;
			}

			//special case -> hero embarked a boat standing on a guarded tile -> we must allow to move away from that tile
			if(cp->accessible == CGPathNode::VISITABLE && guardedSource && cp->theNodeBefore->land && ct->topVisitableId() == Obj::BOAT)
				guardedSource = false;

			int cost = gs->getMovementCost(hero, cp->coord, dp->coord, flying, movement);
			//special case -> moving from src Subterranean gate to dest gate -> it's free
			if(CGTeleport::isConnected(cObj, dObj))
				cost = 0;

			int remains = movement - cost;
			if(useEmbarkCost)
			{
				remains = hero->movementPointsAfterEmbark(movement, cost, useEmbarkCost - 1);
				cost = movement - remains;
			}

			int turnAtNextTile = turn;
			if(remains < 0)
			{
				//occurs rarely, when hero with low movepoints tries to leave the road
				turnAtNextTile++;
				int moveAtNextTile = maxMovePoints(cp);
				cost = gs->getMovementCost(hero, cp->coord, dp->coord, flying, moveAtNextTile); //cost must be updated, movement points changed :(
				remains = moveAtNextTile - cost;
			}

			if((dp->turns==0xff		//we haven't been here before
				|| dp->turns > turnAtNextTile
				|| (dp->turns >= turnAtNextTile  &&  dp->moveRemains < remains)) //this route is faster
				&& (!guardedSource || destIsGuardian)) // Can step into tile of guard
			{
				assert(dp != cp->theNodeBefore); //two tiles can't point to each other
				dp->moveRemains = remains;
				dp->turns = turnAtNextTile;
				dp->theNodeBefore = cp;

				const bool guardedDst = gs->map->guardingCreaturePositions[dp->coord.x][dp->coord.y][dp->coord.z].valid()
										&& dp->accessible == CGPathNode::BLOCKVIS;

				auto checkDestinationTile = [&]() -> bool
				{
					if(dp->accessible == CGPathNode::ACCESSIBLE)
						return true;
					if(dp->coord == CGHeroInstance::convertPosition(hero->pos, false))
						return true; // This one is tricky, we can ignore fact that tile is not ACCESSIBLE in case if it's our hero block it. Though this need investigation
					if(dp->accessible == CGPathNode::VISITABLE && CGTeleport::isTeleport(dt->topVisitableObj()))
						return true; // For now we'll walways allos transit for teleports
					if(useEmbarkCost && allowEmbarkAndDisembark)
						return true;
					if(gs->isTeleportEntrancePassable(dObj, hero->tempOwner))
						return true; // Always add entry teleport with non-dummy channel
					if(CGTeleport::isConnected(cObj, dObj))
						return true; // Always add exit points of teleport
					if(guardedDst && !guardedSource)
						return true; // Can step into a hostile tile once

					return false;
				};

				if(checkDestinationTile())
					mq.push_back(dp);
			}
		} //neighbours loop
	} //queue loop
}

CGPathNode *CPathfinder::getNode(const int3 &coord)
{
	return &out.nodes[coord.x][coord.y][coord.z];
}

bool CPathfinder::canMoveBetween(const int3 &a, const int3 &b) const
{
	return gs->checkForVisitableDir(a, b);
}

CGPathNode::EAccessibility CPathfinder::evaluateAccessibility(const TerrainTile *tinfo) const
{
	CGPathNode::EAccessibility ret = (tinfo->blocked ? CGPathNode::BLOCKED : CGPathNode::ACCESSIBLE);


    if(tinfo->terType == ETerrainType::ROCK || !FoW[curPos.x][curPos.y][curPos.z])
		return CGPathNode::BLOCKED;

	if(tinfo->visitable)
	{
		if(tinfo->visitableObjects.front()->ID == Obj::SANCTUARY && tinfo->visitableObjects.back()->ID == Obj::HERO && tinfo->visitableObjects.back()->tempOwner != hero->tempOwner) //non-owned hero stands on Sanctuary
		{
			return CGPathNode::BLOCKED;
		}
		else
		{
			for(const CGObjectInstance *obj : tinfo->visitableObjects)
			{
				if (obj->passableFor(hero->tempOwner))
				{
					ret = CGPathNode::ACCESSIBLE;
				}
				else if(obj->blockVisit)
				{
					return CGPathNode::BLOCKVIS;
				}
				else if(obj->ID != Obj::EVENT) //pathfinder should ignore placed events
				{
					ret =  CGPathNode::VISITABLE;
				}
			}
		}
	}
	else if (gs->map->guardingCreaturePositions[curPos.x][curPos.y][curPos.z].valid()
		&& !tinfo->blocked)
	{
		// Monster close by; blocked visit for battle.
		return CGPathNode::BLOCKVIS;
	}

	return ret;
}

bool CPathfinder::goodForLandSeaTransition()
{
	if(cp->land != dp->land) //hero can traverse land<->sea only in special circumstances
	{
		if(cp->land) //from land to sea -> embark or assault hero on boat
		{
			if(dp->accessible == CGPathNode::ACCESSIBLE || destTopVisObjID < 0) //cannot enter empty water tile from land -> it has to be visitable
				return false;
			if(destTopVisObjID != Obj::HERO && destTopVisObjID != Obj::BOAT) //only boat or hero can be accessed from land
				return false;
			if(destTopVisObjID == Obj::BOAT)
				useEmbarkCost = 1;
		}
		else //disembark
		{
			//can disembark only on coastal tiles
			if(!dt->isCoastal())
				return false;

			//tile must be accessible -> exception: unblocked blockvis tiles -> clear but guarded by nearby monster coast
			if( (dp->accessible != CGPathNode::ACCESSIBLE && (dp->accessible != CGPathNode::BLOCKVIS || dt->blocked))
				|| dt->visitable)  //TODO: passableness problem -> town says it's passable (thus accessible) but we obviously can't disembark onto town gate
				return false;;

			useEmbarkCost = 2;
		}
	}

	return true;
}

CPathfinder::CPathfinder(CPathsInfo &_out, CGameState *_gs, const CGHeroInstance *_hero) : CGameInfoCallback(_gs, boost::optional<PlayerColor>()), out(_out), hero(_hero), FoW(getPlayerTeam(hero->tempOwner)->fogOfWarMap)
{
	assert(hero);
	assert(hero == getHero(hero->id));

	allowEmbarkAndDisembark = true;
	allowTeleportTwoWay = true;
	allowTeleportOneWay = true;
	allowTeleportOneWayRandom = false;
	allowTeleportWhirlpool = false;
	if (CGWhirlpool::isProtected(hero))
		allowTeleportWhirlpool = true;
}

CRandomGenerator & CGameState::getRandomGenerator()
{
	//logGlobal->traceStream() << "Fetching CGameState::rand with seed " << rand.nextInt();
	return rand;
}

bool CPathfinder::addTeleportTwoWay(const CGTeleport * obj) const
{
	return allowTeleportTwoWay && gs->isTeleportChannelBidirectional(obj->channel, hero->tempOwner);
}

bool CPathfinder::addTeleportOneWay(const CGTeleport * obj) const
{
	if(allowTeleportOneWay && isTeleportChannelUnidirectional(obj->channel, hero->tempOwner))
	{
		auto passableExits = CGTeleport::getPassableExits(gs, hero, gs->getTeleportChannelExits(obj->channel, hero->tempOwner));
		if(passableExits.size() == 1)
			return true;
	}
	return false;
}

bool CPathfinder::addTeleportOneWayRandom(const CGTeleport * obj) const
{
	if(allowTeleportOneWayRandom && isTeleportChannelUnidirectional(obj->channel, hero->tempOwner))
	{
		auto passableExits = CGTeleport::getPassableExits(gs, hero, gs->getTeleportChannelExits(obj->channel, hero->tempOwner));
		if(passableExits.size() > 1)
			return true;
	}
	return false;
}

bool CPathfinder::addTeleportWhirlpool(const CGWhirlpool * obj) const
{
   return allowTeleportWhirlpool && obj;
}
