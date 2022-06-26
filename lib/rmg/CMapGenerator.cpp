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
		map->initTiles(rand);
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

void CMapGenerator::createDirectConnections()
{
	bool waterMode = getMapGenOptions().getWaterContent() != EWaterContent::NONE;
	
	for(auto connection : mapGenOptions.getMapTemplate()->getConnections())
	{
		auto zoneA = map->getZones()[connection.getZoneA()];
		auto zoneB = map->getZones()[connection.getZoneB()];

		//rearrange tiles in random order
		const auto & tiles = zoneA->area().getTiles();

		int3 guardPos(-1,-1,-1);

		int3 posA = zoneA->getPos();
		int3 posB = zoneB->getPos();
		// auto zoneAid = zoneA->getId();
		auto zoneBid = zoneB->getId();
		
		if(posA.z == posB.z)
		{
			std::vector<int3> middleTiles;
			for(const auto& tile : tiles)
			{
				if(map->isUsed(tile)/* || map->getZoneID(tile) == zoneWater.first*/) //tiles may be occupied by towns or water
					continue;
				map->foreachDirectNeighbour(tile, [tile, &middleTiles, this, zoneBid](int3 & pos) //must be direct since paths also also generated between direct neighbours
				{
					if(map->getZoneID(pos) == zoneBid)
						middleTiles.push_back(tile);
				});
			}

			//find tiles with minimum manhattan distance from center of the mass of zone border
			size_t tilesCount = middleTiles.size() ? middleTiles.size() : 1;
			int3 middleTile = std::accumulate(middleTiles.begin(), middleTiles.end(), int3(0, 0, 0));
			middleTile.x /= (si32)tilesCount;
			middleTile.y /= (si32)tilesCount;
			middleTile.z /= (si32)tilesCount; //TODO: implement division operator for int3?
			boost::sort(middleTiles, [middleTile](const int3 &lhs, const int3 &rhs) -> bool
			{
				//choose tiles with both corrdinates in the middle
				return lhs.mandist2d(middleTile) < rhs.mandist2d(middleTile);
			});

			//remove 1/4 tiles from each side - path should cross zone borders at smooth angle
			size_t removedCount = tilesCount / 4; //rounded down
			middleTiles.erase(middleTiles.end() - removedCount, middleTiles.end());
			middleTiles.erase(middleTiles.begin(), middleTiles.begin() + removedCount);

			RandomGeneratorUtil::randomShuffle(middleTiles, rand);
			for (auto tile : middleTiles)
			{
				guardPos = tile;
				if (guardPos.valid())
				{
					//zones can make paths only in their own area
					zoneA->connectPath(guardPos, true);
					zoneB->connectPath(guardPos, true);

					bool monsterPresent = zoneA->getModificator<ObjectManager>()->addMonster(guardPos, connection.getGuardStrength(), false, true);
					zoneB->getModificator<ObjectManager>()->updateDistances(guardPos); //place next objects away from guard in both zones

					//set free tile only after connection is made to the center of the zone
					if (!monsterPresent)
						map->setOccupied(guardPos, ETileType::FREE); //just in case monster is too weak to spawn

					zoneA->getModificator<RoadPlacer>()->addRoadNode(guardPos);
					zoneB->getModificator<RoadPlacer>()->addRoadNode(guardPos);
					break; //we're done with this connection
				}
			}
		}
		
		if (!guardPos.valid())
		{
			if(!waterMode || posA.z != posB.z/* || !zoneWater.second->waterKeepConnection(connection.getZoneA(), connection.getZoneB())*/)
				connectionsLeft.push_back(connection);
		}
	}
}

void CMapGenerator::createConnections2()
{
	for (auto & connection : connectionsLeft)
	{
		auto zoneA = map->getZones()[connection.getZoneA()];
		auto zoneB = map->getZones()[connection.getZoneB()];

		int3 guardPos(-1, -1, -1);

		int3 posA = zoneA->getPos();
		int3 posB = zoneB->getPos();

		auto strength = connection.getGuardStrength();

		if (posA.z != posB.z) //try to place subterranean gates
		{
			auto sgt = VLC->objtypeh->getHandlerFor(Obj::SUBTERRANEAN_GATE, 0)->getTemplates().front();
			auto tilesBlockedByObject = sgt.getBlockedOffsets();

			auto factory = VLC->objtypeh->getHandlerFor(Obj::SUBTERRANEAN_GATE, 0);
			auto gate1 = factory->create(ObjectTemplate());
			auto gate2 = factory->create(ObjectTemplate());

			while (!guardPos.valid())
			{
				bool continueOuterLoop = false;
				
				//find common tiles for both zones
				std::vector<int3> tilesA = zoneA->areaPossible().getTilesVector(),
					tilesB = zoneB->areaPossible().getTilesVector();

				std::vector<int3> commonTiles;

				auto lambda = [](const int3 & lhs, const int3 & rhs) -> bool
				{
					//https://stackoverflow.com/questions/45966807/c-invalid-comparator-assert
					return std::tie(lhs.x, lhs.y) < std::tie(rhs.x, rhs.y); //ignore z coordinate
				};
				//required for set_intersection
				boost::sort(tilesA, lambda);
				boost::sort(tilesB, lambda);

				boost::set_intersection(tilesA, tilesB, std::back_inserter(commonTiles), lambda);

				vstd::erase_if(commonTiles, [](const int3 &tile) -> bool
				{
					return (!tile.x) || (!tile.y); //gates shouldn't go outside map (x = 0) and look bad at the very top (y = 0)
				});

				if (commonTiles.empty())
					break; //nothing more to do

				boost::sort(commonTiles, [posA, posB](const int3 &lhs, const int3 &rhs) -> bool
				{
					//choose tiles which are equidistant to zone centers
					return (std::abs<double>(posA.dist2dSQ(lhs) - posB.dist2dSQ(lhs)) < std::abs<double>((posA.dist2dSQ(rhs) - posB.dist2dSQ(rhs))));
				});

				for (auto tile : commonTiles)
				{
					tile.z = posA.z;
					int3 otherTile = tile;
					otherTile.z = posB.z;

					float distanceFromA = static_cast<float>(posA.dist2d(tile));
					float distanceFromB = static_cast<float>(posB.dist2d(otherTile));

					if (distanceFromA > 5 && distanceFromB > 5)
					{
						if (zoneA->getModificator<ObjectManager>()->areAllTilesAvailable(gate1, tile, tilesBlockedByObject) &&
							zoneB->getModificator<ObjectManager>()->areAllTilesAvailable(gate2, otherTile, tilesBlockedByObject))
						{
							if (zoneA->getModificator<ObjectManager>()->getAccessibleOffset(sgt, tile).valid() && zoneB->getModificator<ObjectManager>()->getAccessibleOffset(sgt, otherTile).valid())
							{
								ObjectManager::EObjectPlacingResult result1 = zoneA->getModificator<ObjectManager>()->tryToPlaceObjectAndConnectToPath(gate1, tile);
								ObjectManager::EObjectPlacingResult result2 = zoneB->getModificator<ObjectManager>()->tryToPlaceObjectAndConnectToPath(gate2, otherTile);

								if ((result1 == ObjectManager::EObjectPlacingResult::SUCCESS) && (result2 == ObjectManager::EObjectPlacingResult::SUCCESS))
								{
									zoneA->getModificator<ObjectManager>()->placeObject(gate1, tile, true);
									zoneA->getModificator<ObjectManager>()->guardObject(gate1, strength, true, true);
									zoneB->getModificator<ObjectManager>()->placeObject(gate2, otherTile, true);
									zoneB->getModificator<ObjectManager>()->guardObject(gate2, strength, true, true);
									guardPos = tile; //set to break the loop
									break;
								}
								else if ((result1 == ObjectManager::EObjectPlacingResult::SEALED_OFF) || (result2 == ObjectManager::EObjectPlacingResult::SEALED_OFF))
								{
									//sealed-off tiles were blocked, exit inner loop and get another tile set
									continueOuterLoop = true;
									break;
								}
								else
									continue; //try with another position
							}
						}
					}
				}
				if (!continueOuterLoop) //we didn't find ANY tile - break outer loop
					break;
			}
			if (!guardPos.valid()) //cleanup? is this safe / enough?
			{
				delete gate1;
				delete gate2;
			}
		}
		if (!guardPos.valid())
		{
			auto factory = VLC->objtypeh->getHandlerFor(Obj::MONOLITH_TWO_WAY, getNextMonlithIndex());
			auto teleport1 = factory->create(ObjectTemplate());
			auto teleport2 = factory->create(ObjectTemplate());

			zoneA->getModificator<ObjectManager>()->addRequiredObject(teleport1, strength);
			zoneB->getModificator<ObjectManager>()->addRequiredObject(teleport2, strength);
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
