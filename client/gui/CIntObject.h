/*
 * CIntObject.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "EventsReceiver.h"

#include "../../lib/Rect.h"
#include "../../lib/Color.h"
#include "../../lib/GameConstants.h"

class GameEngine;
class CPicture;
class Canvas;

VCMI_LIB_NAMESPACE_BEGIN
class CArmedInstance;
VCMI_LIB_NAMESPACE_END

class IShowActivatable
{
public:
	virtual void activate()=0;
	virtual void deactivate()=0;

	virtual void redraw()=0;
	virtual void show(Canvas & to) = 0;
	virtual void showAll(Canvas & to) = 0;

	virtual bool isPopupWindow() const = 0;
	virtual void onScreenResize() = 0;
	virtual ~IShowActivatable() = default;
};

// Base UI element
class CIntObject : public IShowActivatable, public AEventsReceiver //interface object
{
	ui16 used;

	//non-const versions of fields to allow changing them in CIntObject
	CIntObject *parent_m; //parent object

	bool inputEnabled;
	bool redrawParent;

public:
	std::vector<CIntObject *> children;

	/// read-only parent access. May not be a "clean" solution but allows some compatibility
	CIntObject * const & parent;

	/// position of object on the screen. Please do not modify this anywhere but in constructor - use moveBy\moveTo instead
	/*const*/ Rect pos;

	CIntObject(int used=0, Point offset=Point());
	virtual ~CIntObject();

	bool captureThisKey(EShortcut key) override;

	void addUsedEvents(ui16 newActions);
	void removeUsedEvents(ui16 newActions);

	enum {NO_ACTIONS = 0, ACTIVATE=1, DEACTIVATE=2, UPDATE=4, SHOWALL=8, SHARE_POS=16, ALL_ACTIONS=31};
	ui8 recActions; //which calls we allow to receive from parent

	/// deactivates if needed, blocks all automatic activity, allows only disposal
	void disable();
	/// activates if needed, all activity enabled (Warning: may not be symmetric with disable if recActions was limited!)
	void enable();
	/// returns true if element was disabled via disable() call
	bool isDisabled() const;

	/// deactivates or activates UI element based on flag
	void setEnabled(bool on);

	/// Block (or allow) all user input, e.g. mouse/keyboard/touch without hiding element
	void setInputEnabled(bool on);

	/// Mark this input as one that requires parent redraw on update,
	/// for example if current control might have semi-transparent elements and requires redrawing of background
	void setRedrawParent(bool on);

	// activate or deactivate object. Inactive object won't receive any input events (keyboard\mouse)
	// usually used automatically by parent
	void activate() override;
	void deactivate() override;

	//called each frame to update screen
	void show(Canvas & to) override;
	//called on complete redraw only
	void showAll(Canvas & to) override;
	//request complete redraw of this object
	void redraw() override;
	// Move child object to foreground
	void moveChildForeground(const CIntObject * childToMove);

	/// returns true if this element is a popup window
	/// called only for windows
	bool isPopupWindow() const override;

	/// called only for windows whenever screen size changes
	/// default behavior is to re-center, can be overridden
	void onScreenResize() override;

	/// returns true if UI elements wants to handle event of specific type (LCLICK, SHOW_POPUP ...)
	/// by default, usedEvents inside UI elements are always handled
	bool receiveEvent(const Point & position, int eventType) const override;

	const Rect & getPosition() const override;

	const Rect & center(const Rect &r, bool propagate = true); //sets pos so that r will be in the center of screen, assigns sizes of r to pos, returns new position
	const Rect & center(const Point &p, bool propagate = true);  //moves object so that point p will be in its center
	const Rect & center(bool propagate = true); //centers when pos.w and pos.h are set, returns new position
	void fitToScreen(int borderWidth, bool propagate = true); //moves window to fit into screen
	void fitToRect(Rect rect, int borderWidth, bool propagate = true); //moves window to fit into rect
	void moveBy(const Point &p, bool propagate = true);
	void moveTo(const Point &p, bool propagate = true);//move this to new position, coordinates are absolute (0,0 is topleft screen corner)

	void addChild(CIntObject *child, bool adjustPosition = false);
	void removeChild(CIntObject *child, bool adjustPosition = false);

};

/// Class for binding keys to left mouse button clicks
/// Classes wanting use it should have it as one of their base classes
class CKeyShortcut : public virtual CIntObject
{
public:
	bool shortcutPressed;
	EShortcut assignedKey;
	CKeyShortcut();
	CKeyShortcut(EShortcut key);
	void keyPressed(EShortcut key) override;
	void keyReleased(EShortcut key) override;
};

class WindowBase : public CIntObject
{
public:
	WindowBase(int used_ = 0, Point pos_ = Point());
	virtual void close();
};

class IGarrisonHolder
{
public:
	bool holdsGarrisons(std::vector<const CArmedInstance *> armies)
	{
		for (auto const * army : armies)
			if (holdsGarrison(army))
				return true;
		return false;
	}

	virtual bool holdsGarrison(const CArmedInstance * army) = 0;
	virtual void updateGarrisons() = 0;
};

class IMarketHolder
{
public:
	virtual void updateResources() {};
	virtual void updateExperience() {};
	virtual void updateSecondarySkills() {};
	virtual void updateArtifacts() {};
};

class ITownHolder
{
public:
	virtual void buildChanged() = 0;
};

class IStatusBar
{
public:
	virtual ~IStatusBar() = default;

	/// set current text for the status bar
	virtual void write(const std::string & text) = 0;

	/// remove any current text from the status bar
	virtual void clear() = 0;

	/// remove text from status bar if current text matches tested text
	virtual void clearIfMatching(const std::string & testedText) = 0;

	/// enables mode for entering text instead of showing hover text
	virtual void setEnteringMode(bool on) = 0;

	/// overrides hover text from controls with text entered into in-game console (for chat/cheats)
	virtual void setEnteredText(const std::string & text) = 0;

};

class EmptyStatusBar : public IStatusBar
{
	virtual void write(const std::string & text){};
	virtual void clear(){};
	virtual void clearIfMatching(const std::string & testedText){};
	virtual void setEnteringMode(bool on){};
	virtual void setEnteredText(const std::string & text){};
};

class ObjectConstruction : boost::noncopyable
{
public:
	ObjectConstruction(CIntObject *obj);
	~ObjectConstruction();
};

/// If used, all UI widgets created inside this scope will be added to children of 'this'
#define OBJECT_CONSTRUCTION ObjectConstruction obj__i(this)

/// If used, all UI widgets created inside this scope will be added to children of provided object
#define OBJECT_CONSTRUCTION_TARGETED(obj) ObjectConstruction obj__i(obj)
