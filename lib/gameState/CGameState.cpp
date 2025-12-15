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
#include "GameStatePackVisitor.h"
#include "SThievesGuildInfo.h"
#include "QuestInfo.h"

#include "../GameSettings.h"
#include "../texts/CGeneralTextHandler.h"
#include "../CPlayerState.h"
#include "../CStopWatch.h"
#include "../IGameSettings.h"
#include "../StartInfo.h"
#include "../TerrainHandler.h"
#include "../VCMIDirs.h"
#include "../GameLibrary.h"
#include "../bonuses/Limiters.h"
#include "../bonuses/Propagators.h"
#include "../bonuses/Updaters.h"
#include "../battle/BattleInfo.h"
#include "../callback/IGameInfoCallback.h"
#include "../callback/IGameRandomizer.h"
#include "../campaign/CampaignState.h"
#include "../constants/StringConstants.h"
#include "../entities/artifact/ArtifactUtils.h"
#include "../entities/artifact/CArtHandler.h"
#include "../entities/faction/CTownHandler.h"
#include "../entities/hero/CHero.h"
#include "../entities/hero/CHeroClass.h"
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
#include "../mapping/CCastleEvent.h"
#include "../mapping/CMap.h"
#include "../mapping/CMapEditManager.h"
#include "../mapping/CMapService.h"
#include "../modding/ActiveModsInSaveList.h"
#include "../modding/IdentifierStorage.h"
#include "../modding/ModScope.h"
#include "../networkPacks/NetPacksBase.h"
#include "../pathfinder/CPathfinder.h"
#include "../pathfinder/PathfinderOptions.h"
#include "../rmg/CMapGenerator.h"
#include "../serializer/CMemorySerializer.h"
#include "../serializer/CLoadFile.h"
#include "../serializer/CSaveFile.h"
#include "../spells/CSpellHandler.h"
#include "UpgradeInfo.h"

#include <vstd/RNG.h>

VCMI_LIB_NAMESPACE_BEGIN

std::shared_mutex CGameState::mutex;

HeroTypeID CGameState::pickNextHeroType(vstd::RNG & randomGenerator, const PlayerColor & owner)
{
	const PlayerSettings &ps = scenarioOps->getIthPlayersSettings(owner);
	if(ps.hero.isValid() && !isUsedHero(HeroTypeID(ps.hero))) //we haven't used selected hero
	{
		return HeroTypeID(ps.hero);
	}

	return pickUnusedHeroTypeRandomly(randomGenerator, owner);
}

HeroTypeID CGameState::pickUnusedHeroTypeRandomly(vstd::RNG & randomGenerator, const PlayerColor & owner)
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
		return *RandomGeneratorUtil::nextItem(factionHeroes, randomGenerator);
	}

	logGlobal->warn("Cannot find free hero of appropriate faction for player %s - trying to get first available...", owner.toString());
	if(!otherHeroes.empty())
	{
		return *RandomGeneratorUtil::nextItem(otherHeroes, randomGenerator);
	}

	logGlobal->error("No free allowed heroes!");
	auto notAllowedHeroesButStillBetterThanCrash = getUnusedAllowedHeroes(true);
	if(!notAllowedHeroesButStillBetterThanCrash.empty())
		return *notAllowedHeroesButStillBetterThanCrash.begin();

	logGlobal->error("No free heroes at all!");
	throw std::runtime_error("Can not allocate hero. All heroes are already used.");
}

int CGameState::getDate(int d, Date mode)
{
	int temp;
	switch (mode)
	{
	case Date::DAY:
		return d;
	case Date::DAY_OF_WEEK: //day of week
		temp = (d)%7; // 1 - Monday, 7 - Sunday
		return temp ? temp : 7;
	case Date::WEEK:  //current week
		temp = ((d-1)/7)+1;
		if (!(temp%4))
			return 4;
		else
			return (temp%4);
	case Date::MONTH: //current month
		return ((d-1)/28)+1;
	case Date::DAY_OF_MONTH: //day of month
		temp = (d)%28;
		if (temp)
			return temp;
		else return 28;
	}
	return 0;
}

int CGameState::getDate(Date mode) const
{
	return getDate(day, mode);
}

CGameState::CGameState()
	:globalEffects(BonusNodeType::GLOBAL_EFFECTS)
{
	heroesPool = std::make_unique<TavernHeroesPool>(this);
}

CGameState::~CGameState()
{
	// explicitly delete all ongoing battles first - BattleInfo destructor requires valid CGameState
	currentBattles.clear();
}

const IGameSettings & CGameState::getSettings() const
{
	return map->getSettings();
}

void CGameState::preInit(Services * newServices)
{
	services = newServices;
}

void CGameState::init(const IMapService * mapService, StartInfo * si, IGameRandomizer & gameRandomizer, Load::ProgressAccumulator & progressTracking, bool allowSavingRandomMap)
{
	assert(services);
	scenarioOps = CMemorySerializer::deepCopy(*si);
	initialOpts = CMemorySerializer::deepCopy(*si);
	si = nullptr;
	auto & randomGenerator = gameRandomizer.getDefault();

	switch(scenarioOps->mode)
	{
	case EStartMode::NEW_GAME:
		initNewGame(mapService, randomGenerator, allowSavingRandomMap, progressTracking);
		break;
	case EStartMode::CAMPAIGN:
		initCampaign();
		break;
	default:
		logGlobal->error("Wrong mode: %d", static_cast<int>(scenarioOps->mode));
		return;
	}
	logGlobal->info("Map loaded!");

	day = 0;

	logGlobal->debug("Initialization:");

	initGlobalBonuses();
	initPlayerStates();
	if (campaign)
		campaign->placeCampaignHeroes(randomGenerator);
	removeHeroPlaceholders();
	initGrailPosition(randomGenerator);
	initRandomFactionsForPlayers(randomGenerator);
	randomizeMapObjects(gameRandomizer);
	placeStartingHeroes(randomGenerator);
	initOwnedObjects();
	initDifficulty();
	initHeroes(gameRandomizer);
	initStartingBonus(gameRandomizer);
	initTowns(randomGenerator);
	initTownNames(randomGenerator);
	placeHeroesInTowns();
	initMapObjects(gameRandomizer);
	buildBonusSystemTree();
	initVisitingAndGarrisonedHeroes();
	initFogOfWar();

	for(auto & elem : teams)
	{
		map->obelisksVisited[elem.first] = 0;
	}

	logGlobal->debug("\tChecking objectives");
	map->checkForObjectives(); //needs to be run when all objects are properly placed
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
	{
		//index is hero type
		HeroTypeID heroID(index);
		auto heroOnMap = map->getHero(heroID);
		auto heroInPool = map->tryGetFromHeroPool(heroID);

		if(heroOnMap)
		{
			heroOnMap->updateFrom(data);
		}
		else if (heroInPool)
		{
			heroInPool->updateFrom(data);
		}
		else
		{
			logGlobal->error("Update entity: hero index %s is out of range", index);
		}
		break;
	}
	case Metatype::MAP_OBJECT_INSTANCE:
	{
		CGObjectInstance * obj = getObjInstance(ObjectInstanceID(index));
		if(obj)
		{
			obj->updateFrom(data);
		}
		else
		{
			logGlobal->error("Update entity: object index %s is out of range", index);
		}
		break;
	}
	default:
		logGlobal->error("This metatype update is not implemented");
		break;
	}
}

void CGameState::updateOnLoad(const StartInfo & si)
{
	assert(services);
	scenarioOps->playerInfos = si.playerInfos;
	for(auto & i : si.playerInfos)
		players.at(i.first).human = i.second.isControlledByHuman();
	scenarioOps->extraOptionsInfo = si.extraOptionsInfo;
	scenarioOps->turnTimerInfo = si.turnTimerInfo;
	scenarioOps->simturnsInfo = si.simturnsInfo;
}

void CGameState::initNewGame(const IMapService * mapService, vstd::RNG & randomGenerator, bool allowSavingRandomMap, Load::ProgressAccumulator & progressTracking)
{
	if(scenarioOps->createRandomMap())
	{
		logGlobal->info("Create random map.");
		CStopWatch sw;

		// Gen map
		CMapGenerator mapGenerator(*scenarioOps->mapGenOptions, this, randomGenerator.nextInt());
		progressTracking.include(mapGenerator);

		map = mapGenerator.generate();
		progressTracking.exclude(mapGenerator);

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
					playerSettings.name = LIBRARY->generaltexth->allTexts[468];
				}
				playerSettings.color = PlayerColor(i);
			}
			else
			{
				scenarioOps->playerInfos.erase(PlayerColor(i));
			}
		}

		if(allowSavingRandomMap)
		{
			try
			{
				auto path = VCMIDirs::get().userDataPath() / "Maps" / "RandomMaps";
				boost::filesystem::create_directories(path);

				std::shared_ptr<CMapGenOptions> options = scenarioOps->mapGenOptions;

				const std::string templateName = options->getMapTemplate()->getName();
				const std::string dt = vstd::getDateTimeISO8601Basic(std::time(nullptr));

				const std::string fileName = boost::str(boost::format("%s_%s.vmap") % dt % templateName );
				const auto fullPath = path / fileName;

				map->name.appendRawString(boost::str(boost::format(" %s") % dt));

				mapService->saveMap(map, fullPath);

				logGlobal->info("Random map has been saved to:");
				logGlobal->info(fullPath.string());
			}
			catch(...)
			{
				logGlobal->error("Saving random map failed with exception");
			}
		}


		logGlobal->info("Generated random map in %i ms.", sw.getDiff());
	}
	else
	{
		logGlobal->info("Open map file: %s", scenarioOps->mapname);
		const ResourcePath mapURI(scenarioOps->mapname, EResType::MAP);
		map = mapService->loadMap(mapURI, this);
	}
}

void CGameState::initCampaign()
{
	campaign = std::make_unique<CGameStateCampaign>(this);
	map = campaign->getCurrentMap();
}

void CGameState::initGlobalBonuses()
{
	const JsonNode & baseBonuses = getSettings().getValue(EGameSettings::BONUSES_GLOBAL);
	logGlobal->debug("\tLoading global bonuses");
	for(const auto & b : baseBonuses.Struct())
	{
		auto bonus = JsonUtils::parseBonus(b.second);
		bonus->source = BonusSource::GLOBAL;//for all
		bonus->sid = BonusSourceID(); //there is one global object
		globalEffects.addNewBonus(bonus);
	}
	LIBRARY->creh->loadCrExpBon(globalEffects);
}

void CGameState::initDifficulty()
{
	logGlobal->debug("\tLoading difficulty settings");
	JsonNode config = JsonUtils::assembleFromFiles("config/difficulty.json");
	config.setModScope(ModScope::scopeGame()); // FIXME: should be set to actual mod
	
	const JsonNode & difficultyAI(config["ai"][GameConstants::DIFFICULTY_NAMES[scenarioOps->difficulty]]);
	const JsonNode & difficultyHuman(config["human"][GameConstants::DIFFICULTY_NAMES[scenarioOps->difficulty]]);
	
	auto setDifficulty = [this](PlayerState & state, const JsonNode & json)
	{
		//set starting resources
		state.resources.resolveFromJson(json["resources"]);

		//handicap
		const PlayerSettings &ps = scenarioOps->getIthPlayersSettings(state.color);
		state.resources += ps.handicap.startBonus;
		
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

void CGameState::initGrailPosition(vstd::RNG & randomGenerator)
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
					if(!t.blocked()
					   && !t.visitable()
						&& t.isLand()
						&& t.getTerrain()->isPassable()
						&& (int)map->grailPos.dist2dSQ(int3(x, y, z)) <= (map->grailRadius * map->grailRadius))
						allowedPos.emplace_back(x, y, z);
				}
			}
		}

		//remove tiles with holes
		for(auto & elem : map->getObjects())
			if(elem && elem->ID == Obj::HOLE)
				allowedPos -= elem->anchorPos();

		if(!allowedPos.empty())
		{
			map->grailPos = *RandomGeneratorUtil::nextItem(allowedPos, randomGenerator);
		}
		else
		{
			logGlobal->warn("Grail cannot be placed, no appropriate tile found!");
		}
	}
}

void CGameState::initRandomFactionsForPlayers(vstd::RNG & randomGenerator)
{
	logGlobal->debug("\tPicking random factions for players");
	for(auto & elem : scenarioOps->playerInfos)
	{
		if(elem.second.castle==FactionID::RANDOM)
		{
			auto randomID = randomGenerator.nextInt((int)map->players[elem.first.getNum()].allowedFactions.size() - 1);
			auto iter = map->players[elem.first.getNum()].allowedFactions.begin();
			std::advance(iter, randomID);

			elem.second.castle = *iter;
		}
	}
}

void CGameState::randomizeMapObjects(IGameRandomizer & gameRandomizer)
{
	logGlobal->debug("\tRandomizing objects");
	for(const auto & object : map->getObjects())
	{
		object->pickRandomObject(gameRandomizer);

		//handle Favouring Winds - mark tiles under it
		if(object->ID == Obj::FAVORABLE_WINDS)
		{
			for (int i = 0; i < object->getWidth() ; i++)
			{
				for (int j = 0; j < object->getHeight() ; j++)
				{
					int3 pos = object->anchorPos() - int3(i,j,0);
					if(map->isInTheMap(pos)) map->getTile(pos).extTileFlags |= 128;
				}
			}
		}
	}
}

void CGameState::initOwnedObjects()
{
	for(const auto & object : map->getObjects())
	{
		if (object && object->getOwner().isValidPlayer())
			getPlayerState(object->getOwner())->addOwnedObject(object);
	}
}

void CGameState::initPlayerStates()
{
	logGlobal->debug("\tCreating player entries in gs");
	for(auto & elem : scenarioOps->playerInfos)
	{
		players.try_emplace(elem.first, this);
		PlayerState & p = players.at(elem.first);
		p.color=elem.first;
		p.human = elem.second.isControlledByHuman();
		p.team = map->players[elem.first.getNum()].team;
		teams[p.team].id = p.team;//init team
		teams[p.team].players.insert(elem.first);//add player to team
	}
}

void CGameState::placeStartingHero(const PlayerColor & playerColor, const HeroTypeID & heroTypeId, int3 townPos)
{
	for (const auto & townID : map->getAllTowns())
	{
		auto town = getTown(townID);
		if(town->anchorPos() == townPos)
		{
			townPos = town->visitablePos();
			break;
		}
	}

	auto hero = map->tryTakeFromHeroPool(heroTypeId);

	if (!hero)
	{
		auto handler = LIBRARY->objtypeh->getHandlerFor(Obj::HERO, heroTypeId.toHeroType()->heroClass->getIndex());
		auto object = handler->create(this, handler->getTemplates().front());
		hero = std::dynamic_pointer_cast<CGHeroInstance>(object);
		hero->ID = Obj::HERO;
		hero->setHeroType(heroTypeId);
		assert(hero->appearance != nullptr);
	}

	hero->tempOwner = playerColor;
	hero->setAnchorPos(townPos + hero->getVisitableOffset());
	map->getEditManager()->insertObject(hero);
}

void CGameState::placeStartingHeroes(vstd::RNG & randomGenerator)
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

			HeroTypeID heroTypeId = pickNextHeroType(randomGenerator, playerColor);
			if(playerSettingPair.second.hero == HeroTypeID::NONE || playerSettingPair.second.hero == HeroTypeID::RANDOM)
				playerSettingPair.second.hero = heroTypeId;

			placeStartingHero(playerColor, HeroTypeID(heroTypeId), playerInfo.posOfMainTown);
		}
	}
}

void CGameState::removeHeroPlaceholders()
{
	// remove any hero placeholders that remain on map after (potential) campaign heroes placement
	for(auto obj : map->getObjects<CGHeroPlaceholder>())
	{
		map->eraseObject(obj->id);
	}
}

void CGameState::initHeroes(IGameRandomizer & gameRandomizer)
{
	//heroes instances initialization
	for (auto heroID : map->getHeroesOnMap())
	{
		auto hero = getHero(heroID);
		if (hero->getOwner() == PlayerColor::UNFLAGGABLE)
		{
			logGlobal->warn("Hero with uninitialized owner!");
			continue;
		}
		hero->initHero(gameRandomizer);
		hero->armyChanged();
	}

	// generate boats for all heroes on water
	for (auto heroID : map->getHeroesOnMap())
	{
		auto hero = getHero(heroID);
		assert(map->isInTheMap(hero->visitablePos()));
		const auto & tile = map->getTile(hero->visitablePos());

		if (hero->ID == Obj::PRISON)
			continue;

		if (tile.isWater())
		{
			auto handler = LIBRARY->objtypeh->getHandlerFor(Obj::BOAT, hero->getBoatType().getNum());
			auto boat = std::dynamic_pointer_cast<CGBoat>(handler->create(this, nullptr));
			handler->configureObject(boat.get(), gameRandomizer);

			boat->setAnchorPos(hero->anchorPos());
			boat->appearance = handler->getTemplates().front();
			map->generateUniqueInstanceName(boat.get());
			map->addNewObject(boat);
			hero->setBoat(boat.get());
		}
	}

	std::set<HeroTypeID> heroesToCreate = getUnusedAllowedHeroes(); //ids of heroes to be created and put into the pool

	for(const HeroTypeID & htype : heroesToCreate) //all not used allowed heroes go with default state into the pool
	{
		CGHeroInstance * heroInPool = map->tryGetFromHeroPool(htype);

		// some heroes are created as part of map loading (sod+ h3m maps)
		// instances for h3 heroes from roe/ab h3m maps and heroes from mods at this point don't exist -> create them
		if (!heroInPool)
		{
			auto newHeroPtr = std::make_shared<CGHeroInstance>(this);
			newHeroPtr->subID = htype.getNum();
			map->addToHeroPool(newHeroPtr);
			heroInPool = newHeroPtr.get();
		}
		map->generateUniqueInstanceName(heroInPool);
		heroInPool->initHero(gameRandomizer);
		heroesPool->addHeroToPool(htype);
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
		fow.resize(boost::extents[layers][map->width][map->height]);
		std::fill(fow.data(), fow.data() + fow.num_elements(), 0);

		for(const auto & obj : map->getObjects())
		{
			if(!vstd::contains(elem.second.players, obj->getOwner()))
				continue; //not a flagged object

			FowTilesType tiles;
			getTilesInRange(tiles, obj->getSightCenter(), obj->getSightRadius(), ETileVisibility::HIDDEN, obj->tempOwner);
			for(const int3 & tile : tiles)
			{
				elem.second.fogOfWarMap[tile.z][tile.x][tile.y] = 1;
			}
		}
	}
}

void CGameState::initStartingBonus(IGameRandomizer & gameRandomizer)
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
			scenarioOps->playerInfos[elem.first].bonus = static_cast<PlayerStartingBonus>(gameRandomizer.getDefault().nextInt(2));

		switch(scenarioOps->playerInfos[elem.first].bonus)
		{
		case PlayerStartingBonus::GOLD:
			elem.second.resources[EGameResID::GOLD] += gameRandomizer.getDefault().nextInt(5, 10) * 100;
			break;
		case PlayerStartingBonus::RESOURCE:
			{
				auto res = (*LIBRARY->townh)[scenarioOps->playerInfos[elem.first].castle]->town->primaryRes;
				if(res == EGameResID::WOOD_AND_ORE)
				{
					int amount = gameRandomizer.getDefault().nextInt(5, 10);
					elem.second.resources[EGameResID::WOOD] += amount;
					elem.second.resources[EGameResID::ORE] += amount;
				}
				else
				{
					elem.second.resources[res] += gameRandomizer.getDefault().nextInt(3, 6);
				}
				break;
			}
		case PlayerStartingBonus::ARTIFACT:
			{
				if(elem.second.getHeroes().empty())
				{
					logGlobal->error("Cannot give starting artifact - no heroes!");
					break;
				}
				const Artifact * toGive = gameRandomizer.rollArtifact(EArtifactClass::ART_TREASURE).toEntity(LIBRARY);

				CGHeroInstance *hero = elem.second.getHeroes()[0];
				if(!giveHeroArtifact(hero, toGive->getId()))
					logGlobal->error("Cannot give starting artifact - no free slots!");
			}
			break;
		}
	}
}

void CGameState::initTownNames(vstd::RNG & randomGenerator)
{
	std::map<FactionID, std::vector<int>> availableNames;
	for(const auto & faction : LIBRARY->townh->getDefaultAllowed())
	{
		std::vector<int> potentialNames;
		if(faction.toFaction()->town->getRandomNamesCount() > 0)
		{
			for(int i = 0; i < faction.toFaction()->town->getRandomNamesCount(); ++i)
				potentialNames.push_back(i);

			availableNames[faction] = potentialNames;
		}
	}

	for (const auto & townID : map->getAllTowns())
	{
		auto vti = getTown(townID);

		if(!vti->getNameTextID().empty())
			continue;

		FactionID faction = vti->getFactionID();

		if(availableNames.empty())
		{
			logGlobal->warn("Failed to find available name for a random town!");
			vti->setNameTextId("core.genrltxt.508"); // Unnamed
			continue;
		}

		// If town has no available names (for example - all were picked) - pick names from some other faction that still has names available
		if(!availableNames.count(faction))
			faction = RandomGeneratorUtil::nextItem(availableNames, randomGenerator)->first;

		auto nameIt = RandomGeneratorUtil::nextItem(availableNames[faction], randomGenerator);
		vti->setNameTextId(faction.toFaction()->town->getRandomNameTextID(*nameIt));

		availableNames[faction].erase(nameIt);
		if(availableNames[faction].empty())
			availableNames.erase(faction);
	}
}

void CGameState::initTowns(vstd::RNG & randomGenerator)
{
	logGlobal->debug("\tTowns");

	if (campaign)
		campaign->initTowns();

	for (const auto & townID : map->getAllTowns())
	{
		auto vti = getTown(townID);

		assert(vti->getTown()->creatures.size() <= GameConstants::CREATURES_PER_TOWN);

		constexpr std::array basicDwellings = { BuildingID::DWELL_LVL_1, BuildingID::DWELL_LVL_2, BuildingID::DWELL_LVL_3, BuildingID::DWELL_LVL_4, BuildingID::DWELL_LVL_5, BuildingID::DWELL_LVL_6, BuildingID::DWELL_LVL_7, BuildingID::DWELL_LVL_8 };
		constexpr std::array upgradedDwellings = { BuildingID::DWELL_LVL_1_UP, BuildingID::DWELL_LVL_2_UP, BuildingID::DWELL_LVL_3_UP, BuildingID::DWELL_LVL_4_UP, BuildingID::DWELL_LVL_5_UP, BuildingID::DWELL_LVL_6_UP, BuildingID::DWELL_LVL_7_UP, BuildingID::DWELL_LVL_8_UP };
		constexpr std::array hordes = { BuildingID::HORDE_PLACEHOLDER1, BuildingID::HORDE_PLACEHOLDER2, BuildingID::HORDE_PLACEHOLDER3, BuildingID::HORDE_PLACEHOLDER4, BuildingID::HORDE_PLACEHOLDER5, BuildingID::HORDE_PLACEHOLDER6, BuildingID::HORDE_PLACEHOLDER7, BuildingID::HORDE_PLACEHOLDER8 };

		//init buildings
		if(vti->hasBuilt(BuildingID::DEFAULT)) //give standard set of buildings
		{
			vti->removeBuilding(BuildingID::DEFAULT);
			vti->addBuilding(BuildingID::VILLAGE_HALL);
			if(vti->tempOwner != PlayerColor::NEUTRAL)
				vti->addBuilding(BuildingID::TAVERN);

			auto definesBuildingsChances = getSettings().getVector(EGameSettings::TOWNS_STARTING_DWELLING_CHANCES);

			for(int i = 0; i < definesBuildingsChances.size(); i++)
			{
				if((randomGenerator.nextInt(1,100) <= definesBuildingsChances[i]))
				{
					vti->addBuilding(basicDwellings[i]);
				}
			}
		}

		// village hall must always exist
		vti->addBuilding(BuildingID::VILLAGE_HALL);

		//init hordes
		for (int i = 0; i < vti->getTown()->creatures.size(); i++)
		{
			if(vti->hasBuilt(hordes[i])) //if we have horde for this level
			{
				vti->removeBuilding(hordes[i]);//remove old ID
				if (vti->getTown()->hordeLvl.at(0) == i)//if town first horde is this one
				{
					vti->addBuilding(BuildingID::HORDE_1);//add it
					//if we have upgraded dwelling as well
					if(vti->hasBuilt(upgradedDwellings[i]))
						vti->addBuilding(BuildingID::HORDE_1_UPGR);//add it as well
				}
				if (vti->getTown()->hordeLvl.at(1) == i)//if town second horde is this one
				{
					vti->addBuilding(BuildingID::HORDE_2);
					if(vti->hasBuilt(upgradedDwellings[i]))
						vti->addBuilding(BuildingID::HORDE_2_UPGR);
				}
			}
		}

		//#1444 - remove entries that don't have buildings defined (like some unused extra town hall buildings)
		//But DO NOT remove horde placeholders before they are replaced
		for(const auto & building : vti->getBuildings())
		{
			if(!vti->getTown()->buildings.count(building) || !vti->getTown()->buildings.at(building))
				vti->removeBuilding(building);
		}

		if(vti->hasBuilt(BuildingID::SHIPYARD) && vti->shipyardStatus()==IBoatGenerator::TILE_BLOCKED)
			vti->removeBuilding(BuildingID::SHIPYARD);//if we have harbor without water - erase it (this is H3 behaviour)

		//Early check for #1444-like problems
		for([[maybe_unused]] const auto & building : vti->getBuildings())
		{
			assert(vti->getTown()->buildings.at(building) != nullptr);
		}

		//town events
		for(CCastleEvent &ev : vti->events)
		{
			for (int i = 0; i<vti->getTown()->creatures.size(); i++)
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
		vti->possibleSpells -= SpellID::PRESET;

		for(ui32 z=0; z<vti->obligatorySpells.size();z++)
		{
			const auto * s = vti->obligatorySpells[z].toSpell();
			vti->spells[s->getLevel()-1].push_back(s->id);
			vti->possibleSpells -= s->id;
		}

		vstd::erase_if(vti->possibleSpells, [&](const SpellID & spellID)
		{
			const auto * spell = spellID.toSpell();

			if (spell->getProbability(vti->getFactionID()) == 0)
				return true;

			if (spell->isSpecial() || spell->isCreatureAbility())
				return true;

			if (!isAllowed(spellID))
				return true;

			return false;
		});

		std::vector<int> spellWeights;
		for (auto & spellID : vti->possibleSpells)
			spellWeights.push_back(spellID.toSpell()->getProbability(vti->getFactionID()));


		while(!vti->possibleSpells.empty())
		{
			size_t index = RandomGeneratorUtil::nextItemWeighted(spellWeights, randomGenerator);

			const auto * s = vti->possibleSpells[index].toSpell();
			vti->spells[s->getLevel()-1].push_back(s->id);

			vti->possibleSpells.erase(vti->possibleSpells.begin() + index);
			spellWeights.erase(spellWeights.begin() + index);
		}
	}
}

void CGameState::initMapObjects(IGameRandomizer & gameRandomizer)
{
	logGlobal->debug("\tObject initialization");

	for(auto & obj : map->getObjects())
		obj->initObj(gameRandomizer);

	logGlobal->debug("\tObject initialization done");
	for(auto & q : map->getObjects<CGSeerHut>())
	{
		if (q->ID ==Obj::QUEST_GUARD || q->ID ==Obj::SEER_HUT)
			q->setObjToKill();
	}
	CGSubterraneanGate::postInit(this); //pairing subterranean gates

	map->calculateGuardingGreaturePositions(); //calculate once again when all the guards are placed and initialized
}

void CGameState::placeHeroesInTowns()
{
	for(auto & player : players)
	{
		if(player.first == PlayerColor::NEUTRAL)
			continue;

		for(CGHeroInstance * h : player.second.getHeroes())
		{
			for(CGTownInstance * t : player.second.getTowns())
			{
				bool heroOnTownBlockableTile = t->blockingAt(h->visitablePos());

				// current hero position is at one of blocking tiles of current town
				// assume that this hero should be visiting the town (H3M format quirk) and move hero to correct position
				if (heroOnTownBlockableTile)
				{
					int3 correctedPos = h->convertFromVisitablePos(t->visitablePos());
					map->moveObject(h->id, correctedPos);
					assert(t->visitableAt(h->visitablePos()));
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
		for(CGHeroInstance * h : player.second.getHeroes())
		{
			for(CGTownInstance * t : player.second.getTowns())
			{
				if(h->visitablePos().z != t->visitablePos().z)
					continue;

				if (t->visitableAt(h->visitablePos()))
				{
					assert(t->getVisitingHero() == nullptr);
					t->setVisitingHero(h);
				}
			}
		}
	}

	for (auto heroID : map->getHeroesOnMap())
	{
		auto hero = getHero(heroID);
		if (hero->getVisitedTown())
			assert(hero->getVisitedTown()->getVisitingHero() == hero);
	}
}

const BattleInfo * CGameState::getBattle(const PlayerColor & player) const
{
	if (!player.isValidPlayer())
		return nullptr;

	for (const auto & battlePtr : currentBattles)
		if (battlePtr->getSide(BattleSide::ATTACKER).color == player || battlePtr->getSide(BattleSide::DEFENDER).color == player)
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

BattleField CGameState::battleGetBattlefieldType(int3 tile, vstd::RNG & randomGenerator) const
{
	assert(tile.isValid());

	if(!tile.isValid())
		return BattleField::NONE;

	const TerrainTile &t = map->getTile(tile);

	ObjectInstanceID topObjectID = t.visitableObjects.front();
	const CGObjectInstance * topObject = getObjInstance(topObjectID);
	if(topObject && topObject->getBattlefield() != BattleField::NONE)
	{
		return topObject->getBattlefield();
	}

	for(auto & obj : map->getObjects())
	{
		//look only for objects covering given tile
		if( !obj->coveringAt(tile))
			continue;

		auto customBattlefield = obj->getBattlefield();

		if(customBattlefield != BattleField::NONE)
			return customBattlefield;
	}

	if(map->isCoastalTile(tile)) //coastal tile is always ground
		return BattleField(*LIBRARY->identifiers()->getIdentifier("core", "battlefield.sand_shore"));
	
	if (t.getTerrain()->battleFields.empty())
		throw std::runtime_error("Failed to find battlefield for terrain " + t.getTerrain()->getJsonKey());

	return BattleField(*RandomGeneratorUtil::nextItem(t.getTerrain()->battleFields, randomGenerator));
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

void CGameState::apply(CPackForClient & pack)
{
	GameStatePackVisitor visitor(*this);
	pack.visit(visitor);
}

void CGameState::calculatePaths(const std::shared_ptr<PathfinderConfig> & config) const
{
	CPathfinder pathfinder(*this, config);
	pathfinder.calculatePaths();
}

/**
 * Tells if the tile is guarded by a monster as well as the position
 * of the monster that will attack on it.
 *
 * @return int3(-1, -1, -1) if the tile is unguarded, or the position of
 * the monster guarding the tile.
 */
std::vector<const CGObjectInstance*> CGameState::guardingCreatures (int3 pos) const
{
	std::vector<const CGObjectInstance*> guards;
	const int3 originalPos = pos;
	if (!map->isInTheMap(pos))
		return guards;

	const TerrainTile &posTile = map->getTile(pos);
	if (posTile.visitable())
	{
		for (ObjectInstanceID objectID : posTile.visitableObjects)
		{
			const CGObjectInstance * object = getObjInstance(objectID);
			if(object->isBlockedVisitable())
			{
				if (object->ID == Obj::MONSTER) // Monster
					guards.push_back(object);
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
				if (tile.visitable() && (tile.isWater() == posTile.isWater()))
				{
					for (ObjectInstanceID objectID : tile.visitableObjects)
					{
						const CGObjectInstance * object = getObjInstance(objectID);

						if (object->ID == Obj::MONSTER  &&  map->checkForVisitableDir(pos, &map->getTile(originalPos), originalPos)) // Monster being able to attack investigated tile
						{
							guards.push_back(object);
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

bool CGameState::isVisibleFor(int3 pos, PlayerColor player) const
{
	if (!map->isInTheMap(pos))
		return false;
	if(player == PlayerColor::NEUTRAL)
		return false;
	if(player.isSpectator())
		return true;

	return getPlayerTeam(player)->fogOfWarMap[pos.z][pos.x][pos.y];
}

bool CGameState::isVisibleFor(const CGObjectInstance * obj, PlayerColor player) const
{
	//we should always see our own heroes - but sometimes not visible heroes cause crash :?
	// TODO: Mircea: Looks like a bug. See ExplorationBehavior::decompose
	// if (!aiNk->cc->isVisibleFor(aiNk->cc->getObjInstance(exit), aiNk->playerID))
	// First thought: we shouldn't have the following if: if(player == obj->tempOwner)
	// because we need to triggger the actual isInTheMap and isVisibleFor code
	if(player == obj->tempOwner)
		return true;

	if(player == PlayerColor::NEUTRAL) //-> TODO ??? needed?
		return false;

	return iteratePositionsUntilTrue(
		obj,
		[this, obj, player](const int3 & pos) -> bool
		{
			// object is visible when at least one tile is visible
			return map->isInTheMap(pos) && obj->coveringAt(pos) && isVisibleFor(pos, player);
		}
	);
}

EVictoryLossCheckResult CGameState::checkForVictoryAndLoss(const PlayerColor & player) const
{
	const MetaString messageWonSelf = MetaString::createFromTextID("core.genrltxt.659");
	const MetaString messageWonOther = MetaString::createFromTextID("core.genrltxt.5");
	const MetaString messageLostSelf = MetaString::createFromTextID("core.genrltxt.7");
	const MetaString messageLostOther = MetaString::createFromTextID("core.genrltxt.8");

	auto evaluateEvent = [this, player](const EventCondition & condition)
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
			for(const auto & elem : p->getHeroes())
				if(elem->hasArt(condition.objectType.as<ArtifactID>()))
					return true;
			return false;
		}
		case EventCondition::HAVE_CREATURES:
		{
			//check if in players armies there is enough creatures
			// NOTE: only heroes & towns are checked, in line with H3.
			// Garrisons, mines, and guards of owned dwellings(!) are excluded
			int totalCreatures = 0;
			for (const auto & hero : p->getHeroes())
				for(const auto & elem : hero->Slots()) //iterate through army
					if(elem.second->getId() == condition.objectType.as<CreatureID>()) //it's searched creature
						totalCreatures += elem.second->getCount();

			for (const auto & town : p->getTowns())
				for(const auto & elem : town->Slots()) //iterate through army
					if(elem.second->getId() == condition.objectType.as<CreatureID>()) //it's searched creature
						totalCreatures += elem.second->getCount();

			return totalCreatures >= condition.value;
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
				for (const CGTownInstance * t : p->getTowns())
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
				for(const auto & elem : map->getObjects()) // mode B - destroy all objects of this type
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
			// NOTE: CGameInfoCallback specified explicitly in order to get const version
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
				for(const auto & elem : map->getObjects()) // mode B - flag all objects of this type
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
			bool garrisonedWon = t->getGarrisonHero() && t->getGarrisonHero()->getOwner() == player && t->getGarrisonHero()->hasArt(condition.objectType.as<ArtifactID>());
			bool visitingWon = t->getVisitingHero() && t->getVisitingHero()->getOwner() == player && t->getVisitingHero()->hasArt(condition.objectType.as<ArtifactID>());

			return garrisonedWon || visitingWon;
		}
		case EventCondition::DAYS_PASSED:
		{
			return day > condition.value;
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
		if(elem.second.status == EPlayerStatus::WINNER)
			return elem.second.color;

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

void CGameState::obtainPlayersStats(SThievesGuildInfo & tgi, int level) const
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
		tgi.FIELD = Statistic::getRank(stats); \
	}

	for(auto & elem : players)
	{
		if(!playerInactive(elem.second.color))
			tgi.playerColors.push_back(elem.second.color);
	}

	if(level >= 0) //num of towns & num of heroes
	{
		//num of towns
		FILL_FIELD(numOfTowns, g->second.getTowns().size())
		//num of heroes
		FILL_FIELD(numOfHeroes, g->second.getHeroes().size())
	}
	if(level >= 1) //best hero's portrait
	{
		for(const auto & player : players)
		{
			if(playerInactive(player.second.color))
				continue;
			const CGHeroInstance * best = Statistic::findBestHero(this, player.second.color);
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
		FILL_FIELD(obelisks, Statistic::getObeliskVisited(this, getPlayerTeam(g->second.color)->id))
	}
	if(level >= 4) //artifacts
	{
		FILL_FIELD(artifacts, Statistic::getNumberOfArts(&g->second))
	}
	if(level >= 4) //army strength
	{
		FILL_FIELD(army, Statistic::getArmyStrength(&g->second))
	}
	if(level >= 5) //income
	{
		FILL_FIELD(income, Statistic::getIncome(this, &g->second))
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
			for(const auto & elem : player.second.getHeroes())
			{
				for(const auto & it : elem->Slots())
				{
					CreatureID toCmp = it.second->getId(); //ID of creature we should compare with the best one
					if(bestCre == CreatureID::NONE || bestCre.toEntity(LIBRARY)->getAIValue() < toCmp.toEntity(LIBRARY)->getAIValue())
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
	for(auto & armed : map->getObjects<CGObjectInstance>())
		armed->attachToBonusSystem(*this);
}

void CGameState::restoreBonusSystemTree()
{
	heroesPool->setGameState(this);

	buildGlobalTeamPlayerTree();
	for(auto & armed : map->getObjects<CGObjectInstance>())
		armed->restoreBonusSystem(*this);

	for(auto & art : map->getArtifacts())
		art->attachToBonusSystem(*this);

	for(auto & heroID : map->getHeroesInPool())
		map->tryGetFromHeroPool(heroID)->artDeserializationFix(*this, map->tryGetFromHeroPool(heroID));

	if (campaign)
		campaign->setGamestate(this);
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

bool CGameState::giveHeroArtifact(CGHeroInstance * h, const ArtifactID & aid)
{
	 CArtifactInstance * ai = createArtifact(aid);
	 auto slot = ArtifactUtils::getArtAnyPosition(h, aid);
	 if(ArtifactUtils::isSlotEquipment(slot) || ArtifactUtils::isSlotBackpack(slot))
	 {
		 map->putArtifactInstance(*h, ai->getId(), slot);
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

	for (auto heroID : map->getHeroesOnMap())
		ret -= getHero(heroID)->getHeroTypeID();

	return ret;
}

bool CGameState::isUsedHero(const HeroTypeID & hid) const
{
	return getUsedHero(hid);
}

CGHeroInstance * CGameState::getUsedHero(const HeroTypeID & hid) const
{
	for(auto hero : map->getObjects<CGHeroInstance>()) //prisons
	{
		if (hero->getHeroTypeID() == hid)
			return hero;
	}

	return nullptr;
}

TeamState::TeamState()
	:CBonusSystemNode(BonusNodeType::TEAM)
{}

CArtifactInstance * CGameState::createScroll(const SpellID & spellId)
{
	return map->createScroll(spellId);
}

CArtifactInstance * CGameState::createArtifact(const ArtifactID & artID, const SpellID & spellId)
{
	return map->createArtifact(artID, spellId);
}

void CGameState::saveGame(CSaveFile & file) const
{
	ActiveModsInSaveList activeModsDummy;
	logGlobal->info("Saving game state");
	file.save(*getMapHeader());
	file.save(*getStartInfo());
	file.save(activeModsDummy);
	file.save(*this);
}

void CGameState::loadGame(CLoadFile & file)
{
	logGlobal->info("Loading game state...");

	CMapHeader dummyHeader;
	ActiveModsInSaveList dummyActiveMods;

	file.load(dummyHeader);
	if (file.hasFeature(ESerializationVersion::NO_RAW_POINTERS_IN_SERIALIZER))
	{
		StartInfo dummyStartInfo;
		file.load(dummyStartInfo);
		file.load(dummyActiveMods);
		file.load(*this);
	}
	else
	{
		auto dummyStartInfo = std::make_shared<StartInfo>();
		bool dummyA = false;
		uint32_t dummyB = 0;
		uint16_t dummyC = 0;
		file.load(dummyStartInfo);
		file.load(dummyActiveMods);
		file.load(dummyA);
		file.load(dummyB);
		file.load(dummyC);
		file.load(*this);
	}
}

#if SCRIPTING_ENABLED
scripting::Pool * CGameState::getGlobalContextPool() const
{
	return nullptr; // TODO
}
#endif

void CGameState::saveCompatibilityRegisterMissingArtifacts()
{
	for( const auto & newArtifact : saveCompatibilityUnregisteredArtifacts)
		map->saveCompatibilityAddMissingArtifact(newArtifact);
	saveCompatibilityUnregisteredArtifacts.clear();
}

VCMI_LIB_NAMESPACE_END
