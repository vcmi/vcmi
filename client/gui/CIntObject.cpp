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
#include "WindowHandler.h"
#include "Shortcut.h"
#include "../render/Canvas.h"
#include "../windows/CMessage.h"
#include "../CMT.h"

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

void CIntObject::show(Canvas & to)
{
	if(defActions & UPDATE)
		for(auto & elem : children)
			if(elem->recActions & UPDATE)
				elem->show(to);
}

void CIntObject::showAll(Canvas & to)
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
			Canvas buffer = Canvas::createFromSurface(screenBuf);

			showAll(buffer);
			if(screenBuf != screen)
			{
				Canvas screenBuffer = Canvas::createFromSurface(screen);

				showAll(screenBuffer);
			}
		}
	}
}

bool CIntObject::receiveEvent(const Point & position, int eventType) const
{
	return pos.isInside(position);
}

void CIntObject::onScreenResize()
{
	center(pos, true);
}

bool CIntObject::isPopupWindow() const
{
	return false;
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
	, shortcutPressed(false)
{}

CKeyShortcut::CKeyShortcut(EShortcut key)
	: assignedKey(key)
{
}

void CKeyShortcut::keyPressed(EShortcut key)
{
	if( assignedKey == key && assignedKey != EShortcut::NONE && !shortcutPressed)
	{
		shortcutPressed = true;
		clickLeft(true, false);
	}
}

void CKeyShortcut::keyReleased(EShortcut key)
{
	if( assignedKey == key && assignedKey != EShortcut::NONE && shortcutPressed)
	{
		shortcutPressed = false;
		clickLeft(false, true);
	}
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
