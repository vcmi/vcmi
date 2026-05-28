/*
 * Scrollable.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "Scrollable.h"

#include "../GameEngine.h"
#include "../eventsSDL/InputHandler.h"

// SmoothScrollHelper

void SmoothScrollHelper::start()
{
	history.clear();
	vel = 0.0;
}

void SmoothScrollHelper::addStep(int delta, uint32_t ts)
{
	history.emplace_back(ts, delta);
}

void SmoothScrollHelper::finish(uint32_t now)
{
	if(history.empty()) { return; }
	int      diff    = 0;
	uint32_t firstTs = 0;
	for(const auto & e : history)
	{
		if(now - e.first < CATCH_MS)
		{
			if(firstTs == 0)
				firstTs = e.first;
			diff += e.second;
		}
	}
	const uint32_t dt = history.back().first - firstTs;
	// dt==0 happens on very fast single-frame swipes; use CATCH_MS as fallback
	// so a quick flick still produces a nonzero launch velocity.
	if(diff != 0)
		vel = static_cast<double>(diff) / static_cast<double>(dt > 0 ? dt : CATCH_MS);
	history.clear();
}

double SmoothScrollHelper::tick(uint32_t ms)
{
	if(std::abs(vel) < MIN_VEL) { vel = 0.0; return 0.0; }
	const double delta = vel * static_cast<double>(ms);
	vel /= (1.0 + static_cast<double>(ms) * SLOWDOWN);
	return delta;
}

bool SmoothScrollHelper::active() const
{
	return std::abs(vel) >= MIN_VEL;
}

void SmoothScrollHelper::stop()
{
	vel = 0.0;
	history.clear();
}

// Scrollable

Scrollable::Scrollable(int used, Point position, Orientation orientation)
	: CIntObject(used | WHEEL | GESTURE, position)
	, scrollStep(1)
	, panningDistanceSingle(32)
	, panningDistanceAccumulated(0)
	, orientation(orientation)
{
}

void Scrollable::gesture(bool on, const Point & initialPosition, const Point & finalPosition)
{
	panningDistanceAccumulated = 0;
	if(!inertiaEnabled)
		return;

	if(on)
	{
		inertialScroll.start();
		inertialAccum = 0.0;
	}
	else
	{
		inertialScroll.finish(ENGINE->input().getTicks());
	}
}

void Scrollable::wheelScrolled(int distance)
{
	scrollBy(-distance * scrollStep);
}

void Scrollable::gesturePanning(const Point & initialPosition, const Point & currentPosition, const Point & lastUpdateDistance)
{
	const int rawDelta = (orientation == Orientation::HORIZONTAL) ? -lastUpdateDistance.x : lastUpdateDistance.y;
	if(inertiaEnabled)
		inertialScroll.addStep(rawDelta, ENGINE->input().getTicks());

	panningDistanceAccumulated += rawDelta;

	if(-panningDistanceAccumulated > panningDistanceSingle)
	{
		int scrollAmount = (-panningDistanceAccumulated) / panningDistanceSingle;
		scrollBy(-scrollAmount);
		panningDistanceAccumulated += scrollAmount * panningDistanceSingle;
	}

	if(panningDistanceAccumulated > panningDistanceSingle)
	{
		int scrollAmount = panningDistanceAccumulated / panningDistanceSingle;
		scrollBy(scrollAmount);
		panningDistanceAccumulated += -scrollAmount * panningDistanceSingle;
	}
}

void Scrollable::tick(uint32_t msPassed)
{
	if(!inertiaEnabled || isGesturing() || !inertialScroll.active())
		return;

	inertialAccum += inertialScroll.tick(msPassed);

	if(-inertialAccum > panningDistanceSingle)
	{
		int amount = (-inertialAccum) / panningDistanceSingle;
		scrollBy(-amount);
		inertialAccum += amount * panningDistanceSingle;
	}

	if(inertialAccum > panningDistanceSingle)
	{
		int amount = inertialAccum / panningDistanceSingle;
		scrollBy(amount);
		inertialAccum -= amount * panningDistanceSingle;
	}
}

int Scrollable::getScrollStep() const
{
	return scrollStep;
}

Orientation Scrollable::getOrientation() const
{
	return orientation;
}

void Scrollable::scrollNext()
{
	scrollBy(+1);
}

void Scrollable::scrollPrev()
{
	scrollBy(-1);
}

void Scrollable::setScrollStep(int to)
{
	scrollStep = to;
}

void Scrollable::setPanningStep(int to)
{
	panningDistanceSingle = to;
}

void Scrollable::setScrollingEnabled(bool on)
{
	if (on)
		addUsedEvents(WHEEL | GESTURE);
	else
		removeUsedEvents(WHEEL | GESTURE);
}

void Scrollable::setInertiaEnabled(bool on)
{
	inertiaEnabled = on;
	if(on)
		addUsedEvents(TIME);
	else
		removeUsedEvents(TIME);
}
