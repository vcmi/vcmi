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

CMapGenOptions::CMapGenOptions() : width(72), height(72), hasTwoLevels(true),
	playersCnt(RANDOM_SIZE), teamsCnt(RANDOM_SIZE), compOnlyPlayersCnt(0), compOnlyTeamsCnt(RANDOM_SIZE),
	waterContent(EWaterContent::RANDOM), monsterStrength(EMonsterStrength::RANDOM)
{
	resetPlayersMap();
}

si32 CMapGenOptions::getWidth() const
{
	return width;
}

void CMapGenOptions::setWidth(si32 value)
{
	if(value > 0)
	{
		width = value;
	}
	else
	{
		throw std::runtime_error("A map width lower than 1 is not allowed.");
	}
}

si32 CMapGenOptions::getHeight() const
{
	return height;
}

void CMapGenOptions::setHeight(si32 value)
{
	if(value > 0)
	{
		height = value;
	}
	else
	{
		throw std::runtime_error("A map height lower than 1 is not allowed.");
	}
}

bool CMapGenOptions::getHasTwoLevels() const
{
	return hasTwoLevels;
}

void CMapGenOptions::setHasTwoLevels(bool value)
{
	hasTwoLevels = value;
}

si8 CMapGenOptions::getPlayersCnt() const
{
	return playersCnt;
}

void CMapGenOptions::setPlayersCnt(si8 value)
{
	if((value >= 1 && value <= PlayerColor::PLAYER_LIMIT_I) || value == RANDOM_SIZE)
	{
		playersCnt = value;
		resetPlayersMap();
	}
	else
	{
		throw std::runtime_error("Players count of RMG options should be between 1 and " +
								 std::to_string(PlayerColor::PLAYER_LIMIT_I) + " or CMapGenOptions::RANDOM_SIZE for random.");
	}
}

si8 CMapGenOptions::getTeamsCnt() const
{
	return teamsCnt;
}

void CMapGenOptions::setTeamsCnt(si8 value)
{
	if(playersCnt == RANDOM_SIZE || (value >= 0 && value < playersCnt) || value == RANDOM_SIZE)
	{
		teamsCnt = value;
	}
	else
	{
		throw std::runtime_error("Teams count of RMG options should be between 0 and <" +
								 std::to_string(playersCnt) + "(players count) - 1> or CMapGenOptions::RANDOM_SIZE for random.");
	}
}

si8 CMapGenOptions::getCompOnlyPlayersCnt() const
{
	return compOnlyPlayersCnt;
}

void CMapGenOptions::setCompOnlyPlayersCnt(si8 value)
{
	if(value == RANDOM_SIZE || (value >= 0 && value <= PlayerColor::PLAYER_LIMIT_I - playersCnt))
	{
		compOnlyPlayersCnt = value;
		resetPlayersMap();
	}
	else
	{
		throw std::runtime_error(std::string("Computer only players count of RMG options should be ") +
								 "between 0 and <PlayerColor::PLAYER_LIMIT - " + std::to_string(playersCnt) +
								 "(playersCount)> or CMapGenOptions::RANDOM_SIZE for random.");
	}
}

si8 CMapGenOptions::getCompOnlyTeamsCnt() const
{
	return compOnlyTeamsCnt;
}

void CMapGenOptions::setCompOnlyTeamsCnt(si8 value)
{
	if(value == RANDOM_SIZE || compOnlyPlayersCnt == RANDOM_SIZE || (value >= 0 && value <= std::max(compOnlyPlayersCnt - 1, 0)))
	{
		compOnlyTeamsCnt = value;
	}
	else
	{
		throw std::runtime_error(std::string("Computer only teams count of RMG options should be ") +
								 "between 0 and <" + std::to_string(compOnlyPlayersCnt) +
								 "(compOnlyPlayersCnt) - 1> or CMapGenOptions::RANDOM_SIZE for random.");
	}
}

EWaterContent::EWaterContent CMapGenOptions::getWaterContent() const
{
	return waterContent;
}

void CMapGenOptions::setWaterContent(EWaterContent::EWaterContent value)
{
	waterContent = value;
}

EMonsterStrength::EMonsterStrength CMapGenOptions::getMonsterStrength() const
{
	return monsterStrength;
}

void CMapGenOptions::setMonsterStrength(EMonsterStrength::EMonsterStrength value)
{
	monsterStrength = value;
}

void CMapGenOptions::resetPlayersMap()
{
	players.clear();
	int realPlayersCnt = playersCnt == RANDOM_SIZE ? PlayerColor::PLAYER_LIMIT_I : playersCnt;
	int realCompOnlyPlayersCnt = compOnlyPlayersCnt == RANDOM_SIZE ? (PlayerColor::PLAYER_LIMIT_I - realPlayersCnt) : compOnlyPlayersCnt;
	for(int color = 0; color < (realPlayersCnt + realCompOnlyPlayersCnt); ++color)
	{
		CPlayerSettings player;
		player.setColor(PlayerColor(color));
		player.setPlayerType((color >= realPlayersCnt) ? EPlayerType::COMP_ONLY : EPlayerType::AI);
		players[PlayerColor(color)] = player;
	}
}

const std::map<PlayerColor, CMapGenOptions::CPlayerSettings> & CMapGenOptions::getPlayersSettings() const
{
	return players;
}

void CMapGenOptions::setStartingTownForPlayer(PlayerColor color, si32 town)
{
	auto it = players.find(color);
	if(it == players.end()) throw std::runtime_error(boost::str(boost::format("Cannot set starting town for the player with the color '%s'.") % color));
	it->second.setStartingTown(town);
}

void CMapGenOptions::setPlayerTypeForStandardPlayer(PlayerColor color, EPlayerType::EPlayerType playerType)
{
	auto it = players.find(color);
	if(it == players.end()) throw std::runtime_error(boost::str(boost::format("Cannot set player type for the player with the color '%s'.") % color));
	if(playerType == EPlayerType::COMP_ONLY) throw std::runtime_error("Cannot set player type computer only to a standard player.");
	it->second.setPlayerType(playerType);
}

void CMapGenOptions::finalize()
{
	CRandomGenerator gen;
	finalize(gen);
}

void CMapGenOptions::finalize(CRandomGenerator & gen)
{
	if(playersCnt == RANDOM_SIZE)
	{
		// 1 human is required at least
		auto humanPlayers = countHumanPlayers();
		if(humanPlayers == 0) humanPlayers = 1;
		playersCnt = gen.getInteger(humanPlayers, PlayerColor::PLAYER_LIMIT_I);

		// Remove AI players only from the end of the players map if necessary
		for(auto itrev = players.end(); itrev != players.begin();)
		{
			auto it = itrev;
			--it;
			if(players.size() == playersCnt) break;
			if(it->second.getPlayerType() == EPlayerType::AI)
			{
				players.erase(it);
			}
			else
			{
				--itrev;
			}
		}
	}
	if(teamsCnt == RANDOM_SIZE)
	{
		teamsCnt = gen.getInteger(0, playersCnt - 1);
	}
	if(compOnlyPlayersCnt == RANDOM_SIZE)
	{
		compOnlyPlayersCnt = gen.getInteger(0, 8 - playersCnt);
		auto totalPlayersCnt = playersCnt + compOnlyPlayersCnt;

		// Remove comp only players only from the end of the players map if necessary
		for(auto itrev = players.end(); itrev != players.begin();)
		{
			auto it = itrev;
			--it;
			if(players.size() <= totalPlayersCnt) break;
			if(it->second.getPlayerType() == EPlayerType::COMP_ONLY)
			{
				players.erase(it);
			}
			else
			{
				--itrev;
			}
		}

		// Add some comp only players if necessary
		auto compOnlyPlayersToAdd = totalPlayersCnt - players.size();
		for(int i = 0; i < compOnlyPlayersToAdd; ++i)
		{
			CPlayerSettings pSettings;
			pSettings.setPlayerType(EPlayerType::COMP_ONLY);
			pSettings.setColor(getNextPlayerColor());
			players[pSettings.getColor()] = pSettings;
		}
	}
	if(compOnlyTeamsCnt == RANDOM_SIZE)
	{
		compOnlyTeamsCnt = gen.getInteger(0, std::max(compOnlyPlayersCnt - 1, 0));
	}

	// There should be at least 2 players (1-player-maps aren't allowed)
	if(playersCnt + compOnlyPlayersCnt < 2)
	{
		CPlayerSettings pSettings;
		pSettings.setPlayerType(EPlayerType::AI);
		pSettings.setColor(getNextPlayerColor());
		players[pSettings.getColor()] = pSettings;
		playersCnt = 2;
	}

	// 1 team isn't allowed
	if(teamsCnt == 1 && compOnlyPlayersCnt == 0)
	{
		teamsCnt = 0;
	}

	if(waterContent == EWaterContent::RANDOM)
	{
		waterContent = static_cast<EWaterContent::EWaterContent>(gen.getInteger(0, 2));
	}
	if(monsterStrength == EMonsterStrength::RANDOM)
	{
		monsterStrength = static_cast<EMonsterStrength::EMonsterStrength>(gen.getInteger(0, 2));
	}
}

int CMapGenOptions::countHumanPlayers() const
{
	return static_cast<int>(boost::count_if(players, [](const std::pair<PlayerColor, CPlayerSettings> & pair)
	{
		return pair.second.getPlayerType() == EPlayerType::HUMAN;
	}));
}

PlayerColor CMapGenOptions::getNextPlayerColor() const
{
	for(PlayerColor i = PlayerColor(0); i < PlayerColor::PLAYER_LIMIT; i.advance(1))
	{
		if(!players.count(i))
		{
			return i;
		}
	}
	throw std::runtime_error("Shouldn't happen. No free player color exists.");
}

CMapGenOptions::CPlayerSettings::CPlayerSettings() : color(0), startingTown(RANDOM_TOWN), playerType(EPlayerType::AI)
{

}

PlayerColor CMapGenOptions::CPlayerSettings::getColor() const
{
	return color;
}

void CMapGenOptions::CPlayerSettings::setColor(PlayerColor value)
{
	if(value >= PlayerColor(0) && value < PlayerColor::PLAYER_LIMIT)
	{
		color = value;
	}
	else
	{
		throw std::runtime_error("The color of the player is not in a valid range.");
	}
}

si32 CMapGenOptions::CPlayerSettings::getStartingTown() const
{
	return startingTown;
}

void CMapGenOptions::CPlayerSettings::setStartingTown(si32 value)
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

EPlayerType::EPlayerType CMapGenOptions::CPlayerSettings::getPlayerType() const
{
	return playerType;
}

void CMapGenOptions::CPlayerSettings::setPlayerType(EPlayerType::EPlayerType value)
{
	playerType = value;
}

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
	editManager = map->getEditManager();
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
	editManager->clearTerrain(&gen);
	editManager->drawTerrain(MapRect(int3(10, 10, 0), 20, 30), ETerrainType::GRASS, &gen);
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
		editManager->insertObject(int3(townPos[side].x, townPos[side].y + (i / 2) * 5, 0), town);

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
