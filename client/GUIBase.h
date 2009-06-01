#ifndef __GUIBASE_H__
#define __GUIBASE_H__

#include "../global.h"
#include "SDL.h"
#include <set>

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

class CWindowWithGarrison : public IShowActivable
{
public:
	CGarrisonInt *garr;
	CWindowWithGarrison();
};

class CMainInterface : public IShowActivable
{
public:
	IShowActivable *subInt;
};
class CIntObject //interface object
{
public:
	Rect pos; //position of object on the screen
	int ID; //object unique ID, rarely (if at all) used

	//virtual bool isIn(int x, int y)
	//{
	//	return pos.isIn(x,y);
	//}
	virtual ~CIntObject(){}; //d-tor
};
class CSimpleWindow : public IShowActivable, public virtual CIntObject
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
class CButtonBase : public virtual CIntObject, public IShowable, public IActivable //basic buttton class
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
class ClickableL : public virtual CIntObject  //for left-clicks
{
public:
	bool pressedL; //for determining if object is L-pressed
	ClickableL(); //c-tor
	virtual ~ClickableL();//{};//d-tor
	virtual void clickLeft (boost::logic::tribool down)=0;
	virtual void activate();
	virtual void deactivate();
};
class ClickableR : public virtual CIntObject //for right-clicks
{
public:
	bool pressedR; //for determining if object is R-pressed
	ClickableR(); //c-tor
	virtual ~ClickableR();//{};//d-tor
	virtual void clickRight (boost::logic::tribool down)=0;
	virtual void activate()=0;
	virtual void deactivate()=0;
};
class Hoverable  : public virtual CIntObject
{
public:
	Hoverable() : hovered(false){} //c-tor
	virtual ~Hoverable();//{}; //d-tor
	bool hovered;  //for determining if object is hovered
	virtual void hover (bool on)=0;
	virtual void activate()=0;
	virtual void deactivate()=0;
};
class KeyInterested : public virtual CIntObject
{	
public:
	bool captureAllKeys; //if true, only this object should get info about pressed keys
	KeyInterested(): captureAllKeys(false){}
	virtual ~KeyInterested();//{};//d-tor
	virtual void keyPressed(const SDL_KeyboardEvent & key)=0;
	virtual void activate()=0;
	virtual void deactivate()=0;
};

//class for binding keys to left mouse button clicks
//classes wanting use it should have it as one of their base classes
class KeyShortcut : public KeyInterested, public ClickableL
{
public:
	std::set<int> assignedKeys;
	KeyShortcut(){}; //c-tor
	KeyShortcut(int key){assignedKeys.insert(key);}; //c-tor
	KeyShortcut(std::set<int> Keys):assignedKeys(Keys){}; //c-tor
	virtual void keyPressed(const SDL_KeyboardEvent & key); //call-in
};

class MotionInterested: public virtual CIntObject
{
public:
	bool strongInterest; //if true - report all mouse movements, if not - only when hovered
	MotionInterested(){strongInterest=false;};
	virtual ~MotionInterested(){};//d-tor
	virtual void mouseMoved (const SDL_MouseMotionEvent & sEvent)=0;
	virtual void activate()=0;
	virtual void deactivate()=0;
};
class TimeInterested: public virtual CIntObject
{
public:
	virtual ~TimeInterested(){}; //d-tor
	int toNextTick;
	virtual void tick()=0;
	virtual void activate();
	virtual void deactivate();
};



#endif //__GUIBASE_H__
