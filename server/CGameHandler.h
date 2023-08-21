/*
 * CGameHandler.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include <vcmi/Environment.h>

#include "../lib/FunctionList.h"
#include "../lib/IGameCallback.h"
#include "../lib/battle/CBattleInfoCallback.h"
#include "../lib/battle/BattleAction.h"
#include "../lib/LoadProgress.h"
#include "../lib/ScriptHandler.h"
#include "CQuery.h"

VCMI_LIB_NAMESPACE_BEGIN

class CGameState;
struct StartInfo;
struct BattleResult;
struct SideInBattle;
struct BattleAttack;
struct BattleStackAttacked;
struct CPack;
struct Query;
struct SetResources;
struct NewStructures;
class CGHeroInstance;
class IMarket;

class SpellCastEnvironment;

#if SCRIPTING_ENABLED
namespace scripting
{
	class PoolImpl;
}
#endif


template<typename T> class CApplier;

VCMI_LIB_NAMESPACE_END

class HeroPoolProcessor;
class CGameHandler;
class CVCMIServer;
class CBaseForGHApply;
class PlayerMessageProcessor;

struct PlayerStatus
{
	bool makingTurn;

	PlayerStatus():makingTurn(false){};
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & makingTurn;
	}
};
class PlayerStatuses
{
public:
	std::map<PlayerColor,PlayerStatus> players;
	boost::mutex mx;
	boost::condition_variable cv; //notifies when any changes are made

	void addPlayer(PlayerColor player);
	PlayerStatus operator[](PlayerColor player);
	bool checkFlag(PlayerColor player, bool PlayerStatus::*flag);
	void setFlag(PlayerColor player, bool PlayerStatus::*flag, bool val);
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & players;
	}
};

struct CasualtiesAfterBattle
{
	using TStackAndItsNewCount = std::pair<StackLocation, int>;
	using TSummoned = std::map<CreatureID, TQuantity>;
	enum {ERASE = -1};
	const CArmedInstance * army;
	std::vector<TStackAndItsNewCount> newStackCounts;
	std::vector<ArtifactLocation> removedWarMachines;
	TSummoned summoned;
	ObjectInstanceID heroWithDeadCommander; //TODO: unify stack locations

	CasualtiesAfterBattle(const SideInBattle & battleSide, const BattleInfo * bat);
	void updateArmy(CGameHandler *gh);
};

class CGameHandler : public IGameCallback, public CBattleInfoCallback, public Environment
{
	CVCMIServer * lobby;
	std::shared_ptr<CApplier<CBaseForGHApply>> applier;
	std::unique_ptr<boost::thread> battleThread;

public:
	boost::recursive_mutex battleActionMutex;

	std::unique_ptr<HeroPoolProcessor> heroPool;

	using FireShieldInfo = std::vector<std::pair<const CStack *, int64_t>>;
	//use enums as parameters, because doMove(sth, true, false, true) is not readable
	enum EGuardLook {CHECK_FOR_GUARDS, IGNORE_GUARDS};
	enum EVisitDest {VISIT_DEST, DONT_VISIT_DEST};
	enum ELEaveTile {LEAVING_TILE, REMAINING_ON_TILE};

	std::unique_ptr<PlayerMessageProcessor> playerMessages;

	std::map<PlayerColor, std::set<std::shared_ptr<CConnection>>> connections; //player color -> connection to client with interface of that player
	PlayerStatuses states; //player color -> player state

	//queries stuff
	boost::recursive_mutex gsm;
	ui32 QID;
	Queries queries;

	SpellCastEnvironment * spellEnv;

	const Services * services() const override;
	const BattleCb * battle() const override;
	const GameCb * game() const override;
	vstd::CLoggerBase * logger() const override;
	events::EventBus * eventBus() const override;
	CVCMIServer * gameLobby() const;

	bool isValidObject(const CGObjectInstance *obj) const;
	bool isBlockedByQueries(const CPack *pack, PlayerColor player);
	bool isAllowedExchange(ObjectInstanceID id1, ObjectInstanceID id2);
	void giveSpells(const CGTownInstance *t, const CGHeroInstance *h);
	int moveStack(int stack, BattleHex dest); //returned value - travelled distance
	void runBattle();

	////used only in endBattle - don't touch elsewhere
	bool visitObjectAfterVictory;
	//
	void endBattle(int3 tile, const CGHeroInstance * hero1, const CGHeroInstance * hero2); //ends battle
	void endBattleConfirm(const BattleInfo * battleInfo);

	void makeAttack(const CStack * attacker, const CStack * defender, int distance, BattleHex targetHex, bool first, bool ranged, bool counter);

	// damage, drain life & fire shield; returns amount of drained life
	int64_t applyBattleEffects(BattleAttack & bat, std::shared_ptr<battle::CUnitState> attackerState, FireShieldInfo & fireShield, const CStack * def, int distance, bool secondary);

	void sendGenericKilledLog(const CStack * defender, int32_t killed, bool multiple);
	void addGenericKilledLog(BattleLogMessage & blm, const CStack * defender, int32_t killed, bool multiple);

	void checkBattleStateChanges();
	void setupBattle(int3 tile, const CArmedInstance *armies[2], const CGHeroInstance *heroes[2], bool creatureBank, const CGTownInstance *town);
	void setBattleResult(BattleResult::EResult resultType, int victoriusSide);

	CGameHandler() = default;
	CGameHandler(CVCMIServer * lobby);
	~CGameHandler();

	//////////////////////////////////////////////////////////////////////////
	//from IGameCallback
	//do sth
	void changeSpells(const CGHeroInstance * hero, bool give, const std::set<SpellID> &spells) override;
	bool removeObject(const CGObjectInstance * obj) override;
	void createObject(const int3 & visitablePosition, Obj type, int32_t subtype ) override;
	void setOwner(const CGObjectInstance * obj, PlayerColor owner) override;
	void changePrimSkill(const CGHeroInstance * hero, PrimarySkill::PrimarySkill which, si64 val, bool abs=false) override;
	void changeSecSkill(const CGHeroInstance * hero, SecondarySkill which, int val, bool abs=false) override;

	void showBlockingDialog(BlockingDialog *iw) override;
	void showTeleportDialog(TeleportDialog *iw) override;
	void showGarrisonDialog(ObjectInstanceID upobj, ObjectInstanceID hid, bool removableUnits) override;
	void showThievesGuildWindow(PlayerColor player, ObjectInstanceID requestingObjId) override;
	void giveResource(PlayerColor player, GameResID which, int val) override;
	void giveResources(PlayerColor player, TResources resources) override;

	void giveCreatures(const CArmedInstance *objid, const CGHeroInstance * h, const CCreatureSet &creatures, bool remove) override;
	void takeCreatures(ObjectInstanceID objid, const std::vector<CStackBasicDescriptor> &creatures) override;
	bool changeStackType(const StackLocation &sl, const CCreature *c) override;
	bool changeStackCount(const StackLocation &sl, TQuantity count, bool absoluteValue = false) override;
	bool insertNewStack(const StackLocation &sl, const CCreature *c, TQuantity count) override;
	bool eraseStack(const StackLocation &sl, bool forceRemoval = false) override;
	bool swapStacks(const StackLocation &sl1, const StackLocation &sl2) override;
	bool addToSlot(const StackLocation &sl, const CCreature *c, TQuantity count) override;
	void tryJoiningArmy(const CArmedInstance *src, const CArmedInstance *dst, bool removeObjWhenFinished, bool allowMerging) override;
	bool moveStack(const StackLocation &src, const StackLocation &dst, TQuantity count = -1) override;

	void removeAfterVisit(const CGObjectInstance *object) override;

	bool giveHeroNewArtifact(const CGHeroInstance * h, const CArtifact * artType, ArtifactPosition pos = ArtifactPosition::FIRST_AVAILABLE) override;
	bool giveHeroArtifact(const CGHeroInstance * h, const CArtifactInstance * a, ArtifactPosition pos) override;
	void putArtifact(const ArtifactLocation &al, const CArtifactInstance *a) override;
	void removeArtifact(const ArtifactLocation &al) override;
	bool moveArtifact(const ArtifactLocation & al1, const ArtifactLocation & al2) override;
	bool bulkMoveArtifacts(ObjectInstanceID srcHero, ObjectInstanceID dstHero, bool swap);
	bool eraseArtifactByClient(const ArtifactLocation & al);
	void synchronizeArtifactHandlerLists();

	void heroVisitCastle(const CGTownInstance * obj, const CGHeroInstance * hero) override;
	void stopHeroVisitCastle(const CGTownInstance * obj, const CGHeroInstance * hero) override;
	void startBattlePrimary(const CArmedInstance *army1, const CArmedInstance *army2, int3 tile, const CGHeroInstance *hero1, const CGHeroInstance *hero2, bool creatureBank = false, const CGTownInstance *town = nullptr) override; //use hero=nullptr for no hero
	void startBattleI(const CArmedInstance *army1, const CArmedInstance *army2, int3 tile, bool creatureBank = false) override; //if any of armies is hero, hero will be used
	void startBattleI(const CArmedInstance *army1, const CArmedInstance *army2, bool creatureBank = false) override; //if any of armies is hero, hero will be used, visitable tile of second obj is place of battle
	bool moveHero(ObjectInstanceID hid, int3 dst, ui8 teleporting, bool transit = false, PlayerColor asker = PlayerColor::NEUTRAL) override;
	void giveHeroBonus(GiveBonus * bonus) override;
	void setMovePoints(SetMovePoints * smp) override;
	void setManaPoints(ObjectInstanceID hid, int val) override;
	void giveHero(ObjectInstanceID id, PlayerColor player, ObjectInstanceID boatId = ObjectInstanceID()) override;
	void changeObjPos(ObjectInstanceID objid, int3 newPos) override;
	void heroExchange(ObjectInstanceID hero1, ObjectInstanceID hero2) override;

	void changeFogOfWar(int3 center, ui32 radius, PlayerColor player, bool hide) override;
	void changeFogOfWar(std::unordered_set<int3> &tiles, PlayerColor player, bool hide) override;
	
	void castSpell(const spells::Caster * caster, SpellID spellID, const int3 &pos) override;

	bool isVisitCoveredByAnotherQuery(const CGObjectInstance *obj, const CGHeroInstance *hero) override;
	void setObjProperty(ObjectInstanceID objid, int prop, si64 val) override;
	void showInfoDialog(InfoWindow * iw) override;
	void showInfoDialog(const std::string & msg, PlayerColor player) override;

	//////////////////////////////////////////////////////////////////////////
	void useScholarSkill(ObjectInstanceID hero1, ObjectInstanceID hero2);
	void setPortalDwelling(const CGTownInstance * town, bool forced, bool clear);
	void visitObjectOnTile(const TerrainTile &t, const CGHeroInstance * h);
	bool teleportHero(ObjectInstanceID hid, ObjectInstanceID dstid, ui8 source, PlayerColor asker = PlayerColor::NEUTRAL);
	void visitCastleObjects(const CGTownInstance * obj, const CGHeroInstance * hero) override;
	void levelUpHero(const CGHeroInstance * hero, SecondarySkill skill);//handle client respond and send one more request if needed
	void levelUpHero(const CGHeroInstance * hero);//initial call - check if hero have remaining levelups & handle them
	void levelUpCommander (const CCommanderInstance * c, int skill); //secondary skill 1 to 6, special skill : skill - 100
	void levelUpCommander (const CCommanderInstance * c);

	void expGiven(const CGHeroInstance *hero); //triggers needed level-ups, handles also commander of this hero
	//////////////////////////////////////////////////////////////////////////

	void init(StartInfo *si, Load::Progress * progressTracking = nullptr);
	void handleClientDisconnection(std::shared_ptr<CConnection> c);
	void handleReceivedPack(CPackForServer * pack);
	PlayerColor getPlayerAt(std::shared_ptr<CConnection> c) const;
	bool hasPlayerAt(PlayerColor player, std::shared_ptr<CConnection> c) const;

	void updateGateState();
	bool makeBattleAction(BattleAction &ba);
	bool makeAutomaticAction(const CStack *stack, BattleAction &ba); //used when action is taken by stack without volition of player (eg. unguided catapult attack)
	bool makeCustomAction(BattleAction &ba);
	void stackEnchantedTrigger(const CStack * stack);
	void stackTurnTrigger(const CStack *stack);

	void removeObstacle(const CObstacleInstance &obstacle);
	bool queryReply( QueryID qid, const JsonNode & answer, PlayerColor player );
	bool buildBoat( ObjectInstanceID objid, PlayerColor player );
	bool setFormation( ObjectInstanceID hid, ui8 formation );
	bool tradeResources(const IMarket *market, ui32 val, PlayerColor player, ui32 id1, ui32 id2);
	bool sacrificeCreatures(const IMarket * market, const CGHeroInstance * hero, const std::vector<SlotID> & slot, const std::vector<ui32> & count);
	bool sendResources(ui32 val, PlayerColor player, GameResID r1, PlayerColor r2);
	bool sellCreatures(ui32 count, const IMarket *market, const CGHeroInstance * hero, SlotID slot, GameResID resourceID);
	bool transformInUndead(const IMarket *market, const CGHeroInstance * hero, SlotID slot);
	bool assembleArtifacts (ObjectInstanceID heroID, ArtifactPosition artifactSlot, bool assemble, ArtifactID assembleTo);
	bool buyArtifact( ObjectInstanceID hid, ArtifactID aid ); //for blacksmith and mage guild only -> buying for gold in common buildings
	bool buyArtifact( const IMarket *m, const CGHeroInstance *h, GameResID rid, ArtifactID aid); //for artifact merchant and black market -> buying for any resource in special building / advobject
	bool sellArtifact( const IMarket *m, const CGHeroInstance *h, ArtifactInstanceID aid, GameResID rid); //for artifact merchant selling
	//void lootArtifacts (TArtHolder source, TArtHolder dest, std::vector<ui32> &arts); //after battle - move al arts to winer
	bool buySecSkill( const IMarket *m, const CGHeroInstance *h, SecondarySkill skill);
	bool garrisonSwap(ObjectInstanceID tid);
	bool swapGarrisonOnSiege(ObjectInstanceID tid) override;
	bool upgradeCreature( ObjectInstanceID objid, SlotID pos, CreatureID upgID );
	bool recruitCreatures(ObjectInstanceID objid, ObjectInstanceID dst, CreatureID crid, ui32 cram, si32 level);
	bool buildStructure(ObjectInstanceID tid, BuildingID bid, bool force=false);//force - for events: no cost, no checkings
	bool razeStructure(ObjectInstanceID tid, BuildingID bid);
	bool disbandCreature( ObjectInstanceID id, SlotID pos );
	bool arrangeStacks( ObjectInstanceID id1, ObjectInstanceID id2, ui8 what, SlotID p1, SlotID p2, si32 val, PlayerColor player);
	bool bulkMoveArmy(ObjectInstanceID srcArmy, ObjectInstanceID destArmy, SlotID srcSlot);
	bool bulkSplitStack(SlotID src, ObjectInstanceID srcOwner, si32 howMany);
	bool bulkMergeStacks(SlotID slotSrc, ObjectInstanceID srcOwner);
	bool bulkSmartSplitStack(SlotID slotSrc, ObjectInstanceID srcOwner);
	void save(const std::string &fname);
	bool load(const std::string &fname);

	void handleTimeEvents();
	void handleTownEvents(CGTownInstance *town, NewTurn &n);
	bool complain(const std::string &problem); //sends message to all clients, prints on the logs and return true
	void objectVisited( const CGObjectInstance * obj, const CGHeroInstance * h );
	void objectVisitEnded(const CObjectVisitQuery &query);
	void engageIntoBattle( PlayerColor player );
	bool dig(const CGHeroInstance *h);
	void moveArmy(const CArmedInstance *src, const CArmedInstance *dst, bool allowMerging);

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & QID;
		h & states;
		h & finishingBattle;
		h & heroPool;
		h & getRandomGenerator();
		h & playerMessages;

		if (!h.saving)
			deserializationFix();

#if SCRIPTING_ENABLED
		JsonNode scriptsState;
		if(h.saving)
			serverScripts->serializeState(h.saving, scriptsState);
		h & scriptsState;
		if(!h.saving)
			serverScripts->serializeState(h.saving, scriptsState);
#endif
	}

	void sendToAllClients(CPackForClient * pack);
	void sendAndApply(CPackForClient * pack) override;
	void applyAndSend(CPackForClient * pack);
	void sendAndApply(CGarrisonOperationPack * pack);
	void sendAndApply(SetResources * pack);
	void sendAndApply(NewStructures * pack);

	void wrongPlayerMessage(CPackForServer * pack, PlayerColor expectedplayer);
	void throwNotAllowedAction(CPackForServer * pack);
	void throwOnWrongOwner(CPackForServer * pack, ObjectInstanceID id);
	void throwOnWrongPlayer(CPackForServer * pack, PlayerColor player);
	void throwAndComplain(CPackForServer * pack, std::string txt);
	bool isPlayerOwns(CPackForServer * pack, ObjectInstanceID id);

	struct FinishingBattleHelper
	{
		FinishingBattleHelper();
		FinishingBattleHelper(std::shared_ptr<const CBattleQuery> Query, int RemainingBattleQueriesCount);
		
		inline bool isDraw() const {return winnerSide == 2;}

		const CGHeroInstance *winnerHero, *loserHero;
		PlayerColor victor, loser;
		ui8 winnerSide;

		int remainingBattleQueriesCount;

		template <typename Handler> void serialize(Handler &h, const int version)
		{
			h & winnerHero;
			h & loserHero;
			h & victor;
			h & loser;
			h & winnerSide;
			h & remainingBattleQueriesCount;
		}
	};

	std::unique_ptr<FinishingBattleHelper> finishingBattle;

	void battleAfterLevelUp(const BattleResult &result);

	void run(bool resume);
	void newTurn();
	void handleAttackBeforeCasting(bool ranged, const CStack * attacker, const CStack * defender);
	void handleAfterAttackCasting(bool ranged, const CStack * attacker, const CStack * defender);
	void attackCasting(bool ranged, BonusType attackMode, const battle::Unit * attacker, const battle::Unit * defender);
	bool sacrificeArtifact(const IMarket * m, const CGHeroInstance * hero, const std::vector<ArtifactPosition> & slot);
	void spawnWanderingMonsters(CreatureID creatureID);

	// Check for victory and loss conditions
	void checkVictoryLossConditionsForPlayer(PlayerColor player);
	void checkVictoryLossConditions(const std::set<PlayerColor> & playerColors);
	void checkVictoryLossConditionsForAll();

	CRandomGenerator & getRandomGenerator();

#if SCRIPTING_ENABLED
	scripting::Pool * getGlobalContextPool() const override;
	scripting::Pool * getContextPool() const override;
#endif

	std::list<PlayerColor> generatePlayerTurnOrder() const;

	friend class CVCMIServer;
private:
	std::unique_ptr<events::EventBus> serverEventBus;
#if SCRIPTING_ENABLED
	std::shared_ptr<scripting::PoolImpl> serverScripts;
#endif

	void reinitScripting();
	void deserializationFix();


	void makeStackDoNothing(const CStack * next);
	void getVictoryLossMessage(PlayerColor player, const EVictoryLossCheckResult & victoryLossCheckResult, InfoWindow & out) const;

	const std::string complainNoCreatures;
	const std::string complainNotEnoughCreatures;
	const std::string complainInvalidSlot;
};

class ExceptionNotAllowedAction : public std::exception
{

};
