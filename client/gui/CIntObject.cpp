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
#include "SDL_Extensions.h"
#include "../CMessage.h"

IShowActivatable::IShowActivatable()
{
	type = 0;
}

CIntObject::CIntObject(int used_, Point pos_):
	parent_m(nullptr),
	active_m(0),
	parent(parent_m),
	active(active_m)
{
	hovered = captureAllKeys = strongInterest = false;
	toNextTick = timerDelay = 0;
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
	if(active_m)
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

void CIntObject::setTimer(int msToTrigger)
{
	if (!(active & TIME))
		activate(TIME);
	toNextTick = timerDelay = msToTrigger;
	used |= TIME;
}

void CIntObject::onTimer(int timePassed)
{
	toNextTick -= timePassed;
	if (toNextTick < 0)
	{
		toNextTick += timerDelay;
		tick();
	}
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
	if (active_m)
	{
		if ((used | GENERAL) == active_m)
			return;
		else
		{
			logGlobal->warn("Warning: IntObject re-activated with mismatching used and active");
			deactivate(); //FIXME: better to avoid such possibility at all
		}
	}

	active_m |= GENERAL;
	activate(used);

	if(defActions & ACTIVATE)
		for(auto & elem : children)
			if(elem->recActions & ACTIVATE)
				elem->activate();
}

void CIntObject::activate(ui16 what)
{
	GH.handleElementActivate(this, what);
}

void CIntObject::deactivate()
{
	if (!active_m)
		return;

	active_m &= ~ GENERAL;
	deactivate(active_m);

	assert(!active_m);

	if(defActions & DEACTIVATE)
		for(auto & elem : children)
			if(elem->recActions & DEACTIVATE)
				elem->deactivate();
}

void CIntObject::deactivate(ui16 what)
{
	GH.handleElementDeActivate(this, what);
}

void CIntObject::click(EIntObjMouseBtnType btn, tribool down, bool previousState)
{
	switch(btn)
	{
	default:
	case EIntObjMouseBtnType::LEFT:
		clickLeft(down, previousState);
		break;
	case EIntObjMouseBtnType::MIDDLE:
		clickMiddle(down, previousState);
		break;
	case EIntObjMouseBtnType::RIGHT:
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

void CIntObject::blitAtLoc( SDL_Surface * src, int x, int y, SDL_Surface * dst )
{
	blitAt(src, pos.x + x, pos.y + y, dst);
}

void CIntObject::blitAtLoc(SDL_Surface * src, const Point &p, SDL_Surface * dst)
{
	blitAtLoc(src, p.x, p.y, dst);
}

void CIntObject::printAtMiddleWBLoc( const std::string & text, int x, int y, EFonts font, int charpr, SDL_Color kolor, SDL_Surface * dst)
{
	graphics->fonts[font]->renderTextLinesCenter(dst, CMessage::breakText(text, charpr, font), kolor, Point(pos.x + x, pos.y + y));
}

void CIntObject::addUsedEvents(ui16 newActions)
{
	if (active_m)
		activate(~used & newActions);
	used |= newActions;
}

void CIntObject::removeUsedEvents(ui16 newActions)
{
	if (active_m)
		deactivate(used & newActions);
	used &= ~newActions;
}

void CIntObject::disable()
{
	if(active)
		deactivate();

	recActions = DISPOSE;
}

void CIntObject::enable()
{
	if(!active_m && (!parent_m || parent_m->active))
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

void CIntObject::fitToScreen(int borderWidth, bool propagate)
{
	Point newPos = pos.topLeft();
	vstd::amax(newPos.x, borderWidth);
	vstd::amax(newPos.y, borderWidth);
	vstd::amin(newPos.x, screen->w - borderWidth - pos.w);
	vstd::amin(newPos.y, screen->h - borderWidth - pos.h);
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
		child->pos += pos;

	if (!active && child->active)
		child->deactivate();
	if (active && !child->active)
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
		child->pos -= pos;
}

void CIntObject::redraw()
{
	//currently most of calls come from active objects so this check won't affect them
	//it should fix glitches when called by inactive elements located below active window
	if (active)
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

const Rect & CIntObject::center( const Rect &r, bool propagate )
{
	pos.w = r.w;
	pos.h = r.h;
	return center(Point(screen->w/2, screen->h/2), propagate);
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

bool CIntObject::captureThisEvent(const SDL_KeyboardEvent & key)
{
	return captureAllKeys;
}

CKeyShortcut::CKeyShortcut()
{}

CKeyShortcut::CKeyShortcut(int key)
{
	if (key != SDLK_UNKNOWN)
		assignedKeys.insert(key);
}

CKeyShortcut::CKeyShortcut(std::set<int> Keys)
	:assignedKeys(Keys)
{}

void CKeyShortcut::keyPressed(const SDL_KeyboardEvent & key)
{
	if(vstd::contains(assignedKeys,key.keysym.sym)
	 || vstd::contains(assignedKeys, CGuiHandler::numToDigit(key.keysym.sym)))
	{
		bool prev = mouseState(EIntObjMouseBtnType::LEFT);
		updateMouseState(EIntObjMouseBtnType::LEFT, key.state == SDL_PRESSED);
		clickLeft(key.state == SDL_PRESSED, prev);

	}
}

WindowBase::WindowBase(int used_, Point pos_)
	: CIntObject(used_, pos_)
{

}

void WindowBase::close()
{
	if(GH.topInt().get() != this)
		logGlobal->error("Only top interface must be closed");
	GH.popInts(1);
}
