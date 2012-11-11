#include "StdInc.h"
#include "CMapGenOptions.h"

CMapGenOptions::CMapGenOptions() : mapSize(EMapSize::MEDIUM), hasTwoLevels(true),
	playersCnt(-1), teamsCnt(-1), compOnlyPlayersCnt(-1), compOnlyTeamsCnt(-1),
	waterContent(EWaterContent::NORMAL), monsterStrength(EMonsterStrength::NORMAL)
{

}

EMapSize::EMapSize CMapGenOptions::getMapSize() const
{
	return mapSize;
}

void CMapGenOptions::setMapSize(EMapSize::EMapSize value)
{
	mapSize = value;
}

bool CMapGenOptions::getHasTwoLevels() const
{
	return hasTwoLevels;
}

void CMapGenOptions::setHasTwoLevels(bool value)
{
	hasTwoLevels = value;
}

int CMapGenOptions::getPlayersCnt() const
{
	return playersCnt;
}

void CMapGenOptions::setPlayersCnt(int value)
{
	if((value >= 1 && value <= 8) || value == -1)
	{
		playersCnt = value;
	}
	else
	{
		throw std::runtime_error("Players count of RMG options should be between 1 and 8 or -1 for random.");
	}
}

int CMapGenOptions::getTeamsCnt() const
{
	return teamsCnt;
}

void CMapGenOptions::setTeamsCnt(int value)
{
	if(playersCnt == -1 || (value >= 0 && value < playersCnt) || value == -1)
	{
		teamsCnt = value;
	}
	else
	{
		throw std::runtime_error("Teams count of RMG options should be between 0 and <" +
			boost::lexical_cast<std::string>(playersCnt) + "(players count) - 1> or -1 for random.");
	}
}

int CMapGenOptions::getCompOnlyPlayersCnt() const
{
	return compOnlyPlayersCnt;
}

void CMapGenOptions::setCompOnlyPlayersCnt(int value)
{
	if(value == -1 || (value >= 0 && value <= 8 - playersCnt))
	{
		compOnlyPlayersCnt = value;
	}
	else
	{
		throw std::runtime_error(std::string("Computer only players count of RMG options should be ") +
			"between 0 and <8 - " + boost::lexical_cast<std::string>(playersCnt) +
			"(playersCount)> or -1 for random.");
	}
}

int CMapGenOptions::getCompOnlyTeamsCnt() const
{
	return compOnlyTeamsCnt;
}

void CMapGenOptions::setCompOnlyTeamsCnt(int value)
{
	if(value == -1 || compOnlyPlayersCnt == -1 || (value >= 0 && value <= compOnlyPlayersCnt - 1))
	{
		compOnlyTeamsCnt = value;
	}
	else
	{
		throw std::runtime_error(std::string("Computer only teams count of RMG options should be ") +
			"between 0 and <" + boost::lexical_cast<std::string>(compOnlyPlayersCnt) +
			"(compOnlyPlayersCnt) - 1> or -1 for random.");
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
