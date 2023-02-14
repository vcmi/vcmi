/*
 * MapRenderer.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "MapRenderer.h"

#include "MapRendererContext.h"
#include "mapHandler.h"

#include "../CGameInfo.h"
#include "../render/CAnimation.h"
#include "../render/Canvas.h"
#include "../render/IImage.h"

#include "../../CCallback.h"

#include "../../lib/RiverHandler.h"
#include "../../lib/RoadHandler.h"
#include "../../lib/TerrainHandler.h"
#include "../../lib/mapObjects/CGHeroInstance.h"
#include "../../lib/mapping/CMap.h"

struct NeighborTilesInfo
{
	bool d7, //789
		 d8, //456
		 d9, //123
		 d4,
		 d5,
		 d6,
		 d1,
		 d2,
		 d3;
	NeighborTilesInfo( const IMapRendererContext & context, const int3 & pos)
	{
		auto getTile = [&](int dx, int dy)->bool
		{
			if ( dx + pos.x < 0 || dx + pos.x >= context.getMapSize().x
			  || dy + pos.y < 0 || dy + pos.y >= context.getMapSize().y)
				return false;

			//FIXME: please do not read settings for every tile...
			return context.isVisible( pos + int3(dx, dy, 0));
		};
		d7 = getTile(-1, -1); //789
		d8 = getTile( 0, -1); //456
		d9 = getTile(+1, -1); //123
		d4 = getTile(-1, 0);
		d5 = getTile( 0, 0);
		d6 = getTile(+1, 0);
		d1 = getTile(-1, +1);
		d2 = getTile( 0, +1);
		d3 = getTile(+1, +1);
	}

	bool areAllHidden() const
	{
		return !(d1 || d2 || d3 || d4 || d5 || d6 || d7 || d8 || d9);
	}

	int getBitmapID() const
	{
		//NOTE: some images have unused in VCMI pair (same blockmap but a bit different look)
		// 0-1, 2-3, 4-5, 11-13, 12-14
		static const int visBitmaps[256] = {
			-1,  34,   4,   4,  22,  23,   4,   4,  36,  36,  38,  38,  47,  47,  38,  38, //16
			 3,  25,  12,  12,   3,  25,  12,  12,   9,   9,   6,   6,   9,   9,   6,   6, //32
			35,  39,  48,  48,  41,  43,  48,  48,  36,  36,  38,  38,  47,  47,  38,  38, //48
			26,  49,  28,  28,  26,  49,  28,  28,   9,   9,   6,   6,   9,   9,   6,   6, //64
			 0,  45,  29,  29,  24,  33,  29,  29,  37,  37,   7,   7,  50,  50,   7,   7, //80
			13,  27,  44,  44,  13,  27,  44,  44,   8,   8,  10,  10,   8,   8,  10,  10, //96
			 0,  45,  29,  29,  24,  33,  29,  29,  37,  37,   7,   7,  50,  50,   7,   7, //112
			13,  27,  44,  44,  13,  27,  44,  44,   8,   8,  10,  10,   8,   8,  10,  10, //128
			15,  17,  30,  30,  16,  19,  30,  30,  46,  46,  40,  40,  32,  32,  40,  40, //144
			 2,  25,  12,  12,   2,  25,  12,  12,   9,   9,   6,   6,   9,   9,   6,   6, //160
			18,  42,  31,  31,  20,  21,  31,  31,  46,  46,  40,  40,  32,  32,  40,  40, //176
			26,  49,  28,  28,  26,  49,  28,  28,   9,   9,   6,   6,   9,   9,   6,   6, //192
			 0,  45,  29,  29,  24,  33,  29,  29,  37,  37,   7,   7,  50,  50,   7,   7, //208
			13,  27,  44,  44,  13,  27,  44,  44,   8,   8,  10,  10,   8,   8,  10,  10, //224
			 0,  45,  29,  29,  24,  33,  29,  29,  37,  37,   7,   7,  50,  50,   7,   7, //240
			13,  27,  44,  44,  13,  27,  44,  44,   8,   8,  10,  10,   8,   8,  10,  10  //256
		};

		return visBitmaps[d1 + d2 * 2 + d3 * 4 + d4 * 8 + d6 * 16 + d7 * 32 + d8 * 64 + d9 * 128]; // >=0 -> partial hide, <0 - full hide
	}
};

MapObjectsSorter::MapObjectsSorter(const IMapRendererContext & context)
	: context(context)
{
}

bool MapObjectsSorter::operator()(const ObjectInstanceID & left, const ObjectInstanceID & right) const
{
	return (*this)(context.getObject(left), context.getObject(right));
}

bool MapObjectsSorter::operator()(const CGObjectInstance * left, const CGObjectInstance * right) const
{
	//FIXME: remove mh access
	return CGI->mh->compareObjectBlitOrder(left, right);
}

MapTileStorage::MapTileStorage(size_t capacity)
	: animations(capacity)
{
}

void MapTileStorage::load(size_t index, const std::string & filename)
{
	auto & terrainAnimations = animations[index];

	for(auto & entry : terrainAnimations)
	{
		entry = std::make_unique<CAnimation>(filename);
		entry->preload();
	}

	for(size_t i = 0; i < terrainAnimations[0]->size(); ++i)
	{
		terrainAnimations[1]->getImage(i)->verticalFlip();
		terrainAnimations[3]->getImage(i)->verticalFlip();

		terrainAnimations[2]->getImage(i)->horizontalFlip();
		terrainAnimations[3]->getImage(i)->horizontalFlip();
	}
}

std::shared_ptr<IImage> MapTileStorage::find(size_t fileIndex, size_t rotationIndex, size_t imageIndex)
{
	const auto & animation = animations[fileIndex][rotationIndex];
	return animation->getImage(imageIndex);
}

MapRendererTerrain::MapRendererTerrain()
	: storage(VLC->terrainTypeHandler->objects.size())
{
	for(const auto & terrain : VLC->terrainTypeHandler->objects)
		storage.load(terrain->getIndex(), terrain->tilesFilename);
}

void MapRendererTerrain::renderTile(const IMapRendererContext & context, Canvas & target, const int3 & coordinates)
{
	const TerrainTile & mapTile = context.getMapTile(coordinates);

	int32_t terrainIndex = mapTile.terType->getIndex();
	int32_t imageIndex = mapTile.terView;
	int32_t rotationIndex = mapTile.extTileFlags % 4;

	const auto & image = storage.find(terrainIndex, rotationIndex, imageIndex);

	target.draw(image, Point(0, 0));
}

MapRendererRiver::MapRendererRiver()
	: storage(VLC->riverTypeHandler->objects.size())
{
	for(const auto & river : VLC->riverTypeHandler->objects)
		storage.load(river->getIndex(), river->tilesFilename);
}

void MapRendererRiver::renderTile(const IMapRendererContext & context, Canvas & target, const int3 & coordinates)
{
	const TerrainTile & mapTile = context.getMapTile(coordinates);

	if(mapTile.riverType->getId() != River::NO_RIVER)
	{
		int32_t terrainIndex = mapTile.riverType->getIndex();
		int32_t imageIndex = mapTile.riverDir;
		int32_t rotationIndex = (mapTile.extTileFlags >> 2) % 4;

		const auto & image = storage.find(terrainIndex, rotationIndex, imageIndex);
		target.draw(image, Point(0, 0));
	}
}

MapRendererRoad::MapRendererRoad():
	storage(VLC->roadTypeHandler->objects.size())
{
	for(const auto & road : VLC->roadTypeHandler->objects)
		storage.load(road->getIndex(), road->tilesFilename);
}

void MapRendererRoad::renderTile(const IMapRendererContext & context, Canvas & target, const int3 & coordinates)
{
	const int3 coordinatesAbove = coordinates - int3(0,1,0);

	if (context.isInMap(coordinatesAbove))
	{
		const TerrainTile & mapTileAbove = context.getMapTile(coordinatesAbove);
		if (mapTileAbove.roadType->getId() != Road::NO_ROAD)
		{
			int32_t terrainIndex = mapTileAbove.roadType->getIndex();
			int32_t imageIndex = mapTileAbove.roadDir;
			int32_t rotationIndex = (mapTileAbove.extTileFlags >> 4) % 4;

			const auto & image = storage.find(terrainIndex, rotationIndex, imageIndex);
			target.draw(image, Point(0,0), Rect(0, 16, 32, 16));
		}
	}

	const TerrainTile & mapTile = context.getMapTile(coordinates);
	if(mapTile.roadType->getId() != Road::NO_ROAD)
	{
		int32_t terrainIndex = mapTile.roadType->getIndex();
		int32_t imageIndex = mapTile.roadDir;
		int32_t rotationIndex = (mapTile.extTileFlags >> 4) % 4;

		const auto & image = storage.find(terrainIndex, rotationIndex, imageIndex);
		target.draw(image, Point(0,16), Rect(0, 0, 32, 16));
	}
}

MapRendererBorder::MapRendererBorder()
{
	animation = std::make_unique<CAnimation>("EDG");
	animation->preload();
}

size_t MapRendererBorder::getIndexForTile(const IMapRendererContext & context, const int3 & tile)
{
	assert(!context.isInMap(tile));

	int3 size = context.getMapSize();

	if (tile.x < -1 || tile.x > size.x || tile.y < -1 || tile.y > size.y)
		return std::abs(tile.x) % 4 + 4*(std::abs(tile.y) % 4);

	if (tile.x == -1 && tile.y == -1)
		return 16;

	if (tile.x == size.x && tile.y == -1)
		return 17;

	if (tile.x == size.x && tile.y == size.y)
		return 18;

	if (tile.x == -1 && tile.y == size.y)
		return 19;

	if (tile.y == -1)
		return 20 + (tile.x % 4);

	if (tile.x == size.x)
		return 24 + (tile.y % 4);

	if (tile.y == size.y)
		return 28 + (tile.x % 4);

	if (tile.x == -1)
		return 32 + (tile.y % 4);

	//else - visible area, no renderable border
	assert(0);
	return 0;
}

void MapRendererBorder::renderTile(const IMapRendererContext & context, Canvas & target, const int3 & coordinates)
{
	const auto & image = animation->getImage(getIndexForTile(context, coordinates));
	target.draw(image, Point(0,0));
}

MapRendererFow::MapRendererFow()
{
	fogOfWarFullHide = std::make_unique<CAnimation>("TSHRC");
	fogOfWarFullHide->preload();
	fogOfWarPartialHide = std::make_unique<CAnimation>("TSHRE");
	fogOfWarPartialHide->preload();

	static const std::vector<int> rotations = {22, 15, 2, 13, 12, 16, 28, 17, 20, 19, 7, 24, 26, 25, 30, 32, 27};

	size_t size = fogOfWarPartialHide->size(0);//group size after next rotation

	for(const int rotation : rotations)
	{
		fogOfWarPartialHide->duplicateImage(0, rotation, 0);
		auto image = fogOfWarPartialHide->getImage(size, 0);
		image->verticalFlip();
		size++;
	}
}

void MapRendererFow::renderTile(const IMapRendererContext & context, Canvas & target, const int3 & coordinates)
{
	assert(!context.isVisible(coordinates));

	const NeighborTilesInfo neighborInfo(context, coordinates);

	int retBitmapID = neighborInfo.getBitmapID();// >=0 -> partial hide, <0 - full hide
	if (retBitmapID < 0)
	{
		// generate a number that is predefined for each tile,
		// but appears random to break visible pattern in large areas of fow
		// current approach (use primes as magic numbers for formula) looks to be suitable
		size_t pseudorandomNumber = ((coordinates.x * 997) ^ (coordinates.y * 1009)) / 101;
		size_t imageIndex = pseudorandomNumber % fogOfWarFullHide->size();

		target.draw(fogOfWarFullHide->getImage(imageIndex), Point(0,0));
	}
	else
	{
		target.draw(fogOfWarPartialHide->getImage(retBitmapID), Point(0,0));
	}
}

std::shared_ptr<CAnimation> MapRendererObjects::getAnimation(const CGObjectInstance* obj)
{
	const auto & info = obj->appearance;

	//the only(?) invisible object
	if(info->id == Obj::EVENT)
		return std::shared_ptr<CAnimation>();

	if(info->animationFile.empty())
	{
		logGlobal->warn("Def name for obj (%d,%d) is empty!", info->id, info->subid);
		return std::shared_ptr<CAnimation>();
	}

	return getAnimation(info->animationFile);
}

std::shared_ptr<CAnimation> MapRendererObjects::getAnimation(const std::string & filename)
{
	if (animations.count(filename))
		return animations[filename];

	auto ret = std::make_shared<CAnimation>(filename);
	animations[filename] = ret;
	ret->preload();

	return ret;
}

void MapRendererObjects::initializeObjects(const IMapRendererContext & context)
{
	auto mapSize = context.getMapSize();

	objects.resize(boost::extents[mapSize.z][mapSize.x][mapSize.y]);

	for(const auto & obj : context.getAllObjects())
	{
		if(!obj)
			continue;

		if(obj->ID == Obj::HERO && dynamic_cast<const CGHeroInstance *>(obj.get())->inTownGarrison)
			continue;

		if(obj->ID == Obj::BOAT && dynamic_cast<const CGBoat *>(obj.get())->hero)
			continue;

		std::shared_ptr<CAnimation> animation = getAnimation(obj);

		//no animation at all, e.g. Event
		if(!animation)
			continue;

		//empty animation. Illegal?
		assert(animation->size(0) > 0);
		if(animation->size(0) == 0)
			continue;

		auto image = animation->getImage(0, 0);

		int imageWidthTiles = (image->width() + 31) / 32;
		int imageHeightTiles = (image->height() + 31) / 32;

		int objectWidth = std::min(obj->getWidth(), imageWidthTiles);
		int objectHeight = std::min(obj->getHeight(), imageHeightTiles);

		for(int fx = 0; fx < objectWidth; ++fx)
		{
			for(int fy = 0; fy < objectHeight; ++fy)
			{
				int3 currTile(obj->pos.x - fx, obj->pos.y - fy, obj->pos.z);

				if(context.isInMap(currTile) && obj->coveringAt(currTile.x, currTile.y))
					objects[currTile.z][currTile.x][currTile.y].push_back(obj->id);
			}
		}
	}

	for(int z = 0; z < mapSize.z; z++)
	{
		for(int x = 0; x < mapSize.x; x++)
		{
			for(int y = 0; y < mapSize.y; y++)
			{
				auto & array = objects[z][x][y];
				std::sort(array.begin(), array.end(), MapObjectsSorter(context));
			}
		}
	}
}

MapRendererObjects::MapRendererObjects(const IMapRendererContext & context)
{
	initializeObjects(context);
}

std::shared_ptr<CAnimation> MapRendererObjects::getFlagAnimation(const CGObjectInstance* obj)
{
	//TODO: relocate to config file?
	static const std::vector<std::string> heroFlags = {
		"AF00", "AF01", "AF02", "AF03", "AF04", "AF05", "AF06", "AF07"
	};

	//TODO: relocate to config file?
	static const std::vector<std::vector<std::string>> boatFlags = {
		{"ABF01L", "ABF01G", "ABF01R", "ABF01D", "ABF01B", "ABF01P", "ABF01W", "ABF01K"},
		{"ABF02L", "ABF02G", "ABF02R", "ABF02D", "ABF02B", "ABF02P", "ABF02W", "ABF02K"},
		{"ABF03L", "ABF03G", "ABF03R", "ABF03D", "ABF03B", "ABF03P", "ABF03W", "ABF03K"}
	};

	if(obj->ID == Obj::HERO)
	{
		assert(dynamic_cast<const CGHeroInstance *>(obj) != nullptr);
		assert(obj->tempOwner.isValidPlayer());
		return getAnimation(heroFlags[obj->tempOwner.getNum()]);
	}
	if(obj->ID == Obj::BOAT)
	{
		const auto * boat = dynamic_cast<const CGBoat *>(obj);
		assert(boat);
		assert(boat->subID < boatFlags.size());
		assert(!boat->hero || boat->hero->tempOwner.isValidPlayer());

		if(boat->hero)
			return getAnimation(boatFlags[obj->subID][boat->hero->tempOwner.getNum()]);
	}
	return nullptr;
}

std::shared_ptr<IImage> MapRendererObjects::getImage(const IMapRendererContext & context, const CGObjectInstance * obj, const std::shared_ptr<CAnimation>& animation) const
{
	if(!animation)
		return nullptr;

	size_t groupIndex = getAnimationGroup(context, obj);

	if(animation->size(groupIndex) == 0)
		return nullptr;

	size_t frameCounter = context.getAnimationTime() / context.getAnimationPeriod();
	size_t frameIndex = frameCounter % animation->size(groupIndex);

	return animation->getImage(frameIndex, groupIndex);
}

size_t MapRendererObjects::getAnimationGroup(const IMapRendererContext & context, const CGObjectInstance * obj) const
{
	// TODO
	//static const std::vector<size_t> moveGroups = {99, 10, 5, 6, 7, 8, 9, 12, 11};
	static const std::vector<size_t> idleGroups = {99, 13, 0, 1, 2, 3, 4, 15, 14};

	if(obj->ID == Obj::HERO)
	{
		const auto * hero = dynamic_cast<const CGHeroInstance *>(obj);
		return idleGroups[hero->moveDir];
	}

	if(obj->ID == Obj::BOAT)
	{
		const auto * boat = dynamic_cast<const CGBoat *>(obj);
		return idleGroups[boat->direction];
	}

	return 0;
}

void MapRendererObjects::renderImage(Canvas & target, const int3 & coordinates, const CGObjectInstance * object, const std::shared_ptr<IImage>& image)
{
	if(!image)
		return;

	image->setFlagColor(object->tempOwner);

	int3 offsetTiles(object->getPosition() - coordinates);

	Point offsetPixels(offsetTiles.x * 32, offsetTiles.y * 32);
	Point imagePos = image->dimensions() - offsetPixels - Point(32, 32);
	Point tileDimensions(32, 32);

	target.draw(image, Point(0, 0), Rect(imagePos, tileDimensions));
}

void MapRendererObjects::renderObject(const IMapRendererContext & context, Canvas & target, const int3 & coordinates, const CGObjectInstance* instance)
{
	renderImage(target, coordinates, instance, getImage(context, instance, getAnimation(instance)));
	renderImage(target, coordinates, instance, getImage(context, instance, getFlagAnimation(instance)));
}

void MapRendererObjects::renderTile(const IMapRendererContext & context, Canvas & target, const int3 & coordinates)
{
	for(const auto & objectID : objects[coordinates.z][coordinates.x][coordinates.y])
	{
		const auto * objectInstance = context.getObject(objectID);

		assert(objectInstance);
		if(!objectInstance)
		{
			logGlobal->error("Stray map object that isn't fading");
			continue;
		}

		renderObject(context, target, coordinates, objectInstance);
	}
}

void MapRendererObjects::addObject(const IMapRendererContext & context, const CGObjectInstance * object)
{

}

void MapRendererObjects::removeObject(const IMapRendererContext & context, const CGObjectInstance * object)
{

}

void MapRendererDebugGrid::renderTile(const IMapRendererContext & context, Canvas & target, const int3 & coordinates)
{
	if(context.showGrid())
	{
		target.drawLine(Point(0, 0), Point(0, 31), {128, 128, 128, 128}, {128, 128, 128, 128});
		target.drawLine(Point(0, 0), Point(31, 0), {128, 128, 128, 128}, {128, 128, 128, 128});
	}
}

MapRenderer::MapRenderer(const IMapRendererContext & context)
	: rendererObjects(context)
{

}

void MapRenderer::renderTile(const IMapRendererContext & context, Canvas & target, const int3 & coordinates)
{
	if (!context.isInMap(coordinates))
	{
		rendererBorder.renderTile(context, target, coordinates);
		return;
	}

	const NeighborTilesInfo neighborInfo(context, coordinates);

	if (neighborInfo.areAllHidden())
	{
		rendererFow.renderTile(context, target, coordinates);
	}
	else
	{
		rendererTerrain.renderTile(context, target, coordinates);
		rendererRiver.renderTile(context, target, coordinates);
		rendererRoad.renderTile(context, target, coordinates);
		rendererObjects.renderTile(context, target, coordinates);

		if (!context.isVisible(coordinates))
			rendererFow.renderTile(context, target, coordinates);
	}
	rendererDebugGrid.renderTile(context, target,coordinates);
}

void MapRenderer::addObject(const IMapRendererContext & context, const CGObjectInstance * object)
{
	rendererObjects.addObject(context, object);
}

void MapRenderer::removeObject(const IMapRendererContext & context, const CGObjectInstance * object)
{
	rendererObjects.addObject(context, object);
}
