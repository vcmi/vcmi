#pragma once

#include "CCreatureHandler.h"
#include "VCMI_Lib.h"

#include "HeroBonus.h"
#include "CCreatureSet.h"
#include "ConstTransitivePtr.h"
#include "IGameCallback.h"
#include "ResourceSet.h"
#include "int3.h"
#include "CRandomGenerator.h"
#include "CGameStateFwd.h"
#include "CPathfinder.h"

/*
 * CGameState.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

class CTown;
class CCallback;
class IGameCallback;
class CCreatureSet;
class CStack;
class CQuest;
class CGHeroInstance;
class CGTownInstance;
class CArmedInstance;
class CGDwelling;
class CObjectScript;
class CGObjectInstance;
class CCreature;
class CMap;
struct StartInfo;
struct SDL_Surface;
class CMapHandler;
struct SetObjectProperty;
struct MetaString;
struct CPack;
class CSpell;
struct TerrainTile;
class CHeroClass;
class CCampaign;
class CCampaignState;
class IModableArt;
class CGGarrison;
class CGameInfo;
struct QuestInfo;
class CQuest;
class CCampaignScenario;
struct EventCondition;
class CScenarioTravel;

namespace boost
{
	class shared_mutex;
}

struct DLL_LINKAGE SThievesGuildInfo
{
	std::vector<PlayerColor> playerColors; //colors of players that are in-game

	std::vector< std::vector< PlayerColor > > numOfTowns, numOfHeroes, gold, woodOre, mercSulfCrystGems, obelisks, artifacts, army, income; // [place] -> [colours of players]

	std::map<PlayerColor, InfoAboutHero> colorToBestHero; //maps player's color to his best heros'

    std::map<PlayerColor, EAiTactic::EAiTactic> personality; // color to personality // ai tactic
	std::map<PlayerColor, si32> bestCreature; // color to ID // id or -1 if not known

// 	template <typename Handler> void serialize(Handler &h, const int version)
// 	{
// 		h & playerColors & numOfTowns & numOfHeroes & gold & woodOre & mercSulfCrystGems & obelisks & artifacts & army & income;
// 		h & colorToBestHero & personality & bestCreature;
// 	}

};

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
		h & type & last;
	}
};

struct UpgradeInfo
{
	CreatureID oldID; //creature to be upgraded
	std::vector<CreatureID> newID; //possible upgrades
	std::vector<TResources> cost; // cost[upgrade_serial] -> set of pairs<resource_ID,resource_amount>; cost is for single unit (not entire stack)
	UpgradeInfo(){oldID = CreatureID::NONE;};
};

struct DLL_EXPORT DuelParameters
{
	ETerrainType terType;
	BFieldType bfieldType;
	struct DLL_EXPORT SideSettings
	{
		struct DLL_EXPORT StackSettings
		{
			CreatureID type;
			si32 count;
			template <typename Handler> void serialize(Handler &h, const int version)
			{
				h & type & count;
			}

			StackSettings();
			StackSettings(CreatureID Type, si32 Count);
		} stacks[GameConstants::ARMY_SIZE];

		si32 heroId; //-1 if none
		std::vector<si32> heroPrimSkills; //may be empty
		std::map<si32, CArtifactInstance*> artifacts;
		std::vector<std::pair<si32, si8> > heroSecSkills; //may be empty; pairs <id, level>, level [0-3]
		std::set<SpellID> spells;

		SideSettings();
		template <typename Handler> void serialize(Handler &h, const int version)
		{
			h & stacks & heroId & heroPrimSkills & artifacts & heroSecSkills & spells;
		}
	} sides[2];

	std::vector<std::shared_ptr<CObstacleInstance> > obstacles;

	static DuelParameters fromJSON(const std::string &fname);

	struct CusomCreature
	{
		int id;
		int attack, defense, dmg, HP, speed, shoots;

		CusomCreature()
		{
			id = attack = defense = dmg = HP = speed = shoots = -1;
		}
		template <typename Handler> void serialize(Handler &h, const int version)
		{
			h & id & attack & defense & dmg & HP & speed & shoots;
		}
	};

	std::vector<CusomCreature> creatures;

	DuelParameters();
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & terType & bfieldType & sides & obstacles & creatures;
	}
};

struct BattleInfo;

DLL_LINKAGE std::ostream & operator<<(std::ostream & os, const EVictoryLossCheckResult & victoryLossCheckResult);

class DLL_LINKAGE CGameState : public CNonConstInfoCallback
{
public:
	struct DLL_LINKAGE HeroesPool
	{
		std::map<ui32, ConstTransitivePtr<CGHeroInstance> > heroesPool; //[subID] - heroes available to buy; nullptr if not available
		std::map<ui32,ui8> pavailable; // [subid] -> which players can recruit hero (binary flags)

		CGHeroInstance * pickHeroFor(bool native, PlayerColor player, const CTown *town,
			std::map<ui32, ConstTransitivePtr<CGHeroInstance> > &available, CRandomGenerator & rand, const CHeroClass *bannedClass = nullptr) const;

		template <typename Handler> void serialize(Handler &h, const int version)
		{
			h & heroesPool & pavailable;
		}
	} hpool; //we have here all heroes available on this map that are not hired

	CGameState();
	virtual ~CGameState();

	void init(StartInfo * si);

	ConstTransitivePtr<StartInfo> scenarioOps, initialOpts; //second one is a copy of settings received from pregame (not randomized)
	PlayerColor currentPlayer; //ID of player currently having turn
	ConstTransitivePtr<BattleInfo> curB; //current battle
	ui32 day; //total number of days in game
	ConstTransitivePtr<CMap> map;
	std::map<PlayerColor, PlayerState> players;
	std::map<TeamID, TeamState> teams;
	CBonusSystemNode globalEffects;
	RumorState rumor;

	boost::shared_mutex *mx;

	void giveHeroArtifact(CGHeroInstance *h, ArtifactID aid);

	void apply(CPack *pack);
	BFieldType battleGetBattlefieldType(int3 tile);
	UpgradeInfo getUpgradeInfo(const CStackInstance &stack);
	PlayerRelations::PlayerRelations getPlayerRelations(PlayerColor color1, PlayerColor color2);
	bool checkForVisitableDir(const int3 & src, const int3 & dst) const; //check if src tile is visitable from dst tile
	void calculatePaths(const CGHeroInstance *hero, CPathsInfo &out); //calculates possible paths for hero, by default uses current hero position and movement left; returns pointer to newly allocated CPath or nullptr if path does not exists
	int3 guardingCreaturePosition (int3 pos) const;
	std::vector<CGObjectInstance*> guardingCreatures (int3 pos) const;
	void updateRumor();

	// ----- victory, loss condition checks -----

	EVictoryLossCheckResult checkForVictoryAndLoss(PlayerColor player) const;
	bool checkForVictory(PlayerColor player, const EventCondition & condition) const; //checks if given player is winner
	PlayerColor checkForStandardWin() const; //returns color of player that accomplished standard victory conditions or 255 (NEUTRAL) if no winner
	bool checkForStandardLoss(PlayerColor player) const; //checks if given player lost the game

	void obtainPlayersStats(SThievesGuildInfo & tgi, int level); //fills tgi with info about other players that is available at given level of thieves' guild
	std::map<ui32, ConstTransitivePtr<CGHeroInstance> > unusedHeroesFromPool(); //heroes pool without heroes that are available in taverns
	BattleInfo * setupBattle(int3 tile, const CArmedInstance *armies[2], const CGHeroInstance * heroes[2], bool creatureBank, const CGTownInstance *town);

	bool isVisible(int3 pos, PlayerColor player);
	bool isVisible(const CGObjectInstance *obj, boost::optional<PlayerColor> player);

	int getDate(Date::EDateType mode=Date::DAY) const; //mode=0 - total days in game, mode=1 - day of week, mode=2 - current week, mode=3 - current month

	// ----- getters, setters -----
	CRandomGenerator & getRandomGenerator();

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & scenarioOps & initialOpts & currentPlayer & day & map & players & teams & hpool & globalEffects & rand;
		if(version >= 755) //save format backward compatibility
		{
			h & rumor;
		}
		else if(!h.saving)
			rumor = RumorState();

		BONUS_TREE_DESERIALIZATION_FIX
	}

private:
	struct CrossoverHeroesList
	{
		std::vector<CGHeroInstance *> heroesFromPreviousScenario, heroesFromAnyPreviousScenarios;
		void addHeroToBothLists(CGHeroInstance * hero);
		void removeHeroFromBothLists(CGHeroInstance * hero);
	};

	struct CampaignHeroReplacement
	{
		CampaignHeroReplacement(CGHeroInstance * hero, ObjectInstanceID heroPlaceholderId);
		CGHeroInstance * hero;
		ObjectInstanceID heroPlaceholderId;
	};

	// ----- initialization -----

	void initNewGame();
	void initCampaign();
	void initDuel();
	void checkMapChecksum();
	void initGrailPosition();
	void initRandomFactionsForPlayers();
	void randomizeMapObjects();
	void randomizeObject(CGObjectInstance *cur);
	void initPlayerStates();
	void placeCampaignHeroes();
	CrossoverHeroesList getCrossoverHeroesFromPreviousScenarios() const;

	/// returns heroes and placeholders in where heroes will be put
	std::vector<CampaignHeroReplacement> generateCampaignHeroesToReplace(CrossoverHeroesList & crossoverHeroes);

	/// gets prepared and copied hero instances with crossover heroes from prev. scenario and travel options from current scenario
	void prepareCrossoverHeroes(std::vector<CampaignHeroReplacement> & campaignHeroReplacements, const CScenarioTravel & travelOptions) const;

	void replaceHeroesPlaceholders(const std::vector<CampaignHeroReplacement> & campaignHeroReplacements);
	void placeStartingHeroes();
	void placeStartingHero(PlayerColor playerColor, HeroTypeID heroTypeId, int3 townPos);
	void initStartingResources();
	void initHeroes();
	void giveCampaignBonusToHero(CGHeroInstance * hero);
	void initFogOfWar();
	void initStartingBonus();
	void initTowns();
	void initMapObjects();
	void initVisitingAndGarrisonedHeroes();

	// ----- bonus system handling -----

	void buildBonusSystemTree();
	void attachArmedObjects();
	void buildGlobalTeamPlayerTree();
	void deserializationFix();

	// ---- misc helpers -----

	CGHeroInstance * getUsedHero(HeroTypeID hid) const;
	bool isUsedHero(HeroTypeID hid) const; //looks in heroes and prisons
	std::set<HeroTypeID> getUnusedAllowedHeroes(bool alsoIncludeNotAllowed = false) const;
	std::pair<Obj,int> pickObject(CGObjectInstance *obj); //chooses type of object to be randomized, returns <type, subtype>
	int pickUnusedHeroTypeRandomly(PlayerColor owner); // picks a unused hero type randomly
	int pickNextHeroType(PlayerColor owner); // picks next free hero type of the H3 hero init sequence -> chosen starting hero, then unused hero type randomly

	// ---- data -----
	CRandomGenerator rand;

	friend class CCallback;
	friend class CClient;
	friend class IGameCallback;
	friend class CMapHandler;
	friend class CGameHandler;
};
