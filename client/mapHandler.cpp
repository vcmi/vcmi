/*
 * mapHandler.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "mapHandler.h"

#include "CBitmapHandler.h"
#include "gui/CAnimation.h"
#include "gui/SDL_Extensions.h"
#include "CGameInfo.h"
#include "../lib/mapObjects/CGHeroInstance.h"
#include "../lib/mapObjects/CObjectClassesHandler.h"
#include "../lib/CGameState.h"
#include "../lib/CHeroHandler.h"
#include "../lib/CTownHandler.h"
#include "Graphics.h"
#include "../lib/mapping/CMap.h"
#include "../lib/CConfigHandler.h"
#include "../lib/CGeneralTextHandler.h"
#include "../lib/GameConstants.h"
#include "../lib/CStopWatch.h"
#include "CMT.h"
#include "../lib/CRandomGenerator.h"

#define ADVOPT (conf.go()->ac)

static bool objectBlitOrderSorter(const TerrainTileObject & a, const TerrainTileObject & b)
{
	return CMapHandler::compareObjectBlitOrder(a.obj, b.obj);
}

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
	NeighborTilesInfo(const int3 & pos, const int3 & sizes, const std::vector< std::vector< std::vector<ui8> > > & visibilityMap)
	{
		auto getTile = [&](int dx, int dy)->bool
		{
			if ( dx + pos.x < 0 || dx + pos.x >= sizes.x
			  || dy + pos.y < 0 || dy + pos.y >= sizes.y)
				return false;
			return settings["session"]["spectate"].Bool() ? true : visibilityMap[dx+pos.x][dy+pos.y][pos.z];
		};
		d7 = getTile(-1, -1); //789
		d8 = getTile( 0, -1); //456
		d9 = getTile(+1, -1); //123
		d4 = getTile(-1, 0);
		d5 = visibilityMap[pos.x][pos.y][pos.z];
		d6 = getTile(+1, 0);
		d1 = getTile(-1, +1);
		d2 = getTile( 0, +1);
		d3 = getTile(+1, +1);
	}

	bool areAllHidden() const
	{
		return !(d1 || d2 || d3 || d4 || d5 || d6 || d7 || d8 || d8 );
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

void CMapHandler::prepareFOWDefs()
{
	//assume all frames in group 0
	size_t size = graphics->fogOfWarFullHide->size(0);
	FoWfullHide.resize(size);
	for(size_t frame = 0; frame < size; frame++)
		FoWfullHide[frame] = graphics->fogOfWarFullHide->getImage(frame);

	//initialization of type of full-hide image
	hideBitmap.resize(sizes.x);
	for (auto & elem : hideBitmap)
	{
		elem.resize(sizes.y);
	}
	for (auto & elem : hideBitmap)
	{
		for (int j = 0; j < sizes.y; ++j)
		{
			elem[j].resize(sizes.z);
			for(int k = 0; k < sizes.z; ++k)
			{
				elem[j][k] = CRandomGenerator::getDefault().nextInt(size - 1);
			}
		}
	}

	size = graphics->fogOfWarPartialHide->size(0);
	FoWpartialHide.resize(size);
	for(size_t frame = 0; frame < size; frame++)
		FoWpartialHide[frame] = graphics->fogOfWarPartialHide->getImage(frame);
}

EMapAnimRedrawStatus CMapHandler::drawTerrainRectNew(SDL_Surface * targetSurface, const MapDrawingInfo * info, bool redrawOnlyAnim)
{
	assert(info);
	bool hasActiveFade = updateObjectsFade();
	resolveBlitter(info)->blit(targetSurface, info);
	return hasActiveFade ? EMapAnimRedrawStatus::REDRAW_REQUESTED : EMapAnimRedrawStatus::OK;
}

void CMapHandler::initTerrainGraphics()
{
	static const std::vector<std::string> TERRAIN_FILES =
	{
		"DIRTTL",
		"SANDTL",
		"GRASTL",
		"SNOWTL",
		"SWMPTL",

		"ROUGTL",
		"SUBBTL",
		"LAVATL",
		"WATRTL",
		"ROCKTL"
	};

	static const std::vector<std::string> ROAD_FILES =
	{
		"dirtrd",
		"gravrd",
		"cobbrd"
	};

	static const std::vector<std::string> RIVER_FILES =
	{
		"clrrvr",
		"icyrvr",
		"mudrvr",
		"lavrvr"
	};

	auto loadFlipped = [this](int types, TFlippedAnimations & animation, TFlippedCache & cache, const std::vector<std::string> & files)
	{
		animation.resize(types);
		cache.resize(types);

		//no rotation and basic setup
		for(int i = 0; i < types; i++)
		{
			animation[i][0] = make_unique<CAnimation>(files[i]);
			animation[i][0]->preload();
			const size_t views = animation[i][0]->size(0);
			cache[i].resize(views);

			for(int j = 0; j < views; j++)
				cache[i][j][0] = animation[i][0]->getImage(j);
		}

		for(int rotation = 1; rotation < 4; rotation++)
		{
			for(int i = 0; i < types; i++)
			{
				animation[i][rotation] = make_unique<CAnimation>(files[i]);
				animation[i][rotation]->preload();
				const size_t views = animation[i][rotation]->size(0);

				for(int j = 0; j < views; j++)
				{
					IImage * image = animation[i][rotation]->getImage(j);

					if(rotation == 2 || rotation == 3)
						image->horizontalFlip();
					if(rotation == 1 || rotation == 3)
						image->verticalFlip();

					cache[i][j][rotation] = image;
				}
			}
		}
	};

	loadFlipped(GameConstants::TERRAIN_TYPES, terrainAnimations, terrainImages, TERRAIN_FILES);
	loadFlipped(3, roadAnimations, roadImages, ROAD_FILES);
	loadFlipped(4, riverAnimations, riverImages, RIVER_FILES);

	// Create enough room for the whole map and its frame

	ttiles.resize(sizes.x, frameW, frameW);
	for (int i=0-frameW;i<ttiles.size()-frameW;i++)
	{
		ttiles[i].resize(sizes.y, frameH, frameH);
	}
	for (int i=0-frameW;i<ttiles.size()-frameW;i++)
	{
		for (int j=0-frameH;j<(int)sizes.y+frameH;j++)
			ttiles[i][j].resize(sizes.z, 0, 0);
	}
}

void CMapHandler::initBorderGraphics()
{
	egdeImages.resize(egdeAnimation->size(0));
	for(size_t i = 0; i < egdeImages.size(); i++)
		egdeImages[i] = egdeAnimation->getImage(i);

	edgeFrames.resize(sizes.x, frameW, frameW);
	for (int i=0-frameW;i<edgeFrames.size()-frameW;i++)
	{
		edgeFrames[i].resize(sizes.y, frameH, frameH);
	}
	for (int i=0-frameW;i<edgeFrames.size()-frameW;i++)
	{
		for (int j=0-frameH;j<(int)sizes.y+frameH;j++)
			edgeFrames[i][j].resize(sizes.z, 0, 0);
	}

	auto & rand = CRandomGenerator::getDefault();

	for (int i=0-frameW; i<sizes.x+frameW; i++) //by width
	{
		for (int j=0-frameH; j<sizes.y+frameH;j++) //by height
		{
			for(int k=0; k<sizes.z; ++k) //by levels
			{
				ui8 terBitmapNum = 0;
				if(i < 0 || i > (sizes.x-1) || j < 0  || j > (sizes.y-1))
				{
					if(i==-1 && j==-1)
						terBitmapNum = 16;
					else if(i==-1 && j==(sizes.y))
						terBitmapNum = 19;
					else if(i==(sizes.x) && j==-1)
						terBitmapNum = 17;
					else if(i==(sizes.x) && j==(sizes.y))
						terBitmapNum = 18;
					else if(j == -1 && i > -1 && i < sizes.x)
						terBitmapNum = rand.nextInt(22, 23);
					else if(i == -1 && j > -1 && j < sizes.y)
						terBitmapNum = rand.nextInt(33, 34);
					else if(j == sizes.y && i >-1 && i < sizes.x)
						terBitmapNum = rand.nextInt(29, 30);
					else if(i == sizes.x && j > -1 && j < sizes.y)
						terBitmapNum = rand.nextInt(25, 26);
					else
						terBitmapNum = rand.nextInt(15);

				}
				edgeFrames[i][j][k] = terBitmapNum;
			}
		}
	}
}

void CMapHandler::initObjectRects()
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

		std::shared_ptr<CAnimation> animation = graphics->getAnimation(obj);

		//no animation at all
		if(!animation)
			continue;

		//empty animation
		if(animation->size(0) == 0)
			continue;

		IImage * image = animation->getImage(0,0);

		for(int fx=0; fx < obj->getWidth(); ++fx)
		{
			for(int fy=0; fy < obj->getHeight(); ++fy)
			{
				int3 currTile(obj->pos.x - fx, obj->pos.y - fy, obj->pos.z);
				SDL_Rect cr;
				cr.w = 32;
				cr.h = 32;
				cr.x = image->width() - fx * 32 - 32;
				cr.y = image->height() - fy * 32 - 32;
				TerrainTileObject toAdd(obj,cr);


				if( map->isInTheMap(currTile) && // within map
					cr.x + cr.w > 0 &&           // image has data on this tile
					cr.y + cr.h > 0 &&
					obj->coveringAt(currTile.x, currTile.y) // object is visible here
				  )
				{
					ttiles[currTile.x][currTile.y][currTile.z].objects.push_back(toAdd);
				}
			}
		}
	}

	for(int ix=0; ix<ttiles.size()-frameW; ++ix)
	{
		for(int iy=0; iy<ttiles[0].size()-frameH; ++iy)
		{
			for(int iz=0; iz<ttiles[0][0].size(); ++iz)
			{
				stable_sort(ttiles[ix][iy][iz].objects.begin(), ttiles[ix][iy][iz].objects.end(), objectBlitOrderSorter);
			}
		}
	}
}

void CMapHandler::init()
{
	CStopWatch th;
	th.getDiff();

	// Size of visible terrain.
	int mapW = conf.go()->ac.advmapW;
	int mapH = conf.go()->ac.advmapH;

	//sizes of terrain
	sizes.x = map->width;
	sizes.y = map->height;
	sizes.z = map->twoLevel ? 2 : 1;

	// Total number of visible tiles. Subtract the center tile, then
	// compute the number of tiles on each side, and reassemble.
	int t1, t2;
	t1 = (mapW-32)/2;
	t2 = mapW - 32 - t1;
	tilesW = 1 + (t1+31)/32 + (t2+31)/32;

	t1 = (mapH-32)/2;
	t2 = mapH - 32 - t1;
	tilesH = 1 + (t1+31)/32 + (t2+31)/32;

	// Size of the frame around the map. In extremes positions, the
	// frame must not be on the center of the map, but right on the
	// edge of the center tile.
	frameW = (mapW+31) /32 / 2;
	frameH = (mapH+31) /32 / 2;

	offsetX = (mapW - (2*frameW+1)*32)/2;
	offsetY = (mapH - (2*frameH+1)*32)/2;

	prepareFOWDefs();
	initTerrainGraphics();
	initBorderGraphics();
	logGlobal->infoStream()<<"\tPreparing FoW, terrain, roads, rivers, borders: "<<th.getDiff();
	initObjectRects();
	logGlobal->infoStream()<<"\tMaking object rects: "<<th.getDiff();
}

CMapHandler::CMapBlitter *CMapHandler::resolveBlitter(const MapDrawingInfo * info) const
{
	if (info->scaled)
		return worldViewBlitter;
	if (info->puzzleMode)
		return puzzleViewBlitter;

	return normalBlitter;
}

void CMapHandler::CMapNormalBlitter::drawElement(EMapCacheType cacheType, const IImage * source, SDL_Rect * sourceRect, SDL_Surface * targetSurf, SDL_Rect * destRect) const
{
	source->draw(targetSurf, destRect, sourceRect);
}

void CMapHandler::CMapNormalBlitter::init(const MapDrawingInfo * drawingInfo)
{
	info = drawingInfo;
	// Width and height of the portion of the map to process. Units in tiles.
	tileCount.x = parent->tilesW;
	tileCount.y = parent->tilesH;

	topTile = info->topTile;
	initPos.x = parent->offsetX + info->drawBounds->x;
	initPos.y = parent->offsetY + info->drawBounds->y;

	realTileRect = Rect(initPos.x, initPos.y, tileSize, tileSize);

	// If moving, we need to add an extra column/line
	if (info->movement.x != 0)
	{
		tileCount.x++;
		initPos.x += info->movement.x;
		if (info->movement.x > 0)
		{
			// Moving right. We still need to draw the old tile on the
			// left, so adjust our referential
			topTile.x--;
			initPos.x -= tileSize;
		}
	}

	if (info->movement.y != 0)
	{
		tileCount.y++;
		initPos.y += info->movement.y;
		if (info->movement.y > 0)
		{
			// Moving down. We still need to draw the tile on the top,
			// so adjust our referential.
			topTile.y--;
			initPos.y -= tileSize;
		}
	}

	// Reduce sizes if we go out of the full map.
	if (topTile.x < -parent->frameW)
		topTile.x = -parent->frameW;
	if (topTile.y < -parent->frameH)
		topTile.y = -parent->frameH;
	if (topTile.x + tileCount.x > parent->sizes.x + parent->frameW)
		tileCount.x = parent->sizes.x + parent->frameW - topTile.x;
	if (topTile.y + tileCount.y > parent->sizes.y + parent->frameH)
		tileCount.y = parent->sizes.y + parent->frameH - topTile.y;
}

SDL_Rect CMapHandler::CMapNormalBlitter::clip(SDL_Surface * targetSurf) const
{
	SDL_Rect prevClip;
	SDL_GetClipRect(targetSurf, &prevClip);
	SDL_SetClipRect(targetSurf, info->drawBounds);
	return prevClip;
}

CMapHandler::CMapNormalBlitter::CMapNormalBlitter(CMapHandler * parent)
	: CMapBlitter(parent)
{
	tileSize = 32;
	halfTileSizeCeil = 16;
	defaultTileRect = Rect(0, 0, tileSize, tileSize);
}

IImage * CMapHandler::CMapWorldViewBlitter::objectToIcon(Obj id, si32 subId, PlayerColor owner) const
{
	int ownerIndex = 0;
	if(owner < PlayerColor::PLAYER_LIMIT)
	{
		ownerIndex = owner.getNum() * 19;
	}
	else if (owner == PlayerColor::NEUTRAL)
	{
		ownerIndex = PlayerColor::PLAYER_LIMIT.getNum() * 19;
	}

	switch(id)
	{
	case Obj::MONOLITH_ONE_WAY_ENTRANCE:
	case Obj::MONOLITH_ONE_WAY_EXIT:
	case Obj::MONOLITH_TWO_WAY:
		return info->icons->getImage((int)EWorldViewIcon::TELEPORT);
	case Obj::SUBTERRANEAN_GATE:
		return info->icons->getImage((int)EWorldViewIcon::GATE);
	case Obj::ARTIFACT:
		return info->icons->getImage((int)EWorldViewIcon::ARTIFACT);
	case Obj::TOWN:
		return info->icons->getImage((int)EWorldViewIcon::TOWN + ownerIndex);
	case Obj::HERO:
		return info->icons->getImage((int)EWorldViewIcon::HERO + ownerIndex);
	case Obj::MINE:
		return info->icons->getImage((int)EWorldViewIcon::MINE_WOOD + subId + ownerIndex);
	case Obj::RESOURCE:
		return info->icons->getImage((int)EWorldViewIcon::RES_WOOD + subId + ownerIndex);
	}
	return nullptr;
}

void CMapHandler::CMapWorldViewBlitter::calculateWorldViewCameraPos()
{
	bool outsideLeft = topTile.x < 0;
	bool outsideTop = topTile.y < 0;
	bool outsideRight = std::max(0, topTile.x) + tileCount.x > parent->sizes.x;
	bool outsideBottom = std::max(0, topTile.y) + tileCount.y > parent->sizes.y;

	if (tileCount.x > parent->sizes.x)
		topTile.x = parent->sizes.x / 2 - tileCount.x / 2; // center viewport if the whole map can fit into the screen at once
	else if (outsideLeft)
	{
		if (outsideRight)
			topTile.x = parent->sizes.x / 2 - tileCount.x / 2;
		else
			topTile.x = 0;
	}
	else if (outsideRight)
		topTile.x = parent->sizes.x - tileCount.x;

	if (tileCount.y > parent->sizes.y)
		topTile.y = parent->sizes.y / 2 - tileCount.y / 2;
	else if (outsideTop)
	{
		if (outsideBottom)
			topTile.y = parent->sizes.y / 2 - tileCount.y / 2;
		else
			topTile.y = 0;
	}
	else if (outsideBottom)
		topTile.y = parent->sizes.y - tileCount.y;
}

void CMapHandler::CMapWorldViewBlitter::drawElement(EMapCacheType cacheType, const IImage * source, SDL_Rect * sourceRect, SDL_Surface * targetSurf, SDL_Rect * destRect) const
{
	IImage * scaled = parent->cache.requestWorldViewCacheOrCreate(cacheType, source);

	if(scaled)
		scaled->draw(targetSurf, destRect, sourceRect);
}

void CMapHandler::CMapWorldViewBlitter::drawTileOverlay(SDL_Surface * targetSurf, const TerrainTile2 & tile) const
{
	auto drawIcon = [this,targetSurf](Obj id, si32 subId, PlayerColor owner)
	{
		IImage * wvIcon = this->objectToIcon(id, subId, owner);

		if(nullptr != wvIcon)
		{
			// centering icon on the object
			Point dest(realPos.x + tileSize / 2 - wvIcon->width() / 2, realPos.y + tileSize / 2 - wvIcon->height() / 2);
			wvIcon->draw(targetSurf, dest.x, dest.y);
		}
	};

	auto & objects = tile.objects;
	for(auto & object : objects)
	{
		const CGObjectInstance * obj = object.obj;

		const bool sameLevel = obj->pos.z == pos.z;
		const bool isVisible = settings["session"]["spectate"].Bool() ? true : (*info->visibilityMap)[pos.x][pos.y][pos.z];
		const bool isVisitable = obj->visitableAt(pos.x, pos.y);

		if(sameLevel && isVisible && isVisitable)
			drawIcon(obj->ID, obj->subID, obj->tempOwner);
	}
}

void CMapHandler::CMapWorldViewBlitter::drawOverlayEx(SDL_Surface * targetSurf)
{
	if(nullptr == info->additionalIcons)
		return;

	const int3 bottomRight = pos + tileCount;

	for(const ObjectPosInfo & iconInfo : *(info->additionalIcons))
	{
		if( iconInfo.pos.z != pos.z)
			continue;

		if((iconInfo.pos.x < topTile.x) || (iconInfo.pos.y < topTile.y))
			continue;

		if((iconInfo.pos.x > bottomRight.x) || (iconInfo.pos.y > bottomRight.y))
			continue;

		realPos.x = initPos.x + (iconInfo.pos.x - topTile.x) * tileSize;
		realPos.y = initPos.x + (iconInfo.pos.y - topTile.y) * tileSize;

		IImage * wvIcon = this->objectToIcon(iconInfo.id, iconInfo.subId, iconInfo.owner);

		if(nullptr != wvIcon)
		{
			// centering icon on the object
			Point dest(realPos.x + tileSize / 2 - wvIcon->width() / 2, realPos.y + tileSize / 2 - wvIcon->height() / 2);
			wvIcon->draw(targetSurf, dest.x, dest.y);
		}
	}
}

void CMapHandler::CMapWorldViewBlitter::drawHeroFlag(SDL_Surface * targetSurf, const IImage * source, SDL_Rect * sourceRect, SDL_Rect * destRect, bool moving) const
{
	if (moving)
		return;

	CMapBlitter::drawHeroFlag(targetSurf, source, sourceRect, destRect, false);
}

void CMapHandler::CMapWorldViewBlitter::drawObject(SDL_Surface * targetSurf, const IImage * source, SDL_Rect * sourceRect, bool moving) const
{
	if (moving)
		return;

	Rect scaledSourceRect(sourceRect->x * info->scale, sourceRect->y * info->scale, sourceRect->w, sourceRect->h);
	CMapBlitter::drawObject(targetSurf, source, &scaledSourceRect, false);
}

void CMapHandler::CMapBlitter::drawTileTerrain(SDL_Surface * targetSurf, const TerrainTile & tinfo, const TerrainTile2 & tile) const
{
	Rect destRect(realTileRect);

	ui8 rotation = tinfo.extTileFlags % 4;

	drawElement(EMapCacheType::TERRAIN, parent->terrainImages[tinfo.terType][tinfo.terView][rotation], nullptr, targetSurf, &destRect);
}

void CMapHandler::CMapWorldViewBlitter::init(const MapDrawingInfo * drawingInfo)
{
	info = drawingInfo;
	parent->cache.updateWorldViewScale(info->scale);

	topTile = info->topTile;
	tileSize = (int) floorf(32.0f * info->scale);
	halfTileSizeCeil = (int)ceilf(tileSize / 2.0f);

	tileCount.x = (int) ceilf((float)info->drawBounds->w / tileSize);
	tileCount.y = (int) ceilf((float)info->drawBounds->h / tileSize);

	initPos.x = info->drawBounds->x;
	initPos.y = info->drawBounds->y;

	realTileRect = Rect(initPos.x, initPos.y, tileSize, tileSize);
	defaultTileRect = Rect(0, 0, tileSize, tileSize);

	calculateWorldViewCameraPos();
}

SDL_Rect CMapHandler::CMapWorldViewBlitter::clip(SDL_Surface * targetSurf) const
{
	SDL_Rect prevClip;

	SDL_FillRect(targetSurf, info->drawBounds, SDL_MapRGB(targetSurf->format, 0, 0, 0));
	// makes the clip area smaller if the map is smaller than the screen frame
	// (actually, it could be made 1 tile bigger so that overlay icons on edge tiles could be drawn partly outside)
	Rect clipRect(std::max<int>(info->drawBounds->x, info->drawBounds->x - topTile.x * tileSize),
				  std::max<int>(info->drawBounds->y, info->drawBounds->y - topTile.y * tileSize),
				  std::min<int>(info->drawBounds->w, parent->sizes.x * tileSize),
				  std::min<int>(info->drawBounds->h, parent->sizes.y * tileSize));
	SDL_GetClipRect(targetSurf, &prevClip);
	SDL_SetClipRect(targetSurf, &clipRect); //preventing blitting outside of that rect
	return prevClip;
}

CMapHandler::CMapWorldViewBlitter::CMapWorldViewBlitter(CMapHandler * parent)
	: CMapBlitter(parent)
{
}

void CMapHandler::CMapPuzzleViewBlitter::drawObjects(SDL_Surface * targetSurf, const TerrainTile2 & tile) const
{
	CMapBlitter::drawObjects(targetSurf, tile);

	// grail X mark
	if(pos.x == info->grailPos.x && pos.y == info->grailPos.y)
	{
		const IImage * mark = graphics->heroMoveArrows->getImage(0);
		mark->draw(targetSurf,realTileRect.x,realTileRect.y);
	}
}

void CMapHandler::CMapPuzzleViewBlitter::postProcessing(SDL_Surface * targetSurf) const
{
	CSDL_Ext::applyEffect(targetSurf, info->drawBounds, static_cast<int>(!ADVOPT.puzzleSepia));
}

bool CMapHandler::CMapPuzzleViewBlitter::canDrawObject(const CGObjectInstance * obj) const
{
	if (!CMapBlitter::canDrawObject(obj))
		return false;

	//don't print flaggable objects in puzzle mode
	if (obj->isVisitable())
		return false;

	if(std::find(unblittableObjects.begin(), unblittableObjects.end(), obj->ID) != unblittableObjects.end())
		return false;

	return true;
}

CMapHandler::CMapPuzzleViewBlitter::CMapPuzzleViewBlitter(CMapHandler * parent)
	: CMapNormalBlitter(parent)
{
	unblittableObjects.push_back(Obj::HOLE);
}

CMapHandler::CMapBlitter::CMapBlitter(CMapHandler * p)
	:parent(p), tileSize(0), halfTileSizeCeil(0), info(nullptr)
{

}

CMapHandler::CMapBlitter::~CMapBlitter() = default;

void CMapHandler::CMapBlitter::drawFrame(SDL_Surface * targetSurf) const
{
	Rect destRect(realTileRect);
	drawElement(EMapCacheType::FRAME, parent->egdeImages[parent->edgeFrames[pos.x][pos.y][topTile.z]], nullptr, targetSurf, &destRect);
}

void CMapHandler::CMapBlitter::drawOverlayEx(SDL_Surface * targetSurf)
{
//nothing to do here
}

void CMapHandler::CMapBlitter::drawHeroFlag(SDL_Surface * targetSurf, const IImage * source, SDL_Rect * sourceRect, SDL_Rect * destRect, bool moving) const
{
	drawElement(EMapCacheType::HERO_FLAGS, source, sourceRect, targetSurf, destRect);
}

void CMapHandler::CMapBlitter::drawObject(SDL_Surface * targetSurf, const IImage * source, SDL_Rect * sourceRect, bool moving) const
{
	Rect dstRect(realTileRect);
	drawElement(EMapCacheType::OBJECTS, source, sourceRect, targetSurf, &dstRect);
}

void CMapHandler::CMapBlitter::drawObjects(SDL_Surface * targetSurf, const TerrainTile2 & tile) const
{
	auto & objects = tile.objects;
	for(auto & object : objects)
	{
		if (object.fadeAnimKey >= 0)
		{
			auto fadeIter = parent->fadeAnims.find(object.fadeAnimKey);
			if (fadeIter != parent->fadeAnims.end())
			{
				// this object is currently fading, so skip normal drawing
				Rect r2(realTileRect);
				CFadeAnimation * fade = (*fadeIter).second.second;
				fade->draw(targetSurf, nullptr, &r2);
				continue;
			}
			logGlobal->errorStream() << "Fading map object with missing fade anim : " << object.fadeAnimKey;
			continue;
		}

		const CGObjectInstance * obj = object.obj;
		if (!obj)
		{
			logGlobal->errorStream() << "Stray map object that isn't fading";
			continue;
		}

		if (!canDrawObject(obj))
			continue;

		auto objData = findObjectBitmap(obj, info->anim);
		if (objData.objBitmap)
		{
			Rect srcRect(object.rect.x, object.rect.y, tileSize, tileSize);

			drawObject(targetSurf, objData.objBitmap, &srcRect, objData.isMoving);
			if (objData.flagBitmap)
			{
				if (objData.isMoving)
				{
					srcRect.y += FRAMES_PER_MOVE_ANIM_GROUP * 2 - tileSize;
					Rect dstRect(realPos.x, realPos.y - tileSize / 2, tileSize, tileSize);
					drawHeroFlag(targetSurf, objData.flagBitmap, &srcRect, &dstRect, true);
				}
				else if (obj->pos.x == pos.x && obj->pos.y == pos.y)
				{
					Rect dstRect(realPos.x - 2 * tileSize, realPos.y - tileSize, 3 * tileSize, 2 * tileSize);
					drawHeroFlag(targetSurf, objData.flagBitmap, nullptr, &dstRect, false);
				}
			}
		}
	}
}

void CMapHandler::CMapBlitter::drawRoad(SDL_Surface * targetSurf, const TerrainTile & tinfo, const TerrainTile * tinfoUpper) const
{
	if (tinfoUpper && tinfoUpper->roadType != ERoadType::NO_ROAD)
	{
		ui8 rotation = (tinfoUpper->extTileFlags >> 4) % 4;
		Rect source(0, tileSize / 2, tileSize, tileSize / 2);
		Rect dest(realPos.x, realPos.y, tileSize, tileSize / 2);
		drawElement(EMapCacheType::ROADS, parent->roadImages[tinfoUpper->roadType - 1][tinfoUpper->roadDir][rotation],
				&source, targetSurf, &dest);
	}

	if(tinfo.roadType != ERoadType::NO_ROAD) //print road from this tile
	{
		ui8 rotation = (tinfo.extTileFlags >> 4) % 4;
		Rect source(0, 0, tileSize, halfTileSizeCeil);
		Rect dest(realPos.x, realPos.y + tileSize / 2, tileSize, tileSize / 2);
		drawElement(EMapCacheType::ROADS, parent->roadImages[tinfo.roadType - 1][tinfo.roadDir][rotation],
				&source, targetSurf, &dest);
	}
}

void CMapHandler::CMapBlitter::drawRiver(SDL_Surface * targetSurf, const TerrainTile & tinfo) const
{
	Rect destRect(realTileRect);
	ui8 rotation = (tinfo.extTileFlags >> 2) % 4;
	drawElement(EMapCacheType::RIVERS, parent->riverImages[tinfo.riverType-1][tinfo.riverDir][rotation], nullptr, targetSurf, &destRect);
}

void CMapHandler::CMapBlitter::drawFow(SDL_Surface * targetSurf) const
{
	const NeighborTilesInfo neighborInfo(pos, parent->sizes, *info->visibilityMap);

	int retBitmapID = neighborInfo.getBitmapID();// >=0 -> partial hide, <0 - full hide
	if (retBitmapID < 0)
		retBitmapID = - parent->hideBitmap[pos.x][pos.y][pos.z] - 1; //fully hidden

	const IImage * image = nullptr;

	if (retBitmapID >= 0)
		image = parent->FoWpartialHide.at(retBitmapID);
	else
		image = parent->FoWfullHide.at(-retBitmapID - 1);

	Rect destRect(realTileRect);
	drawElement(EMapCacheType::FOW, image, nullptr, targetSurf, &destRect);
}

void CMapHandler::CMapBlitter::blit(SDL_Surface * targetSurf, const MapDrawingInfo * info)
{
	init(info);
	auto prevClip = clip(targetSurf);

	pos = int3(0, 0, topTile.z);

	for (realPos.x = initPos.x, pos.x = topTile.x; pos.x < topTile.x + tileCount.x; pos.x++, realPos.x += tileSize)
	{
		if (pos.x < 0 || pos.x >= parent->sizes.x)
			continue;

		for (realPos.y = initPos.y, pos.y = topTile.y; pos.y < topTile.y + tileCount.y; pos.y++, realPos.y += tileSize)
		{
			if (pos.y < 0 || pos.y >= parent->sizes.y)
				continue;

			const bool isVisible = canDrawCurrentTile();

			realTileRect.x = realPos.x;
			realTileRect.y = realPos.y;

			const TerrainTile2 & tile = parent->ttiles[pos.x][pos.y][pos.z];
			const TerrainTile & tinfo = parent->map->getTile(pos);
			const TerrainTile * tinfoUpper = pos.y > 0 ? &parent->map->getTile(int3(pos.x, pos.y - 1, pos.z)) : nullptr;

			if(isVisible || info->showAllTerrain)
			{
				drawTileTerrain(targetSurf, tinfo, tile);
				if (tinfo.riverType)
					drawRiver(targetSurf, tinfo);
				drawRoad(targetSurf, tinfo, tinfoUpper);
			}

			if(isVisible)
				drawObjects(targetSurf, tile);
		}
	}

	for (realPos.x = initPos.x, pos.x = topTile.x; pos.x < topTile.x + tileCount.x; pos.x++, realPos.x += tileSize)
	{
		for (realPos.y = initPos.y, pos.y = topTile.y; pos.y < topTile.y + tileCount.y; pos.y++, realPos.y += tileSize)
		{
			realTileRect.x = realPos.x;
			realTileRect.y = realPos.y;

			if (pos.x < 0 || pos.x >= parent->sizes.x ||
				pos.y < 0 || pos.y >= parent->sizes.y)
			{
				drawFrame(targetSurf);
			}
			else
			{
				const TerrainTile2 & tile = parent->ttiles[pos.x][pos.y][pos.z];

				if(!settings["session"]["spectate"].Bool() && !(*info->visibilityMap)[pos.x][pos.y][topTile.z] && !info->showAllTerrain)
					drawFow(targetSurf);

				// overlay needs to be drawn over fow, because of artifacts-aura-like spells
				drawTileOverlay(targetSurf, tile);

				// drawDebugVisitables()
				if (settings["session"]["showBlock"].Bool())
				{
					if(parent->map->getTile(int3(pos.x, pos.y, pos.z)).blocked) //temporary hiding blocked positions
					{
						static SDL_Surface * block = nullptr;
						if (!block)
							block = BitmapHandler::loadBitmap("blocked");

						CSDL_Ext::blitSurface(block, nullptr, targetSurf, &realTileRect);
					}
				}
				if (settings["session"]["showVisit"].Bool())
				{
					if(parent->map->getTile(int3(pos.x, pos.y, pos.z)).visitable) //temporary hiding visitable positions
					{
						static SDL_Surface * visit = nullptr;
						if (!visit)
							visit = BitmapHandler::loadBitmap("visitable");

						CSDL_Ext::blitSurface(visit, nullptr, targetSurf, &realTileRect);
					}
				}
			}
		}
	}

	drawOverlayEx(targetSurf);

	// drawDebugGrid()
	if (settings["session"]["showGrid"].Bool())
	{
		for (realPos.x = initPos.x, pos.x = topTile.x; pos.x < topTile.x + tileCount.x; pos.x++, realPos.x += tileSize)
		{
			for (realPos.y = initPos.y, pos.y = topTile.y; pos.y < topTile.y + tileCount.y; pos.y++, realPos.y += tileSize)
			{
				const int3 color(0x555555, 0x555555, 0x555555);

				if (realPos.y >= info->drawBounds->y &&
					realPos.y < info->drawBounds->y + info->drawBounds->h)
					for(int i = 0; i < tileSize; i++)
						if (realPos.x + i >= info->drawBounds->x &&
							realPos.x + i < info->drawBounds->x + info->drawBounds->w)
							CSDL_Ext::SDL_PutPixelWithoutRefresh(targetSurf, realPos.x + i, realPos.y, color.x, color.y, color.z);

				if (realPos.x >= info->drawBounds->x &&
					realPos.x < info->drawBounds->x + info->drawBounds->w)
					for(int i = 0; i < tileSize; i++)
						if (realPos.y + i >= info->drawBounds->y &&
							realPos.y + i < info->drawBounds->y + info->drawBounds->h)
							CSDL_Ext::SDL_PutPixelWithoutRefresh(targetSurf, realPos.x, realPos.y + i, color.x, color.y, color.z);
			}
		}
	}

	postProcessing(targetSurf);

	SDL_SetClipRect(targetSurf, &prevClip);
}

CMapHandler::AnimBitmapHolder CMapHandler::CMapBlitter::findHeroBitmap(const CGHeroInstance * hero, int anim) const
{
	if(hero && hero->moveDir && hero->type) //it's hero or boat
	{
		if(hero->tempOwner >= PlayerColor::PLAYER_LIMIT) //Neutral hero?
		{
			logGlobal->errorStream() << "A neutral hero (" << hero->name << ") at " << hero->pos << ". Should not happen!";
			return CMapHandler::AnimBitmapHolder();
		}

		//pick graphics of hero (or boat if hero is sailing)
		std::shared_ptr<CAnimation> animation;
		if (hero->boat)
			animation = graphics->boatAnimations[hero->boat->subID];
		else
			animation = graphics->heroAnimations[hero->appearance.animationFile];

		bool moving = !hero->isStanding;
		int group = getHeroFrameGroup(hero->moveDir, moving);

        if(animation->size(group) > 0)
		{
			int frame = anim % animation->size(group);
			IImage * heroImage = animation->getImage(frame, group);

			//get flag overlay only if we have main image
			IImage * flagImage = findFlagBitmap(hero, anim, &hero->tempOwner, group);

			return CMapHandler::AnimBitmapHolder(heroImage, flagImage, moving);
		}
	}
	return CMapHandler::AnimBitmapHolder();
}

CMapHandler::AnimBitmapHolder CMapHandler::CMapBlitter::findBoatBitmap(const CGBoat * boat, int anim) const
{
	auto animation = graphics->boatAnimations.at(boat->subID);
	int group = getHeroFrameGroup(boat->direction, false);
	if(animation->size(group) > 0)
		return CMapHandler::AnimBitmapHolder(animation->getImage(anim % animation->size(group), group));
	else
		return CMapHandler::AnimBitmapHolder();
}

IImage * CMapHandler::CMapBlitter::findFlagBitmap(const CGHeroInstance * hero, int anim, const PlayerColor * color, int group) const
{
	if (!hero)
		return nullptr;

	if (hero->boat)
		return findBoatFlagBitmap(hero->boat, anim, color, group, hero->moveDir);
	return findHeroFlagBitmap(hero, anim, color, group);
}

IImage * CMapHandler::CMapBlitter::findHeroFlagBitmap(const CGHeroInstance * hero, int anim, const PlayerColor * color, int group) const
{
	return findFlagBitmapInternal(graphics->heroFlagAnimations.at(color->getNum()), anim, group, hero->moveDir, !hero->isStanding);
}

IImage * CMapHandler::CMapBlitter::findBoatFlagBitmap(const CGBoat * boat, int anim, const PlayerColor * color, int group, ui8 dir) const
{
	int boatType = boat->subID;
	if(boatType < 0 || boatType >= graphics->boatFlagAnimations.size())
	{
		logGlobal->errorStream() << "Not supported boat subtype: " << boat->subID;
		return nullptr;
	}

	const auto & subtypeFlags = graphics->boatFlagAnimations.at(boatType);

	int colorIndex = color->getNum();

	if(colorIndex < 0 || colorIndex >= subtypeFlags.size())
	{
		logGlobal->errorStream() << "Invalid player color " << colorIndex;
		return nullptr;
	}

	return findFlagBitmapInternal(subtypeFlags.at(colorIndex), anim, group, dir, false);
}

IImage * CMapHandler::CMapBlitter::findFlagBitmapInternal(std::shared_ptr<CAnimation> animation, int anim, int group, ui8 dir, bool moving) const
{
	size_t groupSize = animation->size(group);
	if(groupSize == 0)
		return nullptr;

	if(moving)
		return animation->getImage(anim % groupSize, group);
	else
		return animation->getImage((anim / 4) % groupSize, group);
}

CMapHandler::AnimBitmapHolder CMapHandler::CMapBlitter::findObjectBitmap(const CGObjectInstance * obj, int anim) const
{
	if (!obj)
		return CMapHandler::AnimBitmapHolder();
	if (obj->ID == Obj::HERO)
		return findHeroBitmap(static_cast<const CGHeroInstance*>(obj), anim);
	if (obj->ID == Obj::BOAT)
		return findBoatBitmap(static_cast<const CGBoat*>(obj), anim);

	// normal object
	std::shared_ptr<CAnimation> animation = graphics->getAnimation(obj);
	size_t groupSize = animation->size();
    if(groupSize == 0)
		return CMapHandler::AnimBitmapHolder();

	IImage * bitmap = animation->getImage((anim + getPhaseShift(obj)) % groupSize);
	if(!bitmap)
		return CMapHandler::AnimBitmapHolder();

	bitmap->setFlagColor(obj->tempOwner);

	return CMapHandler::AnimBitmapHolder(bitmap);
}

ui8 CMapHandler::CMapBlitter::getPhaseShift(const CGObjectInstance *object) const
{
	auto i = parent->animationPhase.find(object);
	if(i == parent->animationPhase.end())
	{
		ui8 ret = CRandomGenerator::getDefault().nextInt(254);
		parent->animationPhase[object] = ret;
		return ret;
	}

	return i->second;
}

bool CMapHandler::CMapBlitter::canDrawObject(const CGObjectInstance * obj) const
{
	//checking if object has non-empty graphic on this tile
	return obj->ID == Obj::HERO || obj->coveringAt(pos.x, pos.y);
}

bool CMapHandler::CMapBlitter::canDrawCurrentTile() const
{
	if(settings["session"]["spectate"].Bool())
		return true;

	const NeighborTilesInfo neighbors(pos, parent->sizes, *info->visibilityMap);
	return !neighbors.areAllHidden();
}

ui8 CMapHandler::CMapBlitter::getHeroFrameGroup(ui8 dir, bool isMoving) const
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

bool CMapHandler::updateObjectsFade()
{
	for (auto iter = fadeAnims.begin(); iter != fadeAnims.end(); )
	{
		int3 pos = (*iter).second.first;
		CFadeAnimation * anim = (*iter).second.second;

		anim->update();

		if (anim->isFading())
			++iter;
		else // fade finished
		{
			auto &objs = ttiles[pos.x][pos.y][pos.z].objects;
			for (auto objIter = objs.begin(); objIter != objs.end(); ++objIter)
			{
				if ((*objIter).fadeAnimKey == (*iter).first)
				{
					logAnim->traceStream() << "Fade anim finished for obj at " << pos << "; remaining: " << (fadeAnims.size() - 1);
					if (anim->fadingMode == CFadeAnimation::EMode::OUT)
						objs.erase(objIter); // if this was fadeout, remove the object from the map
					else
						(*objIter).fadeAnimKey = -1; // for fadein, just remove its connection to the finished fade
					break;
				}
			}
			delete (*iter).second.second;
			iter = fadeAnims.erase(iter);
		}
	}

	return !fadeAnims.empty();
}

bool CMapHandler::startObjectFade(TerrainTileObject & obj, bool in, int3 pos)
{
	SDL_Surface * fadeBitmap;
	assert(obj.obj);

	auto objData = normalBlitter->findObjectBitmap(obj.obj, 0);
	if (objData.objBitmap)
	{
		if (objData.isMoving) // ignore fading of moving objects (for now?)
		{
			logAnim->debugStream() << "Ignoring fade of moving object";
			return false;
		}

		fadeBitmap = CSDL_Ext::newSurface(32, 32); // TODO cache these bitmaps instead of creating new ones?
		Rect objSrcRect(obj.rect.x, obj.rect.y, 32, 32);
		objData.objBitmap->draw(fadeBitmap,0,0,&objSrcRect);

		if (objData.flagBitmap)
		{
			if (obj.obj->pos.x - 1 == pos.x && obj.obj->pos.y - 1 == pos.y) // -1 to draw flag in top-center instead of right-bottom; kind of a hack
			{
				Rect flagSrcRect(32, 0, 32, 32);
				objData.flagBitmap->draw(fadeBitmap,0,0, &flagSrcRect);
			}
		}
		auto anim = new CFadeAnimation();
		anim->init(in ? CFadeAnimation::EMode::IN : CFadeAnimation::EMode::OUT, fadeBitmap, true);
		fadeAnims[++fadeAnimCounter] = std::pair<int3, CFadeAnimation*>(pos, anim);
		obj.fadeAnimKey = fadeAnimCounter;

		logAnim->traceStream() << "Fade anim started for obj " << obj.obj->ID
							   << " at " << pos << "; anim count: " << fadeAnims.size();
		return true;
	}

	return false;
}

bool CMapHandler::printObject(const CGObjectInstance * obj, bool fadein)
{
	auto animation = graphics->getAnimation(obj);

	if(!animation)
		return false;

	IImage * bitmap = animation->getImage(0);

	if(!bitmap)
		return false;

	const int tilesW = bitmap->width()/32;
	const int tilesH = bitmap->height()/32;

	for(int fx=0; fx<tilesW; ++fx)
	{
		for(int fy=0; fy<tilesH; ++fy)
		{
			SDL_Rect cr;
			cr.w = 32;
			cr.h = 32;
			cr.x = fx*32;
			cr.y = fy*32;
			TerrainTileObject toAdd(obj, cr);

			if((obj->pos.x + fx - tilesW+1)>=0 && (obj->pos.x + fx - tilesW+1)<ttiles.size()-frameW && (obj->pos.y + fy - tilesH+1)>=0 && (obj->pos.y + fy - tilesH+1)<ttiles[0].size()-frameH)
			{
				int3 pos(obj->pos.x + fx - tilesW + 1, obj->pos.y + fy - tilesH + 1, obj->pos.z);
				TerrainTile2 & curt = ttiles[pos.x][pos.y][pos.z];

				if (fadein && ADVOPT.objectFading)
				{
					startObjectFade(toAdd, true, pos);
				}

				auto i = curt.objects.begin();
				for(; i != curt.objects.end(); i++)
				{
					if(objectBlitOrderSorter(toAdd, *i))
					{
						curt.objects.insert(i, toAdd);
						i = curt.objects.begin(); //to validate and avoid adding it second time
						break;
					}
				}

				if(i == curt.objects.end())
					curt.objects.insert(i, toAdd);
			}
		}
	}
	return true;
}

bool CMapHandler::hideObject(const CGObjectInstance * obj, bool fadeout)
{
	//optimized version which reveals weird bugs with missing def name
	//auto pos = obj->pos;

	//for (size_t i = pos.x; i > pos.x - obj->getWidth(); i--)
	//{
	//	for (size_t j = pos.y; j > pos.y - obj->getHeight(); j--)
	//	{
	//		int3 t(i, j, pos.z);
	//		if (!map->isInTheMap(t))
	//			continue;

	//		auto &objs = ttiles[i][j][pos.z].objects;
	//		for (size_t x = 0; x < objs.size(); x++)
	//		{
	//			auto ourObj = objs[x].obj;
	//			if (ourObj && ourObj->id == obj->id)
	//			{
	//				if (fadeout && ADVOPT.objectFading) // object should be faded == erase is delayed until the end of fadeout
	//				{
	//					if (startObjectFade(objs[x], false, t))
	//						objs[x].obj = nullptr; //set original pointer to null
	//					else
	//						objs.erase(objs.begin() + x);
	//				}
	//				else
	//					objs.erase(objs.begin() + x);
	//				break;
	//			}
	//		}
	//	}

	//}

	for (size_t i = 0; i<map->width; i++)
	{
		for (size_t j = 0; j<map->height; j++)
		{
			for (size_t k = 0; k<(map->twoLevel ? 2 : 1); k++)
			{
				auto &objs = ttiles[i][j][k].objects;
				for (size_t x = 0; x < objs.size(); x++)
				{
					if (objs[x].obj && objs[x].obj->id == obj->id)
					{
						if (fadeout && ADVOPT.objectFading) // object should be faded == erase is delayed until the end of fadeout
						{
							if (startObjectFade(objs[x], false, int3(i, j, k)))
								objs[x].obj = nullptr;
							else
								objs.erase(objs.begin() + x);
						}
						else
							objs.erase(objs.begin() + x);
						break;
					}
				}
			}
		}
	}

	return true;
}

bool CMapHandler::canStartHeroMovement()
{
	return fadeAnims.empty(); // don't allow movement during fade animation
}

void CMapHandler::updateWater() //shift colors in palettes of water tiles
{
	for(auto & elem : terrainImages[7])
	{
		for(IImage * img : elem)
			img->shiftPalette(246, 9);
	}

	for(auto & elem : terrainImages[8])
	{
		for(IImage * img : elem)
		{
			img->shiftPalette(229, 12);
			img->shiftPalette(242, 14);
		}
	}

	for(auto & elem : riverImages[0])
	{
		for(IImage * img : elem)
		{
			img->shiftPalette(183, 12);
			img->shiftPalette(195, 6);
		}
	}

	for(auto & elem : riverImages[2])
	{
		for(IImage * img : elem)
		{
			img->shiftPalette(228, 12);
			img->shiftPalette(183, 6);
			img->shiftPalette(240, 6);
		}
	}

	for(auto & elem : riverImages[3])
	{
		for(IImage * img : elem)
			img->shiftPalette(240, 9);
	}
}

CMapHandler::~CMapHandler()
{
	delete normalBlitter;
	delete worldViewBlitter;
	delete puzzleViewBlitter;

	for (auto & elem : fadeAnims)
	{
		delete elem.second.second;
	}
}

CMapHandler::CMapHandler()
{
	frameW = frameH = 0;
	normalBlitter = new CMapNormalBlitter(this);
	worldViewBlitter = new CMapWorldViewBlitter(this);
	puzzleViewBlitter = new CMapPuzzleViewBlitter(this);
	fadeAnimCounter = 0;
	map = nullptr;
	tilesW = tilesH = 0;
	offsetX = offsetY = 0;

	egdeAnimation = make_unique<CAnimation>("EDG");
	egdeAnimation->preload();
}

void CMapHandler::getTerrainDescr( const int3 &pos, std::string & out, bool terName )
{
	out.clear();
	TerrainTile2 & tt = ttiles[pos.x][pos.y][pos.z];
	const TerrainTile &t = map->getTile(pos);
	for(auto & elem : tt.objects)
	{
		if(elem.obj && elem.obj->ID == Obj::HOLE) //Hole
		{
			out = elem.obj->getObjectName();
			return;
		}
	}

	if(t.hasFavorableWinds())
		out = CGI->objtypeh->getObjectName(Obj::FAVORABLE_WINDS);
	else if(terName)
	{
		out = CGI->generaltexth->terrainNames[t.terType];
		if(t.getDiggingStatus(false) == EDiggingStatus::CAN_DIG)
		{
			out = boost::str(boost::format("%s %s") % out % CGI->generaltexth->allTexts[330]); /// digging ok
		}
	}
}

void CMapHandler::discardWorldViewCache()
{
	cache.discardWorldViewCache();
}

CMapHandler::CMapCache::CMapCache()
{
	worldViewCachedScale = 0;
}

void CMapHandler::CMapCache::discardWorldViewCache()
{
	for(auto & cache : data)
		cache.clear();
	logAnim->debug("Discarded world view cache");
}

void CMapHandler::CMapCache::updateWorldViewScale(float scale)
{
	if (fabs(scale - worldViewCachedScale) > 0.001f)
		discardWorldViewCache();
	worldViewCachedScale = scale;
}

IImage * CMapHandler::CMapCache::requestWorldViewCacheOrCreate(CMapHandler::EMapCacheType type, const IImage * fullSurface)
{
	intptr_t key = (intptr_t) fullSurface;
	auto & cache = data[(ui8)type];

	auto iter = cache.find(key);
	if(iter == cache.end())
	{
		auto scaled = fullSurface->scaleFast(worldViewCachedScale);
		IImage * ret = scaled.get();
		cache[key] = std::move(scaled);
		return ret;
	}
	else
	{
		return (*iter).second.get();
	}
}

bool CMapHandler::compareObjectBlitOrder(const CGObjectInstance * a, const CGObjectInstance * b)
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

TerrainTileObject::TerrainTileObject(const CGObjectInstance * obj_, SDL_Rect rect_)
	: obj(obj_),
	  rect(rect_),
	  fadeAnimKey(-1)
{
}

TerrainTileObject::~TerrainTileObject()
{
}

