#ifndef __CGAMEHANDLER_H__
#define __CGAMEHANDLER_H__

#include "../global.h"
#include <set>
#include "../client/FunctionList.h"
#include "../lib/CGameState.h"
#include "../lib/Connection.h"
#include "../lib/IGameCallback.h"
#include "../lib/BattleAction.h"
#include <boost/function.hpp>
#include <boost/thread.hpp>

/*
 * CGameHandler.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

class CVCMIServer;
class CGameState;
struct StartInfo;
class CCPPObjectScript;
class CScriptCallback;
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

extern std::map<ui32, CFunctionList<void(ui32)> > callbacks; //question id => callback functions - for selection dialogs
extern boost::mutex gsm;

struct PlayerStatus
{
	bool makingTurn, engagedIntoBattle;
	std::set<ui32> queries;

	PlayerStatus():makingTurn(false),engagedIntoBattle(false){};
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & makingTurn & engagedIntoBattle & queries;
	}
};
class PlayerStatuses
{
public:
	std::map<ui8,PlayerStatus> players;
	boost::mutex mx;
	boost::condition_variable cv; //notifies when any changes are made

	void addPlayer(ui8 player);
	PlayerStatus operator[](ui8 player);
	bool hasQueries(ui8 player);
	bool checkFlag(ui8 player, bool PlayerStatus::*flag);
	void setFlag(ui8 player, bool PlayerStatus::*flag, bool val);
	void addQuery(ui8 player, ui32 id);
	void removeQuery(ui8 player, ui32 id);
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & players;
	}
};

class CGameHandler : public IGameCallback
{
public:
	CVCMIServer *s;
	std::map<int,CConnection*> connections; //player color -> connection to client with interface of that player
	PlayerStatuses states; //player color -> player state
	std::set<CConnection*> conns;

	//queries stuff
	boost::recursive_mutex gsm;
	ui32 QID;
	std::map<ui32, CFunctionList<void(ui32)> > callbacks; //query id => callback function - for selection and yes/no dialogs
	std::map<ui32, boost::function<void()> > garrisonCallbacks; //query id => callback - for garrison dialogs
	std::map<ui32, std::pair<si32,si32> > allowedExchanges;

	bool isAllowedExchange(int id1, int id2);
	void giveSpells(const CGTownInstance *t, const CGHeroInstance *h);
	int moveStack(int stack, int dest); //returned value - travelled distance
	void startBattle(const CArmedInstance *army1, const CArmedInstance *army2, int3 tile, const CGHeroInstance *hero1, const CGHeroInstance *hero2, bool creatureBank, boost::function<void(BattleResult*)> cb, const CGTownInstance *town = NULL); //use hero=NULL for no hero
	void checkLossVictory(ui8 player);
	void winLoseHandle(ui8 players=255); //players: bit field - colours of players to be checked; default: all
	void getLossVicMessage(ui8 player, ui8 standard, bool victory, InfoWindow &out) const;

	////used only in endBattle - don't touch elsewhere
	boost::function<void(BattleResult*)> * battleEndCallback;
	const CArmedInstance * bEndArmy1, * bEndArmy2;
	bool visitObjectAfterVictory;
	//
	void endBattle(int3 tile, const CGHeroInstance *hero1, const CGHeroInstance *hero2); //ends battle
	void prepareAttack(BattleAttack &bat, const CStack *att, const CStack *def, int distance); //distance - number of hexes travelled before attacking
	void prepareAttacked(BattleStackAttacked &bsa, const CStack *def);
	void checkForBattleEnd( std::vector<CStack*> &stacks );
	void setupBattle( BattleInfo * curB, int3 tile, const CArmedInstance *army1, const CArmedInstance *army2, const CGHeroInstance * hero1, const CGHeroInstance * hero2, bool creatureBank, const CGTownInstance *town);

	CGameHandler(void);
	~CGameHandler(void);

	//////////////////////////////////////////////////////////////////////////
	//from IGameCallback
	//get info
	int getCurrentPlayer();
	int getSelectedHero();


	//do sth
	void changeSpells(int hid, bool give, const std::set<ui32> &spells);
	bool removeObject(int objid);
	void setBlockVis(int objid, bool bv);
	void setOwner(int objid, ui8 owner);
	void setHoverName(int objid, MetaString * name);
	void setObjProperty(int objid, int prop, si64 val);
	void levelUpHero(int ID, int skill);//handle client respond and send one more request if needed 
	void levelUpHero(int ID);//initial call - check if hero have remaining levelups & handle them
	void changePrimSkill(int ID, int which, si64 val, bool abs=false);
	void changeSecSkill(int ID, int which, int val, bool abs=false); 
	void showInfoDialog(InfoWindow *iw);
	void showBlockingDialog(BlockingDialog *iw, const CFunctionList<void(ui32)> &callback);
	ui32 showBlockingDialog(BlockingDialog *iw); //synchronous version of above
	void showGarrisonDialog(int upobj, int hid, bool removableUnits, const boost::function<void()> &cb);
	void showThievesGuildWindow(int requestingObjId); //TODO: make something more general?
	void giveResource(int player, int which, int val);
	void giveCreatures (int objid, const CGHeroInstance * h, CCreatureSet creatures);
	void takeCreatures (int objid, TSlots creatures);
	void changeCreatureType (int objid, TSlot slot, TCreature creature);
	void showCompInfo(ShowInInfobox * comp);
	void heroVisitCastle(int obj, int heroID);
	void vistiCastleObjects (const CGTownInstance *t, const CGHeroInstance *h);
	void stopHeroVisitCastle(int obj, int heroID);
	void giveHeroArtifact(int artid, int hid, int position); //pos==-1 - first free slot in backpack; pos==-2 - default if available or backpack
	void moveArtifact(int hid, int oldPosition, int destPos);
	bool removeArtifact(int artid, int hid);
	void startBattleI(const CArmedInstance *army1, const CArmedInstance *army2, int3 tile, const CGHeroInstance *hero1, const CGHeroInstance *hero2, bool creatureBank = false, boost::function<void(BattleResult*)> cb = 0, const CGTownInstance *town = NULL); //use hero=NULL for no hero
	void startBattleI(const CArmedInstance *army1, const CArmedInstance *army2, int3 tile, boost::function<void(BattleResult*)> cb = 0, bool creatureBank = false); //if any of armies is hero, hero will be used
	void startBattleI(const CArmedInstance *army1, const CArmedInstance *army2, boost::function<void(BattleResult*)> cb = 0, bool creatureBank = false); //if any of armies is hero, hero will be used, visitable tile of second obj is place of battle//void startBattleI(int heroID, CCreatureSet army, int3 tile, boost::function<void(BattleResult*)> cb); //for hero<=>neutral army
	void setAmount(int objid, ui32 val);
	bool teleportHero(si32 hid, si32 dstid, ui8 source, ui8 asker = 255);
	bool moveHero(si32 hid, int3 dst, ui8 instant, ui8 asker = 255);
	bool tryAttackingGuard(const int3 &guardPos, const CGHeroInstance * h);
	void visitObjectOnTile(const TerrainTile &t, const CGHeroInstance * h);
	void giveHeroBonus(GiveBonus * bonus);
	void setMovePoints(SetMovePoints * smp);
	void setManaPoints(int hid, int val);
	void giveHero(int id, int player);
	void changeObjPos(int objid, int3 newPos, ui8 flags);
	void useScholarSkill(si32 hero1, si32 hero2);
	void heroExchange(si32 hero1, si32 hero2);
	void setPortalDwelling(const CGTownInstance * town, bool forced);
	//////////////////////////////////////////////////////////////////////////

	void init(StartInfo *si, int Seed);
	void handleConnection(std::set<int> players, CConnection &c);
	int getPlayerAt(CConnection *c) const;

	void playerMessage( ui8 player, const std::string &message);
	bool makeBattleAction(BattleAction &ba);
	void handleSpellCasting(int spellID, int spellLvl, int destination, ui8 casterSide, ui8 casterColor, const CGHeroInstance * caster, const CGHeroInstance * secHero, int usedSpellPower);
	bool makeCustomAction(BattleAction &ba);
	bool queryReply( ui32 qid, ui32 answer );
	bool hireHero( const CGObjectInstance *obj, ui8 hid, ui8 player );
	bool buildBoat( ui32 objid );
	bool setFormation( si32 hid, ui8 formation );
	bool tradeResources(const IMarket *market, ui32 val, ui8 player, ui32 id1, ui32 id2);
	bool sacrificeCreatures(const IMarket *market, const CGHeroInstance *hero, TSlot slot, ui32 count);
	bool sendResources(ui32 val, ui8 player, ui32 r1, ui32 r2);
	bool sellCreatures(ui32 count, const IMarket *market, const CGHeroInstance * hero, ui32 slot, ui32 resourceID);
	bool transformInUndead(const IMarket *market, const CGHeroInstance * hero, ui32 slot);
	bool assembleArtifacts (si32 heroID, ui16 artifactSlot, bool assemble, ui32 assembleTo);
	bool buyArtifact( ui32 hid, si32 aid ); //for blacksmith and mage guild only -> buying for gold in common buildings
	bool buyArtifact( const IMarket *m, const CGHeroInstance *h, int rid, int aid); //for artifact merchant and black market -> buying for any resource in special building / advobject
	bool buySecSkill( const IMarket *m, const CGHeroInstance *h, int skill);
	bool swapArtifacts(si32 srcHeroID, si32 destHeroID, ui16 srcSlot, ui16 destSlot);
	bool garrisonSwap(si32 tid);
	bool upgradeCreature( ui32 objid, ui8 pos, ui32 upgID );
	bool recruitCreatures(si32 objid, ui32 crid, ui32 cram, si32 level);
	bool buildStructure(si32 tid, si32 bid);
	bool razeStructure(si32 tid, si32 bid);
	bool disbandCreature( si32 id, ui8 pos );
	bool arrangeStacks( si32 id1, si32 id2, ui8 what, ui8 p1, ui8 p2, si32 val, ui8 player);
	void save(const std::string &fname);
	void close();
	void handleTimeEvents();
	bool complain(const std::string &problem); //sends message to all clients, prints on the logs and return true
	void objectVisited( const CGObjectInstance * obj, const CGHeroInstance * h );
	void engageIntoBattle( ui8 player );
	bool dig(const CGHeroInstance *h);
	bool castSpell(const CGHeroInstance *h, int spellID, const int3 &pos);

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & QID & states;
	}

	ui32 getQueryResult(ui8 player, int queryID);
	void sendMessageToAll(const std::string &message);
	void sendMessageTo(CConnection &c, const std::string &message);
	void applyAndAsk(Query * sel, ui8 player, boost::function<void(ui32)> &callback);
	void ask(Query * sel, ui8 player, const CFunctionList<void(ui32)> &callback);
	void sendToAllClients(CPackForClient * info);
	void sendAndApply(CPackForClient * info);
	void sendAndApply(SetGarrisons * info);
	void sendAndApply(SetResource * info);
	void sendAndApply(SetResources * info);
	void sendAndApply(NewStructures * info);

	void run(bool resume, const StartInfo *si = NULL);
	void newTurn();
	void handleAfterAttackCasting( const BattleAttack & bat );
	bool sacrificeArtifact(const IMarket * m, const CGHeroInstance * hero, ui32 artID);
	friend class CVCMIServer;
	friend class CScriptCallback;
};

#endif // __CGAMEHANDLER_H__
