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
	, strongInterestState(false)
{
}

bool AEventsReceiver::isHovered() const
{
	return hoveredState;
}

bool AEventsReceiver::isActive() const
{
	return activeState;
}

bool AEventsReceiver::isMouseButtonPressed(MouseButton btn) const
{
	return currentMouseState.count(btn) ? currentMouseState.at(btn) : false;
}

void AEventsReceiver::setMoveEventStrongInterest(bool on)
{
	strongInterestState = on;
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
		activeState &= ~GENERAL;
	GH.events().deactivateElement(this, what & activeState);
}
