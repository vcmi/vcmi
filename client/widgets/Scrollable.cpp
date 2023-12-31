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
}

void Scrollable::wheelScrolled(int distance)
{
	if (orientation == Orientation::HORIZONTAL)
		scrollBy(distance * scrollStep);
	else
		scrollBy(-distance * scrollStep);
}

void Scrollable::gesturePanning(const Point & initialPosition, const Point & currentPosition, const Point & lastUpdateDistance)
{
	if (orientation == Orientation::HORIZONTAL)
		panningDistanceAccumulated += -lastUpdateDistance.x;
	else
		panningDistanceAccumulated += lastUpdateDistance.y;

	if (-panningDistanceAccumulated > panningDistanceSingle )
	{
		int scrollAmount = (-panningDistanceAccumulated) / panningDistanceSingle;
		scrollBy(-scrollAmount);
		panningDistanceAccumulated += scrollAmount * panningDistanceSingle;
	}

	if (panningDistanceAccumulated > panningDistanceSingle )
	{
		int scrollAmount = panningDistanceAccumulated / panningDistanceSingle;
		scrollBy(scrollAmount);
		panningDistanceAccumulated += -scrollAmount * panningDistanceSingle;
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
