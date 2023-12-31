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

#include "../gui/CIntObject.h"

VCMI_LIB_NAMESPACE_BEGIN

class CArmedInstance;
class CStackInstance;

VCMI_LIB_NAMESPACE_END

class CGarrisonInt;
class CButton;
class CAnimImage;
class CLabel;

enum class EGarrisonType
{
	UPPER, /// up garrison (Garrisoned)
	LOWER, /// down garrison (Visiting)
};

/// A single garrison slot which holds one creature of a specific amount
class CGarrisonSlot : public CIntObject
{
	SlotID ID; //for identification
	CGarrisonInt *owner;
	const CStackInstance * myStack; //nullptr if slot is empty
	const CCreature * creature;
	EGarrisonType upg;

	std::shared_ptr<CAnimImage> creatureImage;
	std::shared_ptr<CAnimImage> selectionImage; // image for selection, not always visible
	std::shared_ptr<CLabel> stackCount;

public:
	bool viewInfo();
	bool highlightOrDropArtifact();
	bool split();

	/// If certain creates cannot be moved, the selection should change
	/// Force reselection in these cases
	///     * When attempting to take creatures from ally
	///     * When attempting to swap creatures with an ally
	///     * When attempting to take unremovable units
	/// @return Whether reselection must be done
	bool mustForceReselection() const;

	void setHighlight(bool on);
	std::function<void()> getDismiss() const;

	const CArmedInstance * getObj() const;
	bool our() const;
	SlotID getSlot() const { return ID; }
	EGarrisonType getGarrison() const { return upg; }
	bool ally() const;

	// CIntObject overrides
	void showPopupWindow(const Point & cursorPosition) override;
	void clickPressed(const Point & cursorPosition) override;
	void hover (bool on) override; //call-in
	void gesture(bool on, const Point & initialPosition, const Point & finalPosition) override;

	void update();
	CGarrisonSlot(CGarrisonInt *Owner, int x, int y, SlotID IID, EGarrisonType Upg, const CStackInstance * creature_);

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

	std::map<EGarrisonType, const CArmedInstance*> armedObjs;

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

	ESlotsLayout layout;

	void selectSlot(CGarrisonSlot * slot); ///< @param slot null = deselect
	const CGarrisonSlot * getSelection() const;

	void setSplittingMode(bool on);
	bool getSplittingMode();

	bool hasEmptySlot(EGarrisonType type) const;
	SlotID getEmptySlot(EGarrisonType type) const;

	void setArmy(const CArmedInstance * army, EGarrisonType type);
	void addSplitBtn(std::shared_ptr<CButton> button);

	void recreateSlots();

	const CArmedInstance* upperArmy() const;
	const CArmedInstance* lowerArmy() const;
	const CArmedInstance* army(EGarrisonType which) const;
	bool isArmyOwned(EGarrisonType which) const;

	void splitClick();  ///< handles click on split button
	void splitStacks(const CGarrisonSlot * from, const CArmedInstance * armyDest, SlotID slotDest, int amount);  ///< TODO: comment me
	void moveStackToAnotherArmy(const CGarrisonSlot * selected);
	void bulkMoveArmy(const CGarrisonSlot * selected);
	void bulkMergeStacks(const CGarrisonSlot * selected); // Gather all creatures of selected type to the selected slot from other hero/garrison slots
	void bulkSplitStack(const CGarrisonSlot * selected); // Used to separate one-creature troops from main stack
	void bulkSmartSplitStack(const CGarrisonSlot * selected);

	/// Constructor
	/// @param position Relative position to parent element
	/// @param slotInterval Distance between slots;
	/// @param secondGarrisonOffset
	/// @param s1, s2 Top and bottom armies
	/// @param _removableUnits You can take units from top
	/// @param smallImgs Units images size 64x58 or 32x32
	/// @param _layout - when TWO_ROWS - Display slots in 2 rows (1st row = 4 slots, 2nd = 3 slots), REVERSED_TWO_ROWS = 3 slots in 1st row
	CGarrisonInt(const Point & position, int slotInterval,
				 const Point & secondGarrisonOffset,
				 const CArmedInstance * upperArmy, const CArmedInstance * lowerArmy = nullptr,
				 bool _removableUnits = true,
				 bool smallImgs = false,
				 ESlotsLayout _layout = ESlotsLayout::ONE_ROW);
};
