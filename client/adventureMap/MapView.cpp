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

MapCache::MapCache(const Point & tileSize, const Point & dimensions)
	: tileSize(tileSize)
	, context(new MapRendererContext())
	, mapRenderer(new MapRenderer(*context))
	, targetDimensionsPixels(dimensions)
	, viewCenter(0, 0)
	, mapLevel(0)
{
	// total number of potentially visible tiles is:
	// 1) number of completely visible tiles
	// 2) additional tile that might be partially visible from left/top size
	// 3) additional tile that might be partially visible from right/bottom size
	Point visibleTiles{
		dimensions.x / tileSize.x + 2,
		dimensions.y / tileSize.y + 2,
	};

	viewDimensionsTiles = visibleTiles;
	viewDimensionsPixels = visibleTiles * tileSize;

	terrain = std::make_unique<Canvas>(viewDimensionsPixels);
}

void MapCache::setViewCenter(const Point & center, int newLevel)
{
	viewCenter = center;
	mapLevel = newLevel;

	int3 mapSize = LOCPLINT->cb->getMapSize();
	Point viewMax = Point(mapSize) * tileSize;

	vstd::abetween(viewCenter.x, 0, viewMax.x);
	vstd::abetween(viewCenter.y, 0, viewMax.y);
}

Canvas MapCache::getTile(const int3 & coordinates)
{
	assert(mapLevel == coordinates.z);
	assert(viewDimensionsTiles.x + coordinates.x >= 0);
	assert(viewDimensionsTiles.y + coordinates.y >= 0);

	Point tileIndex{
		(viewDimensionsTiles.x + coordinates.x) % viewDimensionsTiles.x,
		(viewDimensionsTiles.y + coordinates.y) % viewDimensionsTiles.y
	};

	Rect terrainSection(tileIndex * tileSize, tileSize);

	return Canvas(*terrain, terrainSection);
}

void MapCache::updateTile(const int3 & coordinates)
{
	Canvas target = getTile(coordinates);

	mapRenderer->renderTile(*context, target, coordinates);
}

Rect MapCache::getVisibleAreaTiles() const
{
	Rect visibleAreaPixels = {
		viewCenter.x - targetDimensionsPixels.x / 2,
		viewCenter.y - targetDimensionsPixels.y / 2,
		targetDimensionsPixels.x,
		targetDimensionsPixels.y
	};

	// NOTE: use division via double in order to use floor (which rounds to negative infinity and not towards zero)
	Point topLeftTile{
		static_cast<int>(std::floor(static_cast<double>(visibleAreaPixels.left()) / tileSize.x)),
		static_cast<int>(std::floor(static_cast<double>(visibleAreaPixels.top()) / tileSize.y)),
	};

	Point bottomRightTile{
		visibleAreaPixels.right() / tileSize.x,
		visibleAreaPixels.bottom() / tileSize.y
	};

	return Rect(topLeftTile, bottomRightTile - topLeftTile + Point(1, 1));
}

int3 MapCache::getTileAtPoint(const Point & position) const
{
	Point topLeftOffset = viewCenter - targetDimensionsPixels / 2;

	Point absolutePosition = position + topLeftOffset;

	return {
		static_cast<int>(std::floor(static_cast<double>(absolutePosition.x) / tileSize.x)),
		static_cast<int>(std::floor(static_cast<double>(absolutePosition.y) / tileSize.y)),
		mapLevel
	};
}

int3 MapCache::getTileCenter() const
{
	return getTileAtPoint(getViewCenter());
}

Point MapCache::getViewCenter() const
{
	return viewCenter;
}

void MapCache::update(uint32_t timeDelta)
{
	context->advanceAnimations(timeDelta);

	Rect dimensions = getVisibleAreaTiles();

	for(int y = dimensions.top(); y < dimensions.bottom(); ++y)
		for(int x = dimensions.left(); x < dimensions.right(); ++x)
			updateTile({x, y, mapLevel});
}

void MapCache::render(Canvas & target)
{
	update(GH.mainFPSmng->getElapsedMilliseconds());

	Rect dimensions = getVisibleAreaTiles();

	for(int y = dimensions.top(); y < dimensions.bottom(); ++y)
	{
		for(int x = dimensions.left(); x < dimensions.right(); ++x)
		{
			Point topLeftOffset = viewCenter - targetDimensionsPixels / 2;
			Point tilePosAbsolute = Point(x, y) * tileSize;
			Point tilePosRelative = tilePosAbsolute - topLeftOffset;

			Canvas source = getTile(int3(x, y, mapLevel));
			target.draw(source, tilePosRelative);
		}
	}
}

MapView::MapView(const Point & offset, const Point & dimensions)
	: tilesCache(new MapCache(Point(32, 32), dimensions))
	, tileSize(Point(32, 32))
{
	pos += offset;
	pos.w = dimensions.x;
	pos.h = dimensions.y;
}

Rect MapView::getVisibleAreaTiles() const
{
	return tilesCache->getVisibleAreaTiles();
}

int3 MapView::getTileCenter() const
{
	return tilesCache->getTileCenter();
}

int3 MapView::getTileAtPoint(const Point & position) const
{
	return tilesCache->getTileAtPoint(position);
}

Point MapView::getViewCenter() const
{
	return tilesCache->getViewCenter();
}

void MapView::setViewCenter(const int3 & position)
{
	setViewCenter(Point(position.x, position.y) * tileSize, position.z);
}

void MapView::setViewCenter(const Point & position, int level)
{
	tilesCache->setViewCenter(position, level);
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
	return CGI->mh->map->getTile(coordinates);
}

MapRendererContext::ObjectsVector MapRendererContext::getAllObjects() const
{
	return CGI->mh->map->objects;
}

const CGObjectInstance * MapRendererContext::getObject(ObjectInstanceID objectID) const
{
	return CGI->mh->map->objects.at(objectID.getNum());
}

bool MapRendererContext::isVisible(const int3 & coordinates) const
{
	return LOCPLINT->cb->isVisible(coordinates) || settings["session"]["spectate"].Bool();
}

MapRendererContext::VisibilityMap MapRendererContext::getVisibilityMap() const
{
	return LOCPLINT->cb->getVisibilityMap();
}

uint32_t MapRendererContext::getAnimationPeriod() const
{
	// H3 timing for adventure map objects animation is 180 ms
	// Terrain animations also use identical interval, however it is only present in HotA and/or HD Mod
	// TODO: duration of fade-in/fade-out for teleport, entering/leaving boat, removal of objects
	// TOOD: duration of hero movement animation, frame timing of hero movement animation, effect of hero speed option
	// TOOD: duration of enemy hero movement animation, frame timing of enemy hero movement animation, effect of enemy hero speed option
	return 180;
}

uint32_t MapRendererContext::getAnimationTime() const
{
	return animationTime;
}

Point MapRendererContext::tileSize() const
{
	return Point(32, 32);
}

bool MapRendererContext::showGrid() const
{
	return true; // settings["session"]["showGrid"].Bool();
}
