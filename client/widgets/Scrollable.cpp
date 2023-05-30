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
	: CIntObject(used | WHEEL | GESTURE_PANNING, position)
	, scrollStep(1)
	, panningDistanceSingle(32)
	, panningDistanceAccumulated(0)
	, orientation(orientation)
{
}

void Scrollable::panning(bool on)
{
	panningDistanceAccumulated = 0;
}

void Scrollable::wheelScrolled(int distance)
{
	if (orientation == Orientation::HORIZONTAL)
		scrollBy(distance * 3);
	else
		scrollBy(-distance * 3);
}

void Scrollable::gesturePanning(const Point & distanceDelta)
{
	if (orientation == Orientation::HORIZONTAL)
		panningDistanceAccumulated += -distanceDelta.x;
	else
		panningDistanceAccumulated += distanceDelta.y;

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
		addUsedEvents(WHEEL | GESTURE_PANNING);
	else
		removeUsedEvents(WHEEL | GESTURE_PANNING);
}
