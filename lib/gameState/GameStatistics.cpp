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

VCMI_LIB_NAMESPACE_BEGIN

void StatisticDataSet::add(StatisticDataSetEntry entry)
{
	data.push_back(entry);
}

std::string StatisticDataSet::toCsv()
{
	std::stringstream ss;

	ss << "Day" << ";" << "Player" << "\r\n";

	for(auto & entry : data)
	{
		ss << entry.day << ";" << entry.player.getNum() << "\r\n";
	}

	return ss.str();
}

VCMI_LIB_NAMESPACE_END
