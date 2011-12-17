#pragma once

#include <SDL_events.h>
#include "IShowActivatable.h"
#include "SRect.h"
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

// Base UI element
class CIntObject : public IShowActivatable //interface object
{
public:
	CIntObject *parent; //parent object
	std::vector<CIntObject *> children;

	SRect pos, //position of object on the screen
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
	void redraw();

	void drawBorderLoc(SDL_Surface * sur, const SRect &r, const int3 &color);
	void printAtLoc(const std::string & text, int x, int y, EFonts font, SDL_Color kolor, SDL_Surface * dst);
	void printToLoc(const std::string & text, int x, int y, EFonts font, SDL_Color kolor, SDL_Surface * dst);
	void printAtMiddleLoc(const std::string & text, int x, int y, EFonts font, SDL_Color kolor, SDL_Surface * dst);
	void printAtMiddleLoc(const std::string & text, const SPoint &p, EFonts font, SDL_Color kolor, SDL_Surface * dst);
	void printAtMiddleWBLoc(const std::string & text, int x, int y, EFonts font, int charpr, SDL_Color kolor, SDL_Surface * dst);
	void blitAtLoc(SDL_Surface * src, int x, int y, SDL_Surface * dst);
	void blitAtLoc(SDL_Surface * src, const SPoint &p, SDL_Surface * dst);
	bool isItInLoc(const SDL_Rect &rect, int x, int y);
	bool isItInLoc(const SDL_Rect &rect, const SPoint &p);
	const SRect & center(const SRect &r, bool propagate = true); //sets pos so that r will be in the center of screen, assigns sizes of r to pos, returns new position
	const SRect & center(const SPoint &p, bool propagate = true);  //moves object so that point p will be in its center
	const SRect & center(bool propagate = true); //centers when pos.w and pos.h are set, returns new position
	void fitToScreen(int borderWidth, bool propagate = true); //moves window to fit into screen
	void moveBy(const SPoint &p, bool propagate = true);
	void moveTo(const SPoint &p, bool propagate = true);
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