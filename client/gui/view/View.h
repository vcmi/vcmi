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
#include "../Geometries.h"
#include "../../Graphics.h"

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

class View : public IShowActivatable
{
public:
	View(ui16 used=0, Point offset=Point());
	virtual ~View();
	
	View *getParent() const;
	void setParent(View *parent);
	
	void updateMouseState(EIntObjMouseBtnType btn, bool state) { currentMouseState[btn] = state; }
	bool mouseState(EIntObjMouseBtnType btn) const { return currentMouseState.count(btn) ? currentMouseState.at(btn) : false; }

	virtual void click(EIntObjMouseBtnType btn, tribool down, bool previousState);
	virtual void clickLeft(tribool down, bool previousState) {}
	virtual void clickRight(tribool down, bool previousState) {}
	virtual void clickMiddle(tribool down, bool previousState) {}

	bool hovered;
	virtual void hover (bool on){}

	bool captureAllKeys;
	virtual void keyPressed(const SDL_KeyboardEvent & key){}
	virtual bool captureThisEvent(const SDL_KeyboardEvent & key);

	virtual void textInputed(const SDL_TextInputEvent & event){};
	virtual void textEdited(const SDL_TextEditingEvent & event){};

	bool strongInterest;
	virtual void mouseMoved (const SDL_MouseMotionEvent & sEvent){}

	void setTimer(int msToTrigger);
	virtual void tick(){}

	virtual void wheelScrolled(bool down, bool in){}

	virtual void onDoubleClick(){}

	enum {LCLICK=1, RCLICK=2, HOVER=4, MOVE=8, KEYBOARD=16, TIME=32, GENERAL=64, WHEEL=128, DOUBLECLICK=256, TEXTINPUT=512, MCLICK=1024, ALL=0xffff};
	const ui16 & active;
	void addUsedEvents(ui16 newActions);
	void removeUsedEvents(ui16 newActions);

	enum {ACTIVATE=1, DEACTIVATE=2, UPDATE=4, SHOWALL=8, DISPOSE=16, SHARE_POS=32};

	void disable();
	void enable();

	void activate() override;
	void deactivate() override;

	void show(SDL_Surface * to) override;
	void showAll(SDL_Surface * to) override;
	void redraw() override;

	enum EAlignment {TOPLEFT, CENTER, BOTTOMRIGHT};
	
	const Rect & center(const Rect &r, bool propagate = true);
	const Rect & center(const Point &p, bool propagate = true);
	const Rect & center(bool propagate = true);
	void fitToScreen(int borderWidth, bool propagate = true);
	void moveBy(const Point &p, bool propagate = true);
	void moveTo(const Point &p, bool propagate = true);

	void addChild(View *child, bool adjustPosition = false);
	void removeChild(View *child, bool adjustPosition = false);

	void printAtMiddleLoc(const std::string & text, int x, int y, EFonts font, SDL_Color color, SDL_Surface * dst);
	void printAtMiddleLoc(const std::string & text, const Point &p, EFonts font, SDL_Color color, SDL_Surface * dst);
	void printAtMiddleWBLoc(const std::string & text, int x, int y, EFonts font, int charsPerLine, SDL_Color color, SDL_Surface * dst);

	void blitAtLoc(SDL_Surface * src, int x, int y, SDL_Surface * dst);
	
	std::vector<View *> children;
	
	Rect pos;
	
	ui8 defActions;
	ui8 recActions;
	
	friend class CGuiHandler;
protected:
	void activate(ui16 what);
	void deactivate(ui16 what);
	
private:
	void onTimer(int timePassed);
	
	//TODO make parent const
	View * parent = nullptr;
private:
	
	ui16 used;
	
	int toNextTick;
	int timerDelay;
	
	std::map<EIntObjMouseBtnType, bool> currentMouseState;
	
	
	ui16 active_m;
};

/// Class for binding keys to left mouse button clicks
/// Classes wanting use it should have it as one of their base classes
class CKeyShortcut : public virtual View
{
public:
	std::set<int> assignedKeys;
	CKeyShortcut();
	CKeyShortcut(int key);
	CKeyShortcut(std::set<int> Keys);
	virtual void keyPressed(const SDL_KeyboardEvent & key) override; //call-in
};

class WindowBase : public View
{
public:
	WindowBase(int used_ = 0, Point pos_ = Point());
protected:
    void close();
};
