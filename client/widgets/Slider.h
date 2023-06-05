/*
 * Slider.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "Scrollable.h"
#include "../../lib/FunctionList.h"

class CButton;

/// A typical slider which can be orientated horizontally/vertically.
class CSlider : public Scrollable
{
	//if vertical then left=up
	std::shared_ptr<CButton> left;
	std::shared_ptr<CButton> right;
	std::shared_ptr<CButton> slider;

	std::optional<Rect> scrollBounds;

	/// how many elements are visible simultaneously
	int capacity;
	/// number of highest position, or 0 if there is only one
	int positions;
	/// total amount of elements in the list
	int amount;
	/// topmost vislble (first active) element
	int value;

	CFunctionList<void(int)> moved;

	void updateSliderPos();
	void sliderClicked();

public:
	enum EStyle
	{
		BROWN,
		BLUE
	};

	void block(bool on);

	/// If set, mouse scroll will only scroll slider when inside of this area
	void setScrollBounds(const Rect & bounds );
	void clearScrollBounds();

	/// Value modifiers
	void scrollTo(int value);
	void scrollBy(int amount) override;
	void scrollToMin();
	void scrollToMax();

	/// Amount modifier
	void setAmount(int to);

	/// Accessors
	int getAmount() const;
	int getValue() const;
	int getCapacity() const;

	void addCallback(std::function<void(int)> callback);

	bool receiveEvent(const Point & position, int eventType) const override;
	void keyPressed(EShortcut key) override;
	void clickLeft(tribool down, bool previousState) override;
	void mouseMoved (const Point & cursorPosition) override;
	void showAll(Canvas & to) override;

	 /// @param position coordinates of slider
	 /// @param length length of slider ribbon, including left/right buttons
	 /// @param Moved function that will be called whenever slider moves
	 /// @param Capacity maximal number of visible at once elements
	 /// @param Amount total amount of elements, including not visible
	 /// @param Value starting position
	CSlider(Point position, int length, std::function<void(int)> Moved, int Capacity, int Amount,
		int Value=0, bool Horizontal=true, EStyle style = BROWN);
	~CSlider();
};
