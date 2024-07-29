/*
 * HighScore.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

VCMI_LIB_NAMESPACE_BEGIN

class DLL_LINKAGE HighScoreParameter
{
public:
	int difficulty;
	int day;
	int townAmount;
	bool usedCheat;
	bool hasGrail;
	bool allEnemiesDefeated;
	std::string campaignName;
	std::string scenarioName;
	std::string playerName;

	template <typename Handler> void serialize(Handler &h)
	{
		h & difficulty;
		h & day;
		h & townAmount;
		h & usedCheat;
		h & hasGrail;
		h & allEnemiesDefeated;
		h & campaignName;
		h & scenarioName;
		h & playerName;
	}
};

VCMI_LIB_NAMESPACE_END
