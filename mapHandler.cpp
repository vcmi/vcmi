#include "stdafx.h"
#include "mapHandler.h"
#include "client/SDL_Extensions.h"
#include "client/CGameInfo.h"
#include <cstdlib>
#include "hch/CLodHandler.h"
#include "hch/CDefObjInfoHandler.h"
#include <algorithm>
#include "lib/CGameState.h"
#include "hch/CHeroHandler.h"
#include "hch/CTownHandler.h"
#include "client/Graphics.h"
#include <iomanip>
#include <sstream>
#include "hch/CObjectHandler.h"
#include "lib/map.h"
#include "hch/CDefHandler.h"
#include "client/CConfigHandler.h"

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

static std::string nameFromType (int typ)
{
	switch(static_cast<TerrainTile::EterrainType>(typ))
	{
		case TerrainTile::dirt:
		{
			return std::string("DIRTTL.DEF");
			break;
		}
		case TerrainTile::sand:
		{
			return std::string("SANDTL.DEF");
			break;
		}
		case TerrainTile::grass:
		{
			return std::string("GRASTL.DEF");
			break;
		}
		case TerrainTile::snow:
		{
			return std::string("SNOWTL.DEF");
			break;
		}
		case TerrainTile::swamp:
		{
			return std::string("SWMPTL.DEF");			
			break;
		}
		case TerrainTile::rough:
		{
			return std::string("ROUGTL.DEF");		
			break;
		}
		case TerrainTile::subterranean:
		{
			return std::string("SUBBTL.DEF");		
			break;
		}
		case TerrainTile::lava:
		{
			return std::string("LAVATL.DEF");		
			break;
		}
		case TerrainTile::water:
		{
			return std::string("WATRTL.DEF");
			break;
		}
		case TerrainTile::rock:
		{
			return std::string("ROCKTL.DEF");		
			break;
		}
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
static void alphaTransformDef(CGDefInfo * defInfo)
{	
	SDL_Surface * alphaTransSurf = SDL_CreateRGBSurface(SDL_SWSURFACE, 12, 12, 32, 0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000);
	for(int yy=0;yy<defInfo->handler->ourImages.size();yy++)
	{
		CSDL_Ext::alphaTransform(defInfo->handler->ourImages[yy].bitmap);
	}
	SDL_FreeSurface(alphaTransSurf);
}
void CMapHandler::prepareFOWDefs()
{
	fullHide = CDefHandler::giveDef("TSHRC.DEF");
	partialHide = CDefHandler::giveDef("TSHRE.DEF");

	//adding necessary rotations
	Cimage nw = partialHide->ourImages[22]; nw.bitmap = CSDL_Ext::rotate01(nw.bitmap);
	partialHide->ourImages.push_back(nw);
	nw = partialHide->ourImages[15]; nw.bitmap = CSDL_Ext::rotate01(nw.bitmap);
	partialHide->ourImages.push_back(nw);
	nw = partialHide->ourImages[2]; nw.bitmap = CSDL_Ext::rotate01(nw.bitmap);
	partialHide->ourImages.push_back(nw);
	nw = partialHide->ourImages[13]; nw.bitmap = CSDL_Ext::rotate01(nw.bitmap);
	partialHide->ourImages.push_back(nw);
	nw = partialHide->ourImages[12]; nw.bitmap = CSDL_Ext::rotate01(nw.bitmap);
	partialHide->ourImages.push_back(nw);
	nw = partialHide->ourImages[16]; nw.bitmap = CSDL_Ext::rotate01(nw.bitmap);
	partialHide->ourImages.push_back(nw);
	nw = partialHide->ourImages[18]; nw.bitmap = CSDL_Ext::rotate01(nw.bitmap);
	partialHide->ourImages.push_back(nw);
	nw = partialHide->ourImages[17]; nw.bitmap = CSDL_Ext::rotate01(nw.bitmap);
	partialHide->ourImages.push_back(nw);
	nw = partialHide->ourImages[20]; nw.bitmap = CSDL_Ext::rotate01(nw.bitmap);
	partialHide->ourImages.push_back(nw);
	nw = partialHide->ourImages[19]; nw.bitmap = CSDL_Ext::rotate01(nw.bitmap);
	partialHide->ourImages.push_back(nw);
	nw = partialHide->ourImages[7]; nw.bitmap = CSDL_Ext::rotate01(nw.bitmap);
	partialHide->ourImages.push_back(nw);
	nw = partialHide->ourImages[24]; nw.bitmap = CSDL_Ext::rotate01(nw.bitmap);
	partialHide->ourImages.push_back(nw);
	nw = partialHide->ourImages[26]; nw.bitmap = CSDL_Ext::rotate01(nw.bitmap);
	partialHide->ourImages.push_back(nw);
	nw = partialHide->ourImages[25]; nw.bitmap = CSDL_Ext::rotate01(nw.bitmap);
	partialHide->ourImages.push_back(nw);
	nw = partialHide->ourImages[30]; nw.bitmap = CSDL_Ext::rotate01(nw.bitmap);
	partialHide->ourImages.push_back(nw);
	nw = partialHide->ourImages[32]; nw.bitmap = CSDL_Ext::rotate01(nw.bitmap);
	partialHide->ourImages.push_back(nw);
	nw = partialHide->ourImages[27]; nw.bitmap = CSDL_Ext::rotate01(nw.bitmap);
	partialHide->ourImages.push_back(nw);
	nw = partialHide->ourImages[28]; nw.bitmap = CSDL_Ext::rotate01(nw.bitmap);
	partialHide->ourImages.push_back(nw);
	//necessaary rotations added

	for(size_t i=0; i<partialHide->ourImages.size(); ++i)
	{
		CSDL_Ext::alphaTransform(partialHide->ourImages[i].bitmap);
	}

	hideBitmap.resize(CGI->mh->map->width);
	for (size_t i=0;i<hideBitmap.size();i++)
	{
		hideBitmap[i].resize(CGI->mh->map->height);
	}
	for (size_t i=0; i<hideBitmap.size(); ++i)
	{
		for (int j=0; j < CGI->mh->map->height; ++j)
		{
			hideBitmap[i][j].resize(CGI->mh->map->twoLevel+1);
			for(int k=0; k<CGI->mh->map->twoLevel+1; ++k)
			{
				hideBitmap[i][j][k] = rand()%fullHide->ourImages.size();
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

	sizes.x = CGI->mh->map->width;
	sizes.y = CGI->mh->map->height;
	sizes.z = CGI->mh->map->twoLevel+1;

	// Create enough room for the whole map and its frame
	ttiles.resize(CGI->mh->map->width, frameW, frameW);
	for (int i=0-frameW;i<ttiles.size()-frameW;i++)
	{
		ttiles[i].resize(CGI->mh->map->height, frameH, frameH);
	}
	for (int i=0-frameW;i<ttiles.size()-frameW;i++)
	{
		for (int j=0-frameH;j<(int)CGI->mh->map->height+frameH;j++)
			ttiles[i][j].resize(CGI->mh->map->twoLevel+1, 0, 0);
	}

	// Draw the map
	for (int i=0; i<map->width; i++) //jest po szeroko�ci
	{
		for (int j=0; j<map->height;j++) //po wysoko�ci
		{
			for (int k=0; k<=map->twoLevel; ++k)
			{
				TerrainTile2 &pom(ttiles[i][j][k]);
				pom.pos = int3(i, j, k);
				pom.tileInfo = &(map->terrain[i][j][k]);
				if(pom.tileInfo->malle)
				{
					bool rotV, rotH;

					int roadpom = pom.tileInfo->malle-1,
						impom = pom.tileInfo->roadDir;
					SDL_Surface *bitmap = roadDefs[roadpom]->ourImages[impom].bitmap;

					rotH = (pom.tileInfo->siodmyTajemniczyBajt >> 5) & 1;
					rotV = (pom.tileInfo->siodmyTajemniczyBajt >> 4) & 1;

					if(rotH || rotV) 
					{
						if(rotH)
							bitmap = CSDL_Ext::hFlip(bitmap);

						if(rotV)
						{
							SDL_Surface *bitmap2 = CSDL_Ext::rotate01(bitmap);
							if (rotH)
								// bitmap is already a duplicated surface
								SDL_FreeSurface(bitmap);
							bitmap = bitmap2;
						}

						CSDL_Ext::alphaTransform(bitmap);
					}

					ttiles[i][j][k].roadbitmap.push_back(bitmap);
				}
			}
		}
	}

	for (int i=0; i<map->width; i++) //jest po szeroko�ci
	{
		for (int j=0; j<map->height;j++) //po wysoko�ci
		{
			for(int k=0; k<=map->twoLevel; ++k)
			{
				if(map->terrain[i][j][k].nuine)
				{
					bool rotH, rotV;
					
					SDL_Surface *bitmap = staticRiverDefs[map->terrain[i][j][k].nuine-1]->ourImages[map->terrain[i][j][k].rivDir].bitmap;

					rotH = (map->terrain[i][j][k].siodmyTajemniczyBajt >> 3) & 1;
					rotV = (map->terrain[i][j][k].siodmyTajemniczyBajt >> 2) & 1;

					if(rotH || rotV)
					{
						if(rotH)
							bitmap = CSDL_Ext::hFlip(bitmap);

						if(rotV)
						{
							SDL_Surface *bitmap2 = CSDL_Ext::rotate01(bitmap);
							if (rotH)
								// bitmap is already a duplicated surface
								SDL_FreeSurface(bitmap);
							bitmap = bitmap2;
						}

						CSDL_Ext::alphaTransform(bitmap);
					}

					ttiles[i][j][k].rivbitmap.push_back(bitmap);
				}
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

	for (int i=0-frameW; i<map->width+frameW; i++) //jest po szeroko�ci
	{
		for (int j=0-frameH; j<map->height+frameH;j++) //po wysoko�ci
		{
			for(int k=0; k<=map->twoLevel; ++k)
			{
				if(i < 0 || i > (map->width-1) || j < 0  || j > (map->height-1))
				{
					if(i==-1 && j==-1)
					{
						ttiles[i][j][k].terbitmap = bord->ourImages[16].bitmap;
						continue;
					}
					else if(i==-1 && j==(map->height))
					{
						ttiles[i][j][k].terbitmap = bord->ourImages[19].bitmap;
						continue;
					}
					else if(i==(map->width) && j==-1)
					{
						ttiles[i][j][k].terbitmap = bord->ourImages[17].bitmap;
						continue;
					}
					else if(i==(map->width) && j==(map->height))
					{
						ttiles[i][j][k].terbitmap = bord->ourImages[18].bitmap;
						continue;
					}
					else if(j == -1 && i > -1 && i < map->height)
					{
						ttiles[i][j][k].terbitmap = bord->ourImages[22+rand()%2].bitmap;
						continue;
					}
					else if(i == -1 && j > -1 && j < map->height)
					{
						ttiles[i][j][k].terbitmap = bord->ourImages[33+rand()%2].bitmap;
						continue;
					}
					else if(j == map->height && i >-1 && i < map->width)
					{
						ttiles[i][j][k].terbitmap = bord->ourImages[29+rand()%2].bitmap;
						continue;
					}
					else if(i == map->width && j > -1 && j < map->height)
					{
						ttiles[i][j][k].terbitmap = bord->ourImages[25+rand()%2].bitmap;
						continue;
					}
					else
					{
						ttiles[i][j][k].terbitmap = bord->ourImages[rand()%16].bitmap;
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
		if(!map->objects[f]) continue;
		if((map->objects[f]->ID==HEROI_TYPE && static_cast<CGHeroInstance*>(map->objects[f])->inTownGarrison)
			|| !map->objects[f]->defInfo)
		{
			continue;
		}
		CDefEssential * curd = map->objects[f]->defInfo->handler;
		if(curd)
		{
			const SDL_Surface *bitmap = curd->ourImages[0].bitmap;

			for(int fx=0; fx<bitmap->w>>5; ++fx) //bitmap->w/32
			{
				for(int fy=0; fy<bitmap->h>>5; ++fy) //bitmap->h/32
				{
					SDL_Rect cr;
					cr.w = 32;
					cr.h = 32;
					cr.x = fx<<5; //fx*32
					cr.y = fy<<5; //fy*32
					std::pair<CGObjectInstance*,SDL_Rect> toAdd = std::make_pair(map->objects[f],cr);
					
					if(    (map->objects[f]->pos.x + fx - bitmap->w/32+1)  >=  0 
						&& (map->objects[f]->pos.x + fx - bitmap->w/32+1)  <  ttiles.size() - frameW 
						&& (map->objects[f]->pos.y + fy - bitmap->h/32+1)  >=  0 
						&& (map->objects[f]->pos.y + fy - bitmap->h/32+1)  <  ttiles[0].size() - frameH
					  )
					{
						//TerrainTile2 & curt =
						//	ttiles
						//	[map->objects[f]->pos.x + fx - bitmap->w/32]
						//[map->objects[f]->pos.y + fy - bitmap->h/32]
						//[map->objects[f]->pos.z];
						ttiles[map->objects[f]->pos.x + fx - bitmap->w/32+1][map->objects[f]->pos.y + fy - bitmap->h/32+1][map->objects[f]->pos.z].objects.push_back(toAdd);
					}
				} // for(int fy=0; fy<bitmap->h/32; ++fy)
			} //for(int fx=0; fx<bitmap->w/32; ++fx)
		}//if curd
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
	if(def->id == 26)
		return;
	if(!def->handler) //if object has already set handler (eg. heroes) it should not be overwritten
	{
		if(def->name.size())
		{
			def->handler = CDefHandler::giveDefEss(def->name);
		}
		else
		{
			tlog2 << "No def name for " << def->id << "  " << def->subid << std::endl;
			def->handler = NULL;
			return;
		}
		def->width = def->handler->ourImages[0].bitmap->w/32;
		def->height = def->handler->ourImages[0].bitmap->h/32;
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

	// Size of visible terrain.
	mapW = conf.go()->ac.advmapW;
	mapH = conf.go()->ac.advmapH;

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

	std::ifstream ifs("config/townsDefs.txt");
	int ccc;
	ifs>>ccc;
	for(int i=0;i<ccc*2;i++)
	{
		CGDefInfo *n;
		if(i<ccc)
		{
			n = CGI->state->villages[i];
			map->defy.push_back(CGI->state->forts[i]);
		}
		else 
			n = CGI->state->capitols[i%ccc];
		ifs >> n->name;
		if(!n)
			tlog1 << "*HUGE* Warning - missing town def for " << i << std::endl;
		else
			map->defy.push_back(n);
	} 
	tlog0<<"\tLoading town def info: "<<th.getDif()<<std::endl;

	for(int i=0;i<map->heroes.size();i++)
	{
		if(!map->heroes[i]->defInfo->handler)
		{
			initHeroDef(map->heroes[i]);
		}
	}

	std::for_each(map->defy.begin(),map->defy.end(),processDef); //load h3m defs
	tlog0<<"\tUnpacking and handling defs: "<<th.getDif()<<std::endl;

	for(int i=0;i<PLAYER_LIMIT;i++)
	{
		for(size_t j=0; j < map->players[i].heroesNames.size(); ++j)
		{
			usedHeroes.insert(map->players[i].heroesNames[j].heroID);
		}
	}
	tlog0<<"\tChecking used heroes: "<<th.getDif()<<std::endl;



	for(size_t h=0; h<map->defy.size(); ++h) //initializing loaded def handler's info	{
		CGI->mh->loadedDefs.insert(std::make_pair(map->defy[h]->name, map->defy[h]->handler));
	tlog0<<"\tCollecting loaded def's handlers: "<<th.getDif()<<std::endl;

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
void CMapHandler::terrainRect(int3 top_tile, unsigned char anim, std::vector< std::vector< std::vector<unsigned char> > > * visibilityMap, bool otherHeroAnim, unsigned char heroAnim, SDL_Surface * extSurf, const SDL_Rect * extRect, int moveX, int moveY)
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
	if (moveX != 0) {
		dx++;
		srx_init += moveX;
		if (moveX > 0) {
			// Moving right. We still need to draw the old tile on the
			// left, so adjust our referential
			top_tile.x --;
			srx_init -= 32;
		}
	}

	if (moveY != 0) {
		dy++;
		sry_init += moveY;
		if (moveY > 0) {
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
	if (top_tile.x + dx > map->width + frameW)
		dx = map->width + frameW - top_tile.x;
	if (top_tile.y + dy > map->height + frameH)
		dy = map->height + frameH - top_tile.y;
	
	if(!otherHeroAnim)
		heroAnim = anim; //the same, as it should be

	SDL_Rect prevClip;
	SDL_GetClipRect(extSurf, &prevClip);
	SDL_SetClipRect(extSurf, extRect); //preventing blitting outside of that rect

	// Temp surface for rotating tile and then properly blit. The
	// problem is that blitWithRotate1clip & co do not respect the
	// previous SDL_SetClipRect.
	// TODO: optimize by pre-rotating those surfaces
	SDL_Surface *rSurf = CSDL_Ext::newSurface(32, 32, extSurf);

	// printing terrain
	srx = srx_init;

	for (int bx = 0; bx < dx; bx++, srx+=32)
	{
		// Skip column if not in map
		if (top_tile.x+bx < 0 || top_tile.x+bx >= map->width)
			continue;

		sry = sry_init;

		for (int by=0; by < dy; by++, sry+=32)
		{
			// Skip tile if not in map
			if (top_tile.y+by < 0 || top_tile.y+by >= map->height)
				continue;

			const TerrainTile2 & tile = ttiles[top_tile.x+bx][top_tile.y+by][top_tile.z];
			SDL_Rect sr;

			sr.x=srx;
			sr.y=sry;
			sr.h=sr.w=32;

			if(tile.terbitmap)
			{
				SDL_BlitSurface(tile.terbitmap, &genRect(sr.h, sr.w, 0, 0), extSurf, &sr);
			}
			else
			{
				switch(tile.tileInfo->siodmyTajemniczyBajt%4)
				{
				case 0:
					SDL_BlitSurface(terrainGraphics[tile.tileInfo->tertype][tile.tileInfo->terview],
									&rtile, extSurf, &sr);
					break;

				case 1:
					CSDL_Ext::blitWithRotate1clip(terrainGraphics[tile.tileInfo->tertype][tile.tileInfo->terview],
												  &rtile, rSurf, &rtile);
					SDL_BlitSurface(rSurf, &rtile, extSurf, &sr);
					break;

				case 2:
					CSDL_Ext::blitWithRotate2clip(terrainGraphics[tile.tileInfo->tertype][tile.tileInfo->terview],
												  &rtile, rSurf, &rtile);
					SDL_BlitSurface(rSurf, &rtile, extSurf, &sr);
					break;
				default:
					CSDL_Ext::blitWithRotate3clip(terrainGraphics[tile.tileInfo->tertype][tile.tileInfo->terview],
												  &rtile, rSurf, &rtile);
					SDL_BlitSurface(rSurf, &rtile, extSurf, &sr);
					break;
				}
			}
		}
	}
	// terrain printed

	// printing rivers
	srx = srx_init;

	for (int bx = 0; bx<dx; bx++, srx+=32)
	{
		// Skip column if not in map
		if (top_tile.x+bx < 0 || top_tile.x+bx >= map->width)
			continue;

		sry = sry_init;

		for (int by= 0; by<dy; by++, sry+=32)
		{
			// Skip tile if not in map
			if (top_tile.y+by < 0 || top_tile.y+by >= map->height)
				continue;

			SDL_Rect sr;

			sr.x=srx;
			sr.y=sry;
			sr.h=sr.w=32;

			const std::vector<SDL_Surface *> &rivbitmap = ttiles[top_tile.x+bx][top_tile.y+by][top_tile.z].rivbitmap;
			if(rivbitmap.size())
			{
				CSDL_Ext::blit8bppAlphaTo24bpp(rivbitmap[anim%rivbitmap.size()],&genRect(sr.h, sr.w, 0, 0),extSurf,&sr);
			}
		}
	}
	// rivers printed

	// printing roads
	srx = srx_init;

	for (int bx = 0; bx < dx; bx++, srx+=32)
	{
		// Skip column if not in map
		if (top_tile.x+bx < 0 || top_tile.x+bx >= map->width)
			continue;

		sry = sry_init + 16;

		for (int by = 0; by < dy; by++, sry+=32)
		{
			// Skip tile if not in map
			if (top_tile.y+by < 0 || top_tile.y+by >= map->height)
				continue;

			SDL_Rect sr;

			sr.x = srx;
			sr.y = sry;
			sr.h=sr.w=32;

			const std::vector<SDL_Surface *> &roadbitmap = ttiles[top_tile.x+bx][top_tile.y+by][top_tile.z].roadbitmap;
			if(roadbitmap.size())
			{
				CSDL_Ext::blit8bppAlphaTo24bpp(roadbitmap[anim%roadbitmap.size()], &genRect(sr.h, sr.w, 0, (by==-1 && moveY == 0 ? 16 : 0)),extSurf,&sr);
			}
		}
	}
	// roads printed

	// printing objects
	srx = srx_init;

	for (int bx = 0; bx<dx; bx++, srx+=32)
	{
		// Skip column if not in map
		if (top_tile.x+bx < 0 || top_tile.x+bx >= map->width)
			continue;

		sry = sry_init;

		for (int by = 0; by<dy; by++, sry+=32)
		{
			// Skip tile if not in map
			if (top_tile.y+by < 0 || top_tile.y+by >= map->height)
				continue;

			std::vector < std::pair<const CGObjectInstance*,SDL_Rect> > &objects =
				ttiles[top_tile.x+bx][top_tile.y+by][top_tile.z].objects;

			for(int h=0; h < objects.size(); ++h)
			{
				SDL_Rect sr;

				sr.x = srx;
				sr.y = sry;
				sr.w = sr.h = 32;

				SDL_Rect pp = objects[h].second;
				pp.h = sr.h;
				pp.w = sr.w;
				const CGHeroInstance * themp = (dynamic_cast<const CGHeroInstance*>(objects[h].first));

				if(themp && themp->moveDir && !themp->isStanding && themp->ID!=62) //last condition - this is not prison
				{
					int imgVal = 8;
					SDL_Surface * tb;

					if(themp->type==NULL)
						continue;
					std::vector<Cimage> & iv = graphics->heroAnims[themp->type->heroType]->ourImages;

                    size_t gg;
					for(gg=0; gg<iv.size(); ++gg)
					{
						if(iv[gg].groupNumber==getHeroFrameNum(themp->moveDir, !themp->isStanding))
						{
							tb = iv[gg+heroAnim%imgVal].bitmap;
							break;
						}
					}
					CSDL_Ext::blit8bppAlphaTo24bpp(tb,&pp,extSurf,&sr);
					pp.y+=imgVal*2-32;
					sr.y-=16;
					SDL_BlitSurface(graphics->flags4[themp->getOwner()]->ourImages[gg+heroAnim%imgVal+35].bitmap, &pp, extSurf, &sr);
				}
				else if(themp && themp->moveDir && themp->isStanding && themp->ID!=62) //last condition - this is not prison)
				{
					int imgVal = 8;
					SDL_Surface * tb;

					if(themp->type==NULL)
						continue;
					std::vector<Cimage> & iv = graphics->heroAnims[themp->type->heroType]->ourImages;

                    size_t gg;
					for(gg=0; gg < iv.size(); ++gg)
					{
						if(iv[gg].groupNumber==getHeroFrameNum(themp->moveDir, !themp->isStanding))
						{
							tb = iv[gg].bitmap;
							break;
						}
					}
					CSDL_Ext::blit8bppAlphaTo24bpp(tb,&pp,extSurf,&sr);

					if(themp->pos.x==top_tile.x+bx && themp->pos.y==top_tile.y+by)
					{
						SDL_Rect bufr = sr;
						bufr.x-=2*32;
						bufr.y-=1*32;
						bufr.h = 64;
						bufr.w = 96;
						if(bufr.x-extRect->x>-64)
							SDL_BlitSurface(graphics->flags4[themp->getOwner()]->ourImages[ getHeroFrameNum(themp->moveDir, !themp->isStanding) *8+(heroAnim/4)%imgVal].bitmap, NULL, extSurf, &bufr);
					}
				}
				else
				{
					const CGObjectInstance *obj = objects[h].first;
					const std::vector<Cimage> &ourImages = obj->defInfo->handler->ourImages;
					SDL_Surface *bitmap = ourImages[(anim+obj->animPhaseShift)%ourImages.size()].bitmap;

					//setting appropriate flag color
					if(obj->tempOwner<8 || obj->tempOwner==255)
						CSDL_Ext::setPlayerColor(bitmap, obj->tempOwner);
					
					CSDL_Ext::blit8bppAlphaTo24bpp(bitmap,&pp,extSurf,&sr);
				}
			}
		}
	}
	// objects printed

	// printing shadow
	srx = srx_init;

	for (int bx = 0; bx<dx; bx++, srx+=32)
	{
		// Skip column if not in map
		if (top_tile.x+bx < 0 || top_tile.x+bx >= map->width)
			continue;

		sry = sry_init;

		for (int by = 0; by<dy; by++, sry+=32)
		{
			// Skip tile if not in map
			if (top_tile.y+by < 0 || top_tile.y+by >= map->height)
				continue;

			SDL_Rect sr;

			sr.x = srx;
			sr.y = sry;
			sr.h = sr.w = 32;

			if (top_tile.x+bx >= 0 &&
				top_tile.y+by >= 0 &&
				top_tile.x+bx < CGI->mh->map->width &&
				top_tile.y+by < CGI->mh->map->height &&
				!(*visibilityMap)[top_tile.x+bx][top_tile.y+by][top_tile.z])
			{
				SDL_Surface * hide = getVisBitmap(top_tile.x+bx, top_tile.y+by, *visibilityMap, top_tile.z);
				CSDL_Ext::blit8bppAlphaTo24bpp(hide, &rtile, extSurf, &sr);
			}
		}
	}
	// shadow printed

	// printing borders
	srx = srx_init;

	for (int bx = 0; bx < dx; bx++, srx+=32)
	{
		sry = sry_init;

		for (int by = 0; by<dy; by++, sry+=32)
		{
			if (top_tile.x+bx < 0 || top_tile.x+bx >= map->width ||
				top_tile.y+by < 0 || top_tile.y+by >= map->height) {

				SDL_Rect sr;

				sr.x=srx;
				sr.y=sry;
				sr.h=sr.w=32;

				SDL_BlitSurface(ttiles[top_tile.x+bx][top_tile.y+by][top_tile.z].terbitmap,
								&genRect(sr.h, sr.w, 0, 0),extSurf,&sr);
			} else {
				// TODO: these should be activable by the console
#ifdef MARK_BLOCKED_POSITIONS
				if(ttiles[top_tile.x+bx][top_tile.y+by][top_tile.z].tileInfo->blocked) //temporary hiding blocked positions
				{
					SDL_Rect sr;

					sr.x=srx;
					sr.y=sry;
					sr.h=sr.w=32;

					memset(rSurf->pixels, 128, rSurf->pitch * rSurf->h);
					SDL_BlitSurface(rSurf,&genRect(sr.h, sr.w, 0, 0),extSurf,&sr);
				}
#endif
#ifdef MARK_VISITABLE_POSITIONS
				if(ttiles[top_tile.x+bx][top_tile.y+by][top_tile.z].tileInfo->visitable) //temporary hiding visitable positions
				{
					SDL_Rect sr;

					sr.x=srx;
					sr.y=sry;
					sr.h=sr.w=32;

					memset(rSurf->pixels, 128, rSurf->pitch * rSurf->h);
					SDL_BlitSurface(rSurf,&genRect(sr.h, sr.w, 0, 0),extSurf,&sr);
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

	SDL_SetClipRect(extSurf, &prevClip); //restoring clip_rect

	SDL_FreeSurface(rSurf);
}

SDL_Surface * CMapHandler::getVisBitmap(int x, int y, const std::vector< std::vector< std::vector<unsigned char> > > & visibilityMap, int lvl)
{
	int size = visibilityMap.size()-1;							//is tile visible. arrangement: (like num keyboard)
	bool d7 = (x>0 && y>0) ? visibilityMap[x-1][y-1][lvl] : 0,	//789
		d8 = (y>0) ? visibilityMap[x][y-1][lvl] : 0,			//456
		d9 = (y>0 && x<size) ? visibilityMap[x+1][y-1][lvl] : 0,//123
		d4 = (x>0) ? visibilityMap[x-1][y][lvl] : 0,
		d5 = visibilityMap[x][y][lvl], //TODO use me - OMFG
		d6 = (x<size) ? visibilityMap[x+1][y][lvl] : 0,
		d1 = (x>0 && y<size) ? visibilityMap[x-1][y+1][lvl] : 0,
		d2 = (y<size) ? visibilityMap[x][y+1][lvl] : 0,
		d3 = (x<size && y<size) ? visibilityMap[x+1][y+1][lvl] : 0;

	if(!d2 && !d6 && !d4 && !d8 && !d7 && !d3 && !d9 && !d1)
	{
		return fullHide->ourImages[hideBitmap[x][y][lvl]].bitmap; //fully hidden
	}
	else if(!d2 && !d6 && !d4 && !d8 && !d7 && d3 && !d9 && !d1)
	{
		return partialHide->ourImages[22].bitmap; //visible right bottom corner
	}
	else if(!d2 && !d6 && !d4 && !d8 && !d7 && !d3 && d9 && !d1)
	{
		return partialHide->ourImages[15].bitmap; //visible right top corner
	}
	else if(!d2 && !d6 && !d4 && !d8 && !d7 && !d3 && !d9 && d1)
	{
		//return CSDL_Ext::rotate01(partialHide->ourImages[22].bitmap); //visible left bottom corner
		return partialHide->ourImages[34].bitmap; //visible left bottom corner
	}
	else if(!d2 && !d6 && !d4 && !d8 && d7 && !d3 && !d9 && !d1)
	{
		//return CSDL_Ext::rotate01(partialHide->ourImages[15].bitmap); //visible left top corner
		return partialHide->ourImages[35].bitmap;
	}
	else if(!d2 && !d6 && !d4 && d8 && d7 && !d3 && d9 && !d1)
	{
		//return partialHide->ourImages[rand()%2].bitmap; //visible top
		return partialHide->ourImages[0].bitmap; //visible top
	}
	else if(d2 && !d6 && !d4 && !d8 && !d7 && d3 && !d9 && d1)
	{
		//return partialHide->ourImages[4+rand()%2].bitmap; //visble bottom
		return partialHide->ourImages[4].bitmap; //visble bottom
	}
	else if(!d2 && !d6 && d4 && !d8 && d7 && !d3 && !d9 && d1)
	{
		//return CSDL_Ext::rotate01(partialHide->ourImages[2+rand()%2].bitmap); //visible left
		//return CSDL_Ext::rotate01(partialHide->ourImages[2].bitmap); //visible left
		return partialHide->ourImages[36].bitmap;
	}
	else if(!d2 && d6 && !d4 && !d8 && !d7 && d3 && d9 && !d1)
	{
		//return partialHide->ourImages[2+rand()%2].bitmap; //visible right
		return partialHide->ourImages[2].bitmap; //visible right
	}
	else if(d2 && d6 && !d4 && !d8 && !d7)
	{
		//return partialHide->ourImages[12+2*(rand()%2)].bitmap; //visible bottom, right - bottom, right; left top corner hidden
		return partialHide->ourImages[12].bitmap; //visible bottom, right - bottom, right; left top corner hidden
	}
	else if(!d2 && d6 && !d4 && d8 && !d1)
	{
		return partialHide->ourImages[13].bitmap; //visible right, right - top; left bottom corner hidden
	}
	else if(!d2 && !d6 && d4 && d8 && !d3)
	{
		//return CSDL_Ext::rotate01(partialHide->ourImages[13].bitmap); //visible top, top - left, left; right bottom corner hidden
		return partialHide->ourImages[37].bitmap;
	}
	else if(d2 && !d6 && d4 && !d8 && !d9)
	{
		//return CSDL_Ext::rotate01(partialHide->ourImages[12+2*(rand()%2)].bitmap); //visible left, left - bottom, bottom; right top corner hidden
		//return CSDL_Ext::rotate01(partialHide->ourImages[12].bitmap); //visible left, left - bottom, bottom; right top corner hidden
		return partialHide->ourImages[38].bitmap;
	}
	else if(d2 && d6 && d4 && d8)
	{
		return partialHide->ourImages[10].bitmap; //visible left, right, bottom and top
	}
	if(!d2 && !d6 && !d4 && !d8 && !d7 && d3 && d9 && !d1)
	{
		return partialHide->ourImages[16].bitmap; //visible right corners
	}
	if(!d2 && !d6 && !d4 && !d8 && d7 && !d3 && d9 && !d1)
	{
		return partialHide->ourImages[18].bitmap; //visible top corners
	}
	if(!d2 && !d6 && !d4 && !d8 && d7 && !d3 && !d9 && d1)
	{
		//return CSDL_Ext::rotate01(partialHide->ourImages[16].bitmap); //visible left corners
		return partialHide->ourImages[39].bitmap;
	}
	if(!d2 && !d6 && !d4 && !d8 && !d7 && d3 && !d9 && d1)
	{
		//return CSDL_Ext::hFlip(partialHide->ourImages[18].bitmap); //visible bottom corners
		return partialHide->ourImages[40].bitmap;
	}
	if(!d2 && !d6 && !d4 && !d8 && !d7 && !d3 && d9 && d1)
	{
		return partialHide->ourImages[17].bitmap; //visible right - top and bottom - left corners
	}
	if(!d2 && !d6 && !d4 && !d8 && d7 && d3 && !d9 && !d1)
	{
		//return CSDL_Ext::hFlip(partialHide->ourImages[17].bitmap); //visible top - left and bottom - right corners
		return partialHide->ourImages[41].bitmap;
	}
	if(!d2 && !d6 && !d4 && !d8 && !d7 && d3 && d9 && d1)
	{
		return partialHide->ourImages[19].bitmap; //visible corners without left top
	}
	if(!d2 && !d6 && !d4 && !d8 && d7 && d3 && d9 && !d1)
	{
		return partialHide->ourImages[20].bitmap; //visible corners without left bottom
	}
	if(!d2 && !d6 && !d4 && !d8 && d7 && !d3 && d9 && d1)
	{
		//return CSDL_Ext::rotate01(partialHide->ourImages[20].bitmap); //visible corners without right bottom
		return partialHide->ourImages[42].bitmap;
	}
	if(!d2 && !d6 && !d4 && !d8 && d7 && d3 && !d9 && d1)
	{
		//return CSDL_Ext::rotate01(partialHide->ourImages[19].bitmap); //visible corners without right top
		return partialHide->ourImages[43].bitmap;
	}
	if(!d2 && !d6 && !d4 && !d8 && d7 && d3 && d9 && d1)
	{
		return partialHide->ourImages[21].bitmap; //visible all corners only
	}
	if(d2 && d6 && d4 && !d8)
	{
		return partialHide->ourImages[6].bitmap; //hidden top
	}
	if(d2 && !d6 && d4 && d8)
	{
		return partialHide->ourImages[7].bitmap; //hidden right
	}
	if(!d2 && d6 && d4 && d8)
	{
		return partialHide->ourImages[8].bitmap; //hidden bottom
	}
	if(d2 && d6 && !d4 && d8)
	{
		//return CSDL_Ext::rotate01(partialHide->ourImages[7].bitmap); //hidden left
		return partialHide->ourImages[44].bitmap;
	}
	if(!d2 && d6 && d4 && !d8)
	{
		return partialHide->ourImages[9].bitmap; //hidden top and bottom
	}
	if(d2 && !d6 && !d4 && d8)
	{
		return partialHide->ourImages[29].bitmap;  //hidden left and right
	}
	if(!d2 && !d6 && !d4 && d8 && d3 && !d1)
	{
		return partialHide->ourImages[24].bitmap; //visible top and right bottom corner
	}
	if(!d2 && !d6 && !d4 && d8 && !d3 && d1)
	{
		//return CSDL_Ext::rotate01(partialHide->ourImages[24].bitmap); //visible top and left bottom corner
		return partialHide->ourImages[45].bitmap;
	}
	if(!d2 && !d6 && !d4 && d8 && d3 && d1)
	{
		return partialHide->ourImages[33].bitmap; //visible top and bottom corners
	}
	if(!d2 && !d6 && d4 && !d8 && !d3 && d9)
	{
		//return CSDL_Ext::rotate01(partialHide->ourImages[26].bitmap); //visible left and right top corner
		return partialHide->ourImages[46].bitmap;
	}
	if(!d2 && !d6 && d4 && !d8 && d3 && !d9)
	{
		//return CSDL_Ext::rotate01(partialHide->ourImages[25].bitmap); //visible left and right bottom corner
		return partialHide->ourImages[47].bitmap;
	}
	if(!d2 && !d6 && d4 && !d8 && d3 && d9)
	{
		return partialHide->ourImages[32].bitmap; //visible left and right corners
	}
	if(d2 && !d6 && !d4 && !d8 && d7 && !d9)
	{
		//return CSDL_Ext::rotate01(partialHide->ourImages[30].bitmap); //visible bottom and left top corner
		return partialHide->ourImages[48].bitmap;
	}
	if(d2 && !d6 && !d4 && !d8 && !d7 && d9)
	{
		return partialHide->ourImages[30].bitmap; //visible bottom and right top corner
	}
	if(d2 && !d6 && !d4 && !d8 && d7 && d9)
	{
		return partialHide->ourImages[31].bitmap; //visible bottom and top corners
	}
	if(!d2 && d6 && !d4 && !d8 && !d7 && d1)
	{
		return partialHide->ourImages[25].bitmap; //visible right and left bottom corner
	}
	if(!d2 && d6 && !d4 && !d8 && d7 && !d1)
	{
		return partialHide->ourImages[26].bitmap; //visible right and left top corner
	}
	if(!d2 && d6 && !d4 && !d8 && d7 && d1)
	{
		//return CSDL_Ext::rotate01(partialHide->ourImages[32].bitmap); //visible right and left cornres
		return partialHide->ourImages[49].bitmap;
	}
	if(d2 && d6 && !d4 && !d8 && d7)
	{
		return partialHide->ourImages[28].bitmap; //visible bottom, right - bottom, right; left top corner visible
	}
	else if(!d2 && d6 && !d4 && d8 && d1)
	{
		return partialHide->ourImages[27].bitmap; //visible right, right - top; left bottom corner visible
	}
	else if(!d2 && !d6 && d4 && d8 && d3)
	{
		//return CSDL_Ext::rotate01(partialHide->ourImages[27].bitmap); //visible top, top - left, left; right bottom corner visible
		return partialHide->ourImages[50].bitmap;
	}
	else if(d2 && !d6 && d4 && !d8 && d9)
	{
		//return CSDL_Ext::rotate01(partialHide->ourImages[28].bitmap); //visible left, left - bottom, bottom; right top corner visible
		return partialHide->ourImages[51].bitmap;
	}
	//newly added
	else if(!d2 && !d6 && !d4 && d8 && !d7 && !d3 && d9 && !d1) //visible t and tr
	{
		return partialHide->ourImages[0].bitmap;
	}
	else if(!d2 && !d6 && !d4 && d8 && d7 && !d3 && !d9 && !d1) //visible t and tl
	{
		return partialHide->ourImages[1].bitmap;
	}
	else if(d2 && !d6 && !d4 && !d8 && !d7 && d3 && !d9 && !d1) //visible b and br
	{
		return partialHide->ourImages[4].bitmap;
	}
	else if(d2 && !d6 && !d4 && !d8 && !d7 && !d3 && !d9 && d1) //visible b and bl
	{
		return partialHide->ourImages[5].bitmap;
	}
	else if(!d2 && !d6 && d4 && !d8 && d7 && !d3 && !d9 && !d1) //visible l and tl
	{
		return partialHide->ourImages[36].bitmap;
	}
	else if(!d2 && !d6 && d4 && !d8 && !d7 && !d3 && !d9 && d1) //visible l and bl
	{
		return partialHide->ourImages[36].bitmap;
	}
	else if(!d2 && d6 && !d4 && !d8 && !d7 && !d3 && d9 && !d1) //visible r and tr
	{
		return partialHide->ourImages[2].bitmap;
	}
	else if(!d2 && d6 && !d4 && !d8 && !d7 && d3 && !d9 && !d1) //visible r and br
	{
		return partialHide->ourImages[3].bitmap;
	}
	return fullHide->ourImages[0].bitmap; //this case should never happen, but it is better to hide too much than reveal it....
}

int CMapHandler::getCost(int3 &a, int3 &b, const CGHeroInstance *hero)
{
	int ret=-1;
	if(a.x>=CGI->mh->map->width && a.y>=CGI->mh->map->height)
		ret = hero->type->heroClass->terrCosts[CGI->mh->ttiles[CGI->mh->map->width-1][CGI->mh->map->width-1][a.z].tileInfo->malle];
	else if(a.x>=CGI->mh->map->width && a.y<CGI->mh->map->height)
		ret = hero->type->heroClass->terrCosts[CGI->mh->ttiles[CGI->mh->map->width-1][a.y][a.z].tileInfo->malle];
	else if(a.x<CGI->mh->map->width && a.y>=CGI->mh->map->height)
		ret = hero->type->heroClass->terrCosts[CGI->mh->ttiles[a.x][CGI->mh->map->width-1][a.z].tileInfo->malle];
	else
		ret = hero->type->heroClass->terrCosts[CGI->mh->ttiles[a.x][a.y][a.z].tileInfo->malle];
	if(!(a.x==b.x || a.y==b.y))
		ret*=1.41421;

	//TODO: use hero's pathfinding skill during calculating cost
	return ret;
}

//std::vector < CGObjectInstance * > CMapHandler::getVisitableObjs(int3 pos)
//{
//	std::vector < CGObjectInstance * > ret;
//	for(int h=0; h<ttiles[pos.x][pos.y][pos.z].objects.size(); ++h)
//	{
//		CGObjectInstance * curi = ttiles[pos.x][pos.y][pos.z].objects[h].first;
//		if(curi->visitableAt(- curi->pos.x + pos.x + curi->getWidth() - 1, -curi->pos.y + pos.y + curi->getHeight() - 1))
//			ret.push_back(curi);
//	}
//	return ret;
//}

std::string CMapHandler::getDefName(int id, int subid)
{
	CGDefInfo* temp = CGI->dobjinfo->gobjs[id][subid];
	if(temp)
		return temp->name;
	throw std::string("Def not found.");
}

bool CMapHandler::printObject(const CGObjectInstance *obj)
{
	const SDL_Surface *bitmap = obj->defInfo->handler->ourImages[0].bitmap;
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
				TerrainTile2 & curt = //TODO use me 
					ttiles
					  [obj->pos.x + fx - bitmap->w/32]
				      [obj->pos.y + fy - bitmap->h/32]
					  [obj->pos.z];


				ttiles[obj->pos.x + fx - bitmap->w/32+1][obj->pos.y + fy - bitmap->h/32+1][obj->pos.z].objects.push_back(toAdd);
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

unsigned char CMapHandler::getHeroFrameNum(const unsigned char &dir, const bool &isMoving) const
{
	if(isMoving)
	{
		switch(dir)
		{
		case 1:
			return 10;
		case 2:
			return 5;
		case 3:
			return 6;
		case 4:
			return 7;
		case 5:
			return 8;
		case 6:
			return 9;
		case 7:
			return 12;
		case 8:
			return 11;
		default:
			throw std::string("Something very wrong1.");
		}
	}
	else //if(isMoving)
	{
		switch(dir)
		{
		case 1:
			return 13;
		case 2:
			return 0;
		case 3:
			return 1;
		case 4:
			return 2;
		case 5:
			return 3;
		case 6:
			return 4;
		case 7:
			return 15;
		case 8:
			return 14;
		default:
			throw std::string("Something very wrong2.");
		}
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
	{
		return 0;
	}
	else if(a.x==b.x && a.y==b.y+1) //t
	{
		return 1;
	}
	else if(a.x==b.x-1 && a.y==b.y+1) //rt
	{
		return 2;
	}
	else if(a.x==b.x-1 && a.y==b.y) //r
	{
		return 3;
	}
	else if(a.x==b.x-1 && a.y==b.y-1) //rb
	{
		return 4;
	}
	else if(a.x==b.x && a.y==b.y-1) //b
	{
		return 5;
	}
	else if(a.x==b.x+1 && a.y==b.y-1) //lb
	{
		return 6;
	}
	else if(a.x==b.x+1 && a.y==b.y) //l
	{
		return 7;
	}
	return -2; //shouldn't happen
}

void CMapHandler::updateWater() //shift colors in palettes of water tiles
{
	SDL_Color palette[14];
	for(size_t j=0; j < terrainGraphics[8].size(); ++j)
	{
		for(int i=0; i<12; ++i)
		{
			palette[(i+1)%12] = terrainGraphics[8][j]->format->palette->colors[229 + i];
		}
		SDL_SetColors(terrainGraphics[8][j],palette,229,12);
		for(int i=0; i<14; ++i)
		{
			palette[(i+1)%14] = terrainGraphics[8][j]->format->palette->colors[242 + i];
		}
		SDL_SetColors(terrainGraphics[8][j],palette,242,14);
	}
}

CMapHandler::~CMapHandler()
{
	delete fullHide;
	delete partialHide;

	for(int i=0; i < map->defy.size(); i++)
		delete map->defy[i];

	for(int i=0; i < roadDefs.size(); i++)
		delete roadDefs[i];

	for(int i=0; i < staticRiverDefs.size(); i++)
		delete staticRiverDefs[i];

	//TODO: delete border graphics
}

CMapHandler::CMapHandler()
{
	mapW = mapH = 0;
	frameW = frameH = 0;
	fullHide = NULL;
	partialHide = NULL;
}

TerrainTile2::TerrainTile2()
 :tileInfo(0),terbitmap(0)
{}
