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
#include "../gui/MouseButton.h"

#include "../../lib/CConfigHandler.h"

MapViewActions::MapViewActions(MapView & owner, const std::shared_ptr<MapViewModel> & model)
	: model(model)
	, owner(owner)
	, isSwiping(false)
	, timerCounter(0)
	, timerRunning(false)
	, postSwipeCounter(0)
{
	pos.w = model->getPixelsVisibleDimensions().x;
	pos.h = model->getPixelsVisibleDimensions().y;

	addUsedEvents(LCLICK | RCLICK | MCLICK | HOVER | MOVE | WHEEL);
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

void MapViewActions::wheelScrolled(bool down, bool in)
{
	if (!in)
		return;
	adventureInt->hotkeyZoom(down ? -1 : +1);
}

void MapViewActions::handleSwipeMove(const Point & cursorPosition)
{
	// unless swipe is enabled, swipe move only works with middle mouse button
	if(!swipeEnabled() && !GH.isMouseButtonPressed(MouseButton::MIDDLE))
		return;

	// on mobile platforms with enabled swipe we use left button
	if(swipeEnabled() && !GH.isMouseButtonPressed(MouseButton::LEFT))
		return;

	if(!isSwiping)
	{
		static constexpr int touchSwipeSlop = 16;
		Point distance = (cursorPosition - swipeInitialRealPos);

		// try to distinguish if this touch was meant to be a swipe or just fat-fingering press
		if(std::abs(distance.x) + std::abs(distance.y) > touchSwipeSlop) {
			isSwiping = true;
			startTimer();
			trackCursor(true);
		}
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
		isPostSwiping = false;
		return true;
	}

	if(isSwiping) // only accept this touch if it wasn't a swipe
	{
		isPostSwiping = true;
		postSwipeCounter = 0;
		isSwiping = false;
		return true;
	}
	return false;
}

void MapViewActions::startTimer()
{
    if(!timerRunning) {
	    addUsedEvents(TIME);
	    timerCounter = 1;
	    timerRunning = true;
	}
}

void MapViewActions::tick(uint32_t msPassed)
{
	assert(timerCounter > 0);

	if (msPassed >= timerCounter)
	{
		if(isSwiping)
	    {
	        trackCursor(false);
	    }
		else if(isPostSwiping)
	    {
	        handlePostSwipe();
		}
		
		if(isSwiping || isPostSwiping) {
	        timerCounter = 20;
		} else {
			removeUsedEvents(TIME);
			timerCounter = 0;
		    timerRunning = false;
		}
	}
	else
	{
		timerCounter -= msPassed;
	}
}

void MapViewActions::trackCursor(bool init)
{
	Point cursorPosition = GH.getCursorPosition();

	if(init) for (int i = 0; i < (sizeof(cursorPositionLast)/sizeof(cursorPositionLast[0])); i++) cursorPositionLast[i] = cursorPosition;

	swipeDelta = cursorPositionLast[0] - cursorPosition;

    for (int i = 0; i < (sizeof(cursorPositionLast)/sizeof(cursorPositionLast[0]))-1; i++) cursorPositionLast[i] = cursorPositionLast[i+1]; //change to timer (fixed interval), to avoid small move error and save cursorPosition here (or in mouseMoved) to var
    cursorPositionLast[(sizeof(cursorPositionLast)/sizeof(cursorPositionLast[0]))-1] = cursorPosition; //test also mittle mouse      &&      define minimum point to enable funcionality    && scroll @ border disable functionality
}

void MapViewActions::handlePostSwipe()
{
	Point swipeLastViewPos = model->getMapViewCenter();
    Point swipeDeltaSlowdown = swipeDelta / (postSwipeCounter + 1);
    Point swipeTargetPosition = swipeLastViewPos + swipeDeltaSlowdown;
    owner.onMapSwiped(swipeTargetPosition);
    
    postSwipeCounter++;
    
    if(postSwipeCounter == 100 || swipeDeltaSlowdown == Point(0,0)) {
	    owner.onMapSwipeEnded();
        isPostSwiping = false;
    }
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
		GH.statusbar()->clear();
		CCS->curh->set(Cursor::Map::POINTER);
	}
}
