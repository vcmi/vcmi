/*
 * Buttons.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../gui/CIntObject.h"
#include "../render/EFont.h"
#include "../../lib/FunctionList.h"
#include "../../lib/filesystem/ResourcePath.h"

VCMI_LIB_NAMESPACE_BEGIN
class Rect;
VCMI_LIB_NAMESPACE_END

class CAnimImage;
class InterfaceObjectConfigurable;

enum class EButtonState
{
	NORMAL=0,
	PRESSED=1,
	BLOCKED=2,
	HIGHLIGHTED=3 // used for: highlighted state for selectable buttons, hovered state for hoverable buttons (e.g. main menu)
};

class ButtonBase : public CKeyShortcut
{
	std::shared_ptr<CAnimImage> image; //image for this button
	std::shared_ptr<InterfaceObjectConfigurable> configurable; //image for this button
	std::shared_ptr<CIntObject> overlay;//object-overlay, can be null
	std::unique_ptr<JsonNode> config;

	std::array<int, 4> stateToIndex; // mapping of button state to index of frame in animation

	EButtonState state;//current state of button from enum

	void update();//to refresh button after image or text change

	const JsonNode & getCurrentConfig() const;

protected:
	ButtonBase(Point position, const AnimationPath & defName, EShortcut key, bool playerColoredButton);
	~ButtonBase();

	std::shared_ptr<CIntObject> getOverlay();
	void setStateImpl(EButtonState state);
	EButtonState getState() const;

public:
	/// Appearance modifiers
	void setPlayerColor(PlayerColor player);
	void setImage(const AnimationPath & defName, bool playerColoredButton = false);
	void setConfigurable(const JsonPath & jsonName, bool playerColoredButton = false);
	void setImageOrder(int state1, int state2, int state3, int state4);

	/// adds overlay on top of button image. Only one overlay can be active at once
	void setOverlay(const std::shared_ptr<CIntObject>& newOverlay);
	void setTextOverlay(const std::string & Text, EFonts font, ColorRGBA color);
};

/// Typical Heroes 3 button which can be inactive or active and can
/// hold further information if you right-click it
class CButton : public ButtonBase
{
	CFunctionList<void()> callback;
	CFunctionList<void()> callbackPopup;

	std::array<std::string, 4> hoverTexts; //texts for statusbar, if empty - first entry will be used
	std::optional<ColorRGBA> borderColor; // mapping of button state to border color
	std::optional<ColorRGBA> highlightedBorderColor; // mapping of button state to border color
	std::string helpBox; //for right-click help

	bool actOnDown; //runs when mouse is pressed down over it, not when up
	bool hoverable; //if true, button will be highlighted when hovered (e.g. main menu)
	bool soundDisabled;

protected:
	void onButtonClicked(); // calls callback

	// internal method to change state. Public change can be done only via block()
	void setState(EButtonState newState);

public:
	// sets the same border color for all button states.
	void setBorderColor(std::optional<ColorRGBA> borderColor);
	void setHighlightedBorderColor(std::optional<ColorRGBA> borderColor);

	/// adds one more callback to on-click actions
	void addCallback(const std::function<void()> & callback);
	void addPopupCallback(const std::function<void()> & callback);

	void addHoverText(EButtonState state, const std::string & text);

	void block(bool on);

	void setHoverable(bool on);
	void setSoundDisabled(bool on);
	void setActOnDown(bool on);
	void setHelp(const std::pair<std::string, std::string> & help);

	/// State modifiers
	bool isBlocked();
	bool isHighlighted();
	bool isPressed();

	/// Constructor
	CButton(Point position, const AnimationPath & defName, const std::pair<std::string, std::string> & help,
			CFunctionList<void()> Callback = 0, EShortcut key = {}, bool playerColoredButton = false );

	/// CIntObject overrides
	void showPopupWindow(const Point & cursorPosition) override;
	void clickPressed(const Point & cursorPosition) override;
	void clickReleased(const Point & cursorPosition) override;
	void clickCancel(const Point & cursorPosition) override;
	void hover (bool on) override;
	void showAll(Canvas & to) override;

	/// generates tooltip that can be passed into constructor
	static std::pair<std::string, std::string> tooltip();
	static std::pair<std::string, std::string> tooltipLocalized(const std::string & key);
	static std::pair<std::string, std::string> tooltip(const std::string & hover, const std::string & help = "");
};

class CToggleBase
{
	CFunctionList<void(bool)> callback;

	bool selected;

	/// if set to false - button can not be deselected normally
	bool allowDeselection;

protected:
	// internal method for overrides
	virtual void doSelect(bool on);

	// returns true if toggle can change its state
	bool canActivate() const;

public:
	CToggleBase(CFunctionList<void(bool)> callback);
	virtual ~CToggleBase();

	/// Changes selection to "on", and calls callback
	void setSelected(bool on);

	/// Changes selection to "on" without calling callback
	void setSelectedSilent(bool on);

	bool isSelected() const;

	void setAllowDeselection(bool on);

	void addCallback(const std::function<void(bool)> & callback);

	/// Set whether the toggle is currently enabled for user to use, this is only implemented in ToggleButton, not for other toggles yet.
	virtual void setEnabled(bool enabled);
};

/// A button which can be selected/deselected, checkbox
class CToggleButton : public CButton, public CToggleBase
{
	void doSelect(bool on) override;
	void setEnabled(bool enabled) override;

	CFunctionList<void()> callbackSelected;

public:
	CToggleButton(Point position, const AnimationPath &defName, const std::pair<std::string, std::string> &help,
				  CFunctionList<void(bool)> Callback = nullptr, EShortcut key = {}, bool playerColoredButton = false,
				  CFunctionList<void()> CallbackSelected = nullptr );

	void clickPressed(const Point & cursorPosition) override;
	void clickReleased(const Point & cursorPosition) override;
	void clickCancel(const Point & cursorPosition) override;
	void clickDouble(const Point & cursorPosition) override;

	// bring overrides into scope
	//using CButton::addCallback;
	using CToggleBase::addCallback;
};

class CToggleGroup : public CIntObject
{
	CFunctionList<void(int)> onChange; //called when changing selected button with new button's id

	int selectedID;
	void selectionChanged(int to);
public:
	std::map<int, std::shared_ptr<CToggleBase>> buttons;

	CToggleGroup(const CFunctionList<void(int)> & OnChange);

	void addCallback(const std::function<void(int)> & callback);
	void resetCallback();

	/// add one toggle/button into group
	void addToggle(int index, const std::shared_ptr<CToggleBase> & button);
	/// Changes selection to specific value. Will select toggle with this ID, if present
	void setSelected(int id);
	/// in some cases, e.g. LoadGame difficulty selection, after refreshing the UI, the ToggleGroup should 
	/// reset all of it's child buttons to BLOCK state, then make selection again
	void setSelectedOnly(int id);
	int getSelected() const;
};
