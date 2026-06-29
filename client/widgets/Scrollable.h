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

/// Inertial-scroll helper: tracks swipe history, computes post-lift velocity,
/// and decays it frame by frame.  Callers feed raw pixel deltas and convert
/// the returned pixel delta to their own scroll unit.
struct SmoothScrollHelper
{
	void start();
	void addStep(int delta, uint32_t ts);
	void finish(uint32_t now);
	double tick(uint32_t ms);
	bool active() const;
	void stop();

private:
	std::vector<std::pair<uint32_t, int>> history;
	double vel = 0.0;

	static constexpr uint32_t CATCH_MS = 100;
	static constexpr double   SLOWDOWN = 0.003;
	static constexpr double   MIN_VEL  = 0.05;
};

enum class Orientation
{
	HORIZONTAL,
	VERTICAL
};

/// Simple class that provides scrolling functionality via either mouse wheel or touchscreen gesture
class Scrollable : public CIntObject
{
	/// how many elements will be scrolled via one wheel action, default = 1
	int scrollStep;
	/// How far player must move finger/mouse to move slider by 1 via gesture
	int panningDistanceSingle;
	/// How far have player moved finger/mouse via gesture so far.
	int panningDistanceAccumulated;

	Orientation orientation;

	SmoothScrollHelper inertialScroll;
	double inertialAccum = 0.0;
	bool inertiaEnabled = false;

protected:
	Scrollable(int used, Point position, Orientation orientation);

	void gesture(bool on, const Point & initialPosition, const Point & finalPosition) override;
	void wheelScrolled(int distance) override;
	void gesturePanning(const Point & initialPosition, const Point & currentPosition, const Point & lastUpdateDistance) override;
	void tick(uint32_t msPassed) override;

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

	/// Enables post-swipe inertial scrolling (disabled by default)
	void setInertiaEnabled(bool on);
};
