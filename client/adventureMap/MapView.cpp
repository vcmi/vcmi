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

#include "MapRenderer.h"
#include "mapHandler.h"
#include "CAdvMapInt.h"

#include "../CGameInfo.h"
#include "../CMusicHandler.h"
#include "../CPlayerInterface.h"
#include "../gui/CGuiHandler.h"
#include "../render/CAnimation.h"
#include "../render/CFadeAnimation.h"
#include "../render/Canvas.h"
#include "../render/Colors.h"
#include "../render/Graphics.h"
#include "../render/IImage.h"
#include "../renderSDL/SDL_Extensions.h"

#include "../../CCallback.h"

#include "../../lib/CConfigHandler.h"
#include "../../lib/CGeneralTextHandler.h"
#include "../../lib/CRandomGenerator.h"
#include "../../lib/CStopWatch.h"
#include "../../lib/Color.h"
#include "../../lib/RiverHandler.h"
#include "../../lib/RoadHandler.h"
#include "../../lib/TerrainHandler.h"
#include "../../lib/mapObjects/CGHeroInstance.h"
#include "../../lib/mapObjects/CObjectClassesHandler.h"
#include "../../lib/mapping/CMap.h"

MapCache::~MapCache() = default;

MapCache::MapCache(const std::shared_ptr<MapViewModel> & model)
	: model(model)
	, context(new MapRendererContext())
	, mapRenderer(new MapRenderer(*context))
{
	terrain = std::make_unique<Canvas>(model->getCacheDimensionsPixels());
}

Canvas MapCache::getTile(const int3 & coordinates)
{
	return Canvas(*terrain, model->getCacheTileArea(coordinates));
}

void MapCache::updateTile(const int3 & coordinates)
{
	Canvas target = getTile(coordinates);

	mapRenderer->renderTile(*context, target, coordinates);
}

void MapCache::update(uint32_t timeDelta)
{
	context->advanceAnimations(timeDelta);
	context->setTileSize(model->getSingleTileSize());

	Rect dimensions = model->getTilesTotalRect();

	for(int y = dimensions.top(); y < dimensions.bottom(); ++y)
		for(int x = dimensions.left(); x < dimensions.right(); ++x)
			updateTile({x, y, model->getLevel()});
}

void MapCache::render(Canvas & target)
{
	update(GH.mainFPSmng->getElapsedMilliseconds());

	Rect dimensions = model->getTilesTotalRect();

	for(int y = dimensions.top(); y < dimensions.bottom(); ++y)
	{
		for(int x = dimensions.left(); x < dimensions.right(); ++x)
		{
			int3 tile( x,y,model->getLevel());
			Canvas source = getTile(tile);
			Rect targetRect = model->getTargetTileArea(tile);
			target.draw(source, targetRect.topLeft());
		}
	}
}

std::shared_ptr<MapViewModel> MapView::createModel(const Point & dimensions) const
{
	auto result = std::make_shared<MapViewModel>();

	result->setLevel(0);
	result->setTileSize(Point(32,32));
	result->setViewCenter(Point(0,0));
	result->setViewDimensions(dimensions);

	return result;
}

MapView::MapView(const Point & offset, const Point & dimensions)
	: model(createModel(dimensions))
	, tilesCache(new MapCache(model))
{
	pos += offset;
	pos.w = dimensions.x;
	pos.h = dimensions.y;

}

void MapView::show(SDL_Surface * to)
{
	Canvas target(to);
	Canvas targetClipped(target, pos);

	CSDL_Ext::CClipRectGuard guard(to, pos);

	tilesCache->render(targetClipped);
}

void MapView::showAll(SDL_Surface * to)
{
	show(to);
}

void MapRendererContext::advanceAnimations(uint32_t ms)
{
	animationTime += ms;
}

int3 MapRendererContext::getMapSize() const
{
	return LOCPLINT->cb->getMapSize();
}

bool MapRendererContext::isInMap(const int3 & coordinates) const
{
	return LOCPLINT->cb->isInTheMap(coordinates);
}

const TerrainTile & MapRendererContext::getMapTile(const int3 & coordinates) const
{
	return CGI->mh->getMap()->getTile(coordinates);
}

MapRendererContext::ObjectsVector MapRendererContext::getAllObjects() const
{
	return CGI->mh->getMap()->objects;
}

const CGObjectInstance * MapRendererContext::getObject(ObjectInstanceID objectID) const
{
	return CGI->mh->getMap()->objects.at(objectID.getNum());
}

bool MapRendererContext::isVisible(const int3 & coordinates) const
{
	return LOCPLINT->cb->isVisible(coordinates) || settings["session"]["spectate"].Bool();
}

void MapRendererContext::setTileSize(const Point & dimensions)
{
	tileSize = dimensions;
}

const CGPath * MapRendererContext::currentPath() const
{
	const auto * hero = adventureInt->curHero();

	if(!hero)
		return nullptr;

	if(!LOCPLINT->paths.hasPath(hero))
		return nullptr;

	return &LOCPLINT->paths.getPath(hero);
}

uint32_t MapRendererContext::getAnimationPeriod() const
{
	// H3 timing for adventure map objects animation is 180 ms
	// Terrain animations also use identical interval, however those are only present in HotA and/or HD Mod
	// TODO: duration of fade-in/fade-out for teleport, entering/leaving boat, removal of objects
	// TOOD: duration of hero movement animation, frame timing of hero movement animation, effect of hero speed option
	// TOOD: duration of enemy hero movement animation, frame timing of enemy hero movement animation, effect of enemy hero speed option
	return 180;
}

uint32_t MapRendererContext::getAnimationTime() const
{
	return animationTime;
}

Point MapRendererContext::getTileSize() const
{
	return Point(32, 32);
}

bool MapRendererContext::showGrid() const
{
	return true; // settings["session"]["showGrid"].Bool();
}

void MapView::setViewCenter(const int3 & position)
{
	model->setViewCenter(Point(position.x, position.y) * model->getSingleTileSize());
	model->setLevel(position.z);
}

void MapView::setViewCenter(const Point & position, int level)
{
	model->setViewCenter(position);
	model->setLevel(level);
}

void MapView::setTileSize(const Point & tileSize)
{
	model->setTileSize(tileSize);
}

std::shared_ptr<const MapViewModel> MapView::getModel() const
{
	return model;
}

void MapViewModel::setTileSize(const Point & newValue)
{
	tileSize = newValue;
}

void MapViewModel::setViewCenter(const Point & newValue)
{
	viewCenter = newValue;
}

void MapViewModel::setViewDimensions(const Point & newValue)
{
	viewDimensions = newValue;
}

void MapViewModel::setLevel(int newLevel)
{
	mapLevel = newLevel;
}

Point MapViewModel::getSingleTileSize() const
{
	return tileSize;
}

Point MapViewModel::getMapViewCenter() const
{
	return viewCenter;
}

Point MapViewModel::getPixelsVisibleDimensions() const
{
	return viewDimensions;
}

int MapViewModel::getLevel() const
{
	return mapLevel;
}

Point MapViewModel::getTilesVisibleDimensions() const
{
	// total number of potentially visible tiles is:
	// 1) number of completely visible tiles
	// 2) additional tile that might be partially visible from left/top size
	// 3) additional tile that might be partially visible from right/bottom size
	return {
		getPixelsVisibleDimensions().x / getSingleTileSize().x + 2,
		getPixelsVisibleDimensions().y / getSingleTileSize().y + 2,
	};
}

Rect MapViewModel::getTilesTotalRect() const
{
	return Rect(
		Point(getTileAtPoint(Point(0,0))),
		getTilesVisibleDimensions()
	);
}

int3 MapViewModel::getTileCenter() const
{
	return getTileAtPoint(getMapViewCenter());
}

int3 MapViewModel::getTileAtPoint(const Point & position) const
{
	Point topLeftOffset = getMapViewCenter() - getPixelsVisibleDimensions() / 2;

	Point absolutePosition = position + topLeftOffset;

	// NOTE: using division via double in order to use std::floor
	// which rounds to negative infinity and not towards zero (like integer division)
	return {
		static_cast<int>(std::floor(static_cast<double>(absolutePosition.x) / getSingleTileSize().x)),
		static_cast<int>(std::floor(static_cast<double>(absolutePosition.y) / getSingleTileSize().y)),
		getLevel()
	};
}

Point MapViewModel::getCacheDimensionsPixels() const
{
	return getTilesVisibleDimensions() * getSingleTileSize();
}

Rect MapViewModel::getCacheTileArea(const int3 & coordinates) const
{
	assert(mapLevel == coordinates.z);
	assert(getTilesVisibleDimensions().x + coordinates.x >= 0);
	assert(getTilesVisibleDimensions().y + coordinates.y >= 0);

	Point tileIndex{
		(getTilesVisibleDimensions().x + coordinates.x) % getTilesVisibleDimensions().x,
		(getTilesVisibleDimensions().y + coordinates.y) % getTilesVisibleDimensions().y
	};

	return Rect(tileIndex * tileSize, tileSize);
}

Rect MapViewModel::getTargetTileArea(const int3 & coordinates) const
{
	Point topLeftOffset = getMapViewCenter() - getPixelsVisibleDimensions() / 2;
	Point tilePosAbsolute = Point(coordinates) * tileSize;
	Point tilePosRelative = tilePosAbsolute - topLeftOffset;

	return {
		tilePosRelative,
		tileSize
	};
}
