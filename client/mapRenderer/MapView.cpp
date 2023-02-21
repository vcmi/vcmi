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

#include "MapViewModel.h"
#include "MapViewCache.h"
#include "MapViewController.h"
#include "mapHandler.h"

#include "../adventureMap/CAdvMapInt.h"
#include "../CGameInfo.h"
#include "../CPlayerInterface.h"
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
{
	pos += offset;
	pos.w = dimensions.x;
	pos.h = dimensions.y;
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

std::shared_ptr<const MapViewModel> MapView::getModel() const
{
	return model;
}

std::shared_ptr<MapViewController> MapView::getController()
{
	return controller;
}
