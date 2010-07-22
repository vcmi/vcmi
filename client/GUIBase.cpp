#include "SDL_Extensions.h"
#include <cassert>
#include <boost/thread/locks.hpp>
#include "GUIBase.h"
#include <boost/thread/mutex.hpp>
#include <queue>
#include "CGameInfo.h"
#include "CCursorHandler.h"
#include "CBitmapHandler.h"
#include "Graphics.h"
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
		} 
		else 
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
			if((*i)->captureAllKeys)
			{
				keysCaptured = true;
				break;
			}
		}

		std::list<CIntObject*> miCopy = keyinterested;
		for(std::list<CIntObject*>::iterator i=miCopy.begin(); i != miCopy.end() && current; i++)
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
	handleMoveInterested(sme);
}

void CGuiHandler::run()
{
	try
	{
		while(!terminate)
		{
			if(curInt)
				curInt->update();
			SDL_Delay(20); //give time for other apps
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
}

CGuiHandler::~CGuiHandler()
{

}

void CGuiHandler::breakEventHandling()
{
	current = NULL;
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
	else
		parent = NULL;
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
	activate(used);

	if(defActions & ACTIVATE)
		for(size_t i = 0; i < children.size(); i++)
			if(children[i]->recActions & ACTIVATE)
				children[i]->activate();
}

void CIntObject::activate(ui16 what)
{
	if(what & LCLICK)
		activateLClick();
	if(what & RCLICK)
		activateRClick();
	if(what & HOVER)
		activateHover();
	if(what & MOVE)
		activateMouseMove();
	if(what & KEYBOARD)
		activateKeys();
	if(what & TIME)
		activateTimer();
	if(what & WHEEL)
		activateWheel();
	if(what & DOUBLECLICK)
		activateDClick();
}

void CIntObject::deactivate()
{
	assert(active);
	active &= ~ GENERAL;
	deactivate(used);

	assert(!active);

	if(defActions & DEACTIVATE)
		for(size_t i = 0; i < children.size(); i++)
			if(children[i]->recActions & DEACTIVATE)
				children[i]->deactivate();
}

void CIntObject::deactivate(ui16 what)
{
	if(what & LCLICK)
		deactivateLClick();
	if(what & RCLICK)
		deactivateRClick();
	if(what & HOVER)
		deactivateHover();
	if(what & MOVE)
		deactivateMouseMove();
	if(what & KEYBOARD)
		deactivateKeys();
	if(what & TIME)			// TIME is special
		deactivateTimer();
	if(what & WHEEL)
		deactivateWheel();
	if(what & DOUBLECLICK)
		deactivateDClick();
}

CIntObject::~CIntObject()
{
	assert(!active); //do not delete active obj

	if(defActions & DISPOSE)
		for(size_t i = 0; i < children.size(); i++)
			if(children[i]->recActions & DISPOSE)
				delete children[i];

	if(parent && GH.createdObj.size()) //temporary object destroyed
		parent->children -= this;
}

void CIntObject::printAtLoc( const std::string & text, int x, int y, EFonts font, SDL_Color kolor/*=zwykly*/, SDL_Surface * dst/*=screen*/, bool refresh /*= false*/ )
{
	CSDL_Ext::printAt(text, pos.x + x, pos.y + y, font, kolor, dst, refresh);
}

void CIntObject::printAtMiddleLoc( const std::string & text, int x, int y, EFonts font, SDL_Color kolor/*=zwykly*/, SDL_Surface * dst/*=screen*/, bool refresh /*= false*/ )
{
	CSDL_Ext::printAtMiddle(text, pos.x + x, pos.y + y, font, kolor, dst, refresh);
}

void CIntObject::printAtMiddleLoc(const std::string & text, const Point &p, EFonts font, SDL_Color kolor, SDL_Surface * dst, bool refresh /*= false*/)
{
	printAtMiddleLoc(text, p.x, p.y, font, kolor, dst, refresh);
}

void CIntObject::blitAtLoc( SDL_Surface * src, int x, int y, SDL_Surface * dst )
{
	blitAt(src, pos.x + x, pos.y + y, dst);
}

void CIntObject::blitAtLoc(SDL_Surface * src, const Point &p, SDL_Surface * dst)
{
	blitAtLoc(src, p.x, p.y, dst);
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

const Rect & CIntObject::center( const Rect &r, bool propagate )
{
	pos.w = r.w;
	pos.h = r.h;
	moveBy(Point(screen->w/2 - r.w/2 - pos.x, 
				 screen->h/2 - r.h/2 - pos.y), 
			propagate);
	return pos;
}

const Rect & CIntObject::center( bool propagate )
{
	return center(pos, propagate);
}

void CIntObject::moveBy( const Point &p, bool propagate /*= true*/ )
{
	pos.x += p.x;
	pos.y += p.y;
	if(propagate)
		for(size_t i = 0; i < children.size(); i++)
			children[i]->moveBy(p, propagate);
}

void CIntObject::moveTo( const Point &p, bool propagate /*= true*/ )
{
	moveBy(Point(p.x - pos.x, p.y - pos.y), propagate);
}

void CIntObject::delChild(CIntObject *child)
{
	children -= child;
	delete child;
}

void CIntObject::addChild(CIntObject *child, bool adjustPosition /*= false*/)
{
	assert(!vstd::contains(children, child));
	assert(child->parent == NULL);
	children.push_back(child);
	child->parent = this;
	if(adjustPosition)
		child->pos += pos;
}

void CIntObject::removeChild(CIntObject *child, bool adjustPosition /*= false*/)
{
	assert(vstd::contains(children, child));
	assert(child->parent == this);
	children -= child;
	child->parent = NULL;
	if(adjustPosition)
		child->pos -= pos;
}

void CIntObject::changeUsedEvents(ui16 what, bool enable, bool adjust /*= true*/)
{
	if(enable)
	{
		used |= what;
		if(adjust && active)
			activate(what);
	}
	else
	{
		used &= ~what;
		if(adjust && active)
			deactivate(what);
	}
}

CPicture::CPicture( SDL_Surface *BG, int x, int y, bool Free )
{
	init();
	bg = BG; 
	freeSurf = Free;
	pos.x += x;
	pos.y += y;
	pos.w = BG->w;
	pos.h = BG->h;
}

CPicture::CPicture( const std::string &bmpname, int x, int y )
{
	init();
	bg = BitmapHandler::loadBitmap(bmpname); 
	freeSurf = true;;
	pos.x += x;
	pos.y += y;
	if(bg)
	{
		pos.w = bg->w;
		pos.h = bg->h;
	}
	else
	{
		pos.w = pos.h = 0;
	}
}

CPicture::CPicture(const Rect &r, const SDL_Color &color, bool screenFormat /*= false*/)
{
	init();
	createSimpleRect(r, screenFormat, SDL_MapRGB(bg->format, color.r, color.g,color.b));
}

CPicture::CPicture(const Rect &r, ui32 color, bool screenFormat /*= false*/)
{
	init();
	createSimpleRect(r, screenFormat, color);
}

CPicture::CPicture(SDL_Surface *BG, const Rect &SrcRect, int x /*= 0*/, int y /*= 0*/, bool free /*= false*/)
{
	srcRect = new Rect(SrcRect);
	pos.x += x;
	pos.y += y;
	bg = BG;
	freeSurf = free;
}

CPicture::~CPicture()
{
	if(freeSurf)
		SDL_FreeSurface(bg);
	delete srcRect;
}

void CPicture::init()
{
	srcRect = NULL;
}

void CPicture::showAll( SDL_Surface * to )
{
	if(bg)
	{
		if(srcRect)
		{
			SDL_Rect srcRectCpy = *srcRect;
			SDL_Rect dstRect = srcRectCpy;
			dstRect.x = pos.x;
			dstRect.y = pos.y;

			SDL_BlitSurface(bg, &srcRectCpy, to, &dstRect);
		}
		else
			blitAt(bg, pos, to);
	}
}

void CPicture::convertToScreenBPP()
{
	SDL_Surface *hlp = bg;
	bg = SDL_ConvertSurface(hlp,screen->format,0);
	SDL_SetColorKey(bg,SDL_SRCCOLORKEY,SDL_MapRGB(bg->format,0,255,255));
	SDL_FreeSurface(hlp);
}

void CPicture::createSimpleRect(const Rect &r, bool screenFormat, ui32 color)
{
	pos += r;
	pos.w = r.w;
	pos.h = r.h;
	if(screenFormat)
		bg = CSDL_Ext::newSurface(r.w, r.h);
	else
		bg = SDL_CreateRGBSurface(SDL_SWSURFACE, r.w, r.h, 8, 0, 0, 0, 0);

	SDL_FillRect(bg, NULL, color);
	freeSurf = true;
}

void CPicture::colorizeAndConvert(int player)
{
	assert(bg);
	assert(bg->format->BitsPerPixel == 8);
	graphics->blueToPlayersAdv(bg, player);
	convertToScreenBPP();
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

bool isNumKey( SDLKey key, bool number )
{
	if(number)
		return key >= SDLK_KP0 && key <= SDLK_KP9;
	else
		return key >= SDLK_KP0 && key <= SDLK_KP_EQUALS;
}

bool isArrowKey( SDLKey key )
{
	return key >= SDLK_UP && key <= SDLK_LEFT;
}

CIntObject * moveChild(CIntObject *obj, CIntObject *from, CIntObject *to, bool adjustPos)
{
	from->removeChild(obj, adjustPos);
	to->addChild(obj, adjustPos);
	return obj;
}
Rect Rect::createCentered( int w, int h )
{
	return Rect(screen->w/2 - w/2, screen->h/2 - h/2, w, h);
}