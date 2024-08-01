/*
 * GameSTatistics.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../GameConstants.h"
#include "../ResourceSet.h"

VCMI_LIB_NAMESPACE_BEGIN

struct PlayerState;
class CGameState;

struct DLL_LINKAGE StatisticDataSetEntry
{
    int day;
    PlayerColor player;
	TeamID team;
	TResources resources;
	int heroesCount;
	int townCount;

	template <typename Handler> void serialize(Handler &h)
	{
		h & day;
		h & player;
		h & team;
		h & resources;
		h & heroesCount;
		h & townCount;
	}
};

class DLL_LINKAGE StatisticDataSet
{
    std::vector<StatisticDataSetEntry> data;

public:
    void add(StatisticDataSetEntry entry);
	static StatisticDataSetEntry createEntry(const PlayerState * ps, const CGameState * gs);
    std::string toCsv();

	template <typename Handler> void serialize(Handler &h)
	{
		h & data;
	}
};

VCMI_LIB_NAMESPACE_END
