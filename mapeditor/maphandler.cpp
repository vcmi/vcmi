/*
 * maphandler.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

//code is copied from vcmiclient/mapHandler.cpp with minimal changes
#include "StdInc.h"
#include "maphandler.h"
#include "graphics.h"
#include "../lib/RoadHandler.h"
#include "../lib/RiverHandler.h"
#include "../lib/TerrainHandler.h"
#include "../lib/mapping/CMap.h"
#include "../lib/mapObjects/CGHeroInstance.h"
#include "../lib/mapObjects/ObjectTemplate.h"
#include "../lib/mapObjects/MapObjectDrawOrder.h"
#include "../lib/mapObjects/MiscObjects.h"
#include "../lib/GameConstants.h"
#include "../lib/IGameSettings.h"

namespace
{
const int tileSize = 32;

/// Palette entries 1-4 and 6-7 of the def format are shadow, 5 is the owner color
bool isShadowColor(int index, QRgb color)
{
	return index > 0 && index < 8 && index != 5 && qAlpha(color) > 0 && qAlpha(color) < 255;
}

QImage flippedImage(const std::shared_ptr<QImage> & image, ui8 rotationFlags)
{
	const ui8 rotation = rotationFlags % 4;
	const bool hflip = rotation & 0b01;
	const bool vflip = rotation & 0b10;

#if QT_VERSION >= QT_VERSION_CHECK(6, 9, 0)
	Qt::Orientations orientations;
	if(hflip)
		orientations |= Qt::Horizontal;
	if(vflip)
		orientations |= Qt::Vertical;
	return image->flipped(orientations);
#else
	return image->mirrored(hflip, vflip);
#endif
}
}

int MapHandler::index(int x, int y, int z) const
{
	return z * (map->width * map->height) + y * map->width + x;
}

int MapHandler::index(const int3 & p) const
{
	return index(p.x, p.y, p.z);
}

MapHandler::MapHandler()
{
	initTerrainGraphics();
	logGlobal->info("\tPreparing terrain, roads, rivers, borders");
}

void MapHandler::reset(const CMap * Map)
{
	map = Map;
	splitImages.clear();
	initObjectRects();
	logGlobal->info("\tMaking object rects");
}

void MapHandler::initTerrainGraphics()
{
	auto loadFlipped = [](TFlippedAnimations & animation, TFlippedCache & cache, const std::map<std::string, std::string> & files)
	{
		for(auto & type : files)
		{
			animation[type.first] = std::make_unique<Animation>(type.second);
			animation[type.first]->preload();
			const size_t views = animation[type.first]->size(0);
			cache[type.first].resize(views);

			for(int j = 0; j < views; j++)
				cache[type.first][j] = animation[type.first]->getImage(j);
		}
	};

	std::map<std::string, std::string> terrainFiles;
	std::map<std::string, std::string> roadFiles;
	std::map<std::string, std::string> riverFiles;
	for(const auto & terrain : LIBRARY->terrainTypeHandler->objects)
	{
		terrainFiles[terrain->getJsonKey()] = terrain->tilesFilename.getName();
	}
	for(const auto & river : LIBRARY->riverTypeHandler->objects)
	{
		riverFiles[river->getJsonKey()] = river->tilesFilename.getName();
	}
	for(const auto & road : LIBRARY->roadTypeHandler->objects)
	{
		roadFiles[road->getJsonKey()] = road->tilesFilename.getName();
	}

	loadFlipped(terrainAnimations, terrainImages, terrainFiles);
	loadFlipped(riverAnimations, riverImages, riverFiles);
	loadFlipped(roadAnimations, roadImages, roadFiles);
}

void MapHandler::drawTerrainTile(QPainter & painter, int x, int y, int z, QPointF offset)
{
	const auto & tinfo = map->getTile(int3(x, y, z));

	auto terrainName = tinfo.getTerrain()->getJsonKey();
	if(terrainImages.at(terrainName).size() <= tinfo.terView)
		return;
	painter.drawImage(x * tileSize - offset.x(), y * tileSize - offset.y(), flippedImage(terrainImages.at(terrainName)[tinfo.terView], tinfo.extTileFlags));
}

void MapHandler::drawRoad(QPainter & painter, int x, int y, int z, QPointF offset)
{
	const auto & tinfo = map->getTile(int3(x, y, z));
	auto * tinfoUpper = map->isInTheMap(int3(x, y - 1, z)) ? &map->getTile(int3(x, y - 1, z)) : nullptr;

	if(tinfoUpper && tinfoUpper->roadType)
	{
		auto roadName = tinfoUpper->getRoad()->getJsonKey();
		if(roadImages.at(roadName).size() > tinfoUpper->roadDir)
		{
			const QRect source{0, tileSize / 2, tileSize, tileSize / 2};
			const ui8 rotationFlags = tinfoUpper->extTileFlags >> 4;
			painter.drawImage(QPoint(x * tileSize - offset.x(), y * tileSize - offset.y()), flippedImage(roadImages.at(roadName)[tinfoUpper->roadDir], rotationFlags), source);
		}
	}

	if(tinfo.roadType) //print road from this tile
	{
		auto roadName = tinfo.getRoad()->getJsonKey();
		if(roadImages.at(roadName).size() > tinfo.roadDir)
		{
			const QRect source{0, 0, tileSize, tileSize / 2};
			const ui8 rotationFlags = tinfo.extTileFlags >> 4;
			painter.drawImage(QPoint(x * tileSize - offset.x(), y * tileSize + tileSize / 2 - offset.y()), flippedImage(roadImages.at(roadName)[tinfo.roadDir], rotationFlags), source);
		}
	}
}

void MapHandler::drawRiver(QPainter & painter, int x, int y, int z, QPointF offset)
{
	const auto & tinfo = map->getTile(int3(x, y, z));

	if(!tinfo.hasRiver())
		return;

	//TODO: use ui8 instead of string key
	auto riverName = tinfo.getRiver()->getJsonKey();

	if(riverImages.at(riverName).size() <= tinfo.riverDir)
		return;

	const ui8 rotationFlags = tinfo.extTileFlags >> 2;
	painter.drawImage(x * tileSize - offset.x(), y * tileSize - offset.y(), flippedImage(riverImages.at(riverName)[tinfo.riverDir], rotationFlags));
}

void setPlayerColor(QImage * sur, PlayerColor player)
{
	if(player == PlayerColor::UNFLAGGABLE)
		return;
	if(sur->format() == QImage::Format_Indexed8)
	{
		QRgb color = graphics->neutralColor;
		if(player != PlayerColor::NEUTRAL && player < PlayerColor::PLAYER_LIMIT)
			color = graphics->playerColors.at(player.getNum());

		sur->setColor(5, color);
	}
	else
		logGlobal->warn("Warning, setPlayerColor called on not 8bpp surface!");
}

std::shared_ptr<QImage> MapHandler::getObjectImage(const CGObjectInstance * obj)
{
	if(	!obj
	   || (obj->ID==Obj::HERO && dynamic_cast<const CGHeroInstance*>(obj)->isGarrisoned()) //garrisoned hero
	   || (obj->ID==Obj::BOAT && dynamic_cast<const CGBoat*>(obj)->getBoardedHero())) //boat with hero (hero graphics is used)
	{
		return nullptr;
	}

	std::shared_ptr<Animation> animation = graphics->getAnimation(obj);

	//no animation at all
	if(!animation)
		return nullptr;

	//empty animation
	if(animation->size(0) == 0)
		return nullptr;

	auto image = animation->getImage(0, obj->ID == Obj::HERO ? 2 : 0);
	if(!image)
	{
		//workaround for prisons
		image = animation->getImage(0, 0);
	}

	return image;
}

const MapHandler::SplitImage & MapHandler::getSplitImage(const std::shared_ptr<QImage> & image)
{
	auto it = splitImages.find(image.get());
	if(it != splitImages.end())
		return it->second;

	SplitImage split;
	split.source = image;

	// only indexed images have shadow in their palette, the others are all body
	if(image->format() == QImage::Format_Indexed8)
	{
		QVector<QRgb> shadowColors(image->colorCount(), qRgba(0, 0, 0, 0));
		QVector<QRgb> bodyColors = image->colorTable();
		bool hasShadow = false;

		for(int i = 0; i < bodyColors.size(); ++i)
		{
			if(isShadowColor(i, bodyColors[i]))
			{
				shadowColors[i] = bodyColors[i];
				bodyColors[i] = qRgba(0, 0, 0, 0);
				hasShadow = true;
			}
		}

		if(hasShadow)
		{
			split.shadow = std::make_shared<QImage>(*image);
			split.shadow->setColorTable(shadowColors);
			split.body = std::make_shared<QImage>(*image);
			split.body->setColorTable(bodyColors);
		}
	}
	return splitImages.emplace(image.get(), std::move(split)).first->second;
}

std::set<int3> MapHandler::removeObject(const CGObjectInstance *object)
{
	for(const auto & tile : stampedTiles[object])
		vstd::erase(orderedObjects[index(tile)], object);
	stampedTiles.erase(object);

	std::set<int3> result = tilesCache[object];
	for(auto & t : result)
	{
		auto & objects = getObjects(t);
		for(auto iter = objects.begin(); iter != objects.end(); ++iter)
		{
			if(iter->obj == object)
			{
				objects.erase(iter);
				break;
			}
		}
	}

	tilesCache.erase(object);
	return result;
}

std::vector<int3> MapHandler::getStampTiles(const CGObjectInstance * object) const
{
	std::vector<int3> result;

	if(!object || MapObjectDrawOrder::usesFixedDrawSlot(object))
		return result;

	// like in H3 every cell of an object takes part in ordering, even if nothing is drawn there
	for(int fx = 0; fx < object->getWidth(); ++fx)
	{
		for(int fy = 0; fy < object->getHeight(); ++fy)
		{
			int3 tile(object->anchorPos().x - fx, object->anchorPos().y - fy, object->anchorPos().z);

			if(map->isInTheMap(tile))
				result.push_back(tile);
		}
	}
	return result;
}

void MapHandler::stampObject(const CGObjectInstance * object)
{
	for(const auto & tile : getStampTiles(object))
	{
		auto & list = orderedObjects[index(tile)];
		list.insert(MapObjectDrawOrder::findInsertPosition(list, *map, object, tile, [](const CGObjectInstance * other) { return other; }), object);
		stampedTiles[object].push_back(tile);
	}
}

void MapHandler::restampTiles(const std::set<int3> & tiles)
{
	if(tiles.empty())
		return;

	int3 first = *tiles.begin();
	int3 last = first;
	for(const auto & tile : tiles)
	{
		orderedObjects[index(tile)].clear();
		first = int3(std::min(first.x, tile.x), std::min(first.y, tile.y), 0);
		last = int3(std::max(last.x, tile.x), std::max(last.y, tile.y), 0);
	}

	// stamped again in the order of objects on the map, like when the map is loaded
	for(const auto & object : map->objects)
	{
		if(!object || object->anchorPos().x < first.x || object->anchorPos().y < first.y
			|| object->anchorPos().x - object->getWidth() >= last.x || object->anchorPos().y - object->getHeight() >= last.y)
			continue;

		for(const auto & tile : getStampTiles(object.get()))
		{
			if(!tiles.count(tile))
				continue;

			auto & list = orderedObjects[index(tile)];
			list.insert(MapObjectDrawOrder::findInsertPosition(list, *map, object.get(), tile, [](const CGObjectInstance * other) { return other; }), object.get());
		}
	}
}

void MapHandler::sortTile(const int3 & tile)
{
	const auto & ordered = orderedObjects[index(tile)];
	const auto rank = [&](const CGObjectInstance * object) { return std::find(ordered.begin(), ordered.end(), object) - ordered.begin(); };

	auto & list = tileObjects[index(tile)];
	std::stable_sort(list.begin(), list.end(), [&](const ObjectRect & a, const ObjectRect & b) { return rank(a.obj) < rank(b.obj); });
}

std::set<int3> MapHandler::addObject(const CGObjectInstance * object)
{
	stampObject(object);

	auto image = getObjectImage(object);
	if(!image)
		return std::set<int3>{};

	for(int fx = 0; fx < object->getWidth(); ++fx)
	{
		for(int fy = 0; fy < object->getHeight(); ++fy)
		{
			int3 currTile(object->pos.x - fx, object->pos.y - fy, object->pos.z);
			QRect cr(image->width() - fx * tileSize - tileSize,
					 image->height() - fy * tileSize - tileSize,
					 tileSize,
					 tileSize);

			if( map->isInTheMap(currTile) && // within map
			   cr.x() + cr.width() > 0 &&    // image has data on this tile
			   cr.y() + cr.height() > 0)
			{
				getObjects(currTile).emplace_back(object, cr);
				tilesCache[object].insert(currTile);
			}
		}
	}

	return tilesCache[object];
}

void MapHandler::initObjectRects()
{
	tileObjects.clear();
	tilesCache.clear();
	orderedObjects.clear();
	stampedTiles.clear();
	if(!map)
		return;

	tileObjects.resize(map->width * map->height * map->levels());
	orderedObjects.resize(map->width * map->height * map->levels());

	//initializing objects / rects
	for(const auto & elem : map->objects)
	{
		addObject(elem.get());
	}

	for(int z = 0; z < map->levels(); ++z)
		for(int y = 0; y < map->height; ++y)
			for(int x = 0; x < map->width; ++x)
				sortTile(int3(x, y, z));
}

ObjectRect::ObjectRect(const CGObjectInstance * obj_, QRect rect_)
	: obj(obj_)
	, rect(rect_)
{
}

ObjectRect::~ObjectRect()
{
}

std::shared_ptr<QImage> MapHandler::findFlagBitmap(const CGHeroInstance * hero, int anim, const PlayerColor color, int group) const
{
	if(!hero || hero->inBoat())
		return std::shared_ptr<QImage>();

	return findFlagBitmapInternal(graphics->heroFlagAnimations.at(color.getNum()), anim, group, hero->moveDir, true);
}

std::shared_ptr<QImage> MapHandler::findFlagBitmapInternal(std::shared_ptr<Animation> animation, int anim, int group, ui8 dir, bool moving) const
{
	size_t groupSize = animation->size(group);
	if(groupSize == 0)
		return nullptr;

	if(moving)
		return animation->getImage(anim % groupSize, group);
	else
		return animation->getImage((anim / 4) % groupSize, group);
}

MapHandler::BitmapHolder MapHandler::findObjectBitmap(const CGObjectInstance * obj, int anim, int group) const
{
	if(!obj)
		return MapHandler::BitmapHolder();

	// normal object
	std::shared_ptr<Animation> animation = graphics->getAnimation(obj);
	size_t groupSize = animation->size(group);
	if(groupSize == 0)
		return MapHandler::BitmapHolder();

	animation->playerColored(obj->tempOwner);
	auto bitmap = animation->getImage(anim % groupSize, group);

	if(!bitmap)
		return MapHandler::BitmapHolder();

	setPlayerColor(bitmap.get(), obj->tempOwner);

	return MapHandler::BitmapHolder(bitmap);
}

std::vector<ObjectRect> & MapHandler::getObjects(const int3 & tile)
{
	return tileObjects[index(tile)];
}

std::vector<ObjectRect> & MapHandler::getObjects(int x, int y, int z)
{
	return tileObjects[index(x, y, z)];
}



void MapHandler::drawImageSlice(QPainter & painter, const QImage & image, const QPoint & tilesFromAnchor, const QPoint & target, bool locked)
{
	// the image is aligned with its anchor tile at bottom right
	const QRect source(image.width() - (tilesFromAnchor.x() + 1) * tileSize, image.height() - (tilesFromAnchor.y() + 1) * tileSize, tileSize, tileSize);
	const QRect visible = source.intersected(image.rect());

	if(visible.isEmpty())
		return;

	const QPoint position = target + visible.topLeft() - source.topLeft();

	if(!locked)
	{
		painter.drawImage(position, image, visible);
		return;
	}

	QImage dimmed(visible.size(), QImage::Format_ARGB32_Premultiplied);
	dimmed.fill(Qt::transparent);
	{
		QPainter dimmedPainter(&dimmed);
		dimmedPainter.drawImage(QPoint(0, 0), image, visible);
		dimmedPainter.setCompositionMode(QPainter::CompositionMode_DestinationIn);
		dimmedPainter.fillRect(dimmed.rect(), Qt::Dense4Pattern);
	}
	painter.drawImage(position, dimmed);
}

void MapHandler::drawObjectTile(QPainter & painter, const CGObjectInstance * obj, const int3 & tile, const QPoint & target, bool shadow, bool locked)
{
	const QPoint tilesFromAnchor(obj->anchorPos().x - tile.x, obj->anchorPos().y - tile.y);

	auto objData = findObjectBitmap(obj, 0, obj->ID == Obj::HERO ? 2 : 0);

	if(!objData.objBitmap)
		return;

	// heroes and boats keep the shadow in their image
	const SplitImage & split = getSplitImage(objData.objBitmap);

	if(shadow)
	{
		if(split.shadow)
			drawImageSlice(painter, *split.shadow, tilesFromAnchor, target, locked);
		return;
	}

	const QImage * body = objData.objBitmap.get();
	if(split.body && !MapObjectDrawOrder::usesFixedDrawSlot(obj))
	{
		body = split.body.get();
		setPlayerColor(split.body.get(), obj->tempOwner);
	}
	drawImageSlice(painter, *body, tilesFromAnchor, target, locked);

	if(obj->ID == Obj::HERO && obj->tempOwner.isValidPlayer())
	{
		if(auto flag = findFlagBitmap(dynamic_cast<const CGHeroInstance*>(obj), 0, obj->tempOwner, 4))
			drawImageSlice(painter, *flag, tilesFromAnchor, target, locked);
	}
}

void MapHandler::drawObjects(QPainter & painter, const QRectF & section, int z, std::set<const CGObjectInstance *> & locked)
{
	painter.setRenderHint(QPainter::Antialiasing, false);
	painter.setRenderHint(QPainter::SmoothPixmapTransform, false);

	int left = static_cast<int>(std::round(section.left()))/tileSize;
	int right = static_cast<int>(std::round(section.right()))/tileSize;
	int top = static_cast<int>(std::round(section.top()))/tileSize;
	int bottom = static_cast<int>(std::round(section.bottom()))/tileSize;
	const bool roadsAboveGround = LIBRARY->engineSettings()->getBoolean(EGameSettings::MAP_OBJECTS_ROADS_ABOVE_SPECIAL_GROUND);

	for(int x = left; x < right; ++x)
	{
		for(int y = top; y < bottom; ++y)
		{
			const int3 tile(x, y, z);
			const auto & entries = getObjects(tile);

			if(entries.empty())
				continue;

			const QPoint target(x * tileSize - static_cast<int>(section.left()), y * tileSize - static_cast<int>(section.top()));

			// roads and rivers are part of the terrain layer, so they are redrawn to be above the ground
			const bool hasGround = std::any_of(entries.begin(), entries.end(), [](const ObjectRect & entry) { return MapObjectDrawOrder::isSpecialGround(entry.obj); });

			if(hasGround)
			{
				if(roadsAboveGround)
					drawTerrainTile(painter, x, y, z, section.topLeft());

				for(const auto & entry : entries)
					if(MapObjectDrawOrder::isSpecialGround(entry.obj))
						drawObjectTile(painter, entry.obj, tile, target, false, locked.count(entry.obj));

				if(roadsAboveGround)
				{
					drawRiver(painter, x, y, z, section.topLeft());
					drawRoad(painter, x, y, z, section.topLeft());
				}
			}

			MapObjectDrawOrder::drawTile(entries, tile,
				[](const ObjectRect & entry) { return entry.obj; },
				[&](const CGObjectInstance * obj) { return Point((obj->anchorPos().x - x) * tileSize, (obj->anchorPos().y - y) * tileSize); },
				[&](const CGObjectInstance * obj) { drawObjectTile(painter, obj, tile, target, true, locked.count(obj)); },
				[&](const CGObjectInstance * obj) { drawObjectTile(painter, obj, tile, target, false, locked.count(obj)); });
		}
	}
}

void MapHandler::drawObjectAt(QPainter & painter, const CGObjectInstance * obj, int x, int y, QPointF offset, bool locked)
{
	if (!obj)
	{
		logGlobal->error("Stray map object that isn't fading");
		return;
	}

	uint8_t animationFrame = 0;

	auto objData = findObjectBitmap(obj, animationFrame, obj->ID == Obj::HERO ? 2 : 0);
	if(obj->ID == Obj::HERO && obj->tempOwner.isValidPlayer())
		objData.flagBitmap = findFlagBitmap(dynamic_cast<const CGHeroInstance*>(obj), 0, obj->tempOwner, 4);

	if (objData.objBitmap)
	{
		QPoint point((x + 1) * tileSize - (objData.objBitmap->width() + offset.x()), (y + 1) * tileSize - (objData.objBitmap->height() + offset.y()));
		QRect rect(point, QSize(objData.objBitmap->width(), objData.objBitmap->height()));
		painter.drawImage(rect, *objData.objBitmap);

		if (locked)
		{
			painter.setCompositionMode(QPainter::CompositionMode_DestinationIn);
			painter.fillRect(rect, Qt::Dense4Pattern);
			painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
		}

		if (objData.flagBitmap)
			painter.drawImage(point, *objData.flagBitmap);
	}
}

QRgb MapHandler::getTileColor(int x, int y, int z)
{
	// if object at tile is owned - it will be colored as its owner
	for(auto & object : getObjects(x, y, z))
	{
		if(!object.obj->getBlockedPos().count(int3(x, y, z)))
			continue;

		PlayerColor player = object.obj->getOwner();
		if(player == PlayerColor::NEUTRAL)
			return graphics->neutralColor;
		else
			if (player.isValidPlayer())
				return graphics->playerColors[player.getNum()];
	}

	// else - use terrain color (blocked version or normal)

	auto & tile = map->getTile(int3(x, y, z));

	auto color = tile.getTerrain()->minimapUnblocked;
	if (tile.blocked() && (!tile.visitable()))
		color = tile.getTerrain()->minimapBlocked;

	return qRgb(color.r, color.g, color.b);
}

void MapHandler::drawMinimapTile(QPainter & painter, int x, int y, int z)
{
	painter.setPen(getTileColor(x, y, z));
	painter.drawPoint(x, y);
}

std::set<int3> MapHandler::invalidate(const CGObjectInstance * obj)
{
	std::set<int3> stamped;
	if(stampedTiles.count(obj))
		stamped.insert(stampedTiles[obj].begin(), stampedTiles[obj].end());

	auto t1 = removeObject(obj);
	auto t2 = addObject(obj);
	t1.insert(t2.begin(), t2.end());

	stamped.insert(stampedTiles[obj].begin(), stampedTiles[obj].end());
	restampTiles(stamped);

	for(auto & tt : stamped)
		sortTile(tt);
	for(auto & tt : t2)
		sortTile(tt);

	return t1;
}

void MapHandler::invalidateObjects()
{
	initObjectRects();
}
