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
#include "../lib/mapObjects/MiscObjects.h"
#include "../lib/GameConstants.h"

const int tileSize = 32;

static bool objectBlitOrderSorter(const ObjectRect & a, const ObjectRect & b)
{
	return MapHandler::compareObjectBlitOrder(a.obj, b.obj);
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

void MapHandler::drawTerrainTile(QPainter & painter, int x, int y, int z)
{
	auto & tinfo = map->getTile(int3(x, y, z));
	ui8 rotation = tinfo.extTileFlags % 4;
	
	auto terrainName = tinfo.getTerrain()->getJsonKey();
	
	if(terrainImages.at(terrainName).size() <= tinfo.terView)
		return;
	
	bool hflip = (rotation == 1 || rotation == 3);
	bool vflip = (rotation == 2 || rotation == 3);
	painter.drawImage(x * tileSize, y * tileSize, terrainImages.at(terrainName)[tinfo.terView]->mirrored(hflip, vflip));
}

void MapHandler::drawRoad(QPainter & painter, int x, int y, int z)
{
	auto & tinfo = map->getTile(int3(x, y, z));
	auto * tinfoUpper = map->isInTheMap(int3(x, y - 1, z)) ? &map->getTile(int3(x, y - 1, z)) : nullptr;
	
	if(tinfoUpper && tinfoUpper->roadType)
	{
		auto roadName = tinfoUpper->getRoad()->getJsonKey();
		QRect source(0, tileSize / 2, tileSize, tileSize / 2);
		ui8 rotation = (tinfoUpper->extTileFlags >> 4) % 4;
		bool hflip = (rotation == 1 || rotation == 3);
		bool vflip = (rotation == 2 || rotation == 3);
		if(roadImages.at(roadName).size() > tinfoUpper->roadDir)
		{
			painter.drawImage(QPoint(x * tileSize, y * tileSize), roadImages.at(roadName)[tinfoUpper->roadDir]->mirrored(hflip, vflip), source);
		}
	}
	
	if(tinfo.roadType) //print road from this tile
	{
		auto roadName = tinfo.getRoad()->getJsonKey();
		QRect source(0, 0, tileSize, tileSize / 2);
		ui8 rotation = (tinfo.extTileFlags >> 4) % 4;
		bool hflip = (rotation == 1 || rotation == 3);
		bool vflip = (rotation == 2 || rotation == 3);
		if(roadImages.at(roadName).size() > tinfo.roadDir)
		{
			painter.drawImage(QPoint(x * tileSize, y * tileSize + tileSize / 2), roadImages.at(roadName)[tinfo.roadDir]->mirrored(hflip, vflip), source);
		}
	}
}

void MapHandler::drawRiver(QPainter & painter, int x, int y, int z)
{
	auto & tinfo = map->getTile(int3(x, y, z));

	if(!tinfo.hasRiver())
		return;
	
	//TODO: use ui8 instead of string key
	auto riverName = tinfo.getRiver()->getJsonKey();

	if(riverImages.at(riverName).size() <= tinfo.riverDir)
		return;

	ui8 rotation = (tinfo.extTileFlags >> 2) % 4;
	bool hflip = (rotation == 1 || rotation == 3);
	bool vflip = (rotation == 2 || rotation == 3);

	painter.drawImage(x * tileSize, y * tileSize, riverImages.at(riverName)[tinfo.riverDir]->mirrored(hflip, vflip));
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

std::set<int3> MapHandler::removeObject(const CGObjectInstance *object)
{
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

std::set<int3> MapHandler::addObject(const CGObjectInstance * object)
{
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
	if(!map)
		return;
	
	tileObjects.resize(map->width * map->height * map->mapLevels);
	
	//initializing objects / rects
	for(const auto & elem : map->objects)
	{
		addObject(elem.get());
	}
	
	for(auto & tt : tileObjects)
		stable_sort(tt.begin(), tt.end(), objectBlitOrderSorter);
}

bool MapHandler::compareObjectBlitOrder(const CGObjectInstance * a, const CGObjectInstance * b)
{
	if (!a)
		return true;
	if (!b)
		return false;
	if (a->appearance->printPriority != b->appearance->printPriority)
		return a->appearance->printPriority > b->appearance->printPriority;
	
	if(a->pos.y != b->pos.y)
		return a->pos.y < b->pos.y;
	
	if(b->ID == Obj::HERO && a->ID != Obj::HERO)
		return true;
	if(b->ID != Obj::HERO && a->ID == Obj::HERO)
		return false;
	
	if(!a->isVisitable() && b->isVisitable())
		return true;
	if(!b->isVisitable() && a->isVisitable())
		return false;
	if(a->pos.x < b->pos.x)
		return true;
	return false;
}

ObjectRect::ObjectRect(const CGObjectInstance * obj_, QRect rect_)
: obj(obj_),
rect(rect_)
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

void MapHandler::drawObjects(QPainter & painter, int x, int y, int z, const std::set<const CGObjectInstance *> & locked)
{
	painter.setRenderHint(QPainter::Antialiasing, false);
	painter.setRenderHint(QPainter::SmoothPixmapTransform, false);

	for(auto & object : getObjects(x, y, z))
	{
		const CGObjectInstance * obj = object.obj;
		if(!obj)
		{
			logGlobal->error("Stray map object that isn't fading");
			return;
		}

		uint8_t animationFrame = 0;

		auto objData = findObjectBitmap(obj, animationFrame, obj->ID == Obj::HERO ? 2 : 0);
		if(obj->ID == Obj::HERO && obj->tempOwner.isValidPlayer())
			objData.flagBitmap = findFlagBitmap(dynamic_cast<const CGHeroInstance*>(obj), 0, obj->tempOwner, 4);
		
		if(objData.objBitmap)
		{
			auto pos = obj->anchorPos();

			painter.drawImage(QPoint(x * tileSize, y * tileSize), *objData.objBitmap, object.rect, Qt::AutoColor | Qt::NoOpaqueDetection);

			if(locked.count(obj))
			{
				painter.setCompositionMode(QPainter::CompositionMode_DestinationIn);
				painter.fillRect(x * tileSize, y * tileSize, object.rect.width(), object.rect.height(), Qt::Dense4Pattern);
				painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
			}

			if(objData.flagBitmap)
			{
				if(x == pos.x && y == pos.y)
					painter.drawImage(QPoint((x - 2) * tileSize, (y - 1) * tileSize), *objData.flagBitmap);
			}
		}
	}
}

void MapHandler::drawObjectAt(QPainter & painter, const CGObjectInstance * obj, int x, int y)
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
		painter.drawImage(QPoint((x + 1) * tileSize - objData.objBitmap->width(), (y + 1) * tileSize - objData.objBitmap->height()), *objData.objBitmap);
		
		if (objData.flagBitmap)
			painter.drawImage(QPoint((x + 1) * tileSize - objData.objBitmap->width(), (y + 1) * tileSize - objData.objBitmap->height()), *objData.flagBitmap);
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
	auto t1 = removeObject(obj);
	auto t2 = addObject(obj);
	t1.insert(t2.begin(), t2.end());
	
	for(auto & tt : t2)
		stable_sort(tileObjects[index(tt)].begin(), tileObjects[index(tt)].end(), objectBlitOrderSorter);
	
	return t1;
}

void MapHandler::invalidateObjects()
{
	initObjectRects();
}
