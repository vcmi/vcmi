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
#include "../adventureMap/CAdvMapInt.h"
#include "../gui/CGuiHandler.h"
#include "../render/CAnimation.h"
#include "../render/Canvas.h"
#include "../render/IImage.h"
#include "../renderSDL/SDL_Extensions.h"

#include "../../CCallback.h"

#include "../../lib/CConfigHandler.h"
#include "../../lib/mapObjects/CGHeroInstance.h"
#include "../../lib/mapping/CMap.h"

MapView::~MapView() = default;

std::shared_ptr<MapViewModel> MapView::createModel(const Point & dimensions) const
{
	auto result = std::make_shared<MapViewModel>();

	result->setLevel(0);
	result->setTileSize(Point(32, 32));
	result->setViewCenter(Point(0, 0));
	result->setViewDimensions(dimensions);

	return result;
}

MapView::MapView(const Point & offset, const Point & dimensions)
	: model(createModel(dimensions))
	, controller(new MapViewController(model))
	, tilesCache(new MapViewCache(model))
	, isSwiping(false)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL_NO_DISPOSE;
	pos += offset;
	pos.w = dimensions.x;
	pos.h = dimensions.y;

	actions = std::make_shared<MapViewActions>(*this, model);
	actions->setContext(controller->getContext());
}

void MapView::render(Canvas & target, bool fullUpdate)
{
	Canvas targetClipped(target, pos);

	controller->update(GH.mainFPSmng->getElapsedMilliseconds());
	tilesCache->update(controller->getContext());
	tilesCache->render(controller->getContext(), targetClipped, fullUpdate);
}

void MapView::show(SDL_Surface * to)
{
	Canvas target(to);
	CSDL_Ext::CClipRectGuard guard(to, pos);
	render(target, false);
}

void MapView::showAll(SDL_Surface * to)
{
	Canvas target(to);
	CSDL_Ext::CClipRectGuard guard(to, pos);
	render(target, true);
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
	if(!isSwiping)
		controller->setViewCenter(model->getMapViewCenter() + distance, model->getLevel());
}

void MapView::onMapSwiped(const Point & viewPosition)
{
	isSwiping = true;
	controller->setViewCenter(viewPosition, model->getLevel());
}

void MapView::onMapSwipeEnded()
{
	isSwiping = true;
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
	controller->setTileSize(Point(tileSize, tileSize));
	controller->setOverlayVisibility(objectPositions);
	controller->setTerrainVisibility(showTerrain);
}

void MapView::onViewWorldActivated(uint32_t tileSize)
{
	controller->setTileSize(Point(tileSize, tileSize));
}

void MapView::onViewMapActivated()
{
	controller->setTileSize(Point(32, 32));
	controller->setOverlayVisibility({});
	controller->setTerrainVisibility(false);
}
