/*
 * EventsReceiver.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "EventsReceiver.h"

#include "MouseButton.h"
#include "CGuiHandler.h"
#include "EventDispatcher.h"

AEventsReceiver::AEventsReceiver()
	: activeState(0)
	, hoveredState(false)
	, panningState(false)
	, mouseClickedState(false)
{
}

bool AEventsReceiver::isHovered() const
{
	return hoveredState;
}

bool AEventsReceiver::isGesturing() const
{
	return panningState;
}

bool AEventsReceiver::isActive() const
{
	return activeState;
}

void AEventsReceiver::activateEvents(ui16 what)
{
	assert((what & GENERAL) || (activeState & GENERAL));

	activeState |= GENERAL;
	GH.events().activateElement(this, what);
}

void AEventsReceiver::deactivateEvents(ui16 what)
{
	if (what & GENERAL)
	{
		assert((what & activeState) == activeState);
		activeState &= ~GENERAL;

		// sanity check to avoid unexpected behavior if assertion above fails (e.g. in release)
		// if element is deactivated (has GENERAL flag) then all existing active events should also be deactivated
		what = activeState;
	}
	GH.events().deactivateElement(this, what & activeState);

	if (!(activeState & GESTURE) && panningState)
		panningState = false;

	if (!(activeState & LCLICK) && mouseClickedState)
		mouseClickedState = false;

// FIXME: might lead to regressions, recheck before enabling
//	if (!(activeState & HOVER))
//		hoveredState = false;
}
