/*
 * MapViewCache.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "MapViewCache.h"

#include "IMapRendererContext.h"
#include "MapRenderer.h"
#include "MapViewModel.h"

#include "../render/CAnimation.h"
#include "../render/Canvas.h"
#include "../render/IImage.h"

#include "../../lib/mapObjects/CObjectHandler.h"

MapViewCache::~MapViewCache() = default;

MapViewCache::MapViewCache(const std::shared_ptr<MapViewModel> & model)
	: model(model)
	, cachedLevel(0)
	, mapRenderer(new MapRenderer())
	, iconsStorage(new CAnimation("VwSymbol"))
	, intermediate(new Canvas(Point(32, 32)))
	, terrain(new Canvas(model->getCacheDimensionsPixels()))
	, terrainTransition(new Canvas(model->getPixelsVisibleDimensions()))
{
	iconsStorage->preload();
	for(size_t i = 0; i < iconsStorage->size(); ++i)
		iconsStorage->getImage(i)->setBlitMode(EImageBlitMode::COLORKEY);

	Point visibleSize = model->getTilesVisibleDimensions();
	terrainChecksum.resize(boost::extents[visibleSize.x][visibleSize.y]);
	tilesUpToDate.resize(boost::extents[visibleSize.x][visibleSize.y]);
}

Canvas MapViewCache::getTile(const int3 & coordinates)
{
	return Canvas(*terrain, model->getCacheTileArea(coordinates));
}

std::shared_ptr<IImage> MapViewCache::getOverlayImageForTile(const std::shared_ptr<IMapRendererContext> & context, const int3 & coordinates)
{
	size_t imageIndex = context->overlayImageIndex(coordinates);

	if(imageIndex < iconsStorage->size())
		return iconsStorage->getImage(imageIndex);
	return nullptr;
}

void MapViewCache::invalidate(const std::shared_ptr<IMapRendererContext> & context, const ObjectInstanceID & object)
{
	for(size_t cacheY = 0; cacheY < terrainChecksum.shape()[1]; ++cacheY)
	{
		for(size_t cacheX = 0; cacheX < terrainChecksum.shape()[0]; ++cacheX)
		{
			auto & entry = terrainChecksum[cacheX][cacheY];

			int3 tile(entry.tileX, entry.tileY, cachedLevel);

			if(context->isInMap(tile) && vstd::contains(context->getObjects(tile), object))
				entry = TileChecksum{};
		}
	}
}

void MapViewCache::updateTile(const std::shared_ptr<IMapRendererContext> & context, const int3 & coordinates)
{
	int cacheX = (terrainChecksum.shape()[0] + coordinates.x) % terrainChecksum.shape()[0];
	int cacheY = (terrainChecksum.shape()[1] + coordinates.y) % terrainChecksum.shape()[1];

	auto & oldCacheEntry = terrainChecksum[cacheX][cacheY];
	TileChecksum newCacheEntry;

	newCacheEntry.tileX = coordinates.x;
	newCacheEntry.tileY = coordinates.y;
	newCacheEntry.checksum = mapRenderer->getTileChecksum(*context, coordinates);

	if(cachedLevel == coordinates.z && oldCacheEntry == newCacheEntry && !context->tileAnimated(coordinates))
		return;

	Canvas target = getTile(coordinates);

	if(model->getSingleTileSize() == Point(32, 32))
	{
		mapRenderer->renderTile(*context, target, coordinates);
	}
	else
	{
		mapRenderer->renderTile(*context, *intermediate, coordinates);
		target.drawScaled(*intermediate, Point(0, 0), model->getSingleTileSize());
	}

	if(context->filterGrayscale())
		target.applyGrayscale();

	oldCacheEntry = newCacheEntry;
	tilesUpToDate[cacheX][cacheY] = false;
}

void MapViewCache::update(const std::shared_ptr<IMapRendererContext> & context)
{
	Rect dimensions = model->getTilesTotalRect();
	bool mapResized = cachedSize != model->getSingleTileSize();

	if(mapResized || dimensions.w != terrainChecksum.shape()[0] || dimensions.h != terrainChecksum.shape()[1])
	{
		boost::multi_array<TileChecksum, 2> newCache;
		newCache.resize(boost::extents[dimensions.w][dimensions.h]);
		terrainChecksum.resize(boost::extents[dimensions.w][dimensions.h]);
		terrainChecksum = newCache;
	}

	if(mapResized || dimensions.w != tilesUpToDate.shape()[0] || dimensions.h != tilesUpToDate.shape()[1])
	{
		boost::multi_array<bool, 2> newCache;
		newCache.resize(boost::extents[dimensions.w][dimensions.h]);
		tilesUpToDate.resize(boost::extents[dimensions.w][dimensions.h]);
		tilesUpToDate = newCache;
	}

	for(int y = dimensions.top(); y < dimensions.bottom(); ++y)
		for(int x = dimensions.left(); x < dimensions.right(); ++x)
			updateTile(context, {x, y, model->getLevel()});

	cachedSize = model->getSingleTileSize();
	cachedLevel = model->getLevel();
}

void MapViewCache::render(const std::shared_ptr<IMapRendererContext> & context, Canvas & target, bool fullRedraw)
{
	bool mapMoved = (cachedPosition != model->getMapViewCenter());
	bool lazyUpdate = !mapMoved && !fullRedraw && context->viewTransitionProgress() == 0;

	Rect dimensions = model->getTilesTotalRect();

	for(int y = dimensions.top(); y < dimensions.bottom(); ++y)
	{
		for(int x = dimensions.left(); x < dimensions.right(); ++x)
		{
			int cacheX = (terrainChecksum.shape()[0] + x) % terrainChecksum.shape()[0];
			int cacheY = (terrainChecksum.shape()[1] + y) % terrainChecksum.shape()[1];
			int3 tile(x, y, model->getLevel());

			if(lazyUpdate && tilesUpToDate[cacheX][cacheY])
				continue;

			Canvas source = getTile(tile);
			Rect targetRect = model->getTargetTileArea(tile);
			target.draw(source, targetRect.topLeft());

			if (!fullRedraw)
				tilesUpToDate[cacheX][cacheY] = true;
		}
	}

	if(context->showOverlay())
	{
		for(int y = dimensions.top(); y < dimensions.bottom(); ++y)
		{
			for(int x = dimensions.left(); x < dimensions.right(); ++x)
			{
				int3 tile(x, y, model->getLevel());
				Rect targetRect = model->getTargetTileArea(tile);
				auto overlay = getOverlayImageForTile(context, tile);

				if(overlay)
				{
					Point position = targetRect.center() - overlay->dimensions() / 2;
					target.draw(overlay, position);
				}
			}
		}
	}

	if(context->viewTransitionProgress() != 0)
		target.drawTransparent(*terrainTransition, Point(0, 0), 1.0 - context->viewTransitionProgress());

	cachedPosition = model->getMapViewCenter();
}

void MapViewCache::createTransitionSnapshot(const std::shared_ptr<IMapRendererContext> & context)
{
	update(context);
	render(context, *terrainTransition, true);
}
