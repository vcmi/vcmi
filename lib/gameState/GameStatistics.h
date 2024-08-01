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
	bool isHuman;
	EPlayerStatus status;
	TResources resources;
	int numberHeroes;
	int numberTowns;
	int numberArtifacts;
	si64 armyStrength;
	int income;
	double mapVisitedRatio;

	template <typename Handler> void serialize(Handler &h)
	{
		h & day;
		h & player;
		h & team;
		h & isHuman;
		h & status;
		h & resources;
		h & numberHeroes;
		h & numberTowns;
		h & numberArtifacts;
		h & armyStrength;
		h & income;
		h & mapVisitedRatio;
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

class DLL_LINKAGE Statistic
{
public:
    static int getNumberOfArts(const PlayerState * ps);
	static si64 getArmyStrength(const PlayerState * ps);
	static int getIncome(const CGameState * gs, const PlayerState * ps);
	static double getMapVisitedRatio(const CGameState * gs, PlayerColor player);
};

VCMI_LIB_NAMESPACE_END
