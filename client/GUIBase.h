#ifndef __GUIBASE_H__
#define __GUIBASE_H__

#include "../global.h"
#include "SDL.h"
#include <set>
#include <list>
#include "../timeHandler.h"


#ifdef max
#undef max
#endif
#ifdef min
#undef min
#endif

/*
 * GUIBase.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

class CDefEssential;
class AdventureMapButton;
class CHighlightableButtonsGroup;
class CDefHandler;
struct HeroMoveDetails;
class CDefEssential;
class CGHeroInstance;
class CAdvMapInt;
class CCastleInterface;
class CBattleInterface;
class CStack;
class SComponent;
class CCreature;
struct SDL_Surface;
struct CPath;
class CCreatureAnimation;
class CSelectableComponent;
class CCreatureSet;
class CGObjectInstance;
class CSlider;
struct UpgradeInfo;
template <typename T> struct CondSh;
class CInGameConsole;
class CGarrisonInt;
class CInGameConsole;
class Component;
class CArmedInstance;
class CGTownInstance;
class StackState;
class CPlayerInterface;

struct Point
{
	int x, y;

	//constructors
	Point(){};
	Point(int X, int Y)
		:x(X),y(Y)
	{};
	Point(const int3 &a)
		:x(a.x),y(a.y)
	{}

	Point operator+(const Point &b) const
	{
		return Point(x+b.x,y+b.y);
	}
	Point& operator+=(const Point &b)
	{
		x += b.x;
		y += b.y;
		return *this;
	}
	Point operator-(const Point &b) const
	{
		return Point(x+b.x,y+b.y);
	}
	Point& operator-=(const Point &b)
	{
		x -= b.x;
		y -= b.y;
		return *this;
	}
	bool operator<(const Point &b) const //product order
	{
		return x < b.x   &&   y < b.y;
	}
};

struct Rect : public SDL_Rect
{
	Rect()//default c-tor
	{
		x = y = w = h = -1;
	}
	Rect(int X, int Y, int W, int H) //c-tor
	{
		x = X;
		y = Y;
		w = W;
		h = H;
	}
	Rect(const SDL_Rect & r) //c-tor
	{
		x = r.x;
		y = r.y;
		w = r.w;
		h = r.h;
	}
	bool isIn(int qx, int qy) const //determines if given point lies inside rect
	{
		if (qx > x   &&   qx<x+w   &&   qy>y   &&   qy<y+h)
			return true;
		return false;
	}
	bool isIn(const Point &q) const //determines if given point lies inside rect
	{
		return isIn(q.x,q.y);
	}
	Point topLeft() const //top left corner of this rect
	{
		return Point(x,y);
	}
	Point topRight() const //top right corner of this rect
	{
		return Point(x+w,y);
	}
	Point bottomLeft() const //bottom left corner of this rect
	{
		return Point(x,y+h);
	}
	Point bottomRight() const //bottom right corner of this rect
	{
		return Point(x+w,y+h);
	}
	Rect operator+(const Rect &p) const //moves this rect by p's rect position
	{
		return Rect(x+p.x,y+p.y,w,h);
	}
	Rect operator+(const Point &p) const //moves this rect by p's point position
	{
		return Rect(x+p.x,y+p.y,w,h);
	}
	Rect& operator=(const Rect &p) //assignment operator
	{
		x = p.x;
		y = p.y;
		w = p.w;
		h = p.h;
		return *this;
	}
	Rect& operator+=(const Rect &p) //works as operator+
	{
		x += p.x;
		y += p.y;
		return *this;
	}
	Rect& operator+=(const Point &p) //works as operator+
	{
		x += p.x;
		y += p.y;
		return *this;
	}
	Rect& operator-=(const Rect &p) //works as operator+
	{
		x -= p.x;
		y -= p.y;
		return *this;
	}
	Rect& operator-=(const Point &p) //works as operator+
	{
		x -= p.x;
		y -= p.y;
		return *this;
	}
	Rect operator&(const Rect &p) const //rect intersection
	{
		bool intersect = true;

		if(p.topLeft().y < y && p.bottomLeft().y < y) //rect p is above *this
		{
			intersect = false;
		}
		else if(p.topLeft().y > y+h && p.bottomLeft().y > y+h) //rect p is below *this
		{
			intersect = false;
		}
		else if(p.topLeft().x > x+w && p.topRight().x > x+w) //rect p is on the right hand side of this
		{
			intersect = false;
		}
		else if(p.topLeft().x < x && p.topRight().x < x) //rect p is on the left hand side of this
		{
			intersect = false;
		}

		if(intersect)
		{
			Rect ret;
			ret.x = std::max(this->x, p.x);
			ret.y = std::max(this->y, p.y);
			Point bR; //bottomRight point of returned rect
			bR.x = std::min(this->w+this->x, p.w+p.x);
			bR.y = std::min(this->h+this->y, p.h+p.y);
			ret.w = bR.x - ret.x;
			ret.h = bR.y - ret.y;
			return ret;
		}
		else
		{
			return Rect();
		}
	}
};

class IShowable
{
public:
	virtual void show(SDL_Surface * to)=0;
	virtual void showAll(SDL_Surface * to)
	{
		show(to);
	}
	virtual ~IShowable(){}; //d-tor
};

class IStatusBar
{
public:
	virtual ~IStatusBar(){}; //d-tor
	virtual void print(const std::string & text)=0; //prints text and refreshes statusbar
	virtual void clear()=0;//clears statusbar and refreshes
	virtual void show(SDL_Surface * to)=0; //shows statusbar (with current text)
	virtual std::string getCurrent()=0; //returns currently displayed text
};

class IActivable
{
public:
	virtual void activate()=0;
	virtual void deactivate()=0;
	virtual ~IActivable(){}; //d-tor
};
class IShowActivable : public IShowable, public IActivable
{
public:
	enum {WITH_GARRISON = 1};
	int type; //bin flags using etype
	IShowActivable();
	virtual ~IShowActivable(){}; //d-tor
};

class CIntObject : public IShowActivable //interface object
{
public:
	CIntObject *parent; //parent object
	std::vector<CIntObject *> children;

	Rect pos, //position of object on the screen
		posRelative; //position of object in the parent (not used if no parent)

	int ID; //object ID, rarely used by some classes for identification / internal info

	CIntObject();
	virtual ~CIntObject(){}; //d-tor

	//l-clicks handling
	bool pressedL; //for determining if object is L-pressed
	void activateLClick();
	void deactivateLClick();
	virtual void clickLeft(tribool down, bool previousState);

	//r-clicks handling
	bool pressedR; //for determining if object is R-pressed
	void activateRClick();
	void deactivateRClick();
	virtual void clickRight(tribool down, bool previousState);

	//hover handling
	bool hovered;  //for determining if object is hovered
	void activateHover();
	void deactivateHover();
	virtual void hover (bool on);

	//keyboard handling
	bool captureAllKeys; //if true, only this object should get info about pressed keys
	void activateKeys();
	void deactivateKeys();
	virtual void keyPressed(const SDL_KeyboardEvent & key);

	//mouse movement handling
	bool strongInterest; //if true - report all mouse movements, if not - only when hovered
	void activateMouseMove();
	void deactivateMouseMove();
	virtual void mouseMoved (const SDL_MouseMotionEvent & sEvent);

	//time handling
	int toNextTick;
	void activateTimer();
	void deactivateTimer();
	virtual void tick();

	enum {LCLICK=1, RCLICK=2, HOVER=4, MOVE=8, KEYBOARD=16, TIME=32};
	ui8 active;
	ui8 defActivation;

	void defActivate();
	void defDeactivate();
	void activate();
	void deactivate();
	void show(SDL_Surface * to);
};

//class for binding keys to left mouse button clicks
//classes wanting use it should have it as one of their base classes
class KeyShortcut : public virtual CIntObject
{
public:
	std::set<int> assignedKeys;
	KeyShortcut(){}; //c-tor
	KeyShortcut(int key){assignedKeys.insert(key);}; //c-tor
	KeyShortcut(std::set<int> Keys):assignedKeys(Keys){}; //c-tor
	virtual void keyPressed(const SDL_KeyboardEvent & key); //call-in
};

class CWindowWithGarrison : public CIntObject
{
public:
	CGarrisonInt *garr;
	CWindowWithGarrison();
};

class CSimpleWindow : public CIntObject
{
public:
	SDL_Surface * bitmap; //background
	CIntObject * owner; //who made this window
	virtual void show(SDL_Surface * to);
	CSimpleWindow():bitmap(NULL),owner(NULL){}; //c-tor
	virtual ~CSimpleWindow(); //d-tor
	void activate(){};
	void deactivate(){};
};

class CButtonBase : public KeyShortcut//basic buttton class
{
public:
	int bitmapOffset; //TODO: comment me
	int type; //advmapbutton=2 //TODO: comment me
	bool abs;//TODO: comment me
	bool active; //if true, this button is active and can be pressed
	bool notFreeButton; //TODO: comment me
	CIntObject * ourObj; // "owner"
	int state; //TODO: comment me
	std::vector< std::vector<SDL_Surface*> > imgs; //images for this button
	int curimg; //curently displayed image from imgs
	virtual void show(SDL_Surface * to);
	virtual void activate()=0;
	virtual void deactivate()=0;
	CButtonBase(); //c-tor
	virtual ~CButtonBase(); //d-tor
};

class CGuiHandler
{
public:
	timeHandler th;
	std::list<IShowActivable *> listInt; //list of interfaces - front=foreground; back = background (includes adventure map, window interfaces, all kind of active dialogs, and so on)

	//active GUI elements (listening for events
	std::list<CIntObject*> lclickable;
	std::list<CIntObject*> rclickable;
	std::list<CIntObject*> hoverable;
	std::list<CIntObject*> keyinterested;
	std::list<CIntObject*> motioninterested;
	std::list<CIntObject*> timeinterested;

	//objs to blit
	std::vector<IShowable*> objsToBlit;

	SDL_Event * current; //current event

	void totalRedraw(); //forces total redraw (using showAll)
	void simpleRedraw(); //update only top interface and draw background from buffer
	void popInt(IShowActivable *top); //removes given interface from the top and activates next
	void popIntTotally(IShowActivable *top); //deactivates, deletes, removes given interface from the top and activates next
	void pushInt(IShowActivable *newInt); //deactivate old top interface, activates this one and pushes to the top
	void popInts(int howMany); //pops one or more interfaces - deactivates top, deletes and removes given number of interfaces, activates new front
	IShowActivable *topInt(); //returns top interface
	void updateTime(); //handles timeInterested
	void handleEvents(); //takes events from queue and calls interested objects
	void handleEvent(SDL_Event *sEvent);

	void handleMouseMotion(SDL_Event *sEvent);
};

extern CGuiHandler GH; //global gui handler

#endif //__GUIBASE_H__
