/*
 * CMapGenerator.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "CMapGenerator.h"

#include "../mapping/CMap.h"
#include "../VCMI_Lib.h"
#include "../CGeneralTextHandler.h"
#include "../mapping/CMapEditManager.h"
#include "../CTownHandler.h"
#include "../StringConstants.h"
#include "../filesystem/Filesystem.h"
#include "CZonePlacer.h"
#include "../mapObjects/CObjectClassesHandler.h"
#include "TileInfo.h"
#include "Zone.h"
#include "Functions.h"
#include "RmgMap.h"
#include "ObjectManager.h"
#include "TreasurePlacer.h"
#include "RoadPlacer.h"

CMapGenerator::CMapGenerator(CMapGenOptions& mapGenOptions, int RandomSeed) :
	mapGenOptions(mapGenOptions), randomSeed(RandomSeed),
	prisonsRemaining(0), monolithIndex(0)
{
	loadConfig();
	rand.setSeed(this->randomSeed);
	mapGenOptions.finalize(rand);
	map = std::make_unique<RmgMap>(mapGenOptions);
}

void CMapGenerator::loadConfig()
{
	static const std::map<std::string, Res::ERes> resMap
	{
		{"wood", Res::ERes::WOOD},
		{"ore", Res::ERes::ORE},
		{"gems", Res::ERes::GEMS},
		{"crystal", Res::ERes::CRYSTAL},
		{"mercury", Res::ERes::MERCURY},
		{"sulfur", Res::ERes::SULFUR},
		{"gold", Res::ERes::GOLD},
	};
	static const ResourceID path("config/randomMap.json");
	JsonNode randomMapJson(path);
	for(auto& s : randomMapJson["terrain"]["undergroundAllow"].Vector())
	{
		if(!s.isNull())
			config.terrainUndergroundAllowed.emplace_back(s.String());
	}
	for(auto& s : randomMapJson["terrain"]["groundProhibit"].Vector())
	{
		if(!s.isNull())
			config.terrainGroundProhibit.emplace_back(s.String());
	}
	config.shipyardGuard = randomMapJson["waterZone"]["shipyard"]["value"].Integer();
	for(auto & treasure : randomMapJson["waterZone"]["treasure"].Vector())
	{
		config.waterTreasure.emplace_back(treasure["min"].Integer(), treasure["max"].Integer(), treasure["density"].Integer());
	}
	for(auto& s : resMap)
	{
		config.mineValues[s.second] = randomMapJson["mines"]["value"][s.first].Integer();
	}
	config.mineExtraResources = randomMapJson["mines"]["extraResourcesLimit"].Integer();
	config.minGuardStrength = randomMapJson["minGuardStrength"].Integer();
	config.defaultRoadType = randomMapJson["defaultRoadType"].String();
	config.treasureValueLimit = randomMapJson["treasureValueLimit"].Integer();
	for(auto & i : randomMapJson["prisons"]["experience"].Vector())
		config.prisonExperience.push_back(i.Integer());
	for(auto & i : randomMapJson["prisons"]["value"].Vector())
		config.prisonValues.push_back(i.Integer());
	for(auto & i : randomMapJson["scrolls"]["value"].Vector())
		config.scrollValues.push_back(i.Integer());
	for(auto & i : randomMapJson["pandoras"]["creaturesValue"].Vector())
		config.pandoraCreatureValues.push_back(i.Integer());
	for(auto & i : randomMapJson["quests"]["value"].Vector())
		config.questValues.push_back(i.Integer());
	for(auto & i : randomMapJson["quests"]["rewardValue"].Vector())
		config.questRewardValues.push_back(i.Integer());
	config.pandoraMultiplierGold = randomMapJson["pandoras"]["valueMultiplierGold"].Integer();
	config.pandoraMultiplierExperience = randomMapJson["pandoras"]["valueMultiplierExperience"].Integer();
	config.pandoraMultiplierSpells = randomMapJson["pandoras"]["valueMultiplierSpells"].Integer();
	config.pandoraSpellSchool = randomMapJson["pandoras"]["valueSpellSchool"].Integer();
	config.pandoraSpell60 = randomMapJson["pandoras"]["valueSpell60"].Integer();
}

const CMapGenerator::Config & CMapGenerator::getConfig() const
{
	return config;
}

CMapGenerator::~CMapGenerator()
{
}

const CMapGenOptions& CMapGenerator::getMapGenOptions() const
{
	return mapGenOptions;
}

void CMapGenerator::initPrisonsRemaining()
{
	prisonsRemaining = 0;
	for (auto isAllowed : map->map().allowedHeroes)
	{
		if (isAllowed)
			prisonsRemaining++;
	}
	prisonsRemaining = std::max<int> (0, prisonsRemaining - 16 * mapGenOptions.getPlayerCount()); //so at least 16 heroes will be available for every player
}

void CMapGenerator::initQuestArtsRemaining()
{
	for (auto art : VLC->arth->objects)
	{
		if (art->aClass == CArtifact::ART_TREASURE && VLC->arth->legalArtifact(art->id) && art->constituentOf.empty()) //don't use parts of combined artifacts
			questArtifacts.push_back(art->id);
	}
}

std::unique_ptr<CMap> CMapGenerator::generate()
{
	try
	{
		addHeaderInfo();
		map->initTiles(*this);
		initPrisonsRemaining();
		initQuestArtsRemaining();
		genZones();
		map->map().calculateGuardingGreaturePositions(); //clear map so that all tiles are unguarded
		fillZones();
		//updated guarded tiles will be calculated in CGameState::initMapObjects()
		map->getZones().clear();
	}
	catch (rmgException &e)
	{
		logGlobal->error("Random map generation received exception: %s", e.what());
	}
	return std::move(map->mapInstance);
}

std::string CMapGenerator::getMapDescription() const
{
	assert(map);

	const std::string waterContentStr[3] = { "none", "normal", "islands" };
	const std::string monsterStrengthStr[3] = { "weak", "normal", "strong" };

	int monsterStrengthIndex = mapGenOptions.getMonsterStrength() - EMonsterStrength::GLOBAL_WEAK; //does not start from 0
	const auto * mapTemplate = mapGenOptions.getMapTemplate();

	if(!mapTemplate)
		throw rmgException("Map template for Random Map Generator is not found. Could not start the game.");

    std::stringstream ss;
    ss << boost::str(boost::format(std::string("Map created by the Random Map Generator.\nTemplate was %s, Random seed was %d, size %dx%d") +
        ", levels %s, players %d, computers %d, water %s, monster %s, VCMI map") % mapTemplate->getName() %
		randomSeed % map->map().width % map->map().height % (map->map().twoLevel ? "2" : "1") % static_cast<int>(mapGenOptions.getPlayerCount()) %
		static_cast<int>(mapGenOptions.getCompOnlyPlayerCount()) % waterContentStr[mapGenOptions.getWaterContent()] %
		monsterStrengthStr[monsterStrengthIndex]);

	for(const auto & pair : mapGenOptions.getPlayersSettings())
	{
		const auto & pSettings = pair.second;
		if(pSettings.getPlayerType() == EPlayerType::HUMAN)
		{
			ss << ", " << GameConstants::PLAYER_COLOR_NAMES[pSettings.getColor().getNum()] << " is human";
		}
		if(pSettings.getStartingTown() != CMapGenOptions::CPlayerSettings::RANDOM_TOWN)
		{
			ss << ", " << GameConstants::PLAYER_COLOR_NAMES[pSettings.getColor().getNum()]
			   << " town choice is " << (*VLC->townh)[pSettings.getStartingTown()]->name;
		}
	}

	return ss.str();
}

void CMapGenerator::addPlayerInfo()
{
	// Calculate which team numbers exist

	enum ETeams {CPHUMAN = 0, CPUONLY = 1, AFTER_LAST = 2};
	std::array<std::list<int>, 2> teamNumbers;

	int teamOffset = 0;
	int playerCount = 0;
	int teamCount = 0;

	for (int i = CPHUMAN; i < AFTER_LAST; ++i)
	{
		if (i == CPHUMAN)
		{
			playerCount = mapGenOptions.getPlayerCount();
			teamCount = mapGenOptions.getTeamCount();
		}
		else
		{
			playerCount = mapGenOptions.getCompOnlyPlayerCount();
			teamCount = mapGenOptions.getCompOnlyTeamCount();
		}

		if(playerCount == 0)
		{
			continue;
		}
		int playersPerTeam = playerCount / (teamCount == 0 ? playerCount : teamCount);
		int teamCountNorm = teamCount;
		if(teamCountNorm == 0)
		{
			teamCountNorm = playerCount;
		}
		for(int j = 0; j < teamCountNorm; ++j)
		{
			for(int k = 0; k < playersPerTeam; ++k)
			{
				teamNumbers[i].push_back(j + teamOffset);
			}
		}
		for(int j = 0; j < playerCount - teamCountNorm * playersPerTeam; ++j)
		{
			teamNumbers[i].push_back(j + teamOffset);
		}
		teamOffset += teamCountNorm;
	}

	// Team numbers are assigned randomly to every player
	//TODO: allow customize teams in rmg template
	for(const auto & pair : mapGenOptions.getPlayersSettings())
	{
		const auto & pSettings = pair.second;
		PlayerInfo player;
		player.canComputerPlay = true;
		int j = (pSettings.getPlayerType() == EPlayerType::COMP_ONLY) ? CPUONLY : CPHUMAN;
		if (j == CPHUMAN)
		{
			player.canHumanPlay = true;
		}

		if (teamNumbers[j].empty())
		{
			logGlobal->error("Not enough places in team for %s player", ((j == CPUONLY) ? "CPU" : "CPU or human"));
			assert (teamNumbers[j].size());
		}
		auto itTeam = RandomGeneratorUtil::nextItem(teamNumbers[j], rand);
		player.team = TeamID(*itTeam);
		teamNumbers[j].erase(itTeam);
		map->map().players[pSettings.getColor().getNum()] = player;
	}

	map->map().howManyTeams = (mapGenOptions.getTeamCount() == 0 ? mapGenOptions.getPlayerCount() : mapGenOptions.getTeamCount())
			+ (mapGenOptions.getCompOnlyTeamCount() == 0 ? mapGenOptions.getCompOnlyPlayerCount() : mapGenOptions.getCompOnlyTeamCount());
}

void CMapGenerator::genZones()
{
	CZonePlacer placer(*map);
	placer.placeZones(&rand);
	placer.assignZones(&rand);
	
	//add special zone for water
	/*zoneWater.first = map->getZones().size() + 1;
	zoneWater.second = std::make_shared<Zone>(this);
	{
		rmg::ZoneOptions options;
		options.setId(zoneWater.first);
		options.setType(ETemplateZoneType::WATER);
		zoneWater.second->setOptions(options);
	}*/

	logGlobal->info("Zones generated successfully");
}

/*void CMapGenerator::createWaterTreasures()
{
	//add treasures on water
	for(auto & treasureInfo : getConfig().waterTreasure)
	{
		getZoneWater().second->addTreasureInfo(treasureInfo);
	}
}

void CMapGenerator::prepareWaterTiles()
{
	for(auto & t : zoneWater.second->getTileInfo())
		if(shouldBeBlocked(t))
			setOccupied(t, ETileType::POSSIBLE);
}*/

void CMapGenerator::fillZones()
{
	findZonesForQuestArts();

	logGlobal->info("Started filling zones");

	//we need info about all town types to evaluate dwellings and pandoras with creatures properly
	//place main town in the middle
	
	for(auto it : map->getZones())
	{
		initTownType(*it.second, rand, *map, *it.second->getModificator<ObjectManager>());
		it.second->initFreeTiles();
		
		//it.second->initTownType();
	}
	
	//make sure there are some free tiles in the zone
	//for(auto it : zones)
	//	it.second->initFreeTiles();
	
	//for(auto it : zones)
	//	it.second->createBorder(); //once direct connections are done
	
#ifdef _BETA
	map->dump(false);
#endif
	
	//for(auto it : zones)
	//	it.second->createWater(getMapGenOptions().getWaterContent());
	
	//zoneWater.second->waterInitFreeTiles();

	//createDirectConnections(); //direct
	
#ifdef _BETA
	map->dump(false);
#endif
	//createConnections2(); //subterranean gates and monoliths
	
	//for(auto it : zones)
	//	zoneWater.second->waterConnection(*it.second);
	
	//createWaterTreasures();
	//zoneWater.second->initFreeTiles();
	//zoneWater.second->fill();

	std::vector<std::shared_ptr<Zone>> treasureZones;
	for(auto it : map->getZones())
	{
		//createBorder(*map, *it.second);
		processZone(*it.second, *this, *map);
		
		if (it.second->getType() == ETemplateZoneType::TREASURE)
			treasureZones.push_back(it.second);
	}
#ifdef _BETA
	map->dump(false);
#endif
		
	//set apriopriate free/occupied tiles, including blocked underground rock
	createObstaclesCommon1(*map, rand);
	for(auto it : map->getZones())
	{
		createObstacles1(*it.second, *map, rand);
	}
	
	//set back original terrain for underground zones
	//for(auto it : zones)
//		it.second->createObstacles1();
	
	createObstaclesCommon2(*map, rand);
	
#ifdef _BETA
	map->dump(false);
#endif
	
	//place actual obstacles matching zone terrain
	//for(auto it : zones)
	//	it.second->createObstacles2();
	
	for(auto it : map->getZones())
	{
		createObstacles2(*it.second, *map, rand, *it.second->getModificator<ObjectManager>());
	}
		
	//zoneWater.second->createObstacles2();
	
#ifdef _BETA
	map->dump(false);
#endif

	#define PRINT_MAP_BEFORE_ROADS false
	if (PRINT_MAP_BEFORE_ROADS) //enable to debug
	{
		std::ofstream out("road_debug.txt");
		int levels = map->map().twoLevel ? 2 : 1;
		int width = map->map().width;
		int height = map->map().height;
		for (int k = 0; k < levels; k++)
		{
			for (int j = 0; j<height; j++)
			{
				for (int i = 0; i<width; i++)
				{
					char t = '?';
					switch (map->getTile(int3(i, j, k)).getTileType())
					{
					case ETileType::FREE:
						t = ' '; break;
					case ETileType::BLOCKED:
						t = '#'; break;
					case ETileType::POSSIBLE:
						t = '-'; break;
					case ETileType::USED:
						t = 'O'; break;
					}

					out << t;
				}
				out << std::endl;
			}
			out << std::endl;
		}
		out << std::endl;
	}

	for(auto it : map->getZones())
	{
		it.second->getModificator<RoadPlacer>()->process();
		//it.second->connectRoads(); //draw roads after everything else has been placed
	}

	//find place for Grail
	if(treasureZones.empty())
	{
		for(auto it : map->getZones())
			treasureZones.push_back(it.second);
	}
	auto grailZone = *RandomGeneratorUtil::nextItem(treasureZones, rand);

	map->map().grailPos = *RandomGeneratorUtil::nextItem(grailZone->freePaths().getTiles(), rand);

	logGlobal->info("Zones filled successfully");
}

void CMapGenerator::findZonesForQuestArts()
{
	//we want to place arties in zones that were not yet filled (higher index)

	for (auto connection : mapGenOptions.getMapTemplate()->getConnections())
	{
		auto zoneA = map->getZones()[connection.getZoneA()];
		auto zoneB = map->getZones()[connection.getZoneB()];

		if (zoneA->getId() > zoneB->getId())
		{
			zoneB->getModificator<TreasurePlacer>()->setQuestArtZone(zoneA.get());
		}
		else if (zoneA->getId() < zoneB->getId())
		{
			zoneA->getModificator<TreasurePlacer>()->setQuestArtZone(zoneB.get());
		}
	}
}

void CMapGenerator::addHeaderInfo()
{
	map->map().version = EMapFormat::VCMI;
	map->map().width = mapGenOptions.getWidth();
	map->map().height = mapGenOptions.getHeight();
	map->map().twoLevel = mapGenOptions.getHasTwoLevels();
	map->map().name = VLC->generaltexth->allTexts[740];
	map->map().description = getMapDescription();
	map->map().difficulty = 1;
	addPlayerInfo();
}

int CMapGenerator::getNextMonlithIndex()
{
	if (monolithIndex >= VLC->objtypeh->knownSubObjects(Obj::MONOLITH_TWO_WAY).size())
		throw rmgException(boost::to_string(boost::format("There is no Monolith Two Way with index %d available!") % monolithIndex));
	else
		return monolithIndex++;
}

int CMapGenerator::getPrisonsRemaning() const
{
	return prisonsRemaining;
}

void CMapGenerator::decreasePrisonsRemaining()
{
	prisonsRemaining = std::max (0, prisonsRemaining - 1);
}

std::vector<ArtifactID> CMapGenerator::getQuestArtsRemaning() const
{
	return questArtifacts;
}

void CMapGenerator::banQuestArt(ArtifactID id)
{
	map->map().allowedArtifact[id] = false;
	vstd::erase_if_present(questArtifacts, id);
}

/*CMapGenerator::Zones::value_type CMapGenerator::getZoneWater() const
{
	return zoneWater;
}*/
