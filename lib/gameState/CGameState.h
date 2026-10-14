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

#include "GameStatistics.h"
#include "ReplayLog.h"
#include "RumorState.h"
#include "mapObjects/CGObjectInstance.h"

#include <vcmi/Environment.h>

class EVictoryLossCheckResult;
class Services;
class IGameRandomizer;
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

namespace scripting
{
class MapEventDispatcher;
}

DLL_LINKAGE std::ostream & operator<<(std::ostream & os, const EVictoryLossCheckResult & victoryLossCheckResult);

class GameStateEnvironment final : public Environment
{
	CGameState & owner;
public:
	explicit GameStateEnvironment(CGameState & owner)
		:owner(owner)
	{}

	const Services * services() const final;
	const BattleCb * battle(const BattleID & battleID) const final;
	const GameCb * game() const final;
};

class DLL_LINKAGE CGameState : public CNonConstInfoCallback, public Serializeable
{
	friend class CGameStateCampaign;

	std::shared_ptr<StartInfo> initialOpts; //copy of settings received from pregame (not randomized)
	std::shared_ptr<StartInfo> scenarioOps;
	std::unique_ptr<CMap> map;

	std::unique_ptr<GameStateEnvironment> scriptingEnvironment;
	std::unique_ptr<scripting::Pool> scriptingPool;
	/// Runs the map's converted event script; null when the map has no event system. Server-only, rebuilt on load.
	std::unique_ptr<scripting::MapEventDispatcher> mapEventDispatcher;

public:
	/// List of currently ongoing battles
	std::vector<std::unique_ptr<BattleInfo>> currentBattles;
	/// ID that can be allocated to next battle
	BattleID nextBattleID = BattleID(0);

	//we have here all heroes available on this map that are not hired
	std::unique_ptr<TavernHeroesPool> heroesPool;

	/// list of players currently making turn. Usually - just one, except for simturns
	std::set<PlayerColor> actingPlayers;

	CGameState();
	virtual ~CGameState();

	CGameState & gameState() final { return *this; }
	const CGameState & gameState() const final { return *this; }

	void preInit(Services * services);

	void init(const IMapService * mapService, StartInfo * si, IGameRandomizer & gameRandomizer, Load::ProgressAccumulator &, bool allowSavingRandomMap = true);
	void updateOnLoad(const StartInfo & si);

	ui32 day; //total number of days in game
	std::map<PlayerColor, PlayerState> players;
	std::map<TeamID, TeamState> teams;
	CBonusSystemNode globalEffects;
	RumorState currentRumor;

	/// recording of this game, filled by the server and stored as part of every savegame
	ReplayLog replayLog;

	// NOTE: effectively AI mutex, only used by adventure map AI
	static std::shared_mutex mutex;

	void updateEntity(Metatype metatype, int32_t index, const JsonNode & data) override;

	bool giveHeroArtifact(CGHeroInstance * h, const ArtifactID & aid);
	/// picks next free hero type of the H3 hero init sequence -> chosen starting hero, then unused hero type randomly
	HeroTypeID pickNextHeroType(vstd::RNG & randomGenerator, const PlayerColor & owner);

	void apply(CPackForClient & pack);
	BattleField battleGetBattlefieldType(int3 tile, vstd::RNG & randomGenerator) const;

	PlayerRelations getPlayerRelations(PlayerColor color1, PlayerColor color2) const override;
	void calculatePaths(const std::shared_ptr<PathfinderConfig> & config) const override;
	std::vector<const CGObjectInstance*> guardingCreatures (int3 pos) const;

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

	void markObjectControlled(PlayerColor player, ObjectInstanceID id);
	bool hasEverControlled(PlayerColor player, ObjectInstanceID id) const;

	//fills tgi with info about other players that is available at given level of thieves' guild
	void obtainPlayersStats(SThievesGuildInfo & tgi, int level) const;
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
	void setSaveDirectory(const std::string & value);

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
	template<class BoolPredicate>
	static bool iteratePositionsUntilTrue(const CGObjectInstance * obj, BoolPredicate && boolPredicate)
	{
		for(int fy = 0; fy < obj->getHeight(); ++fy)
		{
			for(int fx = 0; fx < obj->getWidth(); ++fx)
			{
				int3 pos = obj->anchorPos() + int3(-fx, -fy, 0);
				if(boolPredicate(pos))
					return true;
			}
		}
		return false;
	}

	Calendar getCalendar() const override;

	const scripting::Pool & getScriptContextPool() const final;
	const Environment & getScriptingEnvironment() const { return *scriptingEnvironment; }
	/// Null when the map has no event script - callers must check.
	scripting::MapEventDispatcher * getMapEventDispatcher() const { return mapEventDispatcher.get(); }

	void saveGame(CSaveFile & file) const;
	void loadGame(CLoadFile & file);

	/// Serializes the whole gamestate into a memory buffer. The replay log is deliberately left
	/// out - a snapshot lives inside that very log, and including it would nest snapshots.
	std::vector<std::byte> saveToMemory();

	/// Replaces contents of this gamestate with a snapshot made by saveToMemory().
	/// Identity of this object is preserved, so all shared_ptr's to it stay valid.
	void loadFromMemory(std::vector<std::byte> data);

	template <typename Handler> void serialize(Handler &h)
	{
		h & scenarioOps;
		h & initialOpts;
		h & actingPlayers;
		h & day;
		h & map;
		h & players;
		h & teams;
		h & *heroesPool;
		h & globalEffects;
		h & currentRumor;
		h & campaign;

		if(h.hasFeature(Handler::Version::GAME_REPLAY_RECORDING))
		{
			// battles never survive a day boundary, but the counter must - a recorded
			// BattleStart carries the ID the server had handed out when it was recorded
			h & nextBattleID;
			h & replayLog;
		}

		if(!h.saving && h.loadingGamestate)
			restoreBonusSystemTree();
	}

private:
	// ----- initialization -----
	void initNewGame(const IMapService * mapService, vstd::RNG & randomGenerator, bool allowSavingRandomMap, Load::ProgressAccumulator & progressTracking);
	void initGlobalBonuses();
	void initScriptVariables();
	void initGrailPosition(vstd::RNG & randomGenerator);
	void initRandomFactionsForPlayers(vstd::RNG & randomGenerator);
	void initOwnedObjects();
	void randomizeMapObjects(IGameRandomizer & gameRandomizer);
	void rebuildObjectControlHistory();
	void initPlayerStates();
	void placeStartingHeroes(vstd::RNG & randomGenerator);
	void placeStartingHero(const PlayerColor & playerColor, const HeroTypeID & heroTypeId, int3 townPos);
	void removeHeroPlaceholders();
	void initDifficulty();
	void adjustObjectsToMapBounds();
	void initHeroes(IGameRandomizer & gameRandomizer);
	void placeHeroesInTowns();
	void initFogOfWar();
	void initStartingBonus(IGameRandomizer & gameRandomizer);
	void initTowns(vstd::RNG & randomGenerator);
	void initTownNames(vstd::RNG & randomGenerator);
	void initMapObjects(IGameRandomizer & gameRandomizer);
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
	bool isHeroAllowedForPlayer(const HeroTypeID & hid, const PlayerColor & owner);

	// ---- data -----
	Services * services;

	/// Pointer to campaign state manager. Nullptr for single scenarios
	std::unique_ptr<CGameStateCampaign> campaign;

	friend class IGameInfoCallback;
	friend class CMapHandler;
	friend class CGameHandler;
};
