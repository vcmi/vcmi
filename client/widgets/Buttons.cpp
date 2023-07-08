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
#include "../gui/MouseButton.h"
#include "../gui/Shortcut.h"
#include "../windows/InfoWindows.h"
#include "../render/CAnimation.h"
#include "../render/Canvas.h"

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

	if (isActive())
		redraw();
}

void CButton::setBorderColor(std::optional<SDL_Color> borderColor)
{
	setBorderColor(borderColor, borderColor, borderColor, borderColor);
}

void CButton::setBorderColor(std::optional<SDL_Color> normalBorderColor,
							 std::optional<SDL_Color> pressedBorderColor,
							 std::optional<SDL_Color> blockedBorderColor,
							 std::optional<SDL_Color> highlightedBorderColor)
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

void CButton::clickPressed(const Point & cursorPosition)
{
	if(isBlocked())
		return;

	if (getState() != PRESSED)
	{
		if (!soundDisabled)
			CCS->soundh->playSound(soundBase::button);
		setState(PRESSED);

		if (actOnDown)
			onButtonClicked();
	}
}

void CButton::clickReleased(const Point & cursorPosition)
{
	if (getState() == PRESSED)
	{
		if(hoverable && isHovered())
			setState(HIGHLIGHTED);
		else
			setState(NORMAL);

		if (!actOnDown)
			onButtonClicked();
	}
}

void CButton::clickCancel(const Point & cursorPosition)
{
	if (getState() == PRESSED)
	{
		if(hoverable && isHovered())
			setState(HIGHLIGHTED);
		else
			setState(NORMAL);
	}
}

void CButton::showPopupWindow(const Point & cursorPosition)
{
	if(helpBox.size()) //there is no point to show window with nothing inside...
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
			GH.statusbar()->write(name);
		else
			GH.statusbar()->clearIfMatching(name);
	}
}

CButton::CButton(Point position, const std::string &defName, const std::pair<std::string, std::string> &help, CFunctionList<void()> Callback, EShortcut key, bool playerColoredButton):
    CKeyShortcut(key),
    callback(Callback)
{
	defActions = 255-DISPOSE;
	addUsedEvents(LCLICK | SHOW_POPUP | HOVER | KEYBOARD);

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
		setIndex(0);
		if (playerColoredButton)
			image->playerColored(LOCPLINT->playerID);
	}
}

void CButton::setIndex(size_t index)
{
	if (index == currentImage || index>=imageNames.size())
		return;
	currentImage = index;
	auto anim = std::make_shared<CAnimation>(imageNames[index]);
	setImage(anim);
}

void CButton::setImage(std::shared_ptr<CAnimation> anim, int animFlags)
{
	OBJECT_CONSTRUCTION_CUSTOM_CAPTURING(255-DISPOSE);

	image = std::make_shared<CAnimImage>(anim, getState(), 0, 0, 0, animFlags);
	pos = image->pos;
}

void CButton::setPlayerColor(PlayerColor player)
{
	if (image && image->isPlayerColored())
		image->playerColored(player);
}

void CButton::showAll(Canvas & to)
{
	CIntObject::showAll(to);

	auto borderColor = stateToBorderColor[getState()];
	if (borderColor)
		to.drawBorder(Rect::createAround(pos, 1), *borderColor);
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
							 CFunctionList<void(bool)> callback, EShortcut key, bool playerColoredButton):
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

void CToggleButton::clickPressed(const Point & cursorPosition)
{
	// force refresh
	hover(false);
	hover(true);

	if(isBlocked())
		return;

	if (canActivate())
	{
		CCS->soundh->playSound(soundBase::button);
		setState(PRESSED);
	}
}

void CToggleButton::clickReleased(const Point & cursorPosition)
{
	// force refresh
	hover(false);
	hover(true);

	if(isBlocked())
		return;

	if (getState() == PRESSED && canActivate())
	{
		onButtonClicked();
		setSelected(!selected);
	}
	else
		doSelect(selected); // restore
}

void CToggleButton::clickCancel(const Point & cursorPosition)
{
	// force refresh
	hover(false);
	hover(true);

	if(isBlocked())
		return;

	doSelect(selected);
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
