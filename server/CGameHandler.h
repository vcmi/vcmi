#pragma once


#include "../lib/FunctionList.h"
#include "../lib/Connection.h"
#include "../lib/IGameCallback.h"
#include "../lib/BattleAction.h"
#include "CQuery.h"


/*
 * CGameHandler.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

class CGameHandler;
class CVCMIServer;
class CGameState;
struct StartInfo;
struct BattleResult;
struct BattleAttack;
struct BattleStackAttacked;
struct CPack;
struct Query;
struct SetGarrisons;
struct SetResource;
struct SetResources;
struct NewStructures;
class CGHeroInstance;
class IMarket;

class ServerSpellCastEnvironment;

extern std::map<ui32, CFunctionList<void(ui32)> > callbacks; //question id => callback functions - for selection dialogs
extern boost::mutex gsm;

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
	typedef std::pair<StackLocation, int> TStackAndItsNewCount;
	enum {ERASE = -1};
	std::vector<TStackAndItsNewCount> newStackCounts;
	std::vector<ArtifactLocation> removedWarMachines;
	ObjectInstanceID heroWithDeadCommander; //TODO: unify stack loactions

	CasualtiesAfterBattle(const CArmedInstance *army, BattleInfo *bat);
	void takeFromArmy(CGameHandler *gh);
};

class CGameHandler : public IGameCallback, CBattleInfoCallback
{
public:
	//use enums as parameters, because doMove(sth, true, false, true) is not readable
	enum EGuardLook {CHECK_FOR_GUARDS, IGNORE_GUARDS};
	enum EVisitDest {VISIT_DEST, DONT_VISIT_DEST};
	enum ELEaveTile {LEAVING_TILE, REMAINING_ON_TILE};

	CVCMIServer *s;
	std::map<PlayerColor, CConnection*> connections; //player color -> connection to client with interface of that player
	PlayerStatuses states; //player color -> player state
	std::set<CConnection*> conns;

	//queries stuff
	boost::recursive_mutex gsm;
	ui32 QID;
	Queries queries;

	//TODO get rid of cfunctionlist (or similar) and use serialziable callback structure
	std::map<ui32, CFunctionList<void(ui32)> > callbacks; //query id => callback function - for selection and yes/no dialogs

	bool isValidObject(const CGObjectInstance *obj) const;
	bool isBlockedByQueries(const CPack *pack, PlayerColor player); 
	bool isAllowedExchange(ObjectInstanceID id1, ObjectInstanceID id2);
	void giveSpells(const CGTownInstance *t, const CGHeroInstance *h);
	int moveStack(int stack, BattleHex dest); //returned value - travelled distance
	void runBattle();

	////used only in endBattle - don't touch elsewhere
	bool visitObjectAfterVictory;
	//
	void endBattle(int3 tile, const CGHeroInstance *hero1, const CGHeroInstance *hero2); //ends battle
	void prepareAttack(BattleAttack &bat, const CStack *att, const CStack *def, int distance, int targetHex); //distance - number of hexes travelled before attacking
	void applyBattleEffects(BattleAttack &bat, const CStack *att, const CStack *def, int distance, bool secondary); //damage, drain life & fire shield
	void checkForBattleEnd();
	void setupBattle(int3 tile, const CArmedInstance *armies[2], const CGHeroInstance *heroes[2], bool creatureBank, const CGTownInstance *town);
	void setBattleResult(BattleResult::EResult resultType, int victoriusSide);
	void duelFinished();

	CGameHandler(void);
	~CGameHandler(void);

	//////////////////////////////////////////////////////////////////////////
	//from IGameCallback
	//do sth
	void changeSpells(const CGHeroInstance * hero, bool give, const std::set<SpellID> &spells) override;
	bool removeObject(const CGObjectInstance * obj) override;
	void setBlockVis(ObjectInstanceID objid, bool bv) override;
	void setOwner(const CGObjectInstance * obj, PlayerColor owner) override;
	void changePrimSkill(const CGHeroInstance * hero, PrimarySkill::PrimarySkill which, si64 val, bool abs=false) override;
	void changeSecSkill(const CGHeroInstance * hero, SecondarySkill which, int val, bool abs=false) override; 
	//void showInfoDialog(InfoWindow *iw) override;

	void showBlockingDialog(BlockingDialog *iw) override; 
	void showTeleportDialog(TeleportDialog *iw) override;
	void showGarrisonDialog(ObjectInstanceID upobj, ObjectInstanceID hid, bool removableUnits) override;
	void showThievesGuildWindow(PlayerColor player, ObjectInstanceID requestingObjId) override;
	void giveResource(PlayerColor player, Res::ERes which, int val) override;
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

	void giveHeroNewArtifact(const CGHeroInstance *h, const CArtifact *artType, ArtifactPosition pos) override;
	void giveHeroArtifact(const CGHeroInstance *h, const CArtifactInstance *a, ArtifactPosition pos) override;
	void putArtifact(const ArtifactLocation &al, const CArtifactInstance *a) override; 
	void removeArtifact(const ArtifactLocation &al) override;
	bool moveArtifact(const ArtifactLocation &al1, const ArtifactLocation &al2) override;
	void synchronizeArtifactHandlerLists() override;

	void showCompInfo(ShowInInfobox * comp) override;
	void heroVisitCastle(const CGTownInstance * obj, const CGHeroInstance * hero) override;
	void stopHeroVisitCastle(const CGTownInstance * obj, const CGHeroInstance * hero) override;
	//bool removeArtifact(const CArtifact* art, int hid) override;
	void startBattlePrimary(const CArmedInstance *army1, const CArmedInstance *army2, int3 tile, const CGHeroInstance *hero1, const CGHeroInstance *hero2, bool creatureBank = false, const CGTownInstance *town = nullptr) override; //use hero=nullptr for no hero
	void startBattleI(const CArmedInstance *army1, const CArmedInstance *army2, int3 tile, bool creatureBank = false) override; //if any of armies is hero, hero will be used
	void startBattleI(const CArmedInstance *army1, const CArmedInstance *army2, bool creatureBank = false) override; //if any of armies is hero, hero will be used, visitable tile of second obj is place of battle//void startBattleI(int heroID, CCreatureSet army, int3 tile, std::function<void(BattleResult*)> cb) override; //for hero<=>neutral army
	void setAmount(ObjectInstanceID objid, ui32 val) override;
	bool moveHero(ObjectInstanceID hid, int3 dst, ui8 teleporting, bool transit = false, PlayerColor asker = PlayerColor::NEUTRAL) override;
	void giveHeroBonus(GiveBonus * bonus) override;
	void setMovePoints(SetMovePoints * smp) override;
	void setManaPoints(ObjectInstanceID hid, int val) override;
	void giveHero(ObjectInstanceID id, PlayerColor player) override;
	void changeObjPos(ObjectInstanceID objid, int3 newPos, ui8 flags) override;
	void heroExchange(ObjectInstanceID hero1, ObjectInstanceID hero2) override;

	void changeFogOfWar(int3 center, ui32 radius, PlayerColor player, bool hide) override;
	void changeFogOfWar(std::unordered_set<int3, ShashInt3> &tiles, PlayerColor player, bool hide) override;

	bool isVisitCoveredByAnotherQuery(const CGObjectInstance *obj, const CGHeroInstance *hero) override;

	//////////////////////////////////////////////////////////////////////////
	void useScholarSkill(ObjectInstanceID hero1, ObjectInstanceID hero2);
	void setPortalDwelling(const CGTownInstance * town, bool forced, bool clear);
	void visitObjectOnTile(const TerrainTile &t, const CGHeroInstance * h);
	bool teleportHero(ObjectInstanceID hid, ObjectInstanceID dstid, ui8 source, PlayerColor asker = PlayerColor::NEUTRAL);
	void vistiCastleObjects (const CGTownInstance *t, const CGHeroInstance *h);
	void levelUpHero(const CGHeroInstance * hero, SecondarySkill skill);//handle client respond and send one more request if needed
	void levelUpHero(const CGHeroInstance * hero);//initial call - check if hero have remaining levelups & handle them
	void levelUpCommander (const CCommanderInstance * c, int skill); //secondary skill 1 to 6, special skill : skill - 100
	void levelUpCommander (const CCommanderInstance * c);

	void expGiven(const CGHeroInstance *hero); //triggers needed level-ups, handles also commander of this hero
	//////////////////////////////////////////////////////////////////////////

	void commitPackage(CPackForClient *pack) override;

	void init(StartInfo *si);
	void handleConnection(std::set<PlayerColor> players, CConnection &c);
	PlayerColor getPlayerAt(CConnection *c) const;

	void playerMessage( PlayerColor player, const std::string &message, ObjectInstanceID currObj);
	bool makeBattleAction(BattleAction &ba);
	bool makeAutomaticAction(const CStack *stack, BattleAction &ba); //used when action is taken by stack without volition of player (eg. unguided catapult attack)
	bool makeCustomAction(BattleAction &ba);
	void stackTurnTrigger(const CStack * stack);
	void handleDamageFromObstacle(const CObstacleInstance &obstacle, const CStack * curStack); //checks if obstacle is land mine and handles possible consequences
	void removeObstacle(const CObstacleInstance &obstacle);
	bool queryReply( QueryID qid, ui32 answer, PlayerColor player );
	bool hireHero( const CGObjectInstance *obj, ui8 hid, PlayerColor player );
	bool buildBoat( ObjectInstanceID objid );
	bool setFormation( ObjectInstanceID hid, ui8 formation );
	bool tradeResources(const IMarket *market, ui32 val, PlayerColor player, ui32 id1, ui32 id2);
	bool sacrificeCreatures(const IMarket *market, const CGHeroInstance *hero, SlotID slot, ui32 count);
	bool sendResources(ui32 val, PlayerColor player, Res::ERes r1, PlayerColor r2);
	bool sellCreatures(ui32 count, const IMarket *market, const CGHeroInstance * hero, SlotID slot, Res::ERes resourceID);
	bool transformInUndead(const IMarket *market, const CGHeroInstance * hero, SlotID slot);
	bool assembleArtifacts (ObjectInstanceID heroID, ArtifactPosition artifactSlot, bool assemble, ArtifactID assembleTo);
	bool buyArtifact( ObjectInstanceID hid, ArtifactID aid ); //for blacksmith and mage guild only -> buying for gold in common buildings
	bool buyArtifact( const IMarket *m, const CGHeroInstance *h, Res::ERes rid, ArtifactID aid); //for artifact merchant and black market -> buying for any resource in special building / advobject
	bool sellArtifact( const IMarket *m, const CGHeroInstance *h, ArtifactInstanceID aid, Res::ERes rid); //for artifact merchant selling
	//void lootArtifacts (TArtHolder source, TArtHolder dest, std::vector<ui32> &arts); //after battle - move al arts to winer
	bool buySecSkill( const IMarket *m, const CGHeroInstance *h, SecondarySkill skill);
	bool garrisonSwap(ObjectInstanceID tid);
	bool upgradeCreature( ObjectInstanceID objid, SlotID pos, CreatureID upgID );
	bool recruitCreatures(ObjectInstanceID objid, ObjectInstanceID dst, CreatureID crid, ui32 cram, si32 level);
	bool buildStructure(ObjectInstanceID tid, BuildingID bid, bool force=false);//force - for events: no cost, no checkings
	bool razeStructure(ObjectInstanceID tid, BuildingID bid);
	bool disbandCreature( ObjectInstanceID id, SlotID pos );
	bool arrangeStacks( ObjectInstanceID id1, ObjectInstanceID id2, ui8 what, SlotID p1, SlotID p2, si32 val, PlayerColor player);
	void save(const std::string &fname);
	void close();
	void handleTimeEvents();
	void handleTownEvents(CGTownInstance *town, NewTurn &n);
	bool complain(const std::string &problem); //sends message to all clients, prints on the logs and return true
	void objectVisited( const CGObjectInstance * obj, const CGHeroInstance * h );
	void objectVisitEnded(const CObjectVisitQuery &query);
	void engageIntoBattle( PlayerColor player );
	bool dig(const CGHeroInstance *h);
	bool castSpell(const CGHeroInstance *h, SpellID spellID, const int3 &pos);
	void moveArmy(const CArmedInstance *src, const CArmedInstance *dst, bool allowMerging);

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & QID & states & finishingBattle;
	}

	void sendMessageToAll(const std::string &message);
	void sendMessageTo(CConnection &c, const std::string &message);
	void sendToAllClients(CPackForClient * info);
	void sendAndApply(CPackForClient * info) override;
	void applyAndSend(CPackForClient * info);
	void sendAndApply(CGarrisonOperationPack * info);
	void sendAndApply(SetResource * info);
	void sendAndApply(SetResources * info);
	void sendAndApply(NewStructures * info);

	struct FinishingBattleHelper
	{
		FinishingBattleHelper();
		FinishingBattleHelper(std::shared_ptr<const CBattleQuery> Query, bool Duel, int RemainingBattleQueriesCount);

		//std::shared_ptr<const CBattleQuery> query;
		const CGHeroInstance *winnerHero, *loserHero;
		PlayerColor victor, loser;
		bool duel;

		int remainingBattleQueriesCount;

		template <typename Handler> void serialize(Handler &h, const int version)
		{
			h & /*query & */winnerHero & loserHero & victor & loser & duel & remainingBattleQueriesCount;
		}
	};

	std::unique_ptr<FinishingBattleHelper> finishingBattle;

	void battleAfterLevelUp(const BattleResult &result);

	void run(bool resume);
	void newTurn();
	void handleAttackBeforeCasting (const BattleAttack & bat);
	void handleAfterAttackCasting (const BattleAttack & bat);
	void attackCasting(const BattleAttack & bat, Bonus::BonusType attackMode, const CStack * attacker);
	bool sacrificeArtifact(const IMarket * m, const CGHeroInstance * hero, ArtifactPosition slot);
	void spawnWanderingMonsters(CreatureID creatureID);
	friend class CVCMIServer;

private:
	ServerSpellCastEnvironment * spellEnv;

	std::list<PlayerColor> generatePlayerTurnOrder() const;
	void makeStackDoNothing(const CStack * next);
	void getVictoryLossMessage(PlayerColor player, const EVictoryLossCheckResult & victoryLossCheckResult, InfoWindow & out) const;

	// Check for victory and loss conditions
	void checkVictoryLossConditionsForPlayer(PlayerColor player);
	void checkVictoryLossConditions(const std::set<PlayerColor> & playerColors);
	void checkVictoryLossConditionsForAll();
};

void makeStackDoNothing();
