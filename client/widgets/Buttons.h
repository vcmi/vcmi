#pragma once

#include "../gui/CIntObject.h"
#include "../gui/SDL_Extensions.h"

#include "../../lib/FunctionList.h"

struct SDL_Surface;
struct Rect;
class CAnimImage;
class CLabel;
class CAnimation;
class CDefHandler;

namespace config
{
	struct ButtonInfo;
}

/*
 * Buttons.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

class ClickableArea : public CIntObject //TODO: derive from LRCLickableArea? Or somehow use its right-click/hover data?
{
	CFunctionList<void()> callback;

	CIntObject * area;

protected:
	void onClick();

public:
	ClickableArea(CIntObject * object, CFunctionList<void()> callback);

	void addCallback(std::function<void()> callback);
	void setArea(CIntObject * object);

	void clickLeft(tribool down, bool previousState) override;
};

/// Typical Heroes 3 button which can be inactive or active and can
/// hold further information if you right-click it
class CButton : public CKeyShortcut
{
	CFunctionList<void()> callback;

public:
	enum ButtonState
	{
		NORMAL=0,
		PRESSED=1,
		BLOCKED=2,
		HIGHLIGHTED=3
	};
private:
	std::vector<std::string> imageNames;//store list of images that can be used by this button
	size_t currentImage;

	ButtonState state;//current state of button from enum

	std::array<int, 4> stateToIndex; // mapping of button state to index of frame in animation
	std::array<std::string, 4> hoverTexts; //texts for statusbar, if empty - first entry will be used
	std::string helpBox; //for right-click help

	CAnimImage * image; //image for this button
	CIntObject * overlay;//object-overlay, can be null

protected:
	void onButtonClicked(); // calls callback
	void update();//to refresh button after image or text change

	// internal method to change state. Public change can be done only via block()
	void setState(ButtonState newState);
	ButtonState getState();

public:
	bool actOnDown,//runs when mouse is pressed down over it, not when up
		hoverable,//if true, button will be highlighted when hovered (e.g. main menu)
		soundDisabled;

	// if set, button will have 1-px border around it with this color
	boost::optional<SDL_Color> borderColor;

	/// adds one more callback to on-click actions
	void addCallback(std::function<void()> callback);

	/// adds overlay on top of button image. Only one overlay can be active at once
	void addOverlay(CIntObject * newOverlay);
	void addTextOverlay(const std::string &Text, EFonts font, SDL_Color color = Colors::WHITE);

	void addImage(std::string filename);
	void addHoverText(ButtonState state, std::string text);

	void setImageOrder(int state1, int state2, int state3, int state4);
	void block(bool on);

	/// State modifiers
	bool isBlocked();
	bool isHighlighted();

	/// Constructor
	CButton(Point position, const std::string &defName, const std::pair<std::string, std::string> &help,
	        CFunctionList<void()> Callback = 0, int key=0, bool playerColoredButton = false );

	/// Appearance modifiers
	void setIndex(size_t index, bool playerColoredButton=false);
	void setImage(CAnimation* anim, bool playerColoredButton=false, int animFlags=0);
	void setPlayerColor(PlayerColor player);

	/// CIntObject overrides
	void clickRight(tribool down, bool previousState) override;
	void clickLeft(tribool down, bool previousState) override;
	void hover (bool on) override;
	void showAll(SDL_Surface * to) override;

	/// generates tooltip that can be passed into constructor
	static std::pair<std::string, std::string> tooltip();
	static std::pair<std::string, std::string> tooltip(const JsonNode & localizedTexts);
	static std::pair<std::string, std::string> tooltip(const std::string & hover, const std::string & help = "");
};

class CToggleBase
{
	CFunctionList<void(bool)> callback;
protected:

	bool selected;

	// internal method for overrides
	virtual void doSelect(bool on);

	// returns true if toggle can change its state
	bool canActivate();

public:
	/// if set to false - button can not be deselected normally
	bool allowDeselection;

	CToggleBase(CFunctionList<void(bool)> callback);
	virtual ~CToggleBase();

	/// Changes selection to "on", and calls callback
	void setSelected(bool on);

	void addCallback(std::function<void(bool)> callback);
};

class ClickableToggle : public ClickableArea, public CToggleBase
{
public:
	ClickableToggle(CIntObject * object, CFunctionList<void()> selectFun, CFunctionList<void()> deselectFun);
	void clickLeft(tribool down, bool previousState) override;
};

/// A button which can be selected/deselected, checkbox
class CToggleButton : public CButton, public CToggleBase
{
	void doSelect(bool on) override;
public:
	CToggleButton(Point position, const std::string &defName, const std::pair<std::string, std::string> &help,
	              CFunctionList<void(bool)> Callback = 0, int key=0, bool playerColoredButton = false );
	void clickLeft(tribool down, bool previousState) override;

	// bring overrides into scope
	//using CButton::addCallback;
	using CToggleBase::addCallback;
};

class CToggleGroup : public CIntObject
{
	CFunctionList<void(int)> onChange; //called when changing selected button with new button's id

	int selectedID;
	bool musicLike; //determines the behaviour of this group
	void selectionChanged(int to);
public:
	std::map<int, CToggleBase*> buttons;

	CToggleGroup(const CFunctionList<void(int)> & OnChange, bool musicLikeButtons = false);

	void addCallback(std::function<void(int)> callback);

	/// add one toggle/button into group
	void addToggle(int index, CToggleBase * button);
	/// Changes selection to specific value. Will select toggle with this ID, if present
	void setSelected(int id);

	void show(SDL_Surface * to);
	void showAll(SDL_Surface * to);
};

/// A typical slider which can be orientated horizontally/vertically.
class CSlider : public CIntObject
{
	CButton *left, *right, *slider; //if vertical then left=up
	int capacity;//how many elements can be active at same time (e.g. hero list = 5)
	int positions; //number of highest position (0 if there is only one)
	bool horizontal;
	bool wheelScrolling;
	bool keyScrolling;

	int amount; //total amount of elements (e.g. hero list = 0-8)
	int value; //first active element
	int scrollStep; // how many elements will be scrolled via one click, default = 1
	CFunctionList<void(int)> moved;

	void updateSliderPos();
	void sliderClicked();

public:
	enum EStyle {
		BROWN,
		BLUE
	};

	void block(bool on);

	/// Controls how many items wil be scrolled via one click
	void setScrollStep(int to);

	/// Value modifiers
	void moveLeft();
	void moveRight();
	void moveTo(int value);
	void moveBy(int amount);
	void moveToMin();
	void moveToMax();

	/// Amount modifier
	void setAmount(int to);

	/// Accessors
	int getAmount();
	int getValue();

	void addCallback(std::function<void(int)> callback);

	void keyPressed(const SDL_KeyboardEvent & key);
	void wheelScrolled(bool down, bool in);
	void clickLeft(tribool down, bool previousState);
	void mouseMoved (const SDL_MouseMotionEvent & sEvent);
	void showAll(SDL_Surface * to);	

	/**
	 * @param position, coordinates of slider
	 * @param length, length of slider ribbon, including left/right buttons
	 * @param Moved, function that will be called whenever slider moves
	 * @param Capacity, maximal number of visible at once elements
	 * @param Amount, total amount of elements, including not visible
	 * @param Value, starting position
	 */
	CSlider(Point position, int length, std::function<void(int)> Moved, int Capacity, int Amount,
		int Value=0, bool Horizontal=true, EStyle style = BROWN);
	~CSlider();
};
