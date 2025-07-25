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

#include "GameEngine.h"
#include "WindowHandler.h"
#include "EventDispatcher.h"
#include "Shortcut.h"
#include "../render/Canvas.h"
#include "../render/IScreenHandler.h"
#include "../windows/CMessage.h"

CIntObject::CIntObject(int used_, Point pos_):
	parent_m(nullptr),
	parent(parent_m),
	redrawParent(false),
	inputEnabled(true),
	used(used_),
	recActions(ALL_ACTIONS),
	pos(pos_, Point())
{
	if(ENGINE->captureChildren)
		ENGINE->createdObj.front()->addChild(this, true);
}

CIntObject::~CIntObject()
{
	if(isActive())
		deactivate();

	while(!children.empty())
		removeChild(children.front());

	if(parent_m)
		parent_m->removeChild(this);
}

void CIntObject::show(Canvas & to)
{
	for(auto & elem : children)
		if(elem->recActions & UPDATE)
			elem->show(to);
}

void CIntObject::showAll(Canvas & to)
{
	for(auto & elem : children)
		if(elem->recActions & SHOWALL)
			elem->showAll(to);
}

void CIntObject::activate()
{
	if (isActive())
		return;

	if (inputEnabled)
		activateEvents(used | GENERAL);
	else
		activateEvents(GENERAL);

	assert(isActive());

	for(auto & elem : children)
		if(elem->recActions & ACTIVATE)
			elem->activate();
}

void CIntObject::deactivate()
{
	if (!isActive())
		return;

	deactivateEvents(used | GENERAL);

	assert(!isActive());

	for(auto & elem : children)
		if(elem->recActions & DEACTIVATE)
			elem->deactivate();
}

void CIntObject::addUsedEvents(ui16 newActions)
{
	if (isActive() && inputEnabled)
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

	recActions = NO_ACTIONS;
}

void CIntObject::enable()
{
	if(!isActive() && (!parent_m || parent_m->isActive()))
	{
		activate();
		redraw();
	}

	recActions = ALL_ACTIONS;
}

bool CIntObject::isDisabled() const
{
	return recActions == NO_ACTIONS;
}

void CIntObject::setEnabled(bool on)
{
	if (on)
		enable();
	else
		disable();
}

void CIntObject::setInputEnabled(bool on)
{
	if (inputEnabled == on)
		return;

	inputEnabled = on;

	if (isActive())
	{
		assert((used & GENERAL) == 0);

		if (on)
			activateEvents(used);
		else
			deactivateEvents(used);
	}

	for(auto & elem : children)
		elem->setInputEnabled(on);
}

void CIntObject::setRedrawParent(bool on)
{
	redrawParent = on;
}

void CIntObject::fitToScreen(int borderWidth, bool propagate)
{
	fitToRect(Rect(Point(0, 0), ENGINE->screenDimensions()), borderWidth, propagate);
}

void CIntObject::fitToRect(Rect rect, int borderWidth, bool propagate)
{
	Point newPos = pos.topLeft();
	vstd::amax(newPos.x, rect.x + borderWidth);
	vstd::amax(newPos.y, rect.y + borderWidth);
	vstd::amin(newPos.x, rect.x + rect.w - borderWidth - pos.w);
	vstd::amin(newPos.y, rect.y + rect.h - borderWidth - pos.h);
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

	if (inputEnabled != child->inputEnabled)
		child->setInputEnabled(inputEnabled);

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
		if (parent_m && redrawParent)
		{
			parent_m->redraw();
		}
		else
		{
			Canvas buffer = ENGINE->screenHandler().getScreenCanvas();
			showAll(buffer);
		}
	}
}

void CIntObject::moveChildForeground(const CIntObject * childToMove)
{
	for(auto child = children.begin(); child != children.end(); child++)
		if(*child == childToMove && child != children.end())
		{
			std::rotate(child, child + 1, children.end());
		}
}

bool CIntObject::receiveEvent(const Point & position, int eventType) const
{
	return pos.isInside(position);
}

const Rect & CIntObject::getPosition() const
{
	return pos;
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
	return center(Point(ENGINE->screenDimensions().x/2, ENGINE->screenDimensions().y/2), propagate);
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
	return false;
}

CKeyShortcut::CKeyShortcut()
	: assignedKey(EShortcut::NONE)
	, shortcutPressed(false)
{}

CKeyShortcut::CKeyShortcut(EShortcut key)
	: assignedKey(key)
	, shortcutPressed(false)
{
}

void CKeyShortcut::keyPressed(EShortcut key)
{
	if( assignedKey == key && assignedKey != EShortcut::NONE && !shortcutPressed)
	{
		shortcutPressed = true;
		clickPressed(ENGINE->getCursorPosition());
	}
}

void CKeyShortcut::keyReleased(EShortcut key)
{
	if( assignedKey == key && assignedKey != EShortcut::NONE && shortcutPressed)
	{
		shortcutPressed = false;
		clickReleased(ENGINE->getCursorPosition());
	}
}

WindowBase::WindowBase(int used_, Point pos_)
	: CIntObject(used_, pos_)
{

}

void WindowBase::close()
{
	if(!ENGINE->windows().isTopWindow(this))
	{
		auto topWindow = ENGINE->windows().topWindow<IShowActivatable>().get();
		throw std::runtime_error(std::string("Only top interface can be closed! Top window is ") + typeid(*topWindow).name() + " but attempted to close " + typeid(*this).name());
	}
	ENGINE->windows().popWindows(1);
}
