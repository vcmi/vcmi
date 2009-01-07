#include "stdafx.h"
#include "mapHandler.h"
#include "SDL_Extensions.h"
#include "CGameInfo.h"
#include <cstdlib>
#include "hch/CLodHandler.h"
#include "hch/CDefObjInfoHandler.h"
#include <algorithm>
#include "CGameState.h"
#include "hch/CHeroHandler.h"
#include "hch/CTownHandler.h"
#include "client/Graphics.h"
#include <iomanip>
#include <sstream>
#include "hch/CObjectHandler.h"
#include "map.h"
#include "hch/CDefHandler.h"
extern SDL_Surface * screen;
std::string nameFromType (int typ)
{
	switch((EterrainType)typ)
	{
		case dirt:
		{
			return std::string("DIRTTL.DEF");
			break;
		}
		case sand:
		{
			return std::string("SANDTL.DEF");
			break;
		}
		case grass:
		{
			return std::string("GRASTL.DEF");
			break;
		}
		case snow:
		{
			return std::string("SNOWTL.DEF");
			break;
		}
		case swamp:
		{
			return std::string("SWMPTL.DEF");			
			break;
		}
		case rough:
		{
			return std::string("ROUGTL.DEF");		
			break;
		}
		case subterranean:
		{
			return std::string("SUBBTL.DEF");		
			break;
		}
		case lava:
		{
			return std::string("LAVATL.DEF");		
			break;
		}
		case water:
		{
			return std::string("WATRTL.DEF");
			break;
		}
		case rock:
		{
			return std::string("ROCKTL.DEF");		
			break;
		}
                case border:
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
void alphaTransformDef(CGDefInfo * defInfo)
{	
	SDL_Surface * alphaTransSurf = SDL_CreateRGBSurface(SDL_SWSURFACE, 12, 12, 32, 0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000);
	for(int yy=0;yy<defInfo->handler->ourImages.size();yy++)
	{
		defInfo->handler->ourImages[yy].bitmap = CSDL_Ext::alphaTransform(defInfo->handler->ourImages[yy].bitmap);
		defInfo->handler->alphaTransformed = true;
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

	roadDefs.push_back(CDefHandler::giveDef("dirtrd.def"));
	roadDefs.push_back(CDefHandler::giveDef("gravrd.def"));
	roadDefs.push_back(CDefHandler::giveDef("cobbrd.def"));
	staticRiverDefs.push_back(CDefHandler::giveDef("clrrvr.def"));
	staticRiverDefs.push_back(CDefHandler::giveDef("icyrvr.def"));
	staticRiverDefs.push_back(CDefHandler::giveDef("mudrvr.def"));
	staticRiverDefs.push_back(CDefHandler::giveDef("lavrvr.def"));
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
	ttiles.resize(CGI->mh->map->width,Woff);
	for (int i=0-Woff;i<ttiles.size()-Woff;i++)
	{
		ttiles[i].resize(CGI->mh->map->height,Hoff);
	}
	for (int i=0-Woff;i<ttiles.size()-Woff;i++)
	{
		for (int j=0-Hoff;j<(int)CGI->mh->map->height+Hoff;j++)
			ttiles[i][j].resize(CGI->mh->map->twoLevel+1,0);
	}



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
					int cDir;
					bool rotV, rotH;

					int roadpom = pom.tileInfo->malle-1,
						impom = pom.tileInfo->roadDir;
					SDL_Surface *pom1 = roadDefs[roadpom]->ourImages[impom].bitmap;
					ttiles[i][j][k].roadbitmap.push_back(pom1);
					cDir = pom.tileInfo->roadDir;

					rotH = (pom.tileInfo->siodmyTajemniczyBajt >> 5) & 1;
					rotV = (pom.tileInfo->siodmyTajemniczyBajt >> 4) & 1;

					if(rotH)
					{
						ttiles[i][j][k].roadbitmap[0] = CSDL_Ext::hFlip(ttiles[i][j][k].roadbitmap[0]);
					}
					if(rotV)
					{
						ttiles[i][j][k].roadbitmap[0] = CSDL_Ext::rotate01(ttiles[i][j][k].roadbitmap[0]);
					}
					if(rotH || rotV)
					{
						ttiles[i][j][k].roadbitmap[0] = CSDL_Ext::alphaTransform(ttiles[i][j][k].roadbitmap[0]);
					}
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
					int cDir;
					bool rotH, rotV;

					ttiles[i][j][k].rivbitmap.push_back(staticRiverDefs[map->terrain[i][j][k].nuine-1]->ourImages[map->terrain[i][j][k].rivDir].bitmap);
					cDir = map->terrain[i][j][k].rivDir;
					rotH = (map->terrain[i][j][k].siodmyTajemniczyBajt >> 3) & 1;
					rotV = (map->terrain[i][j][k].siodmyTajemniczyBajt >> 2) & 1;

					if(rotH)
					{
						ttiles[i][j][k].rivbitmap[0] = CSDL_Ext::hFlip(ttiles[i][j][k].rivbitmap[0]);
					}
					if(rotV)
					{
						ttiles[i][j][k].rivbitmap[0] = CSDL_Ext::rotate01(ttiles[i][j][k].rivbitmap[0]);
					}
					if(rotH || rotV)
					{
						ttiles[i][j][k].rivbitmap[0] = CSDL_Ext::alphaTransform(ttiles[i][j][k].rivbitmap[0]);
					}
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

	for (int i=0-Woff; i<map->width+Woff; i++) //jest po szeroko�ci
	{
		for (int j=0-Hoff; j<map->height+Hoff;j++) //po wysoko�ci
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
		if((map->objects[f]->ID==34 && static_cast<CGHeroInstance*>(map->objects[f])->inTownGarrison)
			|| !map->objects[f]->defInfo)
		{
			continue;
		}
		CDefHandler * curd = map->objects[f]->defInfo->handler;
		if(curd)
		{
			for(int fx=0; fx<curd->ourImages[0].bitmap->w>>5; ++fx) //curd->ourImages[0].bitmap->w/32
			{
				for(int fy=0; fy<curd->ourImages[0].bitmap->h>>5; ++fy) //curd->ourImages[0].bitmap->h/32
				{
					SDL_Rect cr;
					cr.w = 32;
					cr.h = 32;
					cr.x = fx<<5; //fx*32
					cr.y = fy<<5; //fy*32
					std::pair<CGObjectInstance*,SDL_Rect> toAdd = std::make_pair(map->objects[f],cr);
					
					if((map->objects[f]->pos.x + fx - curd->ourImages[0].bitmap->w/32+1)>=0 && (map->objects[f]->pos.x + fx - curd->ourImages[0].bitmap->w/32+1)<ttiles.size()-Woff && (map->objects[f]->pos.y + fy - curd->ourImages[0].bitmap->h/32+1)>=0 && (map->objects[f]->pos.y + fy - curd->ourImages[0].bitmap->h/32+1)<ttiles[0].size()-Hoff)
					{
						//TerrainTile2 & curt =
						//	ttiles
						//	[map->objects[f]->pos.x + fx - curd->ourImages[0].bitmap->w/32]
						//[map->objects[f]->pos.y + fy - curd->ourImages[0].bitmap->h/32]
						//[map->objects[f]->pos.z];
						ttiles[map->objects[f]->pos.x + fx - curd->ourImages[0].bitmap->w/32+1][map->objects[f]->pos.y + fy - curd->ourImages[0].bitmap->h/32+1][map->objects[f]->pos.z].objects.push_back(toAdd);
					}
				} // for(int fy=0; fy<curd->ourImages[0].bitmap->h/32; ++fy)
			} //for(int fx=0; fx<curd->ourImages[0].bitmap->w/32; ++fx)
		}//if curd
	} // for(int f=0; f<map->objects.size(); ++f)
	for(int ix=0; ix<ttiles.size()-Woff; ++ix)
	{
		for(int iy=0; iy<ttiles[0].size()-Hoff; ++iy)
		{
			for(int iz=0; iz<ttiles[0][0].size(); ++iz)
			{
				stable_sort(ttiles[ix][iy][iz].objects.begin(), ttiles[ix][iy][iz].objects.end(), ocmptwo);
			}
		}
	}
}
void processDef (CGDefInfo* def)
{
	if(def->id == 26)
		return;
	def->handler=CDefHandler::giveDef(def->name);
	def->width = def->handler->ourImages[0].bitmap->w/32;
	def->height = def->handler->ourImages[0].bitmap->h/32;
	CGDefInfo* pom = CGI->dobjinfo->gobjs[def->id][def->subid];
	if(pom && def->id!=98)
	{
		pom->handler = def->handler;
		pom->width = pom->handler->ourImages[0].bitmap->w/32;
		pom->height = pom->handler->ourImages[0].bitmap->h/32;
	}
	else if(def->id != 34 && def->id != 98)
		tlog3 << "\t\tMinor warning: lacking def info for " << def->id << " " << def->subid <<" " << def->name << std::endl;
	if(!def->handler->alphaTransformed)
	{
		for(size_t yy=0; yy < def->handler->ourImages.size(); ++yy)
		{
			def->handler->ourImages[yy].bitmap = CSDL_Ext::alphaTransform(def->handler->ourImages[yy].bitmap);
			def->handler->alphaTransformed = true;
		}
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

	//for(int i=0; i<map->defy.size(); i++)
	//{
	//	map->defy[i]->serial = i;
	//	processDef(map->defy[i]);
	//}

	std::for_each(map->defy.begin(),map->defy.end(),processDef); //load h3m defs
	//std::for_each(map->defs.begin(),map->defs.end(),processDef); //and non-h3m defs
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

SDL_Surface * CMapHandler::terrainRect(int x, int y, int dx, int dy, int level, unsigned char anim, std::vector< std::vector< std::vector<unsigned char> > > * visibilityMap, bool otherHeroAnim, unsigned char heroAnim, SDL_Surface * extSurf, SDL_Rect * extRect)
{
	if(!otherHeroAnim)
		heroAnim = anim; //the same, as it should be
	//setting surface to blit at
	SDL_Surface * su = NULL; //blitting surface CSDL_Ext::newSurface(dx*32, dy*32, CSDL_Ext::std32bppSurface);
	if(extSurf)
	{
		su = extSurf;
	}
	else
	{
		 su = CSDL_Ext::newSurface(dx*32, dy*32, CSDL_Ext::std32bppSurface);
	}

	if (((dx+x)>((map->width+Woff)) || (dy+y)>((map->height+Hoff))) || ((x<-Woff)||(y<-Hoff) ) )
		throw new std::string("terrainRect: out of range");
	////printing terrain
	for (int bx=0; bx<dx; bx++)
	{
		for (int by=0; by<dy; by++)
		{
			const TerrainTile2 & tile = ttiles[x+bx][y+by][level];
			SDL_Rect sr;
			sr.y=by*32;
			sr.x=bx*32;
			sr.h=sr.w=32;
			validateRectTerr(&sr, extRect);
			if(tile.terbitmap)
			{
				SDL_BlitSurface(ttiles[x+bx][y+by][level].terbitmap, &genRect(sr.h, sr.w, 0, 0), su, &sr);
			}
			else
			{
				switch(tile.tileInfo->siodmyTajemniczyBajt%4)
				{
				case 0:
					SDL_BlitSurface(terrainGraphics[tile.tileInfo->tertype][tile.tileInfo->terview],
						&genRect(sr.h, sr.w, 0, 0),su,&sr);
					break;
				case 1:
					CSDL_Ext::blitWithRotate1(terrainGraphics[tile.tileInfo->tertype][tile.tileInfo->terview],
						&genRect(sr.h, sr.w, 0, 0),su,&sr);
					break;
				case 2:
					CSDL_Ext::blitWithRotate2(terrainGraphics[tile.tileInfo->tertype][tile.tileInfo->terview],
						&genRect(sr.h, sr.w, 0, 0),su,&sr);
					break;
				default:
					CSDL_Ext::blitWithRotate3(terrainGraphics[tile.tileInfo->tertype][tile.tileInfo->terview],
						&genRect(sr.h, sr.w, 0, 0),su,&sr);
					break;
				}
			}
		}
	}
	////terrain printed
	////printing rivers
	for (int bx=0; bx<dx; bx++)
	{
		for (int by=0; by<dy; by++)
		{
			SDL_Rect sr;
			sr.y=by*32;
			sr.x=bx*32;
			sr.h=sr.w=32;
			validateRectTerr(&sr, extRect);
			if(ttiles[x+bx][y+by][level].rivbitmap.size())
			{
				CSDL_Ext::blit8bppAlphaTo24bpp(ttiles[x+bx][y+by][level].rivbitmap[anim%ttiles[x+bx][y+by][level].rivbitmap.size()],&genRect(sr.h, sr.w, 0, 0),su,&sr);
			}
		}
	}
	////rivers printed
	////printing roads
	for (int bx=0; bx<dx; bx++)
	{
		for (int by=-1; by<dy; by++)
		{
			if(y+by<=-4)
				continue;
			SDL_Rect sr;
			sr.y=by*32+16;
			sr.x=bx*32;
			sr.h=sr.w=32;
			validateRectTerr(&sr, extRect);
			if(ttiles[x+bx][y+by][level].roadbitmap.size())
			{
				CSDL_Ext::blit8bppAlphaTo24bpp(ttiles[x+bx][y+by][level].roadbitmap[anim%ttiles[x+bx][y+by][level].roadbitmap.size()], &genRect(sr.h, sr.w, 0, (by==-1 ? 16 : 0)),su,&sr);
			}
		}
	}
	////roads printed
	////printing objects

	for (int bx=0; bx<dx; bx++)
	{
		for (int by=0; by<dy; by++)
		{
			for(int h=0; h < ttiles[x+bx][y+by][level].objects.size(); ++h)
			{
				SDL_Rect sr;
				sr.w = 32;
				sr.h = 32;
				sr.x = (bx)*32;
				sr.y = (by)*32;
				validateRectTerr(&sr, extRect);

				SDL_Rect pp = ttiles[x+bx][y+by][level].objects[h].second;
				pp.h = sr.h;
				pp.w = sr.w;
				const CGHeroInstance * themp = (dynamic_cast<const CGHeroInstance*>(ttiles[x+bx][y+by][level].objects[h].first));

				if(themp && themp->moveDir && !themp->isStanding && themp->ID!=62) //last condition - this is not prison
				{
					int imgVal = 8;
					SDL_Surface * tb;

					if(themp->type==NULL)
						continue;
					std::vector<Cimage> & iv = themp->type->heroClass->moveAnim->ourImages;

                    size_t gg;
					for(gg=0; gg<iv.size(); ++gg)
					{
						if(iv[gg].groupNumber==getHeroFrameNum(themp->moveDir, !themp->isStanding))
						{
							tb = iv[gg+heroAnim%imgVal].bitmap;
							break;
						}
					}
					CSDL_Ext::blit8bppAlphaTo24bpp(tb,&pp,su,&sr);
					pp.y+=imgVal*2-32;
					sr.y-=16;
					SDL_BlitSurface(graphics->flags4[themp->getOwner()]->ourImages[gg+heroAnim%imgVal+35].bitmap, &pp, su, &sr);
				}
				else if(themp && themp->moveDir && themp->isStanding && themp->ID!=62) //last condition - this is not prison)
				{
					int imgVal = 8;
					SDL_Surface * tb;

					if(themp->type==NULL)
						continue;
					std::vector<Cimage> & iv = themp->type->heroClass->moveAnim->ourImages;

                    size_t gg;
					for(gg=0; gg < iv.size(); ++gg)
					{
						if(iv[gg].groupNumber==getHeroFrameNum(themp->moveDir, !themp->isStanding))
						{
							tb = iv[gg].bitmap;
							break;
						}
					}
					CSDL_Ext::blit8bppAlphaTo24bpp(tb,&pp,su,&sr);
					if(themp->pos.x==x+bx && themp->pos.y==y+by)
					{
						SDL_Rect bufr = sr;
						bufr.x-=2*32;
						bufr.y-=1*32;
						bufr.h = 64;
						bufr.w = 96;
						if(bufr.x-extRect->x>-64)
							SDL_BlitSurface(graphics->flags4[themp->getOwner()]->ourImages[ getHeroFrameNum(themp->moveDir, !themp->isStanding) *8+(heroAnim/4)%imgVal].bitmap, NULL, su, &bufr);
					}
				}
				else
				{
					int imgVal = ttiles[x+bx][y+by][level].objects[h].first->defInfo->handler->ourImages.size();
					int phaseShift = ttiles[x+bx][y+by][level].objects[h].first->animPhaseShift;

					//setting appropriate flag color
					if((ttiles[x+bx][y+by][level].objects[h].first->tempOwner>=0 && ttiles[x+bx][y+by][level].objects[h].first->tempOwner<8) || ttiles[x+bx][y+by][level].objects[h].first->tempOwner==255)
						CSDL_Ext::setPlayerColor(ttiles[x+bx][y+by][level].objects[h].first->defInfo->handler->ourImages[(anim+phaseShift)%imgVal].bitmap, ttiles[x+bx][y+by][level].objects[h].first->tempOwner);
					
					CSDL_Ext::blit8bppAlphaTo24bpp(ttiles[x+bx][y+by][level].objects[h].first->defInfo->handler->ourImages[(anim+phaseShift)%imgVal].bitmap,&pp,su,&sr);
				}
			}
		}
	}

	////objects printed, printing shadow
	for (int bx=0; bx<dx; bx++)
	{
		for (int by=0; by<dy; by++)
		{
			SDL_Rect sr;
			sr.y=by*32;
			sr.x=bx*32;
			sr.h=sr.w=32;
			validateRectTerr(&sr, extRect);
			
			if(bx+x>=0 && by+y>=0 && bx+x<CGI->mh->map->width && by+y<CGI->mh->map->height && !(*visibilityMap)[bx+x][by+y][level])
			{
				SDL_Surface * hide = getVisBitmap(bx+x, by+y, *visibilityMap, level);
				CSDL_Ext::blit8bppAlphaTo24bpp(hide, &genRect(sr.h, sr.w, 0, 0), su, &sr);
			}
		}
	}
	////shadow printed
	//printing borders
	for (int bx=0; bx<dx; bx++)
	{
		for (int by=0; by<dy; by++)
		{
			if(bx+x<0 || by+y<0 || bx+x>map->width+(-1) || by+y>map->height+(-1))
			{
				SDL_Rect sr;
				sr.y=by*32;
				sr.x=bx*32;
				sr.h=sr.w=32;
				validateRectTerr(&sr, extRect);
				SDL_BlitSurface(ttiles[x+bx][y+by][level].terbitmap,&genRect(sr.h, sr.w, 0, 0),su,&sr);
			}
			else 
			{
#ifdef MARK_BLOCKED_POSITIONS
				if(ttiles[x+bx][y+by][level].tileInfo->blocked) //temporary hiding blocked positions
				{
					SDL_Rect sr;
					sr.y=by*32;
					sr.x=bx*32;
					sr.h=sr.w=32;
					validateRectTerr(&sr, extRect);

					SDL_Surface * ns =  CSDL_Ext::newSurface(32, 32, CSDL_Ext::std32bppSurface);
					for(int f=0; f<ns->w*ns->h*4; ++f)
					{
						*((unsigned char*)(ns->pixels) + f) = 128;
					}

					SDL_BlitSurface(ns,&genRect(sr.h, sr.w, 0, 0),su,&sr);

					SDL_FreeSurface(ns);
				}
#endif
#ifdef MARK_VISITABLE_POSITIONS
				if(ttiles[x+bx][y+by][level].tileInfo->visitable) //temporary hiding visitable positions
				{
					SDL_Rect sr;
					sr.y=by*32;
					sr.x=bx*32;
					sr.h=sr.w=32;
					validateRectTerr(&sr, extRect);

					SDL_Surface * ns = CSDL_Ext::newSurface(32, 32, CSDL_Ext::std32bppSurface);
					for(int f=0; f<ns->w*ns->h*4; ++f)
					{
						*((unsigned char*)(ns->pixels) + f) = 128;
					}

					SDL_BlitSurface(ns,&genRect(sr.h, sr.w, 0, 0),su,&sr);

					SDL_FreeSurface(ns);
				}
#endif
			}
		}
	}
	//borders printed
	return su;
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
#ifndef __GNUC__
	throw new std::exception("Def not found.");
#else
	throw new std::exception();
#endif
}

bool CMapHandler::printObject(const CGObjectInstance *obj)
{
	CDefHandler * curd = obj->defInfo->handler;
	for(int fx=0; fx<curd->ourImages[0].bitmap->w/32; ++fx)
	{
		for(int fy=0; fy<curd->ourImages[0].bitmap->h/32; ++fy)
		{
			SDL_Rect cr;
			cr.w = 32;
			cr.h = 32;
			cr.x = fx*32;
			cr.y = fy*32;
			std::pair<const CGObjectInstance*,SDL_Rect> toAdd = std::make_pair(obj, cr);
			if((obj->pos.x + fx - curd->ourImages[0].bitmap->w/32+1)>=0 && (obj->pos.x + fx - curd->ourImages[0].bitmap->w/32+1)<ttiles.size()-Woff && (obj->pos.y + fy - curd->ourImages[0].bitmap->h/32+1)>=0 && (obj->pos.y + fy - curd->ourImages[0].bitmap->h/32+1)<ttiles[0].size()-Hoff)
			{
				TerrainTile2 & curt = //TODO use me 
					ttiles
					  [obj->pos.x + fx - curd->ourImages[0].bitmap->w/32]
				      [obj->pos.y + fy - curd->ourImages[0].bitmap->h/32]
					  [obj->pos.z];


				ttiles[obj->pos.x + fx - curd->ourImages[0].bitmap->w/32+1][obj->pos.y + fy - curd->ourImages[0].bitmap->h/32+1][obj->pos.z].objects.push_back(toAdd);
			}

		} // for(int fy=0; fy<curd->ourImages[0].bitmap->h/32; ++fy)
	} //for(int fx=0; fx<curd->ourImages[0].bitmap->w/32; ++fx)
	return true;
}

bool CMapHandler::hideObject(const CGObjectInstance *obj)
{
	CDefHandler * curd = obj->defInfo->handler;
	for(int fx=0; fx<curd->ourImages[0].bitmap->w/32; ++fx)
	{
		for(int fy=0; fy<curd->ourImages[0].bitmap->h/32; ++fy)
		{
			if((obj->pos.x + fx - curd->ourImages[0].bitmap->w/32+1)>=0 && (obj->pos.x + fx - curd->ourImages[0].bitmap->w/32+1)<ttiles.size()-Woff && (obj->pos.y + fy - curd->ourImages[0].bitmap->h/32+1)>=0 && (obj->pos.y + fy - curd->ourImages[0].bitmap->h/32+1)<ttiles[0].size()-Hoff)
			{
				std::vector < std::pair<const CGObjectInstance*,SDL_Rect> > & ctile = ttiles[obj->pos.x + fx - curd->ourImages[0].bitmap->w/32+1][obj->pos.y + fy - curd->ourImages[0].bitmap->h/32+1][obj->pos.z].objects;
				for(size_t dd=0; dd < ctile.size(); ++dd)
				{
					if(ctile[dd].first->id==obj->id)
						ctile.erase(ctile.begin() + dd);
				}
			}

		} // for(int fy=0; fy<curd->ourImages[0].bitmap->h/32; ++fy)
	} //for(int fx=0; fx<curd->ourImages[0].bitmap->w/32; ++fx)
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
#ifndef __GNUC__
			throw std::exception("Something very wrong1.");
#else
			throw std::exception();
#endif
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
#ifndef __GNUC__
			throw std::exception("Something very wrong2.");
#else
			throw std::exception();
#endif
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

TerrainTile2::TerrainTile2()
:terbitmap(0),tileInfo(0)
{}
