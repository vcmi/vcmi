#include "StdInc.h"
#include "CIntObject.h"
#include "CGuiHandler.h"
#include "SDL_Extensions.h"

void CIntObject::activateLClick()
{
	GH.lclickable.push_front(this);
	active_m |= LCLICK;
}

void CIntObject::deactivateLClick()
{
	std::list<CIntObject*>::iterator hlp = std::find(GH.lclickable.begin(),GH.lclickable.end(),this);
	assert(hlp != GH.lclickable.end());
	GH.lclickable.erase(hlp);
	active_m &= ~LCLICK;
}

void CIntObject::activateRClick()
{
	GH.rclickable.push_front(this);
	active_m |= RCLICK;
}

void CIntObject::deactivateRClick()
{
	std::list<CIntObject*>::iterator hlp = std::find(GH.rclickable.begin(),GH.rclickable.end(),this);
	assert(hlp != GH.rclickable.end());
	GH.rclickable.erase(hlp);
	active_m &= ~RCLICK;
}

void CIntObject::activateHover()
{
	GH.hoverable.push_front(this);
	active_m |= HOVER;
}

void CIntObject::deactivateHover()
{
	std::list<CIntObject*>::iterator hlp = std::find(GH.hoverable.begin(),GH.hoverable.end(),this);
	assert(hlp != GH.hoverable.end());
	GH.hoverable.erase(hlp);
	active_m &= ~HOVER;
}

void CIntObject::activateKeys()
{
	GH.keyinterested.push_front(this);
	active_m |= KEYBOARD;
}

void CIntObject::deactivateKeys()
{
	std::list<CIntObject*>::iterator hlp = std::find(GH.keyinterested.begin(),GH.keyinterested.end(),this);
	assert(hlp != GH.keyinterested.end());
	GH.keyinterested.erase(hlp);
	active_m &= ~KEYBOARD;
}

void CIntObject::activateMouseMove()
{
	GH.motioninterested.push_front(this);
	active_m |= MOVE;
}

void CIntObject::deactivateMouseMove()
{
	std::list<CIntObject*>::iterator hlp = std::find(GH.motioninterested.begin(),GH.motioninterested.end(),this);
	assert(hlp != GH.motioninterested.end());
	GH.motioninterested.erase(hlp);
	active_m &= ~MOVE;
}

void CIntObject::activateWheel()
{
	GH.wheelInterested.push_front(this);
	active_m |= WHEEL;
}

void CIntObject::deactivateWheel()
{
	std::list<CIntObject*>::iterator hlp = std::find(GH.wheelInterested.begin(),GH.wheelInterested.end(),this);
	assert(hlp != GH.wheelInterested.end());
	GH.wheelInterested.erase(hlp);
	active_m &= ~WHEEL;
}

void CIntObject::activateDClick()
{
	GH.doubleClickInterested.push_front(this);
	active_m |= DOUBLECLICK;
}

void CIntObject::deactivateDClick()
{
	std::list<CIntObject*>::iterator hlp = std::find(GH.doubleClickInterested.begin(),GH.doubleClickInterested.end(),this);
	assert(hlp != GH.doubleClickInterested.end());
	GH.doubleClickInterested.erase(hlp);
	active_m &= ~DOUBLECLICK;
}

void CIntObject::activateTimer()
{
	GH.timeinterested.push_back(this);
	active_m |= TIME;
}

void CIntObject::deactivateTimer()
{
	std::list<CIntObject*>::iterator hlp = std::find(GH.timeinterested.begin(),GH.timeinterested.end(),this);
	assert(hlp != GH.timeinterested.end());
	GH.timeinterested.erase(hlp);
	active_m &= ~TIME;
}

CIntObject::CIntObject(int used_, Point pos_):
	parent_m(nullptr),
	active_m(0),
	parent(parent_m),
	active(active_m)
{
	pressedL = pressedR = hovered = captureAllKeys = strongInterest = false;
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

void CIntObject::setTimer(int msToTrigger)
{
	if (!(active & TIME))
		activateTimer();
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
		for(size_t i = 0; i < children.size(); i++)
			if(children[i]->recActions & UPDATE)
				children[i]->show(to);
}

void CIntObject::showAll(SDL_Surface * to)
{
	if(defActions & SHOWALL)
	{
		for(size_t i = 0; i < children.size(); i++)
			if(children[i]->recActions & SHOWALL)
				children[i]->showAll(to);

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
			tlog1 << "Warning: IntObject re-activated with mismatching used and active\n";
			deactivate(); //FIXME: better to avoid such possibility at all
		}
	}

	active_m |= GENERAL;
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
	if (!active_m)
		return;

	active_m &= ~ GENERAL;
	deactivate(active_m);

	assert(!active_m);

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
	if(what & TIME)
		deactivateTimer();
	if(what & WHEEL)
		deactivateWheel();
	if(what & DOUBLECLICK)
		deactivateDClick();
}

CIntObject::~CIntObject()
{
	if (active_m)
		deactivate();

	if(defActions & DISPOSE)
	{
		while (!children.empty())
			if(children.front()->recActions & DISPOSE)
				delete children.front();
			else
				removeChild(children.front());
	}

	if(parent_m)
		parent_m->removeChild(this);
}

void CIntObject::printAtLoc( const std::string & text, int x, int y, EFonts font, SDL_Color kolor/*=Colors::Cornsilk*/, SDL_Surface * dst/*=screen*/ )
{
	CSDL_Ext::printAt(text, pos.x + x, pos.y + y, font, kolor, dst);
}

void CIntObject::printAtMiddleLoc( const std::string & text, int x, int y, EFonts font, SDL_Color kolor/*=Colors::Cornsilk*/, SDL_Surface * dst/*=screen*/ )
{
	CSDL_Ext::printAtMiddle(text, pos.x + x, pos.y + y, font, kolor, dst);
}

void CIntObject::printAtMiddleLoc(const std::string & text, const Point &p, EFonts font, SDL_Color kolor, SDL_Surface * dst)
{
	printAtMiddleLoc(text, p.x, p.y, font, kolor, dst);
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
	CSDL_Ext::printAtMiddleWB(text, pos.x + x, pos.y + y, font, charpr, kolor, dst);
}

void CIntObject::printToLoc( const std::string & text, int x, int y, EFonts font, SDL_Color kolor, SDL_Surface * dst )
{
	CSDL_Ext::printTo(text, pos.x + x, pos.y + y, font, kolor, dst);
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
	if(!active_m && parent_m->active)
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

void CIntObject::addChild(CIntObject *child, bool adjustPosition /*= false*/)
{
	if (vstd::contains(children, child))
	{
//		tlog4<< "Warning: object already assigned to this parent!\n";
		return;
	}
	if (child->parent_m)
	{
//		tlog4<< "Warning: object already has parent!\n";
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

void CIntObject::removeChild(CIntObject *child, bool adjustPosition /*= false*/)
{
	if (!child)
		return;

	assert(vstd::contains(children, child));
	assert(child->parent_m == this);
	children -= child;
	child->parent_m = NULL;
	if(adjustPosition)
		child->pos -= pos;
}

void CIntObject::drawBorderLoc(SDL_Surface * sur, const Rect &r, const int3 &color)
{
	CSDL_Ext::drawBorder(sur, r + pos, color);
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

const Rect & CIntObject::center(const Point &p, bool propagate /*= true*/)
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

void CKeyShortcut::keyPressed(const SDL_KeyboardEvent & key)
{
	if(vstd::contains(assignedKeys,key.keysym.sym)
	 || vstd::contains(assignedKeys, CGuiHandler::numToDigit(key.keysym.sym)))
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
