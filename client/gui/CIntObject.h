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
class CGuiHandler;
class CPicture;

struct SDL_KeyboardEvent;

using boost::logic::tribool;

// Defines a activate/deactive method
class IActivatable
{
public:
	virtual void activate()=0;
	virtual void deactivate()=0;
	virtual ~IActivatable(){};
};

class IUpdateable
{
public:
	virtual void update()=0;
	virtual ~IUpdateable(){};
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
	virtual ~IShowable(){};
};

class IShowActivatable : public IShowable, public IActivatable
{
public:
	//redraw parent flag - this int may be semi-transparent and require redraw of parent window
	enum {BLOCK_ADV_HOTKEYS = 2, REDRAW_PARENT=8};
	int type; //bin flags using etype
	IShowActivatable();
	virtual ~IShowActivatable(){};
};

enum class EIntObjMouseBtnType { LEFT, MIDDLE, RIGHT };
//typedef ui16 ActivityFlag;

// Base UI element
class CIntObject : public IShowActivatable //interface object
{
	ui16 used;//change via addUsed() or delUsed

	//time handling
	int toNextTick;
	int timerDelay;

	std::map<EIntObjMouseBtnType, bool> currentMouseState;

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


/*
 * Public interface
 */

	/// read-only parent access. May not be a "clean" solution but allows some compatibility
	CIntObject * const & parent;

	/// position of object on the screen. Please do not modify this anywhere but in constructor - use moveBy\moveTo instead
	/*const*/ Rect pos;

	CIntObject(int used=0, Point offset=Point());
	virtual ~CIntObject();

	void updateMouseState(EIntObjMouseBtnType btn, bool state) { currentMouseState[btn] = state; }
	bool mouseState(EIntObjMouseBtnType btn) const { return currentMouseState.count(btn) ? currentMouseState.at(btn) : false; }

	virtual void click(EIntObjMouseBtnType btn, tribool down, bool previousState);
	virtual void clickLeft(tribool down, bool previousState) {}
	virtual void clickRight(tribool down, bool previousState) {}
	virtual void clickMiddle(tribool down, bool previousState) {}

	//hover handling
	/*const*/ bool hovered;  //for determining if object is hovered
	virtual void hover (bool on){}

	//keyboard handling
	bool captureAllKeys; //if true, only this object should get info about pressed keys
	virtual void keyPressed(const SDL_KeyboardEvent & key){}
	virtual bool captureThisEvent(const SDL_KeyboardEvent & key); //allows refining captureAllKeys against specific events (eg. don't capture ENTER)

	virtual void textInputed(const SDL_TextInputEvent & event){};
	virtual void textEdited(const SDL_TextEditingEvent & event){};

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

	// These are the arguments that can be used to determine what kind of input the CIntObject will receive
	enum {LCLICK=1, RCLICK=2, HOVER=4, MOVE=8, KEYBOARD=16, TIME=32, GENERAL=64, WHEEL=128, DOUBLECLICK=256, TEXTINPUT=512, MCLICK=1024, ALL=0xffff};
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
	void activate() override;
	void deactivate() override;

	//called each frame to update screen
	void show(SDL_Surface * to) override;
	//called on complete redraw only
	void showAll(SDL_Surface * to) override;
	//request complete redraw of this object
	void redraw() override;

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

	//functions for printing text. Use CLabel where possible instead
	void printAtLoc(const std::string & text, int x, int y, EFonts font, SDL_Color color, SDL_Surface * dst);
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
	CKeyShortcut();
	CKeyShortcut(int key);
	CKeyShortcut(std::set<int> Keys);
	virtual void keyPressed(const SDL_KeyboardEvent & key) override; //call-in
};

class WindowBase : public CIntObject
{
public:
	WindowBase(int used_ = 0, Point pos_ = Point());
protected:
	void close();
};
