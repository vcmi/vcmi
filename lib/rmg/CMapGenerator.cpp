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

CMapGenerator::CMapGenerator(shared_ptr<CMapGenOptions> mapGenOptions, int randomSeed /*= std::time(nullptr)*/) :
	mapGenOptions(mapGenOptions), randomSeed(randomSeed)
{
	rand.setSeed(randomSeed);
}

CMapGenerator::~CMapGenerator()
{

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
		auto itTeam = std::next(teamNumbers[j].begin(), rand.nextInt (teamNumbers[j].size()));
		player.team = TeamID(*itTeam);
		teamNumbers[j].erase(itTeam);
		map->players[pSettings.getColor().getNum()] = player;
	}

	map->howManyTeams = (mapGenOptions->getTeamCount() == 0 ? mapGenOptions->getPlayerCount() : mapGenOptions->getTeamCount())
			+ (mapGenOptions->getCompOnlyTeamCount() == 0 ? mapGenOptions->getCompOnlyPlayerCount() : mapGenOptions->getCompOnlyTeamCount());
}

void CMapGenerator::genZones()
{
	map->initTerrain();
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
		std::vector<int3> shape;
		int left = part_w*(i%player_per_side);
		int top = part_h*(i/player_per_side);
		shape.push_back(int3(left, top,  0));
		shape.push_back(int3(left + part_w, top,  0));
		shape.push_back(int3(left + part_w, top + part_h,  0));
		shape.push_back(int3(left, top + part_h,  0));
		zone->setShape(shape);
		zone->setType(i < pcnt ? ETemplateZoneType::PLAYER_START : ETemplateZoneType::TREASURE);
		this->zones[it.first] = zone;
		++i;
	}
	logGlobal->infoStream() << "Zones generated successfully";
}

void CMapGenerator::fillZones()
{	
	logGlobal->infoStream() << "Started filling zones";
	for(auto it : zones)
	{
		it.second->fill(this);
	}	
	logGlobal->infoStream() << "Zones filled successfully";
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