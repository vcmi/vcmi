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
#include "gui/SDL_Extensions.h"
#include "CGameInfo.h"
#include "../lib/mapObjects/CGHeroInstance.h"
#include "../lib/mapObjects/CObjectClassesHandler.h"
#include "../lib/CGameState.h"
#include "../lib/CHeroHandler.h"
#include "../lib/CTownHandler.h"
#include "Graphics.h"
#include "../lib/mapping/CMap.h"
#include "CDefHandler.h"
#include "../lib/CConfigHandler.h"
#include "../lib/CGeneralTextHandler.h"
#include "../lib/GameConstants.h"
#include "../lib/CStopWatch.h"
#include "CMT.h"
#include "../lib/CRandomGenerator.h"

#define ADVOPT (conf.go()->ac)

std::string nameFromType (int typ)
{
    switch(ETerrainType(typ))
	{
        case ETerrainType::DIRT:
			return std::string("DIRTTL.DEF");

        case ETerrainType::SAND:
			return std::string("SANDTL.DEF");

        case ETerrainType::GRASS:
			return std::string("GRASTL.DEF");

        case ETerrainType::SNOW:
			return std::string("SNOWTL.DEF");

        case ETerrainType::SWAMP:
			return std::string("SWMPTL.DEF");

        case ETerrainType::ROUGH:
			return std::string("ROUGTL.DEF");

        case ETerrainType::SUBTERRANEAN:
			return std::string("SUBBTL.DEF");

        case ETerrainType::LAVA:
			return std::string("LAVATL.DEF");

        case ETerrainType::WATER:
			return std::string("WATRTL.DEF");

        case ETerrainType::ROCK:
			return std::string("ROCKTL.DEF");

        case ETerrainType::BORDER:
		//TODO use me
		break;
		default:
		//TODO do something here
		break;
	}
	return std::string();
}

static bool objectBlitOrderSorter(const std::pair<const CGObjectInstance*,SDL_Rect>  & a, const std::pair<const CGObjectInstance*,SDL_Rect> & b)
{
	return CMapHandler::compareObjectBlitOrder(a.first, b.first);
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
			return visibilityMap[dx+pos.x][dy+pos.y][pos.z];
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
	graphics->FoWfullHide = CDefHandler::giveDef("TSHRC.DEF");
	graphics->FoWpartialHide = CDefHandler::giveDef("TSHRE.DEF");

	//adding necessary rotations
	static const int missRot [] = {22, 15, 2, 13, 12, 16, 28, 17, 20, 19, 7, 24, 26, 25, 30, 32, 27};

	Cimage nw;
	for(auto & elem : missRot)
	{
		nw = graphics->FoWpartialHide->ourImages[elem];
		nw.bitmap = CSDL_Ext::verticalFlip(nw.bitmap);
		graphics->FoWpartialHide->ourImages.push_back(nw);
	}
	//necessaary rotations added

	//alpha - transformation
	for(auto & elem : graphics->FoWpartialHide->ourImages)
	{
		CSDL_Ext::alphaTransform(elem.bitmap);
	}

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
				elem[j][k] = CRandomGenerator::getDefault().nextInt(graphics->FoWfullHide->ourImages.size() - 1);
			}
		}
	}
}

void CMapHandler::roadsRiverTerrainInit()
{
	//initializing road's and river's DefHandlers

	roadDefs.push_back(CDefHandler::giveDefEss("dirtrd.def"));
	roadDefs.push_back(CDefHandler::giveDefEss("gravrd.def"));
	roadDefs.push_back(CDefHandler::giveDefEss("cobbrd.def"));
	staticRiverDefs.push_back(CDefHandler::giveDefEss("clrrvr.def"));
	staticRiverDefs.push_back(CDefHandler::giveDefEss("icyrvr.def"));
	staticRiverDefs.push_back(CDefHandler::giveDefEss("mudrvr.def"));
	staticRiverDefs.push_back(CDefHandler::giveDefEss("lavrvr.def"));
	for(auto & elem : staticRiverDefs)
	{
		for(size_t h=0; h < elem->ourImages.size(); ++h)
		{
			CSDL_Ext::alphaTransform(elem->ourImages[h].bitmap);
		}
	}
	for(auto & elem : roadDefs)
	{
		for(size_t h=0; h < elem->ourImages.size(); ++h)
		{
			CSDL_Ext::alphaTransform(elem->ourImages[h].bitmap);
		}
	}

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

void CMapHandler::borderAndTerrainBitmapInit()
{
	CDefHandler * bord = CDefHandler::giveDef("EDG.DEF");
	bord->notFreeImgs =  true;
	terrainGraphics.resize(10);
	for (int i = 0; i < 10 ; i++)
	{
		CDefHandler *hlp = CDefHandler::giveDef(nameFromType(i));
		terrainGraphics[i].resize(hlp->ourImages.size());
		hlp->notFreeImgs = true;
		for(size_t j=0; j < hlp->ourImages.size(); ++j)
			terrainGraphics[i][j] = hlp->ourImages[j].bitmap;
		delete hlp;
	}

	for (int i=0-frameW; i<sizes.x+frameW; i++) //by width
	{
		for (int j=0-frameH; j<sizes.y+frameH;j++) //by height
		{
			for(int k=0; k<sizes.z; ++k) //by levles
			{
				if(i < 0 || i > (sizes.x-1) || j < 0  || j > (sizes.y-1))
				{
					int terBitmapNum = -1;

					auto & rand = CRandomGenerator::getDefault();

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

					if(terBitmapNum != -1)
					{
						ttiles[i][j][k].terbitmap = bord->ourImages[terBitmapNum].bitmap;
						continue;
					}
				}
			}
		}
	}
	delete bord;
}

static void processDef (const ObjectTemplate & objTempl)
{
	if(objTempl.id == Obj::EVENT)
	{
		graphics->advmapobjGraphics[objTempl.animationFile] = nullptr;
		return;
	}
	CDefEssential * ourDef = graphics->getDef(objTempl);
	if(!ourDef) //if object has already set handler (eg. heroes) it should not be overwritten
	{
		if(objTempl.animationFile.size())
		{
			graphics->advmapobjGraphics[objTempl.animationFile] = CDefHandler::giveDefEss(objTempl.animationFile);
		}
		else
		{
			logGlobal->warnStream() << "No def name for " << objTempl.id << "  " << objTempl.subid;
			return;
		}
		ourDef = graphics->getDef(objTempl);

	}
	//alpha transformation
	for(auto & elem : ourDef->ourImages)
	{
		CSDL_Ext::alphaTransform(elem.bitmap);
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
		if (!graphics->getDef(obj)) //try to load it
			processDef(obj->appearance);
		if (!graphics->getDef(obj)) // stil no graphics? exit
			continue;

		const SDL_Surface *bitmap = graphics->getDef(obj)->ourImages[0].bitmap;
		for(int fx=0; fx < obj->getWidth(); ++fx)
		{
			for(int fy=0; fy < obj->getHeight(); ++fy)
			{
				int3 currTile(obj->pos.x - fx, obj->pos.y - fy, obj->pos.z);
				SDL_Rect cr;
				cr.w = 32;
				cr.h = 32;
				cr.x = bitmap->w - fx * 32 - 32;
				cr.y = bitmap->h - fy * 32 - 32;
				std::pair<const CGObjectInstance*,SDL_Rect> toAdd = std::make_pair(obj,cr);
				

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

	graphics->advmapobjGraphics["AB01_.DEF"] = graphics->boatAnims[0];
	graphics->advmapobjGraphics["AB02_.DEF"] = graphics->boatAnims[1];
	graphics->advmapobjGraphics["AB03_.DEF"] = graphics->boatAnims[2];
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
	roadsRiverTerrainInit();	//road's and river's DefHandlers; and simple values initialization
	borderAndTerrainBitmapInit();
    logGlobal->infoStream()<<"\tPreparing FoW, roads, rivers,borders: "<<th.getDiff();
	initObjectRects();
    logGlobal->infoStream()<<"\tMaking object rects: "<<th.getDiff();

}

// Update map window screen
// top_tile top left tile to draw. Not necessarily visible.
// extRect, extRect = map window on screen
// moveX, moveY: when a hero is in movement indicates how to shift the map. Range is -31 to + 31.
void CMapHandler::terrainRect( int3 top_tile, ui8 anim, const std::vector< std::vector< std::vector<ui8> > > * visibilityMap, bool otherHeroAnim, ui8 heroAnim, SDL_Surface * extSurf, const SDL_Rect * extRect, int moveX, int moveY, bool puzzleMode, int3 grailPosRel ) const
{
	// Width and height of the portion of the map to process. Units in tiles.
	ui32 dx = tilesW;
	ui32 dy = tilesH;

	// Basic rectangle for a tile. Should be a const but conflicts with SDL headers
	SDL_Rect rtile = { 0, 0, 32, 32 };

	// Absolute coords of the first pixel in the top left corner
	int srx_init = offsetX + extRect->x;
	int sry_init = offsetY + extRect->y;

	int srx, sry;	// absolute screen coordinates in pixels

	// If moving, we need to add an extra column/line
	if (moveX != 0) 
	{
		dx++;
		srx_init += moveX;
		if (moveX > 0) 
		{
			// Moving right. We still need to draw the old tile on the
			// left, so adjust our referential
			top_tile.x --;
			srx_init -= 32;
		}
	}

	if (moveY != 0) 
	{
		dy++;
		sry_init += moveY;
		if (moveY > 0) 
		{
			// Moving down. We still need to draw the tile on the top,
			// so adjust our referential.
			top_tile.y --;
			sry_init -= 32;
		}
	}

	// Reduce sizes if we go out of the full map.
	if (top_tile.x < -frameW)
		top_tile.x = -frameW;
	if (top_tile.y < -frameH)
		top_tile.y = -frameH;
	if (top_tile.x + dx > sizes.x + frameW)
		dx = sizes.x + frameW - top_tile.x;
	if (top_tile.y + dy > sizes.y + frameH)
		dy = sizes.y + frameH - top_tile.y;
	
	if(!otherHeroAnim)
		heroAnim = anim; //the same, as it should be

	SDL_Rect prevClip;
	SDL_GetClipRect(extSurf, &prevClip);
	SDL_SetClipRect(extSurf, extRect); //preventing blitting outside of that rect

	const BlitterWithRotationVal blitterWithRotation = CSDL_Ext::getBlitterWithRotation(extSurf);
	const BlitterWithRotationVal blitterWithRotationAndAlpha = CSDL_Ext::getBlitterWithRotationAndAlpha(extSurf);
	//const BlitterWithRotationAndAlphaVal blitterWithRotation = CSDL_Ext::getBlitterWithRotation(extSurf);

	// printing terrain
	srx = srx_init;

	for (int bx = 0; bx < dx; bx++, srx+=32)
	{
		// Skip column if not in map
		if (top_tile.x+bx < 0 || top_tile.x+bx >= sizes.x)
			continue;

		sry = sry_init;

		for (int by=0; by < dy; by++, sry+=32)
		{
			int3 pos(top_tile.x+bx, top_tile.y+by, top_tile.z); //blitted tile position

			// Skip tile if not in map
			if (pos.y < 0 || pos.y >= sizes.y)
				continue;
			
			//we should not render fully hidden tiles
			if(!puzzleMode)
			{
				const NeighborTilesInfo info(pos,sizes,*visibilityMap);
				
				if(info.areAllHidden())
					continue;
			}			

			const TerrainTile2 & tile = ttiles[pos.x][pos.y][pos.z];
			const TerrainTile &tinfo = map->getTile(int3(pos.x, pos.y, pos.z));

			SDL_Rect sr;
			sr.x=srx;
			sr.y=sry;
			sr.h=sr.w=32;

			//blit terrain with river/road
			if(tile.terbitmap)
			{ //if custom terrain graphic - use it
				SDL_Rect temp_rect = genRect(sr.h, sr.w, 0, 0);
				CSDL_Ext::blitSurface(tile.terbitmap, &temp_rect, extSurf, &sr);
			}
			else //use default terrain graphic
			{
                blitterWithRotation(terrainGraphics[tinfo.terType][tinfo.terView],rtile, extSurf, sr, tinfo.extTileFlags%4);
			}
            if(tinfo.riverType) //print river if present
			{
                blitterWithRotationAndAlpha(staticRiverDefs[tinfo.riverType-1]->ourImages[tinfo.riverDir].bitmap,rtile, extSurf, sr, (tinfo.extTileFlags>>2)%4);
			}

			//Roads are shifted by 16 pixels to bottom. We have to draw both parts separately
			if (pos.y > 0 && map->getTile(int3(pos.x, pos.y-1, pos.z)).roadType != ERoadType::NO_ROAD)
			{ //part from top tile
				const TerrainTile &topTile = map->getTile(int3(pos.x, pos.y-1, pos.z));
				Rect source(0, 16, 32, 16);
				Rect dest(sr.x, sr.y, sr.w, sr.h/2);
                blitterWithRotationAndAlpha(roadDefs[topTile.roadType - 1]->ourImages[topTile.roadDir].bitmap, source, extSurf, dest, (topTile.extTileFlags>>4)%4);
			}

            if(tinfo.roadType != ERoadType::NO_ROAD) //print road from this tile
			{
				Rect source(0, 0, 32, 32);
				Rect dest(sr.x, sr.y+16, sr.w, sr.h/2);
                blitterWithRotationAndAlpha(roadDefs[tinfo.roadType-1]->ourImages[tinfo.roadDir].bitmap, source, extSurf, dest, (tinfo.extTileFlags>>4)%4);
			}

			//blit objects
			const std::vector < std::pair<const CGObjectInstance*,SDL_Rect> > &objects = tile.objects;
			for(auto & object : objects)
			{
				const CGObjectInstance *obj = object.first;
				if (!graphics->getDef(obj))
					processDef(obj->appearance);
				if (!graphics->getDef(obj) && !obj->appearance.animationFile.empty())
				{
					logGlobal->errorStream() << "Failed to load image " << obj->appearance.animationFile;
				}

				PlayerColor color = obj->tempOwner;

				//checking if object has non-empty graphic on this tile
				if(obj->ID != Obj::HERO && !obj->coveringAt(top_tile.x + bx, top_tile.y + by))
					continue;

				static const int notBlittedInPuzzleMode[] = {Obj::HOLE};

				//don't print flaggable objects in puzzle mode
				if(puzzleMode && (obj->isVisitable() || std::find(notBlittedInPuzzleMode, notBlittedInPuzzleMode+1, obj->ID) != notBlittedInPuzzleMode+1)) //?
					continue;

 				SDL_Rect sr2(sr); 

				SDL_Rect pp = object.second;
				pp.h = sr.h;
				pp.w = sr.w;

				const CGHeroInstance * themp = (obj->ID != Obj::HERO
					? nullptr  
					: static_cast<const CGHeroInstance*>(obj));

				//print hero / boat and flag
				if((themp && themp->moveDir && themp->type) || (obj->ID == Obj::BOAT)) //it's hero or boat
				{
					const int IMGVAL = 8; //frames per group of movement animation
					ui8 dir;
					std::vector<Cimage> * iv = nullptr;
					std::vector<CDefEssential *> Graphics::*flg = nullptr;
					SDL_Surface * tb = nullptr; //surface to blitted

					if(themp) //hero
					{
						if(themp->tempOwner >= PlayerColor::PLAYER_LIMIT) //Neutral hero?
						{
                            logGlobal->errorStream() << "A neutral hero (" << themp->name << ") at " << themp->pos << ". Should not happen!";
							continue;
						}

						dir = themp->moveDir;

						//pick graphics of hero (or boat if hero is sailing)
						if (themp->boat)
							iv = &graphics->boatAnims[themp->boat->subID]->ourImages;
						else
							iv = &graphics->heroAnims[themp->appearance.animationFile]->ourImages;

						//pick appropriate flag set
						if(themp->boat)
						{
							switch (themp->boat->subID)
							{
							case 0: flg = &Graphics::flags1; break;
							case 1: flg = &Graphics::flags2; break;
							case 2: flg = &Graphics::flags3; break;
                            default: logGlobal->errorStream() << "Not supported boat subtype: " << themp->boat->subID;
							}
						}
						else
						{
							flg = &Graphics::flags4;
						}
					}
					else //boat
					{
						const CGBoat *boat = static_cast<const CGBoat*>(obj);
						dir = boat->direction;
						iv = &graphics->boatAnims[boat->subID]->ourImages;
					}


					if(themp && !themp->isStanding) //hero is moving
					{
						size_t gg;
						for(gg=0; gg<iv->size(); ++gg)
						{
							if((*iv)[gg].groupNumber==getHeroFrameNum(dir, true))
							{
								tb = (*iv)[gg+heroAnim%IMGVAL].bitmap;
								break;
							}
						}
						CSDL_Ext::blit8bppAlphaTo24bpp(tb,&pp,extSurf,&sr2);

						//printing flag
						pp.y+=IMGVAL*2-32;
						sr2.y-=16;
						CSDL_Ext::blitSurface((graphics->*flg)[color.getNum()]->ourImages[gg+heroAnim%IMGVAL+35].bitmap, &pp, extSurf, &sr2);
					}
					else //hero / boat stands still
					{
						size_t gg;
						for(gg=0; gg < iv->size(); ++gg)
						{
							if((*iv)[gg].groupNumber==getHeroFrameNum(dir, false))
							{
								tb = (*iv)[gg].bitmap;
								break;
							}
						}
						CSDL_Ext::blit8bppAlphaTo24bpp(tb,&pp,extSurf,&sr2);

						//printing flag
						if(flg  
							&&  obj->pos.x == top_tile.x + bx  
							&&  obj->pos.y == top_tile.y + by)
						{
							SDL_Rect bufr = sr2;
							bufr.x-=2*32;
							bufr.y-=1*32;
							bufr.h = 64;
							bufr.w = 96;
							if(bufr.x-extRect->x>-64)
								CSDL_Ext::blitSurface((graphics->*flg)[color.getNum()]->ourImages[getHeroFrameNum(dir, false) *8+(heroAnim/4)%IMGVAL].bitmap, nullptr, extSurf, &bufr);
						}
					}
				}
				else //blit normal object
				{
					const std::vector<Cimage> &ourImages = graphics->getDef(obj)->ourImages;
					SDL_Surface *bitmap = ourImages[(anim+getPhaseShift(obj))%ourImages.size()].bitmap;

					//setting appropriate flag color
					if(color < PlayerColor::PLAYER_LIMIT || color==PlayerColor::NEUTRAL)
						CSDL_Ext::setPlayerColor(bitmap, color);

					CSDL_Ext::blit8bppAlphaTo24bpp(bitmap,&pp,extSurf,&sr2);
				}
			}
			//objects blitted

			//X sign
			if(puzzleMode)
			{
				if(bx == grailPosRel.x && by == grailPosRel.y)
				{
					CSDL_Ext::blit8bppAlphaTo24bpp(graphics->heroMoveArrows->ourImages[0].bitmap, nullptr, extSurf, &sr);
				}
			}
		}
	}
	// terrain printed


	// printing borders
	srx = srx_init;

	for (int bx = 0; bx < dx; bx++, srx+=32)
	{
		sry = sry_init;

		for (int by = 0; by<dy; by++, sry+=32)
		{
			int3 pos(top_tile.x+bx, top_tile.y+by, top_tile.z); //blitted tile position

			SDL_Rect sr;
			sr.x=srx;
			sr.y=sry;
			sr.h=sr.w=32;

			if (pos.x < 0 || pos.x >= sizes.x ||
				pos.y < 0 || pos.y >= sizes.y)
			{
				// outside of the map - print borders
				SDL_Rect temp_rect = genRect(sr.h, sr.w, 0, 0);
				SDL_Surface * src = ttiles[pos.x][pos.y][top_tile.z].terbitmap;
				assert(src);

				CSDL_Ext::blitSurface(src, &temp_rect,extSurf,&sr);
			}
			else 
			{
				//blitting Fog of War
				if (!puzzleMode)
				{
					if (pos.x >= 0 &&
						pos.y >= 0 &&
						pos.x < sizes.x &&
						pos.y < sizes.y &&
						!(*visibilityMap)[pos.x][pos.y][top_tile.z])
					{
						std::pair<SDL_Surface *, bool> hide = getVisBitmap(pos, *visibilityMap);
						if(hide.second)
							CSDL_Ext::blit8bppAlphaTo24bpp(hide.first, &rtile, extSurf, &sr);
						else
							CSDL_Ext::blitSurface(hide.first, &rtile, extSurf, &sr);
					}
				}
				
				//FoW blitted

				SDL_Rect tileRect = genRect(sr.h, sr.w, 0, 0);

				if (settings["session"]["showBlock"].Bool())
				{
					if(map->getTile(int3(pos.x, pos.y, top_tile.z)).blocked) //temporary hiding blocked positions
					{
						static SDL_Surface * block = nullptr;
						if (!block)
							block = BitmapHandler::loadBitmap("blocked");

						SDL_Rect sr;

						sr.x=srx;
						sr.y=sry;
						sr.h=sr.w=32;

						CSDL_Ext::blitSurface(block, &tileRect, extSurf, &sr);
					}
				}
				if (settings["session"]["showVisit"].Bool())
				{
					if(map->getTile(int3(pos.x, pos.y, top_tile.z)).visitable) //temporary hiding visitable positions
					{
						static SDL_Surface * visit = nullptr;
						if (!visit)
							visit = BitmapHandler::loadBitmap("visitable");

						SDL_Rect sr;

						sr.x=srx;
						sr.y=sry;
						sr.h=sr.w=32;
						CSDL_Ext::blitSurface(visit, &tileRect, extSurf, &sr);
					}
				}
			}
		}
	}
	// borders printed

	// print grid
	if (settings["session"]["showGrid"].Bool())
	{
		srx = srx_init;

		for (int bx = 0; bx < dx; bx++, srx+=32)
		{
			sry = sry_init;

			for (int by = 0; by<dy; by++, sry+=32)
			{
				SDL_Rect sr;

				sr.x=srx;
				sr.y=sry;
				sr.h=sr.w=32;

				const int3 color(0x555555, 0x555555, 0x555555);

				if (sr.y >= extRect->y &&
					sr.y < extRect->y+extRect->h)
					for(int i=0;i<sr.w;i++)
						if (sr.x+i >= extRect->x &&
							sr.x+i < extRect->x+extRect->w)
							CSDL_Ext::SDL_PutPixelWithoutRefresh(extSurf,sr.x+i,sr.y,color.x,color.y,color.z);

				if (sr.x >= extRect->x &&
					sr.x < extRect->x+extRect->w)
					for(int i=0; i<sr.h;i++)
						if (sr.y+i >= extRect->y &&
							sr.y+i < extRect->y+extRect->h)
							CSDL_Ext::SDL_PutPixelWithoutRefresh(extSurf,sr.x,sr.y+i,color.x,color.y,color.z);
			}
		}
	}
	// grid

	//applying sepia / gray effect
	if(puzzleMode)
	{
		CSDL_Ext::applyEffect(extSurf, extRect, static_cast<int>(!ADVOPT.puzzleSepia));
	}
	//sepia / gray effect applied

	SDL_SetClipRect(extSurf, &prevClip); //restoring clip_rect
}

std::pair<SDL_Surface *, bool> CMapHandler::getVisBitmap( const int3 & pos, const std::vector< std::vector< std::vector<ui8> > > & visibilityMap ) const
{
	const NeighborTilesInfo info(pos,sizes,visibilityMap);

	int retBitmapID = info.getBitmapID();// >=0 -> partial hide, <0 - full hide
	if (retBitmapID < 0)
	{
		retBitmapID = - hideBitmap[pos.x][pos.y][pos.z] - 1; //fully hidden
	}
	
	if (retBitmapID >= 0)
	{
		return std::make_pair(graphics->FoWpartialHide->ourImages[retBitmapID].bitmap, true);
	}
	else
	{
		return std::make_pair(graphics->FoWfullHide->ourImages[-retBitmapID - 1].bitmap, false);
	}
}

bool CMapHandler::printObject(const CGObjectInstance *obj)
{
    if (!graphics->getDef(obj))
		processDef(obj->appearance);

	const SDL_Surface *bitmap = graphics->getDef(obj)->ourImages[0].bitmap; 
	const int tilesW = bitmap->w/32;
	const int tilesH = bitmap->h/32;

	for(int fx=0; fx<tilesW; ++fx)
	{
		for(int fy=0; fy<tilesH; ++fy)
		{
			SDL_Rect cr;
			cr.w = 32;
			cr.h = 32;
			cr.x = fx*32;
			cr.y = fy*32;
			std::pair<const CGObjectInstance*,SDL_Rect> toAdd = std::make_pair(obj, cr);
			if((obj->pos.x + fx - tilesW+1)>=0 && (obj->pos.x + fx - tilesW+1)<ttiles.size()-frameW && (obj->pos.y + fy - tilesH+1)>=0 && (obj->pos.y + fy - tilesH+1)<ttiles[0].size()-frameH)
			{
				TerrainTile2 & curt = ttiles[obj->pos.x + fx - tilesW+1][obj->pos.y + fy - tilesH+1][obj->pos.z];

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

		} // for(int fy=0; fy<tilesH; ++fy)
	} //for(int fx=0; fx<tilesW; ++fx)
	return true;
}

bool CMapHandler::hideObject(const CGObjectInstance *obj)
{
	for (size_t i=0; i<map->width; i++)
	{
		for (size_t j=0; j<map->height; j++)
		{
			for (size_t k=0; k<(map->twoLevel ? 2 : 1); k++)
			{
				for(size_t x=0; x < ttiles[i][j][k].objects.size(); x++)
				{
					if (ttiles[i][j][k].objects[x].first->id == obj->id)
					{
						ttiles[i][j][k].objects.erase(ttiles[i][j][k].objects.begin() + x);
						break;
					}
				}
			}
		}
	}
	return true;
}
bool CMapHandler::removeObject(CGObjectInstance *obj)
{
	hideObject(obj);
	return true;
}

ui8 CMapHandler::getHeroFrameNum(ui8 dir, bool isMoving) const
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

void CMapHandler::validateRectTerr(SDL_Rect * val, const SDL_Rect * ext)
{
	if(ext)
	{
		if(val->x<0)
		{
			val->w += val->x;
			val->x = ext->x;
		}
		else
		{
			val->x += ext->x;
		}
		if(val->y<0)
		{
			val->h += val->y;
			val->y = ext->y;
		}
		else
		{
			val->y += ext->y;
		}

		if(val->x+val->w > ext->x+ext->w)
		{
			val->w = ext->x+ext->w-val->x;
		}
		if(val->y+val->h > ext->y+ext->h)
		{
			val->h = ext->y+ext->h-val->y;
		}

		//for sign problems
		if(val->h > 20000 || val->w > 20000)
		{
			val->h = val->w = 0;
		}
	}
}

ui8 CMapHandler::getDir(const int3 &a, const int3 &b)
{
	if(a.z!=b.z)
		return -1; //error!
	if(a.x==b.x+1 && a.y==b.y+1) //lt
		return 0;

	else if(a.x==b.x && a.y==b.y+1) //t
		return 1;

	else if(a.x==b.x-1 && a.y==b.y+1) //rt
		return 2;

	else if(a.x==b.x-1 && a.y==b.y) //r
		return 3;

	else if(a.x==b.x-1 && a.y==b.y-1) //rb
		return 4;

	else if(a.x==b.x && a.y==b.y-1) //b
		return 5;

	else if(a.x==b.x+1 && a.y==b.y-1) //lb
		return 6;

	else if(a.x==b.x+1 && a.y==b.y) //l
		return 7;

	return -2; //shouldn't happen
}

void shiftColors(SDL_Surface *img, int from, int howMany) //shifts colors in palette
{
	//works with at most 16 colors, if needed more -> increase values
	assert(howMany < 16);
	SDL_Color palette[16];

	for(int i=0; i<howMany; ++i)
	{
		palette[(i+1)%howMany] =img->format->palette->colors[from + i];
	}
	SDL_SetColors(img,palette,from,howMany);
}

void CMapHandler::updateWater() //shift colors in palettes of water tiles
{
	for(auto & elem : terrainGraphics[7])
	{
		shiftColors(elem,246, 9); 
	}
	for(auto & elem : terrainGraphics[8])
	{
		shiftColors(elem,229, 12); 
		shiftColors(elem,242, 14); 
	}
	for(auto & elem : staticRiverDefs[0]->ourImages)
	{
		shiftColors(elem.bitmap,183, 12); 
		shiftColors(elem.bitmap,195, 6); 
	}
	for(auto & elem : staticRiverDefs[2]->ourImages)
	{
		shiftColors(elem.bitmap,228, 12); 
		shiftColors(elem.bitmap,183, 6); 
		shiftColors(elem.bitmap,240, 6); 
	}
	for(auto & elem : staticRiverDefs[3]->ourImages)
	{
		shiftColors(elem.bitmap,240, 9); 
	}
}

CMapHandler::~CMapHandler()
{
	delete graphics->FoWfullHide;
	delete graphics->FoWpartialHide;

	for(auto & elem : roadDefs)
		delete elem;

	for(auto & elem : staticRiverDefs)
		delete elem;

	for(auto & elem : terrainGraphics)
	{
		for(int j=0; j < elem.size(); ++j)
			SDL_FreeSurface(elem[j]);
	}
	terrainGraphics.clear();
}

CMapHandler::CMapHandler()
{
	frameW = frameH = 0;
	graphics->FoWfullHide = nullptr;
	graphics->FoWpartialHide = nullptr;
}

void CMapHandler::getTerrainDescr( const int3 &pos, std::string & out, bool terName )
{
	out.clear();
	TerrainTile2 & tt = ttiles[pos.x][pos.y][pos.z];
	const TerrainTile &t = map->getTile(pos);
	for(auto & elem : tt.objects)
	{
		if(elem.first->ID == Obj::HOLE) //Hole
		{
			out = elem.first->getObjectName();
			return;
		}
	}

	if(t.hasFavourableWinds())
		out = CGI->objtypeh->getObjectName(Obj::FAVORABLE_WINDS);
	else if(terName)
        out = CGI->generaltexth->terrainNames[t.terType];
}

ui8 CMapHandler::getPhaseShift(const CGObjectInstance *object) const
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

TerrainTile2::TerrainTile2()
 :terbitmap(nullptr)
{}

bool CMapHandler::compareObjectBlitOrder(const CGObjectInstance * a, const CGObjectInstance * b)
{
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
