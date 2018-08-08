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
#include "View.h"

#include "../CGuiHandler.h"
#include "../SDL_Extensions.h"
#include "../../CMessage.h"

IShowActivatable::IShowActivatable()
{
	type = 0;
}

View::View(ui16 used_, Point pos_):
	active(0),
	pos(pos_.x, pos_.y),
	used(used_),
	hovered(false),
	captureAllKeys(false),
	strongInterest(false),
	defActions(GH.defActionsDef),
	recActions(GH.defActionsDef)
{
	if(GH.captureChildren)
		GH.createdObj.front()->addChild(this, true);
}

View::~View()
{
	if(active)
		deactivate();

	while(!children.empty())
	{
		if((defActions & DISPOSE) && (children.front()->recActions & DISPOSE))
			delete children.front();
		else
			removeChild(children.front());
	}

	if(getParent())
		getParent()->removeChild(this);
}

void View::show(SDL_Surface * to)
{
	if(defActions & UPDATE)
		for(auto & elem : children)
			if(elem->recActions & UPDATE)
				elem->show(to);
}

void View::showAll(SDL_Surface * to)
{
	if(defActions & SHOWALL)
	{
		for(auto & elem : children)
			if(elem->recActions & SHOWALL)
				elem->showAll(to);

	}
}

void View::activate()
{
	if (active)
	{
		if ((used | GENERAL) == active)
			return;
		else
		{
			logGlobal->warn("Warning: IntObject re-activated with mismatching used and active");
			deactivate(); //FIXME: better to avoid such possibility at all
		}
	}
	
	active |= GENERAL;
	activate(used);

	if(defActions & ACTIVATE)
		for(auto & elem : children)
			if(elem->recActions & ACTIVATE)
				elem->activate();
}

void View::activate(ui16 what)
{
	GH.handleElementActivate(this, what);
}

void View::deactivate()
{
	if (!active)
		return;
	
	active &= ~ GENERAL;
	deactivate(active);

	assert(!active);

	if(defActions & DEACTIVATE)
		for(auto & elem : children)
			if(elem->recActions & DEACTIVATE)
				elem->deactivate();
}

void View::deactivate(ui16 what)
{
	GH.handleElementDeActivate(this, what);
}

void View::event(const SDL_Event &event)
{
	if (event.type == SDL_MOUSEBUTTONDOWN || event.type == SDL_MOUSEBUTTONUP)
	{
		bool state = (event.type == SDL_MOUSEBUTTONDOWN ? true : false);
		
		tribool down = state;
		if (!isItIn(&pos, event.motion.x, event.motion.y) && !down)
			down = boost::logic::indeterminate;
		
		switch (event.button.button)
		{
			case SDL_BUTTON_LEFT:
				clickLeft(event, down);
				break;
			case SDL_BUTTON_MIDDLE:
				clickMiddle(event, down);
				break;
			case SDL_BUTTON_RIGHT:
				clickRight(event, down);
				break;
		}
	}
}

void View::blitAtLoc( SDL_Surface * src, int x, int y, SDL_Surface * dst )
{
	blitAt(src, pos.x + x, pos.y + y, dst);
}


void View::addUsedEvents(ui16 newActions)
{
	if (active)
		activate(~used & newActions);
	used |= newActions;
}

void View::removeUsedEvents(ui16 newActions)
{
	if (active)
		deactivate(used & newActions);
	used &= ~newActions;
}

void View::disable()
{
	if(active)
		deactivate();

	recActions = DISPOSE;
}

void View::enable()
{
	if(!active && (!getParent() || getParent()->active))
		activate();

	recActions = 255;
}

void View::moveBy(const Point & p, bool propagate)
{
	pos.x += p.x;
	pos.y += p.y;
	if(propagate)
		for(auto & elem : children)
			elem->moveBy(p, propagate);
}

void View::moveTo(const Point & p, bool propagate)
{
	moveBy(Point(p.x - pos.x, p.y - pos.y), propagate);
}

void View::addChild(View * child, bool adjustPosition)
{
	if (vstd::contains(children, child))
	{
		return;
	}
	if (child->getParent())
	{
		child->getParent()->removeChild(child, adjustPosition);
	}
	children.push_back(child);
	child->setParent(this);
	if(adjustPosition)
		child->pos += pos;

	if (!active && child->active)
		child->deactivate();
	if (active && !child->active)
		child->activate();
}

void View::removeChild(View * child, bool adjustPosition)
{
	if (!child)
		return;

	if(!vstd::contains(children, child))
		throw std::runtime_error("Wrong child object");

	if(child->getParent() != this)
		throw std::runtime_error("Wrong child object");

	children -= child;
	child->setParent(nullptr);
	if(adjustPosition)
		child->pos -= pos;
}

void View::redraw()
{
	//currently most of calls come from active objects so this check won't affect them
	//it should fix glitches when called by inactive elements located below active window
	if (active)
	{
		if (getParent() && (type & REDRAW_PARENT))
		{
			getParent()->redraw();
		}
		else
		{
			showAll(screenBuf);
			if(screenBuf != screen)
				showAll(screen);
		}
	}
}

const Rect & View::center( const Rect &r, bool propagate )
{
	pos.w = r.w;
	pos.h = r.h;
	return center(Point(screen->w/2, screen->h/2), propagate);
}

const Rect & View::center( bool propagate )
{
	return center(pos, propagate);
}

const Rect & View::center(const Point & p, bool propagate)
{
	moveBy(Point(p.x - pos.w/2 - pos.x,
		p.y - pos.h/2 - pos.y),
		propagate);
	return pos;
}

bool View::captureThisEvent(const SDL_KeyboardEvent & key)
{
	return captureAllKeys;
}

View *View::getParent() const
{
	return parent;
}

void View::setParent(View *parent)
{
	View::parent = parent;
}

CKeyShortcut::CKeyShortcut()
{}

CKeyShortcut::CKeyShortcut(int key)
{
	if (key != SDLK_UNKNOWN)
		assignedKeys.insert(key);
}

CKeyShortcut::CKeyShortcut(std::set<int> & Keys)
	:assignedKeys(Keys)
{}

void CKeyShortcut::keyPressed(const SDL_Event & event, const SDL_KeyboardEvent & key)
{
	if(vstd::contains(assignedKeys,key.keysym.sym)
	 || vstd::contains(assignedKeys, CGuiHandler::numToDigit(key.keysym.sym)))
	{
		clickLeft(event, key.state == SDL_PRESSED);
	}
}

WindowBase::WindowBase(int used_, Point pos_)
	: View(used_, pos_)
{

}

void WindowBase::close()
{
	if(GH.topInt().get() != this)
		logGlobal->error("Only top interface must be closed");
	GH.popInts(1);
}
