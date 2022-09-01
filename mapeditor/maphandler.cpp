#include "StdInc.h"
#include "maphandler.h"
#include "graphics.h"
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

static bool objectBlitOrderSorter(const TerrainTileObject & a, const TerrainTileObject & b)
{
	return MapHandler::compareObjectBlitOrder(a.obj, b.obj);
}

MapHandler::MapHandler(const CMap * Map):
	map(Map)
{
	init();
}

void MapHandler::init()
{
	//sizes of terrain
	sizes.x = map->width;
	sizes.y = map->height;
	sizes.z = map->twoLevel ? 2 : 1;
	
	initTerrainGraphics();
	logGlobal->info("\tPreparing terrain, roads, rivers, borders");
	initObjectRects();
	logGlobal->info("\tMaking object rects");
}

void MapHandler::initTerrainGraphics()
{
	static const std::map<std::string, std::string> ROAD_FILES =
	{
		{ROAD_NAMES[1], "dirtrd"},
		{ROAD_NAMES[2], "gravrd"},
		{ROAD_NAMES[3], "cobbrd"}
	};
	
	static const std::map<std::string, std::string> RIVER_FILES =
	{
		{RIVER_NAMES[1], "clrrvr"},
		{RIVER_NAMES[2], "icyrvr"},
		{RIVER_NAMES[3], "mudrvr"},
		{RIVER_NAMES[4], "lavrvr"}
	};
	
	
	auto loadFlipped = [](TFlippedAnimations & animation, TFlippedCache & cache, const std::map<std::string, std::string> & files)
	{
		for(auto & type : files)
		{
			animation[type.first] = make_unique<Animation>(type.second);
			animation[type.first]->preload();
			const size_t views = animation[type.first]->size(0);
			cache[type.first].resize(views);
			
			for(int j = 0; j < views; j++)
				cache[type.first][j] = animation[type.first]->getImage(j);
		}
	};
	
	std::map<std::string, std::string> terrainFiles;
	for(auto & terrain : Terrain::Manager::terrains())
	{
		terrainFiles[terrain] = Terrain::Manager::getInfo(terrain).tilesFilename;
	}
	
	loadFlipped(terrainAnimations, terrainImages, terrainFiles);
	loadFlipped(roadAnimations, roadImages, ROAD_FILES);
	loadFlipped(riverAnimations, riverImages, RIVER_FILES);
	
	ttiles.resize(sizes.x * sizes.y * sizes.z);
}

void MapHandler::drawTerrainTile(QPainter & painter, int x, int y, int z)
{
	auto & tinfo = map->getTile(int3(x, y, z));
	//Rect destRect(realTileRect);
	
	ui8 rotation = tinfo.extTileFlags % 4;
	
	if(terrainImages.at(tinfo.terType).size() <= tinfo.terView)
		return;
	
	bool hflip = (rotation == 1 || rotation == 3), vflip = (rotation == 2 || rotation == 3);
	
	painter.drawImage(x * tileSize, y * tileSize, terrainImages.at(tinfo.terType)[tinfo.terView]->mirrored(hflip, vflip));
}

void MapHandler::initObjectRects()
{
	//initializing objects / rects
	for(auto & elem : map->objects)
	{
		const CGObjectInstance *obj = elem;
		if(	!obj
		   || (obj->ID==Obj::HERO && static_cast<const CGHeroInstance*>(obj)->inTownGarrison) //garrisoned hero
		   || (obj->ID==Obj::BOAT && static_cast<const CGBoat*>(obj)->hero)) //boat with hero (hero graphics is used)
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
		
		auto image = animation->getImage(0,0);
		bool real = true;
		for(int fx=0; fx < obj->getWidth(); ++fx)
		{
			for(int fy=0; fy < obj->getHeight(); ++fy)
			{
				int3 currTile(obj->pos.x - fx, obj->pos.y - fy, obj->pos.z);
				QRect cr(image->width() - fx * 32 - 32, image->height() - fy * 32 - 32, image->width(), image->height());
				TerrainTileObject toAdd(obj, cr, real/*obj->visitableAt(currTile.x, currTile.y)*/);
				real = false;
				
				
				if( map->isInTheMap(currTile) && // within map
				   cr.x() + cr.width() > 0 &&           // image has data on this tile
				   cr.y() + cr.height() > 0 &&
				   obj->coveringAt(currTile.x, currTile.y) // object is visible here
				   )
				{
					ttiles[currTile.z * (sizes.x * sizes.y) + currTile.y * sizes.x + currTile.x].objects.push_back(toAdd);
				}
			}
		}
	}
	
	for(auto & tt : ttiles)
	{
		stable_sort(tt.objects.begin(), tt.objects.end(), objectBlitOrderSorter);
	}
}

bool MapHandler::compareObjectBlitOrder(const CGObjectInstance * a, const CGObjectInstance * b)
{
	if (!a)
		return true;
	if (!b)
		return false;
	if (a->appearance.printPriority != b->appearance.printPriority)
		return a->appearance.printPriority > b->appearance.printPriority;
	
	if(a->pos.y != b->pos.y)
		return a->pos.y < b->pos.y;
	
	if(b->ID==Obj::HERO && a->ID!=Obj::HERO)
		return true;
	if(b->ID!=Obj::HERO && a->ID==Obj::HERO)
		return false;
	
	if(!a->isVisitable() && b->isVisitable())
		return true;
	if(!b->isVisitable() && a->isVisitable())
		return false;
	if(a->pos.x < b->pos.x)
		return true;
	return false;
}

TerrainTileObject::TerrainTileObject(const CGObjectInstance * obj_, QRect rect_, bool real_)
: obj(obj_),
rect(rect_),
real(real_)
{
}

TerrainTileObject::~TerrainTileObject()
{
}

ui8 MapHandler::getHeroFrameGroup(ui8 dir, bool isMoving) const
{
	if(isMoving)
	{
		static const ui8 frame [] = {0xff, 10, 5, 6, 7, 8, 9, 12, 11};
		return frame[dir];
	}
	else //if(isMoving)
	{
		static const ui8 frame [] = {0xff, 13, 0, 1, 2, 3, 4, 15, 14};
		return frame[dir];
	}
}

ui8 MapHandler::getPhaseShift(const CGObjectInstance *object) const
{
	auto i = animationPhase.find(object);
	if(i == animationPhase.end())
	{
		ui8 ret = CRandomGenerator::getDefault().nextInt(254);
		animationPhase[object] = ret;
		return ret;
	}
	
	return i->second;
}

MapHandler::AnimBitmapHolder MapHandler::findHeroBitmap(const CGHeroInstance * hero, int anim) const
{
	if(hero && hero->moveDir && hero->type) //it's hero or boat
	{
		if(hero->tempOwner >= PlayerColor::PLAYER_LIMIT) //Neutral hero?
		{
			logGlobal->error("A neutral hero (%s) at %s. Should not happen!", hero->name, hero->pos.toString());
			return MapHandler::AnimBitmapHolder();
		}
		
		//pick graphics of hero (or boat if hero is sailing)
		std::shared_ptr<Animation> animation;
		if (hero->boat)
			animation = graphics->boatAnimations[hero->boat->subID];
		else
			animation = graphics->heroAnimations[hero->appearance.animationFile];
		
		bool moving = !hero->isStanding;
		int group = getHeroFrameGroup(hero->moveDir, moving);
		
		if(animation->size(group) > 0)
		{
			int frame = anim % animation->size(group);
			auto heroImage = animation->getImage(frame, group);
			
			//get flag overlay only if we have main image
			auto flagImage = findFlagBitmap(hero, anim, &hero->tempOwner, group);
			
			return MapHandler::AnimBitmapHolder(heroImage, flagImage);
		}
	}
	return MapHandler::AnimBitmapHolder();
}

MapHandler::AnimBitmapHolder MapHandler::findBoatBitmap(const CGBoat * boat, int anim) const
{
	auto animation = graphics->boatAnimations.at(boat->subID);
	int group = getHeroFrameGroup(boat->direction, false);
	if(animation->size(group) > 0)
		return MapHandler::AnimBitmapHolder(animation->getImage(anim % animation->size(group), group));
	else
		return MapHandler::AnimBitmapHolder();
}

std::shared_ptr<QImage> MapHandler::findFlagBitmap(const CGHeroInstance * hero, int anim, const PlayerColor * color, int group) const
{
	if(!hero)
		return std::shared_ptr<QImage>();
	
	if(hero->boat)
		return findBoatFlagBitmap(hero->boat, anim, color, group, hero->moveDir);
	return findHeroFlagBitmap(hero, anim, color, group);
}

std::shared_ptr<QImage> MapHandler::findHeroFlagBitmap(const CGHeroInstance * hero, int anim, const PlayerColor * color, int group) const
{
	return findFlagBitmapInternal(graphics->heroFlagAnimations.at(color->getNum()), anim, group, hero->moveDir, !hero->isStanding);
}

std::shared_ptr<QImage> MapHandler::findBoatFlagBitmap(const CGBoat * boat, int anim, const PlayerColor * color, int group, ui8 dir) const
{
	int boatType = boat->subID;
	if(boatType < 0 || boatType >= graphics->boatFlagAnimations.size())
	{
		logGlobal->error("Not supported boat subtype: %d", boat->subID);
		return nullptr;
	}
	
	const auto & subtypeFlags = graphics->boatFlagAnimations.at(boatType);
	
	int colorIndex = color->getNum();
	
	if(colorIndex < 0 || colorIndex >= subtypeFlags.size())
	{
		logGlobal->error("Invalid player color %d", colorIndex);
		return nullptr;
	}
	
	return findFlagBitmapInternal(subtypeFlags.at(colorIndex), anim, group, dir, false);
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


MapHandler::AnimBitmapHolder MapHandler::findObjectBitmap(const CGObjectInstance * obj, int anim) const
{
	if (!obj)
		return MapHandler::AnimBitmapHolder();
	if (obj->ID == Obj::HERO)
		return findHeroBitmap(static_cast<const CGHeroInstance*>(obj), anim);
	if (obj->ID == Obj::BOAT)
		return findBoatBitmap(static_cast<const CGBoat*>(obj), anim);
	
	// normal object
	std::shared_ptr<Animation> animation = graphics->getAnimation(obj);
	size_t groupSize = animation->size();
	if(groupSize == 0)
		return MapHandler::AnimBitmapHolder();
	
	animation->playerColored(obj->tempOwner);
	auto bitmap = animation->getImage((anim + getPhaseShift(obj)) % groupSize);
	if(!bitmap)
		return MapHandler::AnimBitmapHolder();
	
	return MapHandler::AnimBitmapHolder(bitmap);
}

const std::vector<TerrainTileObject> & MapHandler::getObjects(int x, int y, int z)
{
	return ttiles[z * (sizes.x * sizes.y) + y * sizes.x + x].objects;
}

void MapHandler::drawObjects(QPainter & painter, int x, int y, int z)
{
	for(auto & object : getObjects(x, y, z))
	{
		const CGObjectInstance * obj = object.obj;
		if (!obj)
		{
			logGlobal->error("Stray map object that isn't fading");
			return;
		}

		uint8_t animationFrame = 0;

		auto objData = findObjectBitmap(obj, animationFrame);
		if (objData.objBitmap)
		{
			auto pos = obj->getPosition();
			QRect srcRect(object.rect.x() + pos.x * 32, object.rect.y() + pos.y * 32, tileSize, tileSize);

			painter.drawImage(QPoint(x * 32, y * 32), *objData.objBitmap, object.rect);
			//painter.drawImage(pos.x * 32 - object.rect.x(), pos.y * 32 - object.rect.y(), *objData.objBitmap);
			//drawObject(targetSurf, objData.objBitmap, &srcRect, objData.isMoving);
			if (objData.flagBitmap)
			{
				/*if (objData.isMoving)
				{
					srcRect.y += FRAMES_PER_MOVE_ANIM_GROUP * 2 - tileSize;
					Rect dstRect(realPos.x, realPos.y - tileSize / 2, tileSize, tileSize);
					drawHeroFlag(targetSurf, objData.flagBitmap, &srcRect, &dstRect, true);
				}
				else if (obj->pos.x == pos.x && obj->pos.y == pos.y)
				{
					Rect dstRect(realPos.x - 2 * tileSize, realPos.y - tileSize, 3 * tileSize, 2 * tileSize);
					drawHeroFlag(targetSurf, objData.flagBitmap, nullptr, &dstRect, false);
				}*/
			}
		}
	}
}

void MapHandler::drawObject(QPainter & painter, const TerrainTileObject & object)
{
	//if(object.visi)
	const CGObjectInstance * obj = object.obj;
	if (!obj)
	{
		logGlobal->error("Stray map object that isn't fading");
		return;
	}

	uint8_t animationFrame = 0;

	auto objData = findObjectBitmap(obj, animationFrame);
	if (objData.objBitmap)
	{
		auto pos = obj->getPosition();
		QRect srcRect(object.rect.x() + pos.x * 32, object.rect.y() + pos.y * 32, tileSize, tileSize);

		painter.drawImage(pos.x * 32 - object.rect.x(), pos.y * 32 - object.rect.y(), *objData.objBitmap);
		//drawObject(targetSurf, objData.objBitmap, &srcRect, objData.isMoving);
		if (objData.flagBitmap)
		{
			/*if (objData.isMoving)
			{
				srcRect.y += FRAMES_PER_MOVE_ANIM_GROUP * 2 - tileSize;
				Rect dstRect(realPos.x, realPos.y - tileSize / 2, tileSize, tileSize);
				drawHeroFlag(targetSurf, objData.flagBitmap, &srcRect, &dstRect, true);
			}
			else if (obj->pos.x == pos.x && obj->pos.y == pos.y)
			{
				Rect dstRect(realPos.x - 2 * tileSize, realPos.y - tileSize, 3 * tileSize, 2 * tileSize);
				drawHeroFlag(targetSurf, objData.flagBitmap, nullptr, &dstRect, false);
			}*/
		}
	}
}
