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
#include "ZonePlacementConfig.h"
#include "../LoadProgress.h"

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
		ZonePlacementConfig zonePlacement;
	};
	
	explicit CMapGenerator(CMapGenOptions& mapGenOptions, IGameInfoCallback * cb, int RandomSeed);
	~CMapGenerator(); // required due to std::unique_ptr
	
	const Config & getConfig() const;
	
	const CMapGenOptions& getMapGenOptions() const;
	
	std::unique_ptr<CMap> generate();

	int getNextMonlithIndex();

	/// True if a subterranean gate pair at the given tiles would keep every gate paired with its intended
	/// partner. Use it while searching for gate positions; the result may go stale once another thread
	/// reserves, so reserveGatePair still has to be checked.
	bool canReserveGatePair(const int3 & posA, const int3 & posB) const;

	/// Reserve a subterranean gate pair at the given tiles (on different levels). In-game gates pair by
	/// nearest 2D distance (CGSubterraneanGate::postInit), so a pair must stay each other's nearest gate.
	/// Commits the pair and returns true, or returns false if it would break some pairing. Thread-safe;
	/// called from the parallel connection-placement phase. posA/posB are final map positions.
	bool reserveGatePair(const int3 & posA, const int3 & posB);

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

	// Subterranean gate pairing registry (see reserveGatePair). Each entry is a placed gate and the squared
	// 2D distance to its committed partner; used to guarantee gate pairs keep the intended pairing.
	struct ReservedGate { int3 pos; int pairingDistanceSqr; };
	std::vector<ReservedGate> reservedGates;
	mutable std::mutex gateReservationMutex;

	/// canReserveGatePair without taking the lock.
	bool canReserveGatePairLocked(const int3 & posA, const int3 & posB) const;

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
