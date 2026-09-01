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
#include "../texts/MetaString.h"

class ITranslator;

class PlayerState;
class CGameState;
class CGHeroInstance;
class CGMine;
struct TeamState;

struct DLL_LINKAGE StatisticDataSetEntry
{
	/// map name lives in a map overlay, so it stays unresolved until it is displayed or exported
	MetaString map;
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

	void serializeJson(JsonSerializeFormat & handler);

	template <typename Handler> void serialize(Handler &h)
	{
		if(h.hasFeature(Handler::Version::RECORD_TEXTS_METASTRING))
		{
			h & map;
		}
		else
		{
			// older saves stored the name already rendered in the writer's language
			std::string legacyMap;
			h & legacyMap;
			map = MetaString::createFromRawString(legacyMap);
		}
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
	static StatisticDataSetEntry createEntry(const PlayerState * ps, const CGameState * gs, const StatisticDataSet & accumulatedData);
	std::string toCsv(const std::string & sep, const ITranslator * translator) const;
	std::string writeCsv(const ITranslator * translator) const;

	void serializeJson(JsonSerializeFormat & handler);

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

		void serializeJson(JsonSerializeFormat & handler);

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

	PlayerAccumulatedValueStorage & getPlayerAccumulator(PlayerColor player);
	const PlayerAccumulatedValueStorage & getPlayerAccumulator(PlayerColor player) const;

	void filterByTeam(const TeamState * team);

private:
	std::map<PlayerColor, PlayerAccumulatedValueStorage> accumulatedValues;

public:
	template <typename Handler> void serialize(Handler &h)
	{
		h & data;
		h & accumulatedValues;
	}
};

class DLL_LINKAGE Statistic
{
public:
	static int getNumberOfArts(const PlayerState * ps);
	static int getNumberOfDwellings(const PlayerState * ps);
	static si64 getArmyStrength(const PlayerState * ps, bool withTownGarrison = false, bool asPerceivedByOthers = false);
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
