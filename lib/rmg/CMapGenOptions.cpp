/*
 * CMapGenOptions.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "CMapGenOptions.h"

#include "../mapping/CMapHeader.h"
#include "CRmgTemplateStorage.h"
#include "CRmgTemplate.h"
#include "CRandomGenerator.h"
#include "../VCMI_Lib.h"
#include "../CTownHandler.h"

VCMI_LIB_NAMESPACE_BEGIN

CMapGenOptions::CMapGenOptions()
	: width(CMapHeader::MAP_SIZE_MIDDLE), height(CMapHeader::MAP_SIZE_MIDDLE), hasTwoLevels(true),
	playerCount(RANDOM_SIZE), teamCount(RANDOM_SIZE), compOnlyPlayerCount(RANDOM_SIZE), compOnlyTeamCount(RANDOM_SIZE),
	waterContent(EWaterContent::RANDOM), monsterStrength(EMonsterStrength::RANDOM), mapTemplate(nullptr)
{
	resetPlayersMap();
}

si32 CMapGenOptions::getWidth() const
{
	return width;
}

void CMapGenOptions::setWidth(si32 value)
{
	assert(value >= 1);
	width = value;
}

si32 CMapGenOptions::getHeight() const
{
	return height;
}

void CMapGenOptions::setHeight(si32 value)
{
	assert(value >= 1);
	height = value;
}

bool CMapGenOptions::getHasTwoLevels() const
{
	return hasTwoLevels;
}

void CMapGenOptions::setHasTwoLevels(bool value)
{
	hasTwoLevels = value;
}

si8 CMapGenOptions::getPlayerCount() const
{
	return playerCount;
}

void CMapGenOptions::setPlayerCount(si8 value)
{
	assert((value >= 1 && value <= PlayerColor::PLAYER_LIMIT_I) || value == RANDOM_SIZE);
	playerCount = value;

	auto possibleCompPlayersCount = PlayerColor::PLAYER_LIMIT_I - value;
	if (compOnlyPlayerCount > possibleCompPlayersCount)
		setCompOnlyPlayerCount(possibleCompPlayersCount);

	resetPlayersMap();
}

si8 CMapGenOptions::getTeamCount() const
{
	return teamCount;
}

void CMapGenOptions::setTeamCount(si8 value)
{
	assert(getPlayerCount() == RANDOM_SIZE || (value >= 0 && value < getPlayerCount()) || value == RANDOM_SIZE);
	teamCount = value;
}

si8 CMapGenOptions::getCompOnlyPlayerCount() const
{
	return compOnlyPlayerCount;
}

void CMapGenOptions::setCompOnlyPlayerCount(si8 value)
{
	assert(value == RANDOM_SIZE || (getPlayerCount() == RANDOM_SIZE || (value >= 0 && value <= PlayerColor::PLAYER_LIMIT_I - getPlayerCount())));
	compOnlyPlayerCount = value;

	resetPlayersMap();
}

si8 CMapGenOptions::getCompOnlyTeamCount() const
{
	return compOnlyTeamCount;
}

void CMapGenOptions::setCompOnlyTeamCount(si8 value)
{
	assert(value == RANDOM_SIZE || compOnlyPlayerCount == RANDOM_SIZE || (value >= 0 && value <= std::max(compOnlyPlayerCount - 1, 0)));
	compOnlyTeamCount = value;
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

	std::map<PlayerColor, FactionID> rememberTownTypes;
	std::map<PlayerColor, TeamID> rememberTeam;

	for(const auto & p : players)
	{
		auto town = p.second.getStartingTown();
		if (town != RANDOM_SIZE)
			rememberTownTypes[p.first] = FactionID(town);
		rememberTeam[p.first] = p.second.getTeam();
	}


	players.clear();
	int realPlayersCnt = playerCount;
	int realCompOnlyPlayersCnt = (compOnlyPlayerCount == RANDOM_SIZE) ? (PlayerColor::PLAYER_LIMIT_I - realPlayersCnt) : compOnlyPlayerCount;
	int totalPlayersLimit = realPlayersCnt + realCompOnlyPlayersCnt;
	if (getPlayerCount() == RANDOM_SIZE || compOnlyPlayerCount == RANDOM_SIZE)
		totalPlayersLimit = static_cast<int>(PlayerColor::PLAYER_LIMIT_I);

	//FIXME: what happens with human players here?
	for(int color = 0; color < totalPlayersLimit; ++color)
	{
		CPlayerSettings player;
		auto pc = PlayerColor(color);
		player.setColor(pc);
		auto playerType = EPlayerType::AI;
		if (getPlayerCount() != RANDOM_SIZE && color < realPlayersCnt)
		{
			playerType = EPlayerType::HUMAN;
		}
		else if((getPlayerCount() != RANDOM_SIZE && color >= realPlayersCnt)
		   || (compOnlyPlayerCount != RANDOM_SIZE && color >= (PlayerColor::PLAYER_LIMIT_I-compOnlyPlayerCount)))
		{
			playerType = EPlayerType::COMP_ONLY;
		}
		player.setPlayerType(playerType);
		player.setTeam(rememberTeam[pc]);
		players[pc] = player;

		if (vstd::contains(rememberTownTypes, pc))
			players[pc].setStartingTown(rememberTownTypes[pc]);
	}
}

const std::map<PlayerColor, CMapGenOptions::CPlayerSettings> & CMapGenOptions::getPlayersSettings() const
{
	return players;
}

void CMapGenOptions::setStartingTownForPlayer(const PlayerColor & color, si32 town)
{
	auto it = players.find(color);
	if(it == players.end()) assert(0);
	it->second.setStartingTown(town);
}

void CMapGenOptions::setPlayerTypeForStandardPlayer(const PlayerColor & color, EPlayerType::EPlayerType playerType)
{
	assert(playerType != EPlayerType::COMP_ONLY);
	auto it = players.find(color);
	if(it == players.end()) assert(0);
	it->second.setPlayerType(playerType);
}

const CRmgTemplate * CMapGenOptions::getMapTemplate() const
{
	return mapTemplate;
}

void CMapGenOptions::setMapTemplate(const CRmgTemplate * value)
{
	mapTemplate = value;
	//validate & adapt options according to template
	if(mapTemplate)
	{
		if(!mapTemplate->matchesSize(int3(getWidth(), getHeight(), 1 + getHasTwoLevels())))
		{
			auto sizes = mapTemplate->getMapSizes();
			setWidth(sizes.first.x);
			setHeight(sizes.first.y);
			setHasTwoLevels(sizes.first.z - 1);
		}
		
		if(!mapTemplate->getPlayers().isInRange(getPlayerCount()))
			setPlayerCount(RANDOM_SIZE);
		if(!mapTemplate->getCpuPlayers().isInRange(getCompOnlyPlayerCount()))
			setCompOnlyPlayerCount(RANDOM_SIZE);
		if(!mapTemplate->getWaterContentAllowed().count(getWaterContent()))
			setWaterContent(EWaterContent::RANDOM);
	}
}

void CMapGenOptions::setMapTemplate(const std::string & name)
{
	if(!name.empty())
		setMapTemplate(VLC->tplh->getTemplate(name));
}

void CMapGenOptions::setRoadEnabled(const std::string & roadName, bool enable)
{
	if(enable)
		disabledRoads.erase(roadName);
	else
		disabledRoads.insert(roadName);
}

bool CMapGenOptions::isRoadEnabled(const std::string & roadName) const
{
	return !disabledRoads.count(roadName);
}

void CMapGenOptions::setPlayerTeam(const PlayerColor & color, const TeamID & team)
{
	auto it = players.find(color);
	if(it == players.end()) assert(0);
	it->second.setTeam(team);
}

void CMapGenOptions::finalize(CRandomGenerator & rand)
{
	logGlobal->info("RMG map: %dx%d, %s underground", getWidth(), getHeight(), getHasTwoLevels() ? "WITH" : "NO");
	logGlobal->info("RMG settings: players %d, teams %d, computer players %d, computer teams %d, water %d, monsters %d",
		static_cast<int>(getPlayerCount()), static_cast<int>(getTeamCount()), static_cast<int>(getCompOnlyPlayerCount()),
		static_cast<int>(getCompOnlyTeamCount()), static_cast<int>(getWaterContent()), static_cast<int>(getMonsterStrength()));

	if(!mapTemplate)
	{
		mapTemplate = getPossibleTemplate(rand);
	}
	assert(mapTemplate);
	
	logGlobal->info("RMG template name: %s", mapTemplate->getName());

	if (getPlayerCount() == RANDOM_SIZE)
	{
		auto possiblePlayers = mapTemplate->getPlayers().getNumbers();
		//ignore all non-randomized players, make sure these players will not be missing after roll
		possiblePlayers.erase(possiblePlayers.begin(), possiblePlayers.lower_bound(countHumanPlayers() + countCompOnlyPlayers()));
		assert(!possiblePlayers.empty());
		setPlayerCount (*RandomGeneratorUtil::nextItem(possiblePlayers, rand));
		updatePlayers();
	}
	if(teamCount == RANDOM_SIZE)
	{
		teamCount = rand.nextInt(getPlayerCount() - 1);
		if (teamCount == 1)
			teamCount = 0;
	}
	if(compOnlyPlayerCount == RANDOM_SIZE)
	{
		auto possiblePlayers = mapTemplate->getCpuPlayers().getNumbers();
		compOnlyPlayerCount = *RandomGeneratorUtil::nextItem(possiblePlayers, rand);
		updateCompOnlyPlayers();
	}
	if(compOnlyTeamCount == RANDOM_SIZE)
	{
		compOnlyTeamCount = rand.nextInt(std::max(compOnlyPlayerCount - 1, 0));
	}

	if(waterContent == EWaterContent::RANDOM)
	{
		auto allowedContent = mapTemplate->getWaterContentAllowed();

		if(!allowedContent.empty())
		{
			waterContent = *RandomGeneratorUtil::nextItem(mapTemplate->getWaterContentAllowed(), rand);
		}
		else
		{
			waterContent = EWaterContent::NONE;
		}
	}

	if(monsterStrength == EMonsterStrength::RANDOM)
	{
		monsterStrength = static_cast<EMonsterStrength::EMonsterStrength>(rand.nextInt(EMonsterStrength::GLOBAL_WEAK, EMonsterStrength::GLOBAL_STRONG));
	}

	assert (vstd::iswithin(waterContent, EWaterContent::NONE, EWaterContent::ISLANDS));
	assert (vstd::iswithin(monsterStrength, EMonsterStrength::GLOBAL_WEAK, EMonsterStrength::GLOBAL_STRONG));


	//rectangular maps are the future of gaming
	//setHeight(20);
	//setWidth(50);

	logGlobal->trace("Player config:");
	int cpuOnlyPlayers = 0;
	for(const auto & player : players)
	{
		std::string playerType;
		switch (player.second.getPlayerType())
		{
		case EPlayerType::AI:
			playerType = "AI";
			break;
		case EPlayerType::COMP_ONLY:
			playerType = "computer only";
			cpuOnlyPlayers++;
			break;
		case EPlayerType::HUMAN:
			playerType = "human only";
			break;
			default:
				assert(false);
		}
		logGlobal->trace("Player %d: %s", player.second.getColor(), playerType);
	}
	setCompOnlyPlayerCount(cpuOnlyPlayers); //human players are set automaticlaly (?)
	logGlobal->info("Final player config: %d total, %d cpu-only", players.size(), static_cast<int>(getCompOnlyPlayerCount()));
}

void CMapGenOptions::updatePlayers()
{
	// Remove AI players only from the end of the players map if necessary
	for(auto itrev = players.end(); itrev != players.begin();)
	{
		auto it = itrev;
		--it;
		if (players.size() == getPlayerCount()) break;
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

void CMapGenOptions::updateCompOnlyPlayers()
{
	// Remove comp only players only from the end of the players map if necessary
	for(auto itrev = players.end(); itrev != players.begin();)
	{
		auto it = itrev;
		--it;
		if (players.size() <= getPlayerCount()) break;
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
	int compOnlyPlayersToAdd = static_cast<int>(getPlayerCount() - players.size());

	if (compOnlyPlayersToAdd < 0)
	{
		logGlobal->error("Incorrect number of players to add. Requested players %d, current players %d", playerCount, players.size());
		assert (compOnlyPlayersToAdd < 0);
	}
	for(int i = 0; i < compOnlyPlayersToAdd; ++i)
	{
		CPlayerSettings pSettings;
		pSettings.setPlayerType(EPlayerType::COMP_ONLY);
		pSettings.setColor(getNextPlayerColor());
		players[pSettings.getColor()] = pSettings;
	}
}

int CMapGenOptions::countHumanPlayers() const
{
	return static_cast<int>(boost::count_if(players, [](const std::pair<PlayerColor, CPlayerSettings> & pair)
	{
		return pair.second.getPlayerType() == EPlayerType::HUMAN;
	}));
}

int CMapGenOptions::countCompOnlyPlayers() const
{
	return static_cast<int>(boost::count_if(players, [](const std::pair<PlayerColor, CPlayerSettings> & pair)
	{
		return pair.second.getPlayerType() == EPlayerType::COMP_ONLY;
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
	logGlobal->error("Failed to get next player color");
	assert(false);
	return PlayerColor(0);
}

bool CMapGenOptions::checkOptions() const
{
	if(mapTemplate)
	{
		return true;
	}
	else
	{
		CRandomGenerator gen;
		return getPossibleTemplate(gen) != nullptr;
	}
}

std::vector<const CRmgTemplate *> CMapGenOptions::getPossibleTemplates() const
{
	int3 tplSize(width, height, (hasTwoLevels ? 2 : 1));
	auto humanPlayers = countHumanPlayers();

	auto templates = VLC->tplh->getTemplates();

	vstd::erase_if(templates, [this, &tplSize, humanPlayers](const CRmgTemplate * tmpl)
	{
		if(!tmpl->matchesSize(tplSize))
			return true;

		if(!tmpl->isWaterContentAllowed(getWaterContent()))
			return true;

		if(getPlayerCount() != -1)
		{
			if (!tmpl->getPlayers().isInRange(getPlayerCount()))
				return true;
		}
		else
		{
			// Human players shouldn't be banned when playing with random player count
			if(humanPlayers > *boost::min_element(tmpl->getPlayers().getNumbers()))
				return true;
		}

		if(compOnlyPlayerCount != -1)
		{
			if (!tmpl->getCpuPlayers().isInRange(compOnlyPlayerCount))
				return true;
		}

		return false;
	});

	return templates;
}

const CRmgTemplate * CMapGenOptions::getPossibleTemplate(CRandomGenerator & rand) const
{
	auto templates = getPossibleTemplates();

	if(templates.empty())
		return nullptr;
	
	return *RandomGeneratorUtil::nextItem(templates, rand);
}

CMapGenOptions::CPlayerSettings::CPlayerSettings() : color(0), startingTown(RANDOM_TOWN), playerType(EPlayerType::AI), team(TeamID::NO_TEAM)
{

}

PlayerColor CMapGenOptions::CPlayerSettings::getColor() const
{
	return color;
}

void CMapGenOptions::CPlayerSettings::setColor(const PlayerColor & value)
{
	assert(value >= PlayerColor(0) && value < PlayerColor::PLAYER_LIMIT);
	color = value;
}

si32 CMapGenOptions::CPlayerSettings::getStartingTown() const
{
	return startingTown;
}

void CMapGenOptions::CPlayerSettings::setStartingTown(si32 value)
{
	assert(value >= -1);
	if(value >= 0)
	{
		assert(value < static_cast<int>(VLC->townh->size()));
		assert((*VLC->townh)[value]->town != nullptr);
	}
	startingTown = value;
}

EPlayerType::EPlayerType CMapGenOptions::CPlayerSettings::getPlayerType() const
{
	return playerType;
}

void CMapGenOptions::CPlayerSettings::setPlayerType(EPlayerType::EPlayerType value)
{
	playerType = value;
}

TeamID CMapGenOptions::CPlayerSettings::getTeam() const
{
	return team;
}

void CMapGenOptions::CPlayerSettings::setTeam(const TeamID & value)
{
	team = value;
}

VCMI_LIB_NAMESPACE_END
