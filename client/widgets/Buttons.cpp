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

#include "../CPlayerInterface.h"
#include "../battle/BattleInterface.h"
#include "../eventsSDL/InputHandler.h"
#include "../GameEngine.h"
#include "../GameInstance.h"
#include "../gui/MouseButton.h"
#include "../gui/Shortcut.h"
#include "../gui/InterfaceObjectConfigurable.h"
#include "../media/ISoundPlayer.h"
#include "../windows/InfoWindows.h"
#include "../render/Canvas.h"
#include "../render/IRenderHandler.h"

#include "../../lib/CConfigHandler.h"
#include "../../lib/texts/CGeneralTextHandler.h"
#include "../../lib/filesystem/Filesystem.h"
#include "../../lib/GameLibrary.h"

void ButtonBase::update()
{
	if (overlay)
	{
		Point targetPos = Rect::createCentered( pos, overlay->pos.dimensions()).topLeft();

		if (state == EButtonState::PRESSED)
			overlay->moveTo(targetPos + Point(1,1));
		else
			overlay->moveTo(targetPos);
	}

	if (image)
	{
		// checkbox - has only have two frames: normal and pressed/highlighted
		// hero movement speed buttons: only three frames: normal, pressed and blocked/highlighted
		if (state == EButtonState::HIGHLIGHTED && image->size() < 4)
			image->setFrame(image->size()-1);
		else
			image->setFrame(stateToIndex[vstd::to_underlying(state)]);
	}

	if (isActive())
		redraw();
}

void CButton::setBorderColor(std::optional<ColorRGBA> newBorderColor)
{
	borderColor = newBorderColor;
}

void CButton::setHighlightedBorderColor(std::optional<ColorRGBA> newBorderColor)
{
	highlightedBorderColor = newBorderColor;
}

void CButton::addCallback(const std::function<void()> & callback)
{
	this->callback += callback;
}

void CButton::addPopupCallback(const std::function<void()> & callback)
{
	this->callbackPopup += callback;
}

void ButtonBase::setTextOverlay(const std::string & Text, EFonts font, ColorRGBA color)
{
	OBJECT_CONSTRUCTION;
	setOverlay(std::make_shared<CLabel>(pos.w/2, pos.h/2, font, ETextAlignment::CENTER, color, Text));
	update();
}

void ButtonBase::setOverlay(const std::shared_ptr<CIntObject>& newOverlay)
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

void ButtonBase::setImage(const AnimationPath & defName, bool playerColoredButton)
{
	OBJECT_CONSTRUCTION;

	configurable.reset();
	image = std::make_shared<CAnimImage>(defName, vstd::to_underlying(getState()));
	pos = image->pos;

	if (playerColoredButton)
		image->setPlayerColor(GAME->interface()->playerID);
}

const JsonNode & ButtonBase::getCurrentConfig() const
{
	if (!config)
		throw std::runtime_error("No config found in button!");

	static constexpr std::array stateToConfig = {
		"normal",
		"pressed",
		"blocked",
		"highlighted"
	};

	std::string key = stateToConfig[vstd::to_underlying(getState())];
	const JsonNode & value = (*config)[key];

	if (value.isNull())
		throw std::runtime_error("No config found in button for state " + key + "!");

	return value;
}

void ButtonBase::setConfigurable(const JsonPath & jsonName, bool playerColoredButton)
{
	OBJECT_CONSTRUCTION;

	config = std::make_unique<JsonNode>(jsonName);

	image.reset();
	configurable = std::make_shared<InterfaceObjectConfigurable>(getCurrentConfig());
	pos = configurable->pos;

	if (playerColoredButton)
		image->setPlayerColor(GAME->interface()->playerID);
}

void CButton::addHoverText(EButtonState state, const std::string & text)
{
	hoverTexts[vstd::to_underlying(state)] = text;
}

void ButtonBase::setImageOrder(int state1, int state2, int state3, int state4)
{
	stateToIndex[0] = state1;
	stateToIndex[1] = state2;
	stateToIndex[2] = state3;
	stateToIndex[3] = state4;
	update();
}

std::shared_ptr<CIntObject> ButtonBase::getOverlay()
{
	return overlay;
}

void ButtonBase::setStateImpl(EButtonState newState)
{
	state = newState;

	if (configurable)
	{
		OBJECT_CONSTRUCTION;
		configurable = std::make_shared<InterfaceObjectConfigurable>(getCurrentConfig());
		pos = configurable->pos;

		if (overlay)
		{
			// Force overlay on top
			removeChild(overlay.get());
			addChild(overlay.get());
		}
	}

	update();
}

void CButton::setState(EButtonState newState)
{
	if (getState() == newState)
		return;

	if (newState == EButtonState::BLOCKED)
		removeUsedEvents(LCLICK | SHOW_POPUP | HOVER | KEYBOARD);
	else
		addUsedEvents(LCLICK | SHOW_POPUP | HOVER | KEYBOARD);

	setStateImpl(newState);
}

EButtonState ButtonBase::getState() const
{
	return state;
}

bool CButton::isBlocked()
{
	return getState() == EButtonState::BLOCKED;
}

bool CButton::isHighlighted()
{
	return getState() == EButtonState::HIGHLIGHTED;
}

bool CButton::isPressed()
{
	return getState() == EButtonState::PRESSED;
}

void CButton::setHoverable(bool on)
{
	hoverable = on;
}

void CButton::setSoundDisabled(bool on)
{
	soundDisabled = on;
}

void CButton::setActOnDown(bool on)
{
	actOnDown = on;
}

void CButton::setHelp(const std::pair<std::string, std::string> & help)
{
	hoverTexts[0] = help.first;
	helpBox = help.second;
}

void CButton::block(bool on)
{
	if(on || getState() == EButtonState::BLOCKED) //dont change button state if unblock requested, but it's not blocked
		setState(on ? EButtonState::BLOCKED : EButtonState::NORMAL);
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

	if (getState() != EButtonState::PRESSED)
	{
		if (!soundDisabled)
		{
			ENGINE->sound().playSound(soundBase::button);
			ENGINE->input().hapticFeedback();
		}
		setState(EButtonState::PRESSED);

		if (actOnDown)
			onButtonClicked();
	}
}

void CButton::clickReleased(const Point & cursorPosition)
{
	if (getState() == EButtonState::PRESSED)
	{
		if(hoverable && isHovered())
			setState(EButtonState::HIGHLIGHTED);
		else
			setState(EButtonState::NORMAL);

		if (!actOnDown)
			onButtonClicked();
	}
}

void CButton::clickCancel(const Point & cursorPosition)
{
	if (getState() == EButtonState::PRESSED)
	{
		if(hoverable && isHovered())
			setState(EButtonState::HIGHLIGHTED);
		else
			setState(EButtonState::NORMAL);
	}
}

void CButton::showPopupWindow(const Point & cursorPosition)
{
	callbackPopup();

	if(!helpBox.empty()) //there is no point to show window with nothing inside...
		CRClickPopup::createAndPush(helpBox);
}

void CButton::hover (bool on)
{
	if(hoverable && !isBlocked())
	{
		if(on)
			setState(EButtonState::HIGHLIGHTED);
		else
			setState(EButtonState::NORMAL);
	}

	/*if(pressedL && on) // WTF is this? When this is used?
		setState(PRESSED);*/

	std::string name = hoverTexts[vstd::to_underlying(getState())].empty()
		? hoverTexts[0]
		: hoverTexts[vstd::to_underlying(getState())];

	if(!name.empty() && !isBlocked()) //if there is no name, there is nothing to display also
	{
		if (on)
			ENGINE->statusbar()->write(name);
		else
			ENGINE->statusbar()->clearIfMatching(name);
	}
}

ButtonBase::ButtonBase(Point position, const AnimationPath & defName, EShortcut key, bool playerColoredButton)
	: CKeyShortcut(key)
	, stateToIndex({0, 1, 2, 3})
	, state(EButtonState::NORMAL)
{
	pos.x += position.x;
	pos.y += position.y;

	JsonPath jsonConfig = defName.toType<EResType::JSON>().addPrefix("CONFIG/WIDGETS/BUTTONS/");
	if (CResourceHandler::get()->existsResource(jsonConfig))
	{
		setConfigurable(jsonConfig, playerColoredButton);
		return;
	}
	else
	{
		setImage(defName, playerColoredButton);
		return;
	}
}

ButtonBase::~ButtonBase() = default;

CButton::CButton(Point position, const AnimationPath &defName, const std::pair<std::string, std::string> &help, CFunctionList<void()> Callback, EShortcut key, bool playerColoredButton):
	ButtonBase(position, defName, key, playerColoredButton),
	callback(Callback),
	helpBox(help.second),
	actOnDown(false),
	hoverable(false),
	soundDisabled(false)
{
	addUsedEvents(LCLICK | SHOW_POPUP | HOVER | KEYBOARD);
	hoverTexts[0] = help.first;
}

void ButtonBase::setPlayerColor(PlayerColor player)
{
	if (image && image->isPlayerColored())
		image->setPlayerColor(player);
}

void CButton::showAll(Canvas & to)
{
	CIntObject::showAll(to);

	if (borderColor)
		to.drawBorder(Rect::createAround(pos, 1), *borderColor);

	if (highlightedBorderColor && isHighlighted())
	{
		to.drawBorder(pos, *highlightedBorderColor);
		to.drawBorder(Rect(pos.topLeft() + Point(1,1), pos.dimensions() - Point(1,1)), *highlightedBorderColor);
	}
}

std::pair<std::string, std::string> CButton::tooltip()
{
	return std::pair<std::string, std::string>();
}

std::pair<std::string, std::string> CButton::tooltipLocalized(const std::string & key)
{
	return std::make_pair(
		LIBRARY->generaltexth->translate(key, "hover"),
		LIBRARY->generaltexth->translate(key, "help")
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

void CToggleBase::setSelectedSilent(bool on)
{
	selected = on;
	doSelect(on);
}

void CToggleBase::setSelected(bool on)
{
	bool changed = (on != selected);
	setSelectedSilent(on);
	if (changed)
		callback(on);
}

bool CToggleBase::isSelected() const
{
	return selected;
}

bool CToggleBase::canActivate() const
{
	return !selected || allowDeselection;
}

void CToggleBase::addCallback(const std::function<void(bool)> & function)
{
	callback += function;
}

void CToggleBase::setAllowDeselection(bool on)
{
	allowDeselection = on;
}

CToggleButton::CToggleButton(Point position, const AnimationPath &defName, const std::pair<std::string, std::string> &help,
							 CFunctionList<void(bool)> callback, EShortcut key, bool playerColoredButton,
				  			 CFunctionList<void()> callbackSelected):
  CButton(position, defName, help, 0, key, playerColoredButton),
  CToggleBase(callback),
  callbackSelected(callbackSelected)
{
	addUsedEvents(DOUBLECLICK);
}

void CToggleButton::doSelect(bool on)
{
	if (on)
	{
		setState(EButtonState::HIGHLIGHTED);
	}
	else
	{
		setState(EButtonState::NORMAL);
	}
}

void CToggleButton::setEnabled(bool enabled)
{
	setState(enabled ? EButtonState::NORMAL : EButtonState::BLOCKED);
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
		ENGINE->sound().playSound(soundBase::button);
		ENGINE->input().hapticFeedback();
		setState(EButtonState::PRESSED);
	}
}

void CToggleButton::clickReleased(const Point & cursorPosition)
{
	// force refresh
	hover(false);
	hover(true);

	if(isBlocked())
		return;

	if (getState() == EButtonState::PRESSED && canActivate())
	{
		onButtonClicked();
		setSelected(!isSelected());
	}
	else
		doSelect(isSelected()); // restore
}

void CToggleButton::clickCancel(const Point & cursorPosition)
{
	// force refresh
	hover(false);
	hover(true);

	if(isBlocked())
		return;

	doSelect(isSelected());
}

void CToggleButton::clickDouble(const Point & cursorPosition)
{
	if(callbackSelected)
		callbackSelected();
}

void CToggleGroup::addCallback(const std::function<void(int)> & callback)
{
	onChange += callback;
}

void CToggleGroup::resetCallback()
{
	onChange.clear();
}

void CToggleGroup::addToggle(int identifier, const std::shared_ptr<CToggleBase> & button)
{
	if(auto intObj = std::dynamic_pointer_cast<CIntObject>(button)) // hack-ish workagound to avoid diamond problem with inheritance
	{
		addChild(intObj.get());
	}

	button->addCallback([this, identifier] (bool on) { if (on) selectionChanged(identifier);});
	button->setAllowDeselection(false);

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
	for(const auto & button : buttons)
		button.second->setEnabled(button.first == id);

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
