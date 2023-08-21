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

#include "bonuses/CBonusSystemNode.h"
#include "IGameCallback.h"
#include "LoadProgress.h"

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

template<typename T> class CApplier;
class CBaseForGSApply;

struct DLL_LINKAGE RumorState
{
	enum ERumorType : ui8
	{
		TYPE_NONE = 0, TYPE_RAND, TYPE_SPECIAL, TYPE_MAP
	};

	enum ERumorTypeSpecial : ui8
	{
		RUMOR_OBELISKS = 208,
		RUMOR_ARTIFACTS = 209,
		RUMOR_ARMY = 210,
		RUMOR_INCOME = 211,
		RUMOR_GRAIL = 212
	};

	ERumorType type;
	std::map<ERumorType, std::pair<int, int>> last;

	RumorState(){type = TYPE_NONE;};
	bool update(int id, int extra);

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & type;
		h & last;
	}
};

struct UpgradeInfo
{
	CreatureID oldID; //creature to be upgraded
	std::vector<CreatureID> newID; //possible upgrades
	std::vector<ResourceSet> cost; // cost[upgrade_serial] -> set of pairs<resource_ID,resource_amount>; cost is for single unit (not entire stack)
	UpgradeInfo(){oldID = CreatureID::NONE;};
};

class BattleInfo;

DLL_LINKAGE std::ostream & operator<<(std::ostream & os, const EVictoryLossCheckResult & victoryLossCheckResult);

class DLL_LINKAGE CGameState : public CNonConstInfoCallback
{
	friend class CGameStateCampaign;

public:
	//we have here all heroes available on this map that are not hired
	std::unique_ptr<TavernHeroesPool> heroesPool;

	CGameState();
	virtual ~CGameState();

	void preInit(Services * services);

	void init(const IMapService * mapService, StartInfo * si, bool allowSavingRandomMap = false, Load::Progress * progressTracking = nullptr);
	void updateOnLoad(StartInfo * si);

	ConstTransitivePtr<StartInfo> scenarioOps, initialOpts; //second one is a copy of settings received from pregame (not randomized)
	PlayerColor currentPlayer; //ID of player currently having turn
	ConstTransitivePtr<BattleInfo> curB; //current battle
	ui32 day; //total number of days in game
	ConstTransitivePtr<CMap> map;
	std::map<PlayerColor, PlayerState> players;
	std::map<TeamID, TeamState> teams;
	CBonusSystemNode globalEffects;
	RumorState rumor;

	static boost::shared_mutex mutex;

	void updateEntity(Metatype metatype, int32_t index, const JsonNode & data) override;

	bool giveHeroArtifact(CGHeroInstance * h, const ArtifactID & aid);

	void apply(CPack *pack);
	BattleField battleGetBattlefieldType(int3 tile, CRandomGenerator & rand);

	void fillUpgradeInfo(const CArmedInstance *obj, SlotID stackPos, UpgradeInfo &out) const override;
	PlayerRelations::PlayerRelations getPlayerRelations(PlayerColor color1, PlayerColor color2) const override;
	bool checkForVisitableDir(const int3 & src, const int3 & dst) const; //check if src tile is visitable from dst tile
	void calculatePaths(const CGHeroInstance *hero, CPathsInfo &out) override; //calculates possible paths for hero, by default uses current hero position and movement left; returns pointer to newly allocated CPath or nullptr if path does not exists
	void calculatePaths(const std::shared_ptr<PathfinderConfig> & config) override;
	int3 guardingCreaturePosition (int3 pos) const override;
	std::vector<CGObjectInstance*> guardingCreatures (int3 pos) const;
	void updateRumor();

	// ----- victory, loss condition checks -----

	EVictoryLossCheckResult checkForVictoryAndLoss(const PlayerColor & player) const;
	bool checkForVictory(const PlayerColor & player, const EventCondition & condition) const; //checks if given player is winner
	PlayerColor checkForStandardWin() const; //returns color of player that accomplished standard victory conditions or 255 (NEUTRAL) if no winner
	bool checkForStandardLoss(const PlayerColor & player) const; //checks if given player lost the game

	void obtainPlayersStats(SThievesGuildInfo & tgi, int level); //fills tgi with info about other players that is available at given level of thieves' guild

	bool isVisible(int3 pos, const std::optional<PlayerColor> & player) const override;
	bool isVisible(const CGObjectInstance * obj, const std::optional<PlayerColor> & player) const override;

	int getDate(Date::EDateType mode=Date::DAY) const override; //mode=0 - total days in game, mode=1 - day of week, mode=2 - current week, mode=3 - current month

	// ----- getters, setters -----

	/// This RNG should only be used inside GS or CPackForClient-derived applyGs
	/// If this doesn't work for your code that mean you need a new netpack
	///
	/// Client-side must use CRandomGenerator::getDefault which is not serialized
	///
	/// CGameHandler have it's own getter for CRandomGenerator::getDefault
	/// Any server-side code outside of GH must use CRandomGenerator::getDefault
	CRandomGenerator & getRandomGenerator();

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & scenarioOps;
		h & initialOpts;
		h & currentPlayer;
		h & day;
		h & map;
		h & players;
		h & teams;
		h & heroesPool;
		h & globalEffects;
		h & rand;
		h & rumor;
		h & campaign;

		BONUS_TREE_DESERIALIZATION_FIX
	}

private:
	// ----- initialization -----
	void preInitAuto();
	void initNewGame(const IMapService * mapService, bool allowSavingRandomMap, Load::Progress * progressTracking = nullptr);
	void checkMapChecksum();
	void initGlobalBonuses();
	void initGrailPosition();
	void initRandomFactionsForPlayers();
	void randomizeMapObjects();
	void randomizeObject(CGObjectInstance *cur);
	void initPlayerStates();
	void placeStartingHeroes();
	void placeStartingHero(const PlayerColor & playerColor, const HeroTypeID & heroTypeId, int3 townPos);
	void removeHeroPlaceholders();
	void initStartingResources();
	void initHeroes();
	void placeHeroesInTowns();
	void initFogOfWar();
	void initStartingBonus();
	void initTowns();
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
	std::pair<Obj,int> pickObject(CGObjectInstance *obj); //chooses type of object to be randomized, returns <type, subtype>
	HeroTypeID pickUnusedHeroTypeRandomly(const PlayerColor & owner); // picks a unused hero type randomly
	HeroTypeID pickNextHeroType(const PlayerColor & owner); // picks next free hero type of the H3 hero init sequence -> chosen starting hero, then unused hero type randomly
	UpgradeInfo fillUpgradeInfo(const CStackInstance &stack) const;

	// ---- data -----
	std::shared_ptr<CApplier<CBaseForGSApply>> applier;
	CRandomGenerator rand;
	Services * services;

	/// Ponter to campaign state manager. Nullptr for single scenarios
	std::unique_ptr<CGameStateCampaign> campaign;

	friend class IGameCallback;
	friend class CMapHandler;
	friend class CGameHandler;
};

VCMI_LIB_NAMESPACE_END
