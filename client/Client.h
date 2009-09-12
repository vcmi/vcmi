#ifndef __CLIENT_H__
#define __CLIENT_H__


#include "../global.h"
#include <boost/thread.hpp>
#include "../lib/IGameCallback.h"
#include "../lib/CondSh.h"

/*
 * Client.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

struct StartInfo;
class CGameState;
class CGameInterface;
class CConnection;
class CCallback;
struct BattleAction;
struct SharedMem;
class CClient;
struct CPathsInfo;

void processCommand(const std::string &message, CClient *&client);
namespace boost
{
	class mutex;
	class condition_variable;
}

template <typename T>
struct CSharedCond
{
	boost::mutex *mx;
	boost::condition_variable *cv;
	T *res;
	CSharedCond(T*R)
	{
		res = R;
		mx = new boost::mutex;
		cv = new boost::condition_variable;
	}
	~CSharedCond()
	{
		delete res;
		delete mx;
		delete cv;
	}
};

class CClient : public IGameCallback
{
public:
	CCallback *cb;
	std::set<CCallback*> callbacks; //callbacks given to player interfaces
	std::map<ui8,CGameInterface *> playerint;
	CConnection *serv;
	bool must_close;
	SharedMem *shared;
	BattleAction *curbaction;
	CPathsInfo *pathInfo;

	CondSh<bool> waitingRequest;

	void waitForMoveAndSend(int color);
	//void sendRequest(const CPackForServer *request, bool waitForRealization);
	CClient(void);
	CClient(CConnection *con, StartInfo *si);
	~CClient(void);

	void init();
	void close();
	void newGame(CConnection *con, StartInfo *si); //con - connection to server
	void save(const std::string & fname);
	void load(const std::string & fname);
	void run();
	//////////////////////////////////////////////////////////////////////////
	//from IGameCallback
	int getCurrentPlayer();
	int getSelectedHero();

	//not working yet, will be implement somewhen later with support for local-sim-based gameplay
	void changeSpells(int hid, bool give, const std::set<ui32> &spells){};
	bool removeObject(int objid){return false;};
	void setBlockVis(int objid, bool bv){};
	void setOwner(int objid, ui8 owner){};
	void setHoverName(int objid, MetaString * name){};
	void setObjProperty(int objid, int prop, si64 val){};
	void changePrimSkill(int ID, int which, si64 val, bool abs=false){};
	void changeSecSkill(int ID, int which, int val, bool abs=false){}; 
	void showInfoDialog(InfoWindow *iw){};
	void showBlockingDialog(BlockingDialog *iw, const CFunctionList<void(ui32)> &callback){};
	ui32 showBlockingDialog(BlockingDialog *iw){return 0;}; //synchronous version of above
	void showGarrisonDialog(int upobj, int hid, bool removableUnits, const boost::function<void()> &cb){};
	void giveResource(int player, int which, int val){};
	void giveCreatures (int objid, const CGHeroInstance * h, CCreatureSet *creatures){};
	void showCompInfo(ShowInInfobox * comp){};
	void heroVisitCastle(int obj, int heroID){};
	void stopHeroVisitCastle(int obj, int heroID){};
	void giveHeroArtifact(int artid, int hid, int position){}; //pos==-1 - first free slot in backpack=0; pos==-2 - default if available or backpack
	void startBattleI(const CArmedInstance *army1, const CArmedInstance *army2, int3 tile, const CGHeroInstance *hero1, const CGHeroInstance *hero2, bool creatureBank = false, boost::function<void(BattleResult*)> cb = 0, const CGTownInstance *town = NULL){}; //use hero=NULL for no hero
	void startBattleI(const CArmedInstance *army1, const CArmedInstance *army2, int3 tile, boost::function<void(BattleResult*)> cb = 0, bool creatureBank = false){}; //if any of armies is hero, hero will be used
	void startBattleI(const CArmedInstance *army1, const CArmedInstance *army2, boost::function<void(BattleResult*)> cb = 0, bool creatureBank = false){}; //if any of armies is hero, hero will be used, visitable tile of second obj is place of battle
	void setAmount(int objid, ui32 val){};
	bool moveHero(si32 hid, int3 dst, ui8 instant, ui8 asker = 255){return false;};
	void giveHeroBonus(GiveBonus * bonus){};
	void setMovePoints(SetMovePoints * smp){};
	void setManaPoints(int hid, int val){};
	void giveHero(int id, int player){};
	void changeObjPos(int objid, int3 newPos, ui8 flags){};
	void sendAndApply(CPackForClient * info){};
	void heroExchange(si32 hero1, si32 hero2){};

	//////////////////////////////////////////////////////////////////////////
	friend class CCallback; //handling players actions
	friend void processCommand(const std::string &message, CClient *&client); //handling console
	
	
	static void runServer(const char * portc);
	void waitForServer();

	//////////////////////////////////////////////////////////////////////////

	template <typename Handler> void serialize(Handler &h, const int version);
};

#endif // __CLIENT_H__
