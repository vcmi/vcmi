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
#include "../battle/CBattleInterface.h"
#include "../battle/CBattleInterfaceClasses.h"
#include "../gui/CAnimation.h"
#include "../gui/CGuiHandler.h"
#include "../windows/InfoWindows.h"
#include "../../lib/CConfigHandler.h"

void CButton::update()
{
	if(overlay)
	{
		if(state == PRESSED)
			overlay->moveTo(overlay->pos.centerIn(pos).topLeft() + Point(1, 1));
		else
			overlay->moveTo(overlay->pos.centerIn(pos).topLeft());
	}

	int newPos = stateToIndex[int(state)];
	if(newPos < 0)
		newPos = 0;

	if(state == HIGHLIGHTED && image->size() < 4)
		newPos = image->size() - 1;
	image->setFrame(newPos);

	if(active)
		redraw();
}

void CButton::addCallback(std::function<void()> callback)
{
	this->callback += callback;
}

void CButton::addTextOverlay(const std::string & Text, EFonts font, SDL_Color color)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL;
	addOverlay(new CLabel(pos.w / 2, pos.h / 2, font, CENTER, color, Text));
	update();
}

void CButton::addOverlay(CIntObject * newOverlay)
{
	delete overlay;
	overlay = newOverlay;
	addChild(newOverlay);
	overlay->moveTo(overlay->pos.centerIn(pos).topLeft());
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

void CButton::setState(ButtonState newState)
{
	if(state == newState)
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
	logAnim->traceStream() << "Button clicked at " << pos.x << "x" << pos.y;
	CIntObject * parent = this->parent;
	std::string prefix = "Parent is";
	while(parent)
	{
		logAnim->traceStream() << prefix << typeid(*parent).name() << " at " << parent->pos.x << "x" << parent->pos.y;
		parent = parent->parent;
		prefix = '\t' + prefix;
	}
	callback();
}

void CButton::clickLeft(tribool down, bool previousState)
{
	if(isBlocked())
		return;

	if(down)
	{
		if(!soundDisabled)
			CCS->soundh->playSound(soundBase::button);
		setState(PRESSED);
	}
	else if(hoverable && hovered)
		setState(HIGHLIGHTED);
	else
		setState(NORMAL);

	if(actOnDown && down)
	{
		onButtonClicked();
	}
	else if(!actOnDown && previousState && (down == false))
	{
		onButtonClicked();
	}
}

void CButton::clickRight(tribool down, bool previousState)
{
	if(down && helpBox.size()) //there is no point to show window with nothing inside...
		CRClickPopup::createAndPush(helpBox);
}

void CButton::hover(bool on)
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

	std::string name = hoverTexts[getState()].empty() ? hoverTexts[0] : hoverTexts[getState()];

	if(!name.empty() && !isBlocked()) //if there is no name, there is nothing to display also
	{
		if(LOCPLINT && LOCPLINT->battleInt) //for battle buttons
		{
			if(on && LOCPLINT->battleInt->console->alterTxt == "")
			{
				LOCPLINT->battleInt->console->alterTxt = name;
				LOCPLINT->battleInt->console->whoSetAlter = 1;
			}
			else if(LOCPLINT->battleInt->console->alterTxt == name)
			{
				LOCPLINT->battleInt->console->alterTxt = "";
				LOCPLINT->battleInt->console->whoSetAlter = 0;
			}
		}
		else if(GH.statusbar) //for other buttons
		{
			if(on)
				GH.statusbar->setText(name);
			else if(GH.statusbar->getText() == (name))
				GH.statusbar->clear();
		}
	}
}

CButton::CButton(Point position, const std::string & defName, const std::pair<std::string, std::string> & help, CFunctionList<void()> Callback, int key, bool playerColoredButton)
	: CKeyShortcut(key), callback(Callback)
{
	addUsedEvents(LCLICK | RCLICK | HOVER | KEYBOARD);

	stateToIndex[0] = 0;
	stateToIndex[1] = 1;
	stateToIndex[2] = 2;
	stateToIndex[3] = 3;

	state = NORMAL;
	image = nullptr;
	overlay = nullptr;

	currentImage = -1;
	hoverable = actOnDown = soundDisabled = false;
	hoverTexts[0] = help.first;
	helpBox = help.second;

	pos.x += position.x;
	pos.y += position.y;

	if(!defName.empty())
	{
		imageNames.push_back(defName);
		setIndex(0, playerColoredButton);
	}
}

void CButton::setIndex(size_t index, bool playerColoredButton)
{
	if(index == currentImage || index >= imageNames.size())
		return;
	currentImage = index;
	auto anim = std::make_shared<CAnimation>(imageNames[index]);
	setImage(anim, playerColoredButton);
}

void CButton::setImage(std::shared_ptr<CAnimation> anim, bool playerColoredButton, int animFlags)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL;

	image = new CAnimImage(anim, getState(), 0, 0, 0, animFlags);
	if(playerColoredButton)
		image->playerColored(LOCPLINT->playerID);
	pos = image->pos;
}

void CButton::setPlayerColor(PlayerColor player)
{
	if(image)
		image->playerColored(player);
}

void CButton::showAll(SDL_Surface * to)
{
	CIntObject::showAll(to);

	if(borderColor && borderColor->a == 0)
		CSDL_Ext::drawBorder(to, pos.x - 1, pos.y - 1, pos.w + 2, pos.h + 2, int3(borderColor->r, borderColor->g, borderColor->b));
}

std::pair<std::string, std::string> CButton::tooltip()
{
	return std::pair<std::string, std::string>();
}

std::pair<std::string, std::string> CButton::tooltip(const JsonNode & localizedTexts)
{
	return std::make_pair(localizedTexts["label"].String(), localizedTexts["help"].String());
}

std::pair<std::string, std::string> CButton::tooltip(const std::string & hover, const std::string & help)
{
	return std::make_pair(hover, help);
}

CToggleBase::CToggleBase(CFunctionList<void(bool)> callback)
	: callback(callback), selected(false), allowDeselection(true)
{
}

CToggleBase::~CToggleBase()
{
}

void CToggleBase::doSelect(bool on)
{
	// for overrides
}

void CToggleBase::setSelected(bool on)
{
	bool changed = (on != selected);
	selected = on;
	doSelect(on);
	if(changed)
		callback(on);
}

bool CToggleBase::canActivate()
{
	if(selected && !allowDeselection)
		return false;
	return true;
}

void CToggleBase::addCallback(std::function<void(bool)> function)
{
	callback += function;
}

CToggleButton::CToggleButton(Point position, const std::string & defName, const std::pair<std::string, std::string> & help, CFunctionList<void(bool)> callback, int key, bool playerColoredButton)
	: CButton(position, defName, help, 0, key, playerColoredButton), CToggleBase(callback)
{
	allowDeselection = true;
}

void CToggleButton::doSelect(bool on)
{
	if(on)
	{
		setState(HIGHLIGHTED);
	}
	else
	{
		setState(NORMAL);
	}
}

void CToggleButton::clickLeft(tribool down, bool previousState)
{
	// force refresh
	hover(false);
	hover(true);

	if(isBlocked())
		return;

	if(down && canActivate())
	{
		CCS->soundh->playSound(soundBase::button);
		setState(PRESSED);
	}

	if(previousState) //mouse up
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

void CToggleGroup::addToggle(int identifier, CToggleBase * bt)
{
	if(auto intObj = dynamic_cast<CIntObject *>(bt)) // hack-ish workagound to avoid diamond problem with inheritance
	{
		if(intObj->parent)
			intObj->parent->removeChild(intObj);
		addChild(intObj);
	}

	bt->addCallback([=](bool on)
	{
		if(on)
			selectionChanged(identifier);
	});
	bt->allowDeselection = false;

	assert(buttons[identifier] == nullptr);
	buttons[identifier] = bt;
}

CToggleGroup::CToggleGroup(const CFunctionList<void(int)> & OnChange)
	: onChange(OnChange), selectedID(-2)
{}

void CToggleGroup::setSelected(int id)
{
	selectionChanged(id);
}

void CToggleGroup::selectionChanged(int to)
{
	if(to == selectedID)
		return;

	int oldSelection = selectedID;
	selectedID = to;

	if(buttons.count(oldSelection))
		buttons[oldSelection]->setSelected(false);

	if(buttons.count(to))
		buttons[to]->setSelected(true);

	onChange(to);
	if(parent)
		parent->redraw();
}

CVolumeSlider::CVolumeSlider(const Point & position, const std::string & defName, const int value, const std::pair<std::string, std::string> * const help)
	: value(value), helpHandlers(help)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL;
	animImage = new CAnimImage(std::make_shared<CAnimation>(defName), 0, 0, position.x, position.y),
	assert(!defName.empty());
	addUsedEvents(LCLICK | RCLICK | WHEEL);
	pos.x += position.x;
	pos.y += position.y;
	pos.w = (animImage->pos.w + 1) * animImage->size();
	pos.h = animImage->pos.h;
	type |= REDRAW_PARENT;
	setVolume(value);
}

void CVolumeSlider::setVolume(int value_)
{
	value = value_;
	moveTo(value * static_cast<double>(animImage->size()) / 100.0);
}

void CVolumeSlider::moveTo(int id)
{
	vstd::abetween(id, 0, animImage->size() - 1);
	animImage->setFrame(id);
	animImage->moveTo(Point(pos.x + (animImage->pos.w + 1) * id, pos.y));
	if(active)
		redraw();
}

void CVolumeSlider::addCallback(std::function<void(int)> callback)
{
	onChange += callback;
}

void CVolumeSlider::clickLeft(tribool down, bool previousState)
{
	if(down)
	{
		double px = GH.current->motion.x - pos.x;
		double rx = px / static_cast<double>(pos.w);
		// setVolume is out of 100
		setVolume(rx * 100);
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
	if(down)
	{
		double px = GH.current->motion.x - pos.x;
		int index = px / static_cast<double>(pos.w) * animImage->size();
		std::string hoverText = helpHandlers[index].first;
		std::string helpBox = helpHandlers[index].second;
		if(!helpBox.empty())
			CRClickPopup::createAndPush(helpBox);
		if(GH.statusbar)
			GH.statusbar->setText(helpBox);
	}
}

void CVolumeSlider::wheelScrolled(bool down, bool in)
{
	if(in)
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

void CSlider::mouseMoved(const SDL_MouseMotionEvent & sEvent)
{
	double v = 0;
	if(horizontal)
	{
		if(std::abs(sEvent.y - (pos.y + pos.h / 2)) > pos.h / 2 + 40 || std::abs(sEvent.x - (pos.x + pos.w / 2)) > pos.w / 2)
			return;
		v = sEvent.x - pos.x - 24;
		v *= positions;
		v /= (pos.w - 48);
	}
	else
	{
		if(std::abs(sEvent.x - (pos.x + pos.w / 2)) > pos.w / 2 + 40 || std::abs(sEvent.y - (pos.y + pos.h / 2)) > pos.h / 2)
			return;
		v = sEvent.y - pos.y - 24;
		v *= positions;
		v /= (pos.h - 48);
	}
	v += 0.5;
	if(v != value)
	{
		moveTo(v);
	}
}

void CSlider::setScrollStep(int to)
{
	scrollStep = to;
}

int CSlider::getAmount()
{
	return amount;
}

int CSlider::getValue()
{
	return value;
}

void CSlider::moveLeft()
{
	moveTo(value - 1);
}

void CSlider::moveRight()
{
	moveTo(value + 1);
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
			part *= (pos.w - 48);
			int newPos = part + pos.x + 16 - slider->pos.x;
			slider->moveBy(Point(newPos, 0));
		}
		else
			slider->moveTo(Point(pos.x + 16, pos.y));
	}
	else
	{
		if(positions)
		{
			double part = static_cast<double>(value) / positions;
			part *= (pos.h - 48);
			int newPos = part + pos.y + 16 - slider->pos.y;
			slider->moveBy(Point(0, newPos));
		}
		else
			slider->moveTo(Point(pos.x, pos.y + 16));
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
			pw = GH.current->motion.x - pos.x - 25;
			rw = pw / static_cast<double>(pos.w - 48);
		}
		else
		{
			pw = GH.current->motion.y - pos.y - 24;
			rw = pw / (pos.h - 48);
		}
		if(pw < -8 || pw > (horizontal ? pos.w : pos.h) - 40)
			return;
		//              if (rw>1) return;
		//              if (rw<0) return;
		slider->clickLeft(true, slider->mouseState(EIntObjMouseBtnType::LEFT));
		moveTo(rw * positions + 0.5);
		return;
	}
	if(active & MOVE)
		removeUsedEvents(MOVE);
}

CSlider::~CSlider()
{

}

CSlider::CSlider(Point position, int totalw, std::function<void(int)> Moved, int Capacity, int Amount, int Value, bool Horizontal, CSlider::EStyle style)
	: capacity(Capacity), horizontal(Horizontal), amount(Amount), value(Value), scrollStep(1), moved(Moved)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL;
	setAmount(amount);
	vstd::amax(value, 0);
	vstd::amin(value, positions);

	addUsedEvents(LCLICK | KEYBOARD | WHEEL);
	strongInterest = true;

	pos.x += position.x;
	pos.y += position.y;

	if(style == BROWN)
	{
		std::string name = horizontal ? "IGPCRDIV.DEF" : "OVBUTN2.DEF";
		//NOTE: this images do not have "blocked" frames. They should be implemented somehow (e.g. palette transform or something...)

		left = new CButton(Point(), name, CButton::tooltip());
		right = new CButton(Point(), name, CButton::tooltip());
		slider = new CButton(Point(), name, CButton::tooltip());

		left->setImageOrder(0, 1, 1, 1);
		right->setImageOrder(2, 3, 3, 3);
		slider->setImageOrder(4, 4, 4, 4);
	}
	else
	{
		left = new CButton(Point(), horizontal ? "SCNRBLF.DEF" : "SCNRBUP.DEF", CButton::tooltip());
		right = new CButton(Point(), horizontal ? "SCNRBRT.DEF" : "SCNRBDN.DEF", CButton::tooltip());
		slider = new CButton(Point(), "SCNRBSL.DEF", CButton::tooltip());
	}
	slider->actOnDown = true;
	slider->soundDisabled = true;
	left->soundDisabled = true;
	right->soundDisabled = true;

	if(horizontal)
		right->moveBy(Point(totalw - right->pos.w, 0));
	else
		right->moveBy(Point(0, totalw - right->pos.h));

	left->addCallback(std::bind(&CSlider::moveLeft, this));
	right->addCallback(std::bind(&CSlider::moveRight, this));
	slider->addCallback(std::bind(&CSlider::sliderClicked, this));

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

void CSlider::block(bool on)
{
	left->block(on);
	right->block(on);
	slider->block(on);
}

void CSlider::setAmount(int to)
{
	amount = to;
	positions = to - capacity;
	vstd::amax(positions, 0);
}

void CSlider::showAll(SDL_Surface * to)
{
	CSDL_Ext::fillRectBlack(to, &pos);
	CIntObject::showAll(to);
}

void CSlider::wheelScrolled(bool down, bool in)
{
	moveTo(value + 3 * (down ? +scrollStep : -scrollStep));
}

void CSlider::keyPressed(const SDL_KeyboardEvent & key)
{
	if(key.state != SDL_PRESSED)
		return;

	int moveDest = value;
	switch(key.keysym.sym)
	{
	case SDLK_UP:
		if(!horizontal)
			moveDest = value - scrollStep;
		break;
	case SDLK_LEFT:
		if(horizontal)
			moveDest = value - scrollStep;
		break;
	case SDLK_DOWN:
		if(!horizontal)
			moveDest = value + scrollStep;
		break;
	case SDLK_RIGHT:
		if(horizontal)
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
