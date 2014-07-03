/*
 * CIntObject.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
 
#pragma once

#include <SDL_events.h>
#include "Geometries.h"
#include "../Graphics.h"

struct SDL_Surface;
class CPicture;
class CGuiHandler;

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

class ILockedUpdatable: protected IUpdateable
{
	boost::recursive_mutex updateGuard;
public:
	virtual void runLocked(std::function<void(IUpdateable * )> cb);	
	virtual ~ILockedUpdatable(){}; //d-tor	
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
	enum {BLOCK_ADV_HOTKEYS = 2, REDRAW_PARENT=8};
	int type; //bin flags using etype
	IShowActivatable();
	virtual ~IShowActivatable(){}; //d-tor
};

//typedef ui16 ActivityFlag;

// Base UI element
class CIntObject : public IShowActivatable //interface object
{

	ui16 used;//change via addUsed() or delUsed

	//time handling
	int toNextTick;
	int timerDelay;

	void onTimer(int timePassed);

	//non-const versions of fields to allow changing them in CIntObject
	CIntObject *parent_m; //parent object
	ui16 active_m;
protected:
	//activate or deactivate specific action (LCLICK, RCLICK...)
	void activate(ui16 what);
	void deactivate(ui16 what);

public:
/*
 * Functions and fields that supposed to be private but are not for now.
 * Don't use them unless you really know what they are for
 */
	std::vector<CIntObject *> children;
	
	//FIXME: workaround
	void deactivateKeyboard()
	{
		deactivate(KEYBOARD);
	}

/*
 * Public interface
 */

	/// read-only parent access. May not be a "clean" solution but allows some compatibility
	CIntObject * const & parent;

	/// position of object on the screen. Please do not modify this anywhere but in constructor - use moveBy\moveTo instead
	/*const*/ Rect pos;

	CIntObject(int used=0, Point offset=Point());
	virtual ~CIntObject(); //d-tor

	//l-clicks handling
	/*const*/ bool pressedL; //for determining if object is L-pressed
	virtual void clickLeft(tribool down, bool previousState){}

	//r-clicks handling
	/*const*/ bool pressedR; //for determining if object is R-pressed
	virtual void clickRight(tribool down, bool previousState){}

	//hover handling
	/*const*/ bool hovered;  //for determining if object is hovered
	virtual void hover (bool on){}

	//keyboard handling
	bool captureAllKeys; //if true, only this object should get info about pressed keys
	virtual void keyPressed(const SDL_KeyboardEvent & key){}
	virtual bool captureThisEvent(const SDL_KeyboardEvent & key); //allows refining captureAllKeys against specific events (eg. don't capture ENTER)
	
#ifndef VCMI_SDL1
	virtual void textInputed(const SDL_TextInputEvent & event){};
	virtual void textEdited(const SDL_TextEditingEvent & event){};
#endif // VCMI_SDL1

	//mouse movement handling
	bool strongInterest; //if true - report all mouse movements, if not - only when hovered
	virtual void mouseMoved (const SDL_MouseMotionEvent & sEvent){}

	//time handling
	void setTimer(int msToTrigger);//set timer delay and activate timer if needed.
	virtual void tick(){}

	//mouse wheel
	virtual void wheelScrolled(bool down, bool in){}

	//double click
	virtual void onDoubleClick(){}

	enum {LCLICK=1, RCLICK=2, HOVER=4, MOVE=8, KEYBOARD=16, TIME=32, GENERAL=64, WHEEL=128, DOUBLECLICK=256, TEXTINPUT=512, ALL=0xffff};
	const ui16 & active;
	void addUsedEvents(ui16 newActions);
	void removeUsedEvents(ui16 newActions);

	enum {ACTIVATE=1, DEACTIVATE=2, UPDATE=4, SHOWALL=8, DISPOSE=16, SHARE_POS=32};
	ui8 defActions; //which calls will be tried to be redirected to children
	ui8 recActions; //which calls we allow to receive from parent

	void disable(); //deactivates if needed, blocks all automatic activity, allows only disposal
	void enable(); //activates if needed, all activity enabled (Warning: may not be symetric with disable if recActions was limited!)

	// activate or deactivate object. Inactive object won't receive any input events (keyboard\mouse)
	// usually used automatically by parent
	void activate();
	void deactivate();

	//called each frame to update screen
	void show(SDL_Surface * to);
	//called on complete redraw only
	void showAll(SDL_Surface * to);
	//request complete redraw of this object
	void redraw();

	enum EAlignment {TOPLEFT, CENTER, BOTTOMRIGHT};

	bool isItInLoc(const SDL_Rect &rect, int x, int y);
	bool isItInLoc(const SDL_Rect &rect, const Point &p);
	const Rect & center(const Rect &r, bool propagate = true); //sets pos so that r will be in the center of screen, assigns sizes of r to pos, returns new position
	const Rect & center(const Point &p, bool propagate = true);  //moves object so that point p will be in its center
	const Rect & center(bool propagate = true); //centers when pos.w and pos.h are set, returns new position
	void fitToScreen(int borderWidth, bool propagate = true); //moves window to fit into screen
	void moveBy(const Point &p, bool propagate = true);
	void moveTo(const Point &p, bool propagate = true);//move this to new position, coordinates are absolute (0,0 is topleft screen corner)

	void addChild(CIntObject *child, bool adjustPosition = false);
	void removeChild(CIntObject *child, bool adjustPosition = false);
	//delChild - not needed, use normal "delete child" instead
	//delChildNull - not needed, use "vstd::clear_pointer(child)" instead

/*
 * Functions that should be used only by specific GUI elements. Don't use them unless you really know why they are here
 */
	//wrappers for CSDL_Ext methods. This versions use coordinates relative to pos
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

	friend class CGuiHandler;
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
