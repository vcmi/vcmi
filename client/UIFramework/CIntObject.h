#pragma once

#include <SDL_events.h>
#include "Geometries.h"
#include "../FontBase.h"

struct SDL_Surface;

/*
 * CIntObject.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

using boost::logic::tribool;

// Defines a activate/deactive method
class IActivatable
{
public:
	virtual void activate()=0;
	virtual void deactivate()=0;
	virtual ~IActivatable(){}; //d-tor
};

class IUpdateable
{
public:
	virtual void update()=0;
	virtual ~IUpdateable(){}; //d-tor
};

// Defines a show method
class IShowable
{
public:
	virtual void redraw()=0;
	virtual void show(SDL_Surface * to) = 0;
	virtual void showAll(SDL_Surface * to)
	{
		show(to);
	}
	virtual ~IShowable(){}; //d-tor
};

class IShowActivatable : public IShowable, public IActivatable
{
public:
	//redraw parent flag - this int may be semi-transparent and require redraw of parent window
	enum {WITH_GARRISON = 1, BLOCK_ADV_HOTKEYS = 2, WITH_ARTIFACTS = 4, REDRAW_PARENT=8};
	int type; //bin flags using etype
	IShowActivatable();
	virtual ~IShowActivatable(){}; //d-tor
};

// Status bar interface
class IStatusBar
{
public:
	virtual ~IStatusBar(){}; //d-tor
	virtual void print(const std::string & text)=0; //prints text and refreshes statusbar
	virtual void clear()=0;//clears statusbar and refreshes
	virtual void show(SDL_Surface * to)=0; //shows statusbar (with current text)
	virtual std::string getCurrent()=0; //returns currently displayed text
};

// Base UI element
class CIntObject : public IShowActivatable //interface object
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
	virtual bool captureThisEvent(const SDL_KeyboardEvent & key); //allows refining captureAllKeys against specific events (eg. don't capture ENTER)

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

	// activate or deactivate object. Inactive object won't receive any input events (keyboard\mouse)
	// usually used automatically by parent
	void activate();
	void deactivate();

	//activate or deactivate specific action (LCLICK, RCLICK...)
	void activate(ui16 what);
	void deactivate(ui16 what);

	//called each frame to update screen
	void show(SDL_Surface * to);
	//called on complete redraw only
	void showAll(SDL_Surface * to);
	//request complete redraw
	void redraw();

	void drawBorderLoc(SDL_Surface * sur, const Rect &r, const int3 &color);
	//functions for printing text. Use CLabel where possible instead
	void printAtLoc(const std::string & text, int x, int y, EFonts font, SDL_Color color, SDL_Surface * dst);
	void printToLoc(const std::string & text, int x, int y, EFonts font, SDL_Color color, SDL_Surface * dst);
	void printAtMiddleLoc(const std::string & text, int x, int y, EFonts font, SDL_Color color, SDL_Surface * dst);
	void printAtMiddleLoc(const std::string & text, const Point &p, EFonts font, SDL_Color color, SDL_Surface * dst);
	void printAtMiddleWBLoc(const std::string & text, int x, int y, EFonts font, int charsPerLine, SDL_Color color, SDL_Surface * dst);

	//image blitting. If possible use CPicture or CAnimImage instead
	void blitAtLoc(SDL_Surface * src, int x, int y, SDL_Surface * dst);
	void blitAtLoc(SDL_Surface * src, const Point &p, SDL_Surface * dst);

	bool isItInLoc(const SDL_Rect &rect, int x, int y);
	bool isItInLoc(const SDL_Rect &rect, const Point &p);
	const Rect & center(const Rect &r, bool propagate = true); //sets pos so that r will be in the center of screen, assigns sizes of r to pos, returns new position
	const Rect & center(const Point &p, bool propagate = true);  //moves object so that point p will be in its center
	const Rect & center(bool propagate = true); //centers when pos.w and pos.h are set, returns new position
	void fitToScreen(int borderWidth, bool propagate = true); //moves window to fit into screen
	void moveBy(const Point &p, bool propagate = true);
	void moveTo(const Point &p, bool propagate = true);//move this to new position, coordinates are absolute (0,0 is topleft screen corner)
	void changeUsedEvents(ui16 what, bool enable, bool adjust = true);

	//add child without parent to this. Use CGuiHandler::moveChild() if child already have parent
	void addChild(CIntObject *child, bool adjustPosition = false);
	//remove child from this without deleting
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
class CKeyShortcut : public virtual CIntObject
{
public:
	std::set<int> assignedKeys;
	CKeyShortcut(){}; //c-tor
	CKeyShortcut(int key){assignedKeys.insert(key);}; //c-tor
	CKeyShortcut(std::set<int> Keys):assignedKeys(Keys){}; //c-tor
	virtual void keyPressed(const SDL_KeyboardEvent & key); //call-in
};