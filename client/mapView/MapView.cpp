/*
 * MapView.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "MapView.h"

#include "MapViewActions.h"
#include "MapViewCache.h"
#include "MapViewController.h"
#include "MapViewModel.h"
#include "mapHandler.h"

#include "../CGameInfo.h"
#include "../CPlayerInterface.h"
#include "../adventureMap/AdventureMapInterface.h"
#include "../gui/CGuiHandler.h"
#include "../render/CAnimation.h"
#include "../render/Canvas.h"
#include "../render/IImage.h"
#include "../renderSDL/SDL_Extensions.h"
#include "../eventsSDL/InputHandler.h"

#include "../../CCallback.h"

#include "../../lib/CConfigHandler.h"
#include "../../lib/mapObjects/CGHeroInstance.h"

BasicMapView::~BasicMapView() = default;

std::shared_ptr<MapViewModel> BasicMapView::createModel(const Point & dimensions) const
{
	auto result = std::make_shared<MapViewModel>();

	result->setLevel(0);
	result->setTileSize(Point(32, 32));
	result->setViewCenter(Point(0, 0));
	result->setViewDimensions(dimensions);

	return result;
}

BasicMapView::BasicMapView(const Point & offset, const Point & dimensions)
	: model(createModel(dimensions))
	, tilesCache(new MapViewCache(model))
	, controller(new MapViewController(model, tilesCache))
{
	OBJ_CONSTRUCTION_CAPTURING_ALL_NO_DISPOSE;
	pos += offset;
	pos.w = dimensions.x;
	pos.h = dimensions.y;
}

void BasicMapView::render(Canvas & target, bool fullUpdate)
{
	Canvas targetClipped(target, pos);
	tilesCache->update(controller->getContext());
	tilesCache->render(controller->getContext(), targetClipped, fullUpdate);
}

void BasicMapView::tick(uint32_t msPassed)
{
	controller->tick(msPassed);
}

void BasicMapView::show(Canvas & to)
{
	CSDL_Ext::CClipRectGuard guard(to.getInternalSurface(), pos);
	render(to, false);

	controller->afterRender();
}

void BasicMapView::showAll(Canvas & to)
{
	CSDL_Ext::CClipRectGuard guard(to.getInternalSurface(), pos);
	render(to, true);
}

void MapView::tick(uint32_t msPassed)
{
	if(settings["adventure"]["smoothDragging"].Bool())
		postSwipe(msPassed);

	BasicMapView::tick(msPassed);
}

void MapView::show(Canvas & to)
{
	actions->setContext(controller->getContext());
	BasicMapView::show(to);
}

MapView::MapView(const Point & offset, const Point & dimensions)
	: BasicMapView(offset, dimensions)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL_NO_DISPOSE;
	actions = std::make_shared<MapViewActions>(*this, model);
	actions->setContext(controller->getContext());
}

void MapView::onMapLevelSwitched()
{
	if(LOCPLINT->cb->getMapSize().z > 1)
	{
		if(model->getLevel() == 0)
			controller->setViewCenter(model->getMapViewCenter(), 1);
		else
			controller->setViewCenter(model->getMapViewCenter(), 0);
	}
}

void MapView::onMapScrolled(const Point & distance)
{
	if(!isGesturing())
		controller->setViewCenter(model->getMapViewCenter() + distance, model->getLevel());
}

void MapView::onMapSwiped(const Point & viewPosition)
{
	if(settings["adventure"]["smoothDragging"].Bool())
		swipeHistory.push_back(std::pair<uint32_t, Point>(GH.input().getTicks(), viewPosition));

	controller->setViewCenter(model->getMapViewCenter() + viewPosition, model->getLevel());
}

void MapView::postSwipe(uint32_t msPassed) {
	if(!actions->dragActive)
	{
		if(swipeHistory.size() > 1)
		{
			Point diff = Point(0, 0);
			std::pair<uint32_t, Point> firstAccepted;
			uint32_t now = GH.input().getTicks();
			for (auto & x : swipeHistory) {
				if(now - x.first < postSwipeCatchIntervalMs) { // only the last x ms are catched
					if(firstAccepted.first == 0)
						firstAccepted = x;
					diff += x.second;
				}
			}

			uint32_t timediff = swipeHistory.back().first - firstAccepted.first;

			if(diff.length() > 0 && timediff > 0)
			{
				postSwipeAngle = diff.angle();
				postSwipeSpeed = static_cast<double>(diff.length()) / static_cast<double>(timediff); // unit: pixel/millisecond
			}	
		}
		swipeHistory.clear();
	} else
		postSwipeSpeed = 0.0;
	if(postSwipeSpeed > postSwipeMinimalSpeed) {
		double len = postSwipeSpeed * static_cast<double>(msPassed);
		Point delta = Point(len * cos(postSwipeAngle), len * sin(postSwipeAngle));

		controller->setViewCenter(model->getMapViewCenter() + delta, model->getLevel());

		postSwipeSpeed /= 1 + msPassed * postSwipeSlowdownSpeed;
	}
}

void MapView::onCenteredTile(const int3 & tile)
{
	controller->setViewCenter(tile);
}

void MapView::onCenteredObject(const CGObjectInstance * target)
{
	controller->setViewCenter(target->getSightCenter());
}

void MapView::onViewSpellActivated(uint32_t tileSize, const std::vector<ObjectPosInfo> & objectPositions, bool showTerrain)
{
	controller->activateSpellViewContext();
	controller->setTileSize(Point(tileSize, tileSize));
	controller->setOverlayVisibility(objectPositions);
	controller->setTerrainVisibility(showTerrain);
}

void MapView::onViewWorldActivated(uint32_t tileSize)
{
	controller->activateWorldViewContext();
	controller->setTileSize(Point(tileSize, tileSize));
}

void MapView::onMapZoomLevelChanged(int stepsChange)
{
	controller->modifyTileSize(stepsChange);
}

void MapView::onViewMapActivated()
{
	controller->activateAdventureContext();
	controller->setTileSize(Point(32, 32));
}

PuzzleMapView::PuzzleMapView(const Point & offset, const Point & dimensions, const int3 & tileToCenter)
	: BasicMapView(offset, dimensions)
{
	controller->activatePuzzleMapContext(tileToCenter);
	controller->setViewCenter(tileToCenter);

}
