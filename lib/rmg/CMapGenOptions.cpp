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

#include "../entities/faction/CTownHandler.h"
#include "../entities/faction/CFaction.h"
#include "../mapping/CMapHeader.h"
#include "CRmgTemplateStorage.h"
#include "CRmgTemplate.h"
#include "../GameLibrary.h"
#include "serializer/JsonSerializeFormat.h"

#include <vstd/RNG.h>

VCMI_LIB_NAMESPACE_BEGIN

CMapGenOptions::CMapGenOptions()
	: width(CMapHeader::MAP_SIZE_MIDDLE), height(CMapHeader::MAP_SIZE_MIDDLE), levels(2),
	humanOrCpuPlayerCount(RANDOM_SIZE), teamCount(RANDOM_SIZE), compOnlyPlayerCount(RANDOM_SIZE), compOnlyTeamCount(RANDOM_SIZE),
	waterContent(EWaterContent::RANDOM), monsterStrength(EMonsterStrength::RANDOM), mapTemplate(nullptr),
	customizedPlayers(false)
{
	initPlayersMap();
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

int CMapGenOptions::getLevels() const
{
	return levels;
}

void CMapGenOptions::setLevels(int value)
{
	levels = value;
}

si8 CMapGenOptions::getHumanOrCpuPlayerCount() const
{
	return humanOrCpuPlayerCount;
}

void CMapGenOptions::setHumanOrCpuPlayerCount(si8 value)
{
	assert((value >= 1 && value <= PlayerColor::PLAYER_LIMIT_I) || value == RANDOM_SIZE);
	humanOrCpuPlayerCount = value;

	// Use template player limit, if any
	auto playerLimit = getPlayerLimit();
	auto possibleCompPlayersCount = playerLimit - std::max<si8>(0, humanOrCpuPlayerCount);
	if (compOnlyPlayerCount > possibleCompPlayersCount)
	{
		setCompOnlyPlayerCount(possibleCompPlayersCount);
	}

	resetPlayersMap();
}

si8 CMapGenOptions::getMinPlayersCount(bool withTemplateLimit) const
{
	auto totalPlayers = 0;
	si8 humans = getHumanOrCpuPlayerCount();
	si8 cpus = getCompOnlyPlayerCount();

	if (humans == RANDOM_SIZE && cpus == RANDOM_SIZE)
	{
		totalPlayers = 2;
	}
	else if (humans == RANDOM_SIZE)
	{
		totalPlayers = cpus + 1; // Must add at least 1 player
	}
	else if (cpus == RANDOM_SIZE)
	{
		totalPlayers = humans;
	}
	else
	{
		totalPlayers = humans + cpus;
	}

	if (withTemplateLimit && mapTemplate)
	{
		auto playersRange = mapTemplate->getPlayers();

		//New template can also impose higher limit than current settings
		vstd::amax(totalPlayers, playersRange.minValue());
	}

	// Can't play without at least 2 players
	vstd::amax(totalPlayers, 2);
	return totalPlayers;
}

si8 CMapGenOptions::getMaxPlayersCount(bool withTemplateLimit) const
{
	// Max number of players possible with current settings
	auto totalPlayers = 0;
	si8 humans = getHumanOrCpuPlayerCount();
	si8 cpus = getCompOnlyPlayerCount();
	if (humans == RANDOM_SIZE || cpus == RANDOM_SIZE)
	{
		totalPlayers = PlayerColor::PLAYER_LIMIT_I;
	}
	else
	{
		totalPlayers = std::max(humans + cpus, 2);
	}

	if (withTemplateLimit && mapTemplate)
	{
		auto playersRange = mapTemplate->getPlayers();

		//New template can also impose higher limit than current settings
		vstd::amin(totalPlayers, playersRange.maxValue());
	}

	assert (totalPlayers <= PlayerColor::PLAYER_LIMIT_I);
	assert (totalPlayers >= 2);
	return totalPlayers;
}

si8 CMapGenOptions::getTeamCount() const
{
	return teamCount;
}

void CMapGenOptions::setTeamCount(si8 value)
{
	assert(getHumanOrCpuPlayerCount() == RANDOM_SIZE || (value >= 0 && value < getHumanOrCpuPlayerCount()) || value == RANDOM_SIZE);
	teamCount = value;
}

si8 CMapGenOptions::getCompOnlyPlayerCount() const
{
	return compOnlyPlayerCount;
}

si8 CMapGenOptions::getPlayerLimit() const
{
	//How many players could we set with current template, ignoring other settings
	si8 playerLimit = PlayerColor::PLAYER_LIMIT_I;
	if (auto temp = getMapTemplate())
	{
		playerLimit = static_cast<si8>(temp->getPlayers().maxValue());
	}
	return playerLimit;
}

void CMapGenOptions::setCompOnlyPlayerCount(si8 value)
{
	assert(value == RANDOM_SIZE || (getHumanOrCpuPlayerCount() == RANDOM_SIZE || (value >= 0 && value <= getPlayerLimit() - getHumanOrCpuPlayerCount())));
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

void CMapGenOptions::initPlayersMap()
{

	std::map<PlayerColor, FactionID> rememberTownTypes;
	std::map<PlayerColor, TeamID> rememberTeam;

	for(const auto & p : players)
	{
		auto town = p.second.getStartingTown();
		if (town != FactionID::RANDOM)
			rememberTownTypes[p.first] = FactionID(town);
		rememberTeam[p.first] = p.second.getTeam();
	}


	players.clear();
	int realPlayersCnt = getHumanOrCpuPlayerCount();

	// Initialize settings for all color even if not present
	for(int color = 0; color < getPlayerLimit(); ++color)
	{
		CPlayerSettings player;
		auto pc = PlayerColor(color);
		player.setColor(pc);

		auto playerType = EPlayerType::AI;
		// Color doesn't have to be continuous. Player colors can later be changed manually
		if (getHumanOrCpuPlayerCount() != RANDOM_SIZE && color < realPlayersCnt)
		{
			playerType = EPlayerType::HUMAN;
		}
		else if((getHumanOrCpuPlayerCount() != RANDOM_SIZE && color >= realPlayersCnt)
		   || (compOnlyPlayerCount != RANDOM_SIZE && color >= (PlayerColor::PLAYER_LIMIT_I - compOnlyPlayerCount)))
		{
			playerType = EPlayerType::COMP_ONLY;
		}
		player.setPlayerType(playerType);

		players[pc] = player;
	}
	savePlayersMap();
}

void CMapGenOptions::resetPlayersMap()
{
	// Called when number of player changes
	// But do not update info about already made selections

	savePlayersMap();

	int realPlayersCnt = getMaxPlayersCount();

	//Trim the number of AI players, then CPU-only players, finally human players
	auto eraseLastPlayer = [this](EPlayerType playerType) -> bool
	{
		for (auto it = players.rbegin(); it != players.rend(); ++it)
		{
			if (it->second.getPlayerType() == playerType)
			{
				players.erase(it->first);
				return true;
			}
		}
		return false; //Can't earse any player of this type
	};

	while (players.size() > realPlayersCnt)
	{
		if (eraseLastPlayer(EPlayerType::AI))
			continue;
		if (eraseLastPlayer(EPlayerType::COMP_ONLY))
			continue;
		if (eraseLastPlayer(EPlayerType::HUMAN))
			continue;
	}

	//First colors from the list are assigned to human players, then to CPU players
	std::vector<PlayerColor> availableColors;
	for (ui8 color = 0; color < PlayerColor::PLAYER_LIMIT_I; color++)
	{
		availableColors.push_back(PlayerColor(color));
	}

	auto removeUsedColors = [this, &availableColors](EPlayerType playerType)
	{
		for (auto& player : players)
		{
			if (player.second.getPlayerType() == playerType)
			{
				vstd::erase(availableColors, player.second.getColor());
			}
		}
	};
	removeUsedColors(EPlayerType::HUMAN);
	removeUsedColors(EPlayerType::COMP_ONLY);
	//removeUsedColors(EPlayerType::AI);

	//Assign unused colors to remaining AI players
	while (players.size() < realPlayersCnt && !availableColors.empty())
	{
		auto color = availableColors.front();
		players[color].setColor(color);
		setPlayerTypeForStandardPlayer(color, EPlayerType::AI);
		availableColors.erase(availableColors.begin());

		if (vstd::contains(savedPlayerSettings, color))
		{
			setPlayerTeam(color, savedPlayerSettings.at(color).getTeam());
			// TODO: setter
			players[color].setStartingTown(savedPlayerSettings.at(color).getStartingTown());
		}
		else
		{
			logGlobal->warn("Adding settings for player %s", color);
			// Usually, all players should be initialized in initPlayersMap()
			CPlayerSettings settings;
			players[color] = settings;
		}
	}

	std::set<TeamID> occupiedTeams;
	for(auto & player : players)
	{
		auto team = player.second.getTeam();
		if (team != TeamID::NO_TEAM)
		{
			occupiedTeams.insert(team);
		}
	}
	// TODO: Handle situation when we remove a player and remaining players belong to only one team

	for(auto & player : players)
	{
		if (player.second.getTeam() == TeamID::NO_TEAM)
		{
			//Find first unused team
			for(int i = 0; i < PlayerColor::PLAYER_LIMIT_I; ++i)
			{
				TeamID team(i);
				if(!occupiedTeams.count(team))
				{
					player.second.setTeam(team);
					occupiedTeams.insert(team);
					break;
				}
			}
		}
	}
}

void CMapGenOptions::savePlayersMap()
{
	//Only save already configured players
	for (const auto& player : players)
	{
		savedPlayerSettings[player.first] = player.second;
	}
}

const std::map<PlayerColor, CMapGenOptions::CPlayerSettings> & CMapGenOptions::getSavedPlayersMap() const
{
	return savedPlayerSettings;
}

const std::map<PlayerColor, CMapGenOptions::CPlayerSettings> & CMapGenOptions::getPlayersSettings() const
{
	return players;
}

void CMapGenOptions::setStartingTownForPlayer(const PlayerColor & color, FactionID town)
{
	auto it = players.find(color);
	assert(it != players.end());
	it->second.setStartingTown(town);
}

void CMapGenOptions::setStartingHeroForPlayer(const PlayerColor & color, HeroTypeID hero)
{
	auto it = players.find(color);
	assert(it != players.end());
	it->second.setStartingHero(hero);
}

void CMapGenOptions::setPlayerTypeForStandardPlayer(const PlayerColor & color, EPlayerType playerType)
{
	// FIXME: Why actually not set it to COMP_ONLY? Ie. when swapping human to another color?
	assert(playerType != EPlayerType::COMP_ONLY);
	auto it = players.find(color);
	assert(it != players.end());
	it->second.setPlayerType(playerType);
	customizedPlayers = true;
}

const CRmgTemplate * CMapGenOptions::getMapTemplate() const
{
	return mapTemplate;
}

void CMapGenOptions::setMapTemplate(const CRmgTemplate * value)
{
	if (mapTemplate == value)
	{
		//Does not trigger during deserialization
		return;
	}

	mapTemplate = value;
	//validate & adapt options according to template
	if(mapTemplate)
	{
		if(!mapTemplate->matchesSize(int3(getWidth(), getHeight(), getLevels())))
		{
			auto sizes = mapTemplate->getMapSizes();
			setWidth(sizes.first.x);
			setHeight(sizes.first.y);
			setLevels(sizes.first.z);
		}

		si8 maxPlayerCount = getMaxPlayersCount(false);
		si8 minPlayerCount = getMinPlayersCount(false);

		// Neither setting can fit within the template range
		if(!mapTemplate->getPlayers().isInRange(minPlayerCount) &&
			!mapTemplate->getPlayers().isInRange(maxPlayerCount))
		{
			setHumanOrCpuPlayerCount(RANDOM_SIZE);
			setCompOnlyPlayerCount(RANDOM_SIZE);
		}
		if(!mapTemplate->getWaterContentAllowed().count(getWaterContent()))
			setWaterContent(EWaterContent::RANDOM);

		resetPlayersMap(); // Update teams and player count
	}
}

void CMapGenOptions::setMapTemplate(const std::string & name)
{
	if(!name.empty())
		setMapTemplate(LIBRARY->tplh->getTemplate(name));
}

void CMapGenOptions::setRoadEnabled(const RoadId & roadType, bool enable)
{
	if (enable)
	{
		enabledRoads.insert(roadType);
	}
	else
	{
		enabledRoads.erase(roadType);
	}	
}

bool CMapGenOptions::isRoadEnabled(const RoadId & roadType) const
{
	return enabledRoads.count(roadType);
}

bool CMapGenOptions::isRoadEnabled() const
{
	return !enabledRoads.empty();
}

void CMapGenOptions::setPlayerTeam(const PlayerColor & color, const TeamID & team)
{
	auto it = players.find(color);
	assert(it != players.end());
	it->second.setTeam(team);
	customizedPlayers = true;
}

void CMapGenOptions::finalize(vstd::RNG & rand)
{
	logGlobal->info("RMG map: %dx%d, %s underground", getWidth(), getHeight(), getLevels() >= 2 ? "WITH" : "NO");
	logGlobal->info("RMG settings: players %d, teams %d, computer players %d, computer teams %d, water %d, monsters %d",
		static_cast<int>(getHumanOrCpuPlayerCount()), static_cast<int>(getTeamCount()), static_cast<int>(getCompOnlyPlayerCount()),
		static_cast<int>(getCompOnlyTeamCount()), static_cast<int>(getWaterContent()), static_cast<int>(getMonsterStrength()));

	if(!mapTemplate)
	{
		mapTemplate = getPossibleTemplate(rand);
	}
	assert(mapTemplate);
	
	logGlobal->info("RMG template name: %s", mapTemplate->getName());

	auto maxPlayers = getMaxPlayersCount();
	if (getHumanOrCpuPlayerCount() == RANDOM_SIZE)
	{
		auto possiblePlayers = mapTemplate->getPlayers().getNumbers();
		int requiredPlayers = countHumanPlayers() + countCompOnlyPlayers();
		//ignore all non-randomized players, make sure these players will not be missing after roll
		possiblePlayers.erase(possiblePlayers.begin(), possiblePlayers.lower_bound(requiredPlayers));

		vstd::erase_if(possiblePlayers, [maxPlayers](int i)
		{
			return i > maxPlayers;
		});
		assert(!possiblePlayers.empty());
		setHumanOrCpuPlayerCount (*RandomGeneratorUtil::nextItem(possiblePlayers, rand));
		updatePlayers();
	}
	if(teamCount == RANDOM_SIZE)
	{
		teamCount = rand.nextInt(getHumanOrCpuPlayerCount() - 1);
		if (teamCount == 1)
			teamCount = 0;
	}
	if(compOnlyPlayerCount == RANDOM_SIZE)
	{
		// Use remaining range
		auto presentPlayers = getHumanOrCpuPlayerCount();
		auto possiblePlayers = mapTemplate->getPlayers().getNumbers();
		vstd::erase_if(possiblePlayers, [maxPlayers, presentPlayers](int i)
		{
			return i > (maxPlayers - presentPlayers);
		});
		if (possiblePlayers.empty())
		{
			compOnlyPlayerCount = 0;
		}
		else
		{
			compOnlyPlayerCount = *RandomGeneratorUtil::nextItem(possiblePlayers, rand);
		}
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
	logGlobal->info("Final player config: %d total, %d cpu-only", players.size(), cpuOnlyPlayers);
}

void CMapGenOptions::updatePlayers()
{
	// Remove non-human players only from the end of the players map if necessary
	for(auto itrev = players.end(); itrev != players.begin();)
	{
		auto it = itrev;
		--it;
		if (players.size() == getHumanOrCpuPlayerCount())
			break;

		if(it->second.getPlayerType() != EPlayerType::HUMAN)
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
		if (players.size() <= getHumanOrCpuPlayerCount()) break;
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
	int compOnlyPlayersToAdd = static_cast<int>(getHumanOrCpuPlayerCount() - players.size());

	if (compOnlyPlayersToAdd < 0)
	{
		logGlobal->error("Incorrect number of players to add. Requested players %d, current players %d", humanOrCpuPlayerCount, players.size());
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
		return !getPossibleTemplates().empty();
	}
}

bool CMapGenOptions::arePlayersCustomized() const
{
	return customizedPlayers;
}

std::vector<const CRmgTemplate *> CMapGenOptions::getPossibleTemplates() const
{
	int3 tplSize(width, height, levels);
	auto humanPlayers = countHumanPlayers();

	auto templates = LIBRARY->tplh->getTemplates();

	vstd::erase_if(templates, [this, &tplSize, humanPlayers](const CRmgTemplate * tmpl)
	{
		if(!tmpl->matchesSize(tplSize))
			return true;

		if(!tmpl->isWaterContentAllowed(getWaterContent()))
			return true;

		auto humanOrCpuPlayerCount = getHumanOrCpuPlayerCount();
		auto compOnlyPlayerCount =  getCompOnlyPlayerCount();
		// Check if total number of players fall inside given range

		if(humanOrCpuPlayerCount != CMapGenOptions::RANDOM_SIZE && compOnlyPlayerCount != CMapGenOptions::RANDOM_SIZE)
		{
			if (!tmpl->getPlayers().isInRange(std::max(humanOrCpuPlayerCount + compOnlyPlayerCount, 2)))
				return true;

		}
		else if(humanOrCpuPlayerCount != CMapGenOptions::RANDOM_SIZE)
		{
			// We can always add any number CPU players, but not subtract
			if (!(humanOrCpuPlayerCount <= tmpl->getPlayers().maxValue()))
				return true;
		}
		else if(compOnlyPlayerCount != CMapGenOptions::RANDOM_SIZE)
		{
			//We must fit at least one more human player, but can add any number
			if (!(compOnlyPlayerCount < tmpl->getPlayers().maxValue()))
				return true;
		}
		else
		{
			// Human players shouldn't be banned when playing with random player count
			if(humanPlayers > tmpl->getPlayers().minValue())
				return true;
		}

		return false;
	});

	return templates;
}

const CRmgTemplate * CMapGenOptions::getPossibleTemplate(vstd::RNG & rand) const
{
	auto templates = getPossibleTemplates();

	if(templates.empty())
		return nullptr;
	
	return *RandomGeneratorUtil::nextItem(templates, rand);
}

CMapGenOptions::CPlayerSettings::CPlayerSettings() : color(0), startingTown(FactionID::RANDOM), startingHero(HeroTypeID::RANDOM), playerType(EPlayerType::AI), team(TeamID::NO_TEAM)
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

FactionID CMapGenOptions::CPlayerSettings::getStartingTown() const
{
	return startingTown;
}

void CMapGenOptions::CPlayerSettings::setStartingTown(FactionID value)
{
	assert(value >= FactionID::RANDOM);
	if(value != FactionID::RANDOM)
	{
		assert(value < FactionID(LIBRARY->townh->size()));
		assert((*LIBRARY->townh)[value]->town != nullptr);
	}
	startingTown = value;
}

HeroTypeID CMapGenOptions::CPlayerSettings::getStartingHero() const
{
	return startingHero;
}

void CMapGenOptions::CPlayerSettings::setStartingHero(HeroTypeID value)
{
	assert(value == HeroTypeID::RANDOM || value.toEntity(LIBRARY) != nullptr);
	startingHero = value;
}

EPlayerType CMapGenOptions::CPlayerSettings::getPlayerType() const
{
	return playerType;
}

void CMapGenOptions::CPlayerSettings::setPlayerType(EPlayerType value)
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

void CMapGenOptions::serializeJson(JsonSerializeFormat & handler)
{
	handler.serializeInt("width", width);
	handler.serializeInt("height", height);
	bool hasTwoLevelsKey = !handler.getCurrent()["haswoLevels"].isNull();
	bool hasTwoLevels = levels == 2;
	if(handler.saving || !hasTwoLevelsKey)
		handler.serializeInt("levels", levels, 1);
	else
	{
		handler.serializeBool("haswoLevels", hasTwoLevels);
		levels = hasTwoLevels ? 2 : 1;
	}
	handler.serializeInt("humanOrCpuPlayerCount", humanOrCpuPlayerCount);
	handler.serializeInt("teamCount", teamCount);
	handler.serializeInt("compOnlyPlayerCount", compOnlyPlayerCount);
	handler.serializeInt("compOnlyTeamCount", compOnlyTeamCount);
	handler.serializeInt("waterContent", waterContent);
	handler.serializeInt("monsterStrength", monsterStrength);

	std::string templateName;
	if(mapTemplate && handler.saving)
	{
		templateName = mapTemplate->getId();
	}
	handler.serializeString("templateName", templateName);
	if(!handler.saving)
	{
		setMapTemplate(templateName);
	}

	handler.serializeIdArray("roads", enabledRoads);
	if (!handler.saving)
	{
		// Player settings won't be saved
		resetPlayersMap();
	}
}

VCMI_LIB_NAMESPACE_END
