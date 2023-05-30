/*
 * Scrollable.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "../gui/CIntObject.h"

enum class Orientation
{
	HORIZONTAL,
	VERTICAL
};

/// Simple class that provides scrolling functionality via either mouse wheel or touchscreen gesture
class Scrollable : public CIntObject
{
	/// how many elements will be scrolled via one click, default = 1
	int scrollStep;
	/// How far player must move finger/mouse to move slider by 1 via gesture
	int panningDistanceSingle;
	/// How far have player moved finger/mouse via gesture so far.
	int panningDistanceAccumulated;

	Orientation orientation;

protected:
	Scrollable(int used, Point position, Orientation orientation);

	void panning(bool on) override;
	void wheelScrolled(int distance) override;
	void gesturePanning(const Point & distanceDelta) override;

	int getScrollStep() const;
	Orientation getOrientation() const;

public:
	/// Scrolls view by specified number of items
	virtual void scrollBy(int distance) = 0;

	/// Scrolls view by 1 item, identical to scrollBy(+1)
	virtual void scrollNext();

	/// Scrolls view by 1 item, identical to scrollBy(-1)
	virtual void scrollPrev();

	/// Controls how many items wil be scrolled via one click
	void setScrollStep(int to);

	/// Controls size of panning step needed to move list by 1 item
	void setPanningStep(int to);

	/// Enables or disabled scrolling
	void setScrollingEnabled(bool on);
};
