/*
 * MapViewActions.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "MapViewActions.h"

#include "IMapRendererContext.h"
#include "MapView.h"
#include "MapViewModel.h"

#include "../CGameInfo.h"
#include "../adventureMap/AdventureMapInterface.h"
#include "../gui/CGuiHandler.h"
#include "../gui/CursorHandler.h"

#include "../../lib/CConfigHandler.h"

MapViewActions::MapViewActions(MapView & owner, const std::shared_ptr<MapViewModel> & model)
	: model(model)
	, owner(owner)
	, isSwiping(false)
{
	pos.w = model->getPixelsVisibleDimensions().x;
	pos.h = model->getPixelsVisibleDimensions().y;

	addUsedEvents(LCLICK | RCLICK | MCLICK | HOVER | MOVE);
}

bool MapViewActions::swipeEnabled() const
{
#if defined(VCMI_ANDROID) || defined(VCMI_IOS)
	return settings["general"]["swipe"].Bool();
#else
	return settings["general"]["swipeDesktop"].Bool();
#endif
}

void MapViewActions::setContext(const std::shared_ptr<IMapRendererContext> & context)
{
	this->context = context;
}

void MapViewActions::clickLeft(tribool down, bool previousState)
{
	if(indeterminate(down))
		return;

	if(swipeEnabled())
	{
		if(handleSwipeStateChange(static_cast<bool>(down)))
		{
			return; // if swipe is enabled, we don't process "down" events and wait for "up" (to make sure this wasn't a swiping gesture)
		}
	}
	else
	{
		if(down == false)
			return;
	}

	int3 tile = model->getTileAtPoint(GH.getCursorPosition() - pos.topLeft());

	if(context->isInMap(tile))
		adventureInt->onTileLeftClicked(tile);
}

void MapViewActions::clickRight(tribool down, bool previousState)
{
	if(isSwiping)
		return;

	int3 tile = model->getTileAtPoint(GH.getCursorPosition() - pos.topLeft());

	if(down && context->isInMap(tile))
		adventureInt->onTileRightClicked(tile);
}

void MapViewActions::clickMiddle(tribool down, bool previousState)
{
	handleSwipeStateChange(static_cast<bool>(down));
}

void MapViewActions::mouseMoved(const Point & cursorPosition)
{
	handleHover(cursorPosition);
	handleSwipeMove(cursorPosition);
}

void MapViewActions::handleSwipeMove(const Point & cursorPosition)
{
	// unless swipe is enabled, swipe move only works with middle mouse button
	if(!swipeEnabled() && !GH.isMouseButtonPressed(MouseButton::MIDDLE))
		return;

	// on mobile platforms with enabled swipe any button is enough
	if(swipeEnabled() && (!GH.isMouseButtonPressed() || GH.multifinger))
		return;

	if(!isSwiping)
	{
		static constexpr int touchSwipeSlop = 16;
		Point distance = (cursorPosition - swipeInitialRealPos);

		// try to distinguish if this touch was meant to be a swipe or just fat-fingering press
		if(std::abs(distance.x) + std::abs(distance.y) > touchSwipeSlop)
			isSwiping = true;
	}

	if(isSwiping)
	{
		Point swipeTargetPosition = swipeInitialViewPos + swipeInitialRealPos - cursorPosition;
		owner.onMapSwiped(swipeTargetPosition);
	}
}

bool MapViewActions::handleSwipeStateChange(bool btnPressed)
{
	if(btnPressed)
	{
		swipeInitialRealPos = GH.getCursorPosition();
		swipeInitialViewPos = model->getMapViewCenter();
		return true;
	}

	if(isSwiping) // only accept this touch if it wasn't a swipe
	{
		owner.onMapSwipeEnded();
		isSwiping = false;
		return true;
	}
	return false;
}

void MapViewActions::handleHover(const Point & cursorPosition)
{
	int3 tile = model->getTileAtPoint(cursorPosition - pos.topLeft());

	if(!context->isInMap(tile))
	{
		CCS->curh->set(Cursor::Map::POINTER);
		return;
	}

	adventureInt->onTileHovered(tile);
}

void MapViewActions::hover(bool on)
{
	if(!on)
	{
		GH.statusbar->clear();
		CCS->curh->set(Cursor::Map::POINTER);
	}
}
