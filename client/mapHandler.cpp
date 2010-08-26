#include "../stdafx.h"
#include "mapHandler.h"
#include "SDL_Extensions.h"
#include "CGameInfo.h"
#include <cstdlib>
#include "../hch/CLodHandler.h"
#include "../hch/CDefObjInfoHandler.h"
#include <algorithm>
#include "../lib/CGameState.h"
#include "../hch/CHeroHandler.h"
#include "../hch/CTownHandler.h"
#include "Graphics.h"
#include <iomanip>
#include <sstream>
#include "../hch/CObjectHandler.h"
#include "../lib/map.h"
#include "../hch/CDefHandler.h"
#include "CConfigHandler.h"
#include <boost/assign/list_of.hpp>
#include "../hch/CGeneralTextHandler.h"

/*
 * mapHandler.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

extern SDL_Surface * screen;
#define ADVOPT (conf.go()->ac)

std::string nameFromType (int typ)
{
	switch(static_cast<TerrainTile::EterrainType>(typ))
	{
		case TerrainTile::dirt:
			return std::string("DIRTTL.DEF");

		case TerrainTile::sand:
			return std::string("SANDTL.DEF");

		case TerrainTile::grass:
			return std::string("GRASTL.DEF");

		case TerrainTile::snow:
			return std::string("SNOWTL.DEF");

		case TerrainTile::swamp:
			return std::string("SWMPTL.DEF");

		case TerrainTile::rough:
			return std::string("ROUGTL.DEF");

		case TerrainTile::subterranean:
			return std::string("SUBBTL.DEF");

		case TerrainTile::lava:
			return std::string("LAVATL.DEF");

		case TerrainTile::water:
			return std::string("WATRTL.DEF");

		case TerrainTile::rock:
			return std::string("ROCKTL.DEF");

		case TerrainTile::border:
		//TODO use me
		break;
		default:
		//TODO do something here
		break;
	}
	return std::string();
}

struct OCM_HLP
{
	bool operator ()(const std::pair<const CGObjectInstance*, SDL_Rect> & a, const std::pair<const CGObjectInstance*, SDL_Rect> & b)
	{
		return (*a.first)<(*b.first);
	}
} ocmptwo ;

// void alphaTransformDef(CGDefInfo * defInfo)
// {	
// 	for(int yy=0; yy<defInfo->handler->ourImages.size(); ++yy)
// 	{
// 		CSDL_Ext::alphaTransform(defInfo->handler->ourImages[yy].bitmap);
// 	}
// }

void CMapHandler::prepareFOWDefs()
{
	graphics->FoWfullHide = CDefHandler::giveDef("TSHRC.DEF");
	graphics->FoWpartialHide = CDefHandler::giveDef("TSHRE.DEF");

	//adding necessary rotations
	static const int missRot [] = {22, 15, 2, 13, 12, 16, 18, 17, 20, 19, 7, 24, 26, 25, 30, 32, 27, 28};

	Cimage nw;
	for(int g=0; g<ARRAY_COUNT(missRot); ++g)
	{
		nw = graphics->FoWpartialHide->ourImages[missRot[g]];
		nw.bitmap = CSDL_Ext::rotate01(nw.bitmap);
		graphics->FoWpartialHide->ourImages.push_back(nw);
	}
	//necessaary rotations added

	//alpha - transformation
	for(size_t i=0; i<graphics->FoWpartialHide->ourImages.size(); ++i)
	{
		CSDL_Ext::alphaTransform(graphics->FoWpartialHide->ourImages[i].bitmap);
	}

	//initialization of type of full-hide image
	hideBitmap.resize(sizes.x);
	for (size_t i=0;i<hideBitmap.size();i++)
	{
		hideBitmap[i].resize(sizes.y);
	}
	for (size_t i=0; i<hideBitmap.size(); ++i)
	{
		for (int j=0; j < sizes.y; ++j)
		{
			hideBitmap[i][j].resize(sizes.z);
			for(int k=0; k<sizes.z; ++k)
			{
				hideBitmap[i][j][k] = rand()%graphics->FoWfullHide->ourImages.size();
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
	for(size_t g=0; g<staticRiverDefs.size(); ++g)
	{
		for(size_t h=0; h < staticRiverDefs[g]->ourImages.size(); ++h)
		{
			CSDL_Ext::alphaTransform(staticRiverDefs[g]->ourImages[h].bitmap);
		}
	}
	for(size_t g=0; g<roadDefs.size(); ++g)
	{
		for(size_t h=0; h < roadDefs[g]->ourImages.size(); ++h)
		{
			CSDL_Ext::alphaTransform(roadDefs[g]->ourImages[h].bitmap);
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

	// prepare the map
	for (int i=0; i<sizes.x; i++) //by width
	{
		for (int j=0; j<sizes.y;j++) //by height
		{
			for (int k=0; k<sizes.z; ++k) //by levels
			{
				TerrainTile2 &pom(ttiles[i][j][k]);
			}
		}
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

					if(i==-1 && j==-1)
						terBitmapNum = 16;
					else if(i==-1 && j==(sizes.y))
						terBitmapNum = 19;
					else if(i==(sizes.x) && j==-1)
						terBitmapNum = 17;
					else if(i==(sizes.x) && j==(sizes.y))
						terBitmapNum = 18;
					else if(j == -1 && i > -1 && i < sizes.y)
						terBitmapNum = 22+rand()%2;
					else if(i == -1 && j > -1 && j < sizes.y)
						terBitmapNum = 33+rand()%2;
					else if(j == sizes.y && i >-1 && i < sizes.x)
						terBitmapNum = 29+rand()%2;
					else if(i == sizes.x && j > -1 && j < sizes.y)
						terBitmapNum = 25+rand()%2;
					else
						terBitmapNum = rand()%16;

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
void CMapHandler::initObjectRects()
{
	//initializing objects / rects
	for(size_t f=0; f < map->objects.size(); ++f)
	{
		const CGObjectInstance *obj = map->objects[f];
		if(	!obj
			|| obj->ID==HEROI_TYPE && static_cast<const CGHeroInstance*>(obj)->inTownGarrison //garrisoned hero
			|| obj->ID==8 && static_cast<const CGBoat*>(obj)->hero //boat wih hero (hero graphics is used)
			|| !obj->defInfo
			|| !obj->defInfo->handler) //no graphic...
		{
			continue;
		}

		const SDL_Surface *bitmap = obj->defInfo->handler->ourImages[0].bitmap;
		for(int fx=0; fx<bitmap->w>>5; ++fx) //bitmap->w/32
		{
			for(int fy=0; fy<bitmap->h>>5; ++fy) //bitmap->h/32
			{
				SDL_Rect cr;
				cr.w = 32;
				cr.h = 32;
				cr.x = fx<<5; //fx*32
				cr.y = fy<<5; //fy*32
				std::pair<const CGObjectInstance*,SDL_Rect> toAdd = std::make_pair(obj,cr);
				
				if(    (obj->pos.x + fx - bitmap->w/32+1)  >=  0 
					&& (obj->pos.x + fx - bitmap->w/32+1)  <  ttiles.size() - frameW 
					&& (obj->pos.y + fy - bitmap->h/32+1)  >=  0 
					&& (obj->pos.y + fy - bitmap->h/32+1)  <  ttiles[0].size() - frameH
				  )
				{
					//TerrainTile2 & curt =
					//	ttiles
					//	[obj->pos.x + fx - bitmap->w/32]
					//[obj->pos.y + fy - bitmap->h/32]
					//[obj->pos.z];
					ttiles[obj->pos.x + fx - bitmap->w/32+1][obj->pos.y + fy - bitmap->h/32+1][obj->pos.z].objects.push_back(toAdd);
				}
			} // for(int fy=0; fy<bitmap->h/32; ++fy)
		} //for(int fx=0; fx<bitmap->w/32; ++fx)
	} // for(int f=0; f<map->objects.size(); ++f)

	for(int ix=0; ix<ttiles.size()-frameW; ++ix)
	{
		for(int iy=0; iy<ttiles[0].size()-frameH; ++iy)
		{
			for(int iz=0; iz<ttiles[0][0].size(); ++iz)
			{
				stable_sort(ttiles[ix][iy][iz].objects.begin(), ttiles[ix][iy][iz].objects.end(), ocmptwo);
			}
		}
	}
}
static void processDef (CGDefInfo* def)
{
	if(def->id == EVENTI_TYPE)
		return;

	if(!def->handler) //if object has already set handler (eg. heroes) it should not be overwritten
	{
		if(def->name.size())
		{
			if(vstd::contains(graphics->mapObjectDefs, def->name))
			{
				def->handler = graphics->mapObjectDefs[def->name];
			}
			else
			{
				graphics->mapObjectDefs[def->name] = def->handler = CDefHandler::giveDefEss(def->name);
			}
		}
		else
		{
			tlog2 << "No def name for " << def->id << "  " << def->subid << std::endl;
			def->handler = NULL;
			return;
		}

// 		def->width = def->handler->ourImages[0].bitmap->w/32;
// 		def->height = def->handler->ourImages[0].bitmap->h/32;
	}

	CGDefInfo* pom = CGI->dobjinfo->gobjs[def->id][def->subid];
	if(pom && def->id!=TOWNI_TYPE)
	{
		pom->handler = def->handler;
		pom->width = pom->handler->ourImages[0].bitmap->w/32;
		pom->height = pom->handler->ourImages[0].bitmap->h/32;
	}
	else if(def->id != HEROI_TYPE && def->id != TOWNI_TYPE)
		tlog3 << "\t\tMinor warning: lacking def info for " << def->id << " " << def->subid <<" " << def->name << std::endl;

	//alpha transformation
	for(size_t yy=0; yy < def->handler->ourImages.size(); ++yy)
	{
		CSDL_Ext::alphaTransform(def->handler->ourImages[yy].bitmap);
	}
}
void CMapHandler::initHeroDef(CGHeroInstance * h)
{
	h->defInfo->handler = graphics->flags1[0];
	h->defInfo->width = h->defInfo->handler->ourImages[0].bitmap->w/32;
	h->defInfo->height = h->defInfo->handler->ourImages[0].bitmap->h/32;
}
void CMapHandler::init()
{
	timeHandler th;
	th.getDif();

	CGI->dobjinfo->gobjs[8][0]->handler = graphics->boatAnims[0];
	CGI->dobjinfo->gobjs[8][1]->handler = graphics->boatAnims[1];
	CGI->dobjinfo->gobjs[8][2]->handler = graphics->boatAnims[2];

	// Size of visible terrain.
	int mapW = conf.go()->ac.advmapW;
	int mapH = conf.go()->ac.advmapH;

	//sizes of terrain
	sizes.x = map->width;
	sizes.y = map->height;
	sizes.z = map->twoLevel+1;

	// Total number of visible tiles. Substract the center tile, then
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

	for(int i=0;i<map->heroes.size();i++)
	{
		if(!map->heroes[i]->defInfo->handler)
		{
			initHeroDef(map->heroes[i]);
		}
	}

	std::for_each(map->defy.begin(),map->defy.end(),processDef); //load h3m defs
	tlog0<<"\tUnpacking and handling defs: "<<th.getDif()<<std::endl;

	//it seems to be completely unnecessary and useless
// 	for(int i=0;i<PLAYER_LIMIT;i++)
// 	{
// 		for(size_t j=0; j < map->players[i].heroesNames.size(); ++j)
// 		{
// 			usedHeroes.insert(map->players[i].heroesNames[j].heroID);
// 		}
// 	}
// 	tlog0<<"\tChecking used heroes: "<<th.getDif()<<std::endl;



	prepareFOWDefs();
	roadsRiverTerrainInit();	//road's and river's DefHandlers; and simple values initialization
	borderAndTerrainBitmapInit();
	tlog0<<"\tPreparing FoW, roads, rivers,borders: "<<th.getDif()<<std::endl;
	initObjectRects();
	tlog0<<"\tMaking object rects: "<<th.getDif()<<std::endl;

}

// Update map window screen
// top_tile top left tile to draw. Not necessarily visible.
// extRect, extRect = map window on screen
// moveX, moveY: when a hero is in movement indicates how to shift the map. Range is -31 to + 31.
void CMapHandler::terrainRect(int3 top_tile, unsigned char anim, const std::vector< std::vector< std::vector<unsigned char> > > * visibilityMap, bool otherHeroAnim, unsigned char heroAnim, SDL_Surface * extSurf, const SDL_Rect * extRect, int moveX, int moveY, bool puzzleMode)
{
	// Width and height of the portion of the map to process. Units in tiles.
	unsigned int dx = tilesW;
	unsigned int dy = tilesH;

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

			const TerrainTile2 & tile = ttiles[pos.x][pos.y][pos.z];
			const TerrainTile &tinfo = map->terrain[pos.x][pos.y][pos.z];

			SDL_Rect sr;
			sr.x=srx;
			sr.y=sry;
			sr.h=sr.w=32;

			//blit terrain with river/road
			if(tile.terbitmap) //if custom terrain graphic - use it
				CSDL_Ext::blitSurface(tile.terbitmap, &genRect(sr.h, sr.w, 0, 0), extSurf, &sr);
			else //use default terrain graphic
				blitterWithRotation(terrainGraphics[tinfo.tertype][tinfo.terview],rtile, extSurf, sr, tinfo.siodmyTajemniczyBajt%4);
			if(tinfo.nuine) //print river if present
				blitterWithRotationAndAlpha(staticRiverDefs[tinfo.nuine-1]->ourImages[tinfo.rivDir].bitmap,rtile, extSurf, sr, (tinfo.siodmyTajemniczyBajt>>2)%4);
			if(tinfo.malle) //print road if present
				blitterWithRotationAndAlpha(roadDefs[tinfo.malle-1]->ourImages[tinfo.roadDir].bitmap,rtile, extSurf, sr, (tinfo.siodmyTajemniczyBajt>>4)%4);

			//blit objects
			const std::vector < std::pair<const CGObjectInstance*,SDL_Rect> > &objects = tile.objects;
			for(int h=0; h < objects.size(); ++h)
			{
				const CGObjectInstance *obj = objects[h].first;
				ui8 color = obj->tempOwner;

				//checking if object has non-empty graphic on this tile
				if(obj->ID != HEROI_TYPE && !obj->coveringAt(obj->pos.x - (top_tile.x + bx), top_tile.y + by - obj->pos.y + 5))
					continue;

				//don't print flaggable objects in puzzle mode
				if(puzzleMode && obj->tempOwner != 254)
					continue;

 				SDL_Rect sr2(sr); 

				SDL_Rect pp = objects[h].second;
				pp.h = sr.h;
				pp.w = sr.w;

				const CGHeroInstance * themp = (obj->ID != HEROI_TYPE  
					? NULL  
					: static_cast<const CGHeroInstance*>(obj));

				//print hero / boat and flag
				if(themp && themp->moveDir && themp->type  ||  obj->ID == 8) //it's hero or boat
				{
					const int IMGVAL = 8; //frames per group of movement animation
					ui8 dir;
					std::vector<Cimage> * iv = NULL;
					std::vector<CDefEssential *> Graphics::*flg = NULL;
					SDL_Surface * tb; //surface to blitted

					if(themp) //hero
					{
						dir = themp->moveDir;

						//pick graphics of hero (or boat if hero is sailing)
						iv = (themp->boat) 
							? &graphics->boatAnims[themp->boat->subID]->ourImages
							: &graphics->heroAnims[themp->type->heroType]->ourImages;

						//pick appropriate flag set
						if(themp->boat)
						{
							switch (themp->boat->subID)
							{
							case 0: flg = &Graphics::flags1; break;
							case 1: flg = &Graphics::flags2; break;
							case 2: flg = &Graphics::flags3; break;
							default: tlog1 << "Not supported boat subtype: " << themp->boat->subID << std::endl;
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
						CSDL_Ext::blitSurface((graphics->*flg)[color]->ourImages[gg+heroAnim%IMGVAL+35].bitmap, &pp, extSurf, &sr2);
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
								CSDL_Ext::blitSurface((graphics->*flg)[color]->ourImages[getHeroFrameNum(dir, false) *8+(heroAnim/4)%IMGVAL].bitmap, NULL, extSurf, &bufr);
						}
					}
				}
				else //blit normal object
				{
					const std::vector<Cimage> &ourImages = obj->defInfo->handler->ourImages;
					SDL_Surface *bitmap = ourImages[(anim+obj->animPhaseShift)%ourImages.size()].bitmap;

					//setting appropriate flag color
					if(color < 8 || color==255)
						CSDL_Ext::setPlayerColor(bitmap, color);

					if( obj->hasShadowAt(obj->pos.x - (top_tile.x + bx), top_tile.y + by - obj->pos.y + 5) )
						CSDL_Ext::blit8bppAlphaTo24bpp(bitmap,&pp,extSurf,&sr2);
					else
						CSDL_Ext::blitSurface(bitmap,&pp,extSurf,&sr2);
				}
			}
			//objects blitted
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


				CSDL_Ext::blitSurface(ttiles[pos.x][pos.y][top_tile.z].terbitmap,
								&genRect(sr.h, sr.w, 0, 0),extSurf,&sr);
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

				// TODO: these should be activable by the console
#ifdef MARK_BLOCKED_POSITIONS
				if(map->terrain[pos.x][pos.y][top_tile.z].blocked) //temporary hiding blocked positions
				{
					SDL_Rect sr;

					sr.x=srx;
					sr.y=sry;
					sr.h=sr.w=32;

					memset(rSurf->pixels, 128, rSurf->pitch * rSurf->h);
					CSDL_Ext::blitSurface(rSurf,&genRect(sr.h, sr.w, 0, 0),extSurf,&sr);
				}
#endif
#ifdef MARK_VISITABLE_POSITIONS
				if(map->terrain[pos.x][pos.y][top_tile.z].visitable) //temporary hiding visitable positions
				{
					SDL_Rect sr;

					sr.x=srx;
					sr.y=sry;
					sr.h=sr.w=32;

					memset(rSurf->pixels, 128, rSurf->pitch * rSurf->h);
					CSDL_Ext::blitSurface(rSurf,&genRect(sr.h, sr.w, 0, 0),extSurf,&sr);
				}
#endif
			}
		}
	}
	// borders printed

#ifdef MARK_GRID_POSITIONS
	// print grid
	// TODO: This option should be activated by the console.
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

	// grid	
#endif

	//applying sepia / gray effect
	if(puzzleMode)
	{
		CSDL_Ext::applyEffect(extSurf, extRect, static_cast<int>(!ADVOPT.puzzleSepia));
	}
	//sepia / gray effect applied

	SDL_SetClipRect(extSurf, &prevClip); //restoring clip_rect
}

std::pair<SDL_Surface *, bool> CMapHandler::getVisBitmap( const int3 & pos, const std::vector< std::vector< std::vector<unsigned char> > > & visibilityMap )
{
	static const int visBitmaps[256] = {-1, 34, -1, 4, 22, 22, 4, 4, 36, 36, 38, 38, 47, 47, 38, 38, 3, 25, 12, 12, 3, 25, 12, 12,
		9, 9, 6, 6, 9, 9, 6, 6, 35, 34, 4, 4, 22, 22, 4, 4, 36, 36, 38, 38, 47, 47, 38, 38, 26, 49, 28, 28, 26, 49, 28,
		28, 9, 9, 6, 6, 9, 9, 6, 6, -3, 0, -3, 4, 0, 0, 4, 4, 37, 37, 7, 7, 50, 50, 7, 7, 13, 27, 44, 44, 13, 27, 44,
		44, 8,8, 10, 10, 8, 8, 10, 10, 0, 0, 4, 4, 0, 0, 4, 4, 37, 37, 7, 7, 50, 50, 7, 7, 13, 27, 44, 44, 13, 27, 44,
		44, 8, 8, 10, 10, 8, 8, 10, 10, 15, 15, 4, 4, 22, 22, 4, 4, 46, 46, 51, 51, 32, 32, 51, 51, 2, 25, 12, 12, 2,
		25, 12, 12, 9, 9, 6, 6, 9, 9, 6, 6, 15, 15, 4, 4, 22, 22, 4, 4, 46, 46, 51, 51, 32, 32, 51, 51, 26, 49, 28, 28,
		26, 49, 28, 28, 9, 9, 6, 6, 9, 9, 6, 6, 0, 0, 4, 4, 0, 0, 4, 4, 37, 37, 7, 7, 50, 50, 7, 7, 13, 27, 44, 44, 13,
		27, 44, 44, 8, 8, 10, 10, 8, 8, 10, 10, 0, 0, 4, 4, 0, 0, 4, 4, 37, 37, 7, 7, 50, 50, 7, 7, 13, 27, 44, 44, 13,
		27, 44, 44, 8, 8, 10, 10, 8, 8, 10, 10};


							//is tile visible. arrangement: (like num keyboard)
	bool d7 = (pos.x>0 && pos.y>0) ? visibilityMap[pos.x-1][pos.y-1][pos.z] : 0,		//789
		d8 = (pos.y>0) ? visibilityMap[pos.x][pos.y-1][pos.z] : 0,					//456
		d9 = (pos.y>0 && pos.x<sizes.x-1) ? visibilityMap[pos.x+1][pos.y-1][pos.z] : 0,	//123
		d4 = (pos.x>0) ? visibilityMap[pos.x-1][pos.y][pos.z] : 0,
		//d5 = visibilityMap[pos.x][y][pos.z], //TODO use me - OMFG
		d6 = (pos.x<sizes.x-1) ? visibilityMap[pos.x+1][pos.y][pos.z] : 0,
		d1 = (pos.x>0 && pos.y<sizes.y-1) ? visibilityMap[pos.x-1][pos.y+1][pos.z] : 0,
		d2 = (pos.y<sizes.y-1) ? visibilityMap[pos.x][pos.y+1][pos.z] : 0,
		d3 = (pos.x<sizes.x-1 && pos.y<sizes.y-1) ? visibilityMap[pos.x+1][pos.y+1][pos.z] : 0;

	int retBitmapID = visBitmaps[d1 + d2 * 2 + d3 * 4 + d4 * 8 + d6 * 16 + d7 * 32 + d8 * 64 + d9 * 128]; // >=0 -> partial hide, <0 - full hide
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
	if(!obj->defInfo->handler)
		processDef(obj->defInfo);

	const SDL_Surface *bitmap = obj->defInfo->handler->ourImages[0].bitmap;
	int tilesW = bitmap->w/32,
		tilesH = bitmap->h/32;
	for(int fx=0; fx<bitmap->w/32; ++fx)
	{
		for(int fy=0; fy<bitmap->h/32; ++fy)
		{
			SDL_Rect cr;
			cr.w = 32;
			cr.h = 32;
			cr.x = fx*32;
			cr.y = fy*32;
			std::pair<const CGObjectInstance*,SDL_Rect> toAdd = std::make_pair(obj, cr);
			if((obj->pos.x + fx - bitmap->w/32+1)>=0 && (obj->pos.x + fx - bitmap->w/32+1)<ttiles.size()-frameW && (obj->pos.y + fy - bitmap->h/32+1)>=0 && (obj->pos.y + fy - bitmap->h/32+1)<ttiles[0].size()-frameH)
			{
				TerrainTile2 & curt = ttiles[obj->pos.x + fx - bitmap->w/32+1][obj->pos.y + fy - bitmap->h/32+1][obj->pos.z];

				std::vector< std::pair<const CGObjectInstance*,SDL_Rect> >::iterator i = curt.objects.begin();
				for(; i != curt.objects.end(); i++)
				{
					OCM_HLP cmp;
					if(cmp(toAdd, *i))
					{
						curt.objects.insert(i, toAdd);
						i = curt.objects.begin(); //to validate and avoid adding it second time
						break;
					}
				}

				if(i == curt.objects.end())
					curt.objects.insert(i, toAdd);
			}

		} // for(int fy=0; fy<bitmap->h/32; ++fy)
	} //for(int fx=0; fx<bitmap->w/32; ++fx)
	return true;
}

bool CMapHandler::hideObject(const CGObjectInstance *obj)
{
	CDefEssential * curd = obj->defInfo->handler;
	if(!curd) return false;
	const SDL_Surface *bitmap = curd->ourImages[0].bitmap;
	for(int fx=0; fx<bitmap->w/32; ++fx)
	{
		for(int fy=0; fy<bitmap->h/32; ++fy)
		{
			if((obj->pos.x + fx - bitmap->w/32+1)>=0 && (obj->pos.x + fx - bitmap->w/32+1)<ttiles.size()-frameW && (obj->pos.y + fy - bitmap->h/32+1)>=0 && (obj->pos.y + fy - bitmap->h/32+1)<ttiles[0].size()-frameH)
			{
				std::vector < std::pair<const CGObjectInstance*,SDL_Rect> > & ctile = ttiles[obj->pos.x + fx - bitmap->w/32+1][obj->pos.y + fy - bitmap->h/32+1][obj->pos.z].objects;
				for(size_t dd=0; dd < ctile.size(); ++dd)
				{
					if(ctile[dd].first->id==obj->id)
						ctile.erase(ctile.begin() + dd);
				}
			}

		} // for(int fy=0; fy<bitmap->h/32; ++fy)
	} //for(int fx=0; fx<bitmap->w/32; ++fx)
	return true;
}
bool CMapHandler::removeObject(CGObjectInstance *obj)
{
	hideObject(obj);
	return true;
}

unsigned char CMapHandler::getHeroFrameNum(unsigned char dir, bool isMoving) const
{
	if(isMoving)
	{
		static const unsigned char frame [] = {-1, 10, 5, 6, 7, 8, 9, 12, 11};
		return frame[dir];
	}
	else //if(isMoving)
	{
		static const unsigned char frame [] = {-1, 13, 0, 1, 2, 3, 4, 15, 14};
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

unsigned char CMapHandler::getDir(const int3 &a, const int3 &b)
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
	for(size_t j=0; j < terrainGraphics[7].size(); ++j)
	{
		shiftColors(terrainGraphics[7][j],246, 9); 
	}
	for(size_t j=0; j < terrainGraphics[8].size(); ++j)
	{
		shiftColors(terrainGraphics[8][j],229, 12); 
		shiftColors(terrainGraphics[8][j],242, 14); 
	}
	for(size_t j=0; j < staticRiverDefs[0]->ourImages.size(); ++j)
	{
		shiftColors(staticRiverDefs[0]->ourImages[j].bitmap,183, 12); 
		shiftColors(staticRiverDefs[0]->ourImages[j].bitmap,195, 6); 
	}
	for(size_t j=0; j < staticRiverDefs[2]->ourImages.size(); ++j)
	{
		shiftColors(staticRiverDefs[2]->ourImages[j].bitmap,228, 12); 
		shiftColors(staticRiverDefs[2]->ourImages[j].bitmap,183, 6); 
		shiftColors(staticRiverDefs[2]->ourImages[j].bitmap,240, 6); 
	}
	for(size_t j=0; j < staticRiverDefs[3]->ourImages.size(); ++j)
	{
		shiftColors(staticRiverDefs[3]->ourImages[j].bitmap,240, 9); 
	}
}

CMapHandler::~CMapHandler()
{
	delete graphics->FoWfullHide;
	delete graphics->FoWpartialHide;

	for(int i=0; i < roadDefs.size(); i++)
		delete roadDefs[i];

	for(int i=0; i < staticRiverDefs.size(); i++)
		delete staticRiverDefs[i];

	//TODO: why this code makes VCMI crash?
	/*for(int i=0; i < terrainGraphics.size(); ++i)
	{
		for(int j=0; j < terrainGraphics[i].size(); ++j)
			SDL_FreeSurface(terrainGraphics[i][j]);
	}
	terrainGraphics.clear();*/
}

CMapHandler::CMapHandler()
{
	frameW = frameH = 0;
	graphics->FoWfullHide = NULL;
	graphics->FoWpartialHide = NULL;
}

void CMapHandler::getTerrainDescr( const int3 &pos, std::string & out, bool terName )
{
	out.clear();
	TerrainTile2 & tt = ttiles[pos.x][pos.y][pos.z];
	const TerrainTile &t = map->terrain[pos.x][pos.y][pos.z];
	for(std::vector < std::pair<const CGObjectInstance*,SDL_Rect> >::const_iterator i = tt.objects.begin(); i != tt.objects.end(); i++)
	{
		if(i->first->ID == 124) //Hole
		{
			out = i->first->hoverName;
			return;
		}
	}

	if(t.siodmyTajemniczyBajt & 128)
		out = CGI->generaltexth->names[225]; //Favourable Winds
	else if(terName)
		out = CGI->generaltexth->terrainNames[t.tertype];
}

TerrainTile2::TerrainTile2()
 :terbitmap(0)
{}
