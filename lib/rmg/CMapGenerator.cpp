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

void CMapGenerator::foreach_neighbour(const int3 &pos, std::function<void(int3& pos)> foo)
{
	for(const int3 &dir : dirs)
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
	prisonsRemaining = std::max<int> (0, prisonsRemaining - 16 * map->players.size()); //so at least 16 heroes will be available for every player
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
        ", levels %s, humans %d, computers %d, water %s, monster %s, second expansion map") % mapGenOptions->getMapTemplate()->getName() %
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
	std::array<std::list<int>, 2> teamNumbers; // 0= cpu/human, 1= cpu only
	int teamOffset = 0;
	for(int i = 0; i < 2; ++i)
	{
		int playerCount = i == 0 ? mapGenOptions->getPlayerCount() : mapGenOptions->getCompOnlyPlayerCount();
		int teamCount = i == 0 ? mapGenOptions->getTeamCount() : mapGenOptions->getCompOnlyTeamCount();

		if(playerCount == 0)
		{
			continue;
		}
		int playersPerTeam = playerCount /
				(teamCount == 0 ? playerCount : teamCount);
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
	for(const auto & pair : mapGenOptions->getPlayersSettings())
	{
		const auto & pSettings = pair.second;
		PlayerInfo player;
		player.canComputerPlay = true;
		int j = pSettings.getPlayerType() == EPlayerType::COMP_ONLY ? 1 : 0;
		if(j == 0)
		{
			player.canHumanPlay = true;
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

	auto pcnt = mapGenOptions->getPlayerCount();
	auto w = mapGenOptions->getWidth();
	auto h = mapGenOptions->getHeight();

	auto tmpl = mapGenOptions->getMapTemplate();
	zones = tmpl->getZones(); //copy from template (refactor?)

	CZonePlacer placer(this);
	placer.placeZones(mapGenOptions, &rand);
	placer.assignZones(mapGenOptions);

	int i = 0;

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
				foreach_neighbour (tile, [&guardPos, tile, &otherZoneTiles, this](int3 &pos)
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
					zoneA->crunchPath (this, guardPos, posA, zoneA->getId(), zoneA->getFreePaths()); //make connection towards our zone center
					zoneB->crunchPath (this, guardPos, posB, zoneB->getId(), zoneB->getFreePaths()); //make connection towards other zone center
					break; //we're done with this connection
				}
			}
		}
		else //create subterranean gates between two zones
		{	
			//find point on the path between zones
			float3 offset (posB.x - posA.x, posB.y - posA.y, 0);

			float distance = posB.dist2d(posA);
			vstd::amax (distance, 0.5f);
			offset /= distance; //get unit vector
			float3 vec (0, 0, 0);
			//use reduced size of underground zone - make sure gate does not stand on rock
			int3 tile = posA;
			int3 otherTile = tile;

			bool stop = false;
			while (!stop)
			{
				vec += offset; //this vector may extend beyond line between zone centers, in case they are directly over each other
				tile = posA + int3(vec.x, vec.y, 0);
				float distanceFromA = posA.dist2d(tile);
				float distanceFromB = posB.dist2d(tile);

				if (distanceFromA + distanceFromB > std::max<int>(zoneA->getSize() + zoneB->getSize(), distance))
					break; //we are too far away to ever connect

				//if zone is underground, gate must fit within its (reduced) radius
				if (distanceFromA > 5 && (!posA.z || distanceFromA < zoneA->getSize() - 3) &&
					distanceFromB > 5 && (!posB.z || distanceFromB < zoneB->getSize() - 3))
				{
					otherTile = tile;
					otherTile.z = posB.z;

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

						if (withinZone)
						{
							auto gate1 = new CGTeleport;
							gate1->ID = Obj::SUBTERRANEAN_GATE;
							gate1->subID = 0;
							zoneA->placeAndGuardObject(this, gate1, tile, connection.getGuardStrength());
							auto gate2 = new CGTeleport(*gate1);
							zoneB->placeAndGuardObject(this, gate2, otherTile, connection.getGuardStrength());

							stop = true; //we are done, go to next connection
						}
					}
				}
			}
			if (stop)
				continue;
		}
		if (!guardPos.valid())
		{
			auto teleport1 = new CGTeleport;
			teleport1->ID = Obj::MONOLITH_TWO_WAY;
			teleport1->subID = getNextMonlithIndex();

			auto teleport2 = new CGTeleport(*teleport1);

			zoneA->addRequiredObject (teleport1, connection.getGuardStrength());
			zoneB->addRequiredObject (teleport2, connection.getGuardStrength());
		}		
	}
}

void CMapGenerator::addHeaderInfo()
{
	map->version = EMapFormat::SOD;
	map->width = mapGenOptions->getWidth();
	map->height = mapGenOptions->getHeight();
	map->twoLevel = mapGenOptions->getHasTwoLevels();
	map->name = VLC->generaltexth->allTexts[740];
	map->description = getMapDescription();
	map->difficulty = 1;
	addPlayerInfo();
}

std::map<TRmgTemplateZoneId, CRmgTemplateZone*> CMapGenerator::getZones() const
{
	return zones;
}

bool CMapGenerator::isBlocked(const int3 &tile) const
{
	if (!map->isInTheMap(tile))
		throw  rmgException(boost::to_string(boost::format("Tile %s is outside the map") % tile));

	return tiles[tile.x][tile.y][tile.z].isBlocked();
}
bool CMapGenerator::shouldBeBlocked(const int3 &tile) const
{
	if (!map->isInTheMap(tile))
		throw rmgException(boost::to_string(boost::format("Tile %s is outside the map") % tile));

	return tiles[tile.x][tile.y][tile.z].shouldBeBlocked();
}
bool CMapGenerator::isPossible(const int3 &tile) const
{
	if (!map->isInTheMap(tile))
		throw rmgException(boost::to_string(boost::format("Tile %s is outside the map") % tile));

	return tiles[tile.x][tile.y][tile.z].isPossible();
}
bool CMapGenerator::isFree(const int3 &tile) const
{
	if (!map->isInTheMap(tile))
		throw rmgException(boost::to_string(boost::format("Tile %s is outside the map") % tile));

	return tiles[tile.x][tile.y][tile.z].isFree();
}
bool CMapGenerator::isUsed(const int3 &tile) const
{
	if (!map->isInTheMap(tile))
		throw  rmgException(boost::to_string(boost::format("Tile %s is outside the map") % tile));

	return tiles[tile.x][tile.y][tile.z].isUsed();
}
void CMapGenerator::setOccupied(const int3 &tile, ETileType::ETileType state)
{
	if (!map->isInTheMap(tile))
		throw rmgException(boost::to_string(boost::format("Tile %s is outside the map") % tile));

	tiles[tile.x][tile.y][tile.z].setOccupied(state);
}

CTileInfo CMapGenerator::getTile(const int3&  tile) const
{
	if (!map->isInTheMap(tile))
		throw rmgException(boost::to_string(boost::format("Tile %s is outside the map") % tile));

	return tiles[tile.x][tile.y][tile.z];
}

void CMapGenerator::setNearestObjectDistance(int3 &tile, float value)
{
	if (!map->isInTheMap(tile))
		throw rmgException(boost::to_string(boost::format("Tile %s is outside the map") % tile));

	tiles[tile.x][tile.y][tile.z].setNearestObjectDistance(value);
}

float CMapGenerator::getNearestObjectDistance(const int3 &tile) const
{
	if (!map->isInTheMap(tile))
		throw rmgException(boost::to_string(boost::format("Tile %s is outside the map") % tile));

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
