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
{
	pos.w = model->getPixelsVisibleDimensions().x;
	pos.h = model->getPixelsVisibleDimensions().y;

	addUsedEvents(LCLICK | SHOW_POPUP | GESTURE_PANNING | HOVER | MOVE | WHEEL);
}

void MapViewActions::setContext(const std::shared_ptr<IMapRendererContext> & context)
{
	this->context = context;
}

void MapViewActions::clickLeft(tribool down, bool previousState)
{
	if(indeterminate(down))
		return;

	if(down == false)
		return;

	int3 tile = model->getTileAtPoint(GH.getCursorPosition() - pos.topLeft());

	if(context->isInMap(tile))
		adventureInt->onTileLeftClicked(tile);
}

void MapViewActions::showPopupWindow()
{
	int3 tile = model->getTileAtPoint(GH.getCursorPosition() - pos.topLeft());

	if(context->isInMap(tile))
		adventureInt->onTileRightClicked(tile);
}

void MapViewActions::mouseMoved(const Point & cursorPosition)
{
	handleHover(cursorPosition);
}

void MapViewActions::wheelScrolled(int distance)
{
	adventureInt->hotkeyZoom(distance * 4);
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

void MapViewActions::panning(bool on, const Point & initialPosition, const Point & finalPosition)
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
