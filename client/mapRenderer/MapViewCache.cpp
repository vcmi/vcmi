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

#include "MapRenderer.h"
#include "MapViewModel.h"

#include "../render/CAnimation.h"
#include "../render/Canvas.h"
#include "../render/IImage.h"

#include "../../lib/mapObjects/CObjectHandler.h"

MapViewCache::~MapViewCache() = default;

MapViewCache::MapViewCache(const std::shared_ptr<MapViewModel> & model)
	: model(model)
	, mapRenderer(new MapRenderer())
	, iconsStorage(new CAnimation("VwSymbol"))
	, intermediate(new Canvas(Point(32,32)))
	, terrain(new Canvas(model->getCacheDimensionsPixels()))
{
	iconsStorage->preload();
	for (size_t i = 0; i < iconsStorage->size(); ++i)
		iconsStorage->getImage(i)->setBlitMode(EImageBlitMode::COLORKEY);

}

Canvas MapViewCache::getTile(const int3 & coordinates)
{
	return Canvas(*terrain, model->getCacheTileArea(coordinates));
}

std::shared_ptr<IImage> MapViewCache::getOverlayImageForTile(const std::shared_ptr<MapRendererContext> & context, const int3 & coordinates)
{
	if (!context->isVisible(coordinates))
		return nullptr;

	for(const auto & objectID : context->getObjects(coordinates))
	{
		const auto * object = context->getObject(objectID);

		if (!object->visitableAt(coordinates.x, coordinates.y))
			continue;

		size_t ownerIndex = PlayerColor::PLAYER_LIMIT.getNum() * static_cast<size_t>(EWorldViewIcon::ICONS_PER_PLAYER);
		if (object->tempOwner.isValidPlayer())
			ownerIndex = object->tempOwner.getNum() * static_cast<size_t>(EWorldViewIcon::ICONS_PER_PLAYER);

		switch (object->ID)
		{
			case Obj::MONOLITH_ONE_WAY_ENTRANCE:
			case Obj::MONOLITH_ONE_WAY_EXIT:
			case Obj::MONOLITH_TWO_WAY:
				return iconsStorage->getImage(ownerIndex + static_cast<size_t>(EWorldViewIcon::TELEPORT));
			case Obj::SUBTERRANEAN_GATE:
				return iconsStorage->getImage(ownerIndex + static_cast<size_t>(EWorldViewIcon::GATE));
			case Obj::ARTIFACT:
				return iconsStorage->getImage(ownerIndex + static_cast<size_t>(EWorldViewIcon::ARTIFACT));
			case Obj::TOWN:
				return iconsStorage->getImage(ownerIndex + static_cast<size_t>(EWorldViewIcon::TOWN));
			case Obj::HERO:
				return iconsStorage->getImage(ownerIndex + static_cast<size_t>(EWorldViewIcon::HERO));
			case Obj::MINE:
				return iconsStorage->getImage(ownerIndex + static_cast<size_t>(EWorldViewIcon::MINE_WOOD) + object->subID);
			case Obj::RESOURCE:
				return iconsStorage->getImage(ownerIndex + static_cast<size_t>(EWorldViewIcon::RES_WOOD) + object->subID);
		}
	}
	return nullptr;
}

void MapViewCache::updateTile(const std::shared_ptr<MapRendererContext> & context, const int3 & coordinates)
{
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
}

void MapViewCache::update(const std::shared_ptr<MapRendererContext> & context)
{
	Rect dimensions = model->getTilesTotalRect();

	for(int y = dimensions.top(); y < dimensions.bottom(); ++y)
		for(int x = dimensions.left(); x < dimensions.right(); ++x)
			updateTile(context, {x, y, model->getLevel()});
}

void MapViewCache::render(const std::shared_ptr<MapRendererContext> & context, Canvas & target)
{
	Rect dimensions = model->getTilesTotalRect();

	for(int y = dimensions.top(); y < dimensions.bottom(); ++y)
	{
		for(int x = dimensions.left(); x < dimensions.right(); ++x)
		{
			int3 tile(x, y, model->getLevel());
			Canvas source = getTile(tile);
			Rect targetRect = model->getTargetTileArea(tile);
			target.draw(source, targetRect.topLeft());

			if (context->showOverlay())
			{
				auto overlay = getOverlayImageForTile(context, tile);

				if (overlay)
				{
					Point position = targetRect.center() - overlay->dimensions() / 2;
					target.draw(overlay, position);
				}
			}
		}
	}
}
