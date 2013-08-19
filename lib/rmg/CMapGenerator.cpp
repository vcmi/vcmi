
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
#include "../CObjectHandler.h"
#include "../CDefObjInfoHandler.h"
#include "../CTownHandler.h"
#include "../StringConstants.h"
#include "CRmgTemplate.h"

CMapGenerator::CMapGenerator() : mapGenOptions(nullptr), randomSeed(0)
{

}

CMapGenerator::~CMapGenerator()
{

}

std::unique_ptr<CMap> CMapGenerator::generate(CMapGenOptions * mapGenOptions, int randomSeed /*= std::time(nullptr)*/)
{
	this->randomSeed = randomSeed;
	gen.seed(this->randomSeed);
	this->mapGenOptions = mapGenOptions;
	this->mapGenOptions->finalize(gen);

	map = make_unique<CMap>();
	editManager = map->getEditManager();
	editManager->getUndoManager().setUndoRedoLimit(0);
	addHeaderInfo();

	genTerrain();
	genTowns();

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
		auto itTeam = std::next(teamNumbers[j].begin(), gen.getInteger(0, teamNumbers[j].size() - 1));
		player.team = TeamID(*itTeam);
		teamNumbers[j].erase(itTeam);
		map->players[pSettings.getColor().getNum()] = player;
	}

	map->howManyTeams = (mapGenOptions->getTeamCount() == 0 ? mapGenOptions->getPlayerCount() : mapGenOptions->getTeamCount())
			+ (mapGenOptions->getCompOnlyTeamCount() == 0 ? mapGenOptions->getCompOnlyPlayerCount() : mapGenOptions->getCompOnlyTeamCount());
}

void CMapGenerator::genTerrain()
{
	map->initTerrain();
	editManager->clearTerrain(&gen);
    editManager->getTerrainSelection().selectRange(MapRect(int3(4, 4, 0), 24, 30));
	editManager->drawTerrain(ETerrainType::GRASS, &gen);
}

void CMapGenerator::genTowns()
{
	//FIXME mock gen
	const int3 townPos[2] = { int3(11, 7, 0), int3(19,7, 0) };

	for(size_t i = 0; i < map->players.size(); ++i)
	{
		auto & playerInfo = map->players[i];
		if(!playerInfo.canAnyonePlay()) break;

		PlayerColor owner(i);
		int side = i % 2;
		auto  town = new CGTownInstance();
		town->ID = Obj::TOWN;
		int townId = mapGenOptions->getPlayersSettings().find(PlayerColor(i))->second.getStartingTown();
		if(townId == CMapGenOptions::CPlayerSettings::RANDOM_TOWN) townId = gen.getInteger(0, 8); // Default towns
		town->subID = townId;
		town->tempOwner = owner;
		town->defInfo = VLC->dobjinfo->gobjs[town->ID][town->subID];
		town->builtBuildings.insert(BuildingID::FORT);
		town->builtBuildings.insert(BuildingID::DEFAULT);
		editManager->insertObject(town, int3(townPos[side].x, townPos[side].y + (i / 2) * 5, 0));

		// Update player info
		playerInfo.allowedFactions.clear();
		playerInfo.allowedFactions.insert(townId);
		playerInfo.hasMainTown = true;
		playerInfo.posOfMainTown = town->pos - int3(2, 0, 0);
		playerInfo.generateHeroAtMainTown = true;
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
