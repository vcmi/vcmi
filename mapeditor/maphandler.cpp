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
#include "../lib/mapObjects/CObjectClassesHandler.h"
#include "../lib/CHeroHandler.h"
#include "../lib/CTownHandler.h"
#include "../lib/CModHandler.h"
#include "../lib/mapping/CMap.h"
#include "../lib/GameConstants.h"
#include "../lib/JsonDetail.h"

const int tileSize = 32;

static bool objectBlitOrderSorter(const TileObject & a, const TileObject & b)
{
	return MapHandler::compareObjectBlitOrder(a.obj, b.obj);
}

int MapHandler::index(int x, int y, int z) const
{
	return z * (sizes.x * sizes.y) + y * sizes.x + x;
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
	ttiles.clear();
	map = Map;
	
	//sizes of terrain
	sizes.x = map->width;
	sizes.y = map->height;
	sizes.z = map->twoLevel ? 2 : 1;
	
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
	for(const auto & terrain : VLC->terrainTypeHandler->objects)
	{
		terrainFiles[terrain->getJsonKey()] = terrain->tilesFilename;
	}
	for(const auto & river : VLC->riverTypeHandler->objects)
	{
		riverFiles[river->getJsonKey()] = river->tilesFilename;
	}
	for(const auto & road : VLC->roadTypeHandler->objects)
	{
		roadFiles[road->getJsonKey()] = road->tilesFilename;
	}
	
	loadFlipped(terrainAnimations, terrainImages, terrainFiles);
	loadFlipped(riverAnimations, riverImages, riverFiles);
	loadFlipped(roadAnimations, roadImages, roadFiles);
}

void MapHandler::drawTerrainTile(QPainter & painter, int x, int y, int z)
{
	auto & tinfo = map->getTile(int3(x, y, z));
	ui8 rotation = tinfo.extTileFlags % 4;
	
	auto terrainName = tinfo.terType->getJsonKey();
	
	if(terrainImages.at(terrainName).size() <= tinfo.terView)
		return;
	
	bool hflip = (rotation == 1 || rotation == 3), vflip = (rotation == 2 || rotation == 3);
	painter.drawImage(x * tileSize, y * tileSize, terrainImages.at(terrainName)[tinfo.terView]->mirrored(hflip, vflip));
}

void MapHandler::drawRoad(QPainter & painter, int x, int y, int z)
{
	auto & tinfo = map->getTile(int3(x, y, z));
	auto * tinfoUpper = map->isInTheMap(int3(x, y - 1, z)) ? &map->getTile(int3(x, y - 1, z)) : nullptr;
	
	if(tinfoUpper && tinfoUpper->roadType->getId() != Road::NO_ROAD)
	{
		auto roadName = tinfoUpper->roadType->getJsonKey();
		QRect source(0, tileSize / 2, tileSize, tileSize / 2);
		ui8 rotation = (tinfoUpper->extTileFlags >> 4) % 4;
		bool hflip = (rotation == 1 || rotation == 3), vflip = (rotation == 2 || rotation == 3);
		if(roadImages.at(roadName).size() > tinfoUpper->roadDir)
		{
			painter.drawImage(QPoint(x * tileSize, y * tileSize), roadImages.at(roadName)[tinfoUpper->roadDir]->mirrored(hflip, vflip), source);
		}
	}
	
	if(tinfo.roadType->getId() != Road::NO_ROAD) //print road from this tile
	{
		auto roadName = tinfo.roadType->getJsonKey();;
		QRect source(0, 0, tileSize, tileSize / 2);
		ui8 rotation = (tinfo.extTileFlags >> 4) % 4;
		bool hflip = (rotation == 1 || rotation == 3), vflip = (rotation == 2 || rotation == 3);
		if(roadImages.at(roadName).size() > tinfo.roadDir)
		{
			painter.drawImage(QPoint(x * tileSize, y * tileSize + tileSize / 2), roadImages.at(roadName)[tinfo.roadDir]->mirrored(hflip, vflip), source);
		}
	}
}

void MapHandler::drawRiver(QPainter & painter, int x, int y, int z)
{
	auto & tinfo = map->getTile(int3(x, y, z));

	if(tinfo.riverType->getId() == River::NO_RIVER)
		return;
	
	//TODO: use ui8 instead of string key
	auto riverName = tinfo.riverType->getJsonKey();

	if(riverImages.at(riverName).size() <= tinfo.riverDir)
		return;

	ui8 rotation = (tinfo.extTileFlags >> 2) % 4;
	bool hflip = (rotation == 1 || rotation == 3), vflip = (rotation == 2 || rotation == 3);

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

void MapHandler::initObjectRects()
{
	ttiles.resize(sizes.x * sizes.y * sizes.z);
	
	//initializing objects / rects
	for(const CGObjectInstance * elem : map->objects)
	{
		auto *obj = const_cast<CGObjectInstance *>(elem);
		if(	!obj
		   || (obj->ID==Obj::HERO && dynamic_cast<const CGHeroInstance*>(obj)->inTownGarrison) //garrisoned hero
		   || (obj->ID==Obj::BOAT && dynamic_cast<const CGBoat*>(obj)->hero)) //boat with hero (hero graphics is used)
		{
			continue;
		}
		
		std::shared_ptr<Animation> animation = graphics->getAnimation(obj);
		
		//no animation at all
		if(!animation)
			continue;
		
		//empty animation
		if(animation->size(0) == 0)
			continue;
		
		auto image = animation->getImage(0, obj->ID == Obj::HERO ? 2 : 0);
		if(!image)
		{
			//workaround for prisons
			image = animation->getImage(0, 0);
			if(!image)
				continue;
		}
			
		
		for(int fx=0; fx < obj->getWidth(); ++fx)
		{
			for(int fy=0; fy < obj->getHeight(); ++fy)
			{
				int3 currTile(obj->pos.x - fx, obj->pos.y - fy, obj->pos.z);
				QRect cr(image->width() - fx * tileSize - tileSize,
						 image->height() - fy * tileSize - tileSize,
						 image->width(),
						 image->height());
				
				TileObject toAdd(obj, cr);
				
				if( map->isInTheMap(currTile) && // within map
				   cr.x() + cr.width() > 0 &&           // image has data on this tile
				   cr.y() + cr.height() > 0)
				{
					ttiles[index(currTile)].push_back(toAdd);
				}
			}
		}
	}
	
	for(auto & tt : ttiles)
	{
		stable_sort(tt.begin(), tt.end(), objectBlitOrderSorter);
	}
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

TileObject::TileObject(CGObjectInstance * obj_, QRect rect_)
: obj(obj_),
rect(rect_)
{
}

TileObject::~TileObject() = default;

std::shared_ptr<QImage> MapHandler::findFlagBitmap(const CGHeroInstance * hero, int anim, const PlayerColor color, int group) const
{
	if(!hero || hero->boat)
		return {};
	
	return findFlagBitmapInternal(graphics->heroFlagAnimations.at(color.getNum()), anim, group, hero->moveDir, !hero->isStanding);
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

MapHandler::AnimBitmapHolder MapHandler::findObjectBitmap(const CGObjectInstance * obj, int anim, int group) const
{
	if(!obj)
		return {};

	// normal object
	std::shared_ptr<Animation> animation = graphics->getAnimation(obj);
	size_t groupSize = animation->size(group);
	if(groupSize == 0)
		return {};
	
	animation->playerColored(obj->tempOwner);
	auto bitmap = animation->getImage(anim % groupSize, group);
	
	if(!bitmap)
		return {};

	setPlayerColor(bitmap.get(), obj->tempOwner);
	
	return {bitmap};
}

std::vector<TileObject> & MapHandler::getObjects(int x, int y, int z)
{
	return ttiles[index(x, y, z)];
}

void MapHandler::drawObjects(QPainter & painter, int x, int y, int z)
{
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
			auto pos = obj->getPosition();

			painter.drawImage(QPoint(x * tileSize, y * tileSize), *objData.objBitmap, object.rect);
			
			if(objData.flagBitmap)
			{
				if(x == pos.x && y == pos.y)
					painter.drawImage(QPoint((x - 2) * tileSize, (y - 1) * tileSize), *objData.flagBitmap);
			}
		}
	}
}

void MapHandler::drawObject(QPainter & painter, const TileObject & object)
{
	const CGObjectInstance * obj = object.obj;
	if (!obj)
	{
		logGlobal->error("Stray map object that isn't fading");
		return;
	}

	uint8_t animationFrame = 0;

	auto objData = findObjectBitmap(obj, animationFrame, obj->ID == Obj::HERO ? 2 : 0);
	if(obj->ID == Obj::HERO && obj->tempOwner.isValidPlayer())
		objData.flagBitmap = findFlagBitmap(dynamic_cast<const CGHeroInstance*>(obj), 0, obj->tempOwner, 0);
	
	if (objData.objBitmap)
	{
		auto pos = obj->getPosition();

		painter.drawImage(pos.x * tileSize - object.rect.x(), pos.y * tileSize - object.rect.y(), *objData.objBitmap);
		
		if (objData.flagBitmap)
		{
			if(object.rect.x() == pos.x && object.rect.y() == pos.y)
				painter.drawImage(pos.x * tileSize - object.rect.x(), pos.y * tileSize - object.rect.y(), *objData.flagBitmap);
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
		painter.drawImage(QPoint((x + 1) * 32 - objData.objBitmap->width(), (y + 1) * 32 - objData.objBitmap->height()), *objData.objBitmap);
		
		if (objData.flagBitmap)
			painter.drawImage(QPoint((x + 1) * 32 - objData.objBitmap->width(), (y + 1) * 32 - objData.objBitmap->height()), *objData.flagBitmap);
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
			if (player < PlayerColor::PLAYER_LIMIT)
				return graphics->playerColors[player.getNum()];
	}
	
	// else - use terrain color (blocked version or normal)
	
	auto & tile = map->getTile(int3(x, y, z));
	
	auto color = tile.terType->minimapUnblocked;
	if (tile.blocked && (!tile.visitable))
		color = tile.terType->minimapBlocked;
	
	return qRgb(color.r, color.g, color.b);
}

void MapHandler::drawMinimapTile(QPainter & painter, int x, int y, int z)
{
	painter.setPen(getTileColor(x, y, z));
	painter.drawPoint(x, y);
}

void MapHandler::invalidate(int x, int y, int z)
{
	auto & objects = getObjects(x, y, z);
	
	for(auto obj = objects.begin(); obj != objects.end();)
	{
		//object was removed
		if(std::find(map->objects.begin(), map->objects.end(), obj->obj) == map->objects.end())
		{
			obj = objects.erase(obj);
			continue;
		}
			
		//object was moved
		auto & pos = obj->obj->pos;
		if(pos.z != z || pos.x < x || pos.y < y || pos.x - obj->obj->getWidth() >= x || pos.y - obj->obj->getHeight() >= y)
		{
			obj = objects.erase(obj);
			continue;
		}
		
		++obj;
	}
	
	stable_sort(objects.begin(), objects.end(), objectBlitOrderSorter);
}

void MapHandler::invalidate(CGObjectInstance * obj)
{
	std::shared_ptr<Animation> animation = graphics->getAnimation(obj);
		
	//no animation at all or empty animation
	if(!animation || animation->size(0) == 0)
		return;
		
	auto image = animation->getImage(0, obj->ID == Obj::HERO ? 2 : 0);
	if(!image)
		return;
	
	for(int fx=0; fx < obj->getWidth(); ++fx)
	{
		for(int fy=0; fy < obj->getHeight(); ++fy)
		{
			//object presented on the tile
			int3 currTile(obj->pos.x - fx, obj->pos.y - fy, obj->pos.z);
			QRect cr(image->width() - fx * tileSize - tileSize, image->height() - fy * tileSize - tileSize, image->width(), image->height());
			
			if( map->isInTheMap(currTile) && // within map
			   cr.x() + cr.width() > 0 &&           // image has data on this tile
			   cr.y() + cr.height() > 0)
			{
				auto & objects = ttiles[index(currTile)];
				bool found = false;
				for(auto & o : objects)
				{
					if(o.obj == obj)
					{
						o.rect = cr;
						found = true;
						break;
					}
				}
				if(!found)
					objects.emplace_back(obj, cr);
				
				stable_sort(objects.begin(), objects.end(), objectBlitOrderSorter);
			}
		}
	}
}

std::vector<int3> MapHandler::getTilesUnderObject(CGObjectInstance * obj) const
{
	std::vector<int3> result;
	for(int fx=0; fx < obj->getWidth(); ++fx)
	{
		for(int fy=0; fy < obj->getHeight(); ++fy)
		{
			//object presented on the tile
			int3 currTile(obj->pos.x - fx, obj->pos.y - fy, obj->pos.z);
			if(map->isInTheMap(currTile)) // within map
			{
				result.push_back(currTile);
			}
		}
	}
	return result;
}

void MapHandler::invalidateObjects()
{
	for(auto obj : map->objects)
	{
		invalidate(obj);
	}
}

void MapHandler::invalidate(const std::vector<int3> & tiles)
{
	for(auto & currTile : tiles)
	{
		invalidate(currTile.x, currTile.y, currTile.z);
	}
}
