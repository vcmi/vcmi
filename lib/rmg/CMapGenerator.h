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

#include "../GameConstants.h"
#include "../CRandomGenerator.h"
#include "CMapGenOptions.h"
#include "../int3.h"
#include "CRmgTemplate.h"
#include "../LoadProgress.h"

VCMI_LIB_NAMESPACE_BEGIN

class CRmgTemplate;
class CMapGenOptions;
class JsonNode;
class RmgMap;
class CMap;
class Zone;
class CZonePlacer;

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
		std::vector<int> prisonExperience, prisonValues;
		std::vector<int> scrollValues;
		int pandoraMultiplierGold, pandoraMultiplierExperience, pandoraMultiplierSpells, pandoraSpellSchool, pandoraSpell60;
		std::vector<int> pandoraCreatureValues;
		std::vector<int> questValues, questRewardValues;
		bool singleThread;
	};
	
	explicit CMapGenerator(CMapGenOptions& mapGenOptions, int RandomSeed = std::time(nullptr));
	~CMapGenerator(); // required due to std::unique_ptr
	
	const Config & getConfig() const;
	
	const CMapGenOptions& getMapGenOptions() const;
	
	std::unique_ptr<CMap> generate();

	int getNextMonlithIndex();
	int getPrisonsRemaning() const;
	std::shared_ptr<CZonePlacer> getZonePlacer() const;
	const std::vector<ArtifactID> & getAllPossibleQuestArtifacts() const;
	void banWaterHeroes();
	const std::vector<HeroTypeID> getAllPossibleHeroes() const;
	void banQuestArt(const ArtifactID & id);
	void banHero(const HeroTypeID& id);

	Zone * getZoneWater() const;
	void addWaterTreasuresInfo();

	int getRandomSeed() const;
	
private:
	CRandomGenerator rand;
	int randomSeed;
	CMapGenOptions& mapGenOptions;
	Config config;
	std::unique_ptr<RmgMap> map;
	std::shared_ptr<CZonePlacer> placer;
	
	std::vector<rmg::ZoneConnection> connectionsLeft;
	
	int allowedPrisons;
	int monolithIndex;
	std::vector<ArtifactID> questArtifacts;

	/// Generation methods
	void loadConfig();
	
	std::string getMapDescription() const;

	void initPrisonsRemaining();
	void initQuestArtsRemaining();
	void addPlayerInfo();
	void addHeaderInfo();
	void genZones();
	void fillZones();
};

VCMI_LIB_NAMESPACE_END
