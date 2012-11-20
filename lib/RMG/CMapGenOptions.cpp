#include "CMapGenOptions.h"

CMapGenOptions::CMapGenOptions() : width(72), height(72), hasTwoLevels(true),
	playersCnt(-1), teamsCnt(-1), compOnlyPlayersCnt(0), compOnlyTeamsCnt(-1),
	waterContent(EWaterContent::RANDOM), monsterStrength(EMonsterStrength::RANDOM)
{

}

si32 CMapGenOptions::getWidth() const
{
	return width;
}

void CMapGenOptions::setWidth(int value)
{
	if(value > 0)
	{
		width = value;
	}
	else
	{
		throw std::runtime_error("Map width lower than 1 not allowed.");
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
		throw std::runtime_error("Map height lower than 1 not allowed.");
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
	if((value >= 1 && value <= 8) || value == RANDOM_SIZE)
	{
		playersCnt = value;
	}
	else
	{
		throw std::runtime_error("Players count of RMG options should be between 1 and 8 or -1 for random.");
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
			boost::lexical_cast<std::string>(playersCnt) + "(players count) - 1> or -1 for random.");
	}
}

si8 CMapGenOptions::getCompOnlyPlayersCnt() const
{
	return compOnlyPlayersCnt;
}

void CMapGenOptions::setCompOnlyPlayersCnt(si8 value)
{
	if(value == RANDOM_SIZE || (value >= 0 && value <= 8 - playersCnt))
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

si8 CMapGenOptions::getCompOnlyTeamsCnt() const
{
	return compOnlyTeamsCnt;
}

void CMapGenOptions::setCompOnlyTeamsCnt(si8 value)
{
	if(value == RANDOM_SIZE || compOnlyPlayersCnt == RANDOM_SIZE || (value >= 0 && value <= compOnlyPlayersCnt - 1))
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
