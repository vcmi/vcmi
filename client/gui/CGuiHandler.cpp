/*
 * CGuiHandler.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "CGuiHandler.h"
#include "../lib/CondSh.h"

#include <SDL.h>

#include "view/View.h"
#include "CCursorHandler.h"

#include "../CGameInfo.h"
#include "../../lib/CThreadHelper.h"
#include "../../lib/CConfigHandler.h"
#include "../CMT.h"
#include "../CPlayerInterface.h"
#include "../battle/CBattleInterface.h"
#include "widgets/AdventureMapClasses.h"

extern std::queue<SDL_Event> events;
extern boost::mutex eventsM;

boost::thread_specific_ptr<bool> inGuiThread;

SObjectConstruction::SObjectConstruction(View *obj)
:myObj(obj)
{
	GH.createdObj.push_front(obj);
	GH.captureChildren = true;
}

SObjectConstruction::~SObjectConstruction()
{
	assert(GH.createdObj.size());
	assert(GH.createdObj.front() == myObj);
	GH.createdObj.pop_front();
	GH.captureChildren = GH.createdObj.size();
}

SSetCaptureState::SSetCaptureState(bool allow, ui8 actions)
{
	previousCapture = GH.captureChildren;
	GH.captureChildren = false;
	prevActions = GH.defActionsDef;
	GH.defActionsDef = actions;
}

SSetCaptureState::~SSetCaptureState()
{
	GH.captureChildren = previousCapture;
	GH.defActionsDef = prevActions;
}

static inline void
processList(const ui16 mask, const ui16 flag, std::list<View*> *lst, std::function<void (std::list<View*> *)> cb)
{
	if (mask & flag)
		cb(lst);
}

void CGuiHandler::processLists(const ui16 activityFlag, std::function<void (std::list<View*> *)> cb)
{
	processList(View::LCLICK,activityFlag,&clickable,cb);
	processList(View::RCLICK,activityFlag,&clickable,cb);
	processList(View::MCLICK,activityFlag,&clickable,cb);
	
	processList(View::HOVER,activityFlag,&hoverable,cb);
	processList(View::MOVE,activityFlag,&motioninterested,cb);
	processList(View::KEYBOARD,activityFlag,&keyinterested,cb);
	processList(View::TIME,activityFlag,&timeinterested,cb);
	processList(View::WHEEL,activityFlag,&wheelInterested,cb);
	processList(View::DOUBLECLICK,activityFlag,&doubleClickInterested,cb);
	processList(View::TEXTINPUT,activityFlag,&textInterested,cb);
}

void CGuiHandler::handleElementActivate(View * elem, ui16 activityFlag)
{
	processLists(activityFlag,[&](std::list<View*> * lst){
		if(vstd::contains(*lst, elem) == false)
			lst->push_front(elem);
	});
	elem->active |= activityFlag;
}

void CGuiHandler::handleElementDeActivate(View * elem, ui16 activityFlag)
{
	processLists(activityFlag,[&](std::list<View*> * lst){
		auto hlp = std::find(lst->begin(),lst->end(),elem);
		if(hlp != lst->end())
		lst->erase(hlp);
	});
	elem->active &= ~activityFlag;
}

void CGuiHandler::popInt(std::shared_ptr<IShowActivatable> top)
{
	assert(listInt.front() == top);
	top->deactivate();
	disposed.push_back(top);
	listInt.pop_front();
	objsToBlit -= top;
	if(!listInt.empty())
		listInt.front()->activate();
	totalRedraw();

	pushSDLEvent(SDL_USEREVENT, EUserEvent::INTERFACE_CHANGED);
}

void CGuiHandler::pushInt(std::shared_ptr<IShowActivatable> newInt)
{
	assert(newInt);
	assert(!vstd::contains(listInt, newInt)); // do not add same object twice

	//a new interface will be present, we'll need to use buffer surface (unless it's advmapint that will alter screenBuf on activate anyway)
	screenBuf = screen2;

	if(!listInt.empty())
		listInt.front()->deactivate();
	listInt.push_front(newInt);
    CCS->curh->changeGraphic(ECursor::ADVENTURE, 0);
	newInt->activate();
	objsToBlit.push_back(newInt);
	totalRedraw();

	pushSDLEvent(SDL_USEREVENT, EUserEvent::INTERFACE_CHANGED);
}

void CGuiHandler::popInts(int howMany)
{
	if(!howMany) return; //senseless but who knows...

	assert(listInt.size() >= howMany);
	listInt.front()->deactivate();
	for(int i=0; i < howMany; i++)
	{
		objsToBlit -= listInt.front();
		disposed.push_back(listInt.front());
		listInt.pop_front();
	}

	if(!listInt.empty())
	{
		listInt.front()->activate();
		totalRedraw();
	}
	fakeMouseMove();

	pushSDLEvent(SDL_USEREVENT, EUserEvent::INTERFACE_CHANGED);
}

std::shared_ptr<IShowActivatable> CGuiHandler::topInt()
{
	if(listInt.empty())
		return std::shared_ptr<IShowActivatable>();
	else
		return listInt.front();
}

void CGuiHandler::totalRedraw()
{
	for(auto & elem : objsToBlit)
		elem->showAll(screen2);
	blitAt(screen2,0,0,screen);
}

void CGuiHandler::updateTime()
{
	int ms = mainFPSmng->getElapsedMilliseconds();
	std::list<View*> hlp = timeinterested;
	for (auto & elem : hlp)
	{
		if(!vstd::contains(timeinterested,elem)) continue;
		auto cast = dynamic_cast<CInfoBar *>(elem);
		if(cast)
			cast->onTimer(ms);
	}
}

void CGuiHandler::handleEvents()
{
	//player interface may want special event handling
	if(nullptr != LOCPLINT && LOCPLINT->capturedAllEvents())
		return;

	boost::unique_lock<boost::mutex> lock(eventsM);
	while(!events.empty())
	{
		continueEventHandling = true;
		event(events.front());
		events.pop();
	}
}

void CGuiHandler::event(const SDL_Event & event)
{
	if(event.type == SDL_KEYDOWN || event.type == SDL_KEYUP)
	{
		SDL_KeyboardEvent key = event.key;
		if(event.type == SDL_KEYDOWN && key.keysym.sym >= SDLK_F1 && key.keysym.sym <= SDLK_F15 && settings["session"]["spectate"].Bool())
		{
			//TODO: we need some central place for all interface-independent hotkeys
			Settings s = settings.write["session"];
			switch(key.keysym.sym)
			{
			case SDLK_F5:
				if(settings["session"]["spectate-locked-pim"].Bool())
					LOCPLINT->pim->unlock();
				else
					LOCPLINT->pim->lock();
				s["spectate-locked-pim"].Bool() = !settings["session"]["spectate-locked-pim"].Bool();
				break;

			case SDLK_F6:
				s["spectate-ignore-hero"].Bool() = !settings["session"]["spectate-ignore-hero"].Bool();
				break;

			case SDLK_F7:
				s["spectate-skip-battle"].Bool() = !settings["session"]["spectate-skip-battle"].Bool();
				break;

			case SDLK_F8:
				s["spectate-skip-battle-result"].Bool() = !settings["session"]["spectate-skip-battle-result"].Bool();
				break;

			case SDLK_F9:
				//not working yet since CClient::run remain locked after CBattleInterface removal
//				if(LOCPLINT->battleInt)
//				{
//					GH.popInts(1);
//					vstd::clear_pointer(LOCPLINT->battleInt);
//				}
				break;

			default:
				break;
			}
			return;
		}

		//translate numpad keys
		if(key.keysym.sym == SDLK_KP_ENTER)
		{
			key.keysym.sym = SDLK_RETURN;
			key.keysym.scancode = SDL_SCANCODE_RETURN;
		}

		bool keysCaptured = false;
		for(auto i = keyinterested.begin(); i != keyinterested.end() && continueEventHandling; i++)
		{
			if((*i)->captureThisEvent(key))
			{
				keysCaptured = true;
				break;
			}
		}

		std::list<View*> miCopy = keyinterested;
		for(auto i = miCopy.begin(); i != miCopy.end() && continueEventHandling; i++)
			if(vstd::contains(keyinterested,*i) && (!keysCaptured || (*i)->captureThisEvent(key)))
				(**i).keyPressed(event, key);
	}
	else if(event.type == SDL_MOUSEMOTION)
	{
		handleMouseMotion(event);
	}
	else if(event.type == SDL_MOUSEBUTTONDOWN)
	{
		switch(event.button.button)
		{
		case SDL_BUTTON_LEFT:
			if(lastClick == event.motion && (SDL_GetTicks() - lastClickTime) < 300)
			{
				std::list<View*> hlp = doubleClickInterested;
				for(auto i = hlp.begin(); i != hlp.end() && continueEventHandling; i++)
				{
					if(!vstd::contains(doubleClickInterested, *i)) continue;
					if(isItIn(&(*i)->pos, event.motion.x, event.motion.y))
					{
						(*i)->onDoubleClick(event);
					}
				}

			}

			lastClick = event.motion;
			lastClickTime = SDL_GetTicks();
			break;
		}
		handleMouseButtonClick(event, true);
	}
	else if(event.type == SDL_MOUSEWHEEL)
	{
		std::list<View*> hlp = wheelInterested;
		for(auto i = hlp.begin(); i != hlp.end() && continueEventHandling; i++)
		{
			if(!vstd::contains(wheelInterested,*i)) continue;
			// SDL doesn't have the proper values for mouse positions on SDL_MOUSEWHEEL, refetch them
			int x = 0, y = 0;
			SDL_GetMouseState(&x, &y);
			(*i)->wheelScrolled(event.wheel.y < 0, isItIn(&(*i)->pos, x, y));
		}
	}
	else if(event.type == SDL_TEXTINPUT)
	{
		for(auto it : textInterested)
		{
			auto cast = dynamic_cast<TextView *>(it);
			if(cast)
				cast->textInputed(event.text);
		}
	}
	else if(event.type == SDL_TEXTEDITING)
	{
		for(auto it : textInterested)
		{
			auto cast = dynamic_cast<TextView *>(it);
			if(cast)
				cast->textEdited(event.edit);
		}
	}
	else if(event.type == SDL_MOUSEBUTTONUP)
				handleMouseButtonClick(event, false);
}

void CGuiHandler::handleMouseButtonClick(const SDL_Event & event, bool isPressed)
{
	auto hlp = clickable;
	for(auto i = hlp.begin(); i != hlp.end() && continueEventHandling; i++)
	{
		if(!vstd::contains(clickable, *i)) continue;
		if(isItIn(&(*i)->pos, event.motion.x, event.motion.y) || !isPressed)
			(*i)->event(event);
	}
}

void CGuiHandler::handleMouseMotion(const SDL_Event & event)
{
	//sending active, hovered hoverable objects hover() call
	std::vector<View*> hlp;
	for(auto & elem : hoverable)
	{
		if(isItIn(&(elem)->pos, event.motion.x, event.motion.y))
		{
			if (!(elem)->hovered)
				hlp.push_back((elem));
		}
		else if ((elem)->hovered)
		{
			(elem)->hover(false);
			(elem)->hovered = false;
		}
	}
	for(auto & elem : hlp)
	{
		elem->hover(true);
		elem->hovered = true;
	}

	handleMoveInterested(event, event.motion);
}

void CGuiHandler::simpleRedraw()
{
	//update only top interface and draw background
	if(objsToBlit.size() > 1)
		blitAt(screen2,0,0,screen); //blit background
	if(!objsToBlit.empty())
		objsToBlit.back()->show(screen); //blit active interface/window
}

void CGuiHandler::handleMoveInterested(const SDL_Event & event, const SDL_MouseMotionEvent & motion)
{
	//sending active, MotionInterested objects mouseMoved() call
	std::list<View*> miCopy = motioninterested;
	for(auto & elem : miCopy)
	{
		if(elem->strongInterest || isItInOrLowerBounds(&elem->pos, motion.x, motion.y)) //checking lower bounds fixes bug #2476
		{
			(elem)->mouseMoved(event, motion);
		}
	}
}

void CGuiHandler::fakeMouseMove()
{
	SDL_Event event;
	SDL_MouseMotionEvent sme = {SDL_MOUSEMOTION, 0, 0, 0, 0, 0, 0, 0, 0};
	int x, y;

	sme.state = SDL_GetMouseState(&x, &y);
	sme.x = x;
	sme.y = y;

	event.motion = sme;
	SDL_PushEvent(&event);
}

void CGuiHandler::renderFrame()
{

	// Updating GUI requires locking pim mutex (that protects screen and GUI state).
	// During game:
	// When ending the game, the pim mutex might be hold by other thread,
	// that will notify us about the ending game by setting terminate_cond flag.
	//in PreGame terminate_cond stay false

	bool acquiredTheLockOnPim = false; //for tracking whether pim mutex locking succeeded
	while(!terminate_cond->get() && !(acquiredTheLockOnPim = CPlayerInterface::pim->try_lock())) //try acquiring long until it succeeds or we are told to terminate
		boost::this_thread::sleep(boost::posix_time::milliseconds(15));

	if(acquiredTheLockOnPim)
	{
		// If we are here, pim mutex has been successfully locked - let's store it in a safe RAII lock.
		boost::unique_lock<boost::recursive_mutex> un(*CPlayerInterface::pim, boost::adopt_lock);

		if(nullptr != curInt)
			curInt->update();

		if(settings["general"]["showfps"].Bool())
			drawFPSCounter();

		SDL_UpdateTexture(screenTexture, nullptr, screen->pixels, screen->pitch);

		SDL_RenderCopy(mainRenderer, screenTexture, nullptr, nullptr);

		CCS->curh->render();

		SDL_RenderPresent(mainRenderer);

		disposed.clear();
	}

	mainFPSmng->framerateDelay(); // holds a constant FPS
}


CGuiHandler::CGuiHandler()
	: lastClick(-500, -500),lastClickTime(0), defActionsDef(0), captureChildren(false)
{
	continueEventHandling = true;
	curInt = nullptr;
	statusbar = nullptr;

	// Creates the FPS manager and sets the framerate to 48 which is doubled the value of the original Heroes 3 FPS rate
	mainFPSmng = new CFramerateManager(48);
	//do not init CFramerateManager here --AVS

	terminate_cond = new CondSh<bool>(false);
}

CGuiHandler::~CGuiHandler()
{
	delete mainFPSmng;
	delete terminate_cond;
}

void CGuiHandler::breakEventHandling()
{
	continueEventHandling = false;
}

void CGuiHandler::drawFPSCounter()
{
	const static SDL_Color yellow = {255, 255, 0, 0};
	static SDL_Rect overlay = { 0, 0, 64, 32};
	Uint32 black = SDL_MapRGB(screen->format, 10, 10, 10);
	SDL_FillRect(screen, &overlay, black);
	std::string fps = boost::lexical_cast<std::string>(mainFPSmng->fps);
	graphics->fonts[FONT_BIG]->renderTextLeft(screen, fps, yellow, Point(10, 10));
}

SDL_Keycode CGuiHandler::arrowToNum(SDL_Keycode key)
{
	switch(key)
	{
	case SDLK_DOWN:
		return SDLK_KP_2;
	case SDLK_UP:
		return SDLK_KP_8;
	case SDLK_LEFT:
		return SDLK_KP_4;
	case SDLK_RIGHT:
		return SDLK_KP_6;
	default:
		throw std::runtime_error("Wrong key!");
	}
}

SDL_Keycode CGuiHandler::numToDigit(SDL_Keycode key)
{

#define REMOVE_KP(keyName) case SDLK_KP_ ## keyName : return SDLK_ ## keyName;
	switch(key)
	{
		REMOVE_KP(0)
		REMOVE_KP(1)
		REMOVE_KP(2)
		REMOVE_KP(3)
		REMOVE_KP(4)
		REMOVE_KP(5)
		REMOVE_KP(6)
		REMOVE_KP(7)
		REMOVE_KP(8)
		REMOVE_KP(9)
		REMOVE_KP(PERIOD)
		REMOVE_KP(MINUS)
		REMOVE_KP(PLUS)
		REMOVE_KP(EQUALS)

	case SDLK_KP_MULTIPLY:
		return SDLK_ASTERISK;
	case SDLK_KP_DIVIDE:
		return SDLK_SLASH;
	case SDLK_KP_ENTER:
		return SDLK_RETURN;
	default:
		return SDLK_UNKNOWN;
	}
#undef REMOVE_KP
}

bool CGuiHandler::isNumKey(SDL_Keycode key, bool number)
{
	if(number)
		return key >= SDLK_KP_1 && key <= SDLK_KP_0;
	else
		return (key >= SDLK_KP_1 && key <= SDLK_KP_0) || key == SDLK_KP_MINUS || key == SDLK_KP_PLUS || key == SDLK_KP_EQUALS;
}

bool CGuiHandler::isArrowKey(SDL_Keycode key)
{
	return key == SDLK_UP || key == SDLK_DOWN || key == SDLK_LEFT || key == SDLK_RIGHT;
}

bool CGuiHandler::amIGuiThread()
{
	return inGuiThread.get() && *inGuiThread;
}

void CGuiHandler::pushSDLEvent(int type, int usercode)
{
	SDL_Event event;
	event.type = type;
	event.user.code = usercode;	// not necessarily used
	SDL_PushEvent(&event);
}

CFramerateManager::CFramerateManager(int rate)
{
	this->rate = rate;
	this->rateticks = (1000.0 / rate);
	this->fps = 0;
	this->accumulatedFrames = 0;
	this->accumulatedTime = 0;
	this->lastticks = 0;
	this->timeElapsed = 0;
}

void CFramerateManager::init()
{
	this->lastticks = SDL_GetTicks();
}

void CFramerateManager::framerateDelay()
{
	ui32 currentTicks = SDL_GetTicks();
	timeElapsed = currentTicks - lastticks;

	// FPS is higher than it should be, then wait some time
	if (timeElapsed < rateticks)
	{
		SDL_Delay(ceil(this->rateticks) - timeElapsed);
	}

	accumulatedTime += timeElapsed;
	accumulatedFrames++;

	if(accumulatedFrames >= 100)
	{
		//about 2 second should be passed
		fps = ceil(1000.0 / (accumulatedTime/accumulatedFrames));
		accumulatedTime = 0;
		accumulatedFrames = 0;
	}

	currentTicks = SDL_GetTicks();
	// recalculate timeElapsed for external calls via getElapsed()
	// limit it to 1000 ms to avoid breaking animation in case of huge lag (e.g. triggered breakpoint)
	timeElapsed = std::min<ui32>(currentTicks - lastticks, 1000);

	lastticks = SDL_GetTicks();
}
