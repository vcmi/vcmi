/*
 * CMapGenerator.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "CMapGenOptions.h"
#include "../LoadProgress.h"

VCMI_LIB_NAMESPACE_BEGIN

class MetaString;
class CRmgTemplate;
class CMapGenOptions;
class JsonNode;
class RmgMap;
class CMap;
class Zone;
class CZonePlacer;
class IGameInfoCallback;

using JsonVector = std::vector<JsonNode>;

/// The map generator creates a map randomly.
class DLL_LINKAGE CMapGenerator: public Load::Progress
{
public:
	struct Config
	{
		std::vector<CTreasureInfo> waterTreasure;
		int shipyardGuard;
		int mineExtraResources;
		int minGuardStrength;
		std::string defaultRoadType;
		std::string secondaryRoadType;
		int treasureValueLimit;
		std::vector<int> prisonExperience;
		std::vector<int> prisonValues;
		std::vector<int> scrollValues;
		int pandoraMultiplierGold;
		int pandoraMultiplierExperience;
		int pandoraMultiplierSpells;
		int pandoraSpellSchool;
		int pandoraSpell60;
		std::vector<int> pandoraCreatureValues;
		std::vector<int> questValues;
		std::vector<int> questRewardValues;
		int seerHutValue;
		bool singleThread;
	};
	
	explicit CMapGenerator(CMapGenOptions& mapGenOptions, IGameInfoCallback * cb, int RandomSeed);
	~CMapGenerator(); // required due to std::unique_ptr
	
	const Config & getConfig() const;
	
	const CMapGenOptions& getMapGenOptions() const;
	
	std::unique_ptr<CMap> generate();

	int getNextMonlithIndex();
	int getPrisonsRemaining() const;
	std::shared_ptr<CZonePlacer> getZonePlacer() const;
	const std::vector<ArtifactID> & getAllPossibleQuestArtifacts() const;
	const std::vector<HeroTypeID> getAllPossibleHeroes() const;
	void banQuestArt(const ArtifactID & id);
	void unbanQuestArt(const ArtifactID & id);
	Zone * getZoneWater() const;
	void addWaterTreasuresInfo();

	int getRandomSeed() const;
	
private:
	std::unique_ptr<vstd::RNG> rand;
	int randomSeed;
	CMapGenOptions& mapGenOptions;
	Config config;
	std::unique_ptr<RmgMap> map;
	std::shared_ptr<CZonePlacer> placer;
	
	std::vector<rmg::ZoneConnection> connectionsLeft;
	
	int monolithIndex;
	std::vector<ArtifactID> questArtifacts;

	/// Generation methods
	void loadConfig();
	
	MetaString getMapDescription() const;

	void initPrisonsRemaining();
	void initQuestArtsRemaining();
	void addPlayerInfo();
	void addHeaderInfo();
	void genZones();
	void fillZones();
};

VCMI_LIB_NAMESPACE_END
