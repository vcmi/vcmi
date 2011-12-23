#include "StdInc.h"
#include "CIntObject.h"
#include "CGuiHandler.h"
#include "SDL_Extensions.h"

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
	pressedL = pressedR = hovered = captureAllKeys = strongInterest = false;
	toNextTick = active = used = 0;

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

void CIntObject::drawBorderLoc(SDL_Surface * sur, const Rect &r, const int3 &color)
{
	CSDL_Ext::drawBorder(sur, r + pos, color);
}

void CIntObject::redraw()
{
	if (parent && (type & REDRAW_PARENT))
	{
		parent->redraw();
	}
	else
	{
		showAll(screenBuf);
		if(screenBuf != screen)
			showAll(screen);
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

void CKeyShortcut::keyPressed(const SDL_KeyboardEvent & key)
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