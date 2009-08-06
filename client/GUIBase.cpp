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
			clickLeft(true);
		else
			clickLeft(false);
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

ClickableL::ClickableL()
{
	pressedL=false;
}

ClickableL::~ClickableL()
{
}

void ClickableL::clickLeft(boost::logic::tribool down)
{
	if (down)
		pressedL=true;
	else
		pressedL=false;
}
void ClickableL::activate()
{
	GH.lclickable.push_front(this);
}
void ClickableL::deactivate()
{
	GH.lclickable.erase(std::find(GH.lclickable.begin(),GH.lclickable.end(),this));
}

ClickableR::ClickableR()
{
	pressedR=false;
}

ClickableR::~ClickableR()
{}

void ClickableR::clickRight(boost::logic::tribool down)
{
	if (down)
		pressedR=true;
	else
		pressedR=false;
}
void ClickableR::activate()
{
	GH.rclickable.push_front(this);
}
void ClickableR::deactivate()
{
	GH.rclickable.erase(std::find(GH.rclickable.begin(),GH.rclickable.end(),this));
}
//ClickableR

Hoverable::~Hoverable()
{}

void Hoverable::activate()
{
	GH.hoverable.push_front(this);
}

void Hoverable::deactivate()
{
	GH.hoverable.erase(std::find(GH.hoverable.begin(),GH.hoverable.end(),this));
}
void Hoverable::hover(bool on)
{
	hovered=on;
}
//Hoverable

KeyInterested::~KeyInterested()
{}

void KeyInterested::activate()
{
	GH.keyinterested.push_front(this);
}

void KeyInterested::deactivate()
{
	GH.keyinterested.erase(std::find(GH.keyinterested.begin(),GH.keyinterested.end(),this));
}
//KeyInterested

void MotionInterested::activate()
{
	GH.motioninterested.push_front(this);
}

void MotionInterested::deactivate()
{
	GH.motioninterested.erase(std::find(GH.motioninterested.begin(),GH.motioninterested.end(),this));
}

void TimeInterested::activate()
{
	GH.timeinterested.push_back(this);
}

void TimeInterested::deactivate()
{
	GH.timeinterested.erase(std::find(GH.timeinterested.begin(),GH.timeinterested.end(),this));
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
	std::list<TimeInterested*> hlp = timeinterested;
	for (std::list<TimeInterested*>::iterator i=hlp.begin(); i != hlp.end();i++)
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
		for(std::list<KeyInterested*>::iterator i=keyinterested.begin(); i != keyinterested.end();i++)
		{
			if((*i)->captureAllKeys)
			{
				keysCaptured = true;
				break;
			}
		}

		std::list<KeyInterested*> miCopy = keyinterested;
		for(std::list<KeyInterested*>::iterator i=miCopy.begin(); i != miCopy.end();i++)
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
		std::list<ClickableL*> hlp = lclickable;
		for(std::list<ClickableL*>::iterator i=hlp.begin(); i != hlp.end();i++)
		{
			if(!vstd::contains(lclickable,*i)) continue;
			if (isItIn(&(*i)->pos,sEvent->motion.x,sEvent->motion.y))
			{
				(*i)->clickLeft(true);
			}
		}
	}
	else if ((sEvent->type==SDL_MOUSEBUTTONUP) && (sEvent->button.button == SDL_BUTTON_LEFT))
	{
		std::list<ClickableL*> hlp = lclickable;
		for(std::list<ClickableL*>::iterator i=hlp.begin(); i != hlp.end();i++)
		{
			if(!vstd::contains(lclickable,*i)) continue;
			if (isItIn(&(*i)->pos,sEvent->motion.x,sEvent->motion.y))
			{
				(*i)->clickLeft(false);
			}
			else
				(*i)->clickLeft(boost::logic::indeterminate);
		}
	}
	else if ((sEvent->type==SDL_MOUSEBUTTONDOWN) && (sEvent->button.button == SDL_BUTTON_RIGHT))
	{
		std::list<ClickableR*> hlp = rclickable;
		for(std::list<ClickableR*>::iterator i=hlp.begin(); i != hlp.end();i++)
		{
			if(!vstd::contains(rclickable,*i)) continue;
			if (isItIn(&(*i)->pos,sEvent->motion.x,sEvent->motion.y))
			{
				(*i)->clickRight(true);
			}
		}
	}
	else if ((sEvent->type==SDL_MOUSEBUTTONUP) && (sEvent->button.button == SDL_BUTTON_RIGHT))
	{
		std::list<ClickableR*> hlp = rclickable;
		for(std::list<ClickableR*>::iterator i=hlp.begin(); i != hlp.end();i++)
		{
			if(!vstd::contains(rclickable,*i)) continue;
			if (isItIn(&(*i)->pos,sEvent->motion.x,sEvent->motion.y))
			{
				(*i)->clickRight(false);
			}
			else
				(*i)->clickRight(boost::logic::indeterminate);
		}
	}
	current = NULL;

} //event end

void CGuiHandler::handleMouseMotion(SDL_Event *sEvent)
{
	//sending active, hovered hoverable objects hover() call
	std::vector<Hoverable*> hlp;
	for(std::list<Hoverable*>::iterator i=hoverable.begin(); i != hoverable.end();i++)
	{
		if (isItIn(&(*i)->pos,sEvent->motion.x,sEvent->motion.y))
		{
			if (!(*i)->hovered)
				hlp.push_back((*i));
		}
		else if ((*i)->hovered)
		{
			(*i)->hover(false);
		}
	}
	for(int i=0; i<hlp.size();i++)
		hlp[i]->hover(true);

	//sending active, MotionInterested objects mouseMoved() call
	std::list<MotionInterested*> miCopy = motioninterested;
	for(std::list<MotionInterested*>::iterator i=miCopy.begin(); i != miCopy.end();i++)
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