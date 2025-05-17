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
#include "../callback/CNonConstInfoCallback.h"
#include "../callback/GameCallbackHolder.h"
#include "../entities/artifact/EArtifactClass.h"
#include "../LoadProgress.h"

#include "RumorState.h"
#include "GameStatistics.h"

VCMI_LIB_NAMESPACE_BEGIN

class EVictoryLossCheckResult;
class Services;
class IMapService;
class CMap;
class CSaveFile;
class CLoadFile;
struct CPackForClient;
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

class DLL_LINKAGE CGameState : public CNonConstInfoCallback, public Serializeable, public GameCallbackHolder
{
	friend class CGameStateCampaign;
	friend class CMapLoaderH3M;

	std::shared_ptr<StartInfo> initialOpts; //copy of settings received from pregame (not randomized)
	std::shared_ptr<StartInfo> scenarioOps;
	std::unique_ptr<CMap> map;

	void saveCompatibilityRegisterMissingArtifacts();
public:
	ArtifactInstanceID saveCompatibilityLastAllocatedArtifactID;
	std::vector<std::shared_ptr<CArtifactInstance>> saveCompatibilityUnregisteredArtifacts;

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

	CGameState(IGameInfoCallback * callback);
	virtual ~CGameState();

	CGameState & gameState() final { return *this; }
	const CGameState & gameState() const final { return *this; }

	void preInit(Services * services);

	void init(const IMapService * mapService, StartInfo * si, vstd::RNG & randomGenerator, Load::ProgressAccumulator &, bool allowSavingRandomMap = true);
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
	HeroTypeID pickNextHeroType(vstd::RNG & randomGenerator, const PlayerColor & owner);

	void apply(CPackForClient & pack);
	BattleField battleGetBattlefieldType(int3 tile, vstd::RNG & randomGenerator);

	PlayerRelations getPlayerRelations(PlayerColor color1, PlayerColor color2) const override;
	bool checkForVisitableDir(const int3 & src, const int3 & dst) const; //check if src tile is visitable from dst tile
	void calculatePaths(const std::shared_ptr<PathfinderConfig> & config) const override;
	std::vector<const CGObjectInstance*> guardingCreatures (int3 pos) const;

	/// Gets a artifact ID randomly and removes the selected artifact from this handler.
	ArtifactID pickRandomArtifact(vstd::RNG & randomGenerator, std::optional<EArtifactClass> type);
	ArtifactID pickRandomArtifact(vstd::RNG & randomGenerator, std::function<bool(ArtifactID)> accepts);
	ArtifactID pickRandomArtifact(vstd::RNG & randomGenerator, std::optional<EArtifactClass> type, std::function<bool(ArtifactID)> accepts);
	ArtifactID pickRandomArtifact(vstd::RNG & randomGenerator, std::set<ArtifactID> filtered);

	/// Creates instance of spell scroll artifact with provided spell
	CArtifactInstance * createScroll(const SpellID & spellId);

	/// Creates instance of requested artifact
	/// For combined artifact this method will also create alll required components
	/// For scrolls this method will also initialize its spell
	CArtifactInstance * createArtifact(const ArtifactID & artId, const SpellID & spellId = SpellID::NONE);

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
	const IGameSettings & getSettings() const override;

	StartInfo * getStartInfo()
	{
		return scenarioOps.get();
	}
	const StartInfo * getStartInfo() const final
	{
		return scenarioOps.get();
	}
	const StartInfo * getInitialStartInfo() const
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

	bool isVisibleFor(int3 pos, const PlayerColor player) const override;
	bool isVisibleFor(const CGObjectInstance * obj, const PlayerColor player) const override;

	static int getDate(int day, Date mode);
	int getDate(Date mode=Date::DAY) const override; //mode=0 - total days in game, mode=1 - day of week, mode=2 - current week, mode=3 - current month

#if SCRIPTING_ENABLED
	scripting::Pool * getGlobalContextPool() const override;
#endif

	void saveGame(CSaveFile & file) const;
	void loadGame(CLoadFile & file);

	template <typename Handler> void serialize(Handler &h)
	{
		h & scenarioOps;
		h & initialOpts;
		h & actingPlayers;
		h & day;
		h & map;
		if (!h.hasFeature(Handler::Version::NO_RAW_POINTERS_IN_SERIALIZER))
			saveCompatibilityRegisterMissingArtifacts();
		h & players;
		h & teams;
		if (h.hasFeature(Handler::Version::NO_RAW_POINTERS_IN_SERIALIZER))
			h & *heroesPool;
		else
			h & heroesPool;
		h & globalEffects;
		h & currentRumor;
		h & campaign;
		h & allocatedArtifacts;
		h & statistic;

		if(!h.saving && h.loadingGamestate)
			restoreBonusSystemTree();
	}

private:
	// ----- initialization -----
	void initNewGame(const IMapService * mapService, vstd::RNG & randomGenerator, bool allowSavingRandomMap, Load::ProgressAccumulator & progressTracking);
	void initGlobalBonuses();
	void initGrailPosition(vstd::RNG & randomGenerator);
	void initRandomFactionsForPlayers(vstd::RNG & randomGenerator);
	void initOwnedObjects();
	void randomizeMapObjects(vstd::RNG & randomGenerator);
	void initPlayerStates();
	void placeStartingHeroes(vstd::RNG & randomGenerator);
	void placeStartingHero(const PlayerColor & playerColor, const HeroTypeID & heroTypeId, int3 townPos);
	void removeHeroPlaceholders();
	void initDifficulty();
	void initHeroes(vstd::RNG & randomGenerator);
	void placeHeroesInTowns();
	void initFogOfWar();
	void initStartingBonus(vstd::RNG & randomGenerator);
	void initTowns(vstd::RNG & randomGenerator);
	void initTownNames(vstd::RNG & randomGenerator);
	void initMapObjects(vstd::RNG & randomGenerator);
	void initVisitingAndGarrisonedHeroes();
	void initCampaign();

	// ----- bonus system handling -----

	void buildBonusSystemTree();
	void buildGlobalTeamPlayerTree();
	void restoreBonusSystemTree();

	// ---- misc helpers -----

	CGHeroInstance * getUsedHero(const HeroTypeID & hid) const;
	bool isUsedHero(const HeroTypeID & hid) const; //looks in heroes and prisons
	std::set<HeroTypeID> getUnusedAllowedHeroes(bool alsoIncludeNotAllowed = false) const;
	HeroTypeID pickUnusedHeroTypeRandomly(vstd::RNG & randomGenerator, const PlayerColor & owner); // picks a unused hero type randomly

	// ---- data -----
	Services * services;

	/// Pointer to campaign state manager. Nullptr for single scenarios
	std::unique_ptr<CGameStateCampaign> campaign;

	friend class IGameInfoCallback;
	friend class CMapHandler;
	friend class CGameHandler;
};

VCMI_LIB_NAMESPACE_END
