#ifndef CGAMEINTERFACE_H
#define CGAMEINTERFACE_H

#include "SDL.h"
#include <boost/logic/tribool.hpp>
#include "SDL_framerate.h"

BOOST_TRIBOOL_THIRD_STATE(outOfRange)

using namespace boost::logic;

class CAdvMapInt;
class CCallback;
class CHeroInstance;
class CDefHandler;
struct HeroMoveDetails;

class CIntObject //interface object
{
public:
	SDL_Rect pos;
	int ID;
};
class CButtonBase : public virtual CIntObject
{
public:
	int type; //advmapbutton=2
	bool abs;
	bool active;
	CIntObject * ourObj;
	int state;
	std::vector< std::vector<SDL_Surface*> > imgs;
	int curimg;
	virtual void show() ;
	virtual void activate()=0;
	virtual void deactivate()=0;
	CButtonBase();
};
class ClickableL : public virtual CIntObject  //for left-clicks
{
public:
	bool pressedL;
	ClickableL();
	virtual void clickLeft (tribool down)=0;
	virtual void activate()=0;
	virtual void deactivate()=0;
};
class ClickableR : public virtual CIntObject //for right-clicks
{
public:
	bool pressedR;
	ClickableR();
	virtual void clickRight (tribool down)=0;
	virtual void activate()=0;
	virtual void deactivate()=0;
};
class Hoverable  : public virtual CIntObject
{
public:
	Hoverable(){hovered=false;}
	bool hovered;
	virtual void hover (bool on)=0;
	virtual void activate()=0;
	virtual void deactivate()=0;
};
class KeyInterested : public virtual CIntObject
{
public:
	virtual void keyPressed (SDL_KeyboardEvent & key)=0;
	virtual void activate()=0;
	virtual void deactivate()=0;
};
class MotionInterested: public virtual CIntObject
{
public:
	virtual void mouseMoved (SDL_MouseMotionEvent & sEvent)=0;
	virtual void activate()=0;
	virtual void deactivate()=0;
};
class CGameInterface
{
public:
	bool human;
	int playerID, serialID;

	virtual void yourTurn()=0{};
	virtual void heroKilled(const CHeroInstance * hero)=0{};
	virtual void heroCreated(const CHeroInstance * hero)=0{};

	virtual void heroMoved(const HeroMoveDetails & details)=0;
};
class CGlobalAI : public CGameInterface // AI class (to derivate)
{
public:
	virtual void yourTurn(){};
	virtual void heroKilled(const CHeroInstance * hero){};
	virtual void heroCreated(const CHeroInstance * hero){};
};
class CPlayerInterface : public CGameInterface
{
public:
	SDL_Event * current;
	CAdvMapInt * adventureInt;
	FPSmanager * mainFPSmng;
	//TODO: town interace, battle interface, other interfaces

	CCallback * cb;

	std::vector<ClickableL*> lclickable;
	std::vector<ClickableR*> rclickable;
	std::vector<Hoverable*> hoverable;
	std::vector<KeyInterested*> keyinterested;
	std::vector<MotionInterested*> motioninterested;

	SDL_Surface * hInfo;

	//overloaded funcs from Interface
	void yourTurn();
	void heroMoved(const HeroMoveDetails & details);
	void heroKilled(const CHeroInstance * hero);
	void heroCreated(const CHeroInstance * hero);
	
	SDL_Surface * infoWin(void * specific); //specific=0 => draws info about selected town/hero //TODO - gdy sie dorobi sensowna hierarchie klas ins. to wywalic tego brzydkiego void*

	void handleEvent(SDL_Event * sEvent);
	void init(CCallback * CB);

	CPlayerInterface(int Player, int serial);
};
#endif //CGAMEINTERFACE_H