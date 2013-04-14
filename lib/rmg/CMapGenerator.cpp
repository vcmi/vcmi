#include "StdInc.h"
#include "CMapGenerator.h"

#include "../mapping/CMap.h"
#include "../VCMI_Lib.h"
#include "../CGeneralTextHandler.h"
#include "../mapping/CMapEditManager.h"
#include "../CObjectHandler.h"
#include "../CDefObjInfoHandler.h"
#include "../GameConstants.h"
#include "../CTownHandler.h"
#include "../StringConstants.h"

CMapGenerator::CMapGenerator(const CMapGenOptions & mapGenOptions, int randomSeed) :
    mapGenOptions(mapGenOptions), randomSeed(randomSeed)
{
    gen.seed(randomSeed);
}

CMapGenerator::~CMapGenerator()
{

}

std::unique_ptr<CMap> CMapGenerator::generate()
{
    mapGenOptions.finalize(gen);

	//TODO select a template based on the map gen options or adapt it if necessary

	map = make_unique<CMap>();
	addHeaderInfo();

	terViewPatternConfig = make_unique<CTerrainViewPatternConfig>();
	mapMgr = make_unique<CMapEditManager>(terViewPatternConfig.get(), map.get(), randomSeed);
	genTerrain();
	genTowns();

	return std::move(map);
}

std::string CMapGenerator::getMapDescription() const
{
	const std::string waterContentStr[3] = { "none", "normal", "islands" };
	const std::string monsterStrengthStr[3] = { "weak", "normal", "strong" };

	std::stringstream ss;
	ss << "Map created by the Random Map Generator.\nTemplate was <MOCK>, ";
	ss << "Random seed was " << randomSeed << ", size " << map->width << "x";
	ss << map->height << ", levels " << (map->twoLevel ? "2" : "1") << ", ";
	ss << "humans " << static_cast<int>(mapGenOptions.getPlayersCnt()) << ", computers ";
	ss << static_cast<int>(mapGenOptions.getCompOnlyPlayersCnt()) << ", water " << waterContentStr[mapGenOptions.getWaterContent()];
	ss << ", monster " << monsterStrengthStr[mapGenOptions.getMonsterStrength()] << ", second expansion map";

    BOOST_FOREACH(const auto & pair, mapGenOptions.getPlayersSettings())
	{
        const auto & pSettings = pair.second;
        if(pSettings.getPlayerType() == EPlayerType::HUMAN)
		{
			ss << ", " << GameConstants::PLAYER_COLOR_NAMES[pSettings.getColor().getNum()] << " is human";
		}
        if(pSettings.getStartingTown() != CMapGenOptions::CPlayerSettings::RANDOM_TOWN)
		{
			ss << ", " << GameConstants::PLAYER_COLOR_NAMES[pSettings.getColor().getNum()]
				<< " town choice is " << ETownType::names[pSettings.getStartingTown()];
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
		int playersCnt = i == 0 ? mapGenOptions.getPlayersCnt() : mapGenOptions.getCompOnlyPlayersCnt();
		int teamsCnt = i == 0 ? mapGenOptions.getTeamsCnt() : mapGenOptions.getCompOnlyTeamsCnt();

		if(playersCnt == 0)
		{
			continue;
		}
		int playersPerTeam = playersCnt /
				(teamsCnt == 0 ? playersCnt : teamsCnt);
		int teamsCntNorm = teamsCnt;
		if(teamsCntNorm == 0)
		{
			teamsCntNorm = playersCnt;
		}
		for(int j = 0; j < teamsCntNorm; ++j)
		{
			for(int k = 0; k < playersPerTeam; ++k)
			{
				teamNumbers[i].push_back(j + teamOffset);
			}
		}
		for(int j = 0; j < playersCnt - teamsCntNorm * playersPerTeam; ++j)
		{
			teamNumbers[i].push_back(j + teamOffset);
		}
		teamOffset += teamsCntNorm;
	}

	// Team numbers are assigned randomly to every player
    BOOST_FOREACH(const auto & pair, mapGenOptions.getPlayersSettings())
	{
        const auto & pSettings = pair.second;
		PlayerInfo player;
		player.canComputerPlay = true;
        int j = pSettings.getPlayerType() == EPlayerType::COMP_ONLY ? 1 : 0;
		if(j == 0)
		{
			player.canHumanPlay = true;
		}
		auto itTeam = std::next(teamNumbers[j].begin(), gen.getInteger(0, teamNumbers[j].size() - 1));
		player.team = TeamID(*itTeam);
		teamNumbers[j].erase(itTeam);
		map->players[pSettings.getColor().getNum()] = player;
	}

	map->howManyTeams = (mapGenOptions.getTeamsCnt() == 0 ? mapGenOptions.getPlayersCnt() : mapGenOptions.getTeamsCnt())
			+ (mapGenOptions.getCompOnlyTeamsCnt() == 0 ? mapGenOptions.getCompOnlyPlayersCnt() : mapGenOptions.getCompOnlyTeamsCnt());
}

void CMapGenerator::genTerrain()
{
	map->initTerrain(); //FIXME nicer solution
	mapMgr->clearTerrain();
	mapMgr->drawTerrain(ETerrainType::GRASS, 10, 10, 20, 30, false);
}

void CMapGenerator::genTowns()
{
	//FIXME mock gen
	const int3 townPos[2] = { int3(17, 13, 0), int3(25,13, 0) };
    const int townTypes[2] = { ETownType::CASTLE, ETownType::DUNGEON };

    for(size_t i = 0; i < map->players.size(); ++i)
	{
        auto & playerInfo = map->players[i];
        if(!playerInfo.canAnyonePlay()) break;

        PlayerColor owner(i);
        int side = i % 2;
		CGTownInstance * town = new CGTownInstance();
		town->ID = Obj::TOWN;
		town->subID = townTypes[side];
		town->tempOwner = owner;
		town->defInfo = VLC->dobjinfo->gobjs[town->ID][town->subID];
		town->builtBuildings.insert(BuildingID::FORT);
		town->builtBuildings.insert(BuildingID::DEFAULT);
        mapMgr->insertObject(town, townPos[side].x, townPos[side].y + (i / 2) * 5, false);

		// Update player info
        playerInfo.allowedFactions.clear();
        playerInfo.allowedFactions.insert(townTypes[side]);
        playerInfo.hasMainTown = true;
        playerInfo.posOfMainTown = town->pos - int3(2, 0, 0);
        playerInfo.generateHeroAtMainTown = true;
	}
}

void CMapGenerator::addHeaderInfo()
{
	map->version = EMapFormat::SOD;
	map->width = mapGenOptions.getWidth();
	map->height = mapGenOptions.getHeight();
	map->twoLevel = mapGenOptions.getHasTwoLevels();
	map->name = VLC->generaltexth->allTexts[740];
	map->description = getMapDescription();
	map->difficulty = 1;
	addPlayerInfo();
}
