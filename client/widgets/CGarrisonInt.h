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

/// A single garrison slot which holds one creature of a specific amount
class CGarrisonSlot : public CIntObject
{
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

	bool viewInfo();
	bool highlightOrDropArtifact();
	bool split();
	bool mustForceReselection() const;

	void setHighlight(bool on);
	std::function<void()> getDismiss() const;

public:
	virtual void hover (bool on) override; //call-in
	const CArmedInstance * getObj() const;
	bool our() const;
	SlotID getSlot() const { return ID; }
	bool ally() const;
	void showPopupWindow() override;
	void clickLeft(tribool down, bool previousState) override;
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
	int interx;  ///< Space between slots
	Point garOffset;  ///< Offset between garrisons (not used if only one hero)
	std::vector<std::shared_ptr<CButton>> splitButtons;  ///< May be empty if no buttons

	SlotID p2; ///< TODO: comment me
	bool pb,
		 smallIcons,      ///< true - 32x32 imgs, false - 58x64
		 removableUnits,  ///< player Can remove units from up
		 twoRows,         ///< slots Will be placed in 2 rows
		 owned[2];        ///< player Owns up or down army ([0] upper, [1] lower)

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
	void splitStacks(int amountLeft, int amountRight);  ///< TODO: comment me
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
	/// @param _twoRows Display slots in 2 row (1st row = 4 slots, 2nd = 3 slots)
	CGarrisonInt(int x, int y, int inx,
			 const Point & garsOffset,
			 const CArmedInstance * s1, const CArmedInstance * s2 = nullptr,
			 bool _removableUnits = true,
			 bool smallImgs = false,
			 bool _twoRows = false);
};

class CGarrisonHolder
{
public:
	virtual void updateGarrisons() = 0;
};

