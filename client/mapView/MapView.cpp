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
#include "Profiler.h"
#include "MapView.h"

#include "MapViewActions.h"
#include "MapViewCache.h"
#include "MapViewController.h"
#include "MapViewModel.h"
#include "mapHandler.h"

#include "../CPlayerInterface.h"
#include "../adventureMap/AdventureMapInterface.h"
#include "../GameEngine.h"
#include "../GameInstance.h"
#include "render/CAnimation.h"
#include "render/Canvas.h"
#include "render/Colors.h"
#include "render/IImage.h"
#include "render/IScreenHandler.h"
#include "events/InputHandler.h"

#include "../../lib/callback/CCallback.h"
#include "../../lib/CConfigHandler.h"
#include "../../lib/mapObjects/CGHeroInstance.h"

#include "MapOverlayLogVisualizer.h"

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

BasicMapView::BasicMapView(const Point & offset, const Point & dimensions, bool useGpuLayer)
	: model(createModel(dimensions))
	, tilesCache(new MapViewCache(model, useGpuLayer))
	, controller(new MapViewController(model, tilesCache))
	, gpuLayerEligible(useGpuLayer)
	, needFullUpdate(false)
{
	OBJECT_CONSTRUCTION;
	pos += offset;
	pos.w = dimensions.x;
	pos.h = dimensions.y;
}

void BasicMapView::render(Canvas & target, bool fullUpdate)
{
	VCMI_PROFILE_N("Map: render");
	if(gpuLayerEligible && ENGINE->screenHandler().isGpuRenderingEnabled())
	{
		// The software screen keeps a transparent hole over the map's GPU layer. Punched every
		// frame, since any widget redrawing its background would fill it back in. Thread safe.
		target.drawColor(pos, ColorRGBA(0, 0, 0, 0));

		// a redraw arriving from another thread is queued by CIntObject::redraw() and reaches us
		// from the next frame instead, so by here the GL context is ours
		assert(ENGINE->amIGuiThread());

		renderGpu(fullUpdate);
		return;
	}

	Canvas targetClipped(target, pos);
	tilesCache->update(controller->getContext());
	tilesCache->render(controller->getContext(), targetClipped, fullUpdate);

	MapOverlayLogVisualizer r(targetClipped, model);
	logVisual->visualize(r);
}

void BasicMapView::renderGpu(bool fullUpdate)
{
	VCMI_PROFILE_N("Map: render (GPU layer)");
	Canvas layer = ENGINE->screenHandler().getLayerCanvas(GpuRenderLayer::MAP);
	Canvas targetClipped(layer, pos);

	// tick() normally filled the cache; a scroll, zoom or level change since then invalidates it
	if(!tilesCache->isUpdatedThisFrame())
		tilesCache->update(controller->getContext());

	tilesCache->render(controller->getContext(), targetClipped, fullUpdate);
}

void BasicMapView::tick(uint32_t msPassed)
{
	VCMI_PROFILE_N("Map: tick");
	controller->tick(msPassed);

	// tick() only ever runs from the timer dispatch, which is part of the frame
	assert(ENGINE->amIGuiThread());

	// Draw the tiles into the cache now, so the GPU can work while the rest of the frame is
	// prepared - reading the cache straight after writing it stalls until that drawing is done.
	if(gpuLayerEligible && ENGINE->screenHandler().isGpuRenderingEnabled())
	{
		tilesCache->update(controller->getContext());
#ifndef VCMI_MOBILE
		// a tile-based mobile GPU resolves its render target on every flush, which costs
		// more than the stall this avoids on desktop
		ENGINE->screenHandler().flushRenderCommands();
#endif
	}
}

void BasicMapView::show(Canvas & to)
{
	VCMI_PROFILE_N("Map: show");
	CanvasClipRectGuard guard(to, pos);
	render(to, needFullUpdate);

	controller->afterRender();
}

void BasicMapView::showAll(Canvas & to)
{
	VCMI_PROFILE_N("Map: show all");
	CanvasClipRectGuard guard(to, pos);
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
	: BasicMapView(offset, dimensions, true)
{
	OBJECT_CONSTRUCTION;
	actions = std::make_shared<MapViewActions>(*this, model);
	actions->setContext(controller->getContext());

	// catch min 6 frames
	postSwipeCatchIntervalMs = std::max(100, static_cast<int>(6.0 * 1000.0 * (1.0 / settings["video"]["targetfps"].Float())));
}

void MapView::onMapLevelSwitched()
{
	if(GAME->interface()->cb->getMapSize().z > 1)
	{
		int newLevel = model->getLevel() + 1;
		if(newLevel < GAME->interface()->cb->getMapSize().z)
			controller->setViewCenter(model->getMapViewCenter(), newLevel);
		else
			controller->setViewCenter(model->getMapViewCenter(), 0);
	}
}

void MapView::onMapScrolled(const Point & distance)
{
	if(!isGesturing())
	{
		postSwipeSpeed = 0.0;
		controller->setViewCenter(model->getMapViewCenter() + distance, model->getLevel());
	}
}

void MapView::onMapSwiped(const Point & viewPosition)
{
	if(settings["adventure"]["smoothDragging"].Bool())
		swipeHistory.push_back(std::pair<uint32_t, Point>(ENGINE->input().getTicks(), viewPosition));

	controller->setViewCenter(model->getMapViewCenter() + viewPosition, model->getLevel());
}

void MapView::postSwipe(uint32_t msPassed)
{
	if(!actions->dragActive)
	{
		if(swipeHistory.size() > 1)
		{
			Point diff = Point(0, 0);
			std::pair<uint32_t, Point> firstAccepted;
			uint32_t now = ENGINE->input().getTicks();
			for (auto & x : swipeHistory) {
				if(now - x.first < postSwipeCatchIntervalMs) { // only the last x ms are caught
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

void MapView::onMapZoomLevelChanged(int stepsChange, bool useDeadZone)
{
	controller->modifyTileSize(stepsChange, useDeadZone);
}

void MapView::onViewMapActivated()
{
	controller->activateAdventureContext();

	int zoom = settings["adventure"]["tileZoom"].Integer();
	if(zoom)
		controller->setTileSize(Point(zoom, zoom));
	else
		controller->setTileSize(Point(32, 32));
}

PuzzleMapView::PuzzleMapView(const Point & offset, const Point & dimensions, const int3 & tileToCenter)
	: BasicMapView(offset, dimensions, false)
{
	controller->activatePuzzleMapContext(tileToCenter);
	controller->setViewCenter(tileToCenter);

}
