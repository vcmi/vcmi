/*
 * CGuiHandler.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

//#include "../../lib/CStopWatch.h"
#include "Geometries.h"
#include "SDL_Extensions.h"

VCMI_LIB_NAMESPACE_BEGIN

template <typename T> struct CondSh;

VCMI_LIB_NAMESPACE_END

class CFramerateManager;
class IStatusBar;
class CIntObject;
class IUpdateable;
class IShowActivatable;
class IShowable;
enum class EIntObjMouseBtnType;

// TODO: event handling need refactoring
enum EUserEvent
{
	/*CHANGE_SCREEN_RESOLUTION = 1,*/
	RETURN_TO_MAIN_MENU = 2,
	//STOP_CLIENT = 3,
	RESTART_GAME = 4,
	RETURN_TO_MENU_LOAD,
	FULLSCREEN_TOGGLED,
	CAMPAIGN_START_SCENARIO,
	FORCE_QUIT, //quit client without question
	INTERFACE_CHANGED
};

// A fps manager which holds game updates at a constant rate
class CFramerateManager
{
private:
	double rateticks;
	ui32 lastticks, timeElapsed;
	int rate;
	ui32 accumulatedTime,accumulatedFrames;
public:
	int fps; // the actual fps value

	CFramerateManager(int rate); // initializes the manager with a given fps rate
	void init(); // needs to be called directly before the main game loop to reset the internal timer
	void framerateDelay(); // needs to be called every game update cycle
	ui32 getElapsedMilliseconds() const {return this->timeElapsed;}
	ui32 getFrameNumber() const { return accumulatedFrames; }
};

// Handles GUI logic and drawing
class CGuiHandler
{
public:
	CFramerateManager * mainFPSmng; //to keep const framerate
	std::list<std::shared_ptr<IShowActivatable>> listInt; //list of interfaces - front=foreground; back = background (includes adventure map, window interfaces, all kind of active dialogs, and so on)
	std::shared_ptr<IStatusBar> statusbar;

private:
	std::vector<std::shared_ptr<IShowActivatable>> disposed;

	std::atomic<bool> continueEventHandling;
	typedef std::list<CIntObject*> CIntObjectList;

	//active GUI elements (listening for events
	CIntObjectList lclickable,
				   rclickable,
				   mclickable,
				   hoverable,
				   keyinterested,
				   motioninterested,
	               timeinterested,
	               wheelInterested,
	               doubleClickInterested,
	               textInterested;


	void handleMouseButtonClick(CIntObjectList & interestedObjs, EIntObjMouseBtnType btn, bool isPressed);
	void processLists(const ui16 activityFlag, std::function<void (std::list<CIntObject*> *)> cb);
public:
	void handleElementActivate(CIntObject * elem, ui16 activityFlag);
	void handleElementDeActivate(CIntObject * elem, ui16 activityFlag);

public:
	//objs to blit
	std::vector<std::shared_ptr<IShowActivatable>> objsToBlit;

	SDL_Event * current; //current event - can be set to nullptr to stop handling event
	IUpdateable *curInt;

	Point lastClick;
	unsigned lastClickTime;

	ui8 defActionsDef; //default auto actions
	bool captureChildren; //all newly created objects will get their parents from stack and will be added to parents children list
	std::list<CIntObject *> createdObj; //stack of objs being created

	CGuiHandler();
	~CGuiHandler();

	void renderFrame();

	void totalRedraw(); //forces total redraw (using showAll), sets a flag, method gets called at the end of the rendering
	void simpleRedraw(); //update only top interface and draw background from buffer, sets a flag, method gets called at the end of the rendering

	void pushInt(std::shared_ptr<IShowActivatable> newInt); //deactivate old top interface, activates this one and pushes to the top
	template <typename T, typename ... Args>
	void pushIntT(Args && ... args)
	{
		auto newInt = std::make_shared<T>(std::forward<Args>(args)...);
		pushInt(newInt);
	}

	void popInts(int howMany); //pops one or more interfaces - deactivates top, deletes and removes given number of interfaces, activates new front

	void popInt(std::shared_ptr<IShowActivatable> top); //removes given interface from the top and activates next

	std::shared_ptr<IShowActivatable> topInt(); //returns top interface

	void updateTime(); //handles timeInterested
	void handleEvents(); //takes events from queue and calls interested objects
	void handleCurrentEvent();
	void handleMouseMotion();
	void handleMoveInterested( const SDL_MouseMotionEvent & motion );
	void fakeMouseMove();
	void breakEventHandling(); //current event won't be propagated anymore
	void drawFPSCounter(); // draws the FPS to the upper left corner of the screen

	static SDL_Keycode arrowToNum(SDL_Keycode key); //converts arrow key to according numpad key
	static SDL_Keycode numToDigit(SDL_Keycode key);//converts numpad digit key to normal digit key
	static bool isNumKey(SDL_Keycode key, bool number = true); //checks if key is on numpad (numbers - check only for numpad digits)
	static bool isArrowKey(SDL_Keycode key);
	static bool amIGuiThread();
	static void pushSDLEvent(int type, int usercode = 0);

	CondSh<bool> * terminate_cond; // confirm termination
};

extern CGuiHandler GH; //global gui handler

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

#define OBJ_CONSTRUCTION SObjectConstruction obj__i(this)
#define OBJ_CONSTRUCTION_TARGETED(obj) SObjectConstruction obj__i(obj)
#define OBJECT_CONSTRUCTION_CAPTURING(actions) defActions = actions; SSetCaptureState obj__i1(true, actions); SObjectConstruction obj__i(this)
#define OBJECT_CONSTRUCTION_CUSTOM_CAPTURING(actions) SSetCaptureState obj__i1(true, actions); SObjectConstruction obj__i(this)

#define OBJ_CONSTRUCTION_CAPTURING_ALL_NO_DISPOSE defActions = 255 - DISPOSE; SSetCaptureState obj__i1(true, 255 - DISPOSE); SObjectConstruction obj__i(this)
