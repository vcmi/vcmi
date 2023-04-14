/*
 * Buttons.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "Buttons.h"

#include "Images.h"
#include "TextControls.h"

#include "../CMusicHandler.h"
#include "../CGameInfo.h"
#include "../CPlayerInterface.h"
#include "../battle/BattleInterface.h"
#include "../battle/BattleInterfaceClasses.h"
#include "../gui/CGuiHandler.h"
#include "../windows/InfoWindows.h"
#include "../render/CAnimation.h"
#include "../renderSDL/SDL_Extensions.h"

#include "../../lib/CConfigHandler.h"
#include "../../lib/CGeneralTextHandler.h"

void CButton::update()
{
	if (overlay)
	{
		Point targetPos = Rect::createCentered( pos, overlay->pos.dimensions()).topLeft();

		if (state == PRESSED)
			overlay->moveTo(targetPos + Point(1,1));
		else
			overlay->moveTo(targetPos);
	}

	int newPos = stateToIndex[int(state)];
	if(animateLonelyFrame)
	{
		if(state == PRESSED)
			image->moveBy(Point(1,1));
		else
			image->moveBy(Point(-1,-1));
	}
	if (newPos < 0)
		newPos = 0;

	if (state == HIGHLIGHTED && image->size() < 4)
		newPos = (int)image->size()-1;
	image->setFrame(newPos);

	if (active)
		redraw();
}

void CButton::setBorderColor(boost::optional<SDL_Color> borderColor)
{
	setBorderColor(borderColor, borderColor, borderColor, borderColor);
}

void CButton::setBorderColor(boost::optional<SDL_Color> normalBorderColor,
                             boost::optional<SDL_Color> pressedBorderColor,
                             boost::optional<SDL_Color> blockedBorderColor,
                             boost::optional<SDL_Color> highlightedBorderColor)
{
	stateToBorderColor[NORMAL] = normalBorderColor;
	stateToBorderColor[PRESSED] = pressedBorderColor;
	stateToBorderColor[BLOCKED] = blockedBorderColor;
	stateToBorderColor[HIGHLIGHTED] = highlightedBorderColor;
	update();
}

void CButton::addCallback(std::function<void()> callback)
{
	this->callback += callback;
}

void CButton::addTextOverlay(const std::string & Text, EFonts font, SDL_Color color)
{
	OBJECT_CONSTRUCTION_CUSTOM_CAPTURING(255-DISPOSE);
	addOverlay(std::make_shared<CLabel>(pos.w/2, pos.h/2, font, ETextAlignment::CENTER, color, Text));
	update();
}

void CButton::addOverlay(std::shared_ptr<CIntObject> newOverlay)
{
	overlay = newOverlay;
	if(overlay)
	{
		addChild(newOverlay.get());
		Point targetPos = Rect::createCentered( pos, overlay->pos.dimensions()).topLeft();
		overlay->moveTo(targetPos);
	}
	update();
}

void CButton::addImage(std::string filename)
{
	imageNames.push_back(filename);
}

void CButton::addHoverText(ButtonState state, std::string text)
{
	hoverTexts[state] = text;
}

void CButton::setImageOrder(int state1, int state2, int state3, int state4)
{
	stateToIndex[0] = state1;
	stateToIndex[1] = state2;
	stateToIndex[2] = state3;
	stateToIndex[3] = state4;
	update();
}

void CButton::setAnimateLonelyFrame(bool agreement)
{
	animateLonelyFrame = agreement;
}
void CButton::setState(ButtonState newState)
{
	if (state == newState)
		return;
	state = newState;
	update();
}

CButton::ButtonState CButton::getState()
{
	return state;
}

bool CButton::isBlocked()
{
	return state == BLOCKED;
}

bool CButton::isHighlighted()
{
	return state == HIGHLIGHTED;
}

void CButton::block(bool on)
{
	if(on || state == BLOCKED) //dont change button state if unblock requested, but it's not blocked
		setState(on ? BLOCKED : NORMAL);
}

void CButton::onButtonClicked()
{
	// debug logging to figure out pressed button (and as result - player actions) in case of crash
	logAnim->trace("Button clicked at %dx%d", pos.x, pos.y);
	CIntObject * parent = this->parent;
	std::string prefix = "Parent is";
	while (parent)
	{
		logAnim->trace("%s%s at %dx%d", prefix, typeid(*parent).name(), parent->pos.x, parent->pos.y);
		parent = parent->parent;
		prefix = '\t' + prefix;
	}
	callback();
}

void CButton::clickLeft(tribool down, bool previousState)
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

void CButton::clickRight(tribool down, bool previousState)
{
	if(down && helpBox.size()) //there is no point to show window with nothing inside...
		CRClickPopup::createAndPush(helpBox);
}

void CButton::hover (bool on)
{
	if(hoverable && !isBlocked())
	{
		if(on)
			setState(HIGHLIGHTED);
		else
			setState(NORMAL);
	}

	/*if(pressedL && on) // WTF is this? When this is used?
		setState(PRESSED);*/

	std::string name = hoverTexts[getState()].empty()
		? hoverTexts[0]
		: hoverTexts[getState()];

	if(!name.empty() && !isBlocked()) //if there is no name, there is nothing to display also
	{
		if (on)
			GH.statusbar->write(name);
		else
			GH.statusbar->clearIfMatching(name);
	}
}

CButton::CButton(Point position, const std::string &defName, const std::pair<std::string, std::string> &help, CFunctionList<void()> Callback, int key, bool playerColoredButton):
    CKeyShortcut(key),
    callback(Callback)
{
	defActions = 255-DISPOSE;
	addUsedEvents(LCLICK | RCLICK | HOVER | KEYBOARD);

	stateToIndex[0] = 0;
	stateToIndex[1] = 1;
	stateToIndex[2] = 2;
	stateToIndex[3] = 3;

	state=NORMAL;

	currentImage = -1;
	hoverable = actOnDown = soundDisabled = false;
	hoverTexts[0] = help.first;
	helpBox=help.second;

	pos.x += position.x;
	pos.y += position.y;

	if (!defName.empty())
	{
		imageNames.push_back(defName);
		setIndex(0, playerColoredButton);
	}
}

void CButton::setIndex(size_t index, bool playerColoredButton)
{
	if (index == currentImage || index>=imageNames.size())
		return;
	currentImage = index;
	auto anim = std::make_shared<CAnimation>(imageNames[index]);
	setImage(anim, playerColoredButton);
}

void CButton::setImage(std::shared_ptr<CAnimation> anim, bool playerColoredButton, int animFlags)
{
	OBJECT_CONSTRUCTION_CUSTOM_CAPTURING(255-DISPOSE);

	image = std::make_shared<CAnimImage>(anim, getState(), 0, 0, 0, animFlags);
	if (playerColoredButton)
		image->playerColored(LOCPLINT->playerID);
	pos = image->pos;
}

void CButton::setPlayerColor(PlayerColor player)
{
	if (image)
		image->playerColored(player);
}

void CButton::showAll(SDL_Surface * to)
{
	CIntObject::showAll(to);

	auto borderColor = stateToBorderColor[getState()];
	if (borderColor)
		CSDL_Ext::drawBorder(to, pos.x-1, pos.y-1, pos.w+2, pos.h+2, *borderColor);
}

std::pair<std::string, std::string> CButton::tooltip()
{
	return std::pair<std::string, std::string>();
}

std::pair<std::string, std::string> CButton::tooltipLocalized(const std::string & key)
{
	return std::make_pair(
		CGI->generaltexth->translate(key, "hover"),
		CGI->generaltexth->translate(key, "help")
	);
}

std::pair<std::string, std::string> CButton::tooltip(const std::string & hover, const std::string & help)
{
	return std::make_pair(hover, help);
}

CToggleBase::CToggleBase(CFunctionList<void (bool)> callback):
    callback(callback),
    selected(false),
    allowDeselection(true)
{
}

CToggleBase::~CToggleBase() = default;

void CToggleBase::doSelect(bool on)
{
	// for overrides
}

void CToggleBase::setEnabled(bool enabled)
{
	// for overrides
}

void CToggleBase::setSelected(bool on)
{
	bool changed = (on != selected);
	selected = on;
	doSelect(on);
	if (changed)
		callback(on);
}

bool CToggleBase::canActivate()
{
	if (selected && !allowDeselection)
		return false;
	return true;
}

void CToggleBase::addCallback(std::function<void(bool)> function)
{
	callback += function;
}

CToggleButton::CToggleButton(Point position, const std::string &defName, const std::pair<std::string, std::string> &help,
                             CFunctionList<void(bool)> callback, int key, bool playerColoredButton):
  CButton(position, defName, help, 0, key, playerColoredButton),
  CToggleBase(callback)
{
	allowDeselection = true;
}

void CToggleButton::doSelect(bool on)
{
	if (on)
	{
		setState(HIGHLIGHTED);
	}
	else
	{
		setState(NORMAL);
	}
}

void CToggleButton::setEnabled(bool enabled)
{
	setState(enabled ? NORMAL : BLOCKED);
}

void CToggleButton::clickLeft(tribool down, bool previousState)
{
	// force refresh
	hover(false);
	hover(true);

	if(isBlocked())
		return;

	if (down && canActivate())
	{
		CCS->soundh->playSound(soundBase::button);
		setState(PRESSED);
	}

	if(previousState)//mouse up
	{
		if(down == false && getState() == PRESSED && canActivate())
		{
			onButtonClicked();
			setSelected(!selected);
		}
		else
			doSelect(selected); // restore
	}
}

void CToggleGroup::addCallback(std::function<void(int)> callback)
{
	onChange += callback;
}

void CToggleGroup::resetCallback()
{
	onChange.clear();
}

void CToggleGroup::addToggle(int identifier, std::shared_ptr<CToggleBase> button)
{
	if(auto intObj = std::dynamic_pointer_cast<CIntObject>(button)) // hack-ish workagound to avoid diamond problem with inheritance
	{
		addChild(intObj.get());
	}

	button->addCallback([=] (bool on) { if (on) selectionChanged(identifier);});
	button->allowDeselection = false;

	if(buttons.count(identifier)>0)
		logAnim->error("Duplicated toggle button id %d", identifier);
	buttons[identifier] = button;
}

CToggleGroup::CToggleGroup(const CFunctionList<void(int)> &OnChange)
	: onChange(OnChange), selectedID(-2)
{
}

void CToggleGroup::setSelected(int id)
{
	selectionChanged(id);
}

void CToggleGroup::setSelectedOnly(int id)
{
	for(auto it = buttons.begin(); it != buttons.end(); it++)
	{
		int buttonId = it->first;
		buttons[buttonId]->setEnabled(buttonId == id);
	}

	selectionChanged(id);
}

void CToggleGroup::selectionChanged(int to)
{
	if (to == selectedID)
		return;

	int oldSelection = selectedID;
	selectedID = to;

	if (buttons.count(oldSelection))
		buttons[oldSelection]->setSelected(false);

	if (buttons.count(to))
		buttons[to]->setSelected(true);

	onChange(to);
	redraw();
}

int CToggleGroup::getSelected() const
{
	return selectedID;
}

CVolumeSlider::CVolumeSlider(const Point & position, const std::string & defName, const int value, ETooltipMode mode)
	: CIntObject(LCLICK | RCLICK | WHEEL),
	value(value),
	mode(mode)
{
	OBJECT_CONSTRUCTION_CAPTURING(255-DISPOSE);
	animImage = std::make_shared<CAnimImage>(std::make_shared<CAnimation>(defName), 0, 0, position.x, position.y),
	pos.x += position.x;
	pos.y += position.y;
	pos.w = (animImage->pos.w + 1) * (int)animImage->size();
	pos.h = animImage->pos.h;
	type |= REDRAW_PARENT;
	setVolume(value);
}

void CVolumeSlider::setVolume(int value_)
{
	value = value_;
	moveTo((int)(value * static_cast<double>(animImage->size()) / 100.0));
}

void CVolumeSlider::moveTo(int id)
{
	vstd::abetween(id, 0, animImage->size() - 1);
	animImage->setFrame(id);
	animImage->moveTo(Point(pos.x + (animImage->pos.w + 1) * id, pos.y));
	if (active)
		redraw();
}

void CVolumeSlider::addCallback(std::function<void(int)> callback)
{
	onChange += callback;
}

void CVolumeSlider::clickLeft(tribool down, bool previousState)
{
	if (down)
	{
		double px = GH.getCursorPosition().x - pos.x;
		double rx = px / static_cast<double>(pos.w);
		// setVolume is out of 100
		setVolume((int)(rx * 100));
		// Volume config is out of 100, set to increments of 5ish roughly based on the half point of the indicator
		// 0.0 -> 0, 0.05 -> 5, 0.09 -> 5,...,
		// 0.1 -> 10, ..., 0.19 -> 15, 0.2 -> 20, ...,
		// 0.28 -> 25, 0.29 -> 30, 0.3 -> 30, ...,
		// 0.85 -> 85, 0.86 -> 90, ..., 0.87 -> 90,...,
		// 0.95 -> 95, 0.96 -> 100, 0.99 -> 100
		int volume = 5 * int(rx * (2 * animImage->size() + 1));
		onChange(volume);
	}
}

void CVolumeSlider::clickRight(tribool down, bool previousState)
{
	if (down)
	{
		double px = GH.getCursorPosition().x - pos.x;
		int index = static_cast<int>(px / static_cast<double>(pos.w) * animImage->size());

		size_t helpIndex = index + (mode == MUSIC ? 326 : 336);
		std::string helpBox = CGI->generaltexth->translate("core.help", helpIndex, "help" );

		if(!helpBox.empty())
			CRClickPopup::createAndPush(helpBox);
		if(GH.statusbar)
			GH.statusbar->write(helpBox);
	}
}

void CVolumeSlider::wheelScrolled(bool down, bool in)
{
	if (in)
	{
		int volume = value + 3 * (down ? 1 : -1);
		vstd::abetween(volume, 0, 100);
		setVolume(volume);
		onChange(volume);
	}
}

void CSlider::sliderClicked()
{
	if(!(active & MOVE))
		addUsedEvents(MOVE);
}

void CSlider::mouseMoved (const Point & cursorPosition)
{
	double v = 0;
	if(horizontal)
	{
		if(	std::abs(cursorPosition.y-(pos.y+pos.h/2)) > pos.h/2+40  ||  std::abs(cursorPosition.x-(pos.x+pos.w/2)) > pos.w/2  )
			return;
		v = cursorPosition.x - pos.x - 24;
		v *= positions;
		v /= (pos.w - 48);
	}
	else
	{
		if(std::abs(cursorPosition.x-(pos.x+pos.w/2)) > pos.w/2+40  ||  std::abs(cursorPosition.y-(pos.y+pos.h/2)) > pos.h/2  )
			return;
		v = cursorPosition.y - pos.y - 24;
		v *= positions;
		v /= (pos.h - 48);
	}
	v += 0.5;
	if(v!=value)
	{
		moveTo(static_cast<int>(v));
	}
}

void CSlider::setScrollStep(int to)
{
	scrollStep = to;
}

void CSlider::setScrollBounds(const Rect & bounds )
{
	scrollBounds = bounds;
}

void CSlider::clearScrollBounds()
{
	scrollBounds = boost::none;
}

int CSlider::getAmount() const
{
	return amount;
}

int CSlider::getValue() const
{
	return value;
}

int CSlider::getCapacity() const
{
	return capacity;
}

void CSlider::moveLeft()
{
	moveTo(value-1);
}

void CSlider::moveRight()
{
	moveTo(value+1);
}

void CSlider::moveBy(int amount)
{
	moveTo(value + amount);
}

void CSlider::updateSliderPos()
{
	if(horizontal)
	{
		if(positions)
		{
			double part = static_cast<double>(value) / positions;
			part*=(pos.w-48);
			int newPos = static_cast<int>(part + pos.x + 16 - slider->pos.x);
			slider->moveBy(Point(newPos, 0));
		}
		else
			slider->moveTo(Point(pos.x+16, pos.y));
	}
	else
	{
		if(positions)
		{
			double part = static_cast<double>(value) / positions;
			part*=(pos.h-48);
			int newPos = static_cast<int>(part + pos.y + 16 - slider->pos.y);
			slider->moveBy(Point(0, newPos));
		}
		else
			slider->moveTo(Point(pos.x, pos.y+16));
	}
}

void CSlider::moveTo(int to)
{
	vstd::amax(to, 0);
	vstd::amin(to, positions);

	//same, old position?
	if(value == to)
		return;
	value = to;

	updateSliderPos();

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
			pw = GH.getCursorPosition().x-pos.x-25;
			rw = pw / static_cast<double>(pos.w - 48);
		}
		else
		{
			pw = GH.getCursorPosition().y-pos.y-24;
			rw = pw / (pos.h-48);
		}
		if(pw < -8  ||  pw > (horizontal ? pos.w : pos.h) - 40)
			return;
		// 		if (rw>1) return;
		// 		if (rw<0) return;
		slider->clickLeft(true, slider->mouseState(MouseButton::LEFT));
		moveTo((int)(rw * positions  +  0.5));
		return;
	}
	if(active & MOVE)
		removeUsedEvents(MOVE);
}

CSlider::CSlider(Point position, int totalw, std::function<void(int)> Moved, int Capacity, int Amount, int Value, bool Horizontal, CSlider::EStyle style)
	: CIntObject(LCLICK | RCLICK | WHEEL),
	capacity(Capacity),
	horizontal(Horizontal),
	amount(Amount),
	value(Value),
	scrollStep(1),
	moved(Moved)
{
	OBJECT_CONSTRUCTION_CAPTURING(255-DISPOSE);
	setAmount(amount);
	vstd::amax(value, 0);
	vstd::amin(value, positions);

	strongInterest = true;

	pos.x += position.x;
	pos.y += position.y;

	if(style == BROWN)
	{
		std::string name = horizontal ? "IGPCRDIV.DEF" : "OVBUTN2.DEF";
		//NOTE: this images do not have "blocked" frames. They should be implemented somehow (e.g. palette transform or something...)

		left = std::make_shared<CButton>(Point(), name, CButton::tooltip());
		right = std::make_shared<CButton>(Point(), name, CButton::tooltip());
		slider = std::make_shared<CButton>(Point(), name, CButton::tooltip());

		left->setImageOrder(0, 1, 1, 1);
		right->setImageOrder(2, 3, 3, 3);
		slider->setImageOrder(4, 4, 4, 4);
	}
	else
	{
		left = std::make_shared<CButton>(Point(), horizontal ? "SCNRBLF.DEF" : "SCNRBUP.DEF", CButton::tooltip());
		right = std::make_shared<CButton>(Point(), horizontal ? "SCNRBRT.DEF" : "SCNRBDN.DEF", CButton::tooltip());
		slider = std::make_shared<CButton>(Point(), "SCNRBSL.DEF", CButton::tooltip());
	}
	slider->actOnDown = true;
	slider->soundDisabled = true;
	left->soundDisabled = true;
	right->soundDisabled = true;

	if (horizontal)
		right->moveBy(Point(totalw - right->pos.w, 0));
	else
		right->moveBy(Point(0, totalw - right->pos.h));

	left->addCallback(std::bind(&CSlider::moveLeft,this));
	right->addCallback(std::bind(&CSlider::moveRight,this));
	slider->addCallback(std::bind(&CSlider::sliderClicked,this));

	if(horizontal)
	{
		pos.h = slider->pos.h;
		pos.w = totalw;
	}
	else
	{
		pos.w = slider->pos.w;
		pos.h = totalw;
	}

	updateSliderPos();
}

CSlider::~CSlider() = default;

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
	CSDL_Ext::fillRect(to, pos, Colors::BLACK);
	CIntObject::showAll(to);
}

void CSlider::wheelScrolled(bool down, bool in)
{
	if (scrollBounds)
	{
		Rect testTarget = *scrollBounds + pos.topLeft();

		if (!testTarget.isInside(GH.getCursorPosition()))
			return;
	}

	// vertical slider -> scrolling up move slider upwards
	// horizontal slider -> scrolling up moves slider towards right
	bool positive = (down != horizontal);

	moveTo(value + 3 * (positive ? +scrollStep : -scrollStep));
}

void CSlider::keyPressed(const SDL_Keycode & key)
{
	int moveDest = value;
	switch(key)
	{
	case SDLK_UP:
		if (!horizontal)
			moveDest = value - scrollStep;
		break;
	case SDLK_LEFT:
		if (horizontal)
			moveDest = value - scrollStep;
		break;
	case SDLK_DOWN:
		if (!horizontal)
			moveDest = value + scrollStep;
		break;
	case SDLK_RIGHT:
		if (horizontal)
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

void CSlider::moveToMin()
{
	moveTo(0);
}

void CSlider::moveToMax()
{
	moveTo(amount);
}
