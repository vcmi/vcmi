/*
 * CGameState.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
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
#include "CModHandler.h"
#include "GameSettings.h"
#include "TerrainHandler.h"
#include "CSkillHandler.h"
#include "mapping/CMap.h"
#include "mapping/CMapService.h"
#include "StartInfo.h"
#include "NetPacks.h"
#include "registerTypes/RegisterTypes.h"
#include "battle/BattleInfo.h"
#include "JsonNode.h"
#include "filesystem/Filesystem.h"
#include "GameConstants.h"
#include "rmg/CMapGenerator.h"
#include "CStopWatch.h"
#include "mapping/CMapEditManager.h"
#include "serializer/CTypeList.h"
#include "serializer/CMemorySerializer.h"
#include "VCMIDirs.h"

VCMI_LIB_NAMESPACE_BEGIN

boost::shared_mutex CGameState::mutex;

template <typename T> class CApplyOnGS;

class CBaseForGSApply
{
public:
	virtual void applyOnGS(CGameState *gs, void *pack) const =0;
	virtual ~CBaseForGSApply() = default;
	template<typename U> static CBaseForGSApply *getApplier(const U * t=nullptr)
	{
		return new CApplyOnGS<U>();
	}
};

template <typename T> class CApplyOnGS : public CBaseForGSApply
{
public:
	void applyOnGS(CGameState *gs, void *pack) const override
	{
		T *ptr = static_cast<T*>(pack);

		boost::unique_lock<boost::shared_mutex> lock(CGameState::mutex);
		ptr->applyGs(gs);
	}
};

void MetaString::getLocalString(const std::pair<ui8, ui32> & txt, std::string & dst) const
{
	int type = txt.first;
	int ser = txt.second;

	if(type == ART_NAMES)
	{
		const auto * art = ArtifactID(ser).toArtifact(VLC->artifacts());
		if(art)
			dst = art->getNameTranslated();
		else
			dst = "#!#";
	}
	else if(type == ART_DESCR)
	{
		const auto * art = ArtifactID(ser).toArtifact(VLC->artifacts());
		if(art)
			dst = art->getDescriptionTranslated();
		else
			dst = "#!#";
	}
	else if (type == ART_EVNTS)
	{
		const auto * art = ArtifactID(ser).toArtifact(VLC->artifacts());
		if(art)
			dst = art->getEventTranslated();
		else
			dst = "#!#";
	}
	else if(type == CRE_PL_NAMES)
	{
		const auto * cre = CreatureID(ser).toCreature(VLC->creatures());
		if(cre)
			dst = cre->getNamePluralTranslated();
		else
			dst = "#!#";
	}
	else if(type == CRE_SING_NAMES)
	{
		const auto * cre = CreatureID(ser).toCreature(VLC->creatures());
		if(cre)
			dst = cre->getNameSingularTranslated();
		else
			dst = "#!#";
	}
	else if(type == MINE_NAMES)
	{
		dst = VLC->generaltexth->translate("core.minename", ser);
	}
	else if(type == MINE_EVNTS)
	{
		dst = VLC->generaltexth->translate("core.mineevnt", ser);
	}
	else if(type == SPELL_NAME)
	{
		const auto * spell = SpellID(ser).toSpell(VLC->spells());
		if(spell)
			dst = spell->getNameTranslated();
		else
			dst = "#!#";
	}
	else if(type == OBJ_NAMES)
	{
		dst = VLC->objtypeh->getObjectName(ser, 0);
	}
	else if(type == SEC_SKILL_NAME)
	{
		dst = VLC->skillh->getByIndex(ser)->getNameTranslated();
	}
	else
	{
		switch(type)
		{
		case GENERAL_TXT:
			dst = VLC->generaltexth->translate("core.genrltxt", ser);
			break;
		case RES_NAMES:
			dst = VLC->generaltexth->translate("core.restypes", ser);
			break;
		case ARRAY_TXT:
			dst = VLC->generaltexth->translate("core.arraytxt", ser);
			break;
		case CREGENS:
			dst = VLC->objtypeh->getObjectName(Obj::CREATURE_GENERATOR1, ser);
			break;
		case CREGENS4:
			dst = VLC->objtypeh->getObjectName(Obj::CREATURE_GENERATOR4, ser);
			break;
		case ADVOB_TXT:
			dst = VLC->generaltexth->translate("core.advevent", ser);
			break;
		case COLOR:
			dst = VLC->generaltexth->translate("vcmi.capitalColors", ser);
			break;
		case JK_TXT:
			dst = VLC->generaltexth->translate("core.jktext", ser);
			break;
		default:
			logGlobal->error("Failed string substitution because type is %d", type);
			dst = "#@#";
			return;
		}
	}
}

DLL_LINKAGE void MetaString::toString(std::string &dst) const
{
	size_t exSt = 0;
	size_t loSt = 0;
	size_t nums = 0;
	dst.clear();

	for(const auto & elem : message)
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
			dst += std::to_string(numbers[nums++]);
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
			boost::replace_first(dst, "%d", std::to_string(numbers[nums++]));
			break;
		case TREPLACE_PLUSNUMBER:
			boost::replace_first(dst, "%+d", '+' + std::to_string(numbers[nums++]));
			break;
		default:
			logGlobal->error("MetaString processing error! Received message of type %d", int(elem));
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

	size_t exSt = 0;
	size_t loSt = 0;
	size_t nums = 0;
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
				lista += std::to_string(numbers[nums++]);
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
				lista.replace (lista.find("%d"), 2, std::to_string(numbers[nums++]));
				break;
			default:
				logGlobal->error("MetaString processing error! Received message of type %d",int(message[i]));
		}

	}
	return lista;
}

void MetaString::addCreReplacement(const CreatureID & id, TQuantity count) //adds sing or plural name;
{
	if (!count)
		addReplacement (CRE_PL_NAMES, id); //no creatures - just empty name (eg. defeat Angels)
	else if (count == 1)
		addReplacement (CRE_SING_NAMES, id);
	else
		addReplacement (CRE_PL_NAMES, id);
}

void MetaString::addReplacement(const CStackBasicDescriptor & stack)
{
	assert(stack.type); //valid type
	addCreReplacement(stack.type->getId(), stack.count);
}

static CGObjectInstance * createObject(const Obj & id, int subid, const int3 & pos, const PlayerColor & owner)
{
	CGObjectInstance * nobj;
	switch(id)
	{
	case Obj::HERO:
		{
			auto handler = VLC->objtypeh->getHandlerFor(id, VLC->heroh->objects[subid]->heroClass->getIndex());
			nobj = handler->create(handler->getTemplates().front());
			break;
		}
	case Obj::TOWN:
		nobj = new CGTownInstance();
		break;
	default: //rest of objects
		nobj = new CGObjectInstance();
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

CGHeroInstance * CGameState::HeroesPool::pickHeroFor(bool native,
													 const PlayerColor & player,
													 const CTown * town,
													 std::map<ui32, ConstTransitivePtr<CGHeroInstance>> & available,
													 CRandomGenerator & rand,
													 const CHeroClass * bannedClass) const
{
	CGHeroInstance *ret = nullptr;

	if(player>=PlayerColor::PLAYER_LIMIT)
	{
		logGlobal->error("Cannot pick hero for faction %s. Wrong owner!", town->faction->getJsonKey());
		return nullptr;
	}

	std::vector<CGHeroInstance *> pool;

	if(native)
	{
		for(auto & elem : available)
		{
			if(pavailable.find(elem.first)->second & 1<<player.getNum()
				&& elem.second->type->heroClass->faction == town->faction->getIndex())
			{
				pool.push_back(elem.second); //get all available heroes
			}
		}
		if(pool.empty())
		{
			logGlobal->error("Cannot pick native hero for %s. Picking any...", player.getStr());
			return pickHeroFor(false, player, town, available, rand);
		}
		else
		{
			ret = *RandomGeneratorUtil::nextItem(pool, rand);
		}
	}
	else
	{
		int sum = 0;
		int r;

		for(auto & elem : available)
		{
			if (pavailable.find(elem.first)->second & (1<<player.getNum()) &&    // hero is available
			    ( !bannedClass || elem.second->type->heroClass != bannedClass) ) // and his class is not same as other hero
			{
				pool.push_back(elem.second);
				sum += elem.second->type->heroClass->selectionProbability[town->faction->getId()]; //total weight
			}
		}
		if(pool.empty() || sum == 0)
		{
			logGlobal->error("There are no heroes available for player %s!", player.getStr());
			return nullptr;
		}

		r = rand.nextInt(sum - 1);
		for (auto & elem : pool)
		{
			r -= elem->type->heroClass->selectionProbability[town->faction->getId()];
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

CGameState::CampaignHeroReplacement::CampaignHeroReplacement(CGHeroInstance * hero, const ObjectInstanceID & heroPlaceholderId):
	hero(hero),
	heroPlaceholderId(heroPlaceholderId)
{

}

int CGameState::pickNextHeroType(const PlayerColor & owner)
{
	const PlayerSettings &ps = scenarioOps->getIthPlayersSettings(owner);
	if(ps.hero >= 0 && !isUsedHero(HeroTypeID(ps.hero))) //we haven't used selected hero
	{
		return ps.hero;
	}

	return pickUnusedHeroTypeRandomly(owner);
}

int CGameState::pickUnusedHeroTypeRandomly(const PlayerColor & owner)
{
	//list of available heroes for this faction and others
	std::vector<HeroTypeID> factionHeroes;
	std::vector<HeroTypeID> otherHeroes;

	const PlayerSettings &ps = scenarioOps->getIthPlayersSettings(owner);
	for(const HeroTypeID & hid : getUnusedAllowedHeroes())
	{
		if(VLC->heroh->objects[hid.getNum()]->heroClass->faction == ps.castle)
			factionHeroes.push_back(hid);
		else
			otherHeroes.push_back(hid);
	}

	// select random hero native to "our" faction
	if(!factionHeroes.empty())
	{
		return RandomGeneratorUtil::nextItem(factionHeroes, getRandomGenerator())->getNum();
	}

	logGlobal->warn("Cannot find free hero of appropriate faction for player %s - trying to get first available...", owner.getStr());
	if(!otherHeroes.empty())
	{
		return RandomGeneratorUtil::nextItem(otherHeroes, getRandomGenerator())->getNum();
	}

	logGlobal->error("No free allowed heroes!");
	auto notAllowedHeroesButStillBetterThanCrash = getUnusedAllowedHeroes(true);
	if(!notAllowedHeroesButStillBetterThanCrash.empty())
		return notAllowedHeroesButStillBetterThanCrash.begin()->getNum();

	logGlobal->error("No free heroes at all!");
	assert(0); //current code can't handle this situation
	return -1; // no available heroes at all
}

std::pair<Obj,int> CGameState::pickObject (CGObjectInstance *obj)
{
	switch(obj->ID)
	{
	case Obj::RANDOM_ART:
		return std::make_pair(Obj::ARTIFACT, VLC->arth->pickRandomArtifact(getRandomGenerator(), CArtifact::ART_TREASURE | CArtifact::ART_MINOR | CArtifact::ART_MAJOR | CArtifact::ART_RELIC));
	case Obj::RANDOM_TREASURE_ART:
		return std::make_pair(Obj::ARTIFACT, VLC->arth->pickRandomArtifact(getRandomGenerator(), CArtifact::ART_TREASURE));
	case Obj::RANDOM_MINOR_ART:
		return std::make_pair(Obj::ARTIFACT, VLC->arth->pickRandomArtifact(getRandomGenerator(), CArtifact::ART_MINOR));
	case Obj::RANDOM_MAJOR_ART:
		return std::make_pair(Obj::ARTIFACT, VLC->arth->pickRandomArtifact(getRandomGenerator(), CArtifact::ART_MAJOR));
	case Obj::RANDOM_RELIC_ART:
		return std::make_pair(Obj::ARTIFACT, VLC->arth->pickRandomArtifact(getRandomGenerator(), CArtifact::ART_RELIC));
	case Obj::RANDOM_HERO:
		return std::make_pair(Obj::HERO, pickNextHeroType(obj->tempOwner));
	case Obj::RANDOM_MONSTER:
		return std::make_pair(Obj::MONSTER, VLC->creh->pickRandomMonster(getRandomGenerator()));
	case Obj::RANDOM_MONSTER_L1:
		return std::make_pair(Obj::MONSTER, VLC->creh->pickRandomMonster(getRandomGenerator(), 1));
	case Obj::RANDOM_MONSTER_L2:
		return std::make_pair(Obj::MONSTER, VLC->creh->pickRandomMonster(getRandomGenerator(), 2));
	case Obj::RANDOM_MONSTER_L3:
		return std::make_pair(Obj::MONSTER, VLC->creh->pickRandomMonster(getRandomGenerator(), 3));
	case Obj::RANDOM_MONSTER_L4:
		return std::make_pair(Obj::MONSTER, VLC->creh->pickRandomMonster(getRandomGenerator(), 4));
	case Obj::RANDOM_RESOURCE:
		return std::make_pair(Obj::RESOURCE,getRandomGenerator().nextInt(6)); //now it's OH3 style, use %8 for mithril
	case Obj::RANDOM_TOWN:
		{
			PlayerColor align = (dynamic_cast<CGTownInstance *>(obj))->alignmentToPlayer;
			si32 f; // can be negative (for random)
			if(align >= PlayerColor::PLAYER_LIMIT) //same as owner / random
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
					f = getRandomGenerator().nextInt((int)VLC->townh->size() - 1);
				}
				while ((*VLC->townh)[f]->town == nullptr); // find playable faction
			}
			return std::make_pair(Obj::TOWN,f);
		}
	case Obj::RANDOM_MONSTER_L5:
		return std::make_pair(Obj::MONSTER, VLC->creh->pickRandomMonster(getRandomGenerator(), 5));
	case Obj::RANDOM_MONSTER_L6:
		return std::make_pair(Obj::MONSTER, VLC->creh->pickRandomMonster(getRandomGenerator(), 6));
	case Obj::RANDOM_MONSTER_L7:
		return std::make_pair(Obj::MONSTER, VLC->creh->pickRandomMonster(getRandomGenerator(), 7));
	case Obj::RANDOM_DWELLING:
	case Obj::RANDOM_DWELLING_LVL:
	case Obj::RANDOM_DWELLING_FACTION:
		{
			auto * dwl = dynamic_cast<CGDwelling *>(obj);
			int faction;

			//if castle alignment available
			if(auto * info = dynamic_cast<CCreGenAsCastleInfo *>(dwl->info))
			{
				faction = getRandomGenerator().nextInt((int)VLC->townh->size() - 1);
				if(info->asCastle && !info->instanceId.empty())
				{
					auto iter = map->instanceNames.find(info->instanceId);

					if(iter == map->instanceNames.end())
						logGlobal->error("Map object not found: %s", info->instanceId);
					else
					{
						auto elem = iter->second;
						if(elem->ID==Obj::RANDOM_TOWN)
						{
							randomizeObject(elem.get()); //we have to randomize the castle first
							faction = elem->subID;
						}
						else if(elem->ID==Obj::TOWN)
							faction = elem->subID;
						else
							logGlobal->error("Map object must be town: %s", info->instanceId);
					}
				}
				else if(info->asCastle)
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
					std::set<int> temp;

					for(int i = 0; i < info->allowedFactions.size(); i++)
						if(info->allowedFactions[i])
							temp.insert(i);

					if(temp.empty())
						logGlobal->error("Random faction selection failed. Nothing is allowed. Fall back to random.");
					else
						faction = *RandomGeneratorUtil::nextItem(temp, getRandomGenerator());
				}
			}
			else // castle alignment fixed
				faction = obj->subID;

			int level;

			//if level set to range
			if(auto * info = dynamic_cast<CCreGenLeveledInfo *>(dwl->info))
			{
				level = getRandomGenerator().nextInt(info->minLevel, info->maxLevel);
			}
			else // fixed level
			{
				level = obj->subID;
			}

			delete dwl->info;
			dwl->info = nullptr;

			std::pair<Obj, int> result(Obj::NO_OBJ, -1);
			CreatureID cid = (*VLC->townh)[faction]->town->creatures[level][0];

			//NOTE: this will pick last dwelling with this creature (Mantis #900)
			//check for block map equality is better but more complex solution
			auto testID = [&](const Obj & primaryID) -> void
			{
				auto dwellingIDs = VLC->objtypeh->knownSubObjects(primaryID);
				for (si32 entry : dwellingIDs)
				{
					const auto * handler = dynamic_cast<const CDwellingInstanceConstructor *>(VLC->objtypeh->getHandlerFor(primaryID, entry).get());

					if (handler->producesCreature(VLC->creh->objects[cid]))
						result = std::make_pair(primaryID, entry);
				}
			};

			testID(Obj::CREATURE_GENERATOR1);
			if (result.first == Obj::NO_OBJ)
				testID(Obj::CREATURE_GENERATOR4);

			if (result.first == Obj::NO_OBJ)
			{
				logGlobal->error("Error: failed to find dwelling for %s of level %d", (*VLC->townh)[faction]->getNameTranslated(), int(level));
				result = std::make_pair(Obj::CREATURE_GENERATOR1, *RandomGeneratorUtil::nextItem(VLC->objtypeh->knownSubObjects(Obj::CREATURE_GENERATOR1), getRandomGenerator()));
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
		auto * h = dynamic_cast<CGHeroInstance *>(cur);
		cur->setType(ran.first, ran.second);
		map->heroesOnMap.emplace_back(h);
	}
	else if(ran.first==Obj::TOWN)//special code for town
	{
		auto * t = dynamic_cast<CGTownInstance *>(cur);
		cur->setType(ran.first, ran.second);
		map->towns.emplace_back(t);
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
	applier = std::make_shared<CApplier<CBaseForGSApply>>();
	registerTypesClientPacks1(*applier);
	registerTypesClientPacks2(*applier);
	//objCaller = new CObjectCallersHandler();
	globalEffects.setDescription("Global effects");
	globalEffects.setNodeType(CBonusSystemNode::GLOBAL_EFFECTS);
}

CGameState::~CGameState()
{
	map.dellNull();
	curB.dellNull();

	for(auto ptr : hpool.heroesPool) // clean hero pool
		ptr.second.dellNull();
}

void CGameState::preInit(Services * services)
{
	this->services = services;
}

void CGameState::init(const IMapService * mapService, StartInfo * si, bool allowSavingRandomMap)
{
	preInitAuto();
	logGlobal->info("\tUsing random seed: %d", si->seedToBeUsed);
	getRandomGenerator().setSeed(si->seedToBeUsed);
	scenarioOps = CMemorySerializer::deepCopy(*si).release();
	initialOpts = CMemorySerializer::deepCopy(*si).release();
	si = nullptr;

	switch(scenarioOps->mode)
	{
	case StartInfo::NEW_GAME:
		initNewGame(mapService, allowSavingRandomMap);
		break;
	case StartInfo::CAMPAIGN:
		initCampaign();
		break;
	default:
		logGlobal->error("Wrong mode: %d", static_cast<int>(scenarioOps->mode));
		return;
	}
	VLC->arth->initAllowedArtifactsList(map->allowedArtifact);
	logGlobal->info("Map loaded!");

	checkMapChecksum();

	day = 0;

	logGlobal->debug("Initialization:");

	initGlobalBonuses();
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
	placeHeroesInTowns();
	initMapObjects();
	buildBonusSystemTree();
	initVisitingAndGarrisonedHeroes();
	initFogOfWar();

	// Explicitly initialize static variables
	for(auto & elem : players)
	{
		CGKeys::playerKeyMap[elem.first] = std::set<ui8>();
	}
	for(auto & elem : teams)
	{
		CGObelisk::visited[elem.first] = 0;
	}

	logGlobal->debug("\tChecking objectives");
	map->checkForObjectives(); //needs to be run when all objects are properly placed

	auto seedAfterInit = getRandomGenerator().nextInt();
	logGlobal->info("Seed after init is %d (before was %d)", seedAfterInit, scenarioOps->seedToBeUsed);
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

void CGameState::updateEntity(Metatype metatype, int32_t index, const JsonNode & data)
{
	switch(metatype)
	{
	case Metatype::ARTIFACT_INSTANCE:
		logGlobal->error("Artifact instance update is not implemented");
		break;
	case Metatype::CREATURE_INSTANCE:
		logGlobal->error("Creature instance update is not implemented");
		break;
	case Metatype::HERO_INSTANCE:
		//index is hero type
		if(index >= 0 && index < map->allHeroes.size())
		{
			CGHeroInstance * hero = map->allHeroes.at(index);
			hero->updateFrom(data);
		}
		else
		{
			logGlobal->error("Update entity: hero index %s is out of range [%d,%d]", index, 0,  map->allHeroes.size());
		}
		break;
	case Metatype::MAP_OBJECT_INSTANCE:
		if(index >= 0 && index < map->objects.size())
		{
			CGObjectInstance * obj = getObjInstance(ObjectInstanceID(index));
			obj->updateFrom(data);
		}
		else
		{
			logGlobal->error("Update entity: object index %s is out of range [%d,%d]", index, 0,  map->objects.size());
		}
		break;
	default:
		services->updateEntity(metatype, index, data);
		break;
	}
}

void CGameState::updateOnLoad(StartInfo * si)
{
	preInitAuto();
	scenarioOps->playerInfos = si->playerInfos;
	for(auto & i : si->playerInfos)
		gs->players[i.first].human = i.second.isControlledByHuman();
}

void CGameState::preInitAuto()
{
	if(services == nullptr)
	{
		logGlobal->error("Game state preinit missing");
		preInit(VLC);
	}
}

void CGameState::initNewGame(const IMapService * mapService, bool allowSavingRandomMap)
{
	if(scenarioOps->createRandomMap())
	{
		logGlobal->info("Create random map.");
		CStopWatch sw;

		// Gen map
		CMapGenerator mapGenerator(*scenarioOps->mapGenOptions, scenarioOps->seedToBeUsed);

		std::unique_ptr<CMap> randomMap = mapGenerator.generate();

		if(allowSavingRandomMap)
		{
			try
			{
				auto path = VCMIDirs::get().userCachePath() / "RandomMaps";
				boost::filesystem::create_directories(path);

				std::shared_ptr<CMapGenOptions> options = scenarioOps->mapGenOptions;

				const std::string templateName = options->getMapTemplate()->getName();
				const ui32 seed = scenarioOps->seedToBeUsed;

				const std::string fileName = boost::str(boost::format("%s_%d.vmap") % templateName % seed );
				const auto fullPath = path / fileName;

				mapService->saveMap(randomMap, fullPath);

				logGlobal->info("Random map has been saved to:");
				logGlobal->info(fullPath.string());
			}
			catch(...)
			{
				logGlobal->error("Saving random map failed with exception");
				handleException();
			}
		}

		map = randomMap.release();
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
				if(playerSettings.isControlledByAI() && playerSettings.name.empty())
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

		logGlobal->info("Generated random map in %i ms.", sw.getDiff());
	}
	else
	{
		logGlobal->info("Open map file: %s", scenarioOps->mapname);
		const ResourceID mapURI(scenarioOps->mapname, EResType::MAP);
		map = mapService->loadMap(mapURI).release();
	}
}

void CGameState::initCampaign()
{
	logGlobal->info("Open campaign map file: %d", scenarioOps->campState->currentMap.value());
	map = scenarioOps->campState->getMap();
}

void CGameState::checkMapChecksum()
{
	logGlobal->info("\tOur checksum for the map: %d", map->checksum);
	if(scenarioOps->mapfileChecksum)
	{
		logGlobal->info("\tServer checksum for %s: %d", scenarioOps->mapname, scenarioOps->mapfileChecksum);
		if(map->checksum != scenarioOps->mapfileChecksum)
		{
			logGlobal->error("Wrong map checksum!!!");
			throw std::runtime_error("Wrong checksum");
		}
	}
	else
	{
		scenarioOps->mapfileChecksum = map->checksum;
	}
}

void CGameState::initGlobalBonuses()
{
	const JsonNode & baseBonuses = VLC->settings()->getValue(EGameSettings::BONUSES_GLOBAL);
	logGlobal->debug("\tLoading global bonuses");
	for(const auto & b : baseBonuses.Struct())
	{
		auto bonus = JsonUtils::parseBonus(b.second);
		bonus->source = BonusSource::GLOBAL;//for all
		bonus->sid = -1; //there is one global object
		globalEffects.addNewBonus(bonus);
	}
	VLC->creh->loadCrExpBon(globalEffects);
}

void CGameState::initGrailPosition()
{
	logGlobal->debug("\tPicking grail position");
	//pick grail location
	if(map->grailPos.x < 0 || map->grailRadius) //grail not set or set within a radius around some place
	{
		if(!map->grailRadius) //radius not given -> anywhere on map
			map->grailRadius = map->width * 2;

		std::vector<int3> allowedPos;
		static const int BORDER_WIDTH = 9; // grail must be at least 9 tiles away from border

		// add all not blocked tiles in range

		for (int z = 0; z < map->levels(); z++)
		{
			for(int x = BORDER_WIDTH; x < map->width - BORDER_WIDTH ; x++)
			{
				for(int y = BORDER_WIDTH; y < map->height - BORDER_WIDTH; y++)
				{
					const TerrainTile &t = map->getTile(int3(x, y, z));
					if(!t.blocked
						&& !t.visitable
						&& t.terType->isLand()
						&& t.terType->isPassable()
						&& (int)map->grailPos.dist2dSQ(int3(x, y, z)) <= (map->grailRadius * map->grailRadius))
						allowedPos.emplace_back(x, y, z);
				}
			}
		}

		//remove tiles with holes
		for(auto & elem : map->objects)
			if(elem && elem->ID == Obj::HOLE)
				allowedPos -= elem->pos;

		if(!allowedPos.empty())
		{
			map->grailPos = *RandomGeneratorUtil::nextItem(allowedPos, getRandomGenerator());
		}
		else
		{
			logGlobal->warn("Grail cannot be placed, no appropriate tile found!");
		}
	}
}

void CGameState::initRandomFactionsForPlayers()
{
	logGlobal->debug("\tPicking random factions for players");
	for(auto & elem : scenarioOps->playerInfos)
	{
		if(elem.second.castle==-1)
		{
			auto randomID = getRandomGenerator().nextInt((int)map->players[elem.first.getNum()].allowedFactions.size() - 1);
			auto iter = map->players[elem.first.getNum()].allowedFactions.begin();
			std::advance(iter, randomID);

			elem.second.castle = *iter;
		}
	}
}

void CGameState::randomizeMapObjects()
{
	logGlobal->debug("\tRandomizing objects");
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
	logGlobal->debug("\tCreating player entries in gs");
	for(auto & elem : scenarioOps->playerInfos)
	{
		PlayerState & p = players[elem.first];
		p.color=elem.first;
		p.human = elem.second.isControlledByHuman();
		p.team = map->players[elem.first.getNum()].team;
		teams[p.team].id = p.team;//init team
		teams[p.team].players.insert(elem.first);//add player to team
	}
}

void CGameState::placeCampaignHeroes()
{
	if (scenarioOps->campState)
	{
		// place bonus hero
		auto campaignBonus = scenarioOps->campState->getBonusForCurrentMap();
		bool campaignGiveHero = campaignBonus && campaignBonus->type == CScenarioTravel::STravelBonus::HERO;

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
			logGlobal->debug("\tGenerate list of hero placeholders");
			auto campaignHeroReplacements = generateCampaignHeroesToReplace(crossoverHeroes);

			logGlobal->debug("\tPrepare crossover heroes");
			prepareCrossoverHeroes(campaignHeroReplacements, scenarioOps->campState->getCurrentScenario().travelOptions);

			// remove same heroes on the map which will be added through crossover heroes
			// INFO: we will remove heroes because later it may be possible that the API doesn't allow having heroes
			// with the same hero type id
			std::vector<CGHeroInstance *> removedHeroes;

			for(auto & campaignHeroReplacement : campaignHeroReplacements)
			{
				auto * hero = getUsedHero(HeroTypeID(campaignHeroReplacement.hero->subID));
				if(hero)
				{
					removedHeroes.push_back(hero);
					map->heroesOnMap -= hero;
					map->objects[hero->id.getNum()] = nullptr;
					map->removeBlockVisTiles(hero, true);
				}
			}

			logGlobal->debug("\tReplace placeholders with heroes");
			replaceHeroesPlaceholders(campaignHeroReplacements);

			// now add removed heroes again with unused type ID
			for(auto * hero : removedHeroes)
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
						heroTypeId = (*RandomGeneratorUtil::nextItem(unusedHeroTypeIds, getRandomGenerator())).getNum();
					}
					else
					{
						logGlobal->error("No free hero type ID found to replace prison.");
						assert(0);
					}
				}
				else
				{
					assert(0); // should not happen
				}

				hero->subID = heroTypeId;
				hero->portrait = hero->subID;
				map->getEditManager()->insertObject(hero);
			}
		}
	}

	// remove hero placeholders on map
	for(auto obj : map->objects)
	{
		if(obj && obj->ID == Obj::HERO_PLACEHOLDER)
		{
			auto heroPlaceholder = dynamic_cast<CGHeroPlaceholder *>(obj.get());
			map->removeBlockVisTiles(heroPlaceholder, true);
			map->instanceNames.erase(obj->instanceName);
			map->objects[heroPlaceholder->id.getNum()] = nullptr;
			delete heroPlaceholder;
		}
	}
}

void CGameState::placeStartingHero(const PlayerColor & playerColor, const HeroTypeID & heroTypeId, int3 townPos)
{
	for(auto town : map->towns)
	{
		if(town->getPosition() == townPos)
		{
			townPos = town->visitablePos();
			break;
		}
	}

	CGObjectInstance * hero = createObject(Obj::HERO, heroTypeId.getNum(), townPos, playerColor);
	hero->pos += hero->getVisitableOffset();
	map->getEditManager()->insertObject(hero);
}

CGameState::CrossoverHeroesList CGameState::getCrossoverHeroesFromPreviousScenarios() const
{
	CrossoverHeroesList crossoverHeroes;

	auto campaignState = scenarioOps->campState;
	auto bonus = campaignState->getBonusForCurrentMap();
	if(bonus && bonus->type == CScenarioTravel::STravelBonus::HEROES_FROM_PREVIOUS_SCENARIO)
	{
		std::vector<CGHeroInstance *> heroes;
		for(auto & node : campaignState->camp->scenarios[bonus->info2].crossoverHeroes)
		{
			auto * h = CCampaignState::crossoverDeserialize(node);
			heroes.push_back(h);
		}
		crossoverHeroes.heroesFromAnyPreviousScenarios = crossoverHeroes.heroesFromPreviousScenario = heroes;
	}
	else
	{
		if(!campaignState->mapsConquered.empty())
		{
			std::vector<CGHeroInstance *> heroes = {};

			crossoverHeroes.heroesFromAnyPreviousScenarios = crossoverHeroes.heroesFromPreviousScenario = heroes;
			crossoverHeroes.heroesFromPreviousScenario = heroes;

			for(auto mapNr : campaignState->mapsConquered)
			{
				// create a list of deleted heroes
				auto & scenario = campaignState->camp->scenarios[mapNr];
				auto lostCrossoverHeroes = scenario.getLostCrossoverHeroes();

				// remove heroes which didn't reached the end of the scenario, but were available at the start
				for(auto * hero : lostCrossoverHeroes)
				{
					//					auto hero = CCampaignState::crossoverDeserialize(node);
					vstd::erase_if(crossoverHeroes.heroesFromAnyPreviousScenarios, [hero](CGHeroInstance * h)
					{
						return hero->subID == h->subID;
					});
				}

				// now add heroes which completed the scenario
				for(auto node : scenario.crossoverHeroes)
				{
					auto * hero = CCampaignState::crossoverDeserialize(node);
					// add new heroes and replace old heroes with newer ones
					auto it = range::find_if(crossoverHeroes.heroesFromAnyPreviousScenarios, [hero](CGHeroInstance * h)
					{
						return hero->subID == h->subID;
					});

					if(it != crossoverHeroes.heroesFromAnyPreviousScenarios.end())
					{
						// replace old hero with newer one
						crossoverHeroes.heroesFromAnyPreviousScenarios[it - crossoverHeroes.heroesFromAnyPreviousScenarios.begin()] = hero;
					}
					else
					{
						// add new hero
						crossoverHeroes.heroesFromAnyPreviousScenarios.push_back(hero);
					}

					if(mapNr == campaignState->mapsConquered.back())
					{
						crossoverHeroes.heroesFromPreviousScenario.push_back(hero);
					}
				}
			}
		}
	}

	return crossoverHeroes;
}

void CGameState::prepareCrossoverHeroes(std::vector<CGameState::CampaignHeroReplacement> & campaignHeroReplacements, const CScenarioTravel & travelOptions)
{
	// create heroes list for convenience iterating
	std::vector<CGHeroInstance *> crossoverHeroes;
	crossoverHeroes.reserve(campaignHeroReplacements.size());
	for(auto & campaignHeroReplacement : campaignHeroReplacements)
	{
		crossoverHeroes.push_back(campaignHeroReplacement.hero);
	}

	// TODO replace magic numbers with named constants
	// TODO this logic (what should be kept) should be part of CScenarioTravel and be exposed via some clean set of methods
	if(!travelOptions.whatHeroKeeps.experience)
	{
		//trimming experience
		for(CGHeroInstance * cgh : crossoverHeroes)
		{
			cgh->initExp(getRandomGenerator());
		}
	}

	if(!travelOptions.whatHeroKeeps.primarySkills)
	{
		//trimming prim skills
		for(CGHeroInstance * cgh : crossoverHeroes)
		{
			for(int g=0; g<GameConstants::PRIMARY_SKILLS; ++g)
			{
				auto sel = Selector::type()(BonusType::PRIMARY_SKILL)
					.And(Selector::subtype()(g))
					.And(Selector::sourceType()(BonusSource::HERO_BASE_SKILL));

				cgh->getBonusLocalFirst(sel)->val = cgh->type->heroClass->primarySkillInitial[g];
			}
		}
	}

	if(!travelOptions.whatHeroKeeps.secondarySkills)
	{
		//trimming sec skills
		for(CGHeroInstance * cgh : crossoverHeroes)
		{
			cgh->secSkills = cgh->type->secSkillsInit;
			cgh->recreateSecondarySkillsBonuses();
		}
	}

	if(!travelOptions.whatHeroKeeps.spells)
	{
		for(CGHeroInstance * cgh : crossoverHeroes)
		{
			cgh->removeSpellbook();
		}
	}

	if(!travelOptions.whatHeroKeeps.artifacts)
	{
		//trimming artifacts
		for(CGHeroInstance * hero : crossoverHeroes)
		{
			size_t totalArts = GameConstants::BACKPACK_START + hero->artifactsInBackpack.size();
			for (size_t i = 0; i < totalArts; i++ )
			{
				auto artifactPosition = ArtifactPosition((si32)i);
				if(artifactPosition == ArtifactPosition::SPELLBOOK) continue; // do not handle spellbook this way

				const ArtSlotInfo *info = hero->getSlot(artifactPosition);
				if(!info)
					continue;

				// TODO: why would there be nullptr artifacts?
				const CArtifactInstance *art = info->artifact;
				if(!art)
					continue;

				bool takeable = travelOptions.artifactsKeptByHero.count(art->artType->getId());

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
			return !travelOptions.monstersKeptByHero.count(crid);
		};

		auto stacksCopy = cgh->stacks; //copy of the map, so we can iterate iover it and remove stacks
		for(auto &slotPair : stacksCopy)
			if(shouldSlotBeErased(slotPair))
				cgh->eraseStack(slotPair.first);
	}

	// Removing short-term bonuses
	for(CGHeroInstance * cgh : crossoverHeroes)
	{
		cgh->removeBonusesRecursive(CSelector(Bonus::OneDay)
			.Or(CSelector(Bonus::OneWeek))
			.Or(CSelector(Bonus::NTurns))
			.Or(CSelector(Bonus::NDays))
			.Or(CSelector(Bonus::OneBattle)));
	}

}

void CGameState::placeStartingHeroes()
{
	logGlobal->debug("\tGiving starting hero");

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
					if(campaignBonus->type == CScenarioTravel::STravelBonus::HERO && playerColor == PlayerColor(campaignBonus->info1))
						continue;
				}
			}

			int heroTypeId = pickNextHeroType(playerColor);
			if(playerSettingPair.second.hero == -1)
				playerSettingPair.second.hero = heroTypeId;

			placeStartingHero(playerColor, HeroTypeID(heroTypeId), playerInfo.posOfMainTown);
		}
	}
}

void CGameState::initStartingResources()
{
	logGlobal->debug("\tSetting up resources");
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
		for(const auto & playerInfo : scenarioOps->playerInfos)
		{
			if(playerInfo.second.isControlledByHuman())
				ret.push_back(&playerInfo.second);
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
					res.push_back(GameResID(EGameResID::WOOD)); 
					res.push_back(GameResID(EGameResID::ORE));
					break;
				case 0xFE:  //rare
					res.push_back(GameResID(EGameResID::MERCURY));
					res.push_back(GameResID(EGameResID::SULFUR));
					res.push_back(GameResID(EGameResID::CRYSTAL));
					res.push_back(GameResID(EGameResID::GEMS));
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
			logGlobal->warn("Hero with uninitialized owner!");
			continue;
		}

		hero->initHero(getRandomGenerator());
		getPlayerState(hero->getOwner())->heroes.push_back(hero);
		map->allHeroes[hero->type->getIndex()] = hero;
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
		ph->initHero(getRandomGenerator());
		hpool.heroesPool[ph->subID] = ph;
		hpool.pavailable[ph->subID] = 0xff;
		heroesToCreate.erase(ph->type->getId());

		map->allHeroes[ph->subID] = ph;
	}

	for(const HeroTypeID & htype : heroesToCreate) //all not used allowed heroes go with default state into the pool
	{
		auto * vhi = new CGHeroInstance();
		vhi->initHero(getRandomGenerator(), htype);

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
					logGlobal->warn("Cannot give bonus to hero cause there are no heroes!");
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
	const std::optional<CScenarioTravel::STravelBonus> & curBonus = scenarioOps->campState->getBonusForCurrentMap();
	if(!curBonus)
		return;

	if(curBonus->isBonusForHero())
	{
		//apply bonus
		switch (curBonus->type)
		{
		case CScenarioTravel::STravelBonus::SPELL:
			hero->addSpellToSpellbook(SpellID(curBonus->info2));
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
			if(!gs->giveHeroArtifact(hero, static_cast<ArtifactID>(curBonus->info2)))
				logGlobal->error("Cannot give starting artifact - no free slots!");
			break;
		case CScenarioTravel::STravelBonus::SPELL_SCROLL:
			{
				CArtifactInstance * scroll = CArtifactInstance::createScroll(SpellID(curBonus->info2));
				const auto slot = ArtifactUtils::getArtAnyPosition(hero, scroll->getTypeId());
				if(ArtifactUtils::isSlotEquipment(slot) || ArtifactUtils::isSlotBackpack(slot))
					scroll->putAt(ArtifactLocation(hero, slot));
				else
					logGlobal->error("Cannot give starting scroll - no free slots!");
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
					auto bb = std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::PRIMARY_SKILL, BonusSource::CAMPAIGN_BONUS, val, *scenarioOps->campState->currentMap, g);
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
	logGlobal->debug("\tFog of war"); //FIXME: should be initialized after all bonuses are set

	int layers = map->levels();
	for(auto & elem : teams)
	{
		auto fow = elem.second.fogOfWarMap;
		fow->resize(boost::extents[layers][map->width][map->height]);
		std::fill(fow->data(), fow->data() + fow->num_elements(), 0);

		for(CGObjectInstance *obj : map->objects)
		{
			if(!obj || !vstd::contains(elem.second.players, obj->tempOwner)) continue; //not a flagged object

			std::unordered_set<int3> tiles;
			getTilesInRange(tiles, obj->getSightCenter(), obj->getSightRadius(), obj->tempOwner, 1);
			for(const int3 & tile : tiles)
			{
				(*elem.second.fogOfWarMap)[tile.z][tile.x][tile.y] = 1;
			}
		}
	}
}

void CGameState::initStartingBonus()
{
	if (scenarioOps->mode == StartInfo::CAMPAIGN)
		return;
	// These are the single scenario bonuses; predefined
	// campaign bonuses are spread out over other init* functions.

	logGlobal->debug("\tStarting bonuses");
	for(auto & elem : players)
	{
		//starting bonus
		if(scenarioOps->playerInfos[elem.first].bonus==PlayerSettings::RANDOM)
			scenarioOps->playerInfos[elem.first].bonus = static_cast<PlayerSettings::Ebonus>(getRandomGenerator().nextInt(2));
		switch(scenarioOps->playerInfos[elem.first].bonus)
		{
		case PlayerSettings::GOLD:
			elem.second.resources[EGameResID::GOLD] += getRandomGenerator().nextInt(5, 10) * 100;
			break;
		case PlayerSettings::RESOURCE:
			{
				auto res = (*VLC->townh)[scenarioOps->playerInfos[elem.first].castle]->town->primaryRes;
				if(res == EGameResID::WOOD_AND_ORE)
				{
					int amount = getRandomGenerator().nextInt(5, 10);
					elem.second.resources[EGameResID::WOOD] += amount;
					elem.second.resources[EGameResID::ORE] += amount;
				}
				else
				{
					elem.second.resources[res] += getRandomGenerator().nextInt(3, 6);
				}
				break;
			}
		case PlayerSettings::ARTIFACT:
			{
				if(elem.second.heroes.empty())
				{
					logGlobal->error("Cannot give starting artifact - no heroes!");
					break;
				}
				const Artifact * toGive = VLC->arth->pickRandomArtifact(getRandomGenerator(), CArtifact::ART_TREASURE).toArtifact(VLC->artifacts());

				CGHeroInstance *hero = elem.second.heroes[0];
				if(!giveHeroArtifact(hero, toGive->getId()))
					logGlobal->error("Cannot give starting artifact - no free slots!");
			}
			break;
		}
	}
}

void CGameState::initTowns()
{
	logGlobal->debug("\tTowns");

	//campaign bonuses for towns
	if (scenarioOps->mode == StartInfo::CAMPAIGN)
	{
		auto chosenBonus = scenarioOps->campState->getBonusForCurrentMap();

		if (chosenBonus && chosenBonus->type == CScenarioTravel::STravelBonus::BUILDING)
		{
			for (int g=0; g<map->towns.size(); ++g)
			{
				PlayerState * owner = getPlayerState(map->towns[g]->getOwner());
				if (owner)
				{
					PlayerInfo & pi = map->players[owner->color.getNum()];

					if (owner->human && //human-owned
						map->towns[g]->pos == pi.posOfMainTown)
					{
						BuildingID buildingId;
						if(scenarioOps->campState->camp->header.version == CampaignVersion::VCMI)
							buildingId = BuildingID(chosenBonus->info1);
						else
							buildingId = CBuildingHandler::campToERMU(chosenBonus->info1, map->towns[g]->subID, map->towns[g]->builtBuildings);

						map->towns[g]->builtBuildings.insert(buildingId);
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
			vti->town = (*VLC->townh)[vti->subID]->town;
		}
		if(vti->getNameTranslated().empty())
		{
			size_t nameID = getRandomGenerator().nextInt(vti->getTown()->getRandomNamesCount() - 1);
			vti->setNameTranslated(vti->getTown()->getRandomNameTranslated(nameID));
		}

		//init buildings
		if(vstd::contains(vti->builtBuildings, BuildingID::DEFAULT)) //give standard set of buildings
		{
			vti->builtBuildings.erase(BuildingID::DEFAULT);
			vti->builtBuildings.insert(BuildingID::VILLAGE_HALL);
			if(vti->tempOwner != PlayerColor::NEUTRAL)
				vti->builtBuildings.insert(BuildingID::TAVERN);

			auto definesBuildingsChances = VLC->settings()->getVector(EGameSettings::TOWNS_STARTING_DWELLING_CHANCES);

			BuildingID basicDwellings[] = { BuildingID::DWELL_FIRST, BuildingID::DWELL_LVL_2, BuildingID::DWELL_LVL_3, BuildingID::DWELL_LVL_4, BuildingID::DWELL_LVL_5, BuildingID::DWELL_LVL_6, BuildingID::DWELL_LVL_7 };

			for(int i = 0; i < definesBuildingsChances.size(); i++)
			{
				if((getRandomGenerator().nextInt(1,100) <= definesBuildingsChances[i]))
				{
					vti->builtBuildings.insert(basicDwellings[i]);
				}
			}
		}

		//init hordes
		for (int i = 0; i < GameConstants::CREATURES_PER_TOWN; i++)
		{
			if (vstd::contains(vti->builtBuildings, (BuildingID::HORDE_PLACEHOLDER1 - i))) //if we have horde for this level
			{
				vti->builtBuildings.erase(BuildingID(BuildingID::HORDE_PLACEHOLDER1 - i));//remove old ID
				if (vti->getTown()->hordeLvl.at(0) == i)//if town first horde is this one
				{
					vti->builtBuildings.insert(BuildingID::HORDE_1);//add it
					//if we have upgraded dwelling as well
					if (vstd::contains(vti->builtBuildings, (BuildingID::DWELL_UP_FIRST + i)))
						vti->builtBuildings.insert(BuildingID::HORDE_1_UPGR);//add it as well
				}
				if (vti->getTown()->hordeLvl.at(1) == i)//if town second horde is this one
				{
					vti->builtBuildings.insert(BuildingID::HORDE_2);
					if (vstd::contains(vti->builtBuildings, (BuildingID::DWELL_UP_FIRST + i)))
						vti->builtBuildings.insert(BuildingID::HORDE_2_UPGR);
				}
			}
		}

		//#1444 - remove entries that don't have buildings defined (like some unused extra town hall buildings)
		//But DO NOT remove horde placeholders before they are replaced
		vstd::erase_if(vti->builtBuildings, [vti](const BuildingID & bid)
			{
				return !vti->getTown()->buildings.count(bid) || !vti->getTown()->buildings.at(bid);
			});

		if (vstd::contains(vti->builtBuildings, BuildingID::SHIPYARD) && vti->shipyardStatus()==IBoatGenerator::TILE_BLOCKED)
			vti->builtBuildings.erase(BuildingID::SHIPYARD);//if we have harbor without water - erase it (this is H3 behaviour)

		//Early check for #1444-like problems
		for([[maybe_unused]] const auto & building : vti->builtBuildings)
		{
			assert(vti->getTown()->buildings.at(building) != nullptr);
		}

		//town events
		for(CCastleEvent &ev : vti->events)
		{
			for (int i = 0; i<GameConstants::CREATURES_PER_TOWN; i++)
				if (vstd::contains(ev.buildings,(-31-i))) //if we have horde for this level
				{
					ev.buildings.erase(BuildingID(-31-i));
					if (vti->getTown()->hordeLvl.at(0) == i)
						ev.buildings.insert(BuildingID::HORDE_1);
					if (vti->getTown()->hordeLvl.at(1) == i)
						ev.buildings.insert(BuildingID::HORDE_2);
				}
		}
		//init spells
		vti->spells.resize(GameConstants::SPELL_LEVELS);

		for(ui32 z=0; z<vti->obligatorySpells.size();z++)
		{
			const auto * s = vti->obligatorySpells[z].toSpell();
			vti->spells[s->level-1].push_back(s->id);
			vti->possibleSpells -= s->id;
		}
		while(!vti->possibleSpells.empty())
		{
			ui32 total=0;
			int sel = -1;

			for(ui32 ps=0;ps<vti->possibleSpells.size();ps++)
				total += vti->possibleSpells[ps].toSpell()->getProbability(vti->getFaction());

			if (total == 0) // remaining spells have 0 probability
				break;

			auto r = getRandomGenerator().nextInt(total - 1);
			for(ui32 ps=0; ps<vti->possibleSpells.size();ps++)
			{
				r -= vti->possibleSpells[ps].toSpell()->getProbability(vti->getFaction());
				if(r<0)
				{
					sel = ps;
					break;
				}
			}
			if(sel<0)
				sel=0;

			const auto * s = vti->possibleSpells[sel].toSpell();
			vti->spells[s->level-1].push_back(s->id);
			vti->possibleSpells -= s->id;
		}
		vti->possibleSpells.clear();
		if(vti->getOwner() != PlayerColor::NEUTRAL)
			getPlayerState(vti->getOwner())->towns.emplace_back(vti);
	}
}

void CGameState::initMapObjects()
{
	logGlobal->debug("\tObject initialization");

//	objCaller->preInit();
	for(CGObjectInstance *obj : map->objects)
	{
		if(obj)
		{
			logGlobal->trace("Calling Init for object %d, %s, %s", obj->id.getNum(), obj->typeName, obj->subTypeName);
			obj->initObj(getRandomGenerator());
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
				auto * q = dynamic_cast<CGSeerHut *>(obj);
				assert (q);
				q->setObjToKill();
			}
		}
	}
	CGSubterraneanGate::postInit(); //pairing subterranean gates

	map->calculateGuardingGreaturePositions(); //calculate once again when all the guards are placed and initialized
}

void CGameState::placeHeroesInTowns()
{
	for(auto & player : players)
	{
		if(player.first == PlayerColor::NEUTRAL)
			continue;

		for(CGHeroInstance * h : player.second.heroes)
		{
			for(CGTownInstance * t : player.second.towns)
			{
				if(h->visitablePos().z != t->visitablePos().z)
					continue;

				bool heroOnTownBlockableTile = t->blockingAt(h->visitablePos().x, h->visitablePos().y);

				// current hero position is at one of blocking tiles of current town
				// assume that this hero should be visiting the town (H3M format quirk) and move hero to correct position
				if (heroOnTownBlockableTile)
				{
					int3 correctedPos = h->convertFromVisitablePos(t->visitablePos());

					map->removeBlockVisTiles(h);
					h->pos = correctedPos;
					map->addBlockVisTiles(h);

					assert(t->visitableAt(h->visitablePos().x, h->visitablePos().y));
				}
			}
		}
	}
}

void CGameState::initVisitingAndGarrisonedHeroes()
{
	for(auto & player : players)
	{
		if(player.first == PlayerColor::NEUTRAL)
			continue;

		//init visiting and garrisoned heroes
		for(CGHeroInstance * h : player.second.heroes)
		{
			for(CGTownInstance * t : player.second.towns)
			{
				if(h->visitablePos().z != t->visitablePos().z)
					continue;

				if (t->visitableAt(h->visitablePos().x, h->visitablePos().y))
				{
					assert(t->visitingHero == nullptr);
					t->setVisitingHero(h);
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

BattleField CGameState::battleGetBattlefieldType(int3 tile, CRandomGenerator & rand)
{
	if(!tile.valid() && curB)
		tile = curB->tile;
	else if(!tile.valid() && !curB)
		return BattleField::NONE;

	const TerrainTile &t = map->getTile(tile);

	auto * topObject = t.visitableObjects.front();
	if(topObject && topObject->getBattlefield() != BattleField::NONE)
	{
		return topObject->getBattlefield();
	}

	for(auto &obj : map->objects)
	{
		//look only for objects covering given tile
		if( !obj || obj->pos.z != tile.z || !obj->coveringAt(tile.x, tile.y))
			continue;

		auto customBattlefield = obj->getBattlefield();

		if(customBattlefield != BattleField::NONE)
			return customBattlefield;
	}

	if(map->isCoastalTile(tile)) //coastal tile is always ground
		return BattleField::fromString("sand_shore");
	
	return BattleField(*RandomGeneratorUtil::nextItem(t.terType->battleFields, rand));
}


void CGameState::fillUpgradeInfo(const CArmedInstance *obj, SlotID stackPos, UpgradeInfo &out) const
{
	assert(obj);
	assert(obj->hasStackAtSlot(stackPos));

	out = fillUpgradeInfo(obj->getStack(stackPos));
}

UpgradeInfo CGameState::fillUpgradeInfo(const CStackInstance &stack) const
{
	UpgradeInfo ret;
	const CCreature *base = stack.type;

	const CGHeroInstance * h = stack.armyObj->ID == Obj::HERO ? dynamic_cast<const CGHeroInstance *>(stack.armyObj) : nullptr;
	const CGTownInstance *t = nullptr;

	if(stack.armyObj->ID == Obj::TOWN)
		t = dynamic_cast<const CGTownInstance *>(stack.armyObj);
	else if(h)
	{	//hero specialty
		TConstBonusListPtr lista = h->getBonuses(Selector::typeSubtype(BonusType::SPECIAL_UPGRADE, base->getId()));
		for(const auto & it : *lista)
		{
			auto nid = CreatureID(it->additionalInfo[0]);
			if (nid != base->getId()) //in very specific case the upgrade is available by default (?)
			{
				ret.newID.push_back(nid);
				ret.cost.push_back(nid.toCreature()->getFullRecruitCost() - base->getFullRecruitCost());
			}
		}
		t = h->visitedTown;
	}
	if(t)
	{
		for(const CGTownInstance::TCreaturesSet::value_type & dwelling : t->creatures)
		{
			if (vstd::contains(dwelling.second, base->getId())) //Dwelling with our creature
			{
				for(const auto & upgrID : dwelling.second)
				{
					if(vstd::contains(base->upgrades, upgrID)) //possible upgrade
					{
						ret.newID.push_back(upgrID);
						ret.cost.push_back(upgrID.toCreature()->getFullRecruitCost() - base->getFullRecruitCost());
					}
				}
			}
		}
	}

	//hero is visiting Hill Fort
	if(h && map->getTile(h->visitablePos()).visitableObjects.front()->ID == Obj::HILL_FORT)
	{
		static const int costModifiers[] = {0, 25, 50, 75, 100}; //we get cheaper upgrades depending on level
		const int costModifier = costModifiers[std::min<int>(std::max((int)base->getLevel() - 1, 0), std::size(costModifiers) - 1)];

		for(const auto & nid : base->upgrades)
		{
			ret.newID.push_back(nid);
			ret.cost.push_back((nid.toCreature()->getFullRecruitCost() - base->getFullRecruitCost()) * costModifier / 100);
		}
	}

	if(!ret.newID.empty())
		ret.oldID = base->getId();

	for (ResourceSet &cost : ret.cost)
		cost.positive(); //upgrade cost can't be negative, ignore missing resources

	return ret;
}

PlayerRelations::PlayerRelations CGameState::getPlayerRelations( PlayerColor color1, PlayerColor color2 ) const
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

void CGameState::apply(CPack *pack)
{
	ui16 typ = typeList.getTypeID(pack);
	applier->getApplier(typ)->applyOnGS(this, pack);
}

void CGameState::calculatePaths(const CGHeroInstance *hero, CPathsInfo &out)
{
	calculatePaths(std::make_shared<SingleHeroPathfinderConfig>(out, this, hero));
}

void CGameState::calculatePaths(const std::shared_ptr<PathfinderConfig> & config)
{
	//FIXME: creating pathfinder is costly, maybe reset / clear is enough?
	CPathfinder pathfinder(this, config);
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
	return gs->map->guardingCreaturePositions[pos.z][pos.x][pos.y];
}

void CGameState::updateRumor()
{
	static std::vector<RumorState::ERumorType> rumorTypes = {RumorState::TYPE_MAP, RumorState::TYPE_SPECIAL, RumorState::TYPE_RAND, RumorState::TYPE_RAND};
	std::vector<RumorState::ERumorTypeSpecial> sRumorTypes = {
		RumorState::RUMOR_OBELISKS, RumorState::RUMOR_ARTIFACTS, RumorState::RUMOR_ARMY, RumorState::RUMOR_INCOME};
	if(map->grailPos.valid()) // Grail should always be on map, but I had related crash I didn't manage to reproduce
		sRumorTypes.push_back(RumorState::RUMOR_GRAIL);

	int rumorId = -1;
	int rumorExtra = -1;
	auto & rand = getRandomGenerator();
	rumor.type = *RandomGeneratorUtil::nextItem(rumorTypes, rand);

	do
	{
		switch(rumor.type)
		{
		case RumorState::TYPE_SPECIAL:
		{
			SThievesGuildInfo tgi;
			obtainPlayersStats(tgi, 20);
			rumorId = *RandomGeneratorUtil::nextItem(sRumorTypes, rand);
			if(rumorId == RumorState::RUMOR_GRAIL)
			{
				rumorExtra = getTile(map->grailPos)->terType->getIndex();
				break;
			}

			std::vector<PlayerColor> players = {};
			switch(rumorId)
			{
			case RumorState::RUMOR_OBELISKS:
				players = tgi.obelisks[0];
				break;

			case RumorState::RUMOR_ARTIFACTS:
				players = tgi.artifacts[0];
				break;

			case RumorState::RUMOR_ARMY:
				players = tgi.army[0];
				break;

			case RumorState::RUMOR_INCOME:
				players = tgi.income[0];
				break;
			}
			rumorExtra = RandomGeneratorUtil::nextItem(players, rand)->getNum();

			break;
		}
		case RumorState::TYPE_MAP:
			// Makes sure that map rumors only used if there enough rumors too choose from
			if(!map->rumors.empty() && (map->rumors.size() > 1 || !rumor.last.count(RumorState::TYPE_MAP)))
			{
				rumorId = rand.nextInt((int)map->rumors.size() - 1);
				break;
			}
			else
				rumor.type = RumorState::TYPE_RAND;
			[[fallthrough]];

		case RumorState::TYPE_RAND:
			auto vector = VLC->generaltexth->findStringsWithPrefix("core.randtvrn");
			rumorId = rand.nextInt((int)vector.size() - 1);

			break;
		}
	}
	while(!rumor.update(rumorId, rumorExtra));
}

bool CGameState::isVisible(int3 pos, const std::optional<PlayerColor> & player) const
{
	if (!map->isInTheMap(pos))
		return false;
	if (!player)
		return true;
	if(player == PlayerColor::NEUTRAL)
		return false;
	if(player->isSpectator())
		return true;

	return (*getPlayerTeam(*player)->fogOfWarMap)[pos.z][pos.x][pos.y];
}

bool CGameState::isVisible(const CGObjectInstance * obj, const std::optional<PlayerColor> & player) const
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

EVictoryLossCheckResult CGameState::checkForVictoryAndLoss(const PlayerColor & player) const
{
	const std::string & messageWonSelf = VLC->generaltexth->allTexts[659];
	const std::string & messageWonOther = VLC->generaltexth->allTexts[5];
	const std::string & messageLostSelf = VLC->generaltexth->allTexts[7];
	const std::string & messageLostOther = VLC->generaltexth->allTexts[8];

	auto evaluateEvent = [=](const EventCondition & condition)
	{
		return this->checkForVictory(player, condition);
	};

	const PlayerState *p = CGameInfoCallback::getPlayerState(player);

	//cheater or tester, but has entered the code...
	if (p->enteredWinningCheatCode)
		return EVictoryLossCheckResult::victory(messageWonSelf, messageWonOther);

	if (p->enteredLosingCheatCode)
		return EVictoryLossCheckResult::defeat(messageLostSelf, messageLostOther);

	for (const TriggeredEvent & event : map->triggeredEvents)
	{
		if (event.trigger.test(evaluateEvent))
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

bool CGameState::checkForVictory(const PlayerColor & player, const EventCondition & condition) const
{
	const PlayerState *p = CGameInfoCallback::getPlayerState(player);
	switch (condition.condition)
	{
		case EventCondition::STANDARD_WIN:
		{
			return player == checkForStandardWin();
		}
		case EventCondition::HAVE_ARTIFACT: //check if any hero has winning artifact
		{
			for(const auto & elem : p->heroes)
				if(elem->hasArt(ArtifactID(condition.objectType)))
					return true;
			return false;
		}
		case EventCondition::HAVE_CREATURES:
		{
			//check if in players armies there is enough creatures
			int total = 0; //creature counter
			for(auto object : map->objects)
			{
				const CArmedInstance *ai = nullptr;
				if(object
					&& object->tempOwner == player //object controlled by player
					&& (ai = dynamic_cast<const CArmedInstance *>(object.get()))) //contains army
				{
					for(const auto & elem : ai->Slots()) //iterate through army
						if(elem.second->type->getId() == condition.objectType) //it's searched creature
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
				const auto * t = dynamic_cast<const CGTownInstance *>(condition.object);
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
				if(const auto * hero = dynamic_cast<const CGHeroInstance *>(condition.object))
					return boost::range::find(gs->map->heroesOnMap, hero) == gs->map->heroesOnMap.end();
				else
					return getObj(condition.object->id) == nullptr;
			}
			else
			{
				for(const auto & elem : map->objects) // mode B - destroy all objects of this type
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
			const auto & team = CGameInfoCallback::getPlayerTeam(player)->players;

			if (condition.object) // mode A - flag one specific object, like town
			{
				return team.count(condition.object->tempOwner) != 0;
			}
			else
			{
				for(const auto & elem : map->objects) // mode B - flag all objects of this type
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
			const auto * t = dynamic_cast<const CGTownInstance *>(condition.object);
			return (t->visitingHero && t->visitingHero->hasArt(ArtifactID(condition.objectType))) ||
				   (t->garrisonHero && t->garrisonHero->hasArt(ArtifactID(condition.objectType)));
		}
		case EventCondition::DAYS_PASSED:
		{
			return (si32)gs->day > condition.value;
		}
		case EventCondition::IS_HUMAN:
		{
			return p->human ? condition.value == 1 : condition.value == 0;
		}
		case EventCondition::DAYS_WITHOUT_TOWN:
		{
			if (p->daysWithoutCastle)
				return p->daysWithoutCastle >= condition.value;
			else
				return false;
		}
		case EventCondition::CONST_VALUE:
		{
			return condition.value; // just convert to bool
		}
		case EventCondition::HAVE_0:
		{
			logGlobal->debug("Not implemented event condition type: %d", (int)condition.condition);
			//TODO: support new condition format
			return false;
		}
		case EventCondition::HAVE_BUILDING_0:
		{
			logGlobal->debug("Not implemented event condition type: %d", (int)condition.condition);
			//TODO: support new condition format
			return false;
		}
		case EventCondition::DESTROY_0:
		{
			logGlobal->debug("Not implemented event condition type: %d", (int)condition.condition);
			//TODO: support new condition format
			return false;
		}
		default:
			logGlobal->error("Invalid event condition type: %d", (int)condition.condition);
			return false;
	}
}

PlayerColor CGameState::checkForStandardWin() const
{
	//std victory condition is:
	//all enemies lost
	PlayerColor supposedWinner = PlayerColor::NEUTRAL;
	TeamID winnerTeam = TeamID::NO_TEAM;
	for(const auto & elem : players)
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

bool CGameState::checkForStandardLoss(const PlayerColor & player) const
{
	//std loss condition is: player lost all towns and heroes
	const PlayerState & pState = *CGameInfoCallback::getPlayerState(player);
	return pState.checkVanquished();
}

struct statsHLP
{
	using TStat = std::pair<PlayerColor, si64>;
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

	static const CGHeroInstance * findBestHero(CGameState * gs, const PlayerColor & color)
	{
		std::vector<ConstTransitivePtr<CGHeroInstance> > &h = gs->players[color].heroes;
		if(h.empty())
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
			ret += (int)h->artifactsInBackpack.size() + (int)h->artifactsWorn.size();
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

	// get total gold income
	static int getIncome(const PlayerState * ps)
	{
		int totalIncome = 0;
		const CGObjectInstance * heroOrTown = nullptr;

		//Heroes can produce gold as well - skill, specialty or arts
		for(const auto & h : ps->heroes)
		{
			totalIncome += h->valOfBonuses(Selector::typeSubtype(BonusType::GENERATE_RESOURCE, GameResID(EGameResID::GOLD)));

			if(!heroOrTown)
				heroOrTown = h;
		}

		//Add town income of all towns
		for(const auto & t : ps->towns)
		{
			totalIncome += t->dailyIncome()[EGameResID::GOLD];

			if(!heroOrTown)
				heroOrTown = t;
		}

		/// FIXME: Dirty dirty hack
		/// Stats helper need some access to gamestate.
		std::vector<const CGObjectInstance *> ownedObjects;
		for(const CGObjectInstance * obj : heroOrTown->cb->gameState()->map->objects)
		{
			if(obj && obj->tempOwner == ps->color)
				ownedObjects.push_back(obj);
		}
		/// This is code from CPlayerSpecificInfoCallback::getMyObjects
		/// I'm really need to find out about callback interface design...

		for(const auto * object : ownedObjects)
		{
			//Mines
			if ( object->ID == Obj::MINE )
			{
				const auto * mine = dynamic_cast<const CGMine *>(object);
				assert(mine);

				if (mine->producedResource == EGameResID::GOLD)
					totalIncome += mine->producedQuantity;
			}
		}

		return totalIncome;
	}
};

void CGameState::obtainPlayersStats(SThievesGuildInfo & tgi, int level)
{
	auto playerInactive = [&](const PlayerColor & color) 
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

	if(level >= 0) //num of towns & num of heroes
	{
		//num of towns
		FILL_FIELD(numOfTowns, g->second.towns.size())
		//num of heroes
		FILL_FIELD(numOfHeroes, g->second.heroes.size())
	}
	if(level >= 1) //best hero's portrait
	{
		for(const auto & player : players)
		{
			if(playerInactive(player.second.color))
				continue;
			const CGHeroInstance * best = statsHLP::findBestHero(this, player.second.color);
			InfoAboutHero iah;
			iah.initFromHero(best, (level >= 2) ? InfoAboutHero::EInfoLevel::DETAILED : InfoAboutHero::EInfoLevel::BASIC);
			iah.army.clear();
			tgi.colorToBestHero[player.second.color] = iah;
		}
	}
	if(level >= 2) //gold
	{
		FILL_FIELD(gold, g->second.resources[EGameResID::GOLD])
	}
	if(level >= 2) //wood & ore
	{
		FILL_FIELD(woodOre, g->second.resources[EGameResID::WOOD] + g->second.resources[EGameResID::ORE])
	}
	if(level >= 3) //mercury, sulfur, crystal, gems
	{
		FILL_FIELD(mercSulfCrystGems, g->second.resources[EGameResID::MERCURY] + g->second.resources[EGameResID::SULFUR] + g->second.resources[EGameResID::CRYSTAL] + g->second.resources[EGameResID::GEMS])
	}
	if(level >= 3) //obelisks found
	{
		auto getObeliskVisited = [](const TeamID & t)
		{
			if(CGObelisk::visited.count(t))
				return CGObelisk::visited[t];
			else
				return ui8(0);
		};

		FILL_FIELD(obelisks, getObeliskVisited(gs->getPlayerTeam(g->second.color)->id))
	}
	if(level >= 4) //artifacts
	{
		FILL_FIELD(artifacts, statsHLP::getNumberOfArts(&g->second))
	}
	if(level >= 4) //army strength
	{
		FILL_FIELD(army, statsHLP::getArmyStrength(&g->second))
	}
	if(level >= 5) //income
	{
		FILL_FIELD(income, statsHLP::getIncome(&g->second))
	}
	if(level >= 2) //best hero's stats
	{
		//already set in  lvl 1 handling
	}
	if(level >= 3) //personality
	{
		for(const auto & player : players)
		{
			if(playerInactive(player.second.color)) //do nothing for neutral player
				continue;
			if(player.second.human)
			{
				tgi.personality[player.second.color] = EAiTactic::NONE;
			}
			else //AI
			{
				tgi.personality[player.second.color] = map->players[player.second.color.getNum()].aiTactic;
			}

		}
	}
	if(level >= 4) //best creature
	{
		//best creatures belonging to player (highest AI value)
		for(const auto & player : players)
		{
			if(playerInactive(player.second.color)) //do nothing for neutral player
				continue;
			int bestCre = -1; //best creature's ID
			for(const auto & elem : player.second.heroes)
			{
				for(const auto & it : elem->Slots())
				{
					int toCmp = it.second->type->getId(); //ID of creature we should compare with the best one
					if(bestCre == -1 || VLC->creh->objects[bestCre]->getAIValue() < VLC->creh->objects[toCmp]->getAIValue())
					{
						bestCre = toCmp;
					}
				}
			}
			tgi.bestCreature[player.second.color] = bestCre;
		}
	}

#undef FILL_FIELD
}

std::map<ui32, ConstTransitivePtr<CGHeroInstance> > CGameState::unusedHeroesFromPool()
{
	std::map<ui32, ConstTransitivePtr<CGHeroInstance> > pool = hpool.heroesPool;
	for(const auto & player : players)
		for(auto availableHero : player.second.availableHeroes)
			if(availableHero)
				pool.erase((*availableHero).subID);

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
	for(auto & team : teams)
	{
		TeamState * t = &team.second;
		t->attachTo(globalEffects);

		for(const PlayerColor & teamMember : team.second.players)
		{
			PlayerState *p = getPlayerState(teamMember);
			assert(p);
			p->attachTo(*t);
		}
	}
}

void CGameState::attachArmedObjects()
{
	for(CGObjectInstance *obj : map->objects)
	{
		if(auto * armed = dynamic_cast<CArmedInstance *>(obj))
		{
			armed->whatShouldBeAttached().attachTo(armed->whereShouldBeAttached(this));
		}
	}
}

bool CGameState::giveHeroArtifact(CGHeroInstance * h, const ArtifactID & aid)
{
	 CArtifact * const artifact = VLC->arth->objects[aid]; //pointer to constant object
	 CArtifactInstance * ai = CArtifactInstance::createNewArtifactInstance(artifact);
	 map->addNewArtifactInstance(ai);
	 auto slot = ArtifactUtils::getArtAnyPosition(h, aid);
	 if(ArtifactUtils::isSlotEquipment(slot) || ArtifactUtils::isSlotBackpack(slot))
	 {
		 ai->putAt(ArtifactLocation(h, slot));
		 return true;
	 }
	 else
	 {
		 return false;
	 }
}

std::set<HeroTypeID> CGameState::getUnusedAllowedHeroes(bool alsoIncludeNotAllowed) const
{
	std::set<HeroTypeID> ret;
	for(int i = 0; i < map->allowedHeroes.size(); i++)
		if(map->allowedHeroes[i] || alsoIncludeNotAllowed)
			ret.insert(HeroTypeID(i));

	for(const auto & playerSettingPair : scenarioOps->playerInfos) //remove uninitialized yet heroes picked for start by other players
	{
		if(playerSettingPair.second.hero != PlayerSettings::RANDOM)
			ret -= HeroTypeID(playerSettingPair.second.hero);
	}

	for(auto hero : map->heroesOnMap)  //heroes instances initialization
	{
		if(hero->type)
			ret -= hero->type->getId();
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
			auto * heroPlaceholder = dynamic_cast<CGHeroPlaceholder *>(obj.get());
			if(heroPlaceholder->subID != 0xFF) //select by type
			{
				auto it = range::find_if(crossoverHeroes.heroesFromAnyPreviousScenarios, [heroPlaceholder](CGHeroInstance * hero)
				{
					return hero->subID == heroPlaceholder->subID;
				});

				if(it != crossoverHeroes.heroesFromAnyPreviousScenarios.end())
				{
					auto * hero = *it;
					crossoverHeroes.removeHeroFromBothLists(hero);
					campaignHeroReplacements.emplace_back(CMemorySerializer::deepCopy(*hero).release(), heroPlaceholder->id);
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
			auto * heroPlaceholder = dynamic_cast<CGHeroPlaceholder *>(obj.get());
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
		auto * heroPlaceholder = heroPlaceholders[i];
		if(crossoverHeroes.heroesFromPreviousScenario.size() > i)
		{
			auto * hero = crossoverHeroes.heroesFromPreviousScenario[i];
			campaignHeroReplacements.emplace_back(CMemorySerializer::deepCopy(*hero).release(), heroPlaceholder->id);
		}
	}

	return campaignHeroReplacements;
}

void CGameState::replaceHeroesPlaceholders(const std::vector<CGameState::CampaignHeroReplacement> & campaignHeroReplacements)
{
	for(const auto & campaignHeroReplacement : campaignHeroReplacements)
	{
		auto * heroPlaceholder = dynamic_cast<CGHeroPlaceholder *>(getObjInstance(campaignHeroReplacement.heroPlaceholderId));

		CGHeroInstance *heroToPlace = campaignHeroReplacement.hero;
		heroToPlace->id = campaignHeroReplacement.heroPlaceholderId;
		heroToPlace->tempOwner = heroPlaceholder->tempOwner;
		heroToPlace->pos = heroPlaceholder->pos;
		heroToPlace->type = VLC->heroh->objects[heroToPlace->subID];
		heroToPlace->appearance = VLC->objtypeh->getHandlerFor(Obj::HERO,
															   heroToPlace->type->heroClass->getIndex())->getTemplates().front();

		for(auto &&i : heroToPlace->stacks)
			i.second->type = VLC->creh->objects[i.second->getCreatureID()];

		auto fixArtifact = [&](CArtifactInstance * art)
		{
			art->artType = VLC->arth->objects[art->artType->getId()];
			gs->map->artInstances.emplace_back(art);
			art->id = ArtifactInstanceID((si32)gs->map->artInstances.size() - 1);
		};

		for(auto &&i : heroToPlace->artifactsWorn)
			fixArtifact(i.second.artifact);
		for(auto &&i : heroToPlace->artifactsInBackpack)
			fixArtifact(i.artifact);

		map->removeBlockVisTiles(heroPlaceholder, true);
		map->objects[heroPlaceholder->id.getNum()] = nullptr;
		map->instanceNames.erase(heroPlaceholder->instanceName);

		map->heroesOnMap.emplace_back(heroToPlace);
		map->objects[heroToPlace->id.getNum()] = heroToPlace;
		map->addBlockVisTiles(heroToPlace);
		map->instanceNames[heroToPlace->instanceName] = heroToPlace;

		delete heroPlaceholder;

		scenarioOps->campState->getCurrentScenario().placedCrossoverHeroes.push_back(CCampaignState::crossoverSerialize(heroToPlace));
	}
}

bool CGameState::isUsedHero(const HeroTypeID & hid) const
{
	return getUsedHero(hid);
}

CGHeroInstance * CGameState::getUsedHero(const HeroTypeID & hid) const
{
	for(auto hero : map->heroesOnMap)  //heroes instances initialization
	{
		if(hero->type && hero->type->getId() == hid)
		{
			return hero;
		}
	}

	for(auto obj : map->objects) //prisons
	{
		if(obj && obj->ID == Obj::PRISON )
		{
			auto * hero = dynamic_cast<CGHeroInstance *>(obj.get());
			assert(hero);
			if ( hero->type && hero->type->getId() == hid )
				return hero;
		}
	}

	return nullptr;
}

bool RumorState::update(int id, int extra)
{
	if(vstd::contains(last, type))
	{
		if(last[type].first != id)
		{
			last[type].first = id;
			last[type].second = extra;
		}
		else
			return false;
	}
	else
		last[type] = std::make_pair(id, extra);

	return true;
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
	vstd::clear_pointer(details);
	InfoAboutArmy::operator = (iah);

	details = (iah.details ? new Details(*iah.details) : nullptr);
	hclass = iah.hclass;
	portrait = iah.portrait;
}

InfoAboutHero::InfoAboutHero(): portrait(-1) {}

InfoAboutHero::InfoAboutHero(const InfoAboutHero & iah): InfoAboutArmy(iah)
{
	assign(iah);
}

InfoAboutHero::InfoAboutHero(const CGHeroInstance * h, InfoAboutHero::EInfoLevel infoLevel):
	portrait(-1)
{
	initFromHero(h, infoLevel);
}

InfoAboutHero::~InfoAboutHero()
{
	vstd::clear_pointer(details);
}

InfoAboutHero & InfoAboutHero::operator=(const InfoAboutHero & iah)
{
	assign(iah);
	return *this;
}

void InfoAboutHero::initFromHero(const CGHeroInstance *h, InfoAboutHero::EInfoLevel infoLevel)
{
	vstd::clear_pointer(details);
	if(!h)
		return;

	bool detailed = ( (infoLevel == EInfoLevel::DETAILED) || (infoLevel == EInfoLevel::INBATTLE) );

	initFromArmy(h, detailed);

	hclass = h->type->heroClass;
	name = h->getNameTranslated();
	portrait = h->portrait;

	if(detailed)
	{
		//include details about hero
		details = new Details();
		details->luck = h->luckVal();
		details->morale = h->moraleVal();
		details->mana = h->mana;
		details->primskills.resize(GameConstants::PRIMARY_SKILLS);

		for (int i = 0; i < GameConstants::PRIMARY_SKILLS ; i++)
		{
			details->primskills[i] = h->getPrimSkillLevel(static_cast<PrimarySkill::PrimarySkill>(i));
		}
		if (infoLevel == EInfoLevel::INBATTLE)
			details->manaLimit = h->manaLimit();
		else
			details->manaLimit = -1; //we do not want to leak max mana info outside battle so set to meaningless value
	}
}

InfoAboutTown::InfoAboutTown():
	details(nullptr),
	tType(nullptr),
	built(0),
	fortLevel(0)
{

}

InfoAboutTown::InfoAboutTown(const CGTownInstance *t, bool detailed):
	details(nullptr),
	tType(nullptr),
	built(0),
	fortLevel(0)
{
	initFromTown(t, detailed);
}

InfoAboutTown::~InfoAboutTown()
{
	vstd::clear_pointer(details);
}

void InfoAboutTown::initFromTown(const CGTownInstance *t, bool detailed)
{
	initFromArmy(t, detailed);
	army = ArmyDescriptor(t->getUpperArmy(), detailed);
	built = t->builded;
	fortLevel = t->fortLevel();
	name = t->getNameTranslated();
	tType = t->getTown();

	vstd::clear_pointer(details);

	if(detailed)
	{
		//include details about hero
		details = new Details();
		TResources income = t->dailyIncome();
		details->goldIncome = income[EGameResID::GOLD];
		details->customRes = t->hasBuilt(BuildingID::RESOURCE_SILO);
		details->hallLevel = t->hallLevel();
		details->garrisonedHero = t->garrisonHero;
	}
}

ArmyDescriptor::ArmyDescriptor(const CArmedInstance *army, bool detailed)
	: isDetailed(detailed)
{
	for(const auto & elem : army->Slots())
	{
		if(detailed)
			(*this)[elem.first] = *elem.second;
		else
			(*this)[elem.first] = CStackBasicDescriptor(elem.second->type, (int)elem.second->getQuantityID());
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
		for(const auto & elem : *this)
			ret += elem.second.type->getAIValue() * elem.second.count;
	}
	else
	{
		for(const auto & elem : *this)
			ret += elem.second.type->getAIValue() * CCreature::estimateCreatureCount(elem.second.count);
	}
	return static_cast<int>(ret);
}

TeamState::TeamState()
{
	setNodeType(TEAM);
	fogOfWarMap = std::make_shared<boost::multi_array<ui8, 3>>();
}

TeamState::TeamState(TeamState && other) noexcept:
	CBonusSystemNode(std::move(other)),
	id(other.id)
{
	std::swap(players, other.players);
	std::swap(fogOfWarMap, other.fogOfWarMap);
}

CRandomGenerator & CGameState::getRandomGenerator()
{
	return rand;
}

VCMI_LIB_NAMESPACE_END
