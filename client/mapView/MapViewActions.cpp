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

#include "../CPlayerInterface.h"
#include "../adventureMap/CInGameConsole.h"

#include "../../lib/CConfigHandler.h"

MapViewActions::MapViewActions(MapView & owner, const std::shared_ptr<MapViewModel> & model)
	: model(model)
	, owner(owner)
	, pinchZoomFactor(1.0)
	, dragActive(false)
{
	pos.w = model->getPixelsVisibleDimensions().x;
	pos.h = model->getPixelsVisibleDimensions().y;

	addUsedEvents(LCLICK | SHOW_POPUP | DRAG | GESTURE | HOVER | MOVE | WHEEL);
}

void MapViewActions::setContext(const std::shared_ptr<IMapRendererContext> & context)
{
	this->context = context;
}

void MapViewActions::clickPressed(const Point & cursorPosition)
{
	if (!settings["adventure"]["leftButtonDrag"].Bool())
	{
		int3 tile = model->getTileAtPoint(cursorPosition - pos.topLeft());

		if(context->isInMap(tile))
			adventureInt->onTileLeftClicked(tile);
	}
}

void MapViewActions::clickReleased(const Point & cursorPosition)
{
	if (!dragActive && settings["adventure"]["leftButtonDrag"].Bool())
	{
		int3 tile = model->getTileAtPoint(cursorPosition - pos.topLeft());

		if(context->isInMap(tile))
			adventureInt->onTileLeftClicked(tile);
	}
	dragActive = false;
	dragDistance = Point(0,0);
}

void MapViewActions::clickCancel(const Point & cursorPosition)
{
	dragActive = false;
	dragDistance = Point(0,0);
}

void MapViewActions::showPopupWindow(const Point & cursorPosition)
{
	int3 tile = model->getTileAtPoint(cursorPosition - pos.topLeft());

	if(context->isInMap(tile))
		adventureInt->onTileRightClicked(tile);
}

void MapViewActions::mouseMoved(const Point & cursorPosition, const Point & lastUpdateDistance)
{
	handleHover(cursorPosition);
}

void MapViewActions::wheelScrolled(int distance)
{
	adventureInt->hotkeyZoom(distance * 4);
}

void MapViewActions::mouseDragged(const Point & cursorPosition, const Point & lastUpdateDistance)
{
	dragDistance += lastUpdateDistance;

	if (dragDistance.length() > 16)
		dragActive = true;

	if (dragActive && settings["adventure"]["leftButtonDrag"].Bool())
		owner.onMapSwiped(lastUpdateDistance);
}

void MapViewActions::gesturePanning(const Point & initialPosition, const Point & currentPosition, const Point & lastUpdateDistance)
{
	owner.onMapSwiped(lastUpdateDistance);
}

void MapViewActions::gesturePinch(const Point & centerPosition, double lastUpdateFactor)
{
	double newZoom = pinchZoomFactor * lastUpdateFactor;

	int newZoomSteps = std::round(std::log(newZoom) / std::log(1.01));
	int oldZoomSteps = std::round(std::log(pinchZoomFactor) / std::log(1.01));

	if (newZoomSteps != oldZoomSteps)
		adventureInt->hotkeyZoom(newZoomSteps - oldZoomSteps);

	pinchZoomFactor = newZoom;
}

void MapViewActions::gesture(bool on, const Point & initialPosition, const Point & finalPosition)
{
	pinchZoomFactor = 1.0;
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
