#ifndef CGAMEINTERFACE_H
#define CGAMEINTERFACE_H

#include "SDL.h"
#include "CDefHandler.h"
#include "SDL_Extensions.h"
#include <boost/logic/tribool.hpp>
BOOST_TRIBOOL_THIRD_STATE(outOfRange)
using namespace boost::logic;
class CAdvMapInt;

class CIntObject //interface object
{
public:
	SDL_Rect pos;
	int ID;
};
class CButtonBase : public CIntObject
{
public:
	int type; //advmapbutton=2
	bool abs;
	bool active;
	CIntObject * ourObj;
	int state;
	std::vector<SDL_Surface*> imgs;
	virtual void show() ;
	virtual void activate()=0;
	virtual void deactivate()=0;
	CButtonBase();
};
class ClickableL  //for left-clicks
{
public:
	bool pressed;
	virtual void clickLeft (tribool down)=0;
	virtual void activate()=0;
	virtual void deactivate()=0;
};
class ClickableR //for right-clicks
{
public:
	bool pressed;
	virtual void clickRight (tribool down)=0;
	virtual void activate()=0;
	virtual void deactivate()=0;
};
class Hoverable 
{
public:
	bool hovered;
	virtual void hover (bool on)=0;
	virtual void activate()=0;
	virtual void deactivate()=0;
};
class KeyInterested
{
public:
	virtual void keyPressed (SDL_KeyboardEvent & key)=0;
	virtual void activate()=0;
	virtual void deactivate()=0;
};
class CGameInterface
{
public:
	bool human;
	int playerID;

	virtual void yourTurn()=0{};
};
class CGlobalAI : public CGameInterface // callback for AI
{
public:
	virtual void yourTurn(){};
};
class CPlayerInterface : public CGameInterface
{
public:
	CAdvMapInt * adventureInt;
	//TODO: town interace, battle interface, other interfaces

	std::vector<ClickableL*> lclickable;
	std::vector<ClickableR*> rclickable;
	std::vector<Hoverable*> hoverable;
	std::vector<KeyInterested*> keyinterested;

	void yourTurn();
	void handleEvent(SDL_Event * sEvent);

	CPlayerInterface(int Player);
};
#endif //CGAMEINTERFACE_H