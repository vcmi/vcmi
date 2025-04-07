/*
 * CGameState.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../bonuses/CBonusSystemNode.h"
#include "../IGameCallback.h"
#include "../LoadProgress.h"

#include "RumorState.h"
#include "GameStatistics.h"

namespace boost
{
class shared_mutex;
}

VCMI_LIB_NAMESPACE_BEGIN

class EVictoryLossCheckResult;
class Services;
class IMapService;
class CMap;
struct CPack;
class CHeroClass;
struct EventCondition;
struct CampaignTravel;
class CStackInstance;
class CGameStateCampaign;
class TavernHeroesPool;
struct SThievesGuildInfo;
class CRandomGenerator;
class GameSettings;
class BattleInfo;
class UpgradeInfo;

DLL_LINKAGE std::ostream & operator<<(std::ostream & os, const EVictoryLossCheckResult & victoryLossCheckResult);

class DLL_LINKAGE CGameState : public CNonConstInfoCallback, public Serializeable
{
	friend class CGameStateCampaign;

	std::unique_ptr<StartInfo> initialOpts; //copy of settings received from pregame (not randomized)
	std::unique_ptr<StartInfo> scenarioOps;
	std::unique_ptr<CMap> map;
public:
	/// Stores number of times each artifact was placed on map via randomization
	std::map<ArtifactID, int> allocatedArtifacts;

	/// List of currently ongoing battles
	std::vector<std::unique_ptr<BattleInfo>> currentBattles;
	/// ID that can be allocated to next battle
	BattleID nextBattleID = BattleID(0);

	//we have here all heroes available on this map that are not hired
	std::unique_ptr<TavernHeroesPool> heroesPool;

	/// list of players currently making turn. Usually - just one, except for simturns
	std::set<PlayerColor> actingPlayers;

	IGameCallback * callback;

	CGameState();
	virtual ~CGameState();

	void preInit(Services * services, IGameCallback * callback);

	void init(const IMapService * mapService, StartInfo * si, Load::ProgressAccumulator &, bool allowSavingRandomMap = true);
	void updateOnLoad(StartInfo * si);

	ui32 day; //total number of days in game
	std::map<PlayerColor, PlayerState> players;
	std::map<TeamID, TeamState> teams;
	CBonusSystemNode globalEffects;
	RumorState currentRumor;

	StatisticDataSet statistic;

	// NOTE: effectively AI mutex, only used by adventure map AI
	static std::shared_mutex mutex;

	void updateEntity(Metatype metatype, int32_t index, const JsonNode & data) override;

	bool giveHeroArtifact(CGHeroInstance * h, const ArtifactID & aid);
	/// picks next free hero type of the H3 hero init sequence -> chosen starting hero, then unused hero type randomly
	HeroTypeID pickNextHeroType(const PlayerColor & owner);

	void apply(CPackForClient & pack);
	BattleField battleGetBattlefieldType(int3 tile, vstd::RNG & rand);

	void fillUpgradeInfo(const CArmedInstance *obj, SlotID stackPos, UpgradeInfo &out) const override;
	PlayerRelations getPlayerRelations(PlayerColor color1, PlayerColor color2) const override;
	bool checkForVisitableDir(const int3 & src, const int3 & dst) const; //check if src tile is visitable from dst tile
	void calculatePaths(const std::shared_ptr<PathfinderConfig> & config) const override;
	int3 guardingCreaturePosition (int3 pos) const override;
	std::vector<CGObjectInstance*> guardingCreatures (int3 pos) const;

	/// Gets a artifact ID randomly and removes the selected artifact from this handler.
	ArtifactID pickRandomArtifact(vstd::RNG & rand, int flags);
	ArtifactID pickRandomArtifact(vstd::RNG & rand, std::function<bool(ArtifactID)> accepts);
	ArtifactID pickRandomArtifact(vstd::RNG & rand, int flags, std::function<bool(ArtifactID)> accepts);
	ArtifactID pickRandomArtifact(vstd::RNG & rand, std::set<ArtifactID> filtered);

	/// Returns battle in which selected player is engaged, or nullptr if none.
	/// Can NOT be used with neutral player, use battle by ID instead
	const BattleInfo * getBattle(const PlayerColor & player) const;
	/// Returns battle by its unique identifier, or nullptr if not found
	const BattleInfo * getBattle(const BattleID & battle) const;
	BattleInfo * getBattle(const BattleID & battle);

	// ----- victory, loss condition checks -----

	EVictoryLossCheckResult checkForVictoryAndLoss(const PlayerColor & player) const;
	bool checkForVictory(const PlayerColor & player, const EventCondition & condition) const; //checks if given player is winner
	PlayerColor checkForStandardWin() const; //returns color of player that accomplished standard victory conditions or 255 (NEUTRAL) if no winner
	bool checkForStandardLoss(const PlayerColor & player) const; //checks if given player lost the game

	void obtainPlayersStats(SThievesGuildInfo & tgi, int level); //fills tgi with info about other players that is available at given level of thieves' guild
	const IGameSettings & getSettings() const;

	StartInfo * getStartInfo()
	{
		return scenarioOps.get();
	}
	const StartInfo * getStartInfo() const final
	{
		return scenarioOps.get();
	}
	const StartInfo * getInitialStartInfo() const final
	{
		return initialOpts.get();
	}

	CMap & getMap()
	{
		return *map;
	}
	const CMap & getMap() const
	{
		return *map;
	}

	bool isVisible(int3 pos, const std::optional<PlayerColor> & player) const override;
	bool isVisible(const CGObjectInstance * obj, const std::optional<PlayerColor> & player) const override;

	static int getDate(int day, Date mode);
	int getDate(Date mode=Date::DAY) const override; //mode=0 - total days in game, mode=1 - day of week, mode=2 - current week, mode=3 - current month

	// ----- getters, setters -----

	/// This RNG should only be used inside GS or CPackForClient-derived applyGs
	/// If this doesn't work for your code that mean you need a new netpack
	///
	/// Client-side must use vstd::RNG::getDefault which is not serialized
	///
	/// CGameHandler have it's own getter for vstd::RNG::getDefault
	/// Any server-side code outside of GH must use vstd::RNG::getDefault
	vstd::RNG & getRandomGenerator();

	template <typename Handler> void serialize(Handler &h)
	{
		h & scenarioOps;
		h & initialOpts;
		h & actingPlayers;
		h & day;
		h & map;
		h & players;
		h & teams;
		h & heroesPool;
		h & globalEffects;
		h & currentRumor;
		h & campaign;
		h & allocatedArtifacts;
		h & statistic;

		BONUS_TREE_DESERIALIZATION_FIX
	}

private:
	// ----- initialization -----
	void initNewGame(const IMapService * mapService, bool allowSavingRandomMap, Load::ProgressAccumulator & progressTracking);
	void initGlobalBonuses();
	void initGrailPosition();
	void initRandomFactionsForPlayers();
	void initOwnedObjects();
	void randomizeMapObjects();
	void initPlayerStates();
	void placeStartingHeroes();
	void placeStartingHero(const PlayerColor & playerColor, const HeroTypeID & heroTypeId, int3 townPos);
	void removeHeroPlaceholders();
	void initDifficulty();
	void initHeroes();
	void placeHeroesInTowns();
	void initFogOfWar();
	void initStartingBonus();
	void initTowns();
	void initTownNames();
	void initMapObjects();
	void initVisitingAndGarrisonedHeroes();
	void initCampaign();

	// ----- bonus system handling -----

	void buildBonusSystemTree();
	void attachArmedObjects();
	void buildGlobalTeamPlayerTree();
	void deserializationFix();

	// ---- misc helpers -----

	CGHeroInstance * getUsedHero(const HeroTypeID & hid) const;
	bool isUsedHero(const HeroTypeID & hid) const; //looks in heroes and prisons
	std::set<HeroTypeID> getUnusedAllowedHeroes(bool alsoIncludeNotAllowed = false) const;
	HeroTypeID pickUnusedHeroTypeRandomly(const PlayerColor & owner); // picks a unused hero type randomly
	UpgradeInfo fillUpgradeInfo(const CStackInstance &stack) const;

	// ---- data -----
	Services * services;

	/// Pointer to campaign state manager. Nullptr for single scenarios
	std::unique_ptr<CGameStateCampaign> campaign;

	friend class IGameCallback;
	friend class CMapHandler;
	friend class CGameHandler;
};

VCMI_LIB_NAMESPACE_END
