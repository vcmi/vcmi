#include "SDL_Extensions.h"
#include <cassert>
#include <boost/thread/locks.hpp>
#include "GUIBase.h"
#include <boost/thread/mutex.hpp>
#include <queue>
#include "CGameInfo.h"
#include "CCursorHandler.h"
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
		if(key.state == SDL_PRESSED)
			clickLeft(true, pressedL);
		else
			clickLeft(false, pressedL);
	}
}


CButtonBase::CButtonBase()
{
	bitmapOffset = 0;
	curimg=0;
	type=-1;
	abs=false;
	active=false;
	notFreeButton = false;
	ourObj=NULL;
	state=0;
}

CButtonBase::~CButtonBase()
{
	if(notFreeButton)
		return;
	for(int i =0; i<imgs.size();i++)
		for(int j=0;j<imgs[i].size();j++)
			SDL_FreeSurface(imgs[i][j]);
}

void CButtonBase::show(SDL_Surface * to)
{
	int img = std::min(state+bitmapOffset,int(imgs[curimg].size()-1));
	img = std::max(0, img);

	if (abs)
	{
		blitAt(imgs[curimg][img],pos.x,pos.y,to);
	}
	else
	{
		blitAt(imgs[curimg][img],pos.x+ourObj->pos.x,pos.y+ourObj->pos.y,to);
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
		if (key.keysym.sym >= SDLK_KP0  && key.keysym.sym <= SDLK_KP9)
		{
			key.keysym.sym = (SDLKey) (key.keysym.sym - SDLK_KP0 + SDLK_0);
		}
		else if(key.keysym.sym == SDLK_KP_ENTER)
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
	else if ((sEvent->type==SDL_MOUSEBUTTONDOWN) && (sEvent->button.button == SDL_BUTTON_LEFT))
	{
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
	else if ((sEvent->type==SDL_MOUSEBUTTONDOWN) && (sEvent->button.button == SDL_BUTTON_RIGHT))
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
	pressedL = pressedR = hovered = captureAllKeys = strongInterest = toNextTick = active = defActivation = 0;
}

void CIntObject::show( SDL_Surface * to )
{

}

void CIntObject::activate()
{

}

void CIntObject::deactivate()
{

}