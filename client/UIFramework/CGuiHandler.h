#pragma once

#include "../../lib/CStopWatch.h"
#include "Geometries.h"

class CFramerateManager;
class IStatusBar;
class CIntObject;
class IUpdateable;
class IShowActivatable;
class IShowable;

/*
 * CGuiHandler.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

// A fps manager which holds game updates at a constant rate
class CFramerateManager
{
private:
	double rateticks;
	ui32 lastticks, timeElapsed;
	int rate;

public:
	int fps; // the actual fps value

	CFramerateManager(int rate); // initializes the manager with a given fps rate
	void init(); // needs to be called directly before the main game loop to reset the internal timer
	void framerateDelay(); // needs to be called every game update cycle
	double getElapsedSeconds() const { return this->timeElapsed / 1000; }
};

// Handles GUI logic and drawing
class CGuiHandler
{
public:
	CFramerateManager * mainFPSmng; //to keep const framerate
	CStopWatch th;
	std::list<IShowActivatable *> listInt; //list of interfaces - front=foreground; back = background (includes adventure map, window interfaces, all kind of active dialogs, and so on)
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

	void popInt(IShowActivatable *top); //removes given interface from the top and activates next
	void popIntTotally(IShowActivatable *top); //deactivates, deletes, removes given interface from the top and activates next
	void pushInt(IShowActivatable *newInt); //deactivate old top interface, activates this one and pushes to the top
	void popInts(int howMany); //pops one or more interfaces - deactivates top, deletes and removes given number of interfaces, activates new front
	IShowActivatable *topInt(); //returns top interface

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
	
	static CIntObject * moveChild(CIntObject *obj, CIntObject *from, CIntObject *to, bool adjustPos = false);
	static SDLKey arrowToNum(SDLKey key); //converts arrow key to according numpad key
	static SDLKey numToDigit(SDLKey key);//converts numpad digit key to normal digit key
	static bool isNumKey(SDLKey key, bool number = true); //checks if key is on numpad (numbers - check only for numpad digits)
	static bool isArrowKey(SDLKey key); 
};

extern CGuiHandler GH; //global gui handler

template <typename T> void pushIntT()
{
	GH.pushInt(new T());
}

struct SObjectConstruction
{
	CIntObject *myObj;
	SObjectConstruction(CIntObject *obj);
	~SObjectConstruction();
};

struct SSetCaptureState
{
	bool previousCapture;
	ui8 prevActions;
	SSetCaptureState(bool allow, ui8 actions);
	~SSetCaptureState();
};

namespace Colors
{
	
};

#define OBJ_CONSTRUCTION SObjectConstruction obj__i(this)
#define OBJ_CONSTRUCTION_CAPTURING_ALL defActions = 255; SSetCaptureState obj__i1(true, 255); SObjectConstruction obj__i(this)
#define BLOCK_CAPTURING SSetCaptureState obj__i(false, 0)
#define BLOCK_CAPTURING_DONT_TOUCH_REC_ACTIONS SSetCaptureState obj__i(false, GH.defActionsDef)
