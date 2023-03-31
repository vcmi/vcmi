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

#include "IMapRendererContext.h"
#include "mapHandler.h"

#include "../CGameInfo.h"
#include "../render/CAnimation.h"
#include "../render/Canvas.h"
#include "../render/IImage.h"

#include "../../CCallback.h"

#include "../../lib/CPathfinder.h"
#include "../../lib/RiverHandler.h"
#include "../../lib/RoadHandler.h"
#include "../../lib/TerrainHandler.h"
#include "../../lib/mapObjects/CGHeroInstance.h"
#include "../../lib/mapping/CMap.h"

struct NeighborTilesInfo
{
	//567
	//3 4
	//012
	std::bitset<8> d;

	NeighborTilesInfo(IMapRendererContext & context, const int3 & pos)
	{
		auto checkTile = [&](int dx, int dy)
		{
			return context.isVisible(pos + int3(dx, dy, 0));
		};

		// sides
		d[1] = checkTile(0, +1);
		d[3] = checkTile(-1, 0);
		d[4] = checkTile(+1, 0);
		d[6] = checkTile(0, -1);

		// corners - select visible image if either corner or adjacent sides are visible
		d[0] = d[1] || d[3] || checkTile(-1, +1);
		d[2] = d[1] || d[4] || checkTile(+1, +1);
		d[5] = d[3] || d[6] || checkTile(-1, -1);
		d[7] = d[4] || d[6] || checkTile(+1, -1);
	}

	bool areAllHidden() const
	{
		return d.none();
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

		return visBitmaps[d.to_ulong()]; // >=0 -> partial hide, <0 - full hide
	}
};

MapTileStorage::MapTileStorage(size_t capacity)
	: animations(capacity)
{
}

void MapTileStorage::load(size_t index, const std::string & filename, EImageBlitMode blitMode)
{
	auto & terrainAnimations = animations[index];

	for(auto & entry : terrainAnimations)
	{
		if (!filename.empty())
		{
			entry = std::make_unique<CAnimation>(filename);
			entry->preload();
		}
		else
			entry = std::make_unique<CAnimation>();

		for(size_t i = 0; i < entry->size(); ++i)
			entry->getImage(i)->setBlitMode(blitMode);
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
		storage.load(terrain->getIndex(), terrain->tilesFilename, EImageBlitMode::OPAQUE);
}

void MapRendererTerrain::renderTile(IMapRendererContext & context, Canvas & target, const int3 & coordinates)
{
	const TerrainTile & mapTile = context.getMapTile(coordinates);

	int32_t terrainIndex = mapTile.terType->getIndex();
	int32_t imageIndex = mapTile.terView;
	int32_t rotationIndex = mapTile.extTileFlags % 4;

	const auto & image = storage.find(terrainIndex, rotationIndex, imageIndex);

	if(mapTile.terType->getId() == ETerrainId::LAVA)
	{
		image->shiftPalette(246, 9, context.terrainImageIndex(9));
	}

	if(mapTile.terType->getId() == ETerrainId::WATER)
	{
		image->shiftPalette(229, 12, context.terrainImageIndex(12));
		image->shiftPalette(242, 14, context.terrainImageIndex(14));
	}

	target.draw(image, Point(0, 0));
}

uint8_t MapRendererTerrain::checksum(IMapRendererContext & context, const int3 & coordinates)
{
	const TerrainTile & mapTile = context.getMapTile(coordinates);

	if(mapTile.terType->getId() == ETerrainId::LAVA || mapTile.terType->getId() == ETerrainId::WATER)
		return context.terrainImageIndex(250);
	return 0xff - 1;
}

MapRendererRiver::MapRendererRiver()
	: storage(VLC->riverTypeHandler->objects.size())
{
	for(const auto & river : VLC->riverTypeHandler->objects)
		storage.load(river->getIndex(), river->tilesFilename, EImageBlitMode::COLORKEY);
}

void MapRendererRiver::renderTile(IMapRendererContext & context, Canvas & target, const int3 & coordinates)
{
	const TerrainTile & mapTile = context.getMapTile(coordinates);

	if(mapTile.riverType->getId() == River::NO_RIVER)
		return;

	int32_t terrainIndex = mapTile.riverType->getIndex();
	int32_t imageIndex = mapTile.riverDir;
	int32_t rotationIndex = (mapTile.extTileFlags >> 2) % 4;

	const auto & image = storage.find(terrainIndex, rotationIndex, imageIndex);

	if(mapTile.riverType->getId() == River::WATER_RIVER)
	{
		image->shiftPalette(183, 12, context.terrainImageIndex(12));
		image->shiftPalette(195, 6, context.terrainImageIndex(6));
	}

	if(mapTile.riverType->getId() == River::MUD_RIVER)
	{
		image->shiftPalette(228, 12, context.terrainImageIndex(12));
		image->shiftPalette(183, 6, context.terrainImageIndex(6));
		image->shiftPalette(240, 6, context.terrainImageIndex(6));
	}

	if(mapTile.riverType->getId() == River::LAVA_RIVER)
	{
		image->shiftPalette(240, 9, context.terrainImageIndex(9));
	}

	target.draw(image, Point(0, 0));
}

uint8_t MapRendererRiver::checksum(IMapRendererContext & context, const int3 & coordinates)
{
	const TerrainTile & mapTile = context.getMapTile(coordinates);

	if(mapTile.riverType->getId() == River::WATER_RIVER ||
	   mapTile.riverType->getId() == River::MUD_RIVER ||
	   mapTile.riverType->getId() == River::LAVA_RIVER)
		return context.terrainImageIndex(250);
	return 0xff-1;
}

MapRendererRoad::MapRendererRoad()
	: storage(VLC->roadTypeHandler->objects.size())
{
	for(const auto & road : VLC->roadTypeHandler->objects)
		storage.load(road->getIndex(), road->tilesFilename, EImageBlitMode::COLORKEY);
}

void MapRendererRoad::renderTile(IMapRendererContext & context, Canvas & target, const int3 & coordinates)
{
	const int3 coordinatesAbove = coordinates - int3(0, 1, 0);

	if(context.isInMap(coordinatesAbove))
	{
		const TerrainTile & mapTileAbove = context.getMapTile(coordinatesAbove);
		if(mapTileAbove.roadType->getId() != Road::NO_ROAD)
		{
			int32_t terrainIndex = mapTileAbove.roadType->getIndex();
			int32_t imageIndex = mapTileAbove.roadDir;
			int32_t rotationIndex = (mapTileAbove.extTileFlags >> 4) % 4;

			const auto & image = storage.find(terrainIndex, rotationIndex, imageIndex);
			target.draw(image, Point(0, 0), Rect(0, 16, 32, 16));
		}
	}

	const TerrainTile & mapTile = context.getMapTile(coordinates);
	if(mapTile.roadType->getId() != Road::NO_ROAD)
	{
		int32_t terrainIndex = mapTile.roadType->getIndex();
		int32_t imageIndex = mapTile.roadDir;
		int32_t rotationIndex = (mapTile.extTileFlags >> 4) % 4;

		const auto & image = storage.find(terrainIndex, rotationIndex, imageIndex);
		target.draw(image, Point(0, 16), Rect(0, 0, 32, 16));
	}
}

uint8_t MapRendererRoad::checksum(IMapRendererContext & context, const int3 & coordinates)
{
	return 0;
}

MapRendererBorder::MapRendererBorder()
{
	emptyFill = std::make_unique<Canvas>(Point(32,32));
	animation = std::make_unique<CAnimation>("EDG");
	animation->preload();
}

size_t MapRendererBorder::getIndexForTile(IMapRendererContext & context, const int3 & tile)
{
	assert(!context.isInMap(tile));

	int3 size = context.getMapSize();

	if(tile.x < -1 || tile.x > size.x || tile.y < -1 || tile.y > size.y)
		return std::abs(tile.x) % 4 + 4 * (std::abs(tile.y) % 4);

	if(tile.x == -1 && tile.y == -1)
		return 16;

	if(tile.x == size.x && tile.y == -1)
		return 17;

	if(tile.x == size.x && tile.y == size.y)
		return 18;

	if(tile.x == -1 && tile.y == size.y)
		return 19;

	if(tile.y == -1)
		return 20 + (tile.x % 4);

	if(tile.x == size.x)
		return 24 + (tile.y % 4);

	if(tile.y == size.y)
		return 28 + (tile.x % 4);

	if(tile.x == -1)
		return 32 + (tile.y % 4);

	//else - visible area, no renderable border
	assert(0);
	return 0;
}

void MapRendererBorder::renderTile(IMapRendererContext & context, Canvas & target, const int3 & coordinates)
{
	if (context.showBorder())
	{
		const auto & image = animation->getImage(getIndexForTile(context, coordinates));
		target.draw(image, Point(0, 0));
	}
	else
	{
		target.draw(*emptyFill, Point(0,0));
	}
}

uint8_t MapRendererBorder::checksum(IMapRendererContext & context, const int3 & coordinates)
{
	return 0;
}

MapRendererFow::MapRendererFow()
{
	fogOfWarFullHide = std::make_unique<CAnimation>("TSHRC");
	fogOfWarFullHide->preload();
	fogOfWarPartialHide = std::make_unique<CAnimation>("TSHRE");
	fogOfWarPartialHide->preload();

	for(size_t i = 0; i < fogOfWarFullHide->size(); ++i)
		fogOfWarFullHide->getImage(i)->setBlitMode(EImageBlitMode::OPAQUE);

	static const std::vector<int> rotations = {22, 15, 2, 13, 12, 16, 28, 17, 20, 19, 7, 24, 26, 25, 30, 32, 27};

	size_t size = fogOfWarPartialHide->size(0); //group size after next rotation

	for(const int rotation : rotations)
	{
		fogOfWarPartialHide->duplicateImage(0, rotation, 0);
		auto image = fogOfWarPartialHide->getImage(size, 0);
		image->verticalFlip();
		size++;
	}
}

void MapRendererFow::renderTile(IMapRendererContext & context, Canvas & target, const int3 & coordinates)
{
	assert(!context.isVisible(coordinates));

	const NeighborTilesInfo neighborInfo(context, coordinates);

	int retBitmapID = neighborInfo.getBitmapID(); // >=0 -> partial hide, <0 - full hide
	if(retBitmapID < 0)
	{
		// generate a number that is predefined for each tile,
		// but appears random to break visible pattern in large areas of fow
		// current approach (use primes as magic numbers for formula) looks to be suitable
		size_t pseudorandomNumber = ((coordinates.x * 997) ^ (coordinates.y * 1009)) / 101;
		size_t imageIndex = pseudorandomNumber % fogOfWarFullHide->size();

		target.draw(fogOfWarFullHide->getImage(imageIndex), Point(0, 0));
	}
	else
	{
		target.draw(fogOfWarPartialHide->getImage(retBitmapID), Point(0, 0));
	}
}

uint8_t MapRendererFow::checksum(IMapRendererContext & context, const int3 & coordinates)
{
	const NeighborTilesInfo neighborInfo(context, coordinates);
	int retBitmapID = neighborInfo.getBitmapID();
	if(retBitmapID < 0)
		return 0xff - 1;
	return retBitmapID;
}

std::shared_ptr<CAnimation> MapRendererObjects::getBaseAnimation(const CGObjectInstance* obj)
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

	bool generateMovementGroups = (info->id == Obj::BOAT) || (info->id == Obj::HERO);

	//TODO: relocate to config file?
	// Boat appearance files only contain single, unanimated image
	// proper boat animations are actually in different file
	static const std::vector<std::string> boatAnimations = {
		"AB01_.def", "AB02_.def", "AB03_.def"
	};

	if (info->id == Obj::BOAT)
		return getAnimation(boatAnimations[info->subid], generateMovementGroups);

	return getAnimation(info->animationFile, generateMovementGroups);
}

std::shared_ptr<CAnimation> MapRendererObjects::getAnimation(const std::string & filename, bool generateMovementGroups)
{
	auto it = animations.find(filename);

	if(it != animations.end())
		return it->second;

	auto ret = std::make_shared<CAnimation>(filename);
	animations[filename] = ret;
	ret->preload();

	if(generateMovementGroups)
	{
		ret->createFlippedGroup(1, 13);
		ret->createFlippedGroup(2, 14);
		ret->createFlippedGroup(3, 15);

		ret->createFlippedGroup(6, 10);
		ret->createFlippedGroup(7, 11);
		ret->createFlippedGroup(8, 12);
	}
	return ret;
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
		return getAnimation(heroFlags[obj->tempOwner.getNum()], true);
	}

	if(obj->ID == Obj::BOAT)
	{
		const auto * boat = dynamic_cast<const CGBoat *>(obj);
		assert(boat->subID < boatFlags.size());
		if (boat->hero)
		{
			assert(boat->hero->tempOwner.isValidPlayer());
			return getAnimation(boatFlags[boat->subID][boat->hero->tempOwner.getNum()], true);
		}
	}

	return nullptr;
}

std::shared_ptr<CAnimation> MapRendererObjects::getOverlayAnimation(const CGObjectInstance * obj)
{
	if(obj->ID == Obj::BOAT)
	{
		//TODO: relocate to config file?
		// Boats have additional animation with waves around boat
		static const std::vector<std::string> boatAnimations = {
			"ABM01_.def", "ABM02_.def", "ABM03_.def"
		};

		const auto * boat = dynamic_cast<const CGBoat *>(obj);
		if (boat->hero)
			return getAnimation(boatAnimations[obj->subID], true);
	}
	return nullptr;
}

std::shared_ptr<IImage> MapRendererObjects::getImage(IMapRendererContext & context, const CGObjectInstance * obj, const std::shared_ptr<CAnimation>& animation) const
{
	if(!animation)
		return nullptr;

	size_t groupIndex = context.objectGroupIndex(obj->id);

	if(animation->size(groupIndex) == 0)
		return nullptr;

	size_t frameIndex = context.objectImageIndex(obj->id, animation->size(groupIndex));

	return animation->getImage(frameIndex, groupIndex);
}

void MapRendererObjects::renderImage(IMapRendererContext & context, Canvas & target, const int3 & coordinates, const CGObjectInstance * object, const std::shared_ptr<IImage>& image)
{
	if(!image)
		return;

	auto transparency = static_cast<uint8_t>(std::round(255 * context.objectTransparency(object->id, coordinates)));

	if (transparency == 0)
		return;

	image->setAlpha(transparency);
	image->setFlagColor(object->tempOwner);

	Point offsetPixels = context.objectImageOffset(object->id, coordinates);

	if ( offsetPixels.x < image->dimensions().x && offsetPixels.y < image->dimensions().y)
	{
		Point imagePos = image->dimensions() - offsetPixels - Point(32, 32);

		//if (transparency == 255)
		//{
		//	Canvas intermediate(Point(32,32));
		//	intermediate.enableTransparency(true);
		//	image->setBlitMode(EImageBlitMode::OPAQUE);
		//	intermediate.draw(image, Point(0, 0), Rect(imagePos, Point(32,32)));
		//	target.draw(intermediate, Point(0,0));
		//	return;
		//}
		target.draw(image, Point(0, 0), Rect(imagePos, Point(32,32)));
	}
}

void MapRendererObjects::renderObject(IMapRendererContext & context, Canvas & target, const int3 & coordinates, const CGObjectInstance * instance)
{
	renderImage(context, target, coordinates, instance, getImage(context, instance, getBaseAnimation(instance)));
	renderImage(context, target, coordinates, instance, getImage(context, instance, getFlagAnimation(instance)));
	renderImage(context, target, coordinates, instance, getImage(context, instance, getOverlayAnimation(instance)));
}

void MapRendererObjects::renderTile(IMapRendererContext & context, Canvas & target, const int3 & coordinates)
{
	for(const auto & objectID : context.getObjects(coordinates))
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

uint8_t MapRendererObjects::checksum(IMapRendererContext & context, const int3 & coordinates)
{
	for(const auto & objectID : context.getObjects(coordinates))
	{
		const auto * objectInstance = context.getObject(objectID);
		size_t groupIndex = context.objectGroupIndex(objectInstance->id);
		Point offsetPixels = context.objectImageOffset(objectInstance->id, coordinates);

		auto base = getBaseAnimation(objectInstance);
		auto flag = getFlagAnimation(objectInstance);

		if (base && base->size(groupIndex) > 1)
		{
			auto imageIndex = context.objectImageIndex(objectID, base->size(groupIndex));
			auto image = base->getImage(imageIndex, groupIndex);
			if ( offsetPixels.x < image->dimensions().x && offsetPixels.y < image->dimensions().y)
				return context.objectImageIndex(objectID, 250);
		}

		if (flag && flag->size(groupIndex) > 1)
		{
			auto imageIndex = context.objectImageIndex(objectID, flag->size(groupIndex));
			auto image = flag->getImage(imageIndex, groupIndex);
			if ( offsetPixels.x < image->dimensions().x && offsetPixels.y < image->dimensions().y)
				return context.objectImageIndex(objectID, 250);
		}
	}
	return 0xff-1;
}

MapRendererDebug::MapRendererDebug()
	: imageGrid(IImage::createFromFile("debug/grid", EImageBlitMode::ALPHA))
	, imageBlocked(IImage::createFromFile("debug/blocked", EImageBlitMode::ALPHA))
	, imageVisitable(IImage::createFromFile("debug/visitable", EImageBlitMode::ALPHA))
{

}

void MapRendererDebug::renderTile(IMapRendererContext & context, Canvas & target, const int3 & coordinates)
{
	if(context.showGrid())
		target.draw(imageGrid, Point(0,0));

	if(context.showVisitable() || context.showBlocked())
	{
		bool blocking = false;
		bool visitable = false;

		for(const auto & objectID : context.getObjects(coordinates))
		{
			const auto * object = context.getObject(objectID);

			if(context.objectTransparency(objectID, coordinates) > 0 && !context.isActiveHero(object))
			{
				visitable |= object->visitableAt(coordinates.x, coordinates.y);
				blocking |= object->blockingAt(coordinates.x, coordinates.y);
			}
		}

		if (context.showBlocked() && blocking)
			target.draw(imageBlocked, Point(0,0));
		if (context.showVisitable() && visitable)
			target.draw(imageVisitable, Point(0,0));
	}
}

uint8_t MapRendererDebug::checksum(IMapRendererContext & context, const int3 & coordinates)
{
	uint8_t result = 0;

	if (context.showVisitable())
		result += 1;

	if (context.showBlocked())
		result += 2;

	if (context.showGrid())
		result += 4;

	return result;
}

MapRendererPath::MapRendererPath()
	: pathNodes(new CAnimation("ADAG"))
{
	pathNodes->preload();
}

size_t MapRendererPath::selectImageReachability(bool reachableToday, size_t imageIndex)
{
	const static size_t unreachableTodayOffset = 25;

	if(!reachableToday)
		return unreachableTodayOffset + imageIndex;

	return imageIndex;
}

size_t MapRendererPath::selectImageCross(bool reachableToday, const int3 & curr)
{
	return selectImageReachability(reachableToday, 0);
}

size_t MapRendererPath::selectImageArrow(bool reachableToday, const int3 & curr, const int3 & prev, const int3 & next)
{
	// Vector directions
	//  0   1   2
	//      |
	//  3 - 4 - 5
	//      |
	//  6   7   8
	//For example:
	//  |
	//  +->
	// is (directionToArrowIndex[7][5])
	//
	const static size_t directionToArrowIndex[9][9] = {
		{16, 17, 18, 7,  0, 19, 6,  5,  0 },
		{8,  9,  18, 7,  0, 19, 6,  0,  20},
		{8,  1,  10, 7,  0, 19, 0,  21, 20},
		{24, 17, 18, 15, 0, 0,  6,  5,  4 },
		{0,  0,  0,  0,  0, 0,  0,  0,  0 },
		{8,  1,  2,  0,  0, 11, 22, 21, 20},
		{24, 17, 0,  23, 0, 3,  14, 5,  4 },
		{24, 0,  2,  23, 0, 3,  22, 13, 4 },
		{0,  1,  2,  23, 0, 3,  22, 21, 12}
	};

	size_t enterDirection = (curr.x - next.x + 1) + 3 * (curr.y - next.y + 1);
	size_t leaveDirection = (prev.x - curr.x + 1) + 3 * (prev.y - curr.y + 1);
	size_t imageIndex = directionToArrowIndex[enterDirection][leaveDirection];

	return selectImageReachability(reachableToday, imageIndex);
}

void MapRendererPath::renderTile(IMapRendererContext & context, Canvas & target, const int3 & coordinates)
{
	size_t imageID = selectImage(context, coordinates);

	if (imageID < pathNodes->size())
		target.draw(pathNodes->getImage(imageID), Point(0,0));
}

size_t MapRendererPath::selectImage(IMapRendererContext & context, const int3 & coordinates)
{
	const auto & functor = [&](const CGPathNode & node)
	{
		return node.coord == coordinates;
	};

	const auto * path = context.currentPath();
	if(!path)
		return std::numeric_limits<size_t>::max();

	const auto & iter = boost::range::find_if(path->nodes, functor);

	if(iter == path->nodes.end())
		return std::numeric_limits<size_t>::max();

	bool reachableToday = iter->turns == 0;
	if(iter == path->nodes.begin())
		return selectImageCross(reachableToday, iter->coord);

	auto next = iter + 1;
	auto prev = iter - 1;

	// start of path - current hero location
	if(next == path->nodes.end())
		return std::numeric_limits<size_t>::max();

	bool pathContinuous = iter->coord.areNeighbours(next->coord) && iter->coord.areNeighbours(prev->coord);
	bool embarking = iter->action == CGPathNode::EMBARK || iter->action == CGPathNode::DISEMBARK;

	if(pathContinuous && !embarking)
		return selectImageArrow(reachableToday, iter->coord, prev->coord, next->coord);

	return selectImageCross(reachableToday, iter->coord);
}

uint8_t MapRendererPath::checksum(IMapRendererContext & context, const int3 & coordinates)
{
	return selectImage(context, coordinates) & 0xff;
}

MapRenderer::TileChecksum MapRenderer::getTileChecksum(IMapRendererContext & context, const int3 & coordinates)
{
	// computes basic checksum to determine whether tile needs an update
	// if any component gives different value, tile will be updated
	TileChecksum result;
	boost::range::fill(result, std::numeric_limits<uint8_t>::max());

	if(!context.isInMap(coordinates))
	{
		result[0] = rendererBorder.checksum(context, coordinates);
		return result;
	}

	const NeighborTilesInfo neighborInfo(context, coordinates);

	if(!context.isVisible(coordinates) && neighborInfo.areAllHidden())
	{
		result[7] = rendererFow.checksum(context, coordinates);
	}
	else
	{
		result[1] = rendererTerrain.checksum(context, coordinates);
		if (context.showRivers())
			result[2] = rendererRiver.checksum(context, coordinates);
		if (context.showRoads())
			result[3] = rendererRoad.checksum(context, coordinates);
		result[4] = rendererObjects.checksum(context, coordinates);
		result[5] = rendererPath.checksum(context, coordinates);
		result[6] = rendererDebug.checksum(context, coordinates);

		if(!context.isVisible(coordinates))
			result[7] = rendererFow.checksum(context, coordinates);
	}
	return result;
}

void MapRenderer::renderTile(IMapRendererContext & context, Canvas & target, const int3 & coordinates)
{
	if(!context.isInMap(coordinates))
	{
		rendererBorder.renderTile(context, target, coordinates);
		return;
	}

	const NeighborTilesInfo neighborInfo(context, coordinates);

	if(!context.isVisible(coordinates) && neighborInfo.areAllHidden())
	{
		rendererFow.renderTile(context, target, coordinates);
	}
	else
	{
		rendererTerrain.renderTile(context, target, coordinates);

		if (context.showRivers())
			rendererRiver.renderTile(context, target, coordinates);

		if (context.showRoads())
			rendererRoad.renderTile(context, target, coordinates);

		rendererObjects.renderTile(context, target, coordinates);
		rendererPath.renderTile(context, target, coordinates);
		rendererDebug.renderTile(context, target, coordinates);

		if(!context.isVisible(coordinates))
			rendererFow.renderTile(context, target, coordinates);
	}
}
