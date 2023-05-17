/*
 * CIntObject.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "CIntObject.h"

#include "CGuiHandler.h"
#include "InterfaceEventDispatcher.h"
#include "WindowHandler.h"
#include "Shortcut.h"
#include "../renderSDL/SDL_Extensions.h"
#include "../windows/CMessage.h"
#include "../CMT.h"

#include <SDL_pixels.h>

AEventsReceiver::AEventsReceiver()
	: activeState(0)
	, hoveredState(false)
	, strongInterestState(false)
{
}

bool AEventsReceiver::isHovered() const
{
	return hoveredState;
}

bool AEventsReceiver::isActive() const
{
	return activeState;
}

bool AEventsReceiver::isActive(int flags) const
{
	return activeState & flags;
}

bool AEventsReceiver::isMouseButtonPressed(MouseButton btn) const
{
	return currentMouseState.count(btn) ? currentMouseState.at(btn) : false;
}

void AEventsReceiver::setMoveEventStrongInterest(bool on)
{
	strongInterestState = on;
}

void AEventsReceiver::activateEvents(ui16 what)
{
	assert((what & GENERAL) || (activeState & GENERAL));

	activeState |= GENERAL;
	GH.eventDispatcher().handleElementActivate(this, what);
}

void AEventsReceiver::deactivateEvents(ui16 what)
{
	if (what & GENERAL)
		activeState &= ~GENERAL;
	GH.eventDispatcher().handleElementDeActivate(this, what & activeState);
}

CIntObject::CIntObject(int used_, Point pos_):
	parent_m(nullptr),
	parent(parent_m),
	type(0)
{
	captureAllKeys = false;
	used = used_;

	recActions = defActions = GH.defActionsDef;

	pos.x = pos_.x;
	pos.y = pos_.y;
	pos.w = 0;
	pos.h = 0;

	if(GH.captureChildren)
		GH.createdObj.front()->addChild(this, true);
}

CIntObject::~CIntObject()
{
	if(isActive())
		deactivate();

	while(!children.empty())
	{
		if((defActions & DISPOSE) && (children.front()->recActions & DISPOSE))
			delete children.front();
		else
			removeChild(children.front());
	}

	if(parent_m)
		parent_m->removeChild(this);
}

void CIntObject::show(SDL_Surface * to)
{
	if(defActions & UPDATE)
		for(auto & elem : children)
			if(elem->recActions & UPDATE)
				elem->show(to);
}

void CIntObject::showAll(SDL_Surface * to)
{
	if(defActions & SHOWALL)
	{
		for(auto & elem : children)
			if(elem->recActions & SHOWALL)
				elem->showAll(to);
	}
}

void CIntObject::activate()
{
	if (isActive())
		return;

	activateEvents(used | GENERAL);
	assert(isActive());

	if(defActions & ACTIVATE)
		for(auto & elem : children)
			if(elem->recActions & ACTIVATE)
				elem->activate();
}

void CIntObject::deactivate()
{
	if (!isActive())
		return;

	deactivateEvents(ALL);

	assert(!isActive());

	if(defActions & DEACTIVATE)
		for(auto & elem : children)
			if(elem->recActions & DEACTIVATE)
				elem->deactivate();
}

void CIntObject::click(MouseButton btn, tribool down, bool previousState)
{
	switch(btn)
	{
	default:
	case MouseButton::LEFT:
		clickLeft(down, previousState);
		break;
	case MouseButton::MIDDLE:
		clickMiddle(down, previousState);
		break;
	case MouseButton::RIGHT:
		clickRight(down, previousState);
		break;
	}
}

void CIntObject::printAtLoc(const std::string & text, int x, int y, EFonts font, SDL_Color kolor, SDL_Surface * dst)
{
	graphics->fonts[font]->renderTextLeft(dst, text, kolor, Point(pos.x + x, pos.y + y));
}

void CIntObject::printAtMiddleLoc(const std::string & text, int x, int y, EFonts font, SDL_Color kolor, SDL_Surface * dst)
{
	printAtMiddleLoc(text, Point(x,y), font, kolor, dst);
}

void CIntObject::printAtMiddleLoc(const std::string & text, const Point &p, EFonts font, SDL_Color kolor, SDL_Surface * dst)
{
	graphics->fonts[font]->renderTextCenter(dst, text, kolor, pos.topLeft() + p);
}

void CIntObject::printAtMiddleWBLoc( const std::string & text, int x, int y, EFonts font, int charpr, SDL_Color kolor, SDL_Surface * dst)
{
	graphics->fonts[font]->renderTextLinesCenter(dst, CMessage::breakText(text, charpr, font), kolor, Point(pos.x + x, pos.y + y));
}

void CIntObject::addUsedEvents(ui16 newActions)
{
	if (isActive())
		activateEvents(~used & newActions);
	used |= newActions;
}

void CIntObject::removeUsedEvents(ui16 newActions)
{
	if (isActive())
		deactivateEvents(used & newActions);
	used &= ~newActions;
}

void CIntObject::disable()
{
	if(isActive())
		deactivate();

	recActions = DISPOSE;
}

void CIntObject::enable()
{
	if(!isActive() && (!parent_m || parent_m->isActive()))
	{
		activate();
		redraw();
	}

	recActions = 255;
}

void CIntObject::setEnabled(bool on)
{
	if (on)
		enable();
	else
		disable();
}

void CIntObject::fitToScreen(int borderWidth, bool propagate)
{
	Point newPos = pos.topLeft();
	vstd::amax(newPos.x, borderWidth);
	vstd::amax(newPos.y, borderWidth);
	vstd::amin(newPos.x, GH.screenDimensions().x - borderWidth - pos.w);
	vstd::amin(newPos.y, GH.screenDimensions().y - borderWidth - pos.h);
	if (newPos != pos.topLeft())
		moveTo(newPos, propagate);
}

void CIntObject::moveBy(const Point & p, bool propagate)
{
	pos.x += p.x;
	pos.y += p.y;
	if(propagate)
		for(auto & elem : children)
			elem->moveBy(p, propagate);
}

void CIntObject::moveTo(const Point & p, bool propagate)
{
	moveBy(Point(p.x - pos.x, p.y - pos.y), propagate);
}

void CIntObject::addChild(CIntObject * child, bool adjustPosition)
{
	if (vstd::contains(children, child))
	{
		return;
	}
	if (child->parent_m)
	{
		child->parent_m->removeChild(child, adjustPosition);
	}
	children.push_back(child);
	child->parent_m = this;
	if(adjustPosition)
		child->moveBy(pos.topLeft(), adjustPosition);

	if (!isActive() && child->isActive())
		child->deactivate();
	if (isActive()&& !child->isActive())
		child->activate();
}

void CIntObject::removeChild(CIntObject * child, bool adjustPosition)
{
	if (!child)
		return;

	if(!vstd::contains(children, child))
		throw std::runtime_error("Wrong child object");

	if(child->parent_m != this)
		throw std::runtime_error("Wrong child object");

	children -= child;
	child->parent_m = nullptr;
	if(adjustPosition)
		child->pos -= pos.topLeft();
}

void CIntObject::redraw()
{
	//currently most of calls come from active objects so this check won't affect them
	//it should fix glitches when called by inactive elements located below active window
	if (isActive())
	{
		if (parent_m && (type & REDRAW_PARENT))
		{
			parent_m->redraw();
		}
		else
		{
			showAll(screenBuf);
			if(screenBuf != screen)
				showAll(screen);
		}
	}
}

bool CIntObject::isInside(const Point & position)
{
	return pos.isInside(position);
}

void CIntObject::onScreenResize()
{
	center(pos, true);
}

const Rect & CIntObject::center( const Rect &r, bool propagate )
{
	pos.w = r.w;
	pos.h = r.h;
	return center(Point(GH.screenDimensions().x/2, GH.screenDimensions().y/2), propagate);
}

const Rect & CIntObject::center( bool propagate )
{
	return center(pos, propagate);
}

const Rect & CIntObject::center(const Point & p, bool propagate)
{
	moveBy(Point(p.x - pos.w/2 - pos.x,
		p.y - pos.h/2 - pos.y),
		propagate);
	return pos;
}

bool CIntObject::captureThisKey(EShortcut key)
{
	return captureAllKeys;
}

CKeyShortcut::CKeyShortcut()
	: assignedKey(EShortcut::NONE)
{}

CKeyShortcut::CKeyShortcut(EShortcut key)
	: assignedKey(key)
{
}

void CKeyShortcut::keyPressed(EShortcut key)
{
	if( assignedKey == key && assignedKey != EShortcut::NONE)
		clickLeft(true, false);
}

void CKeyShortcut::keyReleased(EShortcut key)
{
	if( assignedKey == key && assignedKey != EShortcut::NONE)
		clickLeft(false, true);
}

WindowBase::WindowBase(int used_, Point pos_)
	: CIntObject(used_, pos_)
{

}

void WindowBase::close()
{
	if(!GH.windows().isTopWindow(this))
		logGlobal->error("Only top interface must be closed");
	GH.windows().popWindows(1);
}

IStatusBar::~IStatusBar()
{}
