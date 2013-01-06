#include "StdInc.h"
#include "CMapGenerator.h"

#include "../Mapping/CMap.h"
#include "../VCMI_Lib.h"
#include "../CGeneralTextHandler.h"
#include "../Mapping/CMapEditManager.h"
#include "../CObjectHandler.h"
#include "../CDefObjInfoHandler.h"
#include "../GameConstants.h"
#include "../CTownHandler.h"
#include "../StringConstants.h"

CMapGenerator::CMapGenerator(const CMapGenOptions & mapGenOptions, const std::map<TPlayerColor, CPlayerSettings> & players, int randomSeed) :
	mapGenOptions(mapGenOptions), randomSeed(randomSeed), players(players)
{
	gen.seed(randomSeed);
	validateOptions();
}

CMapGenerator::~CMapGenerator()
{

}

std::unique_ptr<CMap> CMapGenerator::generate()
{
	finalizeMapGenOptions();

	//TODO select a template based on the map gen options or adapt it if necessary

	map = std::unique_ptr<CMap>(new CMap());
	addHeaderInfo();

	terViewPatternConfig = std::unique_ptr<CTerrainViewPatternConfig>(new CTerrainViewPatternConfig());
	mapMgr = std::unique_ptr<CMapEditManager>(new CMapEditManager(terViewPatternConfig.get(), map.get(), randomSeed));
	genTerrain();
	genTowns();

	return std::move(map);
}

void CMapGenerator::validateOptions() const
{
	int playersCnt = 0;
	int compOnlyPlayersCnt = 0;
	BOOST_FOREACH(const tPlayersMap::value_type & pair, players)
	{
		if(pair.second.getPlayerType() == CPlayerSettings::COMP_ONLY)
		{
			++compOnlyPlayersCnt;
		}
		else
		{
			++playersCnt;
		}
	}
	if(mapGenOptions.getPlayersCnt() == CMapGenOptions::RANDOM_SIZE)
	{
		if(playersCnt != GameConstants::PLAYER_LIMIT)
		{
			throw std::runtime_error(std::string("If the count of players is random size, ")
				+ "the count of the items in the players map should equal GameConstants::PLAYER_LIMIT.");
		}
		if(playersCnt == mapGenOptions.getPlayersCnt())
		{
			throw std::runtime_error(std::string("If the count of players is random size, ")
				+ "all items in the players map should be either of player type AI or HUMAN.");
		}
	}
	else
	{
		if(mapGenOptions.getCompOnlyPlayersCnt() != CMapGenOptions::RANDOM_SIZE)
		{
			if(playersCnt != mapGenOptions.getPlayersCnt() || compOnlyPlayersCnt != mapGenOptions.getCompOnlyPlayersCnt())
			{
				throw std::runtime_error(std::string("The count of players and computer only players in the players map ")
					+ "doesn't conform with the specified map gen options.");
			}
		}
		else
		{
			if(playersCnt != mapGenOptions.getPlayersCnt() || (playersCnt == mapGenOptions.getPlayersCnt()
				&& compOnlyPlayersCnt != GameConstants::PLAYER_LIMIT - playersCnt))
			{
				throw std::runtime_error(std::string("If the count of players is fixed and the count of comp only players random, ")
					+ "the items in the players map should equal GameConstants::PLAYER_LIMIT.");
			}
		}
	}

	if(countHumanPlayers() < 1)
	{
		throw std::runtime_error("1 human player is required at least");
	}

	BOOST_FOREACH(const tPlayersMap::value_type & pair, players)
	{
		if(pair.first != pair.second.getColor())
		{
			throw std::runtime_error("The color of an item in player settings and the key of it has to be the same.");
		}
	}
}

void CMapGenerator::finalizeMapGenOptions()
{
	if(mapGenOptions.getPlayersCnt() == CMapGenOptions::RANDOM_SIZE)
	{
		mapGenOptions.setPlayersCnt(gen.getInteger(countHumanPlayers(), GameConstants::PLAYER_LIMIT));

		// Remove AI players only from the end of the players map if necessary
		for(auto itrev = players.end(); itrev != players.begin();)
		{
			auto it = itrev;
			--it;
			if(players.size() == mapGenOptions.getPlayersCnt())
			{
				break;
			}
			const CPlayerSettings & pSettings = it->second;
			if(pSettings.getPlayerType() == CPlayerSettings::AI)
			{
				players.erase(it);
			}
			else
			{
				--itrev;
			}
		}
	}
	if(mapGenOptions.getTeamsCnt() == CMapGenOptions::RANDOM_SIZE)
	{
		mapGenOptions.setTeamsCnt(gen.getInteger(0, mapGenOptions.getPlayersCnt() - 1));
	}
	if(mapGenOptions.getCompOnlyPlayersCnt() == CMapGenOptions::RANDOM_SIZE)
	{
		mapGenOptions.setCompOnlyPlayersCnt(gen.getInteger(0, 8 - mapGenOptions.getPlayersCnt()));
		int totalPlayersCnt = mapGenOptions.getPlayersCnt() + mapGenOptions.getCompOnlyPlayersCnt();

		// Remove comp only players only from the end of the players map if necessary
		for(auto itrev = players.end(); itrev != players.begin();)
		{
			auto it = itrev;
			--it;
			if(players.size() <= totalPlayersCnt)
			{
				break;
			}
			const CPlayerSettings & pSettings = it->second;
			if(pSettings.getPlayerType() == CPlayerSettings::COMP_ONLY)
			{
				players.erase(it);
			}
			else
			{
				--itrev;
			}
		}

		// Add some comp only players if necessary
		int compOnlyPlayersToAdd = totalPlayersCnt - players.size();
		for(int i = 0; i < compOnlyPlayersToAdd; ++i)
		{
			CPlayerSettings pSettings;
			pSettings.setPlayerType(CPlayerSettings::COMP_ONLY);
			pSettings.setColor(getNextPlayerColor());
			players[pSettings.getColor()] = pSettings;
		}
	}
	if(mapGenOptions.getCompOnlyTeamsCnt() == CMapGenOptions::RANDOM_SIZE)
	{
		mapGenOptions.setCompOnlyTeamsCnt(gen.getInteger(0, std::max(mapGenOptions.getCompOnlyPlayersCnt() - 1, 0)));
	}

	// There should be at least 2 players (1-player-maps aren't allowed)
	if(mapGenOptions.getPlayersCnt() + mapGenOptions.getCompOnlyPlayersCnt() < 2)
	{
		CPlayerSettings pSettings;
		pSettings.setPlayerType(CPlayerSettings::AI);
		pSettings.setColor(getNextPlayerColor());
		players[pSettings.getColor()] = pSettings;
		mapGenOptions.setPlayersCnt(2);
	}

	// 1 team isn't allowed
	if(mapGenOptions.getTeamsCnt() == 1 && mapGenOptions.getCompOnlyPlayersCnt() == 0)
	{
		mapGenOptions.setTeamsCnt(0);
	}

	if(mapGenOptions.getWaterContent() == EWaterContent::RANDOM)
	{
		mapGenOptions.setWaterContent(static_cast<EWaterContent::EWaterContent>(gen.getInteger(0, 2)));
	}
	if(mapGenOptions.getMonsterStrength() == EMonsterStrength::RANDOM)
	{
		mapGenOptions.setMonsterStrength(static_cast<EMonsterStrength::EMonsterStrength>(gen.getInteger(0, 2)));
	}
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

	BOOST_FOREACH(const tPlayersMap::value_type & pair, players)
	{
		const CPlayerSettings & pSettings = pair.second;
		if(pSettings.getPlayerType() == CPlayerSettings::HUMAN)
		{
			ss << ", " << GameConstants::PLAYER_COLOR_NAMES[pSettings.getColor()] << " is human";
		}
		if(pSettings.getStartingTown() != CPlayerSettings::RANDOM_TOWN)
		{
			ss << ", " << GameConstants::PLAYER_COLOR_NAMES[pSettings.getColor()]
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
	BOOST_FOREACH(const tPlayersMap::value_type & pair, players)
	{
		const CPlayerSettings & pSettings = pair.second;
		PlayerInfo player;
		player.canComputerPlay = true;
		int j = pSettings.getPlayerType() == CPlayerSettings::COMP_ONLY ? 1 : 0;
		if(j == 0)
		{
			player.canHumanPlay = true;
		}
		auto itTeam = std::next(teamNumbers[j].begin(), gen.getInteger(0, teamNumbers[j].size() - 1));
		player.team = *itTeam;
		teamNumbers[j].erase(itTeam);
		map->players[pSettings.getColor()] = player;
	}

	map->howManyTeams = (mapGenOptions.getTeamsCnt() == 0 ? mapGenOptions.getPlayersCnt() : mapGenOptions.getTeamsCnt())
			+ (mapGenOptions.getCompOnlyTeamsCnt() == 0 ? mapGenOptions.getCompOnlyPlayersCnt() : mapGenOptions.getCompOnlyTeamsCnt());
}

int CMapGenerator::countHumanPlayers() const
{
	return static_cast<int>(std::count_if(players.begin(), players.end(), [](const std::pair<TPlayerColor, CPlayerSettings> & pair)
	{
		return pair.second.getPlayerType() == CPlayerSettings::HUMAN;
	}));
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
	const TFaction townTypes[2] = { ETownType::CASTLE, ETownType::DUNGEON };

	for(auto it = players.begin(); it != players.end(); ++it)
	{
		TPlayerColor owner = it->first;
		int pos = std::distance(players.begin(), it);
		int side = pos % 2;
		CGTownInstance * town = new CGTownInstance();
		town->ID = Obj::TOWN;
		town->subID = townTypes[side];
		town->tempOwner = owner;
		town->defInfo = VLC->dobjinfo->gobjs[town->ID][town->subID];
		town->builtBuildings.insert(EBuilding::FORT);
		town->builtBuildings.insert(-50);
		mapMgr->insertObject(town, townPos[side].x, townPos[side].y + (pos / 2) * 5, false);
		map->players[owner].allowedFactions.clear();
		map->players[owner].allowedFactions.insert(townTypes[side]);
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

TPlayerColor CMapGenerator::getNextPlayerColor() const
{
	for(TPlayerColor i = 0; i < GameConstants::PLAYER_LIMIT; ++i)
	{
		if(players.find(i) == players.end())
		{
			return i;
		}
	}
	throw std::runtime_error("Shouldn't happen. No free player color exists.");
}

CMapGenerator::CPlayerSettings::CPlayerSettings() : color(0), startingTown(RANDOM_TOWN), playerType(AI)
{

}

int CMapGenerator::CPlayerSettings::getColor() const
{
	return color;
}


void CMapGenerator::CPlayerSettings::setColor(int value)
{
	if(value >= 0 && value < GameConstants::PLAYER_LIMIT)
	{
		color = value;
	}
	else
	{
		throw std::runtime_error("The color of the player is not in a valid range.");
	}
}

int CMapGenerator::CPlayerSettings::getStartingTown() const
{
	return startingTown;
}

void CMapGenerator::CPlayerSettings::setStartingTown(int value)
{
	if(value >= -1 && value < static_cast<int>(VLC->townh->towns.size()))
	{
		startingTown = value;
	}
	else
	{
		throw std::runtime_error("The starting town of the player is not in a valid range.");
	}
}

CMapGenerator::CPlayerSettings::EPlayerType CMapGenerator::CPlayerSettings::getPlayerType() const
{
	return playerType;
}

void CMapGenerator::CPlayerSettings::setPlayerType(EPlayerType value)
{
	playerType = value;
}
