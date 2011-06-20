#ifndef __CGAMEINTERFACE_H__
#define __CGAMEINTERFACE_H__
#include "global.h"
#include <set>
#include <vector>
#include "lib/BattleAction.h"
#include "client/FunctionList.h"
#include "lib/IGameEventsReceiver.h"

/*
 * CGameInterface.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

using namespace boost::logic;
class CCallback;
class CBattleCallback;
class ICallback;
class CGlobalAI;
struct Component;
class CSelectableComponent;
struct TryMoveHero;
class CGHeroInstance;
class CGTownInstance;
class CGObjectInstance;
class CGBlackMarket;
class CGDwelling;
class CCreatureSet;
class CArmedInstance;
class IShipyard;
class IMarket;
struct BattleResult;
struct BattleAttack;
struct BattleStackAttacked;
struct BattleSpellCast;
struct SetStackEffect;
struct Bonus;
struct PackageApplied;
struct SetObjectProperty;
struct CatapultAttack;
struct BattleStacksRemoved;
struct StackLocation;
class CStackInstance;
class CStack;
class CCreature;
class CLoadFile;
class CSaveFile;
typedef si32 TQuantity;
template <typename Serializer> class CISer;
template <typename Serializer> class COSer;
struct ArtifactLocation;
class CScriptingModule;

class CBattleGameInterface : public IBattleEventsReceiver
{
public:
	bool human;
	int playerID;
	std::string dllName;

	virtual ~CBattleGameInterface() {};
	virtual void init(CBattleCallback * CB){};

	//battle call-ins
	virtual BattleAction activeStack(const CStack * stack)=0; //called when it's turn of that stack
};

/// Central class for managing human player / AI interface logic
class CGameInterface : public CBattleGameInterface, public IGameEventsReceiver
{
public:
	virtual void init(CCallback * CB){};
	virtual void yourTurn(){};
	virtual void heroGotLevel(const CGHeroInstance *hero, int pskill, std::vector<ui16> &skills, boost::function<void(ui32)> &callback)=0; //pskill is gained primary skill, interface has to choose one of given skills and call callback with selection id
	virtual void showBlockingDialog(const std::string &text, const std::vector<Component> &components, ui32 askID, const int soundID, bool selection, bool cancel) = 0; //Show a dialog, player must take decision. If selection then he has to choose between one of given components, if cancel he is allowed to not choose. After making choice, CCallback::selectionMade should be called with number of selected component (1 - n) or 0 for cancel (if allowed) and askID.
	virtual void showGarrisonDialog(const CArmedInstance *up, const CGHeroInstance *down, bool removableUnits, boost::function<void()> &onEnd) = 0; //all stacks operations between these objects become allowed, interface has to call onEnd when done
	virtual void serialize(COSer<CSaveFile> &h, const int version){}; //saving
	virtual void serialize(CISer<CLoadFile> &h, const int version){}; //loading
};

class CDynLibHandler
{
public:
	static CGlobalAI * getNewAI(std::string dllname);
	static CBattleGameInterface * getNewBattleAI(std::string dllname);
	static CScriptingModule * getNewScriptingModule(std::string dllname);
};

class CGlobalAI : public CGameInterface // AI class (to derivate)
{
public:
	//CGlobalAI();
	virtual void yourTurn() OVERRIDE{};
	virtual void heroKilled(const CGHeroInstance*){};
	virtual void heroCreated(const CGHeroInstance*) OVERRIDE{};
	virtual void battleStackMoved(const CStack * stack, THex dest, int distance, bool end) OVERRIDE{};
	virtual void battleStackAttacking(int ID, int dest) {};
	virtual void battleStacksAttacked(const std::vector<BattleStackAttacked> & bsa) OVERRIDE{};
	virtual BattleAction activeStack(const CStack * stack) OVERRIDE;
};

#endif // __CGAMEINTERFACE_H__
