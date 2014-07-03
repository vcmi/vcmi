#include "StdInc.h"
#include "CIntObjectClasses.h"

#include "../CBitmapHandler.h"
#include "SDL_Pixels.h"
#include "SDL_Extensions.h"
#include "../Graphics.h"
#include "../CAnimation.h"
#include "CCursorHandler.h"
#include "../CGameInfo.h"
#include "../../CCallback.h"
#include "../../lib/CConfigHandler.h"
#include "../battle/CBattleInterface.h"
#include "../battle/CBattleInterfaceClasses.h"
#include "../CPlayerInterface.h"
#include "../CMessage.h"
#include "../CMusicHandler.h"
#include "../GUIClasses.h"
#include "CGuiHandler.h"
#include "../CAdvmapInterface.h"
#include "../../lib/CGeneralTextHandler.h" //for Unicode related stuff

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

void CPicture::setSurface(SDL_Surface *to)
{
	bg = to;
	if (srcRect)
	{
		pos.w = srcRect->w;
		pos.h = srcRect->h;
	}
	else
	{
		pos.w = bg->w;
		pos.h = bg->h;
	}
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
	srcRect = nullptr;
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
	CSDL_Ext::setDefaultColorKey(bg);	
	SDL_FreeSurface(hlp);
}

void CPicture::setAlpha(int value)
{	
	#ifdef VCMI_SDL1
	SDL_SetAlpha(bg, SDL_SRCALPHA, value);	
	#else
	SDL_SetSurfaceAlphaMod(bg,value);
	#endif // 0
}

void CPicture::scaleTo(Point size)
{
	SDL_Surface * scaled = CSDL_Ext::scaleSurface(bg, size.x, size.y);

	if(freeSurf)
		SDL_FreeSurface(bg);

	setSurface(scaled);
	freeSurf = false;
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

	SDL_FillRect(bg, nullptr, color);
	freeSurf = true;
}

void CPicture::colorizeAndConvert(PlayerColor player)
{
	assert(bg);
	colorize(player);
	convertToScreenBPP();
}

void CPicture::colorize(PlayerColor player)
{
	assert(bg);
	graphics->blueToPlayersAdv(bg, player);
}

CFilledTexture::CFilledTexture(std::string imageName, Rect position):
    CIntObject(0, position.topLeft()),
    texture(BitmapHandler::loadBitmap(imageName))
{
	pos.w = position.w;
	pos.h = position.h;
}

CFilledTexture::~CFilledTexture()
{
	SDL_FreeSurface(texture);
}

void CFilledTexture::showAll(SDL_Surface *to)
{
	CSDL_Ext::CClipRectGuard guard(to, pos);
	CSDL_Ext::fillTexture(to, texture);
}

CButtonBase::CButtonBase()
{
	swappedImages = keepFrame = false;
	bitmapOffset = 0;
	state=NORMAL;
	image = nullptr;
	text = nullptr;
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
	delete text;
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
	CSDL_Ext::colorSetAlpha(borderColor,1);// represents a transparent color, used for HighlightableButton
	addUsedEvents(LCLICK | RCLICK | HOVER | KEYBOARD);
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

CAdventureMapButton::CAdventureMapButton( const std::pair<std::string, std::string> &help, const CFunctionList<void()> &Callback, int x, int y, const std::string &defName, int key/*=0*/, std::vector<std::string> * add /*= nullptr*/, bool playerColoredButton /*= false */ )
{
	std::map<int,std::string> pom;
	pom[0] = help.first;
	init(Callback, pom, help.second, playerColoredButton, defName, add, x, y, key);
}

void CAdventureMapButton::onButtonClicked()
{
	// debug logging to figure out pressed button (and as result - player actions) in case of crash
	logAnim->traceStream() << "Button clicked at " << pos.x << "x" << pos.y;
	CIntObject * parent = this->parent;
	std::string prefix = "Parent is";
	while (parent)
	{
		logAnim->traceStream() << prefix << typeid(*parent).name() << " at " << parent->pos.x << "x" << parent->pos.y;
		parent = parent->parent;
		prefix = '\t' + prefix;
	}
	callback();
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
		onButtonClicked();
	}
	else if (!actOnDown && previousState && (down==false))
	{
		onButtonClicked();
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
		: (vstd::contains(hoverTexts,0) ? (&hoverTexts[0]) : nullptr);
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
				GH.statusbar->setText(*name);
			else if ( GH.statusbar->getText()==(*name) )
				GH.statusbar->clear();
		}
	}
}

void CAdventureMapButton::init(const CFunctionList<void()> &Callback, const std::map<int,std::string> &Name, const std::string &HelpBox, bool playerColoredButton, const std::string &defName, std::vector<std::string> * add, int x, int y, int key)
{
	currentImage = -1;
	addUsedEvents(LCLICK | RCLICK | HOVER | KEYBOARD);
	callback = Callback;
	hoverable = actOnDown = borderEnabled = soundDisabled = false;
	CSDL_Ext::colorSetAlpha(borderColor,1);// represents a transparent color, used for HighlightableButton
	hoverTexts = Name;
	helpBox=HelpBox;

	if (key != SDLK_UNKNOWN)
		assignedKeys.insert(key);

	pos.x += x;
	pos.y += y;

	if (!defName.empty())
		imageNames.push_back(defName);
	if (add)
		for (auto & elem : *add)
			imageNames.push_back(elem);
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

	delete image;
	image = new CAnimImage(anim, getState(), 0, 0, 0, animFlags);
	if (playerColoredButton)
		image->playerColored(LOCPLINT->playerID);

	pos.w = image->pos.w;
	pos.h = image->pos.h;
}

void CAdventureMapButton::setPlayerColor(PlayerColor player)
{
	if (image)
		image->playerColored(player);
}

void CAdventureMapButton::showAll(SDL_Surface * to)
{
	CIntObject::showAll(to);
	
	#ifdef VCMI_SDL1
	if (borderEnabled && borderColor.unused == 0)
		CSDL_Ext::drawBorder(to, pos.x-1, pos.y-1, pos.w+2, pos.h+2, int3(borderColor.r, borderColor.g, borderColor.b));	
	#else
	if (borderEnabled && borderColor.a == 0)
		CSDL_Ext::drawBorder(to, pos.x-1, pos.y-1, pos.w+2, pos.h+2, int3(borderColor.r, borderColor.g, borderColor.b));	
	#endif // 0
}

void CHighlightableButton::select(bool on)
{
	selected = on;
	if (on)
	{
		borderEnabled = true;
		setState(HIGHLIGHTED);
		callback();
	}
	else
	{
		borderEnabled = false;
		setState(NORMAL);
		callback2();
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

CHighlightableButton::CHighlightableButton( const std::pair<std::string, std::string> &help, const CFunctionList<void()> &onSelect, int x, int y, const std::string &defName, int myid, int key/*=0*/, std::vector<std::string> * add /*= nullptr*/, bool playerColoredButton /*= false */ )
: onlyOn(false), selected(false) // TODO: callback2(???)
{
	ID = myid;
	std::map<int,std::string> pom;
	pom[0] = help.first;
	init(onSelect, pom, help.second, playerColoredButton, defName, add, x, y, key);
}

CHighlightableButton::CHighlightableButton( const std::string &Name, const std::string &HelpBox, const CFunctionList<void()> &onSelect, int x, int y, const std::string &defName, int myid, int key/*=0*/, std::vector<std::string> * add /*= nullptr*/, bool playerColoredButton /*= false */ )
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
	CHighlightableButton *bt = new CHighlightableButton(OnSelect, 0, tooltip, HelpBox, false, defName, nullptr, x, y, key);
	if(musicLike)
	{
		bt->setOffset(buttons.size()-3);
	}
	bt->ID = uid;
	bt->callback += boost::bind(&CHighlightableButtonsGroup::selectionChanged,this,bt->ID);
	bt->onlyOn = true;
	buttons.push_back(bt);
}

CHighlightableButtonsGroup::CHighlightableButtonsGroup(const CFunctionList<void(int)> &OnChange, bool musicLikeButtons)
: onChange(OnChange), musicLike(musicLikeButtons)
{}

CHighlightableButtonsGroup::~CHighlightableButtonsGroup()
{

}

void CHighlightableButtonsGroup::select(int id, bool mode)
{
	assert(!buttons.empty());

	CHighlightableButton *bt = buttons.front();
	if(mode)
	{
		for(auto btn : buttons)
			if (btn->ID == id)
				bt = btn;
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
	for(auto & elem : buttons)
		if(elem->ID!=to && elem->isHighlighted())
			elem->select(false);
	onChange(to);
	if (parent)
		parent->redraw();
}

void CHighlightableButtonsGroup::show(SDL_Surface * to)
{
	if (musicLike)
	{
		for(auto & elem : buttons)
			if(elem->isHighlighted())
				elem->show(to);
	}
	else
		CIntObject::show(to);
}

void CHighlightableButtonsGroup::showAll(SDL_Surface * to)
{
	if (musicLike)
	{
		for(auto & elem : buttons)
			if(elem->isHighlighted())
				elem->showAll(to);
	}
	else
		CIntObject::showAll(to);
}

void CHighlightableButtonsGroup::block( ui8 on )
{
	for(auto & elem : buttons)
	{
		elem->block(on);
	}
}

void CSlider::sliderClicked()
{
	if(!(active & MOVE))
		addUsedEvents(MOVE);
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
		removeUsedEvents(MOVE);
}

CSlider::~CSlider()
{

}

CSlider::CSlider(int x, int y, int totalw, std::function<void(int)> Moved, int Capacity, int Amount, int Value, bool Horizontal, int style):
    capacity(Capacity),
    amount(Amount),
    scrollStep(1),
    horizontal(Horizontal),
    moved(Moved)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL;
	setAmount(amount);

	addUsedEvents(LCLICK | KEYBOARD | WHEEL);
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
		//NOTE: this images do not have "blocked" frames. They should be implemented somehow (e.g. palette transform or something...)

		//use source def to create custom animations. Format "name.def:123" will load this frame from def file
		auto animLeft = new CAnimation();
		animLeft->setCustom(name + ":0", 0);
		animLeft->setCustom(name + ":1", 1);
		left->setImage(animLeft);

		auto animRight = new CAnimation();
		animRight->setCustom(name + ":2", 0);
		animRight->setCustom(name + ":3", 1);
		right->setImage(animRight);

		auto animSlider = new CAnimation();
		animSlider->setCustom(name + ":4", 0);
		slider->setImage(animSlider);
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

void CSlider::show(SDL_Surface * to)
{
	//todo: find better solution for background glitches.
	#ifdef VCMI_SDL1	
	CIntObject::show(to);	
	#else
	CSDL_Ext::fillRect(to, &pos, 0);
	CIntObject::showAll(to);	
	#endif // VCMI_SDL1
}


void CSlider::wheelScrolled(bool down, bool in)
{
	moveTo(value + 3 * (down ? +scrollStep : -scrollStep));
}

void CSlider::keyPressed(const SDL_KeyboardEvent & key)
{
	if(key.state != SDL_PRESSED) return;

	int moveDest = 0;
	switch(key.keysym.sym)
	{
	case SDLK_UP:
	case SDLK_LEFT:
		moveDest = value - scrollStep;
		break;
	case SDLK_DOWN:
	case SDLK_RIGHT:
		moveDest = value + scrollStep;
		break;
	case SDLK_PAGEUP:
		moveDest = value - capacity + scrollStep;
		break;
	case SDLK_PAGEDOWN:
		moveDest = value + capacity - scrollStep;
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
	removeChild(item);
	destroyObject(item);
}

CIntObject* CObjectList::createItem(size_t index)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL;
	CIntObject * item = createObject(index);
	if (item == nullptr)
		item = new CIntObject();

	item->recActions = defActions;

	addChild(item);
	return item;
}

CTabbedInt::CTabbedInt(CreateFunc create, DestroyFunc destroy, Point position, size_t ActiveID):
CObjectList(create, destroy),
activeTab(nullptr),
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
	itemOffset(ItemOffset),
    slider(nullptr)
{
	pos += Pos;
	items.resize(VisibleSize, nullptr);

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
	for (auto & elem : items)
	{
		(elem)->moveTo(itemPos);
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
	for (auto & elem : items)
	{
		deleteItem(elem);
		elem = createItem(current++);
	}
	updatePositions();
}

void CListBox::resize(size_t newSize)
{
	totalSize = newSize;
	if (slider)
		slider->setAmount(totalSize);
	reset();
}

size_t CListBox::size()
{
	return totalSize;
}

CIntObject * CListBox::getItem(size_t which)
{
	if (which < first || which > first + items.size() || which > totalSize)
		return nullptr;

	size_t i=first;
	for (auto iter = items.begin(); iter != items.end(); iter++, i++)
		if( i == which)
			return *iter;
	return nullptr;
}

size_t CListBox::getIndexOf(CIntObject *item)
{
	size_t i=first;
	for (auto iter = items.begin(); iter != items.end(); iter++, i++)
		if(*iter == item)
			return i;
	return size_t(-1);
}

void CListBox::scrollTo(size_t which)
{
	//scroll up
	if (first > which)
		moveToPos(which);
	//scroll down
	else if (first + items.size() <= which && which < totalSize)
		moveToPos(which - items.size() + 1);
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

size_t CListBox::getPos()
{
	return first;
}

const std::list<CIntObject *> &CListBox::getItems()
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
		bitmap=nullptr;
	}
}

void CHoverableArea::hover (bool on)
{
	if (on)
		GH.statusbar->setText(hoverText);
	else if (GH.statusbar->getText()==hoverText)
		GH.statusbar->clear();
}

CHoverableArea::CHoverableArea()
{
	addUsedEvents(HOVER);
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
	addUsedEvents(LCLICK | RCLICK | HOVER);
}

std::string CLabel::visibleText()
{
	return text;
}

void CLabel::showAll(SDL_Surface * to)
{
	CIntObject::showAll(to);

	if(!visibleText().empty())
		blitLine(to, pos, visibleText());

}

CLabel::CLabel(int x, int y, EFonts Font /*= FONT_SMALL*/, EAlignment Align, const SDL_Color &Color /*= Colors::WHITE*/, const std::string &Text /*= ""*/)
:CTextContainer(Align, Font, Color), text(Text)
{
	type |= REDRAW_PARENT;
	autoRedraw = true;
	pos.x += x;
	pos.y += y;
	pos.w = pos.h = 0;
	bg = nullptr;

	if (alignment == TOPLEFT) // causes issues for MIDDLE
	{
		pos.w = graphics->fonts[font]->getStringWidth(visibleText().c_str());
		pos.h = graphics->fonts[font]->getLineHeight();
	}
}

Point CLabel::getBorderSize()
{
	return Point(0, 0);
}

std::string CLabel::getText()
{
	return text;
}

void CLabel::setText(const std::string &Txt)
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

CMultiLineLabel::CMultiLineLabel(Rect position, EFonts Font, EAlignment Align, const SDL_Color &Color, const std::string &Text):
    CLabel(position.x, position.y, Font, Align, Color, Text),
    visibleSize(0, 0, position.w, position.h)
{
	pos.w = position.w;
	pos.h = position.h;
	splitText(Text);
}

void CMultiLineLabel::setVisibleSize(Rect visibleSize)
{
	this->visibleSize = visibleSize;
	redraw();
}

void CMultiLineLabel::scrollTextBy(int distance)
{
	scrollTextTo(visibleSize.y + distance);
}

void CMultiLineLabel::scrollTextTo(int distance)
{
	Rect size = visibleSize;
	size.y = distance;
	setVisibleSize(size);
}

void CMultiLineLabel::setText(const std::string &Txt)
{
	splitText(Txt);
	CLabel::setText(Txt);
}

void CTextContainer::blitLine(SDL_Surface *to, Rect destRect, std::string what)
{
	const IFont * f = graphics->fonts[font];
	Point where = destRect.topLeft();

	// input is rect in which given text should be placed
	// calculate proper position for top-left corner of the text
	if (alignment == TOPLEFT)
	{
		where.x += getBorderSize().x;
		where.y += getBorderSize().y;
	}

	if (alignment == CENTER)
	{
		where.x += (int(destRect.w) - int(f->getStringWidth(what))) / 2;
		where.y += (int(destRect.h) - int(f->getLineHeight())) / 2;
	}

	if (alignment == BOTTOMRIGHT)
	{
		where.x += getBorderSize().x + destRect.w - f->getStringWidth(what);
		where.y += getBorderSize().y + destRect.h - f->getLineHeight();
	}

	size_t begin = 0;
	std::string delimeters = "{}";
	size_t currDelimeter = 0;

	do
	{
		size_t end = what.find_first_of(delimeters[currDelimeter % 2], begin);
		if (begin != end)
		{
			std::string toPrint = what.substr(begin, end - begin);

			if (currDelimeter % 2) // Enclosed in {} text - set to yellow
				f->renderTextLeft(to, toPrint, Colors::YELLOW, where);
			else // Non-enclosed text, use default color
				f->renderTextLeft(to, toPrint, color, where);
			begin = end;

			where.x += f->getStringWidth(toPrint);
		}
		currDelimeter++;
	}
	while (begin++ != std::string::npos);
}

CTextContainer::CTextContainer(EAlignment alignment, EFonts font, SDL_Color color):
	alignment(alignment),
	font(font),
	color(color)
{}

void CMultiLineLabel::showAll(SDL_Surface * to)
{
	CIntObject::showAll(to);

	const IFont * f = graphics->fonts[font];

	// calculate which lines should be visible
	int totalLines = lines.size();
	int beginLine  = visibleSize.y;
	int endLine    = getTextLocation().h + visibleSize.y;

	if (beginLine < 0)
		beginLine = 0;
	else
		beginLine /= f->getLineHeight();

	if (endLine < 0)
		endLine = 0;
	else
		endLine /= f->getLineHeight();
	endLine++;

	// and where they should be displayed
	Point lineStart = getTextLocation().topLeft() - visibleSize + Point(0, beginLine * f->getLineHeight());
	Point lineSize  = Point(getTextLocation().w, f->getLineHeight());

	CSDL_Ext::CClipRectGuard guard(to, getTextLocation()); // to properly trim text that is too big to fit

	for (int i = beginLine; i < std::min(totalLines, endLine); i++)
	{
		if (!lines[i].empty()) //non-empty line
			blitLine(to, Rect(lineStart, lineSize), lines[i]);

		lineStart.y += f->getLineHeight();
	}
}

void CMultiLineLabel::splitText(const std::string &Txt)
{
	lines.clear();

	const IFont * f = graphics->fonts[font];
	int lineHeight =  f->getLineHeight();

	lines = CMessage::breakText(Txt, pos.w, font);

	 textSize.y = lineHeight * lines.size();
	 textSize.x = 0;
	for(const std::string &line : lines)
		vstd::amax( textSize.x, f->getStringWidth(line.c_str()));
	redraw();
}

Rect CMultiLineLabel::getTextLocation()
{
	// this method is needed for vertical alignment alignment of text
	// when height of available text is smaller than height of widget
	// in this case - we should add proper offset to display text at required position
	if (pos.h <= textSize.y)
		return pos;

	Point textSize(pos.w, graphics->fonts[font]->getLineHeight() * lines.size());
	Point textOffset(pos.w - textSize.x, pos.h - textSize.y);

	switch(alignment)
	{
	case TOPLEFT:     return Rect(pos.topLeft(), textSize);
	case CENTER:      return Rect(pos.topLeft() + textOffset / 2, textSize);
	case BOTTOMRIGHT: return Rect(pos.topLeft() + textOffset, textSize);
	}
	assert(0);
	return Rect();
}

CLabelGroup::CLabelGroup(EFonts Font, EAlignment Align, const SDL_Color &Color):
	font(Font), align(Align), color(Color)
{}

void CLabelGroup::add(int x, int y, const std::string &text)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL;
	new CLabel(x, y, font, align, color, text);
}

CTextBox::CTextBox(std::string Text, const Rect &rect, int SliderStyle, EFonts Font /*= FONT_SMALL*/, EAlignment Align /*= TOPLEFT*/, const SDL_Color &Color /*= Colors::WHITE*/):
    sliderStyle(SliderStyle),
    slider(nullptr)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL;
	label = new CMultiLineLabel(rect, Font, Align, Color);

	type |= REDRAW_PARENT;
	pos.x += rect.x;
	pos.y += rect.y;
	pos.h = rect.h;
	pos.w = rect.w;

	assert(pos.w >= 40); //we need some space
	setText(Text);
}

void CTextBox::sliderMoved(int to)
{
	label->scrollTextTo(to);
}

void CTextBox::resize(Point newSize)
{
	pos.w = newSize.x;
	pos.h = newSize.y;
	label->pos.w = pos.w;
	label->pos.h = pos.h;
	if (slider)
		vstd::clear_pointer(slider); // will be recreated if needed later

	setText(label->getText()); // force refresh
}

void CTextBox::setText(const std::string &text)
{
	label->setText(text);
	if (label->textSize.y <= label->pos.h && slider)
	{
		// slider is no longer needed
		vstd::clear_pointer(slider);
		label->pos.w = pos.w;
		label->setText(text);
	}
	else if (label->textSize.y > label->pos.h && !slider)
	{
		// create slider and update widget
		label->pos.w = pos.w - 32;
		label->setText(text);

		OBJ_CONSTRUCTION_CAPTURING_ALL;
		slider = new CSlider(pos.w - 32, 0, pos.h, boost::bind(&CTextBox::sliderMoved, this, _1),
		                     label->pos.h, label->textSize.y, 0, false, sliderStyle);
		slider->scrollStep = graphics->fonts[label->font]->getLineHeight();
	}
}

void CGStatusBar::setText(const std::string & Text)
{
	if(!textLock)
		CLabel::setText(Text);
}

void CGStatusBar::clear()
{
	setText("");
}

CGStatusBar::CGStatusBar(CPicture *BG, EFonts Font /*= FONT_SMALL*/, EAlignment Align /*= CENTER*/, const SDL_Color &Color /*= Colors::WHITE*/)
: CLabel(BG->pos.x, BG->pos.y, Font, Align, Color, "")
{
	init();
	bg = BG;
	addChild(bg);
	pos = bg->pos;
	getBorderSize();
    textLock = false;
}

CGStatusBar::CGStatusBar(int x, int y, std::string name/*="ADROLLVR.bmp"*/, int maxw/*=-1*/)
: CLabel(x, y, FONT_SMALL, CENTER)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL;
	init();
	bg = new CPicture(name);
	pos = bg->pos;
	if((unsigned int)maxw < pos.w)
	{
		vstd::amin(pos.w, maxw);
		bg->srcRect = new Rect(0, 0, maxw, pos.h);
	}
    textLock = false;
}

CGStatusBar::~CGStatusBar()
{
	GH.statusbar = oldStatusBar;
}

void CGStatusBar::show(SDL_Surface * to)
{
    showAll(to);
}

void CGStatusBar::init()
{
	oldStatusBar = GH.statusbar;
	GH.statusbar = this;
}

Point CGStatusBar::getBorderSize()
{
	//Width of borders where text should not be printed
	static const Point borderSize(5,1);

	switch(alignment)
	{
	case TOPLEFT:     return Point(borderSize.x, borderSize.y);
	case CENTER:      return Point(pos.w/2, pos.h/2);
	case BOTTOMRIGHT: return Point(pos.w - borderSize.x, pos.h - borderSize.y);
	}
	assert(0);
	return Point();
}

void CGStatusBar::lock(bool shouldLock)
{
    textLock = shouldLock;
}

CTextInput::CTextInput(const Rect &Pos, EFonts font, const CFunctionList<void(const std::string &)> &CB):
    CLabel(Pos.x, Pos.y, font, CENTER),
    cb(CB)
{
	type |= REDRAW_PARENT;
	focus = false;
	pos.h = Pos.h;
	pos.w = Pos.w;
	captureAllKeys = true;
	bg = nullptr;
	addUsedEvents(LCLICK | KEYBOARD | TEXTINPUT);
	giveFocus();
}

CTextInput::CTextInput( const Rect &Pos, const Point &bgOffset, const std::string &bgName, const CFunctionList<void(const std::string &)> &CB )
:cb(CB)
{
	focus = false;
	pos += Pos;
	captureAllKeys = true;
	OBJ_CONSTRUCTION;
	bg = new CPicture(bgName, bgOffset.x, bgOffset.y);
	addUsedEvents(LCLICK | KEYBOARD | TEXTINPUT);
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
		CSDL_Ext::blitSurface(srf, &hlp, *bg, nullptr);
	else
		SDL_FillRect(*bg, nullptr, 0);
	pos.w = bg->pos.w;
	pos.h = bg->pos.h;
	bg->pos = pos;
	addUsedEvents(LCLICK | KEYBOARD | TEXTINPUT);
	giveFocus();
}

void CTextInput::focusGot()
{
	CSDL_Ext::startTextInput(&pos);	
}

void CTextInput::focusLost()
{
	CSDL_Ext::stopTextInput();
}


std::string CTextInput::visibleText()
{
	return focus ? text + newText + "_" : text;
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

	bool redrawNeeded = false;
	#ifdef VCMI_SDL1
	std::string oldText = text;
	#endif // 0	
	switch(key.keysym.sym)
	{
	case SDLK_DELETE: // have index > ' ' so it won't be filtered out by default section
		return;
	case SDLK_BACKSPACE:
		if(!newText.empty())
		{
			Unicode::trimRight(newText);
			redrawNeeded = true;
		}
		else if(!text.empty())
		{
			Unicode::trimRight(text);
			redrawNeeded = true;
		}			
		break;
	default:
		#ifdef VCMI_SDL1
		if (key.keysym.unicode < ' ')
			return;
		else
		{
			text += key.keysym.unicode; //TODO 16-/>8
			redrawNeeded = true;
		}			
		#endif // 0
		break;
	}
	#ifdef VCMI_SDL1
	filters(text, oldText);
	#endif // 0
	if (redrawNeeded)
	{
		redraw();
		cb(text);
	}	
}

void CTextInput::setText( const std::string &nText, bool callCb )
{
	CLabel::setText(nText);
	if(callCb)
		cb(text);
}

bool CTextInput::captureThisEvent(const SDL_KeyboardEvent & key)
{
	if(key.keysym.sym == SDLK_RETURN || key.keysym.sym == SDLK_KP_ENTER)
		return false;
	
	#ifdef VCMI_SDL1
	//this should allow all non-printable keys to go through (for example arrows)
	if (key.keysym.unicode < ' ')
		return false;

	return true;
	#else
	return false;
	#endif
}

#ifndef VCMI_SDL1
void CTextInput::textInputed(const SDL_TextInputEvent & event)
{
	if(!focus)
		return;
	std::string oldText = text;
	
	text += event.text;	
	
	filters(text,oldText);
	if (text != oldText)
	{
		redraw();
		cb(text);
	}	
	newText = "";
}

void CTextInput::textEdited(const SDL_TextEditingEvent & event)
{
	if(!focus)
		return;
		
	newText = event.text;
	redraw();
	cb(text+newText);	
}

#endif


void CTextInput::filenameFilter(std::string & text, const std::string &)
{
	static const std::string forbiddenChars = "<>:\"/\\|?*\r\n"; //if we are entering a filename, some special characters won't be allowed
	size_t pos;
	while ((pos = text.find_first_of(forbiddenChars)) != std::string::npos)
		text.erase(pos, 1);
}

void CTextInput::numberFilter(std::string & text, const std::string & oldText, int minValue, int maxValue)
{
	assert(minValue < maxValue);

	if (text.empty())
		text = "0";

	size_t pos = 0;
	if (text[0] == '-') //allow '-' sign as first symbol only
		pos++;

	while (pos < text.size())
	{
		if (text[pos] < '0' || text[pos] > '9')
		{
			text = oldText;
			return; //new text is not number.
		}
		pos++;
	}
	try
	{
		int value = boost::lexical_cast<int>(text);
		if (value < minValue)
			text = boost::lexical_cast<std::string>(minValue);
		else if (value > maxValue)
			text = boost::lexical_cast<std::string>(maxValue);
	}
	catch(boost::bad_lexical_cast &)
	{
		//Should never happen. Unless I missed some cases
        logGlobal->warnStream() << "Warning: failed to convert "<< text << " to number!";
		text = oldText;
	}
}

CFocusable::CFocusable()
{
	focusables.push_back(this);
}

CFocusable::~CFocusable()
{
	if(inputWithFocus == this)
	{
		focusLost();
		inputWithFocus = nullptr;
	}	

	focusables -= this;
}
void CFocusable::giveFocus()
{
	if(inputWithFocus)
	{
		inputWithFocus->focus = false;
		inputWithFocus->focusLost();
		inputWithFocus->redraw();
	}

	focus = true;
	inputWithFocus = this;
	focusGot();
	redraw();	
}

void CFocusable::moveFocus()
{
	auto i = vstd::find(focusables, this),
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

CWindowObject::CWindowObject(int options_, std::string imageName, Point centerAt):
    CIntObject(getUsedEvents(options_), Point()),
    shadow(nullptr),
    options(options_),
    background(createBg(imageName, options & PLAYER_COLORED))
{
	assert(parent == nullptr); //Safe to remove, but windows should not have parent

	if (options & RCLICK_POPUP)
		CCS->curh->hide();

	if (background)
		pos = background->center(centerAt);
	else
		center(centerAt);

	if (!(options & SHADOW_DISABLED))
		setShadow(true);
}

CWindowObject::CWindowObject(int options_, std::string imageName):
    CIntObject(getUsedEvents(options_), Point()),
    shadow(nullptr),
    options(options_),
    background(createBg(imageName, options & PLAYER_COLORED))
{
	assert(parent == nullptr); //Safe to remove, but windows should not have parent

	if (options & RCLICK_POPUP)
		CCS->curh->hide();

	if (background)
		pos = background->center();
	else
		center(Point(screen->w/2, screen->h/2));

	if (!(options & SHADOW_DISABLED))
		setShadow(true);
}

CWindowObject::~CWindowObject()
{
	setShadow(false);
}

CPicture * CWindowObject::createBg(std::string imageName, bool playerColored)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL;

	if (imageName.empty())
		return nullptr;

	auto  image = new CPicture(imageName);
	if (playerColored)
		image->colorize(LOCPLINT->playerID);
	return image;
}

void CWindowObject::setBackground(std::string filename)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL;

	delete background;
	background = createBg(filename, options & PLAYER_COLORED);

	if (background)
		pos = background->center(Point(pos.w/2 + pos.x, pos.h/2 + pos.y));

	updateShadow();
}

int CWindowObject::getUsedEvents(int options)
{
	if (options & RCLICK_POPUP)
		return RCLICK;
	return 0;
}

void CWindowObject::updateShadow()
{
	setShadow(false);
	if (!(options & SHADOW_DISABLED))
		setShadow(true);
}

void CWindowObject::setShadow(bool on)
{
	//size of shadow
	static const int size = 8;

	if (on == bool(shadow))
		return;

	vstd::clear_pointer(shadow);

	//object too small to cast shadow
	if (pos.h <= size || pos.w <= size)
		return;

	if (on)
	{

		//helper to set last row
		auto blitAlphaRow = [](SDL_Surface *surf, size_t row)
		{
			Uint8 * ptr = (Uint8*)surf->pixels + surf->pitch * (row);

			for (size_t i=0; i< surf->w; i++)
			{
				Channels::px<4>::a.set(ptr, 128);
				ptr+=4;
			}
		};

		// helper to set last column
		auto blitAlphaCol = [](SDL_Surface *surf, size_t col)
		{
			Uint8 * ptr = (Uint8*)surf->pixels + 4 * (col);

			for (size_t i=0; i< surf->h; i++)
			{
				Channels::px<4>::a.set(ptr, 128);
				ptr+= surf->pitch;
			}
		};

		static SDL_Surface * shadowCornerTempl = nullptr;
		static SDL_Surface * shadowBottomTempl = nullptr;
		static SDL_Surface * shadowRightTempl = nullptr;

		//one-time initialization
		if (!shadowCornerTempl)
		{
			//create "template" surfaces
			shadowCornerTempl = CSDL_Ext::createSurfaceWithBpp<4>(size, size);
			shadowBottomTempl = CSDL_Ext::createSurfaceWithBpp<4>(1, size);
			shadowRightTempl  = CSDL_Ext::createSurfaceWithBpp<4>(size, 1);

			Uint32 shadowColor = SDL_MapRGBA(shadowCornerTempl->format, 0, 0, 0, 192);

			//fill with shadow body color
			SDL_FillRect(shadowCornerTempl, nullptr, shadowColor);
			SDL_FillRect(shadowBottomTempl, nullptr, shadowColor);
			SDL_FillRect(shadowRightTempl,  nullptr, shadowColor);

			//fill last row and column with more transparent color
			blitAlphaCol(shadowRightTempl , size-1);
			blitAlphaCol(shadowCornerTempl, size-1);
			blitAlphaRow(shadowBottomTempl, size-1);
			blitAlphaRow(shadowCornerTempl, size-1);
		}

		OBJ_CONSTRUCTION_CAPTURING_ALL;

		//FIXME: do something with this points
		Point shadowStart;
		if (options & BORDERED)
			shadowStart = Point(size - 14, size - 14);
		else
			shadowStart = Point(size, size);

		Point shadowPos;
		if (options & BORDERED)
			shadowPos = Point(pos.w + 14, pos.h + 14);
		else
			shadowPos = Point(pos.w, pos.h);

		Point fullsize;
		if (options & BORDERED)
			fullsize = Point(pos.w + 28, pos.h + 29);
		else
			fullsize = Point(pos.w, pos.h);

		//create base 8x8 piece of shadow
		SDL_Surface * shadowCorner = CSDL_Ext::copySurface(shadowCornerTempl);
		SDL_Surface * shadowBottom = CSDL_Ext::scaleSurfaceFast(shadowBottomTempl, fullsize.x - size, size);
		SDL_Surface * shadowRight  = CSDL_Ext::scaleSurfaceFast(shadowRightTempl,  size, fullsize.y - size);

		blitAlphaCol(shadowBottom, 0);
		blitAlphaRow(shadowRight, 0);

		//generate "shadow" object with these 3 pieces in it
		shadow = new CIntObject;
		shadow->addChild(new CPicture(shadowCorner, shadowPos.x, shadowPos.y));
		shadow->addChild(new CPicture(shadowRight,  shadowPos.x, shadowStart.y));
		shadow->addChild(new CPicture(shadowBottom, shadowStart.x, shadowPos.y));
	}
}

void CWindowObject::showAll(SDL_Surface *to)
{
	CIntObject::showAll(to);
	if ((options & BORDERED) && (pos.h != to->h || pos.w != to->w))
		CMessage::drawBorder(LOCPLINT ? LOCPLINT->playerID : PlayerColor(1), to, pos.w+28, pos.h+29, pos.x-14, pos.y-15);
}

void CWindowObject::close()
{
	GH.popIntTotally(this);
}

void CWindowObject::clickRight(tribool down, bool previousState)
{
	close();
	CCS->curh->show();
}
