/*
 * GameStatistics.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "GameStatistics.h"
#include "../CPlayerState.h"
#include "../constants/StringConstants.h"
#include "CGameState.h"

VCMI_LIB_NAMESPACE_BEGIN

void StatisticDataSet::add(StatisticDataSetEntry entry)
{
	data.push_back(entry);
}

StatisticDataSetEntry StatisticDataSet::createEntry(const PlayerState * ps, const CGameState * gs)
{
	StatisticDataSetEntry data;

	data.day = gs->getDate(Date::DAY);
	data.player = ps->color;
	data.team = ps->team;
	data.resources = ps->resources;
	data.heroesCount = ps->heroes.size();
	data.townCount = ps->towns.size();

	return data;
}

std::string StatisticDataSet::toCsv()
{
	std::stringstream ss;

	auto resources = std::vector<EGameResID>{EGameResID::GOLD, EGameResID::WOOD, EGameResID::MERCURY, EGameResID::ORE, EGameResID::SULFUR, EGameResID::CRYSTAL, EGameResID::GEMS};

	ss << "Day" << ";" << "Player" << ";" << "Team" << ";" << "HeroesCount" << ";" << "TownCount";
	for(auto & resource : resources)
		ss << ";" << GameConstants::RESOURCE_NAMES[resource];
	ss << "\r\n";

	for(auto & entry : data)
	{
		ss << entry.day << ";" << GameConstants::PLAYER_COLOR_NAMES[entry.player] << ";" << entry.team.getNum() <<  ";" << entry.heroesCount <<  ";" << entry.townCount;
		for(auto & resource : resources)
			ss << ";" << entry.resources[resource];
		ss << "\r\n";
	}

	return ss.str();
}

VCMI_LIB_NAMESPACE_END
