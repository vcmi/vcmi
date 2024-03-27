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

#include "EVictoryLossCheckResult.h"
#include "InfoAboutArmy.h"
#include "TavernHeroesPool.h"
#include "CGameStateCampaign.h"
#include "SThievesGuildInfo.h"

#include "../ArtifactUtils.h"
#include "../CBuildingHandler.h"
#include "../CGeneralTextHandler.h"
#include "../CHeroHandler.h"
#include "../CPlayerState.h"
#include "../CStopWatch.h"
#include "../GameSettings.h"
#include "../StartInfo.h"
#include "../TerrainHandler.h"
#include "../VCMIDirs.h"
#include "../VCMI_Lib.h"
#include "../battle/BattleInfo.h"
#include "../campaign/CampaignState.h"
#include "../constants/StringConstants.h"
#include "../filesystem/ResourcePath.h"
#include "../json/JsonBonus.h"
#include "../json/JsonUtils.h"
#include "../mapObjectConstructors/AObjectTypeHandler.h"
#include "../mapObjectConstructors/CObjectClassesHandler.h"
#include "../mapObjectConstructors/DwellingInstanceConstructor.h"
#include "../mapObjects/CGHeroInstance.h"
#include "../mapObjects/CGTownInstance.h"
#include "../mapObjects/CQuest.h"
#include "../mapObjects/MiscObjects.h"
#include "../mapping/CMap.h"
#include "../mapping/CMapEditManager.h"
#include "../mapping/CMapService.h"
#include "../modding/IdentifierStorage.h"
#include "../pathfinder/CPathfinder.h"
#include "../pathfinder/PathfinderOptions.h"
#include "../registerTypes/RegisterTypesClientPacks.h"
#include "../rmg/CMapGenerator.h"
#include "../serializer/CMemorySerializer.h"
#include "../serializer/CTypeList.h"
#include "../spells/CSpellHandler.h"

VCMI_LIB_NAMESPACE_BEGIN

boost::shared_mutex CGameState::mutex;

template <typename T> class CApplyOnGS;

class CBaseForGSApply
{
public:
	virtual void applyOnGS(CGameState *gs, CPack * pack) const =0;
	virtual ~CBaseForGSApply() = default;
	template<typename U> static CBaseForGSApply *getApplier(const U * t=nullptr)
	{
		return new CApplyOnGS<U>();
	}
};

template <typename T> class CApplyOnGS : public CBaseForGSApply
{
public:
	void applyOnGS(CGameState *gs, CPack * pack) const override
	{
		T *ptr = static_cast<T*>(pack);

		boost::unique_lock<boost::shared_mutex> lock(CGameState::mutex);
		ptr->applyGs(gs);
	}
};

HeroTypeID CGameState::pickNextHeroType(const PlayerColor & owner)
{
	const PlayerSettings &ps = scenarioOps->getIthPlayersSettings(owner);
	if(ps.hero >= HeroTypeID(0) && !isUsedHero(HeroTypeID(ps.hero))) //we haven't used selected hero
	{
		return HeroTypeID(ps.hero);
	}

	return pickUnusedHeroTypeRandomly(owner);
}

HeroTypeID CGameState::pickUnusedHeroTypeRandomly(const PlayerColor & owner)
{
	//list of available heroes for this faction and others
	std::vector<HeroTypeID> factionHeroes;
	std::vector<HeroTypeID> otherHeroes;

	const PlayerSettings &ps = scenarioOps->getIthPlayersSettings(owner);
	for(const HeroTypeID & hid : getUnusedAllowedHeroes())
	{
		if(hid.toHeroType()->heroClass->faction == ps.castle)
			factionHeroes.push_back(hid);
		else
			otherHeroes.push_back(hid);
	}

	// select random hero native to "our" faction
	if(!factionHeroes.empty())
	{
		return *RandomGeneratorUtil::nextItem(factionHeroes, getRandomGenerator());
	}

	logGlobal->warn("Cannot find free hero of appropriate faction for player %s - trying to get first available...", owner.toString());
	if(!otherHeroes.empty())
	{
		return *RandomGeneratorUtil::nextItem(otherHeroes, getRandomGenerator());
	}

	logGlobal->error("No free allowed heroes!");
	auto notAllowedHeroesButStillBetterThanCrash = getUnusedAllowedHeroes(true);
	if(!notAllowedHeroesButStillBetterThanCrash.empty())
		return *notAllowedHeroesButStillBetterThanCrash.begin();

	logGlobal->error("No free heroes at all!");
	throw std::runtime_error("Can not allocate hero. All heroes are already used.");
}

int CGameState::getDate(Date mode) const
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
	heroesPool = std::make_unique<TavernHeroesPool>();
	applier = std::make_shared<CApplier<CBaseForGSApply>>();
	registerTypesClientPacks(*applier);
	globalEffects.setNodeType(CBonusSystemNode::GLOBAL_EFFECTS);
}

CGameState::~CGameState()
{
	// explicitly delete all ongoing battles first - BattleInfo destructor requires valid CGameState
	currentBattles.clear();
	map.dellNull();
	scenarioOps.dellNull();
	initialOpts.dellNull();
}

void CGameState::preInit(Services * newServices, IGameCallback * newCallback)
{
	services = newServices;
	callback = newCallback;
}

void CGameState::init(const IMapService * mapService, StartInfo * si, Load::ProgressAccumulator & progressTracking, bool allowSavingRandomMap)
{
	assert(services);
	assert(callback);
	logGlobal->info("\tUsing random seed: %d", si->seedToBeUsed);
	getRandomGenerator().setSeed(si->seedToBeUsed);
	scenarioOps = CMemorySerializer::deepCopy(*si).release();
	initialOpts = CMemorySerializer::deepCopy(*si).release();
	si = nullptr;

	switch(scenarioOps->mode)
	{
	case EStartMode::NEW_GAME:
		initNewGame(mapService, allowSavingRandomMap, progressTracking);
		break;
	case EStartMode::CAMPAIGN:
		initCampaign();
		break;
	default:
		logGlobal->error("Wrong mode: %d", static_cast<int>(scenarioOps->mode));
		return;
	}
	logGlobal->info("Map loaded!");

	checkMapChecksum();

	day = 0;

	logGlobal->debug("Initialization:");

	initGlobalBonuses();
	initPlayerStates();
	if (campaign)
		campaign->placeCampaignHeroes();
	removeHeroPlaceholders();
	initGrailPosition();
	initRandomFactionsForPlayers();
	randomizeMapObjects();
	placeStartingHeroes();
	initDifficulty();
	initHeroes();
	initStartingBonus();
	initTowns();
	initTownNames();
	placeHeroesInTowns();
	initMapObjects();
	buildBonusSystemTree();
	initVisitingAndGarrisonedHeroes();
	initFogOfWar();

	for(auto & elem : teams)
	{
		map->obelisksVisited[elem.first] = 0;
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
	assert(services);
	assert(callback);
	scenarioOps->playerInfos = si->playerInfos;
	for(auto & i : si->playerInfos)
		gs->players[i.first].human = i.second.isControlledByHuman();
}

void CGameState::initNewGame(const IMapService * mapService, bool allowSavingRandomMap, Load::ProgressAccumulator & progressTracking)
{
	if(scenarioOps->createRandomMap())
	{
		logGlobal->info("Create random map.");
		CStopWatch sw;

		// Gen map
		CMapGenerator mapGenerator(*scenarioOps->mapGenOptions, callback, scenarioOps->seedToBeUsed);
		progressTracking.include(mapGenerator);

		std::unique_ptr<CMap> randomMap = mapGenerator.generate();
		progressTracking.exclude(mapGenerator);

		if(allowSavingRandomMap)
		{
			try
			{
				auto path = VCMIDirs::get().userDataPath() / "Maps" / "RandomMaps";
				boost::filesystem::create_directories(path);

				std::shared_ptr<CMapGenOptions> options = scenarioOps->mapGenOptions;

				const std::string templateName = options->getMapTemplate()->getName();
				const ui32 seed = scenarioOps->seedToBeUsed;
				const std::string dt = vstd::getDateTimeISO8601Basic(std::time(nullptr));

				const std::string fileName = boost::str(boost::format("%s_%s_%d.vmap") % dt % templateName % seed );
				const auto fullPath = path / fileName;

				randomMap->name.appendRawString(boost::str(boost::format(" %s") % dt));

				mapService->saveMap(randomMap, fullPath);

				logGlobal->info("Random map has been saved to:");
				logGlobal->info(fullPath.string());
			}
			catch(...)
			{
				logGlobal->error("Saving random map failed with exception");
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
		const ResourcePath mapURI(scenarioOps->mapname, EResType::MAP);
		map = mapService->loadMap(mapURI, callback).release();
	}
}

void CGameState::initCampaign()
{
	campaign = std::make_unique<CGameStateCampaign>(this);
	map = campaign->getCurrentMap().release();
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
		bonus->sid = BonusSourceID(); //there is one global object
		globalEffects.addNewBonus(bonus);
	}
	VLC->creh->loadCrExpBon(globalEffects);
}

void CGameState::initDifficulty()
{
	logGlobal->debug("\tLoading difficulty settings");
	const JsonNode config = JsonUtils::assembleFromFiles("config/difficulty.json");
	
	const JsonNode & difficultyAI(config["ai"][GameConstants::DIFFICULTY_NAMES[scenarioOps->difficulty]]);
	const JsonNode & difficultyHuman(config["human"][GameConstants::DIFFICULTY_NAMES[scenarioOps->difficulty]]);
	
	auto setDifficulty = [](PlayerState & state, const JsonNode & json)
	{
		//set starting resources
		state.resources = TResources(json["resources"]);
		
		//set global bonuses
		for(auto & jsonBonus : json["globalBonuses"].Vector())
			if(auto bonus = JsonUtils::parseBonus(jsonBonus))
				state.addNewBonus(bonus);
		
		//set battle bonuses
		for(auto & jsonBonus : json["battleBonuses"].Vector())
			if(auto bonus = JsonUtils::parseBonus(jsonBonus))
				state.battleBonuses.push_back(*bonus);
		
	};

	for (auto & elem : players)
	{
		PlayerState &p = elem.second;
		setDifficulty(p, p.human ? difficultyHuman : difficultyAI);
	}

	if (campaign)
		campaign->initStartingResources();
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
		if(elem.second.castle==FactionID::RANDOM)
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
	for(CGObjectInstance *object : map->objects)
	{
		if(!object)
			continue;

		object->pickRandomObject(getRandomGenerator());

		//handle Favouring Winds - mark tiles under it
		if(object->ID == Obj::FAVORABLE_WINDS)
		{
			for (int i = 0; i < object->getWidth() ; i++)
			{
				for (int j = 0; j < object->getHeight() ; j++)
				{
					int3 pos = object->pos - int3(i,j,0);
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

	auto handler = VLC->objtypeh->getHandlerFor(Obj::HERO, heroTypeId.toHeroType()->heroClass->getIndex());
	CGObjectInstance * obj = handler->create(callback, handler->getTemplates().front());
	CGHeroInstance * hero = dynamic_cast<CGHeroInstance *>(obj);

	hero->ID = Obj::HERO;
	hero->setHeroType(heroTypeId);
	hero->tempOwner = playerColor;

	hero->pos = townPos;
	hero->pos += hero->getVisitableOffset();
	map->getEditManager()->insertObject(hero);
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
			if (campaign && campaign->playerHasStartingHero(playerColor))
				continue;

			HeroTypeID heroTypeId = pickNextHeroType(playerColor);
			if(playerSettingPair.second.hero == HeroTypeID::NONE)
				playerSettingPair.second.hero = heroTypeId;

			placeStartingHero(playerColor, HeroTypeID(heroTypeId), playerInfo.posOfMainTown);
		}
	}
}

void CGameState::removeHeroPlaceholders()
{
	// remove any hero placeholders that remain on map after (potential) campaign heroes placement
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
		map->allHeroes[hero->getHeroType().getNum()] = hero;
	}

	// generate boats for all heroes on water
	for(auto hero : map->heroesOnMap)
	{
		assert(map->isInTheMap(hero->visitablePos()));
		const auto & tile = map->getTile(hero->visitablePos());
		if (tile.terType->isWater())
		{
			auto handler = VLC->objtypeh->getHandlerFor(Obj::BOAT, hero->getBoatType().getNum());
			auto boat = dynamic_cast<CGBoat*>(handler->create(callback, nullptr));
			handler->configureObject(boat, gs->getRandomGenerator());

			boat->pos = hero->pos;
			boat->appearance = handler->getTemplates().front();
			boat->id = ObjectInstanceID(static_cast<si32>(gs->map->objects.size()));

			map->objects.emplace_back(boat);
			map->addBlockVisTiles(boat);

			hero->attachToBoat(boat);
		}
	}

	for(auto obj : map->objects) //prisons
	{
		if(obj && obj->ID == Obj::PRISON)
		{
			auto * hero = dynamic_cast<CGHeroInstance*>(obj.get());
			hero->initHero(getRandomGenerator());
			map->allHeroes[hero->getHeroType().getNum()] = hero;
		}
	}

	std::set<HeroTypeID> heroesToCreate = getUnusedAllowedHeroes(); //ids of heroes to be created and put into the pool
	for(auto ph : map->predefinedHeroes)
	{
		if(!vstd::contains(heroesToCreate, ph->getHeroType()))
			continue;
		ph->initHero(getRandomGenerator());
		heroesPool->addHeroToPool(ph);
		heroesToCreate.erase(ph->type->getId());

		map->allHeroes[ph->getHeroType().getNum()] = ph;
	}

	for(const HeroTypeID & htype : heroesToCreate) //all not used allowed heroes go with default state into the pool
	{
		auto * vhi = new CGHeroInstance(callback);
		vhi->initHero(getRandomGenerator(), htype);

		int typeID = htype.getNum();
		map->allHeroes[typeID] = vhi;
		heroesPool->addHeroToPool(vhi);
	}

	for(auto & elem : map->disposedHeroes)
		heroesPool->setAvailability(elem.heroId, elem.players);

	if (campaign)
		campaign->initHeroes();
}

void CGameState::initFogOfWar()
{
	logGlobal->debug("\tFog of war"); //FIXME: should be initialized after all bonuses are set

	int layers = map->levels();
	for(auto & elem : teams)
	{
		auto & fow = elem.second.fogOfWarMap;
		fow->resize(boost::extents[layers][map->width][map->height]);
		std::fill(fow->data(), fow->data() + fow->num_elements(), 0);

		for(CGObjectInstance *obj : map->objects)
		{
			if(!obj || !vstd::contains(elem.second.players, obj->tempOwner)) continue; //not a flagged object

			std::unordered_set<int3> tiles;
			getTilesInRange(tiles, obj->getSightCenter(), obj->getSightRadius(), ETileVisibility::HIDDEN, obj->tempOwner);
			for(const int3 & tile : tiles)
			{
				(*elem.second.fogOfWarMap)[tile.z][tile.x][tile.y] = 1;
			}
		}
	}
}

void CGameState::initStartingBonus()
{
	if (scenarioOps->mode == EStartMode::CAMPAIGN)
		return;
	// These are the single scenario bonuses; predefined
	// campaign bonuses are spread out over other init* functions.

	logGlobal->debug("\tStarting bonuses");
	for(auto & elem : players)
	{
		//starting bonus
		if(scenarioOps->playerInfos[elem.first].bonus == PlayerStartingBonus::RANDOM)
			scenarioOps->playerInfos[elem.first].bonus = static_cast<PlayerStartingBonus>(getRandomGenerator().nextInt(2));

		switch(scenarioOps->playerInfos[elem.first].bonus)
		{
		case PlayerStartingBonus::GOLD:
			elem.second.resources[EGameResID::GOLD] += getRandomGenerator().nextInt(5, 10) * 100;
			break;
		case PlayerStartingBonus::RESOURCE:
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
		case PlayerStartingBonus::ARTIFACT:
			{
				if(elem.second.heroes.empty())
				{
					logGlobal->error("Cannot give starting artifact - no heroes!");
					break;
				}
				const Artifact * toGive = pickRandomArtifact(getRandomGenerator(), CArtifact::ART_TREASURE).toEntity(VLC);

				CGHeroInstance *hero = elem.second.heroes[0];
				if(!giveHeroArtifact(hero, toGive->getId()))
					logGlobal->error("Cannot give starting artifact - no free slots!");
			}
			break;
		}
	}
}

void CGameState::initTownNames()
{
	std::map<FactionID, std::vector<int>> availableNames;
	for(const auto & faction : VLC->townh->getDefaultAllowed())
	{
		std::vector<int> potentialNames;
		if(faction.toFaction()->town->getRandomNamesCount() > 0)
		{
			for(int i = 0; i < faction.toFaction()->town->getRandomNamesCount(); ++i)
				potentialNames.push_back(i);

			availableNames[faction] = potentialNames;
		}
	}

	for(auto & vti : map->towns)
	{
		assert(vti->town);

		if(!vti->getNameTextID().empty())
			continue;

		FactionID faction = vti->getFaction();

		if(availableNames.empty())
		{
			logGlobal->warn("Failed to find available name for a random town!");
			vti->setNameTextId("core.genrltxt.508"); // Unnamed
			continue;
		}

		// If town has no available names (for example - all were picked) - pick names from some other faction that still has names available
		if(!availableNames.count(faction))
			faction = RandomGeneratorUtil::nextItem(availableNames, getRandomGenerator())->first;

		auto nameIt = RandomGeneratorUtil::nextItem(availableNames[faction], getRandomGenerator());
		vti->setNameTextId(faction.toFaction()->town->getRandomNameTextID(*nameIt));

		availableNames[faction].erase(nameIt);
		if(availableNames[faction].empty())
			availableNames.erase(faction);
	}
}

void CGameState::initTowns()
{
	logGlobal->debug("\tTowns");

	if (campaign)
		campaign->initTowns();

	map->townUniversitySkills.clear();
	map->townUniversitySkills.push_back(SecondarySkill(SecondarySkill::FIRE_MAGIC));
	map->townUniversitySkills.push_back(SecondarySkill(SecondarySkill::AIR_MAGIC));
	map->townUniversitySkills.push_back(SecondarySkill(SecondarySkill::WATER_MAGIC));
	map->townUniversitySkills.push_back(SecondarySkill(SecondarySkill::EARTH_MAGIC));

	for (auto & vti : map->towns)
	{
		assert(vti->town);

		constexpr std::array basicDwellings = { BuildingID::DWELL_FIRST, BuildingID::DWELL_LVL_2, BuildingID::DWELL_LVL_3, BuildingID::DWELL_LVL_4, BuildingID::DWELL_LVL_5, BuildingID::DWELL_LVL_6, BuildingID::DWELL_LVL_7 };
		constexpr std::array upgradedDwellings = { BuildingID::DWELL_UP_FIRST, BuildingID::DWELL_LVL_2_UP, BuildingID::DWELL_LVL_3_UP, BuildingID::DWELL_LVL_4_UP, BuildingID::DWELL_LVL_5_UP, BuildingID::DWELL_LVL_6_UP, BuildingID::DWELL_LVL_7_UP };
		constexpr std::array hordes = { BuildingID::HORDE_PLACEHOLDER1, BuildingID::HORDE_PLACEHOLDER2, BuildingID::HORDE_PLACEHOLDER3, BuildingID::HORDE_PLACEHOLDER4, BuildingID::HORDE_PLACEHOLDER5, BuildingID::HORDE_PLACEHOLDER6, BuildingID::HORDE_PLACEHOLDER7 };

		//init buildings
		if(vstd::contains(vti->builtBuildings, BuildingID::DEFAULT)) //give standard set of buildings
		{
			vti->builtBuildings.erase(BuildingID::DEFAULT);
			vti->builtBuildings.insert(BuildingID::VILLAGE_HALL);
			if(vti->tempOwner != PlayerColor::NEUTRAL)
				vti->builtBuildings.insert(BuildingID::TAVERN);

			auto definesBuildingsChances = VLC->settings()->getVector(EGameSettings::TOWNS_STARTING_DWELLING_CHANCES);

			for(int i = 0; i < definesBuildingsChances.size(); i++)
			{
				if((getRandomGenerator().nextInt(1,100) <= definesBuildingsChances[i]))
				{
					vti->builtBuildings.insert(basicDwellings[i]);
				}
			}
		}

		// village hall must always exist
		vti->builtBuildings.insert(BuildingID::VILLAGE_HALL);

		//init hordes
		for (int i = 0; i < GameConstants::CREATURES_PER_TOWN; i++)
		{
			if (vstd::contains(vti->builtBuildings, hordes[i])) //if we have horde for this level
			{
				vti->builtBuildings.erase(hordes[i]);//remove old ID
				if (vti->getTown()->hordeLvl.at(0) == i)//if town first horde is this one
				{
					vti->builtBuildings.insert(BuildingID::HORDE_1);//add it
					//if we have upgraded dwelling as well
					if (vstd::contains(vti->builtBuildings, upgradedDwellings[i]))
						vti->builtBuildings.insert(BuildingID::HORDE_1_UPGR);//add it as well
				}
				if (vti->getTown()->hordeLvl.at(1) == i)//if town second horde is this one
				{
					vti->builtBuildings.insert(BuildingID::HORDE_2);
					if (vstd::contains(vti->builtBuildings, upgradedDwellings[i]))
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
				if (vstd::contains(ev.buildings,hordes[i])) //if we have horde for this level
				{
					ev.buildings.erase(hordes[i]);
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
			vti->spells[s->getLevel()-1].push_back(s->id);
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
			vti->spells[s->getLevel()-1].push_back(s->id);
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
			obj->initObj(getRandomGenerator());
	}
	logGlobal->debug("\tObject initialization done");
	for(CGObjectInstance *obj : map->objects)
	{
		if(!obj)
			continue;

		switch(obj->ID.toEnum())
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
	CGSubterraneanGate::postInit(callback); //pairing subterranean gates

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

const BattleInfo * CGameState::getBattle(const PlayerColor & player) const
{
	if (!player.isValidPlayer())
		return nullptr;

	for (const auto & battlePtr : currentBattles)
		if (battlePtr->sides[0].color == player || battlePtr->sides[1].color == player)
			return battlePtr.get();

	return nullptr;
}

const BattleInfo * CGameState::getBattle(const BattleID & battle) const
{
	for (const auto & battlePtr : currentBattles)
		if (battlePtr->battleID == battle)
			return battlePtr.get();

	return nullptr;
}

BattleInfo * CGameState::getBattle(const BattleID & battle)
{
	for (const auto & battlePtr : currentBattles)
		if (battlePtr->battleID == battle)
			return battlePtr.get();

	return nullptr;
}

BattleField CGameState::battleGetBattlefieldType(int3 tile, CRandomGenerator & rand)
{
	assert(tile.valid());

	if(!tile.valid())
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
		return BattleField(*VLC->identifiers()->getIdentifier("core", "battlefield.sand_shore"));
	
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

	if (stack.armyObj->ID == Obj::HERO)
	{
		auto hero = dynamic_cast<const CGHeroInstance *>(stack.armyObj);
		hero->fillUpgradeInfo(ret, stack);

		if (hero->visitedTown)
		{
			hero->visitedTown->fillUpgradeInfo(ret, stack);
		}
		else
		{
			auto object = vstd::frontOrNull(getVisitableObjs(hero->visitablePos()));
			auto upgradeSource = dynamic_cast<const ICreatureUpgrader*>(object);
			if (object != hero && upgradeSource != nullptr)
				upgradeSource->fillUpgradeInfo(ret, stack);
		}
	}

	if (stack.armyObj->ID == Obj::TOWN)
	{
		auto town = dynamic_cast<const CGTownInstance *>(stack.armyObj);
		town->fillUpgradeInfo(ret, stack);
	}

	if(!ret.newID.empty())
		ret.oldID = base->getId();

	for (ResourceSet &cost : ret.cost)
		cost.positive(); //upgrade cost can't be negative, ignore missing resources

	return ret;
}

PlayerRelations CGameState::getPlayerRelations( PlayerColor color1, PlayerColor color2 ) const
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
	ui16 typ = CTypeList::getInstance().getTypeID(pack);
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
			if(obj->isBlockedVisitable())
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
	static const std::vector<RumorState::ERumorType> rumorTypes = {RumorState::TYPE_MAP, RumorState::TYPE_SPECIAL, RumorState::TYPE_RAND, RumorState::TYPE_RAND};
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
	const MetaString messageWonSelf = MetaString::createFromTextID("core.genrltxt.659");
	const MetaString messageWonOther = MetaString::createFromTextID("core.genrltxt.5");
	const MetaString messageLostSelf = MetaString::createFromTextID("core.genrltxt.7");
	const MetaString messageLostOther = MetaString::createFromTextID("core.genrltxt.8");

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
				if(elem->hasArt(condition.objectType.as<ArtifactID>()))
					return true;
			return false;
		}
		case EventCondition::HAVE_CREATURES:
		{
			//check if in players armies there is enough creatures
			int total = 0; //creature counter
			for(auto object : map->objects)
			{
				const auto * ai = dynamic_cast<const CArmedInstance *>(object.get());
				if(ai && ai->getOwner() == player)
				{
					for(const auto & elem : ai->Slots()) //iterate through army
						if(elem.second->getId() == condition.objectType.as<CreatureID>()) //it's searched creature
							total += elem.second->count;
				}
			}
			return total >= condition.value;
		}
		case EventCondition::HAVE_RESOURCES:
		{
			return p->resources[condition.objectType.as<GameResID>()] >= condition.value;
		}
		case EventCondition::HAVE_BUILDING:
		{
			if (condition.objectID != ObjectInstanceID::NONE) // specific town
			{
				const auto * t = getTown(condition.objectID);
				return (t->tempOwner == player && t->hasBuilt(condition.objectType.as<BuildingID>()));
			}
			else // any town
			{
				for (const CGTownInstance * t : p->towns)
				{
					if (t->hasBuilt(condition.objectType.as<BuildingID>()))
						return true;
				}
				return false;
			}
		}
		case EventCondition::DESTROY:
		{
			if (condition.objectID != ObjectInstanceID::NONE) // mode A - destroy specific object of this type
			{
				return p->destroyedObjects.count(condition.objectID);
			}
			else
			{
				for(const auto & elem : map->objects) // mode B - destroy all objects of this type
				{
					if(elem && elem->ID == condition.objectType.as<MapObjectID>())
						return false;
				}
				return true;
			}
		}
		case EventCondition::CONTROL:
		{
			// list of players that need to control object to fulfull condition
			// NOTE: cgameinfocallback specified explicitly in order to get const version
			const auto * team = CGameInfoCallback::getPlayerTeam(player);

			if (condition.objectID != ObjectInstanceID::NONE) // mode A - flag one specific object, like town
			{
				const auto * object = getObjInstance(condition.objectID);

				if (!object)
					return false;
				return team->players.count(object->getOwner()) != 0;
			}
			else
			{
				for(const auto & elem : map->objects) // mode B - flag all objects of this type
				{
					 //check not flagged objs
					if ( elem && elem->ID == condition.objectType.as<MapObjectID>() && team->players.count(elem->getOwner()) == 0 )
						return false;
				}
				return true;
			}
		}
		case EventCondition::TRANSPORT:
		{
			const auto * t = getTown(condition.objectID);
			return (t->visitingHero && t->visitingHero->hasArt(condition.objectType.as<ArtifactID>())) ||
				   (t->garrisonHero && t->garrisonHero->hasArt(condition.objectType.as<ArtifactID>()));
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
		if(elem.second.status == EPlayerStatus::INGAME && elem.first.isValidPlayer())
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
			totalIncome += h->valOfBonuses(Selector::typeSubtype(BonusType::GENERATE_RESOURCE, BonusSubtypeID(GameResID(GameResID::GOLD))));

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
		auto getObeliskVisited = [&](const TeamID & t)
		{
			if(map->obelisksVisited.count(t))
				return map->obelisksVisited[t];
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
			CreatureID bestCre; //best creature's ID
			for(const auto & elem : player.second.heroes)
			{
				for(const auto & it : elem->Slots())
				{
					CreatureID toCmp = it.second->type->getId(); //ID of creature we should compare with the best one
					if(bestCre == CreatureID::NONE || bestCre.toEntity(VLC)->getAIValue() < toCmp.toEntity(VLC)->getAIValue())
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

void CGameState::buildBonusSystemTree()
{
	buildGlobalTeamPlayerTree();
	attachArmedObjects();

	for(CGTownInstance *t : map->towns)
	{
		t->deserializationFix();
	}
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
	 CArtifactInstance * ai = ArtifactUtils::createNewArtifactInstance(aid);
	 map->addNewArtifactInstance(ai);
	 auto slot = ArtifactUtils::getArtAnyPosition(h, aid);
	 if(ArtifactUtils::isSlotEquipment(slot) || ArtifactUtils::isSlotBackpack(slot))
	 {
		 ai->putAt(*h, slot);
		 return true;
	 }
	 else
	 {
		 return false;
	 }
}

std::set<HeroTypeID> CGameState::getUnusedAllowedHeroes(bool alsoIncludeNotAllowed) const
{
	std::set<HeroTypeID> ret = map->allowedHeroes;

	for(const auto & playerSettingPair : scenarioOps->playerInfos) //remove uninitialized yet heroes picked for start by other players
	{
		if(playerSettingPair.second.hero != HeroTypeID::RANDOM)
			ret -= HeroTypeID(playerSettingPair.second.hero);
	}

	for(auto hero : map->heroesOnMap)  //heroes instances initialization
	{
		if(hero->type)
			ret -= hero->type->getId();
		else
			ret -= hero->getHeroType();
	}

	for(auto obj : map->objects) //prisons
	{
		auto * hero = dynamic_cast<const CGHeroInstance *>(obj.get());
		if(hero && hero->ID == Obj::PRISON)
			ret -= hero->getHeroType();
	}

	return ret;
}

bool CGameState::isUsedHero(const HeroTypeID & hid) const
{
	return getUsedHero(hid);
}

CGHeroInstance * CGameState::getUsedHero(const HeroTypeID & hid) const
{
	for(auto obj : map->objects) //prisons
	{
		if (!obj)
			continue;

		if ( obj->ID !=Obj::PRISON && obj->ID != Obj::HERO)
			continue;

		auto * hero = dynamic_cast<CGHeroInstance *>(obj.get());
		assert(hero);

		if (hero->getHeroType() == hid)
			return hero;
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

TeamState::TeamState()
{
	setNodeType(TEAM);
	fogOfWarMap = std::make_unique<boost::multi_array<ui8, 3>>();
}

CRandomGenerator & CGameState::getRandomGenerator()
{
	return rand;
}

ArtifactID CGameState::pickRandomArtifact(CRandomGenerator & rand, int flags, std::function<bool(ArtifactID)> accepts)
{
	std::set<ArtifactID> potentialPicks;

	// Select artifacts that satisfy provided criterias
	for (auto const & artifactID : map->allowedArtifact)
	{
		if (!VLC->arth->legalArtifact(artifactID))
			continue;

		auto const * artifact = artifactID.toArtifact();

		assert(artifact->aClass != CArtifact::ART_SPECIAL); // should be filtered out when allowedArtifacts is initialized

		if ((flags & CArtifact::ART_TREASURE) == 0 && artifact->aClass == CArtifact::ART_TREASURE)
			continue;

		if ((flags & CArtifact::ART_MINOR) == 0 && artifact->aClass == CArtifact::ART_MINOR)
			continue;

		if ((flags & CArtifact::ART_MAJOR) == 0 && artifact->aClass == CArtifact::ART_MAJOR)
			continue;

		if ((flags & CArtifact::ART_RELIC) == 0 && artifact->aClass == CArtifact::ART_RELIC)
			continue;

		if (!accepts(artifact->getId()))
			continue;

		potentialPicks.insert(artifact->getId());
	}

	return pickRandomArtifact(rand, potentialPicks);
}

ArtifactID CGameState::pickRandomArtifact(CRandomGenerator & rand, std::set<ArtifactID> potentialPicks)
{
	// No allowed artifacts at all - give Grail - this can't be banned (hopefully)
	// FIXME: investigate how such cases are handled by H3 - some heavily customized user-made maps likely rely on H3 behavior
	if (potentialPicks.empty())
	{
		logGlobal->warn("Failed to find artifact that matches requested parameters!");
		return ArtifactID::GRAIL;
	}

	// Find how many times least used artifacts were picked by randomizer
	int leastUsedTimes = std::numeric_limits<int>::max();
	for (auto const & artifact : potentialPicks)
		if (allocatedArtifacts[artifact] < leastUsedTimes)
			leastUsedTimes = allocatedArtifacts[artifact];

	// Pick all artifacts that were used least number of times
	std::set<ArtifactID> preferredPicks;
	for (auto const & artifact : potentialPicks)
		if (allocatedArtifacts[artifact] == leastUsedTimes)
			preferredPicks.insert(artifact);

	assert(!preferredPicks.empty());

	ArtifactID artID = *RandomGeneratorUtil::nextItem(preferredPicks, rand);
	allocatedArtifacts[artID] += 1; // record +1 more usage
	return artID;
}

ArtifactID CGameState::pickRandomArtifact(CRandomGenerator & rand, std::function<bool(ArtifactID)> accepts)
{
	return pickRandomArtifact(rand, 0xff, std::move(accepts));
}

ArtifactID CGameState::pickRandomArtifact(CRandomGenerator & rand, int flags)
{
	return pickRandomArtifact(rand, flags, [](const ArtifactID &) { return true; });
}

VCMI_LIB_NAMESPACE_END
