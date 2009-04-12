#ifndef __CLIENT_H__
#define __CLIENT_H__


#include "../global.h"
#include <boost/thread.hpp>
#include "../lib/IGameCallback.h"

struct StartInfo;
class CGameState;
class CGameInterface;
class CConnection;
class CCallback;
struct BattleAction;
struct SharedMem;
class CClient;
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
	SharedMem *shared;
	BattleAction *curbaction;

	void waitForMoveAndSend(int color);
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
	void removeObject(int objid){};
	void setBlockVis(int objid, bool bv){};
	void setOwner(int objid, ui8 owner){};
	void setHoverName(int objid, MetaString * name){};
	void setObjProperty(int objid, int prop, int val){};
	void changePrimSkill(int ID, int which, int val, bool abs=false){};
	void changeSecSkill(int ID, int which, int val, bool abs=false){}; 
	void showInfoDialog(InfoWindow *iw){};
	void showBlockingDialog(BlockingDialog *iw, const CFunctionList<void(ui32)> &callback){};
	ui32 showBlockingDialog(BlockingDialog *iw){return 0;}; //synchronous version of above
	void showGarrisonDialog(int upobj, int hid, const boost::function<void()> &cb){};
	void giveResource(int player, int which, int val){};
	void showCompInfo(ShowInInfobox * comp){};
	void heroVisitCastle(int obj, int heroID){};
	void stopHeroVisitCastle(int obj, int heroID){};
	void giveHeroArtifact(int artid, int hid, int position){}; //pos==-1 - first free slot in backpack=0; pos==-2 - default if available or backpack
	void startBattleI(const CCreatureSet * army1, const CCreatureSet * army2, int3 tile, const CGHeroInstance *hero1, const CGHeroInstance *hero2, boost::function<void(BattleResult*)> cb){}; //use hero=NULL for no hero
	void startBattleI(int heroID, CCreatureSet army, int3 tile, boost::function<void(BattleResult*)> cb){}; //for hero<=>neutral army
	void setAmount(int objid, ui32 val){};
	void moveHero(si32 hid, int3 dst, ui8 instant, ui8 asker = 255){};
	void giveHeroBonus(GiveBonus * bonus){};
	void setMovePoints(SetMovePoints * smp){};
	void setManaPoints(int hid, int val){};
	void giveHero(int id, int player){};
	void changeObjPos(int objid, int3 newPos, ui8 flags){};
	void sendAndApply(CPackForClient * info){};

	//////////////////////////////////////////////////////////////////////////
	friend class CCallback; //handling players actions
	friend void processCommand(const std::string &message, CClient *&client); //handling console
	
	
	static void runServer(const char * portc);
	void waitForServer();

	//////////////////////////////////////////////////////////////////////////

	template <typename Handler> void serialize(Handler &h, const int version);
};

#endif // __CLIENT_H__
