/*
 * CGarrisonInt.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../windows/CWindowObject.h"

VCMI_LIB_NAMESPACE_BEGIN

class CArmedInstance;
class CCreatureSet;
class CStackInstance;

VCMI_LIB_NAMESPACE_END

class CGarrisonInt;
class CButton;
class CAnimImage;
class CGarrisonSlot;
class CLabel;
class IImage;

class RadialMenuItem : public CIntObject
{
	std::shared_ptr<IImage> image;
	std::shared_ptr<CPicture> picture;
public:
	std::function<void()> callback;

	RadialMenuItem(std::string imageName, std::function<void()> callback);

	bool isInside(const Point & position);

	void gesturePanning(const Point & initialPosition, const Point & currentPosition, const Point & lastUpdateDistance) override;
	void gesture(bool on, const Point & initialPosition, const Point & finalPosition) override;
};

class RadialMenu : public CIntObject
{
	static constexpr Point ITEM_NW = Point( -35, -85);
	static constexpr Point ITEM_NE = Point( +35, -85);
	static constexpr Point ITEM_WW = Point( -85, 0);
	static constexpr Point ITEM_EE = Point( +85, 0);
	static constexpr Point ITEM_SW = Point( -35, +85);
	static constexpr Point ITEM_SE = Point( +35, +85);

	std::vector<std::shared_ptr<RadialMenuItem>> items;

	void addItem(const Point & offset, const std::string & path, std::function<void()> callback );
public:
	RadialMenu(CGarrisonInt * army, CGarrisonSlot * slot);

	void gesturePanning(const Point & initialPosition, const Point & currentPosition, const Point & lastUpdateDistance) override;
	void gesture(bool on, const Point & initialPosition, const Point & finalPosition) override;
	void show(Canvas & to) override;
};

/// A single garrison slot which holds one creature of a specific amount
class CGarrisonSlot : public CIntObject
{
public:
	SlotID ID; //for identification
	CGarrisonInt *owner;
	const CStackInstance * myStack; //nullptr if slot is empty
	const CCreature * creature;

	/// Type of Garrison for slot (up or down)
	enum EGarrisonType
	{
		UP=0,  ///< 0 - up garrison (Garrisoned)
		DOWN,  ///< 1 - down garrison (Visiting)
	} upg; ///< Flag indicating if it is the up or down garrison

	std::shared_ptr<CAnimImage> creatureImage;
	std::shared_ptr<CAnimImage> selectionImage; // image for selection, not always visible
	std::shared_ptr<CLabel> stackCount;
	std::shared_ptr<RadialMenu> radialMenu;

public:
	bool viewInfo();
	bool highlightOrDropArtifact();
	bool split();
	bool mustForceReselection() const;

	void setHighlight(bool on);
	std::function<void()> getDismiss() const;

	virtual void hover (bool on) override; //call-in
	const CArmedInstance * getObj() const;
	bool our() const;
	SlotID getSlot() const { return ID; }
	bool ally() const;
	void showPopupWindow(const Point & cursorPosition) override;
	void clickPressed(const Point & cursorPosition) override;

	void gesturePanning(const Point & initialPosition, const Point & currentPosition, const Point & lastUpdateDistance) override;
	void gesture(bool on, const Point & initialPosition, const Point & finalPosition) override;

	void update();
	CGarrisonSlot(CGarrisonInt *Owner, int x, int y, SlotID IID, EGarrisonType Upg=EGarrisonType::UP, const CStackInstance * creature_ = nullptr);

	void splitIntoParts(EGarrisonType type, int amount);
	bool handleSplittingShortcuts(); /// Returns true when some shortcut is pressed, false otherwise

	friend class CGarrisonInt;
};

/// Class which manages slots of upper and lower garrison, splitting of units
class CGarrisonInt :public CIntObject
{
	/// Chosen slot. Should be changed only via selectSlot.
	CGarrisonSlot * highlighted;
	bool inSplittingMode;
	std::vector<std::shared_ptr<CGarrisonSlot>> availableSlots;  ///< Slots of upper and lower garrison

	void createSlots();
	bool checkSelected(const CGarrisonSlot * selected, TQuantity min = 0) const;

public:
	enum class ESlotsLayout
	{
		ONE_ROW,
		TWO_ROWS,
		REVERSED_TWO_ROWS
	};

	int interx;  ///< Space between slots
	Point garOffset;  ///< Offset between garrisons (not used if only one hero)
	std::vector<std::shared_ptr<CButton>> splitButtons;  ///< May be empty if no buttons

	bool smallIcons;      ///< true - 32x32 imgs, false - 58x64
	bool removableUnits;  ///< player Can remove units from up
	bool twoRows;         ///< slots Will be placed in 2 rows
	bool owned[2];        ///< player Owns up or down army ([0] upper, [1] lower)

	ESlotsLayout layout;

	void selectSlot(CGarrisonSlot * slot); ///< @param slot null = deselect
	const CGarrisonSlot * getSelection() const;

	void setSplittingMode(bool on);
	bool getSplittingMode();

	bool hasEmptySlot(CGarrisonSlot::EGarrisonType type) const;
	SlotID getEmptySlot(CGarrisonSlot::EGarrisonType type) const;

	const CArmedInstance * armedObjs[2];  ///< [0] is upper, [1] is down

	void setArmy(const CArmedInstance * army, bool bottomGarrison);
	void addSplitBtn(std::shared_ptr<CButton> button);

	void recreateSlots();

	void splitClick();  ///< handles click on split button
	void splitStacks(const CGarrisonSlot * from, const CArmedInstance * armyDest, SlotID slotDest, int amount);  ///< TODO: comment me
	void moveStackToAnotherArmy(const CGarrisonSlot * selected);
	void bulkMoveArmy(const CGarrisonSlot * selected);
	void bulkMergeStacks(const CGarrisonSlot * selected); // Gather all creatures of selected type to the selected slot from other hero/garrison slots
	void bulkSplitStack(const CGarrisonSlot * selected); // Used to separate one-creature troops from main stack
	void bulkSmartSplitStack(const CGarrisonSlot * selected);

	/// Constructor
	/// @param x, y Position
	/// @param inx Distance between slots;
	/// @param garsOffset
	/// @param s1, s2 Top and bottom armies
	/// @param _removableUnits You can take units from top
	/// @param smallImgs Units images size 64x58 or 32x32
	/// @param _layout - when TWO_ROWS - Display slots in 2 rows (1st row = 4 slots, 2nd = 3 slots), REVERSED_TWO_ROWS = 3 slots in 1st row
	CGarrisonInt(int x, int y, int inx,
				 const Point & garsOffset,
				 const CArmedInstance * s1, const CArmedInstance * s2 = nullptr,
				 bool _removableUnits = true,
				 bool smallImgs = false,
				 ESlotsLayout _layout = ESlotsLayout::ONE_ROW);
};

class CGarrisonHolder
{
public:
	virtual void updateGarrisons() = 0;
};

