#include "SDL_Extensions.h"
#include <cassert>
#include <boost/thread/locks.hpp>
#include "GUIBase.h"
#include <boost/thread/mutex.hpp>
#include <queue>
#include "CGameInfo.h"
#include "CCursorHandler.h"
#include "CBitmapHandler.h"
/*
 * GUIBase.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

extern std::queue<SDL_Event*> events;
extern boost::mutex eventsM;

void KeyShortcut::keyPressed(const SDL_KeyboardEvent & key)
{
	if(vstd::contains(assignedKeys,key.keysym.sym))
	{
		bool prev = pressedL;
		if(key.state == SDL_PRESSED) 
		{
			pressedL = true;
			clickLeft(true, prev);
		} else 
		{
			pressedL = false;
			clickLeft(false, prev);
		}
	}
}


void CGuiHandler::popInt( IShowActivable *top )
{
	assert(listInt.front() == top);
	top->deactivate();
	listInt.pop_front();
	objsToBlit -= top;
	if(listInt.size())
		listInt.front()->activate();
	totalRedraw();
}

void CGuiHandler::popIntTotally( IShowActivable *top )
{
	assert(listInt.front() == top);
	popInt(top);
	delete top;
}

void CGuiHandler::pushInt( IShowActivable *newInt )
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

	assert(listInt.size() > howMany);
	listInt.front()->deactivate();
	for(int i=0; i < howMany; i++)
	{
		objsToBlit -= listInt.front();
		delete listInt.front();
		listInt.pop_front();
	}
	listInt.front()->activate();
	totalRedraw();
}

IShowActivable * CGuiHandler::topInt()
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

	blitAt(screen2,0,0,screen);

	if(objsToBlit.size())
		objsToBlit.back()->showAll(screen);
}

void CGuiHandler::updateTime()
{
	int tv = th.getDif();
	std::list<CIntObject*> hlp = timeinterested;
	for (std::list<CIntObject*>::iterator i=hlp.begin(); i != hlp.end();i++)
	{
		if(!vstd::contains(timeinterested,*i)) continue;
		if ((*i)->toNextTick>=0)
			(*i)->toNextTick-=tv;
		if ((*i)->toNextTick<0)
			(*i)->tick();
	}
}

void CGuiHandler::handleEvents()
{
	while(true)
	{
		SDL_Event *ev = NULL;
		{
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
		for(std::list<CIntObject*>::iterator i=keyinterested.begin(); i != keyinterested.end();i++)
		{
			if((*i)->captureAllKeys)
			{
				keysCaptured = true;
				break;
			}
		}

		std::list<CIntObject*> miCopy = keyinterested;
		for(std::list<CIntObject*>::iterator i=miCopy.begin(); i != miCopy.end();i++)
			if(vstd::contains(keyinterested,*i) && (!keysCaptured || (*i)->captureAllKeys))
				(**i).keyPressed(key);
	}
	else if(sEvent->type==SDL_MOUSEMOTION)
	{
		CGI->curh->cursorMove(sEvent->motion.x, sEvent->motion.y);
		handleMouseMotion(sEvent);
	}
	else if (sEvent->type==SDL_MOUSEBUTTONDOWN)
	{
		if(sEvent->button.button == SDL_BUTTON_LEFT)
		{
			
			if(lastClick == sEvent->motion  &&  (SDL_GetTicks() - lastClickTime) < 300)
			{
				std::list<CIntObject*> hlp = doubleClickInterested;
				for(std::list<CIntObject*>::iterator i=hlp.begin(); i != hlp.end();i++)
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
			for(std::list<CIntObject*>::iterator i=hlp.begin(); i != hlp.end();i++)
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
			for(std::list<CIntObject*>::iterator i=hlp.begin(); i != hlp.end();i++)
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
			for(std::list<CIntObject*>::iterator i=hlp.begin(); i != hlp.end();i++)
			{
				if(!vstd::contains(wheelInterested,*i)) continue;
				(*i)->wheelScrolled(sEvent->button.button == SDL_BUTTON_WHEELDOWN, isItIn(&(*i)->pos,sEvent->motion.x,sEvent->motion.y));
			}
		}
	}
	else if ((sEvent->type==SDL_MOUSEBUTTONUP) && (sEvent->button.button == SDL_BUTTON_LEFT))
	{
		std::list<CIntObject*> hlp = lclickable;
		for(std::list<CIntObject*>::iterator i=hlp.begin(); i != hlp.end();i++)
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
		for(std::list<CIntObject*>::iterator i=hlp.begin(); i != hlp.end();i++)
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


	//sending active, MotionInterested objects mouseMoved() call
	std::list<CIntObject*> miCopy = motioninterested;
	for(std::list<CIntObject*>::iterator i=miCopy.begin(); i != miCopy.end();i++)
	{
		if ((*i)->strongInterest || isItIn(&(*i)->pos,sEvent->motion.x,sEvent->motion.y))
		{
			(*i)->mouseMoved(sEvent->motion);
		}
	}
}

void CGuiHandler::simpleRedraw()
{
	//update only top interface and draw background
	if(objsToBlit.size() > 1)
		blitAt(screen2,0,0,screen); //blit background
	objsToBlit.back()->show(screen); //blit active interface/window
}

void CIntObject::activateLClick()
{
	GH.lclickable.push_front(this);
	active |= LCLICK;
}

void CIntObject::deactivateLClick()
{
	std::list<CIntObject*>::iterator hlp = std::find(GH.lclickable.begin(),GH.lclickable.end(),this);
	assert(hlp != GH.lclickable.end());
	GH.lclickable.erase(hlp);
	active &= ~LCLICK;
}

void CIntObject::clickLeft(tribool down, bool previousState)
{
}

void CIntObject::activateRClick()
{
	GH.rclickable.push_front(this);
	active |= RCLICK;
}

void CIntObject::deactivateRClick()
{
	std::list<CIntObject*>::iterator hlp = std::find(GH.rclickable.begin(),GH.rclickable.end(),this);
	assert(hlp != GH.rclickable.end());
	GH.rclickable.erase(hlp);
	active &= ~RCLICK;
}

void CIntObject::clickRight(tribool down, bool previousState)
{
}

void CIntObject::activateHover()
{
	GH.hoverable.push_front(this);
	active |= HOVER;
}

void CIntObject::deactivateHover()
{
	std::list<CIntObject*>::iterator hlp = std::find(GH.hoverable.begin(),GH.hoverable.end(),this);
	assert(hlp != GH.hoverable.end());
	GH.hoverable.erase(hlp);
	active &= ~HOVER;
}

void CIntObject::hover( bool on )
{
}

void CIntObject::activateKeys()
{
	GH.keyinterested.push_front(this);
	active |= KEYBOARD;
}

void CIntObject::deactivateKeys()
{
	std::list<CIntObject*>::iterator hlp = std::find(GH.keyinterested.begin(),GH.keyinterested.end(),this);
	assert(hlp != GH.keyinterested.end());
	GH.keyinterested.erase(hlp);
	active &= ~KEYBOARD;
}

void CIntObject::keyPressed( const SDL_KeyboardEvent & key )
{
}

void CIntObject::activateMouseMove()
{
	GH.motioninterested.push_front(this);
	active |= MOVE;
}

void CIntObject::deactivateMouseMove()
{
	std::list<CIntObject*>::iterator hlp = std::find(GH.motioninterested.begin(),GH.motioninterested.end(),this);
	assert(hlp != GH.motioninterested.end());
	GH.motioninterested.erase(hlp);
	active &= ~MOVE;
}

void CIntObject::mouseMoved( const SDL_MouseMotionEvent & sEvent )
{
}

void CIntObject::activateTimer()
{
	GH.timeinterested.push_back(this);
	active |= TIME;
}

void CIntObject::deactivateTimer()
{
	std::list<CIntObject*>::iterator hlp = std::find(GH.timeinterested.begin(),GH.timeinterested.end(),this);
	assert(hlp != GH.timeinterested.end());
	GH.timeinterested.erase(hlp);
	active &= ~TIME;
}

void CIntObject::tick()
{
}

CIntObject::CIntObject()
{
	pressedL = pressedR = hovered = captureAllKeys = strongInterest = toNextTick = active = used = 0;

	recActions = defActions = GH.defActionsDef;

	pos.x = 0;
	pos.y = 0;
	pos.w = 0;
	pos.h = 0;

	if(GH.captureChildren)
	{
		assert(GH.createdObj.size());
		parent = GH.createdObj.front();
		parent->children.push_back(this);

		if(parent->defActions & SHARE_POS)
		{
			pos.x = parent->pos.x;
			pos.y = parent->pos.y;
		}
	}
}

void CIntObject::show( SDL_Surface * to )
{
	if(defActions & UPDATE)
		for(size_t i = 0; i < children.size(); i++)
			if(children[i]->recActions & UPDATE)
				children[i]->show(to);
}

void CIntObject::showAll( SDL_Surface * to )
{
	if(defActions & SHOWALL)
	{
		for(size_t i = 0; i < children.size(); i++)
			if(children[i]->recActions & SHOWALL)
				children[i]->showAll(to);

	}
	else
		show(to);
}

void CIntObject::activate()
{
	assert(!active);
	active |= GENERAL;
	if(used & LCLICK)
		activateLClick();
	if(used & RCLICK)
		activateRClick();
	if(used & HOVER)
		activateHover();
	if(used & MOVE)
		activateMouseMove();
	if(used & KEYBOARD)
		activateKeys();
	if(used & TIME)
		activateTimer();
	if(used & WHEEL)
		activateWheel();
	if(used & DOUBLECLICK)
		activateDClick();

	if(defActions & ACTIVATE)
		for(size_t i = 0; i < children.size(); i++)
			if(children[i]->recActions & ACTIVATE)
				children[i]->activate();
}

void CIntObject::deactivate()
{
	assert(active);
	active &= ~ GENERAL;
	if(used & LCLICK)
		deactivateLClick();
	if(used & RCLICK)
		deactivateRClick();
	if(used & HOVER)
		deactivateHover();
	if(used & MOVE)
		deactivateMouseMove();
	if(used & KEYBOARD)
		deactivateKeys();
	if(used & TIME)
		deactivateTimer();
	if(used & WHEEL)
		deactivateWheel();
	if(used & DOUBLECLICK)
		deactivateDClick();

	if(defActions & DEACTIVATE)
		for(size_t i = 0; i < children.size(); i++)
			if(children[i]->recActions & DEACTIVATE)
				children[i]->deactivate();
}

CIntObject::~CIntObject()
{
	assert(!active); //do not delete active obj

	if(defActions & DISPOSE)
		for(size_t i = 0; i < children.size(); i++)
			if(children[i]->recActions & DISPOSE)
				delete children[i];
}

void CIntObject::printAtLoc( const std::string & text, int x, int y, EFonts font, SDL_Color kolor/*=zwykly*/, SDL_Surface * dst/*=screen*/, bool refresh /*= false*/ )
{
	CSDL_Ext::printAt(text, pos.x + x, pos.y + y, font, kolor, dst, refresh);
}

void CIntObject::printAtMiddleLoc( const std::string & text, int x, int y, EFonts font, SDL_Color kolor/*=zwykly*/, SDL_Surface * dst/*=screen*/, bool refresh /*= false*/ )
{
	CSDL_Ext::printAtMiddle(text, pos.x + x, pos.y + y, font, kolor, dst, refresh);
}

void CIntObject::blitAtLoc( SDL_Surface * src, int x, int y, SDL_Surface * dst )
{
	blitAt(src, pos.x + x, pos.y + y, dst);
}

void CIntObject::printAtMiddleWBLoc( const std::string & text, int x, int y, EFonts font, int charpr, SDL_Color kolor, SDL_Surface * dst, bool refrsh /*= false*/ )
{
	CSDL_Ext::printAtMiddleWB(text, pos.x + x, pos.y + y, font, charpr, kolor, dst, refrsh);
}

void CIntObject::printToLoc( const std::string & text, int x, int y, EFonts font, SDL_Color kolor, SDL_Surface * dst, bool refresh /*= false*/ )
{
	CSDL_Ext::printTo(text, pos.x + x, pos.y + y, font, kolor, dst, refresh);
}

void CIntObject::disable()
{
	if(active)
		deactivate();

	recActions = DISPOSE;
}

void CIntObject::enable(bool activation)
{
	if(!active && activation)
		activate();

	recActions = 255;
}

bool CIntObject::isItInLoc( const SDL_Rect &rect, int x, int y )
{
	return isItIn(&rect, x - pos.x, y - pos.y);
}

bool CIntObject::isItInLoc( const SDL_Rect &rect, const Point &p )
{
	return isItIn(&rect, p.x - pos.x, p.y - pos.y);
}

void CIntObject::activateWheel()
{
	GH.wheelInterested.push_front(this);
	active |= WHEEL;
}

void CIntObject::deactivateWheel()
{
	std::list<CIntObject*>::iterator hlp = std::find(GH.wheelInterested.begin(),GH.wheelInterested.end(),this);
	assert(hlp != GH.wheelInterested.end());
	GH.wheelInterested.erase(hlp);
	active &= ~WHEEL;
}

void CIntObject::wheelScrolled(bool down, bool in)
{
}

void CIntObject::activateDClick()
{
	GH.doubleClickInterested.push_front(this);
	active |= DOUBLECLICK;
}

void CIntObject::deactivateDClick()
{
	std::list<CIntObject*>::iterator hlp = std::find(GH.doubleClickInterested.begin(),GH.doubleClickInterested.end(),this);
	assert(hlp != GH.doubleClickInterested.end());
	GH.doubleClickInterested.erase(hlp);
	active &= ~DOUBLECLICK;
}

void CIntObject::onDoubleClick()
{
}

const Rect & CIntObject::center( const Rect &r )
{
	pos.w = r.w;
	pos.h = r.h;
	pos.x = screen->w/2 - r.w/2;
	pos.y = screen->h/2 - r.h/2;
	return pos;
}

const Rect & CIntObject::center()
{
	return center(pos);
}

CPicture::CPicture( SDL_Surface *BG, int x, int y, bool Free )
{
	bg = BG; 
	freeSurf = Free;
	pos.x += x;
	pos.y += y;
	pos.w = BG->w;
	pos.h = BG->h;
}

CPicture::CPicture( const std::string &bmpname, int x, int y )
{
	bg = BitmapHandler::loadBitmap(bmpname); 
	freeSurf = true;;
	pos.x += x;
	pos.y += y;
	pos.w = bg->w;
	pos.h = bg->h;
}

CPicture::~CPicture()
{
	if(freeSurf)
		SDL_FreeSurface(bg);
}

void CPicture::showAll( SDL_Surface * to )
{
	blitAt(bg, pos, to);
}

ObjectConstruction::ObjectConstruction( CIntObject *obj )
	:myObj(obj)
{
	GH.createdObj.push_front(obj);
	GH.captureChildren = true;
}

ObjectConstruction::~ObjectConstruction()
{
	assert(GH.createdObj.size());
	assert(GH.createdObj.front() == myObj);
	GH.createdObj.pop_front();
	GH.captureChildren = GH.createdObj.size();
}

SetCaptureState::SetCaptureState(bool allow, ui8 actions)
{
	previousCapture = GH.captureChildren;
	GH.captureChildren = false;
	prevActions = GH.defActionsDef;
	GH.defActionsDef = actions;
}

SetCaptureState::~SetCaptureState()
{
	GH.captureChildren = previousCapture;
	GH.defActionsDef = prevActions;
}

void IShowable::redraw()
{
	showAll(screenBuf);
	if(screenBuf != screen)
		showAll(screen);
}

SDLKey arrowToNum( SDLKey key )
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
	throw std::string("Wrong key!");
}

SDLKey numToDigit( SDLKey key )
{
	return SDLKey(key - SDLK_KP0 + SDLK_0);
}

bool isNumKey( SDLKey key, bool number )
{
	if(number)
		return key >= SDLK_KP0 && key <= SDLK_KP_EQUALS;
	else
		return key >= SDLK_KP0 && key <= SDLK_KP9;
}

bool isArrowKey( SDLKey key )
{
	return key >= SDLK_UP && key <= SDLK_LEFT;
}