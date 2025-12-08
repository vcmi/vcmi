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

#include "../lib/callback/IGameEventCallback.h"
#include "../lib/LoadProgress.h"
#include "../lib/ScriptHandler.h"
#include "../lib/gameState/GameStatistics.h"
#include "../lib/networkPacks/PacksForServer.h"
#include "../lib/serializer/GameConnectionID.h"

VCMI_LIB_NAMESPACE_BEGIN

struct SideInBattle;
class IMarket;
class SpellCastEnvironment;
class CCommanderInstance;
class EVictoryLossCheckResult;
class CRandomGenerator;
class GameRandomizer;
class StatisticDataSet;
class ServerCallback;

struct StartInfo;
struct TerrainTile;
struct CPackForServer;
struct NewTurn;
struct CGarrisonOperationPack;
struct SetResources;
struct NewStructures;

#if SCRIPTING_ENABLED
namespace scripting
{
	class PoolImpl;
}
#endif

VCMI_LIB_NAMESPACE_END

class HeroPoolProcessor;
class CVCMIServer;
class CBaseForGHApply;
class PlayerMessageProcessor;
class BattleProcessor;
class TurnOrderProcessor;
class TurnTimerHandler;
class QueriesProcessor;
class CObjectVisitQuery;
class NewTurnProcessor;
class IGameServer;

class CGameHandler : public Environment, public IGameEventCallback
{
	IGameServer & server;

public:
	std::unique_ptr<HeroPoolProcessor> heroPool;
	std::unique_ptr<BattleProcessor> battles;
	std::unique_ptr<QueriesProcessor> queries;
	std::unique_ptr<TurnOrderProcessor> turnOrder;
	std::unique_ptr<TurnTimerHandler> turnTimerHandler;
	std::unique_ptr<NewTurnProcessor> newTurnProcessor;
	std::unique_ptr<GameRandomizer> randomizer;
	std::unique_ptr<StatisticDataSet> statistics;
	std::unique_ptr<SpellCastEnvironment> spellEnv;
	std::shared_ptr<CGameState> gs;

	//use enums as parameters, because doMove(sth, true, false, true) is not readable
	enum EGuardLook {CHECK_FOR_GUARDS, IGNORE_GUARDS};
	enum EVisitDest {VISIT_DEST, DONT_VISIT_DEST};
	enum ELEaveTile {LEAVING_TILE, REMAINING_ON_TILE};

	std::unique_ptr<PlayerMessageProcessor> playerMessages;

	//queries stuff
	QueryID QID;


	const Services * services() const override;
	const BattleCb * battle(const BattleID & battleID) const override;
	const GameCb * game() const override;
	vstd::CLoggerBase * logger() const override;
	events::EventBus * eventBus() const override;
	IGameServer & gameServer() const;
	ServerCallback * spellcastEnvironment() const;

	bool isBlockedByQueries(const CPackForServer *pack, PlayerColor player);
	bool isAllowedExchange(ObjectInstanceID id1, ObjectInstanceID id2);
	void giveSpells(const CGTownInstance *t, const CGHeroInstance *h);

	IGameInfoCallback & gameInfo();
	const CGameState & gameState() const { return *gs; }

	// Helpers to create new object of specified type

	std::shared_ptr<CGObjectInstance> createNewObject(const int3 & visitablePosition, MapObjectID objectID, MapObjectSubID subID);
	void createWanderingMonster(const int3 & visitablePosition, CreatureID creature, int unitSize);
	void createBoat(const int3 & visitablePosition, BoatId type, PlayerColor initiator) override;
	void createHole(const int3 & visitablePosition, PlayerColor initiator);
	void newObject(std::shared_ptr<CGObjectInstance> object, PlayerColor initiator);

	explicit CGameHandler(IGameServer & server);
	CGameHandler(IGameServer & server, const std::shared_ptr<CGameState> & gamestate);
	~CGameHandler();

	//////////////////////////////////////////////////////////////////////////
	//from IGameInfoCallback
	//do sth
	void changeSpells(const CGHeroInstance * hero, bool give, const std::set<SpellID> &spells) override;
	void setResearchedSpells(const CGTownInstance * town, int level, const std::vector<SpellID> & spells, bool accepted) override;
	bool removeObject(const CGObjectInstance * obj, const PlayerColor & initiator) override;
	void setOwner(const CGObjectInstance * obj, PlayerColor owner) override;
	void giveExperience(const CGHeroInstance * hero, TExpType val) override;
	void giveStackExperience(const CArmedInstance * army, TExpType val);
	void changePrimSkill(const CGHeroInstance * hero, PrimarySkill which, si64 val, ChangeValueMode mode) override;
	void changeSecSkill(const CGHeroInstance * hero, SecondarySkill which, int val, ChangeValueMode mode) override;

	void showBlockingDialog(const IObjectInterface * caller, BlockingDialog *iw) override;
	void showTeleportDialog(TeleportDialog *iw) override;
	void showGarrisonDialog(ObjectInstanceID upobj, ObjectInstanceID hid, bool removableUnits) override;
	void showObjectWindow(const CGObjectInstance * object, EOpenWindowMode window, const CGHeroInstance * visitor, bool addQuery) override;
	void giveResource(PlayerColor player, GameResID which, int val) override;
	void giveResources(PlayerColor player, const ResourceSet & resources) override;

	void giveCreatures(const CGHeroInstance * h, const CCreatureSet &creatures) override;
	void giveCreatures(const CArmedInstance *objid, const CGHeroInstance * h, const CCreatureSet &creatures, bool remove) override;
	void takeCreatures(ObjectInstanceID objid, const std::vector<CStackBasicDescriptor> &creatures, bool forceRemoval) override;
	bool changeStackType(const StackLocation &sl, const CCreature *c) override;
	bool changeStackCount(const StackLocation &sl, TQuantity count, ChangeValueMode mode) override;
	bool insertNewStack(const StackLocation &sl, const CCreature *c, TQuantity count) override;
	bool eraseStack(const StackLocation &sl, bool forceRemoval = false) override;
	bool swapStacks(const StackLocation &sl1, const StackLocation &sl2) override;
	bool addToSlot(const StackLocation &sl, const CCreature *c, TQuantity count) override;
	void tryJoiningArmy(const CArmedInstance *src, const CArmedInstance *dst, bool removeObjWhenFinished, bool allowMerging) override;
	bool moveStack(const StackLocation &src, const StackLocation &dst, TQuantity count = -1) override;

	void removeAfterVisit(const ObjectInstanceID & id) override;

	bool giveHeroNewArtifact(const CGHeroInstance * h, const CArtifact * artType, const SpellID & spellId, const ArtifactPosition & pos);
	bool giveHeroNewArtifact(const CGHeroInstance * h, const ArtifactID & artId, const ArtifactPosition & pos) override;
	bool giveHeroNewScroll(const CGHeroInstance * h, const SpellID & spellId, const ArtifactPosition & pos) override;
	bool putArtifact(const ArtifactLocation & al, const ArtifactInstanceID & id, std::optional<bool> askAssemble) override;
	void removeArtifact(const ArtifactLocation &al) override;
	void removeArtifact(const ObjectInstanceID & srcId, const std::vector<ArtifactPosition> & slotsPack);
	bool moveArtifact(const PlayerColor & player, const ArtifactLocation & src, const ArtifactLocation & dst) override;
	bool bulkMoveArtifacts(const PlayerColor & player, ObjectInstanceID srcId, ObjectInstanceID dstId, bool swap, bool equipped, bool backpack);
	bool manageBackpackArtifacts(const PlayerColor & player, const ObjectInstanceID & heroID, const ManageBackpackArtifacts::ManageCmd & sortType);
	bool saveArtifactsCostume(const PlayerColor & player, const ObjectInstanceID & heroID, uint32_t costumeIdx);
	bool switchArtifactsCostume(const PlayerColor & player, const ObjectInstanceID & heroID, uint32_t costumeIdx);
	bool eraseArtifactByClient(const ArtifactLocation & al);

	void heroVisitCastle(const CGTownInstance * obj, const CGHeroInstance * hero) override;
	void stopHeroVisitCastle(const CGTownInstance * obj, const CGHeroInstance * hero) override;
	void startBattle(const CArmedInstance *army1, const CArmedInstance *army2, int3 tile, const CGHeroInstance *hero1, const CGHeroInstance *hero2, const BattleLayout & layout, const CGTownInstance *town) override; //use hero=nullptr for no hero
	void startBattle(const CArmedInstance *army1, const CArmedInstance *army2) override; //if any of armies is hero, hero will be used, visitable tile of second obj is place of battle
	bool moveHero(ObjectInstanceID hid, int3 dst, EMovementMode movementMode, bool transit = false, PlayerColor asker = PlayerColor::NEUTRAL) override;
	void giveHeroBonus(GiveBonus * bonus) override;
	void setMovePoints(SetMovePoints * smp) override;
	void setMovePoints(ObjectInstanceID hid, int val) override;
	void setManaPoints(ObjectInstanceID hid, int val) override;
	void giveHero(ObjectInstanceID id, PlayerColor player, ObjectInstanceID boatId = ObjectInstanceID()) override;
	void changeObjPos(ObjectInstanceID objid, int3 newPos, const PlayerColor & initiator) override;
	void heroExchange(ObjectInstanceID hero1, ObjectInstanceID hero2) override;

	void changeFogOfWar(int3 center, ui32 radius, PlayerColor player, ETileVisibility mode) override;
	void changeFogOfWar(const FowTilesType &tiles, PlayerColor player,ETileVisibility mode) override;
	
	void castSpell(const spells::Caster * caster, SpellID spellID, const int3 &pos) override;
	void useChargeBasedSpell(const ObjectInstanceID & heroObjectID, const SpellID & spellID);

	/// Returns hero that is currently visiting this object, or nullptr if no visit is active
	const CGHeroInstance * getVisitingHero(const CGObjectInstance *obj);
	const CGObjectInstance * getVisitingObject(const CGHeroInstance *hero);
	bool isVisitCoveredByAnotherQuery(const CGObjectInstance *obj, const CGHeroInstance *hero) override;
	void setObjPropertyValue(ObjectInstanceID objid, ObjProperty prop, int32_t value) override;
	void setObjPropertyID(ObjectInstanceID objid, ObjProperty prop, ObjPropertyID identifier) override;
	void setRewardableObjectConfiguration(ObjectInstanceID objid, const Rewardable::Configuration & configuration) override;
	void setRewardableObjectConfiguration(ObjectInstanceID townInstanceID, BuildingID buildingID, const Rewardable::Configuration & configuration) override;
	void showInfoDialog(InfoWindow * iw) override;

	//////////////////////////////////////////////////////////////////////////
	void useScholarSkill(ObjectInstanceID hero1, ObjectInstanceID hero2);
	void setPortalDwelling(const CGTownInstance * town, bool forced, bool clear);
	void visitObjectOnTile(const TerrainTile &t, const CGHeroInstance * h);
	bool teleportHero(ObjectInstanceID hid, ObjectInstanceID dstid, ui8 source, PlayerColor asker = PlayerColor::NEUTRAL);
	void visitCastleObjects(const CGTownInstance * obj, const CGHeroInstance * hero) override;
	void visitCastleObjects(const CGTownInstance * obj, const std::vector<const CGHeroInstance * > & visitors);
	void levelUpHero(const CGHeroInstance * hero, SecondarySkill skill);//handle client respond and send one more request if needed
	void levelUpHero(const CGHeroInstance * hero);//initial call - check if hero have remaining levelups & handle them
	void levelUpCommander (const CCommanderInstance * c, int skill); //secondary skill 1 to 6, special skill : skill - 100
	void levelUpCommander (const CCommanderInstance * c);

	void expGiven(const CGHeroInstance *hero); //triggers needed level-ups, handles also commander of this hero
	//////////////////////////////////////////////////////////////////////////

	void init(StartInfo *si, Load::ProgressAccumulator & progressTracking);
	void handleClientDisconnection(GameConnectionID connectionI);
	void handleReceivedPack(GameConnectionID connectionId, CPackForServer & pack);
	bool hasPlayerAt(PlayerColor player, GameConnectionID connectionId) const;
	bool hasBothPlayersAtSameConnection(PlayerColor left, PlayerColor right) const;

	bool queryReply( QueryID qid, std::optional<int32_t> reply, PlayerColor player );
	bool buildBoat( ObjectInstanceID objid, PlayerColor player );
	bool setFormation( ObjectInstanceID hid, EArmyFormation formation );
	bool setTownName( ObjectInstanceID tid, std::string & name );
	bool tradeResources(const IMarket *market, ui32 amountToSell, PlayerColor player, GameResID toSell, GameResID toBuy);
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
	bool upgradeCreature( ObjectInstanceID objid, SlotID pos, CreatureID upgID );
	bool recruitCreatures(ObjectInstanceID objid, ObjectInstanceID dst, CreatureID crid, int32_t cram, int32_t level, PlayerColor player);
	bool buildStructure(ObjectInstanceID tid, BuildingID bid, bool force=false);//force - for events: no cost, no checkings
	bool visitTownBuilding(ObjectInstanceID tid, BuildingID bid);
	bool razeStructure(ObjectInstanceID tid, BuildingID bid);
	bool spellResearch(ObjectInstanceID tid, SpellID spellAtSlot, bool accepted);
	bool disbandCreature( ObjectInstanceID id, SlotID pos );
	bool arrangeStacks( ObjectInstanceID id1, ObjectInstanceID id2, ui8 what, SlotID p1, SlotID p2, si32 val, PlayerColor player);
	bool bulkMoveArmy(ObjectInstanceID srcArmy, ObjectInstanceID destArmy, SlotID srcSlot);
	bool bulkSplitStack(SlotID src, ObjectInstanceID srcOwner, si32 howMany);
	bool bulkMergeStacks(SlotID slotSrc, ObjectInstanceID srcOwner);
	bool bulkSplitAndRebalanceStack(SlotID slotSrc, ObjectInstanceID srcOwner);
	bool responseStatistic(PlayerColor player);
	void save(const std::string &fname);
	void load(const StartInfo &info);

	void onPlayerTurnStarted(PlayerColor which);
	void onPlayerTurnEnded(PlayerColor which);
	void onNewTurn();
	void addStatistics(StatisticDataSet &stat) const;

	bool complain(const std::string &problem); //sends message to all clients, prints on the logs and return true
	void objectVisited( const CGObjectInstance * obj, const CGHeroInstance * h );
	void objectVisitEnded(const ObjectInstanceID & heroObjectID, PlayerColor player);
	bool dig(const CGHeroInstance *h);
	void moveArmy(const CArmedInstance *src, const CArmedInstance *dst, bool allowMerging);

	template <typename Handler> void serialize(Handler &h)
	{
		h & QID;
		h & *randomizer;
		h & *battles;
		h & *heroPool;
		h & *playerMessages;
		h & *turnOrder;
		h & *turnTimerHandler;

		if (h.hasFeature(Handler::Version::SERVER_STATISTICS))
		{
			h & *statistics;
		}


#if SCRIPTING_ENABLED
		JsonNode scriptsState;
		if(h.saving)
			serverScripts->serializeState(h.saving, scriptsState);
		h & scriptsState;
		if(!h.saving)
			serverScripts->serializeState(h.saving, scriptsState);
#endif
	}

	void sendAndApply(CPackForClient & pack) override;
	void sendAndApply(CGarrisonOperationPack & pack);
	void sendAndApply(SetResources & pack);
	void sendAndApply(NewStructures & pack);

	void wrongPlayerMessage(GameConnectionID connectionID, const CPackForServer * pack, PlayerColor expectedplayer);
	/// Unconditionally throws with "Action not allowed" message
	[[noreturn]] void throwNotAllowedAction(GameConnectionID connectionID);
	/// Throws if player stated in pack is not making turn right now
	void throwIfPlayerNotActive(GameConnectionID connectionID, const CPackForServer * pack);
	/// Throws if object is not owned by pack sender
	void throwIfWrongOwner(GameConnectionID connectionID, const CPackForServer * pack, ObjectInstanceID id);
	/// Throws if player is not present on connection of this pack
	void throwIfWrongPlayer(GameConnectionID connectionID, const CPackForServer * pack, PlayerColor player);
	void throwIfWrongPlayer(GameConnectionID connectionID, const CPackForServer * pack);
	[[noreturn]] void throwAndComplain(GameConnectionID connectionID, const std::string & txt);

	bool isPlayerOwns(GameConnectionID connectionID, const CPackForServer * pack, ObjectInstanceID id);

	void start(bool resume);
	void tick(int millisecondsPassed);
	bool sacrificeArtifact(const IMarket * market, const CGHeroInstance * hero, const std::vector<ArtifactInstanceID> & arts);
	void spawnWanderingMonsters(CreatureID creatureID);

	// Check for victory and loss conditions
	void checkVictoryLossConditionsForPlayer(PlayerColor player);
	void checkVictoryLossConditions(const std::set<PlayerColor> & playerColors);
	void checkVictoryLossConditionsForAll();

	vstd::RNG & getRandomGenerator() override;

//#if SCRIPTING_ENABLED
//	scripting::Pool * getGlobalContextPool() const override;
//	scripting::Pool * getContextPool() const override;
//#endif

	friend class CVCMIServer;
private:
	std::unique_ptr<events::EventBus> serverEventBus;
#if SCRIPTING_ENABLED
	std::shared_ptr<scripting::PoolImpl> serverScripts;
#endif

	void reinitScripting();

	void getVictoryLossMessage(PlayerColor player, const EVictoryLossCheckResult & victoryLossCheckResult, InfoWindow & out) const;

	const std::string complainNoCreatures;
	const std::string complainNotEnoughCreatures;
	const std::string complainInvalidSlot;
};

class ExceptionNotAllowedAction : public std::exception
{

};
