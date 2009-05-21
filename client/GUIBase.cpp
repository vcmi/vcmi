#include "GUIBase.h"
#include "SDL_Extensions.h"
#include "CPlayerInterface.h"

/*
 * GUIBase.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

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
	LOCPLINT->lclickable.push_front(this);
}
void ClickableL::deactivate()
{
	LOCPLINT->lclickable.erase
		(std::find(LOCPLINT->lclickable.begin(),LOCPLINT->lclickable.end(),this));
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
	LOCPLINT->rclickable.push_front(this);
}
void ClickableR::deactivate()
{
	LOCPLINT->rclickable.erase(std::find(LOCPLINT->rclickable.begin(),LOCPLINT->rclickable.end(),this));
}
//ClickableR

Hoverable::~Hoverable()
{}

void Hoverable::activate()
{
	LOCPLINT->hoverable.push_front(this);
}

void Hoverable::deactivate()
{
	LOCPLINT->hoverable.erase(std::find(LOCPLINT->hoverable.begin(),LOCPLINT->hoverable.end(),this));
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
	LOCPLINT->keyinterested.push_front(this);
}

void KeyInterested::deactivate()
{
	LOCPLINT->
		keyinterested.erase(std::find(LOCPLINT->keyinterested.begin(),LOCPLINT->keyinterested.end(),this));
}
//KeyInterested

void MotionInterested::activate()
{
	LOCPLINT->motioninterested.push_front(this);
}

void MotionInterested::deactivate()
{
	LOCPLINT->
		motioninterested.erase(std::find(LOCPLINT->motioninterested.begin(),LOCPLINT->motioninterested.end(),this));
}

void TimeInterested::activate()
{
	LOCPLINT->timeinterested.push_back(this);
}

void TimeInterested::deactivate()
{
	LOCPLINT->
		timeinterested.erase(std::find(LOCPLINT->timeinterested.begin(),LOCPLINT->timeinterested.end(),this));
}

