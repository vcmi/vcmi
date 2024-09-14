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

#include "../GameConstants.h"

VCMI_LIB_NAMESPACE_BEGIN

class CGameState;

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
class DLL_LINKAGE HighScore
{
public:
	static HighScoreParameter prepareHighScores(const CGameState * gs, PlayerColor player, bool victory);
};

class DLL_LINKAGE HighScoreCalculation
{
public:
	struct Result
	{
		int basic = 0;
		int total = 0;
		int sumDays = 0;
		bool cheater = false;
	};

	std::vector<HighScoreParameter> parameters;
	bool isCampaign = false;

	Result calculate();
	static CreatureID getCreatureForPoints(int points, bool campaign);
};

VCMI_LIB_NAMESPACE_END
