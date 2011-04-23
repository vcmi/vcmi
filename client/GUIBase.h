#ifndef __GUIBASE_H__
#define __GUIBASE_H__

#include "../global.h"
#include "SDL.h"
#include <set>
#include <list>
#include "../timeHandler.h"
#include "FontBase.h"
#include "SDL_framerate.h"

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
struct Component;
class CArmedInstance;
class CGTownInstance;
class StackState;
class CPlayerInterface;

/// A point with x/y coordinate, used mostly for graphic rendering
struct Point
{
	int x, y;

	//constructors
	Point()
	{
		x = y = 0;
	};
	Point(int X, int Y)
		:x(X),y(Y)
	{};
	Point(const int3 &a)
		:x(a.x),y(a.y)
	{}
	Point(const SDL_MouseMotionEvent &a)
		:x(a.x),y(a.y)
	{}
	
	template<typename T>
	Point operator+(const T &b) const
	{
		return Point(x+b.x,y+b.y);
	}

	template<typename T>
	Point operator*(const T &mul) const
	{
		return Point(x*mul, y*mul);
	}

	template<typename T>
	Point& operator+=(const T &b)
	{
		x += b.x;
		y += b.y;
		return *this;
	}

	template<typename T>
	Point operator-(const T &b) const
	{
		return Point(x - b.x, y - b.y);
	}

	template<typename T>
	Point& operator-=(const T &b)
	{
		x -= b.x;
		y -= b.y;
		return *this;
	}
	bool operator<(const Point &b) const //product order
	{
		return x < b.x   &&   y < b.y;
	}
	template<typename T> Point& operator=(const T &t)
	{
		x = t.x;
		y = t.y;
		return *this;
	}
	template<typename T> bool operator==(const T &t) const
	{
		return x == t.x  &&  y == t.y;
	}
	template<typename T> bool operator!=(const T &t) const
	{
		return !(*this == t);
	}
};

/// Rectangle class, which have a position and a size
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
	Rect(const Point &position, const Point &size) //c-tor
	{
		x = position.x;
		y = position.y;
		w = size.x;
		h = size.y;
	}
	Rect(const SDL_Rect & r) //c-tor
	{
		x = r.x;
		y = r.y;
		w = r.w;
		h = r.h;
	}
	explicit Rect(const SDL_Surface * const &surf)
	{
		x = y = 0;
		w = surf->w;
		h = surf->h;
	}

	Rect centerIn(const Rect &r);
	static Rect createCentered(int w, int h);
	static Rect around(const Rect &r, int width = 1); //creates rect around another

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
	Rect& operator=(const Point &p) //assignment operator
	{
		x = p.x;
		y = p.y;
		return *this;
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
	template<typename T> Rect operator-(const T &t)
	{
		return Rect(x - t.x, y - t.y, w, h);
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

/// Defines a show method
class IShowable
{
public:
	void redraw();
	virtual void show(SDL_Surface * to)=0;
	virtual void showAll(SDL_Surface * to)
	{
		show(to);
	}
	virtual ~IShowable(){}; //d-tor
};

/// Status bar interface
class IStatusBar
{
public:
	virtual ~IStatusBar(){}; //d-tor
	virtual void print(const std::string & text)=0; //prints text and refreshes statusbar
	virtual void clear()=0;//clears statusbar and refreshes
	virtual void show(SDL_Surface * to)=0; //shows statusbar (with current text)
	virtual std::string getCurrent()=0; //returns currently displayed text
};

/// Defines a activate/deactive method
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
	enum {WITH_GARRISON = 1, BLOCK_ADV_HOTKEYS = 2, WITH_ARTIFACTS = 4};
	int type; //bin flags using etype
	IShowActivable();
	virtual ~IShowActivable(){}; //d-tor
};


class IUpdateable
{
public:
	virtual void update()=0;
	virtual ~IUpdateable(){}; //d-tor
};

/// Base UI element
class CIntObject : public IShowActivable //interface object
{
public:
	CIntObject *parent; //parent object
	std::vector<CIntObject *> children;

	Rect pos, //position of object on the screen
		posRelative; //position of object in the parent (not used if no parent)

	CIntObject();
	virtual ~CIntObject(); //d-tor

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

	//mouse wheel
	void activateWheel();
	void deactivateWheel();
	virtual void wheelScrolled(bool down, bool in);

	//double click
	void activateDClick();
	void deactivateDClick();
	virtual void onDoubleClick();

	enum {LCLICK=1, RCLICK=2, HOVER=4, MOVE=8, KEYBOARD=16, TIME=32, GENERAL=64, WHEEL=128, DOUBLECLICK=256, ALL=0xffff};
	ui16 active;
	ui16 used;

	enum {ACTIVATE=1, DEACTIVATE=2, UPDATE=4, SHOWALL=8, DISPOSE=16, SHARE_POS=32};
	ui8 defActions; //which calls will be tried to be redirected to children
	ui8 recActions; //which calls we allow te receive from parent

	enum EAlignment {TOPLEFT, CENTER, BOTTOMRIGHT};

	void disable(); //deactivates if needed, blocks all automatic activity, allows only disposal
	void enable(bool activation = true); //activates if needed, all activity enabled (Warning: may not be symetric with disable if recActions was limited!)
	void defActivate();
	void defDeactivate();
	void activate();
	void deactivate();
	void activate(ui16 what);
	void deactivate(ui16 what);
	void show(SDL_Surface * to);
	void showAll(SDL_Surface * to);

	void printAtLoc(const std::string & text, int x, int y, EFonts font, SDL_Color kolor, SDL_Surface * dst);
	void printToLoc(const std::string & text, int x, int y, EFonts font, SDL_Color kolor, SDL_Surface * dst);
	void printAtMiddleLoc(const std::string & text, int x, int y, EFonts font, SDL_Color kolor, SDL_Surface * dst);
	void printAtMiddleLoc(const std::string & text, const Point &p, EFonts font, SDL_Color kolor, SDL_Surface * dst);
	void printAtMiddleWBLoc(const std::string & text, int x, int y, EFonts font, int charpr, SDL_Color kolor, SDL_Surface * dst);
	void blitAtLoc(SDL_Surface * src, int x, int y, SDL_Surface * dst);
	void blitAtLoc(SDL_Surface * src, const Point &p, SDL_Surface * dst);
	bool isItInLoc(const SDL_Rect &rect, int x, int y);
	bool isItInLoc(const SDL_Rect &rect, const Point &p);
	const Rect & center(const Rect &r, bool propagate = true); //sets pos so that r will be in the center of screen, assigns sizes of r to pos, returns new position
	const Rect & center(const Point &p, bool propagate = true);  //moves object so that point p will be in its center
	const Rect & center(bool propagate = true); //centers when pos.w and pos.h are set, returns new position
	void fitToScreen(int borderWidth, bool propagate = true); //moves window to fit into screen
	void moveBy(const Point &p, bool propagate = true);
	void moveTo(const Point &p, bool propagate = true);
	void changeUsedEvents(ui16 what, bool enable, bool adjust = true);

	void addChild(CIntObject *child, bool adjustPosition = false);
	void removeChild(CIntObject *child, bool adjustPosition = false);
	void delChild(CIntObject *child); //removes from children list, deletes
	template <typename T> void delChildNUll(T *&child, bool deactivateIfNeeded = false) //removes from children list, deletes and sets pointer to NULL
	{
		if(!child)
			return;

		if(deactivateIfNeeded && child->active)
			child->deactivate();

		delChild(child);
		child = NULL;
	}
};

/// Class for binding keys to left mouse button clicks
/// Classes wanting use it should have it as one of their base classes
class KeyShortcut : public virtual CIntObject
{
public:
	std::set<int> assignedKeys;
	KeyShortcut(){}; //c-tor
	KeyShortcut(int key){assignedKeys.insert(key);}; //c-tor
	KeyShortcut(std::set<int> Keys):assignedKeys(Keys){}; //c-tor
	virtual void keyPressed(const SDL_KeyboardEvent & key); //call-in
};

/// to unify updating garrisons via PlayerInterface
class CGarrisonHolder : public virtual CIntObject
{
public:
	CGarrisonHolder();
	virtual void updateGarrisons(){};
};

class CWindowWithGarrison : public CGarrisonHolder
{
public:
	CGarrisonInt *garr;
	virtual void updateGarrisons();
};
/// Window GUI class
class CSimpleWindow : public CIntObject
{
public:
	SDL_Surface * bitmap; //background
	virtual void show(SDL_Surface * to);
	CSimpleWindow():bitmap(NULL){}; //c-tor
	virtual ~CSimpleWindow(); //d-tor
};
/// Image class
class CPicture : public CIntObject
{
public: 
	SDL_Surface *bg;
	Rect *srcRect; //if NULL then whole surface will be used
	bool freeSurf; //whether surface will be freed upon CPicture destruction
	bool needRefresh;//Surface needs to be displayed each frame

	operator SDL_Surface*()
	{
		return bg;
	}

	CPicture(const Rect &r, const SDL_Color &color, bool screenFormat = false); //rect filled with given color
	CPicture(const Rect &r, ui32 color, bool screenFormat = false); //rect filled with given color
	CPicture(SDL_Surface *BG, int x=0, int y=0, bool Free = true); //wrap existing SDL_Surface
	CPicture(const std::string &bmpname, int x=0, int y=0);
	CPicture(SDL_Surface *BG, const Rect &SrcRext, int x = 0, int y = 0, bool free = false); //wrap subrect of given surface
	void init();

	void createSimpleRect(const Rect &r, bool screenFormat, ui32 color);
	~CPicture();
	void show(SDL_Surface * to);
	void showAll(SDL_Surface * to);
	void convertToScreenBPP();
	void colorizeAndConvert(int player);
	void colorize(int player);
};

/// Handles GUI logic and drawing
class CGuiHandler
{
private:
	bool invalidateTotalRedraw;
	bool invalidateSimpleRedraw;

	void internalTotalRedraw();
	void internalSimpleRedraw();

public:
	FPSManager *mainFPSmng; //to keep const framerate
	timeHandler th;
	std::list<IShowActivable *> listInt; //list of interfaces - front=foreground; back = background (includes adventure map, window interfaces, all kind of active dialogs, and so on)
	IStatusBar * statusbar;

	//active GUI elements (listening for events
	std::list<CIntObject*> lclickable;
	std::list<CIntObject*> rclickable;
	std::list<CIntObject*> hoverable;
	std::list<CIntObject*> keyinterested;
	std::list<CIntObject*> motioninterested;
	std::list<CIntObject*> timeinterested;
	std::list<CIntObject*> wheelInterested;
	std::list<CIntObject*> doubleClickInterested;

	//objs to blit
	std::vector<IShowable*> objsToBlit;

	SDL_Event * current; //current event - can be set to NULL to stop handling event
	IUpdateable *curInt;

	Point lastClick;
	unsigned lastClickTime;
	bool terminate;

	CGuiHandler();
	~CGuiHandler();
	void run(); // holds the main loop for the whole program after initialization and manages the update/rendering system
	
	void totalRedraw(); //forces total redraw (using showAll), sets a flag, method gets called at the end of the rendering
	void simpleRedraw(); //update only top interface and draw background from buffer, sets a flag, method gets called at the end of the rendering
	
	void popInt(IShowActivable *top); //removes given interface from the top and activates next
	void popIntTotally(IShowActivable *top); //deactivates, deletes, removes given interface from the top and activates next
	void pushInt(IShowActivable *newInt); //deactivate old top interface, activates this one and pushes to the top
	void popInts(int howMany); //pops one or more interfaces - deactivates top, deletes and removes given number of interfaces, activates new front
	IShowActivable *topInt(); //returns top interface
	
	void updateTime(); //handles timeInterested
	void handleEvents(); //takes events from queue and calls interested objects
	void handleEvent(SDL_Event *sEvent);
	void handleMouseMotion(SDL_Event *sEvent);
	void handleMoveInterested( const SDL_MouseMotionEvent & motion );
	void fakeMouseMove();
	void breakEventHandling(); //current event won't be propagated anymore
	void drawFPSCounter(); // draws the FPS to the upper left corner of the screen
	ui8 defActionsDef; //default auto actions
	ui8 captureChildren; //all newly created objects will get their parents from stack and will be added to parents children list
	std::list<CIntObject *> createdObj; //stack of objs being created
};

extern CGuiHandler GH; //global gui handler

SDLKey arrowToNum(SDLKey key); //converts arrow key to according numpad key
SDLKey numToDigit(SDLKey key);//converts numpad digit key to normal digit key
bool isNumKey(SDLKey key, bool number = true); //checks if key is on numpad (numbers - check only for numpad digits)
bool isArrowKey(SDLKey key); 
CIntObject *  moveChild(CIntObject *obj, CIntObject *from, CIntObject *to, bool adjustPos = false);

template <typename T> void pushIntT()
{
	GH.pushInt(new T());
}

struct ObjectConstruction
{
	CIntObject *myObj;
	ObjectConstruction(CIntObject *obj);
	~ObjectConstruction();
};

struct SetCaptureState
{
	bool previousCapture;
	ui8 prevActions;
	SetCaptureState(bool allow, ui8 actions);
	~SetCaptureState();
};

#define OBJ_CONSTRUCTION ObjectConstruction obj__i(this)
#define OBJ_CONSTRUCTION_CAPTURING_ALL defActions = 255; SetCaptureState obj__i1(true, 255); ObjectConstruction obj__i(this)
#define BLOCK_CAPTURING SetCaptureState obj__i(false, 0)
#define BLOCK_CAPTURING_DONT_TOUCH_REC_ACTIONS SetCaptureState obj__i(false, GH.defActionsDef)

#endif //__GUIBASE_H__
