#include "AdventureMapButton.h"
#include "CAnimation.h"
//#include "CAdvmapInterface.h"
#include "SDL_Extensions.h"
#include "CGameInfo.h"
//#include "../lib/CGeneralTextHandler.h"
//#include "../lib/CTownHandler.h"
#include "../CCallback.h"
#include "CConfigHandler.h"
//#include "Graphics.h"
#include "CBattleInterface.h"
#include "CPlayerInterface.h"
#include "CMessage.h"
#include "CMusicHandler.h"
#include "GUIClasses.h"

/*
 * AdventureMapButton.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

CButtonBase::CButtonBase()
{
	swappedImages = keepFrame = false;
	bitmapOffset = 0;
	state=NORMAL;
	image = NULL;
	text = NULL;
}

CButtonBase::~CButtonBase()
{
	
}

void CButtonBase::update()
{
	if (text)
	{
		if (state == PRESSED)
			text->moveTo(Point(pos.x+pos.w/2+1, pos.y+pos.h/2+1));
		else
			text->moveTo(Point(pos.x+pos.w/2, pos.y+pos.h/2));
	}
	
	size_t newPos = (int)state + bitmapOffset;
	if (state == HIGHLIGHTED && image->size() < 4)
		newPos = image->size()-1;

	if (swappedImages)
	{
		     if (newPos == 0) newPos = 1;
		else if (newPos == 1) newPos = 0;
	}

	if (!keepFrame)
	{
		image->setFrame(newPos);
	}
	
	if (active)
	{
		if (parent)
			parent->redraw();
		else
			redraw();
	}
}

void CButtonBase::addTextOverlay( const std::string &Text, EFonts font, SDL_Color color)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL;
	delChild(text);
	text = new CLabel(pos.w/2, pos.h/2, font, CENTER, color, Text);
	update();
}

void CButtonBase::setOffset(int newOffset)
{
	if (bitmapOffset == newOffset)
		return;
	bitmapOffset = newOffset;
	update();
}

void CButtonBase::setState(ButtonState newState)
{
	if (state == newState)
		return;
	state = newState;
	update();
}

CButtonBase::ButtonState CButtonBase::getState()
{
	return state;
}

bool CButtonBase::isBlocked()
{
	return state == BLOCKED;
}

bool CButtonBase::isHighlighted()
{
	return state == HIGHLIGHTED;
}

void CButtonBase::block(bool on)
{
	setState(on?BLOCKED:NORMAL);
}

AdventureMapButton::AdventureMapButton ()
{
	hoverable = actOnDown = borderEnabled = soundDisabled = false;
	borderColor.unused = 1; // represents a transparent color, used for HighlightableButton
	used = LCLICK | RCLICK | HOVER | KEYBOARD;
}

AdventureMapButton::AdventureMapButton( const std::string &Name, const std::string &HelpBox, const CFunctionList<void()> &Callback, int x, int y,  const std::string &defName,int key, std::vector<std::string> * add, bool playerColoredButton )
{
	std::map<int,std::string> pom;
	pom[0] = Name;
	init(Callback, pom, HelpBox, playerColoredButton, defName, add, x, y, key);
}

AdventureMapButton::AdventureMapButton( const std::string &Name, const std::string &HelpBox, const CFunctionList<void()> &Callback, config::ButtonInfo *info, int key/*=0*/ )
{
	std::map<int,std::string> pom;
	pom[0] = Name;
	init(Callback, pom, HelpBox, info->playerColoured, info->defName, &info->additionalDefs, info->x, info->y, key);
}

AdventureMapButton::AdventureMapButton( const std::pair<std::string, std::string> &help, const CFunctionList<void()> &Callback, int x, int y, const std::string &defName, int key/*=0*/, std::vector<std::string> * add /*= NULL*/, bool playerColoredButton /*= false */ )
{
	std::map<int,std::string> pom;
	pom[0] = help.first;
	init(Callback, pom, help.second, playerColoredButton, defName, add, x, y, key);
}
void AdventureMapButton::clickLeft(tribool down, bool previousState)
{
	if(isBlocked())
		return;

	if (down) 
	{
		if (!soundDisabled)
			CCS->soundh->playSound(soundBase::button);
		setState(PRESSED);
	} 
	else if(hoverable && hovered)
		setState(HIGHLIGHTED);
	else
		setState(NORMAL);

	if (actOnDown && down)
	{
		callback();
	}
	else if (!actOnDown && previousState && (down==false))
	{
		callback();
	}
}

void AdventureMapButton::clickRight(tribool down, bool previousState)
{
	if(down && helpBox.size()) //there is no point to show window with nothing inside...
		CRClickPopup::createAndPush(helpBox);
}

void AdventureMapButton::hover (bool on)
{
	if(hoverable)
	{
		if(on)
			setState(HIGHLIGHTED);
		else
			setState(NORMAL);
	}

	if(pressedL && on)
		setState(PRESSED);

	std::string *name = (vstd::contains(hoverTexts,getState())) 
							? (&hoverTexts[getState()]) 
							: (vstd::contains(hoverTexts,0) ? (&hoverTexts[0]) : NULL);
	if(name && name->size() && !isBlocked()) //if there is no name, there is nohing to display also
	{
		if (LOCPLINT && LOCPLINT->battleInt) //for battle buttons
		{
			if(on && LOCPLINT->battleInt->console->alterTxt == "")
			{
				LOCPLINT->battleInt->console->alterTxt = *name;
				LOCPLINT->battleInt->console->whoSetAlter = 1;
			}
			else if (LOCPLINT->battleInt->console->alterTxt == *name)
			{
				LOCPLINT->battleInt->console->alterTxt = "";
				LOCPLINT->battleInt->console->whoSetAlter = 0;
			}
		}
		else if(GH.statusbar) //for other buttons
		{
			if (on)
				GH.statusbar->print(*name);
			else if ( GH.statusbar->getCurrent()==(*name) )
				GH.statusbar->clear();
		}
	}
}

void AdventureMapButton::init(const CFunctionList<void()> &Callback, const std::map<int,std::string> &Name, const std::string &HelpBox, bool playerColoredButton, const std::string &defName, std::vector<std::string> * add, int x, int y, int key)
{
	currentImage = -1;
	used = LCLICK | RCLICK | HOVER | KEYBOARD;
	callback = Callback;
	hoverable = actOnDown = borderEnabled = soundDisabled = false;
	borderColor.unused = 1; // represents a transparent color, used for HighlightableButton
	assignedKeys.insert(key);
	hoverTexts = Name;
	helpBox=HelpBox;

	pos.x += x;
	pos.y += y;

	if (!defName.empty())
		imageNames.push_back(defName);
	if (add)
		for (size_t i=0; i<add->size();i++ )
			imageNames.push_back(add->at(i));
	setIndex(0, playerColoredButton);
}

void AdventureMapButton::setIndex(size_t index, bool playerColoredButton)
{
	if (index == currentImage || index>=imageNames.size())
		return;
	currentImage = index;
	setImage(new CAnimation(imageNames[index]), playerColoredButton);
}

void AdventureMapButton::setImage(CAnimation* anim, bool playerColoredButton)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL;
	
	if (image && active)
		image->deactivate();
	delChild(image);
	image = new CAnimImage(anim, getState());
	if (active)
		image->activate();
	if (playerColoredButton)
		image->playerColored(LOCPLINT->playerID);

	pos.w = image->pos.w;
	pos.h = image->pos.h;
}

void AdventureMapButton::setPlayerColor(int player)
{
	if (image)
	image->playerColored(player);
}

void AdventureMapButton::showAll(SDL_Surface *to)
{
	CIntObject::showAll(to);

	if (borderEnabled && borderColor.unused == 0)
		CSDL_Ext::drawBorder(to, pos.x - 1, pos.y - 1, pos.w + 2, pos.h + 2, int3(borderColor.r, borderColor.g, borderColor.b));
}

void CHighlightableButton::select(bool on)
{
	selected = on;
	if (on)
	{
		setState(HIGHLIGHTED);
		callback();
		borderEnabled = true;
	}
	else
	{
		setState(NORMAL);
		callback2();
		borderEnabled = false;
	}
	
	if(hoverTexts.size()>1)
	{
		hover(false);
		hover(true);
	}
}

void CHighlightableButton::clickLeft(tribool down, bool previousState)
{
	if(isBlocked())
		return;

	if (down && !(onlyOn && isHighlighted()))
	{
		CCS->soundh->playSound(soundBase::button);
		setState(PRESSED);
	}

	if(previousState  &&  down == false && getState() == PRESSED)
	{
		//if(!onlyOn || !isHighlighted())
			select(!selected);
	}
}

CHighlightableButton::CHighlightableButton( const CFunctionList<void()> &onSelect, const CFunctionList<void()> &onDeselect, const std::map<int,std::string> &Name, const std::string &HelpBox, bool playerColoredButton, const std::string &defName, std::vector<std::string> * add, int x, int y, int key)
: onlyOn(false), selected(false), callback2(onDeselect)
{
	init(onSelect,Name,HelpBox,playerColoredButton,defName,add,x,y,key);
}

CHighlightableButton::CHighlightableButton( const std::pair<std::string, std::string> &help, const CFunctionList<void()> &onSelect, int x, int y, const std::string &defName, int myid, int key/*=0*/, std::vector<std::string> * add /*= NULL*/, bool playerColoredButton /*= false */ )
: onlyOn(false), selected(false) // TODO: callback2(???)
{
	ID = myid;
	std::map<int,std::string> pom;
	pom[0] = help.first;
	init(onSelect, pom, help.second, playerColoredButton, defName, add, x, y, key);
}

CHighlightableButton::CHighlightableButton( const std::string &Name, const std::string &HelpBox, const CFunctionList<void()> &onSelect, int x, int y, const std::string &defName, int myid, int key/*=0*/, std::vector<std::string> * add /*= NULL*/, bool playerColoredButton /*= false */ )
: onlyOn(false), selected(false) // TODO: callback2(???)
{
	ID = myid;
	std::map<int,std::string> pom;
	pom[0] = Name;
	init(onSelect, pom,HelpBox, playerColoredButton, defName, add, x, y, key);
}

void CHighlightableButtonsGroup::addButton(CHighlightableButton* bt)
{
	if (bt->parent)
		bt->parent->removeChild(bt);
	addChild(bt);
	bt->recActions = defActions;//FIXME: not needed?

	bt->callback += boost::bind(&CHighlightableButtonsGroup::selectionChanged,this,bt->ID);
	bt->onlyOn = true;
	buttons.push_back(bt);
}

void CHighlightableButtonsGroup::addButton(const std::map<int,std::string> &tooltip, const std::string &HelpBox, const std::string &defName, int x, int y, int uid, const CFunctionList<void()> &OnSelect, int key)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL;
	CHighlightableButton *bt = new CHighlightableButton(OnSelect, 0, tooltip, HelpBox, false, defName, 0, x, y, key);
	if(musicLike)
	{
		bt->setOffset(buttons.size() - 3);
	}
	bt->ID = uid;
	bt->callback += boost::bind(&CHighlightableButtonsGroup::selectionChanged,this,bt->ID);
	bt->onlyOn = true;
	buttons.push_back(bt);
}	

CHighlightableButtonsGroup::CHighlightableButtonsGroup(const CFunctionList2<void(int)> &OnChange, bool musicLikeButtons)
: onChange(OnChange), musicLike(musicLikeButtons)
{}

CHighlightableButtonsGroup::~CHighlightableButtonsGroup()
{
	
}

void CHighlightableButtonsGroup::select(int id, bool mode)
{
	CHighlightableButton *bt = NULL;
	if(mode)
	{
		for(size_t i=0;i<buttons.size() && !bt; ++i)
			if (buttons[i]->ID == id)
				bt = buttons[i];
	}
	else
	{
		bt = buttons[id];
	}
	bt->select(true);
	selectionChanged(bt->ID);
}

void CHighlightableButtonsGroup::selectionChanged(int to)
{
	for(size_t i=0;i<buttons.size(); ++i)
		if(buttons[i]->ID!=to && buttons[i]->isHighlighted())
			buttons[i]->select(false);
	onChange(to);
}

void CHighlightableButtonsGroup::show(SDL_Surface * to )
{
	if (musicLike)
	{
		for(size_t i=0;i<buttons.size(); ++i)
			if(buttons[i]->isHighlighted())
				buttons[i]->show(to);
	}
	else
		CIntObject::show(to);
}

void CHighlightableButtonsGroup::showAll( SDL_Surface * to )
{
	if (musicLike)
	{
		for(size_t i=0;i<buttons.size(); ++i)
			if(buttons[i]->isHighlighted())
				buttons[i]->showAll(to);
	}
	else
		CIntObject::showAll(to);
}

void CHighlightableButtonsGroup::block( ui8 on )
{
	for(size_t i=0;i<buttons.size(); ++i) 
	{
		buttons[i]->block(on);
	}
}

void CSlider::sliderClicked()
{
	if(!(active & MOVE))
	{
		activateMouseMove();
		used |= MOVE;
	}
}

void CSlider::mouseMoved (const SDL_MouseMotionEvent & sEvent)
{
	float v = 0;
	if(horizontal)
	{
		if(	std::abs(sEvent.y-(pos.y+pos.h/2)) > pos.h/2+40  ||  std::abs(sEvent.x-(pos.x+pos.w/2)) > pos.w/2  ) 
			return;
		v = sEvent.x - pos.x - 24;
		v *= positions;
		v /= (pos.w - 48);
	}
	else
	{
		if(std::abs(sEvent.x-(pos.x+pos.w/2)) > pos.w/2+40  ||  std::abs(sEvent.y-(pos.y+pos.h/2)) > pos.h/2  ) 
			return;
		v = sEvent.y - pos.y - 24;
		v *= positions;
		v /= (pos.h - 48);
	}
	v += 0.5f;
	if(v!=value)
	{
		moveTo(v);
		redrawSlider();
	}
}

void CSlider::redrawSlider()
{
	//slider->show(screenBuf);
}

void CSlider::moveLeft()
{
	moveTo(value-1);
}

void CSlider::moveRight()
{
	moveTo(value+1);
}

void CSlider::moveTo(int to)
{
	amax(to, 0);
	amin(to, positions);

	//same, old position?
	if(value == to)
		return;

	value = to;
	if(horizontal)
	{
		if(positions)
		{
			float part = (float)to/positions;
			part*=(pos.w-48);
			int newPos = part + pos.x + 16 - slider->pos.x;
			slider->moveBy(Point(newPos, 0));
		}
		else
			slider->moveTo(Point(pos.x+16, pos.y));
	}
	else
	{
		if(positions)
		{
			float part = (float)to/positions;
			part*=(pos.h-48);
			int newPos = part + pos.y + 16 - slider->pos.y;
			slider->moveBy(Point(0, newPos));
		}
		else
			slider->moveTo(Point(pos.x, pos.y+16));
	}

	if(moved)
		moved(to);
}

void CSlider::clickLeft(tribool down, bool previousState)
{
	if(down && !slider->isBlocked())
	{
		float pw = 0;
		float rw = 0;
		if(horizontal)
		{
			pw = GH.current->motion.x-pos.x-25;
			rw = pw / ((float)(pos.w-48));
		}
		else
		{
			pw = GH.current->motion.y-pos.y-24;
			rw = pw / ((float)(pos.h-48));
		}
		if(pw < -8  ||  pw > (horizontal ? pos.w : pos.h) - 40)
			return;
// 		if (rw>1) return;
// 		if (rw<0) return;
		slider->clickLeft(true, slider->pressedL);
		moveTo(rw * positions  +  0.5f);
		return;
	}
	if(active & MOVE)
	{
		deactivateMouseMove();
		used &= ~MOVE;
	}
}

CSlider::~CSlider()
{
	
}

CSlider::CSlider(int x, int y, int totalw, boost::function<void(int)> Moved, int Capacity, int Amount, int Value, bool Horizontal, int style)
:capacity(Capacity),amount(Amount),horizontal(Horizontal), moved(Moved)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL;
	setAmount(amount);

	used = LCLICK;
	strongInterest = true;


	left = new AdventureMapButton();
	right = new AdventureMapButton();
	slider = new AdventureMapButton();

	pos.x += x;
	pos.y += y;

	if(horizontal)
	{
		left->pos.y = slider->pos.y = right->pos.y = pos.y;
		left->pos.x = pos.x;
		right->pos.x = pos.x + totalw - 16;
	}
	else 
	{
		left->pos.x = slider->pos.x = right->pos.x = pos.x;
		left->pos.y = pos.y;
		right->pos.y = pos.y + totalw - 16;
	}

	left->callback = boost::bind(&CSlider::moveLeft,this);
	right->callback = boost::bind(&CSlider::moveRight,this);
	slider->callback = boost::bind(&CSlider::sliderClicked,this);
	left->pos.w = left->pos.h = right->pos.w = right->pos.h = slider->pos.w = slider->pos.h = 16;
	if(horizontal)
	{
		pos.h = 16;
		pos.w = totalw;
	}
	else
	{
		pos.w = 16;
		pos.h = totalw;
	}

	if(style == 0)
	{
		std::string name = horizontal?"IGPCRDIV.DEF":"OVBUTN2.DEF";
		CAnimation *animLeft = new CAnimation(name);
		left->setImage(animLeft);
		left->setOffset(0);

		CAnimation *animRight = new CAnimation(name);
		right->setImage(animRight);
		right->setOffset(2);

		CAnimation *animSlider = new CAnimation(name);
		slider->setImage(animSlider);
		slider->setOffset(4);
	}
	else
	{
		left->setImage(new CAnimation(horizontal ? "SCNRBLF.DEF" : "SCNRBUP.DEF"));
		right->setImage(new CAnimation(horizontal ? "SCNRBRT.DEF" : "SCNRBDN.DEF"));
		slider->setImage(new CAnimation("SCNRBSL.DEF"));
	}
	slider->actOnDown = true;
	slider->soundDisabled = true;
	left->soundDisabled = true;
	right->soundDisabled = true;

	value = -1;
	moveTo(Value);
}

void CSlider::block( bool on )
{
	left->block(on);
	right->block(on);
	slider->block(on);
}

void CSlider::setAmount( int to )
{
	amount = to;
	positions = to - capacity;
	amax(positions, 0);
}

void CSlider::showAll(SDL_Surface * to)
{
	CSDL_Ext::fillRect(to, &pos, 0);
	CIntObject::showAll(to);
}

void CSlider::wheelScrolled(bool down, bool in)
{
	moveTo(value + 3 * (down ? +1 : -1));
}

void CSlider::keyPressed(const SDL_KeyboardEvent & key)
{
	if(key.state != SDL_PRESSED) return;

	int moveDest = 0;
	switch(key.keysym.sym)
	{
	case SDLK_UP:
		moveDest = value - 1;
		break;
	case SDLK_DOWN:
		moveDest = value + 1;
		break;
	case SDLK_PAGEUP:
		moveDest = value - capacity + 1;
		break;
	case SDLK_PAGEDOWN:
		moveDest = value + capacity - 1;
		break;
	case SDLK_HOME:
		moveDest = 0;
		break;
	case SDLK_END:
		moveDest = amount - capacity;
		break;
	default:
		return;
	}

	moveTo(moveDest); 
}

void CSlider::moveToMax()
{
	moveTo(amount);
}
