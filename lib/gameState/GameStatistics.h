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
class CGHeroInstance;
class CGMine;

struct DLL_LINKAGE StatisticDataSetEntry
{
	std::string map;
	time_t timestamp;
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
	int obeliskVisited;
	double mightMagicRatio;
	std::map<EGameResID, int> numMines;
	int score;
	int maxHeroLevel;
	int numBattlesNeutral;
	int numBattlesPlayer;
	int numWinBattlesNeutral;
	int numWinBattlesPlayer;
	int numHeroSurrendered;
	int numHeroEscaped;

	template <typename Handler> void serialize(Handler &h)
	{
		h & map;
		h & timestamp;
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
		h & obeliskVisited;
		h & mightMagicRatio;
		h & numMines;
		h & score;
		h & maxHeroLevel;
		h & numBattlesNeutral;
		h & numBattlesPlayer;
		h & numWinBattlesNeutral;
		h & numWinBattlesPlayer;
		h & numHeroSurrendered;
		h & numHeroEscaped;
	}
};

class DLL_LINKAGE StatisticDataSet
{
    std::vector<StatisticDataSetEntry> data;

public:
    void add(StatisticDataSetEntry entry);
	static StatisticDataSetEntry createEntry(const PlayerState * ps, const CGameState * gs);
    std::string toCsv();

	struct ValueStorage
	{
		std::map<PlayerColor, int> numBattlesNeutral;
		std::map<PlayerColor, int> numBattlesPlayer;
		std::map<PlayerColor, int> numWinBattlesNeutral;
		std::map<PlayerColor, int> numWinBattlesPlayer;
		std::map<PlayerColor, int> numHeroSurrendered;
		std::map<PlayerColor, int> numHeroEscaped;

		template <typename Handler> void serialize(Handler &h)
		{
			h & numBattlesNeutral;
			h & numBattlesPlayer;
			h & numWinBattlesNeutral;
			h & numWinBattlesPlayer;
			h & numHeroSurrendered;
			h & numHeroEscaped;
		}
	};
	ValueStorage values;

	template <typename Handler> void serialize(Handler &h)
	{
		h & data;
		h & values;
	}
};

class DLL_LINKAGE Statistic
{
	static std::vector<const CGMine *> getMines(const CGameState * gs, const PlayerState * ps);
public:
	using TStat = std::pair<PlayerColor, si64>;

	static int getNumberOfArts(const PlayerState * ps);
	static si64 getArmyStrength(const PlayerState * ps, bool withTownGarrison = false);
	static int getIncome(const CGameState * gs, const PlayerState * ps);
	static double getMapVisitedRatio(const CGameState * gs, PlayerColor player);
	static const CGHeroInstance * findBestHero(const CGameState * gs, const PlayerColor & color);
	static std::vector<std::vector<PlayerColor>> getRank(std::vector<TStat> stats);
	static int getObeliskVisited(const CGameState * gs, const TeamID & t);
	static double getMightMagicRatio(const PlayerState * ps);
	static std::map<EGameResID, int> getNumMines(const CGameState * gs, const PlayerState * ps);
};

VCMI_LIB_NAMESPACE_END
