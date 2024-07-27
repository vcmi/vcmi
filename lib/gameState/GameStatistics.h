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

VCMI_LIB_NAMESPACE_BEGIN

struct DLL_LINKAGE StatisticDataSetEntry
{
    int day;
    PlayerColor player;

	template <typename Handler> void serialize(Handler &h)
	{
		h & day;
		h & player;
	}
};

class DLL_LINKAGE StatisticDataSet
{
    std::vector<StatisticDataSetEntry> data;

public:
    void add(StatisticDataSetEntry entry);
    std::string toCsv();

	template <typename Handler> void serialize(Handler &h)
	{
		h & data;
	}
};

VCMI_LIB_NAMESPACE_END
