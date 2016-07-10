#include "StdInc.h"
#include "CMapGenerator.h"

#include "../mapping/CMap.h"
#include "../VCMI_Lib.h"
#include "../CGeneralTextHandler.h"
#include "../mapping/CMapEditManager.h"
#include "../CTownHandler.h"
#include "../StringConstants.h"
#include "../filesystem/Filesystem.h"
#include "CRmgTemplate.h"
#include "CRmgTemplateZone.h"
#include "CZonePlacer.h"
#include "../mapObjects/CObjectClassesHandler.h"

static const int3 dirs4[] = {int3(0,1,0),int3(0,-1,0),int3(-1,0,0),int3(+1,0,0)};

void CMapGenerator::foreach_neighbour(const int3 &pos, std::function<void(int3& pos)> foo)
{
	for(const int3 &dir : int3::getDirs())
	{
		int3 n = pos + dir;
		/*important notice: perform any translation before this function is called,
		so the actual map position is checked*/
		if(map->isInTheMap(n))
			foo(n);
	}
}

void CMapGenerator::foreachDirectNeighbour(const int3& pos, std::function<void(int3& pos)> foo)
{
	for(const int3 &dir : dirs4)
	{
		int3 n = pos + dir;
		if(map->isInTheMap(n))
			foo(n);
	}
}


CMapGenerator::CMapGenerator() :
    zonesTotal(0), monolithIndex(0)
{
}

void CMapGenerator::initTiles()
{
	map->initTerrain();

	int width = map->width;
	int height = map->height;

	int level = map->twoLevel ? 2 : 1;
	tiles = new CTileInfo**[width];
	for (int i = 0; i < width; ++i)
	{
		tiles[i] = new CTileInfo*[height];
		for (int j = 0; j < height; ++j)
		{
			tiles[i][j] = new CTileInfo[level];
		}
	}
}

CMapGenerator::~CMapGenerator()
{
	if (tiles)
	{
		int width = mapGenOptions->getWidth();
		int height = mapGenOptions->getHeight();
		for (int i=0; i < width; i++)
		{
			for(int j=0; j < height; j++)
			{
				delete [] tiles[i][j];
			}
			delete [] tiles[i];
		}
		delete [] tiles;
	}
}

void CMapGenerator::initPrisonsRemaining()
{
	prisonsRemaining = 0;
	for (auto isAllowed : map->allowedHeroes)
	{
		if (isAllowed)
			prisonsRemaining++;
	}
	prisonsRemaining = std::max<int> (0, prisonsRemaining - 16 * mapGenOptions->getPlayerCount()); //so at least 16 heroes will be available for every player
}

void CMapGenerator::initQuestArtsRemaining()
{
	for (auto art : VLC->arth->artifacts)
	{
		if (art->aClass == CArtifact::ART_TREASURE && VLC->arth->legalArtifact(art->id) && art->constituentOf.empty()) //don't use parts of combined artifacts
			questArtifacts.push_back(art->id);
	}
}

std::unique_ptr<CMap> CMapGenerator::generate(CMapGenOptions * mapGenOptions, int randomSeed /*= std::time(nullptr)*/)
{
	this->mapGenOptions = mapGenOptions;
	this->randomSeed = randomSeed;

	assert(mapGenOptions);

	rand.setSeed(this->randomSeed);
	mapGenOptions->finalize(rand);

	map = make_unique<CMap>();
	editManager = map->getEditManager();

	try
	{
		editManager->getUndoManager().setUndoRedoLimit(0);
		//FIXME:  somehow mapGenOption is nullptr at this point :?
		addHeaderInfo();
		initTiles();

		initPrisonsRemaining();
		initQuestArtsRemaining();
		genZones();
		map->calculateGuardingGreaturePositions(); //clear map so that all tiles are unguarded
		fillZones();
		//updated guarded tiles will be calculated in CGameState::initMapObjects()
	}
	catch (rmgException &e)
	{
		logGlobal->errorStream() << "Random map generation received exception: " << e.what();
	}
	return std::move(map);
}

std::string CMapGenerator::getMapDescription() const
{
	assert(mapGenOptions);
	assert(map);

	const std::string waterContentStr[3] = { "none", "normal", "islands" };
	const std::string monsterStrengthStr[3] = { "weak", "normal", "strong" };

	int monsterStrengthIndex = mapGenOptions->getMonsterStrength() - EMonsterStrength::GLOBAL_WEAK; //does not start from 0

    std::stringstream ss;
    ss << boost::str(boost::format(std::string("Map created by the Random Map Generator.\nTemplate was %s, Random seed was %d, size %dx%d") +
        ", levels %s, players %d, computers %d, water %s, monster %s, VCMI map") % mapGenOptions->getMapTemplate()->getName() %
		randomSeed % map->width % map->height % (map->twoLevel ? "2" : "1") % static_cast<int>(mapGenOptions->getPlayerCount()) %
		static_cast<int>(mapGenOptions->getCompOnlyPlayerCount()) % waterContentStr[mapGenOptions->getWaterContent()] %
		monsterStrengthStr[monsterStrengthIndex]);

	for(const auto & pair : mapGenOptions->getPlayersSettings())
	{
		const auto & pSettings = pair.second;
		if(pSettings.getPlayerType() == EPlayerType::HUMAN)
		{
			ss << ", " << GameConstants::PLAYER_COLOR_NAMES[pSettings.getColor().getNum()] << " is human";
		}
		if(pSettings.getStartingTown() != CMapGenOptions::CPlayerSettings::RANDOM_TOWN)
		{
			ss << ", " << GameConstants::PLAYER_COLOR_NAMES[pSettings.getColor().getNum()]
			   << " town choice is " << VLC->townh->factions[pSettings.getStartingTown()]->name;
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
			playerCount = mapGenOptions->getPlayerCount();
			teamCount = mapGenOptions->getTeamCount();
		}
		else
		{
			playerCount = mapGenOptions->getCompOnlyPlayerCount();
			teamCount = mapGenOptions->getCompOnlyTeamCount();
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
	for(const auto & pair : mapGenOptions->getPlayersSettings())
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
			logGlobal->errorStream() << boost::format("Not enough places in team for %s player") % ((j == CPUONLY) ? "CPU" : "CPU or human");
			assert (teamNumbers[j].size());
		}
        auto itTeam = RandomGeneratorUtil::nextItem(teamNumbers[j], rand);
		player.team = TeamID(*itTeam);
		teamNumbers[j].erase(itTeam);
		map->players[pSettings.getColor().getNum()] = player;
	}

	map->howManyTeams = (mapGenOptions->getTeamCount() == 0 ? mapGenOptions->getPlayerCount() : mapGenOptions->getTeamCount())
			+ (mapGenOptions->getCompOnlyTeamCount() == 0 ? mapGenOptions->getCompOnlyPlayerCount() : mapGenOptions->getCompOnlyTeamCount());
}

void CMapGenerator::genZones()
{
	editManager->clearTerrain(&rand);
	editManager->getTerrainSelection().selectRange(MapRect(int3(0, 0, 0), mapGenOptions->getWidth(), mapGenOptions->getHeight()));
	editManager->drawTerrain(ETerrainType::GRASS, &rand);

	auto tmpl = mapGenOptions->getMapTemplate();
	zones = tmpl->getZones(); //copy from template (refactor?)

	CZonePlacer placer(this);
	placer.placeZones(mapGenOptions, &rand);
	placer.assignZones(mapGenOptions);

	logGlobal->infoStream() << "Zones generated successfully";
}

void CMapGenerator::fillZones()
{
	//init native town count with 0
	for (auto faction : VLC->townh->getAllowedFactions())
		zonesPerFaction[faction] = 0;

	logGlobal->infoStream() << "Started filling zones";

	//initialize possible tiles before any object is actually placed
	for (auto it : zones)
	{
		it.second->initFreeTiles(this);
	}

	findZonesForQuestArts();
	createConnections();
	//make sure all connections are passable before creating borders
	for (auto it : zones)
	{
		it.second->createBorder(this);
		 //we need info about all town types to evaluate dwellings and pandoras with creatures properly
		it.second->initTownType(this);
	}
	std::vector<CRmgTemplateZone*> treasureZones;
	for (auto it : zones)
	{
		it.second->fill(this);
		if (it.second->getType() == ETemplateZoneType::TREASURE)
			treasureZones.push_back(it.second);
	}

	//set apriopriate free/occupied tiles, including blocked underground rock
	createObstaclesCommon1();
	//set back original terrain for underground zones
	for (auto it : zones)
		it.second->createObstacles1(this);
	createObstaclesCommon2();
	//place actual obstacles matching zone terrain
	for (auto it : zones)
	{
		it.second->createObstacles2(this);
	}

	#define PRINT_MAP_BEFORE_ROADS true
	if (PRINT_MAP_BEFORE_ROADS) //enable to debug
	{
		std::ofstream out("road debug");
		int levels = map->twoLevel ? 2 : 1;
		int width = map->width;
		int height = map->height;
		for (int k = 0; k < levels; k++)
		{
			for (int j = 0; j<height; j++)
			{
				for (int i = 0; i<width; i++)
				{
					char t = '?';
					switch (getTile(int3(i, j, k)).getTileType())
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

	for (auto it : zones)
	{
		it.second->connectRoads(this); //draw roads after everything else has been placed
	}

	//find place for Grail
	if (treasureZones.empty())
	{
		for (auto it : zones)
			treasureZones.push_back(it.second);
	}
	auto grailZone = *RandomGeneratorUtil::nextItem(treasureZones, rand);

	map->grailPos = *RandomGeneratorUtil::nextItem(*grailZone->getFreePaths(), rand);

	logGlobal->infoStream() << "Zones filled successfully";
}

void CMapGenerator::createObstaclesCommon1()
{
	if (map->twoLevel) //underground
	{
		//negative approach - create rock tiles first, then make sure all accessible tiles have no rock
		std::vector<int3> rockTiles;

		for (int x = 0; x < map->width; x++)
		{
			for (int y = 0; y < map->height; y++)
			{
				int3 tile(x, y, 1);
				if (shouldBeBlocked(tile))
				{
					rockTiles.push_back(tile);
				}
			}
		}
		editManager->getTerrainSelection().setSelection(rockTiles);
		editManager->drawTerrain(ETerrainType::ROCK, &rand);
	}
}

void CMapGenerator::createObstaclesCommon2()
{
	if (map->twoLevel)
	{
		//finally mark rock tiles as occupied, spawn no obstacles there
		for (int x = 0; x < map->width; x++)
		{
			for (int y = 0; y < map->height; y++)
			{
				int3 tile(x, y, 1);
				if (map->getTile(tile).terType == ETerrainType::ROCK)
				{
					setOccupied(tile, ETileType::USED);
				}
			}
		}
	}

	//tighten obstacles to improve visuals

	for (int i = 0; i < 3; ++i)
	{
		int blockedTiles = 0;
		int freeTiles = 0;

		for (int z = 0; z < (map->twoLevel ? 2 : 1); z++)
		{
			for (int x = 0; x < map->width; x++)
			{
				for (int y = 0; y < map->height; y++)
				{
					int3 tile(x, y, z);
					if (!isPossible(tile)) //only possible tiles can change
						continue;

					int blockedNeighbours = 0;
					int freeNeighbours = 0;
					foreach_neighbour(tile, [this, &blockedNeighbours, &freeNeighbours](int3 &pos)
					{
						if (this->isBlocked(pos))
							blockedNeighbours++;
						if (this->isFree(pos))
							freeNeighbours++;
					});
					if (blockedNeighbours > 4)
					{
						setOccupied(tile, ETileType::BLOCKED);
						blockedTiles++;
					}
					else if (freeNeighbours > 4)
					{
						setOccupied(tile, ETileType::FREE);
						freeTiles++;
					}
				}
			}
		}
		logGlobal->traceStream() << boost::format("Set %d tiles to BLOCKED and %d tiles to FREE") % blockedTiles % freeTiles;
	}
}

void CMapGenerator::findZonesForQuestArts()
{
	//we want to place arties in zones that were not yet filled (higher index)

	for (auto connection : mapGenOptions->getMapTemplate()->getConnections())
	{
		auto zoneA = connection.getZoneA();
		auto zoneB = connection.getZoneB();

		if (zoneA->getId() > zoneB->getId())
		{
			zoneB->setQuestArtZone(zoneA);
		}
		else if (zoneA->getId() < zoneB->getId())
		{
			zoneA->setQuestArtZone(zoneB);
		}
	}
}

void CMapGenerator::createConnections()
{
	for (auto connection : mapGenOptions->getMapTemplate()->getConnections())
	{
		auto zoneA = connection.getZoneA();
		auto zoneB = connection.getZoneB();

		//rearrange tiles in random order
		auto tilesCopy = zoneA->getTileInfo();
		std::vector<int3> tiles(tilesCopy.begin(), tilesCopy.end());
		RandomGeneratorUtil::randomShuffle(tiles, rand);

		int3 guardPos(-1,-1,-1);

		auto otherZoneTiles = zoneB->getTileInfo();

		int3 posA = zoneA->getPos();
		int3 posB = zoneB->getPos();

		if (posA.z == posB.z)
		{
			for (auto tile : tiles)
			{
				if (isBlocked(tile)) //tiles may be occupied by subterranean gates already placed
					continue;
				foreachDirectNeighbour (tile, [&guardPos, tile, &otherZoneTiles, this](int3 &pos) //must be direct since paths also also generated between direct neighbours
				{
					//if (vstd::contains(otherZoneTiles, pos) && !this->isBlocked(pos))
					if (vstd::contains(otherZoneTiles, pos))
						guardPos = tile;
				});
				if (guardPos.valid())
				{
					setOccupied (guardPos, ETileType::FREE); //just in case monster is too weak to spawn
					zoneA->addMonster (this, guardPos, connection.getGuardStrength(), false, true);
					//zones can make paths only in their own area
					zoneA->crunchPath(this, guardPos, posA, true, zoneA->getFreePaths()); //make connection towards our zone center
					zoneB->crunchPath(this, guardPos, posB, true, zoneB->getFreePaths()); //make connection towards other zone center

					zoneA->addRoadNode(guardPos);
					zoneB->addRoadNode(guardPos);
					break; //we're done with this connection
				}
			}
		}
		else //create subterranean gates between two zones
		{
			//find common tiles for both zones

			std::vector<int3> commonTiles;
			auto tileSetA = zoneA->getTileInfo(),
				tileSetB = zoneB->getTileInfo();
			std::vector<int3> tilesA (tileSetA.begin(), tileSetA.end()),
				tilesB (tileSetB.begin(), tileSetB.end());
			boost::sort(tilesA),
			boost::sort(tilesB);

			boost::set_intersection(tilesA, tilesB, std::back_inserter(commonTiles), [](const int3 &lhs, const int3 &rhs) -> bool
			{
				//ignore z coordinate
				if (lhs.x < rhs.x)
					return true;
				else
					return lhs.y < rhs.y;
			});

			boost::sort(commonTiles, [posA, posB](const int3 &lhs, const int3 &rhs) -> bool
			{
				//choose tiles which are equidistant to zone centers
				return (std::abs<double>(posA.dist2dSQ(lhs) - posB.dist2dSQ(lhs)) < std::abs<double>((posA.dist2dSQ(rhs) - posB.dist2dSQ(rhs))));
			});

			auto sgt = VLC->objtypeh->getHandlerFor(Obj::SUBTERRANEAN_GATE, 0)->getTemplates().front();

			bool stop = false;
			for (auto tile : commonTiles)
			{
				tile.z = posA.z;
				int3 otherTile = tile;
				otherTile.z = posB.z;

				float distanceFromA = posA.dist2d(tile);
				float distanceFromB = posB.dist2d(otherTile);

				//if zone is underground, gate must fit within its (reduced) radius
				if (distanceFromA > 5 && (!posA.z || distanceFromA < zoneA->getSize() - 3) &&
					distanceFromB > 5 && (!posB.z || distanceFromB < zoneB->getSize() - 3))
				{
					//all neightbouring tiles also belong to zone
					if (vstd::contains(tiles, tile) && vstd::contains(otherZoneTiles, otherTile))
					{
						bool withinZone = true;

						foreach_neighbour (tile, [&withinZone, &tiles](int3 &pos)
						{
							if (!vstd::contains(tiles, pos))
								withinZone = false;
						});
						foreach_neighbour (otherTile, [&withinZone, &otherZoneTiles](int3 &pos)
						{
							if (!vstd::contains(otherZoneTiles, pos))
								withinZone = false;
						});

						//make sure both gates has some free tiles below them
						if (zoneA->getAccessibleOffset(this, sgt, tile).valid() && zoneB->getAccessibleOffset(this, sgt, otherTile).valid())
						{
							if (withinZone)
							{
								zoneA->placeSubterraneanGate(this, tile, connection.getGuardStrength());
								zoneB->placeSubterraneanGate(this, otherTile, connection.getGuardStrength());
								guardPos = tile; //just any valid value
								break; //we're done
							}
						}
					}
				}
			}
		}
		if (!guardPos.valid())
		{
			auto factory = VLC->objtypeh->getHandlerFor(Obj::MONOLITH_TWO_WAY, getNextMonlithIndex());
			auto teleport1 = factory->create(ObjectTemplate());

			auto teleport2 = factory->create(ObjectTemplate());

			zoneA->addRequiredObject (teleport1, connection.getGuardStrength());
			zoneB->addRequiredObject (teleport2, connection.getGuardStrength());
		}
	}
}

void CMapGenerator::addHeaderInfo()
{
	map->version = EMapFormat::VCMI;
	map->width = mapGenOptions->getWidth();
	map->height = mapGenOptions->getHeight();
	map->twoLevel = mapGenOptions->getHasTwoLevels();
	map->name = VLC->generaltexth->allTexts[740];
	map->description = getMapDescription();
	map->difficulty = 1;
	addPlayerInfo();
}

void CMapGenerator::checkIsOnMap(const int3& tile) const
{
	if (!map->isInTheMap(tile))
		throw  rmgException(boost::to_string(boost::format("Tile %s is outside the map") % tile));
}


std::map<TRmgTemplateZoneId, CRmgTemplateZone*> CMapGenerator::getZones() const
{
	return zones;
}

bool CMapGenerator::isBlocked(const int3 &tile) const
{
	checkIsOnMap(tile);

	return tiles[tile.x][tile.y][tile.z].isBlocked();
}
bool CMapGenerator::shouldBeBlocked(const int3 &tile) const
{
	checkIsOnMap(tile);

	return tiles[tile.x][tile.y][tile.z].shouldBeBlocked();
}
bool CMapGenerator::isPossible(const int3 &tile) const
{
	checkIsOnMap(tile);

	return tiles[tile.x][tile.y][tile.z].isPossible();
}
bool CMapGenerator::isFree(const int3 &tile) const
{
	checkIsOnMap(tile);

	return tiles[tile.x][tile.y][tile.z].isFree();
}
bool CMapGenerator::isUsed(const int3 &tile) const
{
	checkIsOnMap(tile);

	return tiles[tile.x][tile.y][tile.z].isUsed();
}

bool CMapGenerator::isRoad(const int3& tile) const
{
	checkIsOnMap(tile);

	return tiles[tile.x][tile.y][tile.z].isRoad();
}

void CMapGenerator::setOccupied(const int3 &tile, ETileType::ETileType state)
{
	checkIsOnMap(tile);

	tiles[tile.x][tile.y][tile.z].setOccupied(state);
}

void CMapGenerator::setRoad(const int3& tile, ERoadType::ERoadType roadType)
{
	checkIsOnMap(tile);

	tiles[tile.x][tile.y][tile.z].setRoadType(roadType);
}


CTileInfo CMapGenerator::getTile(const int3& tile) const
{
	checkIsOnMap(tile);

	return tiles[tile.x][tile.y][tile.z];
}

bool CMapGenerator::isAllowedSpell(SpellID sid) const
{
	assert(sid >= 0);
	if (sid < map->allowedSpell.size())
	{
		return map->allowedSpell[sid];
	}
	else
		return false;
}

void CMapGenerator::setNearestObjectDistance(int3 &tile, float value)
{
	checkIsOnMap(tile);

	tiles[tile.x][tile.y][tile.z].setNearestObjectDistance(value);
}

float CMapGenerator::getNearestObjectDistance(const int3 &tile) const
{
	checkIsOnMap(tile);

	return tiles[tile.x][tile.y][tile.z].getNearestObjectDistance();
}

int CMapGenerator::getNextMonlithIndex()
{
	if (monolithIndex >= VLC->objtypeh->knownSubObjects(Obj::MONOLITH_TWO_WAY).size())
	{
		//logGlobal->errorStream() << boost::to_string(boost::format("RMG Error! There is no Monolith Two Way with index %d available!") % monolithIndex);
		//monolithIndex++;
		//return VLC->objtypeh->knownSubObjects(Obj::MONOLITH_TWO_WAY).size() - 1;
		//TODO: interrupt map generation and report error
		throw rmgException(boost::to_string(boost::format("There is no Monolith Two Way with index %d available!") % monolithIndex));
	}
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
	map->allowedArtifact[id] = false;
	vstd::erase_if_present (questArtifacts, id);
}

void CMapGenerator::registerZone (TFaction faction)
{
	zonesPerFaction[faction]++;
	zonesTotal++;
}
ui32 CMapGenerator::getZoneCount(TFaction faction)
{
	return zonesPerFaction[faction];
}
ui32 CMapGenerator::getTotalZoneCount() const
{
	return zonesTotal;
}
