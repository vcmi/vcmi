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
public:
	enum EStyle
	{
		BROWN,
		BLUE
	};

private:
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
	/// length of slider
	int length;
	/// length of slider button
	int barLength;
	/// color of slider
	EStyle style;

	CFunctionList<void(int)> moved;

	void updateSliderPos();
	void updateSlider();

	double getClickPos(const Point & cursorPosition);

public:
	void block(bool on);

	/// If set, mouse scroll will only scroll slider when inside of this area
	void setScrollBounds(const Rect & bounds );
	void clearScrollBounds();

	/// Value modifiers
	void scrollTo(int value, bool callCallbacks = true);
	void scrollBy(int amount) override;
	void scrollToMin();
	void scrollToMax();

	/// Amount modifier
	void setAmount(int to);
	virtual void setValue(int to, bool callCallbacks = true);

	/// Accessors
	int getAmount() const;
	virtual int getValue() const;
	int getCapacity() const;

	void addCallback(std::function<void(int)> callback);

	bool receiveEvent(const Point & position, int eventType) const override;
	void keyPressed(EShortcut key) override;
	void clickPressed(const Point & cursorPosition) override;
	void clickReleased(const Point & cursorPosition) override;
	void mouseDragged(const Point & cursorPosition, const Point & lastUpdateDistance) override;
	void gesturePanning(const Point & initialPosition, const Point & currentPosition, const Point & lastUpdateDistance) override;
	void showAll(Canvas & to) override;

	using SliderMovingFunctor = std::function<void(int)>;

	 /// @param position coordinates of slider
	 /// @param length length of slider ribbon, including left/right buttons
	 /// @param Moved function that will be called whenever slider moves
	 /// @param Capacity maximal number of visible at once elements
	 /// @param Amount total amount of elements, including not visible
	 /// @param Value starting position
	CSlider(Point position, int length, const SliderMovingFunctor & Moved, int Capacity, int Amount,
		int Value, Orientation orientation, EStyle style = BROWN);
	~CSlider();
};

class SliderNonlinear : public CSlider
{
	/// If non-empty then slider has non-linear values, e.g. if slider is at position 5 out of 10 then actual "value" is not 5, but 5th value in this vector
	std::vector<int> scaledValues;

	using CSlider::setAmount; // make private
public:
	void setValue(int to, bool callCallbacks) override;
	int getValue() const override;

	SliderNonlinear(Point position, int length, const std::function<void(int)> & Moved, const std::vector<int> & values, int Value, Orientation orientation, EStyle style);
};
