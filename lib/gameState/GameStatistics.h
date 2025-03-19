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

class PlayerState;
class CGameState;
class CGHeroInstance;
class CGMine;

struct DLL_LINKAGE StatisticDataSetEntry
{
	std::string map;
	time_t timestamp;
    int day;
    PlayerColor player;
    std::string playerName;
	TeamID team;
	bool isHuman;
	EPlayerStatus status;
	TResources resources;
	int numberHeroes;
	int numberTowns;
	int numberArtifacts;
	int numberDwellings;
	si64 armyStrength;
	si64 totalExperience;
	int income;
	float mapExploredRatio;
	float obeliskVisitedRatio;
	float townBuiltRatio;
	bool hasGrail;
	std::map<EGameResID, int> numMines;
	int score;
	int maxHeroLevel;
	int numBattlesNeutral;
	int numBattlesPlayer;
	int numWinBattlesNeutral;
	int numWinBattlesPlayer;
	int numHeroSurrendered;
	int numHeroEscaped;
	TResources spentResourcesForArmy;
	TResources spentResourcesForBuildings;
	TResources tradeVolume;
	bool eventCapturedTown;
	bool eventDefeatedStrongestHero;
	si64 movementPointsUsed;

	template <typename Handler> void serialize(Handler &h)
	{
		h & map;
		h & timestamp;
		h & day;
		h & player;
		h & playerName;
		h & team;
		h & isHuman;
		h & status;
		h & resources;
		h & numberHeroes;
		h & numberTowns;
		h & numberArtifacts;
		h & numberDwellings;
		h & armyStrength;
		h & totalExperience;
		h & income;
		h & mapExploredRatio;
		h & obeliskVisitedRatio;
		h & townBuiltRatio;
		h & hasGrail;
		h & numMines;
		h & score;
		h & maxHeroLevel;
		h & numBattlesNeutral;
		h & numBattlesPlayer;
		h & numWinBattlesNeutral;
		h & numWinBattlesPlayer;
		h & numHeroSurrendered;
		h & numHeroEscaped;
		h & spentResourcesForArmy;
		h & spentResourcesForBuildings;
		h & tradeVolume;
		h & eventCapturedTown;
		h & eventDefeatedStrongestHero;
		h & movementPointsUsed;
	}
};

class DLL_LINKAGE StatisticDataSet
{
public:
    void add(StatisticDataSetEntry entry);
	static StatisticDataSetEntry createEntry(const PlayerState * ps, const CGameState * gs);
    std::string toCsv(std::string sep);
    std::string writeCsv();

	struct PlayerAccumulatedValueStorage // holds some actual values needed for stats
	{
		int numBattlesNeutral;
		int numBattlesPlayer;
		int numWinBattlesNeutral;
		int numWinBattlesPlayer;
		int numHeroSurrendered;
		int numHeroEscaped;
		TResources spentResourcesForArmy;
		TResources spentResourcesForBuildings;
		TResources tradeVolume;
		si64 movementPointsUsed;
		int lastCapturedTownDay;
		int lastDefeatedStrongestHeroDay;

		template <typename Handler> void serialize(Handler &h)
		{
			h & numBattlesNeutral;
			h & numBattlesPlayer;
			h & numWinBattlesNeutral;
			h & numWinBattlesPlayer;
			h & numHeroSurrendered;
			h & numHeroEscaped;
			h & spentResourcesForArmy;
			h & spentResourcesForBuildings;
			h & tradeVolume;
			h & movementPointsUsed;
			h & lastCapturedTownDay;
			h & lastDefeatedStrongestHeroDay;
		}
	};
	std::vector<StatisticDataSetEntry> data;
	std::map<PlayerColor, PlayerAccumulatedValueStorage> accumulatedValues;

	template <typename Handler> void serialize(Handler &h)
	{
		h & data;
		h & accumulatedValues;
	}
};

class DLL_LINKAGE Statistic
{
	static std::vector<const CGMine *> getMines(const CGameState * gs, const PlayerState * ps);
public:
	static int getNumberOfArts(const PlayerState * ps);
	static int getNumberOfDwellings(const PlayerState * ps);
	static si64 getArmyStrength(const PlayerState * ps, bool withTownGarrison = false);
	static si64 getTotalExperience(const PlayerState * ps);
	static int getIncome(const CGameState * gs, const PlayerState * ps);
	static float getMapExploredRatio(const CGameState * gs, PlayerColor player);
	static const CGHeroInstance * findBestHero(const CGameState * gs, const PlayerColor & color);
	static std::vector<std::vector<PlayerColor>> getRank(std::vector<std::pair<PlayerColor, si64>> stats);
	static int getObeliskVisited(const CGameState * gs, const TeamID & t);
	static float getObeliskVisitedRatio(const CGameState * gs, const TeamID & t);
	static std::map<EGameResID, int> getNumMines(const CGameState * gs, const PlayerState * ps);
	static float getTownBuiltRatio(const PlayerState * ps);
};

VCMI_LIB_NAMESPACE_END
