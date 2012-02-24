#include "StdInc.h"
#include "CIntObjectClasses.h"

#include "../CBitmapHandler.h"
#include "SDL_Extensions.h"
#include "../Graphics.h"
#include "../CAnimation.h"
#include "../CGameInfo.h"
#include "../../CCallback.h"
#include "../CConfigHandler.h"
#include "../BattleInterface/CBattleInterface.h"
#include "../BattleInterface/CBattleInterfaceClasses.h"
#include "../CPlayerInterface.h"
#include "../CMessage.h"
#include "../CMusicHandler.h"
#include "../GUIClasses.h"
#include "CGuiHandler.h"
#include "../CAdvmapInterface.h"

CPicture::CPicture( SDL_Surface *BG, int x, int y, bool Free )
{
	init();
	bg = BG; 
	freeSurf = Free;
	pos.x += x;
	pos.y += y;
	pos.w = BG->w;
	pos.h = BG->h;
}

CPicture::CPicture( const std::string &bmpname, int x, int y )
{
	init();
	bg = BitmapHandler::loadBitmap(bmpname); 
	freeSurf = true;;
	pos.x += x;
	pos.y += y;
	if(bg)
	{
		pos.w = bg->w;
		pos.h = bg->h;
	}
	else
	{
		pos.w = pos.h = 0;
	}
}

CPicture::CPicture(const Rect &r, const SDL_Color &color, bool screenFormat /*= false*/)
{
	init();
	createSimpleRect(r, screenFormat, SDL_MapRGB(bg->format, color.r, color.g,color.b));
}

CPicture::CPicture(const Rect &r, ui32 color, bool screenFormat /*= false*/)
{
	init();
	createSimpleRect(r, screenFormat, color);
}

CPicture::CPicture(SDL_Surface *BG, const Rect &SrcRect, int x /*= 0*/, int y /*= 0*/, bool free /*= false*/)
{
	needRefresh = false;
	srcRect = new Rect(SrcRect);
	pos.x += x;
	pos.y += y;
	pos.w = srcRect->w;
	pos.h = srcRect->h;
	bg = BG;
	freeSurf = free;
}

CPicture::~CPicture()
{
	if(freeSurf)
		SDL_FreeSurface(bg);
	delete srcRect;
}

void CPicture::init()
{
	needRefresh = false;
	srcRect = NULL;
}

void CPicture::show(SDL_Surface * to)
{
	if (needRefresh)
		showAll(to);
}

void CPicture::showAll(SDL_Surface * to)
{
	if(bg)
	{
		if(srcRect)
		{
			SDL_Rect srcRectCpy = *srcRect;
			SDL_Rect dstRect = srcRectCpy;
			dstRect.x = pos.x;
			dstRect.y = pos.y;

			CSDL_Ext::blitSurface(bg, &srcRectCpy, to, &dstRect);
		}
		else
			blitAt(bg, pos, to);
	}
}

void CPicture::convertToScreenBPP()
{
	SDL_Surface *hlp = bg;
	bg = SDL_ConvertSurface(hlp,screen->format,0);
	SDL_SetColorKey(bg,SDL_SRCCOLORKEY,SDL_MapRGB(bg->format,0,255,255));
	SDL_FreeSurface(hlp);
}

void CPicture::createSimpleRect(const Rect &r, bool screenFormat, ui32 color)
{
	pos += r;
	pos.w = r.w;
	pos.h = r.h;
	if(screenFormat)
		bg = CSDL_Ext::newSurface(r.w, r.h);
	else
		bg = SDL_CreateRGBSurface(SDL_SWSURFACE, r.w, r.h, 8, 0, 0, 0, 0);

	SDL_FillRect(bg, NULL, color);
	freeSurf = true;
}

void CPicture::colorizeAndConvert(int player)
{
	assert(bg);
	colorize(player);
	convertToScreenBPP();
}

void CPicture::colorize(int player)
{
	assert(bg);
	assert(bg->format->BitsPerPixel == 8);
	graphics->blueToPlayersAdv(bg, player);
}

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

	int newPos = (int)state + bitmapOffset;
	if (newPos < 0)
		newPos = 0;

	if (state == HIGHLIGHTED && image->size() < 4)
		newPos = image->size()-1;

	if (swappedImages)
	{
		if (newPos == 0) newPos = 1;
		else if (newPos == 1) newPos = 0;
	}

	if (!keepFrame)
		image->setFrame(newPos);

	if (active)
		redraw();
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

CAdventureMapButton::CAdventureMapButton ()
{
	hoverable = actOnDown = borderEnabled = soundDisabled = false;
	borderColor.unused = 1; // represents a transparent color, used for HighlightableButton
	used = LCLICK | RCLICK | HOVER | KEYBOARD;
}

CAdventureMapButton::CAdventureMapButton( const std::string &Name, const std::string &HelpBox, const CFunctionList<void()> &Callback, int x, int y,  const std::string &defName,int key, std::vector<std::string> * add, bool playerColoredButton )
{
	std::map<int,std::string> pom;
	pom[0] = Name;
	init(Callback, pom, HelpBox, playerColoredButton, defName, add, x, y, key);
}

CAdventureMapButton::CAdventureMapButton( const std::string &Name, const std::string &HelpBox, const CFunctionList<void()> &Callback, config::ButtonInfo *info, int key/*=0*/ )
{
	std::map<int,std::string> pom;
	pom[0] = Name;
	init(Callback, pom, HelpBox, info->playerColoured, info->defName, &info->additionalDefs, info->x, info->y, key);
}

CAdventureMapButton::CAdventureMapButton( const std::pair<std::string, std::string> &help, const CFunctionList<void()> &Callback, int x, int y, const std::string &defName, int key/*=0*/, std::vector<std::string> * add /*= NULL*/, bool playerColoredButton /*= false */ )
{
	std::map<int,std::string> pom;
	pom[0] = help.first;
	init(Callback, pom, help.second, playerColoredButton, defName, add, x, y, key);
}
void CAdventureMapButton::clickLeft(tribool down, bool previousState)
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

void CAdventureMapButton::clickRight(tribool down, bool previousState)
{
	if(down && helpBox.size()) //there is no point to show window with nothing inside...
		CRClickPopup::createAndPush(helpBox);
}

void CAdventureMapButton::hover (bool on)
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

void CAdventureMapButton::init(const CFunctionList<void()> &Callback, const std::map<int,std::string> &Name, const std::string &HelpBox, bool playerColoredButton, const std::string &defName, std::vector<std::string> * add, int x, int y, int key)
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

void CAdventureMapButton::setIndex(size_t index, bool playerColoredButton)
{
	if (index == currentImage || index>=imageNames.size())
		return;
	currentImage = index;
	setImage(new CAnimation(imageNames[index]), playerColoredButton);
}

void CAdventureMapButton::setImage(CAnimation* anim, bool playerColoredButton, int animFlags)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL;

	if (image && active)
		image->deactivate();
	delChild(image);
	image = new CAnimImage(anim, getState(), 0, 0, 0, animFlags);
	if (active)
		image->activate();
	if (playerColoredButton)
		image->playerColored(LOCPLINT->playerID);

	pos.w = image->pos.w;
	pos.h = image->pos.h;
}

void CAdventureMapButton::setPlayerColor(int player)
{
	if (image)
		image->playerColored(player);
}

void CAdventureMapButton::showAll(SDL_Surface * to)
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

	if(previousState)//mouse up
	{
		if(down == false && getState() == PRESSED)
			select(!selected);
		else
			setState(selected?HIGHLIGHTED:NORMAL);
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
		if (buttons.size() > 3)
			bt->setOffset(buttons.size()-3);
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
	if (parent)
		parent->redraw();
}

void CHighlightableButtonsGroup::show(SDL_Surface * to)
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

void CHighlightableButtonsGroup::showAll(SDL_Surface * to)
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
	double v = 0;
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
	v += 0.5;
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
	vstd::amax(to, 0);
	vstd::amin(to, positions);

	//same, old position?
	if(value == to)
		return;

	value = to;
	if(horizontal)
	{
		if(positions)
		{
			double part = static_cast<double>(to) / positions;
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
			double part = static_cast<double>(to) / positions;
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
		double pw = 0;
		double rw = 0;
		if(horizontal)
		{
			pw = GH.current->motion.x-pos.x-25;
			rw = pw / static_cast<double>(pos.w - 48);
		}
		else
		{
			pw = GH.current->motion.y-pos.y-24;
			rw = pw / (pos.h-48);
		}
		if(pw < -8  ||  pw > (horizontal ? pos.w : pos.h) - 40)
			return;
		// 		if (rw>1) return;
		// 		if (rw<0) return;
		slider->clickLeft(true, slider->pressedL);
		moveTo(rw * positions  +  0.5);
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


	left = new CAdventureMapButton();
	right = new CAdventureMapButton();
	slider = new CAdventureMapButton();

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
	vstd::amax(positions, 0);
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

static void intDeleter(CIntObject* object)
{
	delete object;
}

CObjectList::CObjectList(CreateFunc create, DestroyFunc destroy):
createObject(create),
destroyObject(destroy)
{
	if (!destroyObject)
		destroyObject = intDeleter;
}

void CObjectList::deleteItem(CIntObject* item)
{
	if (!item)
		return;
	if (active)
		item->deactivate();
	removeChild(item);
	destroyObject(item);
}

CIntObject* CObjectList::createItem(size_t index)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL;
	CIntObject * item = createObject(index);
	if (item == NULL)
		item = new CIntObject();

	item->recActions = defActions;

	//May happen if object was created before call to getObject()
	if(item->parent != this)
	{
		if (item->parent)
			CGuiHandler::moveChild(item, item->parent, this);
		else
			addChild(item);
	}

	if (item && active)
		item->activate();
	return item;
}

CTabbedInt::CTabbedInt(CreateFunc create, DestroyFunc destroy, Point position, size_t ActiveID):
CObjectList(create, destroy),
activeTab(NULL),
activeID(ActiveID)
{
	pos += position;
	reset();
}

void CTabbedInt::setActive(size_t which)
{
	if (which != activeID)
	{
		activeID = which;
		reset();
	}
}

void CTabbedInt::reset()
{
	deleteItem(activeTab);
	activeTab = createItem(activeID);
	activeTab->moveTo(pos.topLeft());

	if (active)
		redraw();
}

CIntObject * CTabbedInt::getItem()
{
	return activeTab;
}

CListBox::CListBox(CreateFunc create, DestroyFunc destroy, Point Pos, Point ItemOffset, size_t VisibleSize,
				   size_t TotalSize, size_t InitialPos, int Slider, Rect SliderPos):
CObjectList(create, destroy),
first(InitialPos),
totalSize(TotalSize),
itemOffset(ItemOffset)
{
	pos += Pos;
	items.resize(VisibleSize, NULL);

	if (Slider & 1)
	{
		OBJ_CONSTRUCTION_CAPTURING_ALL;
		slider = new CSlider(SliderPos.x, SliderPos.y, SliderPos.w, boost::bind(&CListBox::moveToPos, this, _1),
			VisibleSize, TotalSize, InitialPos, Slider & 2, Slider & 4);
	}
	reset();
}

// Used to move active items after changing list position
void CListBox::updatePositions()
{
	Point itemPos = pos.topLeft();
	for (std::list<CIntObject*>::iterator it = items.begin(); it!=items.end(); it++)
	{
		(*it)->moveTo(itemPos);
		itemPos += itemOffset;
	}
	if (active)
	{
		redraw();
		if (slider)
			slider->moveTo(first);
	}
}

void CListBox::reset()
{
	size_t current = first;
	for (std::list<CIntObject*>::iterator it = items.begin(); it!=items.end(); it++)
	{
		deleteItem(*it);
		*it = createItem(current++);
	}
	updatePositions();
}

void CListBox::scrollTo(size_t which)
{
	//scroll up
	if (first > which)
		moveToPos(which);
	//scroll down
	else if (first + items.size() <= which)
		moveToPos(which - items.size());
}

void CListBox::moveToPos(size_t which)
{
	//Calculate new position
	size_t maxPossible;
	if (totalSize > items.size())
		maxPossible = totalSize - items.size();
	else
		maxPossible = 0;

	size_t newPos = std::min(which, maxPossible);

	//If move distance is 1 (most of calls from Slider) - use faster shifts instead of resetting all items
	if (first - newPos == 1)
		moveToPrev();
	else if (newPos - first == 1)
		moveToNext();
	else if (newPos != first)
	{
		first = newPos;
		reset();
	}
}

void CListBox::moveToNext()
{
	//Remove front item and insert new one to end
	if (first + items.size() < totalSize)
	{
		first++;
		deleteItem(items.front());
		items.pop_front();
		items.push_back(createItem(first+items.size()));
		updatePositions();
	}
}

void CListBox::moveToPrev()
{
	//Remove last item and insert new one at start
	if (first)
	{
		first--;
		deleteItem(items.back());
		items.pop_back();
		items.push_front(createItem(first));
		updatePositions();
	}
}

std::list<CIntObject*> CListBox::getItems()
{
	return items;
}

void CSimpleWindow::show(SDL_Surface * to)
{
	if(bitmap)
		blitAt(bitmap,pos.x,pos.y,to);
}
CSimpleWindow::~CSimpleWindow()
{
	if (bitmap)
	{
		SDL_FreeSurface(bitmap);
		bitmap=NULL;
	}
}

CStatusBar::CStatusBar(int x, int y, std::string name, int maxw)
{
	bg=BitmapHandler::loadBitmap(name);
	SDL_SetColorKey(bg,SDL_SRCCOLORKEY,SDL_MapRGB(bg->format,0,255,255));
	pos.x += x;
	pos.y += y;
	if(maxw >= 0)
		pos.w = std::min(bg->w,maxw);
	else
		pos.w=bg->w;
	pos.h=bg->h;
	middlex=(pos.w/2)+pos.x;
	middley=(bg->h/2)+pos.y;
}

CStatusBar::~CStatusBar()
{
	SDL_FreeSurface(bg);
}

void CStatusBar::clear()
{
	if(LOCPLINT->cingconsole->enteredText == "") //for appropriate support for in-game console
	{
		current="";
		redraw();
	}
}

void CStatusBar::print(const std::string & text)
{
	if(LOCPLINT->cingconsole->enteredText == "" || text == LOCPLINT->cingconsole->enteredText) //for appropriate support for in-game console
	{
		current=text;
		redraw();
	}
}

void CStatusBar::show(SDL_Surface * to)
{
	SDL_Rect srcRect = genRect(pos.h,pos.w,0,0);
	SDL_Rect dstRect = genRect(pos.h,pos.w,pos.x,pos.y);
	CSDL_Ext::blitSurface(bg,&srcRect,to,&dstRect);
	CSDL_Ext::printAtMiddle(current,middlex,middley,FONT_SMALL,Colors::Cornsilk,to);
}

std::string CStatusBar::getCurrent()
{
	return current;
}

void CList::activate()
{
	activateLClick();
	activateRClick();
	activateHover();
	activateKeys();
	activateMouseMove();
};

void CList::deactivate()
{
	deactivateLClick();
	deactivateRClick();
	deactivateHover();
	deactivateKeys();
	deactivateMouseMove();
};

void CList::clickLeft(tribool down, bool previousState)
{
};

CList::CList(int Size)
:SIZE(Size)
{
}

void CList::fixPos()
{
	if(selected < 0) //no selection, do nothing
		return;
	if(selected < from) //scroll up
		from = selected;
	else if(from + SIZE <= selected)
		from = selected - SIZE + 1; 

	vstd::amin(from, size() - SIZE);
	vstd::amax(from, 0);
	draw(screen);
}

void CHoverableArea::hover (bool on)
{
	if (on)
		GH.statusbar->print(hoverText);
	else if (GH.statusbar->getCurrent()==hoverText)
		GH.statusbar->clear();
}

CHoverableArea::CHoverableArea()
{
	used |= HOVER;
}

CHoverableArea::~CHoverableArea()
{
}

void LRClickableAreaWText::clickLeft(tribool down, bool previousState)
{
	if(!down && previousState)
	{
		LOCPLINT->showInfoDialog(text);
	}
}
void LRClickableAreaWText::clickRight(tribool down, bool previousState)
{
	adventureInt->handleRightClick(text, down);
}

LRClickableAreaWText::LRClickableAreaWText()
{
	init();
}

LRClickableAreaWText::LRClickableAreaWText(const Rect &Pos, const std::string &HoverText /*= ""*/, const std::string &ClickText /*= ""*/)
{
	init();
	pos = Pos + pos;
	hoverText = HoverText;
	text = ClickText;
}

LRClickableAreaWText::~LRClickableAreaWText()
{
}

void LRClickableAreaWText::init()
{
	used = LCLICK | RCLICK | HOVER;
}

void CLabel::showAll(SDL_Surface * to)
{
	CIntObject::showAll(to);
	std::string *hlpText = NULL;  //if NULL, text field will be used
	if(ignoreLeadingWhitespace)
	{
		hlpText = new std::string(text);
		boost::trim_left(*hlpText);
	}

	std::string &toPrint = hlpText ? *hlpText : text;
	if(!toPrint.length())
		return;

	static void (*printer[3])(const std::string &, int, int, EFonts, SDL_Color, SDL_Surface *) = {&CSDL_Ext::printAt, &CSDL_Ext::printAtMiddle, &CSDL_Ext::printTo}; //array of printing functions
	printer[alignment](toPrint, pos.x + textOffset.x, pos.y + textOffset.y, font, color, to);
}

CLabel::CLabel(int x, int y, EFonts Font /*= FONT_SMALL*/, EAlignment Align, const SDL_Color &Color /*= Colors::Cornsilk*/, const std::string &Text /*= ""*/)
:alignment(Align), font(Font), color(Color), text(Text)
{
	autoRedraw = true;
	pos.x += x;
	pos.y += y;
	pos.w = pos.h = 0;
	bg = NULL;
	ignoreLeadingWhitespace = false;

	pos.w = graphics->fonts[font]->getWidth(text.c_str());
	pos.h = graphics->fonts[font]->height;
}

void CLabel::setTxt(const std::string &Txt)
{
	text = Txt;
	if(autoRedraw)
	{
		if(bg || !parent)
			redraw();
		else
			parent->redraw();
	}
}

CLabelGroup::CLabelGroup(EFonts Font, EAlignment Align, const SDL_Color &Color):
	font(Font), align(Align), color(Color)
{};

void CLabelGroup::add(int x, int y, const std::string &text)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL;
	new CLabel(x, y, font, align, color, text);
};

CTextBox::CTextBox(std::string Text, const Rect &rect, int SliderStyle, EFonts Font /*= FONT_SMALL*/, EAlignment Align /*= TOPLEFT*/, const SDL_Color &Color /*= Colors::Cornsilk*/)
:CLabel(rect.x, rect.y, Font, Align, Color, Text), sliderStyle(SliderStyle), slider(NULL)
{
	type |= REDRAW_PARENT;
	autoRedraw = false;
	pos.h = rect.h;
	pos.w = rect.w;
	assert(Align == TOPLEFT || Align == CENTER); //TODO: support for other alignments
	assert(pos.w >= 80); //we need some space
	setTxt(Text);
}

void CTextBox::showAll(SDL_Surface * to)
{
	CIntObject::showAll(to);

	const Font &f = *graphics->fonts[font];
	int dy = f.height; //line height
	int base_y = pos.y;
	if(alignment == CENTER)
		base_y += std::max((pos.h - maxH)/2,0);

	int howManyLinesToPrint = slider ? slider->capacity : lines.size();
	int firstLineToPrint = slider ? slider->value : 0;

	for (int i = 0; i < howManyLinesToPrint; i++)
	{
		const std::string &line = lines[i + firstLineToPrint];
		if(!line.size()) continue;

		int x = pos.x;
		if(alignment == CENTER)
		{
			x += (pos.w - f.getWidth(line.c_str())) / 2;
			if(slider)
				x -= slider->pos.w / 2 + 5;
		}

		if(line[0] == '{' && line[line.size()-1] == '}')
			CSDL_Ext::printAt(line, x, base_y + i*dy, font, Colors::Jasmine, to);
		else
			CSDL_Ext::printAt(line, x, base_y + i*dy, font, color, to);
	}

}

void CTextBox::setTxt(const std::string &Txt)
{
	recalculateLines(Txt);
	CLabel::setTxt(Txt);
}

void CTextBox::sliderMoved(int to)
{
	if(!slider)
		return;

	redraw();
}

void CTextBox::setBounds(int limitW, int limitH)
{
	pos.h = limitH;
	pos.w = limitW;
	recalculateLines(text);
}

void CTextBox::recalculateLines(const std::string &Txt)
{
	delChildNUll(slider, true);
	lines.clear();

	const Font &f = *graphics->fonts[font];
	int lineHeight =  f.height; 
	int lineCapacity = pos.h / lineHeight;

	lines = CMessage::breakText(Txt, pos.w, font);
	if(lines.size() > lineCapacity) //we need to add a slider
	{
		lines = CMessage::breakText(Txt, pos.w - 32 - 10, font);
		OBJ_CONSTRUCTION_CAPTURING_ALL;
		slider = new CSlider(pos.w - 32, 0, pos.h, boost::bind(&CTextBox::sliderMoved, this, _1), lineCapacity, lines.size(), 0, false, sliderStyle);
		if(active)
			slider->activate();
	}

	maxH = lineHeight * lines.size();
	maxW = 0;
	BOOST_FOREACH(const std::string &line, lines)
		vstd::amax(maxW, f.getWidth(line.c_str()));
}

void CGStatusBar::print(const std::string & Text)
{
	setTxt(Text);
}

void CGStatusBar::clear()
{
	setTxt("");
}

std::string CGStatusBar::getCurrent()
{
	return text;
}

CGStatusBar::CGStatusBar(int x, int y, EFonts Font /*= FONT_SMALL*/, EAlignment Align, const SDL_Color &Color /*= Colors::Cornsilk*/, const std::string &Text /*= ""*/)
: CLabel(x, y, Font, Align, Color, Text)
{
	init();
}

CGStatusBar::CGStatusBar(CPicture *BG, EFonts Font /*= FONT_SMALL*/, EAlignment Align /*= CENTER*/, const SDL_Color &Color /*= Colors::Cornsilk*/)
: CLabel(BG->pos.x, BG->pos.y, Font, Align, Color, "")
{
	init();
	bg = BG;
	CGuiHandler::moveChild(bg, bg->parent, this);
	pos = bg->pos;
	calcOffset();
}

CGStatusBar::CGStatusBar(int x, int y, std::string name/*="ADROLLVR.bmp"*/, int maxw/*=-1*/)
: CLabel(x, y, FONT_SMALL, CENTER)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL;
	init();
	bg = new CPicture(name);
	pos = bg->pos;
	if(maxw < pos.w)
	{
		vstd::amin(pos.w, maxw);
		bg->srcRect = new Rect(0, 0, maxw, pos.h);
	}
	calcOffset();
}

CGStatusBar::~CGStatusBar()
{
	GH.statusbar = oldStatusBar;
}

void CGStatusBar::show(SDL_Surface * to)
{

}

void CGStatusBar::init()
{
	oldStatusBar = GH.statusbar;
	GH.statusbar = this;
}

void CGStatusBar::calcOffset()
{
	switch(alignment)
	{
	case CENTER:
		textOffset = Point(pos.w/2, pos.h/2);
		break;
	case BOTTOMRIGHT:
		textOffset = Point(pos.w, pos.h);
		break;
	}
}

CTextInput::CTextInput( const Rect &Pos, const Point &bgOffset, const std::string &bgName, const CFunctionList<void(const std::string &)> &CB )
:cb(CB)
{
	focus = false;
	pos += Pos;
	captureAllKeys = true;
	OBJ_CONSTRUCTION;
	bg = new CPicture(bgName, bgOffset.x, bgOffset.y);
	used = LCLICK | KEYBOARD;
	giveFocus();
}

CTextInput::CTextInput(const Rect &Pos, SDL_Surface *srf)
{
	focus = false;
	pos += Pos;
	captureAllKeys = true;
	OBJ_CONSTRUCTION;
	bg = new CPicture(Pos, 0, true);
	Rect hlp = Pos;
	if(srf)
		CSDL_Ext::blitSurface(srf, &hlp, *bg, NULL);
	else
		SDL_FillRect(*bg, NULL, 0);
	pos.w = bg->pos.w;
	pos.h = bg->pos.h;
	bg->pos = pos;
	used = LCLICK | KEYBOARD;
	giveFocus();
}

void CTextInput::showAll(SDL_Surface * to)
{
	CIntObject::showAll(to);
	const std::string toPrint = focus ? text + "_" : text;
	CSDL_Ext::printAt(toPrint, pos.x, pos.y, FONT_SMALL, Colors::Cornsilk, to);
}

void CTextInput::clickLeft( tribool down, bool previousState )
{
	if(down && !focus)
		giveFocus();
}

void CTextInput::keyPressed( const SDL_KeyboardEvent & key )
{
	if(!focus || key.state != SDL_PRESSED) 
		return;

	if(key.keysym.sym == SDLK_TAB)
	{
		moveFocus();
		GH.breakEventHandling();
		return;
	}

	switch(key.keysym.sym)
	{
	case SDLK_BACKSPACE:
		if(text.size())
			text.resize(text.size()-1);
		break;
	default:
		char c = key.keysym.unicode; //TODO 16-/>8
		static const std::string forbiddenChars = "<>:\"/\\|?*"; //if we are entering a filename, some special characters won't be allowed
		if(!vstd::contains(forbiddenChars,c) && std::isprint(c))
			text += c;
		break;
	}
	redraw();
	cb(text);
}

void CTextInput::setText( const std::string &nText, bool callCb )
{
	text = nText;
	redraw();
	if(callCb)
		cb(text);
}

CTextInput::~CTextInput()
{
}

CFocusable::CFocusable()
{
	focusables.push_back(this);
}

CFocusable::~CFocusable()
{
	if(inputWithFocus == this)
		inputWithFocus = NULL;

	focusables -= this;
}
void CFocusable::giveFocus()
{
	if(inputWithFocus)
	{
		inputWithFocus->focus = false;
		inputWithFocus->redraw();
	}

	focus = true;
	inputWithFocus = this;
	redraw();
}

void CFocusable::moveFocus()
{
	std::list<CFocusable*>::iterator i = vstd::find(focusables, this),
		ourIt = i;
	for(i++; i != ourIt; i++)
	{
		if(i == focusables.end())
			i = focusables.begin();

		if((*i)->active)
		{
			(*i)->giveFocus();
			break;;
		}
	}
}