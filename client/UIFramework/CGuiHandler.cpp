#include "StdInc.h"
#include "CGuiHandler.h"

#include "SDL_Extensions.h"
#include "CIntObject.h"
#include "../CGameInfo.h"
#include "CCursorHandler.h"
#include "../../lib/CThreadHelper.h"
#include "../CConfigHandler.h"

extern SDL_Surface * screenBuf, * screen2, * screen;
extern std::queue<SDL_Event*> events;
extern boost::mutex eventsM;

boost::thread_specific_ptr<bool> inGuiThread;

SObjectConstruction::SObjectConstruction( CIntObject *obj )
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

void CGuiHandler::popInt( IShowActivatable *top )
{
	assert(listInt.front() == top);
	top->deactivate();
	listInt.pop_front();
	objsToBlit -= top;
	if(listInt.size())
		listInt.front()->activate();
	totalRedraw();
}

void CGuiHandler::popIntTotally( IShowActivatable *top )
{
	assert(listInt.front() == top);
	popInt(top);
	delete top;
	fakeMouseMove();
}

void CGuiHandler::pushInt( IShowActivatable *newInt )
{
	//a new interface will be present, we'll need to use buffer surface (unless it's advmapint that will alter screenBuf on activate anyway)
	screenBuf = screen2; 

	if(listInt.size())
		listInt.front()->deactivate();
	listInt.push_front(newInt);
	newInt->activate();
	objsToBlit.push_back(newInt);
	totalRedraw();
}

void CGuiHandler::popInts( int howMany )
{
	if(!howMany) return; //senseless but who knows...

	assert(listInt.size() >= howMany);
	listInt.front()->deactivate();
	for(int i=0; i < howMany; i++)
	{
		objsToBlit -= listInt.front();
		delete listInt.front();
		listInt.pop_front();
	}

	if(listInt.size())
	{
		listInt.front()->activate();
		totalRedraw();
	}
	fakeMouseMove();
}

IShowActivatable * CGuiHandler::topInt()
{
	if(!listInt.size())
		return NULL;
	else 
		return listInt.front();
}

void CGuiHandler::totalRedraw()
{
	for(int i=0;i<objsToBlit.size();i++)
		objsToBlit[i]->showAll(screen2);

// 	static int a = 0;
// 	if(a)
// 	{
// 		SDL_SaveBMP(screen, "s1.bmp");
// 		SDL_SaveBMP(screen2, "s2.bmp");
// 	}
	blitAt(screen2,0,0,screen);
}

void CGuiHandler::updateTime()
{
	int ms = mainFPSmng->getElapsedMilliseconds();
	std::list<CIntObject*> hlp = timeinterested;
	for (std::list<CIntObject*>::iterator i=hlp.begin(); i != hlp.end();i++)
	{
		if(!vstd::contains(timeinterested,*i)) continue;
		(*i)->onTimer(ms);
	}
}

void CGuiHandler::handleEvents()
{
	while(true)
	{
		SDL_Event *ev = NULL;
		boost::unique_lock<boost::mutex> lock(eventsM);
		if(!events.size())
		{
			return;
		}
		else
		{
			ev = events.front();
			events.pop();
		}
		handleEvent(ev);
		delete ev;
	}
}

void CGuiHandler::handleEvent(SDL_Event *sEvent)
{
	current = sEvent;
	bool prev;

	if (sEvent->type==SDL_KEYDOWN || sEvent->type==SDL_KEYUP)
	{
		SDL_KeyboardEvent key = sEvent->key;

		//translate numpad keys
		if(key.keysym.sym == SDLK_KP_ENTER)
		{
			key.keysym.sym = (SDLKey)SDLK_RETURN;
		}

		bool keysCaptured = false;
		for(std::list<CIntObject*>::iterator i=keyinterested.begin(); i != keyinterested.end() && current; i++)
		{
			if((*i)->captureThisEvent(key))
			{
				keysCaptured = true;
				break;
			}
		}

		std::list<CIntObject*> miCopy = keyinterested;
		for(std::list<CIntObject*>::iterator i=miCopy.begin(); i != miCopy.end() && current; i++)
			if(vstd::contains(keyinterested,*i) && (!keysCaptured || (*i)->captureThisEvent(key)))
				(**i).keyPressed(key);
	}
	else if(sEvent->type==SDL_MOUSEMOTION)
	{
		CCS->curh->cursorMove(sEvent->motion.x, sEvent->motion.y);
		handleMouseMotion(sEvent);
	}
	else if (sEvent->type==SDL_MOUSEBUTTONDOWN)
	{
		if(sEvent->button.button == SDL_BUTTON_LEFT)
		{

			if(lastClick == sEvent->motion  &&  (SDL_GetTicks() - lastClickTime) < 300)
			{
				std::list<CIntObject*> hlp = doubleClickInterested;
				for(std::list<CIntObject*>::iterator i=hlp.begin(); i != hlp.end() && current; i++)
				{
					if(!vstd::contains(doubleClickInterested,*i)) continue;
					if (isItIn(&(*i)->pos,sEvent->motion.x,sEvent->motion.y))
					{
						(*i)->onDoubleClick();
					}
				}

			}

			lastClick = sEvent->motion;
			lastClickTime = SDL_GetTicks();

			std::list<CIntObject*> hlp = lclickable;
			for(std::list<CIntObject*>::iterator i=hlp.begin(); i != hlp.end() && current; i++)
			{
				if(!vstd::contains(lclickable,*i)) continue;
				if (isItIn(&(*i)->pos,sEvent->motion.x,sEvent->motion.y))
				{
					prev = (*i)->pressedL;
					(*i)->pressedL = true;
					(*i)->clickLeft(true, prev);
				}
			}
		}
		else if (sEvent->button.button == SDL_BUTTON_RIGHT)
		{
			std::list<CIntObject*> hlp = rclickable;
			for(std::list<CIntObject*>::iterator i=hlp.begin(); i != hlp.end() && current; i++)
			{
				if(!vstd::contains(rclickable,*i)) continue;
				if (isItIn(&(*i)->pos,sEvent->motion.x,sEvent->motion.y))
				{
					prev = (*i)->pressedR;
					(*i)->pressedR = true;
					(*i)->clickRight(true, prev);
				}
			}
		}
		else if(sEvent->button.button == SDL_BUTTON_WHEELDOWN || sEvent->button.button == SDL_BUTTON_WHEELUP)
		{
			std::list<CIntObject*> hlp = wheelInterested;
			for(std::list<CIntObject*>::iterator i=hlp.begin(); i != hlp.end() && current; i++)
			{
				if(!vstd::contains(wheelInterested,*i)) continue;
				(*i)->wheelScrolled(sEvent->button.button == SDL_BUTTON_WHEELDOWN, isItIn(&(*i)->pos,sEvent->motion.x,sEvent->motion.y));
			}
		}
	}
	else if ((sEvent->type==SDL_MOUSEBUTTONUP) && (sEvent->button.button == SDL_BUTTON_LEFT))
	{
		std::list<CIntObject*> hlp = lclickable;
		for(std::list<CIntObject*>::iterator i=hlp.begin(); i != hlp.end() && current; i++)
		{
			if(!vstd::contains(lclickable,*i)) continue;
			prev = (*i)->pressedL;
			(*i)->pressedL = false;
			if (isItIn(&(*i)->pos,sEvent->motion.x,sEvent->motion.y))
			{
				(*i)->clickLeft(false, prev);
			}
			else
				(*i)->clickLeft(boost::logic::indeterminate, prev);
		}
	}
	else if ((sEvent->type==SDL_MOUSEBUTTONUP) && (sEvent->button.button == SDL_BUTTON_RIGHT))
	{
		std::list<CIntObject*> hlp = rclickable;
		for(std::list<CIntObject*>::iterator i=hlp.begin(); i != hlp.end() && current; i++)
		{
			if(!vstd::contains(rclickable,*i)) continue;
			prev = (*i)->pressedR;
			(*i)->pressedR = false;
			if (isItIn(&(*i)->pos,sEvent->motion.x,sEvent->motion.y))
			{
				(*i)->clickRight(false, prev);
			}
			else
				(*i)->clickRight(boost::logic::indeterminate, prev);
		}
	}
	current = NULL;

} //event end

void CGuiHandler::handleMouseMotion(SDL_Event *sEvent)
{
	//sending active, hovered hoverable objects hover() call
	std::vector<CIntObject*> hlp;
	for(std::list<CIntObject*>::iterator i=hoverable.begin(); i != hoverable.end();i++)
	{
		if (isItIn(&(*i)->pos,sEvent->motion.x,sEvent->motion.y))
		{
			if (!(*i)->hovered)
				hlp.push_back((*i));
		}
		else if ((*i)->hovered)
		{
			(*i)->hover(false);
			(*i)->hovered = false;
		}
	}
	for(int i=0; i<hlp.size();i++)
	{
		hlp[i]->hover(true);
		hlp[i]->hovered = true;
	}

	handleMoveInterested(sEvent->motion);
}

void CGuiHandler::simpleRedraw()
{
	//update only top interface and draw background
	if(objsToBlit.size() > 1)
		blitAt(screen2,0,0,screen); //blit background
	objsToBlit.back()->show(screen); //blit active interface/window
}

void CGuiHandler::handleMoveInterested( const SDL_MouseMotionEvent & motion )
{	
	//sending active, MotionInterested objects mouseMoved() call
	std::list<CIntObject*> miCopy = motioninterested;
	for(std::list<CIntObject*>::iterator i=miCopy.begin(); i != miCopy.end();i++)
	{
		if ((*i)->strongInterest || isItIn(&(*i)->pos, motion.x, motion.y))
		{
			(*i)->mouseMoved(motion);
		}
	}
}

void CGuiHandler::fakeMouseMove()
{
	SDL_Event evnt;

	SDL_MouseMotionEvent sme = {SDL_MOUSEMOTION, 0, 0, 0, 0, 0, 0};
	int x, y;
	sme.state = SDL_GetMouseState(&x, &y);
	sme.x = x;
	sme.y = y;

	evnt.motion = sme;
	current = &evnt;
	handleMouseMotion(&evnt);
}

void CGuiHandler::run()
{
	setThreadName(-1, "CGuiHandler::run");
	inGuiThread.reset(new bool(true));
	try
	{
		if (settings["video"]["fullscreen"].Bool())
			CCS->curh->centerCursor();

		mainFPSmng->init(); // resets internal clock, needed for FPS manager
		while(!terminate)
		{
			if(curInt)
				curInt->update(); // calls a update and drawing process of the loaded game interface object at the moment

			mainFPSmng->framerateDelay(); // holds a constant FPS
		}
	} HANDLE_EXCEPTION
}

CGuiHandler::CGuiHandler()
:lastClick(-500, -500)
{
	curInt = NULL;
	current = NULL;
	terminate = false;
	statusbar = NULL;

	// Creates the FPS manager and sets the framerate to 48 which is doubled the value of the original Heroes 3 FPS rate
	mainFPSmng = new CFramerateManager(48);
	mainFPSmng->init(); // resets internal clock, needed for FPS manager
}

CGuiHandler::~CGuiHandler()
{
	delete mainFPSmng;
}

void CGuiHandler::breakEventHandling()
{
	current = NULL;
}

void CGuiHandler::drawFPSCounter()
{
	const static SDL_Color yellow = {255, 255, 0, 0};
	static SDL_Rect overlay = { 0, 0, 64, 32};
	Uint32 black = SDL_MapRGB(screen->format, 10, 10, 10);
	SDL_FillRect(screen, &overlay, black);
	std::string fps = boost::lexical_cast<std::string>(mainFPSmng->fps);
	CSDL_Ext::printAt(fps, 10, 10, FONT_BIG, yellow, screen);
}

SDLKey CGuiHandler::arrowToNum( SDLKey key )
{
	switch(key)
	{
	case SDLK_DOWN:
		return SDLK_KP2;
	case SDLK_UP:
		return SDLK_KP8;
	case SDLK_LEFT:
		return SDLK_KP4;
	case SDLK_RIGHT:
		return SDLK_KP6;
	default:
		assert(0);
	}
	throw std::runtime_error("Wrong key!");
}

SDLKey CGuiHandler::numToDigit( SDLKey key )
{
	if(key >= SDLK_KP0 && key <= SDLK_KP9)
		return SDLKey(key - SDLK_KP0 + SDLK_0);

#define REMOVE_KP(keyName) case SDLK_KP_ ## keyName : return SDLK_ ## keyName;
	switch(key)
	{
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
		tlog3 << "Illegal numkey conversion!" << std::endl;
		return SDLK_UNKNOWN;
	}
#undef REMOVE_KP
}

bool CGuiHandler::isNumKey( SDLKey key, bool number )
{
	if(number)
		return key >= SDLK_KP0 && key <= SDLK_KP9;
	else
		return key >= SDLK_KP0 && key <= SDLK_KP_EQUALS;
}

bool CGuiHandler::isArrowKey( SDLKey key )
{
	return key >= SDLK_UP && key <= SDLK_LEFT;
}

bool CGuiHandler::amIGuiThread()
{
	return inGuiThread.get() && *inGuiThread;
}

CFramerateManager::CFramerateManager(int rate)
{
	this->rate = rate;
	this->rateticks = (1000.0 / rate);
	this->fps = 0;
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
	currentTicks = SDL_GetTicks();

	fps = ceil(1000.0 / timeElapsed);

	//recalculate timeElapsed for external calls via getElapsed()
	timeElapsed = currentTicks - lastticks;
	lastticks = SDL_GetTicks();
}