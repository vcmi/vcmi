#ifndef CGAMEINTERFACE_H
#define CGAMEINTERFACE_H
#include "global.h"
#include "CCallback.h"
BOOST_TRIBOOL_THIRD_STATE(outOfRange)

using namespace boost::logic;
class CCallback;
class CGlobalAI;
class CGHeroInstance;
class CSelectableComponent;
class CGameInterface
{
public:
	bool human;
	int playerID, serialID;

	virtual void init(ICallback * CB)=0{};
	virtual void yourTurn()=0{};
	virtual void heroKilled(const CGHeroInstance*)=0{};
	virtual void heroCreated(const CGHeroInstance*)=0{};
	virtual void heroPrimarySkillChanged(const CGHeroInstance * hero, int which, int val)=0{};
	virtual void heroMoved(const HeroMoveDetails & details)=0{};
	virtual void heroVisitsTown(const CGHeroInstance* hero, const CGTownInstance * town){};
	virtual void tileRevealed(int3 pos){};
	virtual void tileHidden(int3 pos){};
	virtual void receivedResource(int type, int val){};
	virtual void showSelDialog(std::string text, std::vector<CSelectableComponent*> & components, int askID)=0{};
	virtual void garrisonChanged(const CGObjectInstance * obj){};
};
class CAIHandler
{
public:
	static CGlobalAI * getNewAI(CCallback * cb, std::string dllname);
};
class CGlobalAI : public CGameInterface // AI class (to derivate)
{
public:
	//CGlobalAI();
	virtual void yourTurn(){};
	virtual void heroKilled(const CGHeroInstance*){};
	virtual void heroCreated(const CGHeroInstance*){};
};
#endif //CGAMEINTERFACE_H