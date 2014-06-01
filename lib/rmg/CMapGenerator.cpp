#include "StdInc.h"
#include "CMapGenerator.h"

#include "../mapping/CMap.h"
#include "../VCMI_Lib.h"
#include "../CGeneralTextHandler.h"
#include "../mapping/CMapEditManager.h"
#include "../CObjectHandler.h"
#include "../CDefObjInfoHandler.h"
#include "../CTownHandler.h"
#include "../StringConstants.h"
#include "../filesystem/Filesystem.h"
#include "CRmgTemplate.h"
#include "CRmgTemplateZone.h"
#include "CZonePlacer.h"

void CMapGenerator::foreach_neighbour(const int3 &pos, std::function<void(int3& pos)> foo)
{
	for(const int3 &dir : dirs)
	{
		int3 n = pos + dir;
		if(map->isInTheMap(n))
			foo(pos+dir);
	}
}


CMapGenerator::CMapGenerator(shared_ptr<CMapGenOptions> mapGenOptions, int randomSeed /*= std::time(nullptr)*/) :
	mapGenOptions(mapGenOptions), randomSeed(randomSeed), monolithIndex(0)
{
	rand.setSeed(randomSeed);
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
	//FIXME: what if map is not present anymore?
	if (tiles && map)
	{
		for (int i=0; i < map->width; i++)
		{
			for(int j=0; j < map->height; j++)
			{
				delete [] tiles[i][j];
			}
			delete [] tiles[i];
		}
		delete [] tiles;
	}
}

std::unique_ptr<CMap> CMapGenerator::generate()
{
	mapGenOptions->finalize(rand);

	map = make_unique<CMap>();
	editManager = map->getEditManager();
	try
	{
		editManager->getUndoManager().setUndoRedoLimit(0);
		addHeaderInfo();
		initTiles();

		genZones();
		map->calculateGuardingGreaturePositions(); //clear map so that all tiles are unguarded
		fillZones();
	}
	catch (rmgException &e)
	{
		logGlobal->errorStream() << "Random map generation received exception: " << e.what();
	}
	return std::move(map);
}

std::string CMapGenerator::getMapDescription() const
{
	const std::string waterContentStr[3] = { "none", "normal", "islands" };
	const std::string monsterStrengthStr[3] = { "weak", "normal", "strong" };

    std::stringstream ss;
    ss << boost::str(boost::format(std::string("Map created by the Random Map Generator.\nTemplate was %s, Random seed was %d, size %dx%d") +
        ", levels %s, humans %d, computers %d, water %s, monster %s, second expansion map") % mapGenOptions->getMapTemplate()->getName() %
		randomSeed % map->width % map->height % (map->twoLevel ? "2" : "1") % static_cast<int>(mapGenOptions->getPlayerCount()) %
		static_cast<int>(mapGenOptions->getCompOnlyPlayerCount()) % waterContentStr[mapGenOptions->getWaterContent()] %
        monsterStrengthStr[mapGenOptions->getMonsterStrength()]);

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

	int player_per_side = zones.size() > 4 ? 3 : 2;
	int zones_cnt = zones.size() > 4 ? 9 : 4;
		
	logGlobal->infoStream() << boost::format("Map size %d %d, players per side %d") % w % h % player_per_side;

	CZonePlacer placer(this);
	placer.placeZones(mapGenOptions, &rand);
	placer.assignZones(mapGenOptions);

	int i = 0;
	int part_w = w/player_per_side;
	int part_h = h/player_per_side;


	for(auto const it : zones)
	{
		CRmgTemplateZone * zone = it.second;
		zone->setType(i < pcnt ? ETemplateZoneType::PLAYER_START : ETemplateZoneType::TREASURE);
		this->zones[it.first] = zone;
		++i;
	}
	logGlobal->infoStream() << "Zones generated successfully";
}

void CMapGenerator::fillZones()
{	
	logGlobal->infoStream() << "Started filling zones";

	createConnections();
	for (auto it : zones)
	{
		//make sure all connections are passable before creating borders
		it.second->createBorder(this);
		it.second->fill(this);
	}	
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
		//TODO: hwo to use std::shuffle with our generator?
		//std::random_shuffle (tiles.begin(), tiles.end(), &gen->rand.nextInt);

		int i, n;
		n = (tiles.end() - tiles.begin());
		for (i=n-1; i>0; --i)
		{
			std::swap (tiles.begin()[i],tiles.begin()[rand.nextInt(i+1)]);
		}

		int3 guardPos(-1,-1,-1);

		auto otherZoneTiles = zoneB->getTileInfo();
		auto otherZoneCenter = zoneB->getPos();

		for (auto tile : tiles)
		{
			foreach_neighbour (tile, [&guardPos, tile, &otherZoneTiles](int3 &pos)
			{
				if (vstd::contains(otherZoneTiles, pos))
					guardPos = tile;
			});
			if (guardPos.valid())
			{
				zoneA->addMonster (this, guardPos, connection.getGuardStrength()); //TODO: set value according to template
				//zones can make paths only in their own area
				zoneA->crunchPath (this, guardPos, zoneA->getPos(), zoneA->getId()); //make connection towards our zone center
				zoneB->crunchPath (this, guardPos, zoneB->getPos(), zoneB->getId()); //make connection towards other zone center
				break; //we're done with this connection
			}
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
		throw  rmgException(boost::to_string(boost::format("Tile %s is outside the map") % tile));

	return tiles[tile.x][tile.y][tile.z].shouldBeBlocked();
}
bool CMapGenerator::isPossible(const int3 &tile) const
{
	if (!map->isInTheMap(tile))
		throw  rmgException(boost::to_string(boost::format("Tile %s is outside the map") % tile));

	return tiles[tile.x][tile.y][tile.z].isPossible();
}
bool CMapGenerator::isFree(const int3 &tile) const
{
	if (!map->isInTheMap(tile))
		throw  rmgException(boost::to_string(boost::format("Tile %s is outside the map") % tile));

	return tiles[tile.x][tile.y][tile.z].isFree();
}
void CMapGenerator::setOccupied(int3 &tile, ETileType::ETileType state)
{
	if (!map->isInTheMap(tile))
		throw  rmgException(boost::to_string(boost::format("Tile %s is outside the map") % tile));

	tiles[tile.x][tile.y][tile.z].setOccupied(state);
}

CTileInfo CMapGenerator::getTile(int3 tile) const
{
	if (!map->isInTheMap(tile))
		throw  rmgException(boost::to_string(boost::format("Tile %s is outside the map") % tile));

	return tiles[tile.x][tile.y][tile.z];
}

void CMapGenerator::setNearestObjectDistance(int3 &tile, int value)
{
	if (!map->isInTheMap(tile))
		throw  rmgException(boost::to_string(boost::format("Tile %s is outside the map") % tile));

	tiles[tile.x][tile.y][tile.z].setNearestObjectDistance(value);
}

int CMapGenerator::getNearestObjectDistance(const int3 &tile) const
{
	if (!map->isInTheMap(tile))
		throw  rmgException(boost::to_string(boost::format("Tile %s is outside the map") % tile));

	return tiles[tile.x][tile.y][tile.z].getNearestObjectDistance();
}

int CMapGenerator::getNextMonlithIndex()
{
	return monolithIndex++;
}