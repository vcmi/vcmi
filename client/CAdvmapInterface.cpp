#include "AdventureMapButton.h"
#include "CAdvmapInterface.h"
#include "../CCallback.h"
#include "CCastleInterface.h"
#include "CCursorHandler.h"
#include "CGameInfo.h"
#include "CHeroWindow.h"
#include "CKingdomInterface.h"
#include "CMessage.h"
#include "CPlayerInterface.h"
#include "SDL_Extensions.h"
#include "CBitmapHandler.h"
#include "CConfigHandler.h"
#include "CSpellWindow.h"
#include "Graphics.h"
#include "CDefHandler.h"
#include "../lib/CGeneralTextHandler.h"
#include "../lib/CHeroHandler.h"
#include "../lib/CObjectHandler.h"
#include "../lib/CTownHandler.h"
#include "../lib/map.h"
#include "mapHandler.h"
#include "../stdafx.h"
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/assign/std/vector.hpp>
#include <boost/thread.hpp>
#include <sstream>
#include "CPreGame.h"
#include "../lib/VCMI_Lib.h"
#include "../lib/CSpellHandler.h"
#include <boost/foreach.hpp>
#include "CSoundBase.h"

#ifdef _MSC_VER
#pragma warning (disable : 4355)
#endif

/*
 * CAdvMapInterface.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#define ADVOPT (conf.go()->ac)
using namespace boost::logic;
using namespace boost::assign;
using namespace CSDL_Ext;

CAdvMapInt *adventureInt;

CMinimap::CMinimap(bool draw)
{
	int3 mapSizes = LOCPLINT->cb->getMapSize();
	statusbarTxt = CGI->generaltexth->zelp[291].first;
	rcText = CGI->generaltexth->zelp[291].second;
	pos.x=ADVOPT.minimapX;//630
	pos.y=ADVOPT.minimapY;//26
	pos.h=ADVOPT.minimapW;//144
	pos.w=ADVOPT.minimapH;//144

	temps = newSurface(pos.w,pos.h);

	std::ifstream is(DATA_DIR "/config/minimap.txt",std::ifstream::in);
	for (int i=0;i<TERRAIN_TYPES;i++)
	{
		std::pair<int,SDL_Color> vinya;
		std::pair<int,SDL_Color> vinya2;
		int pom;
		is >> pom;
		vinya2.first=vinya.first=pom;
		is >> pom;
		vinya.second.r=pom;
		is >> pom;
		vinya.second.g=pom;
		is >> pom;
		vinya.second.b=pom;
		is >> pom;
		vinya2.second.r=pom;
		is >> pom;
		vinya2.second.g=pom;
		is >> pom;
		vinya2.second.b=pom;
		vinya.second.unused=vinya2.second.unused=255;
		colors.insert(vinya);
		colorsBlocked.insert(vinya2);
	}
	is.close();

	if (draw)
		redraw();
}
CMinimap::~CMinimap()
{
	SDL_FreeSurface(temps);
	
	for(int g=0; g<map.size(); ++g)
		SDL_FreeSurface(map[g]);
	map.clear();

	for(int g=0; g<FoW.size(); ++g)
		SDL_FreeSurface(FoW[g]);
	FoW.clear();

	for(int g=0; g<flObjs.size(); ++g)
		SDL_FreeSurface(flObjs[g]);
	flObjs.clear();
}

void CMinimap::draw(SDL_Surface * to)
{
	int3 mapSizes = LOCPLINT->cb->getMapSize();
	//draw terrain
	blitAt(map[adventureInt->position.z],0,0,temps);

	//draw heroes
	std::vector <const CGHeroInstance *> hh = LOCPLINT->cb->getHeroesInfo(false);
	int mw = map[0]->w, mh = map[0]->h,
		wo = mw/mapSizes.x, ho = mh/mapSizes.y;

	for (size_t i=0; i < hh.size(); ++i)
	{
		int3 hpos = hh[i]->getPosition(false);
		if(hpos.z!=adventureInt->position.z)
			continue;
		//float zawx = ((float)hpos.x/CGI->mh->sizes.x), zawy = ((float)hpos.y/CGI->mh->sizes.y);
		int3 maplgp ( (hpos.x*mw)/mapSizes.x, (hpos.y*mh)/mapSizes.y, hpos.z );
		for (int ii=0; ii<wo; ii++)
		{
			for (int jj=0; jj<ho; jj++)
			{
				SDL_PutPixelWithoutRefresh(temps,maplgp.x+ii,maplgp.y+jj,graphics->playerColors[hh[i]->getOwner()].r,
						graphics->playerColors[hh[i]->getOwner()].g,graphics->playerColors[hh[i]->getOwner()].b);
			}
		}
	}

	blitAt(flObjs[adventureInt->position.z],0,0,temps);

	blitAt(FoW[adventureInt->position.z],0,0,temps);

	//draw radar
	const int tilesw=(ADVOPT.advmapW+31)/32;
	const int tilesh=(ADVOPT.advmapH+31)/32;
	int bx = (((float)adventureInt->position.x)/(((float)mapSizes.x)))*pos.w,
		by = (((float)adventureInt->position.y)/(((float)mapSizes.y)))*pos.h,
		rx = (((float)tilesw)/(mapSizes.x))*((float)pos.w), //width
		ry = (((float)tilesh)/(mapSizes.y))*((float)pos.h); //height

	CSDL_Ext::drawDashedBorder(temps, Rect(bx, by, rx, ry), int3(255,75,125));

	//blitAt(radar,bx,by,temps);
	blitAt(temps,pos.x,pos.y,to);
}
void CMinimap::redraw(int level)// (level==-1) => redraw all levels
{
	initMap(level);

	//FoW
	initFoW(level);

	//flaggable objects
	initFlaggableObjs(level);

	//showing tiles
	showVisibleTiles();
}

void CMinimap::initMap(int level)
{
	/*for(int g=0; g<map.size(); ++g)
	{
		SDL_FreeSurface(map[g]);
	}
	map.clear();*/

	int3 mapSizes = LOCPLINT->cb->getMapSize();
	for (size_t i=0; i<CGI->mh->sizes.z; i++)
	{
		SDL_Surface * pom ;
		if ((level>=0) && (i!=level))
			continue;
		if (map.size()<i+1)
			pom = CSDL_Ext::newSurface(pos.w,pos.h,screen);
		else pom = map[i];
		for (int x=0;x<pos.w;x++)
		{
			for (int y=0;y<pos.h;y++)
			{
				int mx=(mapSizes.x*x)/pos.w;
				int my=(mapSizes.y*y)/pos.h;
				const TerrainTile * tile = LOCPLINT->cb->getTileInfo(int3(mx, my, i));
				if(tile)
				{
					if (tile->blocked && (!tile->visitable))
						SDL_PutPixelWithoutRefresh(pom, x, y, colorsBlocked[tile->tertype].r, colorsBlocked[tile->tertype].g, colorsBlocked[tile->tertype].b);
					else SDL_PutPixelWithoutRefresh(pom, x, y, colors[tile->tertype].r, colors[tile->tertype].g, colors[tile->tertype].b);
				}
			}
		}
		map.push_back(pom);

	}
}

void CMinimap::initFoW(int level)
{
	/*for(int g=0; g<FoW.size(); ++g)
	{
		SDL_FreeSurface(FoW[g]);
	}
	FoW.clear();*/

	int3 mapSizes = LOCPLINT->cb->getMapSize();
	int mw = map[0]->w, mh = map[0]->h;//,
		//wo = mw/mapSizes.x, ho = mh/mapSizes.y; //TODO use me
	for(int d=0; d<CGI->mh->map->twoLevel+1; ++d)
	{
		if(level>=0 && d!=level)
			continue;
		SDL_Surface * pt = CSDL_Ext::newSurface(pos.w, pos.h, CSDL_Ext::std32bppSurface);
		for (int i=0; i<mw; i++)
		{
			for (int j=0; j<mh; j++)
			{
				int3 pp( ((i*mapSizes.x)/mw), ((j*mapSizes.y)/mh), d );
				if ( !LOCPLINT->cb->isVisible(pp) )
				{
					CSDL_Ext::SDL_PutPixelWithoutRefresh(pt,i,j,0,0,0);
				}
			}
		}
		FoW.push_back(pt);
	}
}

void CMinimap::initFlaggableObjs(int level)
{
	/*for(int g=0; g<flObjs.size(); ++g)
	{
		SDL_FreeSurface(flObjs[g]);
	}
	flObjs.clear();*/

	int3 mapSizes = LOCPLINT->cb->getMapSize();
	int mw = map[0]->w, mh = map[0]->h;
	for(int d=0; d<CGI->mh->map->twoLevel+1; ++d)
	{
		if(level>=0 && d!=level)
			continue;
		SDL_Surface * pt = CSDL_Ext::newSurface(pos.w, pos.h, CSDL_Ext::std32bppSurface);
		for (int i=0; i<mw; i++)
		{
			for (int j=0; j<mh; j++)
			{
				CSDL_Ext::SDL_PutPixelWithoutRefresh(pt,i,j,0,0,0,0);
			}
		}
		flObjs.push_back(pt);
	}
}

void CMinimap::updateRadar()
{}

void CMinimap::clickRight(tribool down, bool previousState)
{
	adventureInt->handleRightClick(rcText,down);
}

void CMinimap::clickLeft(tribool down, bool previousState)
{
	if (down && (!previousState))
		activateMouseMove();
	else if (!down)
	{
		if (std::find(GH.motioninterested.begin(),GH.motioninterested.end(),this)!=GH.motioninterested.end())
			deactivateMouseMove();
	}
	//ClickableL::clickLeft(down);
	if (!((bool)down))
		return;

	float dx=((float)(GH.current->motion.x-pos.x))/((float)pos.w),
		dy=((float)(GH.current->motion.y-pos.y))/((float)pos.h);

	int3 newCPos;
	newCPos.x = (CGI->mh->sizes.x*dx);
	newCPos.y = (CGI->mh->sizes.y*dy);
	newCPos.z = adventureInt->position.z;
	adventureInt->centerOn(newCPos);
}

void CMinimap::hover (bool on)
{
	//Hoverable::hover(on);
	if (on)
		adventureInt->statusbar.print(statusbarTxt);
	else if (adventureInt->statusbar.current==statusbarTxt)
		adventureInt->statusbar.clear();
}

void CMinimap::mouseMoved (const SDL_MouseMotionEvent & sEvent)
{
	if (pressedL)
	{
		clickLeft(true, true);
	}
}
void CMinimap::activate()
{
	activateLClick();
	activateRClick();
	activateHover();
	if (pressedL)
		activateMouseMove();
}

void CMinimap::deactivate()
{
	if (pressedL)
		deactivateMouseMove();
	deactivateLClick();
	deactivateRClick();
	deactivateHover();
}

void CMinimap::showTile(const int3 &pos)
{
	int3 mapSizes = LOCPLINT->cb->getMapSize();
	//drawing terrain
	int mw = map[0]->w, mh = map[0]->h;
	double wo = ((double)mw)/mapSizes.x, ho = ((double)mh)/mapSizes.y;
	for (int ii=0; ii<wo; ii++)
	{
		for (int jj=0; jj<ho; jj++)
		{
			if ((pos.x*wo+ii<this->pos.w) && (pos.y*ho+jj<this->pos.h))
				CSDL_Ext::SDL_PutPixelWithoutRefresh(FoW[pos.z],pos.x*wo+ii,pos.y*ho+jj,0,0,0,0);

			const TerrainTile * tile = LOCPLINT->cb->getTileInfo(pos);
			if(tile)
			{
				if (tile->blocked && (!tile->visitable))
					SDL_PutPixelWithoutRefresh(map[pos.z], pos.x*wo+ii, pos.y*ho+jj, colorsBlocked[tile->tertype].r, colorsBlocked[tile->tertype].g, colorsBlocked[tile->tertype].b);
				else SDL_PutPixelWithoutRefresh(map[pos.z], pos.x*wo+ii, pos.y*ho+jj, colors[tile->tertype].r, colors[tile->tertype].g, colors[tile->tertype].b);
			}
		}
	}
	//drawing flaggable objects
	int woShifted = wo, hoShifted = ho; //for better minimap rendering on L-sized maps
	std::vector < const CGObjectInstance * > oo = LOCPLINT->cb->getFlaggableObjects(pos);
	for(size_t v=0; v<oo.size(); ++v)
	{
		if(!dynamic_cast< const CGHeroInstance * >(oo[v])) //heroes have been printed
		{
			int3 maplgp ( (pos.x*mw)/mapSizes.x, (pos.y*mh)/mapSizes.y, pos.z );
			if(((int)wo) * mapSizes.x != mw   &&   pos.x+1 < mapSizes.x)//minimap size in X is not multiple of map size in X
				 
			{
				std::vector < const CGObjectInstance * > op1x = LOCPLINT->cb->getFlaggableObjects(int3(pos.x+1, pos.y, pos.z));
				if(op1x.size()!=0)
				{
					woShifted = wo + 1;
				}
				else
				{
					woShifted = wo;
				}
			}
			if(((int)ho) * mapSizes.y != mh   &&   pos.y+1 < mapSizes.y) //minimap size in Y is not multiple of map size in Y
			{
				std::vector < const CGObjectInstance * > op1y = LOCPLINT->cb->getFlaggableObjects(int3(pos.x, pos.y+1, pos.z));
				if(op1y.size()!=0)
				{
					hoShifted = ho + 1;
				}
				else
				{
					hoShifted = ho;
				}
			}

			for (int ii=0; ii<woShifted; ii++) //rendering flaggable objects
			{
				for (int jj=0; jj<hoShifted; jj++)
				{
					if(oo[v]->tempOwner == 255)
						SDL_PutPixelWithoutRefresh(flObjs[pos.z],maplgp.x+ii,maplgp.y+jj,graphics->neutralColor->b,
							graphics->neutralColor->g,graphics->neutralColor->r);
					else
						SDL_PutPixelWithoutRefresh(flObjs[pos.z],maplgp.x+ii,maplgp.y+jj,graphics->playerColors[oo[v]->getOwner()].b,
							graphics->playerColors[oo[v]->getOwner()].g,graphics->playerColors[oo[v]->getOwner()].r);
				}
			}
		}
	}
	//flaggable objects drawn
}

void CMinimap::showVisibleTiles(int level)
{
	int3 mapSizes = LOCPLINT->cb->getMapSize();
	for(int d=0; d<CGI->mh->map->twoLevel+1; ++d)
	{
		if(level>=0 && d!=level)
			continue;
		for(int x=0; x<mapSizes.x; ++x)
		{
			for(int y=0; y<mapSizes.y; ++y)
			{
				if(LOCPLINT->cb->isVisible(int3(x, y, d)))
				{
					showTile(int3(x, y, d));
				}
			}
		}
	}
}

void CMinimap::hideTile(const int3 &pos)
{
	int3 mapSizes = LOCPLINT->cb->getMapSize();
	//drawing terrain
	int mw = map[0]->w, mh = map[0]->h;
	double wo = ((double)mw)/mapSizes.x, ho = ((double)mh)/mapSizes.y;
	for (int ii=0; ii<wo; ii++)
	{
		for (int jj=0; jj<ho; jj++)
		{
			if ((pos.x*wo+ii<this->pos.w) && (pos.y*ho+jj<this->pos.h))
				CSDL_Ext::SDL_PutPixelWithoutRefresh(FoW[pos.z],pos.x*wo+ii,pos.y*ho+jj,0,0,0,0);
		}
	}
}

void CMinimap::show( SDL_Surface * to )
{

}

CTerrainRect::CTerrainRect()
	:curHoveredTile(-1,-1,-1), currentPath(NULL)
{
	tilesw=(ADVOPT.advmapW+31)/32;
	tilesh=(ADVOPT.advmapH+31)/32;
	pos.x=ADVOPT.advmapX;
	pos.y=ADVOPT.advmapY;
	pos.w=ADVOPT.advmapW;
	pos.h=ADVOPT.advmapH;
	moveX = moveY = 0;
}
CTerrainRect::~CTerrainRect()
{
}
void CTerrainRect::activate()
{
	activateLClick();
	activateRClick();
	activateHover();
	activateMouseMove();
}
void CTerrainRect::deactivate()
{
	deactivateLClick();
	deactivateRClick();
	deactivateHover();
	deactivateMouseMove();
	curHoveredTile = int3(-1,-1,-1); //we lost info about hovered tile when disabling
}
void CTerrainRect::clickLeft(tribool down, bool previousState)
{
	if ((down==false) || indeterminate(down))
		return;

	int3 mp = whichTileIsIt();
	if (mp.x<0 || mp.y<0 || mp.x >= LOCPLINT->cb->getMapSize().x || mp.y >= LOCPLINT->cb->getMapSize().y)
		return;

	adventureInt->tileLClicked(mp);
}
void CTerrainRect::clickRight(tribool down, bool previousState)
{
	int3 mp = whichTileIsIt();

	if (!CGI->mh->map->isInTheMap(mp) || down != true)
		return;

	adventureInt->tileRClicked(mp);
}
void CTerrainRect::mouseMoved (const SDL_MouseMotionEvent & sEvent)
{
	int3 tHovered = whichTileIsIt(sEvent.x,sEvent.y);
	int3 pom = adventureInt->verifyPos(tHovered);

	if(tHovered != pom) //tile outside the map
	{
		CCS->curh->changeGraphic(0, 0);
		return;
	}

	if (pom != curHoveredTile)
		curHoveredTile=pom;
	else
		return;

	adventureInt->tileHovered(curHoveredTile);
}
void CTerrainRect::hover(bool on)
{
	if (!on)
	{
		adventureInt->statusbar.clear();
		CCS->curh->changeGraphic(0,0);
	}
	//Hoverable::hover(on);
}
void CTerrainRect::showPath(const SDL_Rect * extRect, SDL_Surface * to)
{
	for (size_t i=0; i < currentPath->nodes.size()-1; ++i)
	{
		int pn=-1;//number of picture
		if (i==0) //last tile
		{
			int x = 32*(currentPath->nodes[i].coord.x-adventureInt->position.x)+CGI->mh->offsetX + pos.x,
				y = 32*(currentPath->nodes[i].coord.y-adventureInt->position.y)+CGI->mh->offsetY + pos.y;
			if (x<0 || y<0 || x>pos.w || y>pos.h)
				continue;
			pn=0;
		}
		else
		{
			/*
			 * notation of arrow direction:
			 * 1 2 3
			 * 4 5 6
			 * 7 8 9
			 * ie. 157 means an arrow from left upper tile to left bottom tile through 5 (all arrows go through 5 in this notation)
			*/
			std::vector<CGPathNode> & cv = currentPath->nodes;
			if (cv[i+1].coord.x == cv[i].coord.x-1 && cv[i+1].coord.y == cv[i].coord.y-1) //15x
			{
				if(cv[i-1].coord.x == cv[i].coord.x+1 && cv[i-1].coord.y == cv[i].coord.y) //156
				{
					pn = 3;
				}
				else if(cv[i-1].coord.x == cv[i].coord.x+1 && cv[i-1].coord.y == cv[i].coord.y+1) //159
				{
					pn = 12;
				}
				else if(cv[i-1].coord.x == cv[i].coord.x && cv[i-1].coord.y == cv[i].coord.y+1) //158
				{
					pn = 21;
				}
				else if(cv[i-1].coord.x == cv[i].coord.x-1 && cv[i-1].coord.y == cv[i].coord.y+1) //157
				{
					pn = 22;
				}
				else if(cv[i-1].coord.x == cv[i].coord.x+1 && cv[i-1].coord.y == cv[i].coord.y-1) //153
				{
					pn = 2;
				}
				else if(cv[i-1].coord.x == cv[i].coord.x-1 && cv[i-1].coord.y == cv[i].coord.y) //154
				{
					pn = 23;
				}
				else if(cv[i-1].coord.x == cv[i].coord.x && cv[i-1].coord.y == cv[i].coord.y-1) //152
				{
					pn = 1;
				}
			}
			else if (cv[i+1].coord.x == cv[i].coord.x && cv[i+1].coord.y == cv[i].coord.y-1) //25x
			{
				if(cv[i-1].coord.x == cv[i].coord.x+1 && cv[i-1].coord.y == cv[i].coord.y-1) //253
				{
					pn = 2;
				}
				if(cv[i-1].coord.x == cv[i].coord.x+1 && cv[i-1].coord.y == cv[i].coord.y) //256
				{
					pn = 3;
				}
				if(cv[i-1].coord.x == cv[i].coord.x+1 && cv[i-1].coord.y == cv[i].coord.y+1) //259
				{
					pn = 4;
				}
				else if(cv[i-1].coord.x == cv[i].coord.x && cv[i-1].coord.y == cv[i].coord.y+1) //258
				{
					pn = 13;
				}
				else if(cv[i-1].coord.x == cv[i].coord.x-1 && cv[i-1].coord.y == cv[i].coord.y+1) //257
				{
					pn = 22;
				}
				if(cv[i-1].coord.x == cv[i].coord.x-1 && cv[i-1].coord.y == cv[i].coord.y) //254
				{
					pn = 23;
				}
				if(cv[i-1].coord.x == cv[i].coord.x-1 && cv[i-1].coord.y == cv[i].coord.y-1) //251
				{
					pn = 24;
				}
			}
			else if (cv[i+1].coord.x == cv[i].coord.x+1 && cv[i+1].coord.y == cv[i].coord.y-1) //35x
			{
				if(cv[i-1].coord.x == cv[i].coord.x && cv[i-1].coord.y == cv[i].coord.y+1) //358
				{
					pn = 5;
				}
				else if(cv[i-1].coord.x == cv[i].coord.x-1 && cv[i-1].coord.y == cv[i].coord.y+1) //357
				{
					pn = 14;
				}
				else if(cv[i-1].coord.x == cv[i].coord.x-1 && cv[i-1].coord.y == cv[i].coord.y) //354
				{
					pn = 23;
				}
				else if(cv[i-1].coord.x == cv[i].coord.x-1 && cv[i-1].coord.y == cv[i].coord.y-1) //351
				{
					pn = 24;
				}
				else if(cv[i-1].coord.x == cv[i].coord.x+1 && cv[i-1].coord.y == cv[i].coord.y+1) //359
				{
					pn = 4;
				}
				else if(cv[i-1].coord.x == cv[i].coord.x+1 && cv[i-1].coord.y == cv[i].coord.y) //356
				{
					pn = 3;
				}
				else if(cv[i-1].coord.x == cv[i].coord.x && cv[i-1].coord.y == cv[i].coord.y-1) //352
				{
					pn = 17;
				}
			}
			else if (cv[i+1].coord.x == cv[i].coord.x+1 && cv[i+1].coord.y == cv[i].coord.y) //65x
			{
				if(cv[i-1].coord.x == cv[i].coord.x-1 && cv[i-1].coord.y == cv[i].coord.y+1) //657
				{
					pn = 6;
				}
				else if(cv[i-1].coord.x == cv[i].coord.x-1 && cv[i-1].coord.y == cv[i].coord.y) //654
				{
					pn = 15;
				}
				else if(cv[i-1].coord.x == cv[i].coord.x-1 && cv[i-1].coord.y == cv[i].coord.y-1) //651
				{
					pn = 24;
				}
				else if(cv[i-1].coord.x == cv[i].coord.x && cv[i-1].coord.y == cv[i].coord.y-1) //652
				{
					pn = 17;
				}
				else if(cv[i-1].coord.x == cv[i].coord.x && cv[i-1].coord.y == cv[i].coord.y+1) //658
				{
					pn = 5;
				}
				else if(cv[i-1].coord.x == cv[i].coord.x+1 && cv[i-1].coord.y == cv[i].coord.y-1) //653
				{
					pn = 18;
				}
				else if(cv[i-1].coord.x == cv[i].coord.x+1 && cv[i-1].coord.y == cv[i].coord.y+1) //659
				{
					pn = 4;
				}
			}
			else if (cv[i+1].coord.x == cv[i].coord.x+1 && cv[i+1].coord.y == cv[i].coord.y+1) //95x
			{
				if(cv[i-1].coord.x == cv[i].coord.x-1 && cv[i-1].coord.y == cv[i].coord.y) //954
				{
					pn = 7;
				}
				else if(cv[i-1].coord.x == cv[i].coord.x-1 && cv[i-1].coord.y == cv[i].coord.y-1) //951
				{
					pn = 16;
				}
				else if(cv[i-1].coord.x == cv[i].coord.x && cv[i-1].coord.y == cv[i].coord.y-1) //952
				{
					pn = 17;
				}
				else if(cv[i-1].coord.x == cv[i].coord.x-1 && cv[i-1].coord.y == cv[i].coord.y+1) //957
				{
					pn = 6;
				}
				else if(cv[i-1].coord.x == cv[i].coord.x+1 && cv[i-1].coord.y == cv[i].coord.y-1) //953
				{
					pn = 18;
				}
				else if(cv[i-1].coord.x == cv[i].coord.x+1 && cv[i-1].coord.y == cv[i].coord.y) //956
				{
					pn = 19;
				}
				else if(cv[i-1].coord.x == cv[i].coord.x && cv[i-1].coord.y == cv[i].coord.y+1) //958
				{
					pn = 5;
				}
			}
			else if (cv[i+1].coord.x == cv[i].coord.x && cv[i+1].coord.y == cv[i].coord.y+1) //85x
			{
				if(cv[i-1].coord.x == cv[i].coord.x-1 && cv[i-1].coord.y == cv[i].coord.y+1) //857
				{
					pn = 6;
				}
				if(cv[i-1].coord.x == cv[i].coord.x-1 && cv[i-1].coord.y == cv[i].coord.y) //854
				{
					pn = 7;
				}
				if(cv[i-1].coord.x == cv[i].coord.x-1 && cv[i-1].coord.y == cv[i].coord.y-1) //851
				{
					pn = 8;
				}
				else if(cv[i-1].coord.x == cv[i].coord.x && cv[i-1].coord.y == cv[i].coord.y-1) //852
				{
					pn = 9;
				}
				else if(cv[i-1].coord.x == cv[i].coord.x+1 && cv[i-1].coord.y == cv[i].coord.y-1) //853
				{
					pn = 18;
				}
				else if(cv[i-1].coord.x == cv[i].coord.x+1 && cv[i-1].coord.y == cv[i].coord.y) //856
				{
					pn = 19;
				}
				else if(cv[i-1].coord.x == cv[i].coord.x+1 && cv[i-1].coord.y == cv[i].coord.y+1) //859
				{
					pn = 20;
				}
			}
			else if (cv[i+1].coord.x == cv[i].coord.x-1 && cv[i+1].coord.y == cv[i].coord.y+1) //75x
			{
				if(cv[i-1].coord.x == cv[i].coord.x && cv[i-1].coord.y == cv[i].coord.y-1) //752
				{
					pn = 1;
				}
				else if(cv[i-1].coord.x == cv[i].coord.x+1 && cv[i-1].coord.y == cv[i].coord.y-1) //753
				{
					pn = 10;
				}
				else if(cv[i-1].coord.x == cv[i].coord.x+1 && cv[i-1].coord.y == cv[i].coord.y) //756
				{
					pn = 19;
				}
				else if(cv[i-1].coord.x == cv[i].coord.x-1 && cv[i-1].coord.y == cv[i].coord.y-1) //751
				{
					pn = 8;
				}
				else if(cv[i-1].coord.x == cv[i].coord.x+1 && cv[i-1].coord.y == cv[i].coord.y+1) //759
				{
					pn = 20;
				}
			}
			else if (cv[i+1].coord.x == cv[i].coord.x-1 && cv[i+1].coord.y == cv[i].coord.y) //45x
			{
				if(cv[i-1].coord.x == cv[i].coord.x+1 && cv[i-1].coord.y == cv[i].coord.y-1) //453
				{
					pn = 2;
				}
				else if(cv[i-1].coord.x == cv[i].coord.x+1 && cv[i-1].coord.y == cv[i].coord.y) //456
				{
					pn = 11;
				}
				else if(cv[i-1].coord.x == cv[i].coord.x+1 && cv[i-1].coord.y == cv[i].coord.y+1) //459
				{
					pn = 20;
				}
				else if(cv[i-1].coord.x == cv[i].coord.x && cv[i-1].coord.y == cv[i].coord.y-1) //452
				{
					pn = 1;
				}
				else if(cv[i-1].coord.x == cv[i].coord.x && cv[i-1].coord.y == cv[i].coord.y+1) //456
				{
					pn = 21;
				}
				else if(cv[i-1].coord.x == cv[i].coord.x-1 && cv[i-1].coord.y == cv[i].coord.y-1) //451
				{
					pn = 8;
				}
				else if(cv[i-1].coord.x == cv[i].coord.x-1 && cv[i-1].coord.y == cv[i].coord.y+1) //457
				{
					pn = 22;
				}
			}

		}
		if (currentPath->nodes[i].turns)
			pn+=25;
		if (pn>=0)
		{
			CDefEssential * arrows = graphics->heroMoveArrows;
			int x = 32*(currentPath->nodes[i].coord.x-adventureInt->position.x)+CGI->mh->offsetX + pos.x,
				y = 32*(currentPath->nodes[i].coord.y-adventureInt->position.y)+CGI->mh->offsetY + pos.y;
			if (x<0 || y<0 || x>pos.w || y>pos.h)
				continue;
			int hvx = (x+arrows->ourImages[pn].bitmap->w)-(pos.x+pos.w),
				hvy = (y+arrows->ourImages[pn].bitmap->h)-(pos.y+pos.h);

			SDL_Rect prevClip;
			SDL_GetClipRect(to, &prevClip);
			SDL_SetClipRect(to, extRect); //preventing blitting outside of that rect

			if(ADVOPT.smoothMove) //version for smooth hero move, with pos shifts
			{
				if (hvx<0 && hvy<0)
				{
					Rect dstRect = genRect(32, 32, x + moveX, y + moveY);
					CSDL_Ext::blit8bppAlphaTo24bpp(arrows->ourImages[pn].bitmap, NULL, to, &dstRect);
				}
				else if(hvx<0)
				{
					Rect srcRect = genRect(arrows->ourImages[pn].bitmap->h-hvy, arrows->ourImages[pn].bitmap->w, 0, 0);
					Rect dstRect = genRect(arrows->ourImages[pn].bitmap->h-hvy, arrows->ourImages[pn].bitmap->w, x + moveX, y + moveY);
					CSDL_Ext::blit8bppAlphaTo24bpp(arrows->ourImages[pn].bitmap, &srcRect, to, &dstRect);
				}
				else if (hvy<0)
				{
					Rect srcRect = genRect(arrows->ourImages[pn].bitmap->h, arrows->ourImages[pn].bitmap->w-hvx, 0, 0);
					Rect dstRect = genRect(arrows->ourImages[pn].bitmap->h, arrows->ourImages[pn].bitmap->w-hvx, x + moveX, y + moveY);
					CSDL_Ext::blit8bppAlphaTo24bpp(arrows->ourImages[pn].bitmap, &srcRect, to, &dstRect);
				}
				else
				{
					Rect srcRect = genRect(arrows->ourImages[pn].bitmap->h-hvy, arrows->ourImages[pn].bitmap->w-hvx, 0, 0);
					Rect dstRect = genRect(arrows->ourImages[pn].bitmap->h-hvy, arrows->ourImages[pn].bitmap->w-hvx, x + moveX, y + moveY);
					CSDL_Ext::blit8bppAlphaTo24bpp(arrows->ourImages[pn].bitmap, &srcRect, to, &dstRect);
				}
			}
			else //standard version
			{
				if (hvx<0 && hvy<0)
				{
					Rect dstRect = genRect(32, 32, x, y);
					CSDL_Ext::blit8bppAlphaTo24bpp(arrows->ourImages[pn].bitmap, NULL, to, &dstRect);
				}
				else if(hvx<0)
				{
					Rect srcRect = genRect(arrows->ourImages[pn].bitmap->h-hvy, arrows->ourImages[pn].bitmap->w, 0, 0);
					Rect dstRect = genRect(arrows->ourImages[pn].bitmap->h-hvy, arrows->ourImages[pn].bitmap->w, x, y);
					CSDL_Ext::blit8bppAlphaTo24bpp(arrows->ourImages[pn].bitmap, &srcRect, to, &dstRect);
				}
				else if (hvy<0)
				{
					Rect srcRect = genRect(arrows->ourImages[pn].bitmap->h, arrows->ourImages[pn].bitmap->w-hvx, 0, 0);
					Rect dstRect = genRect(arrows->ourImages[pn].bitmap->h, arrows->ourImages[pn].bitmap->w-hvx, x, y);
					CSDL_Ext::blit8bppAlphaTo24bpp(arrows->ourImages[pn].bitmap, &srcRect, to, &dstRect);
				}
				else
				{
					Rect srcRect = genRect(arrows->ourImages[pn].bitmap->h-hvy, arrows->ourImages[pn].bitmap->w-hvx, 0, 0);
					Rect dstRect = genRect(arrows->ourImages[pn].bitmap->h-hvy, arrows->ourImages[pn].bitmap->w-hvx, x, y);
					CSDL_Ext::blit8bppAlphaTo24bpp(arrows->ourImages[pn].bitmap, &srcRect, to, &dstRect);
				}
			}
			SDL_SetClipRect(to, &prevClip);

		}
	} //for (int i=0;i<currentPath->nodes.size()-1;i++)
}
void CTerrainRect::show(SDL_Surface * to)
{
	if(ADVOPT.smoothMove)
		CGI->mh->terrainRect
			(adventureInt->position, adventureInt->anim,
			 &LOCPLINT->cb->getVisibilityMap(), true, adventureInt->heroAnim,
			 to, &pos, moveX, moveY, false, int3());
	else
		CGI->mh->terrainRect
			(adventureInt->position, adventureInt->anim,
			 &LOCPLINT->cb->getVisibilityMap(), true, adventureInt->heroAnim,
			 to, &pos, 0, 0, false, int3());
	
	//SDL_BlitSurface(teren,&genRect(pos.h,pos.w,0,0),screen,&genRect(547,594,7,6));
	//SDL_FreeSurface(teren);
	if (currentPath && adventureInt->position.z==currentPath->startPos().z) //drawing path
	{
		showPath(&pos, to);
	}
}

int3 CTerrainRect::whichTileIsIt(const int & x, const int & y)
{
	int3 ret;
	ret.x = adventureInt->position.x + ((GH.current->motion.x-CGI->mh->offsetX-pos.x)/32);
	ret.y = adventureInt->position.y + ((GH.current->motion.y-CGI->mh->offsetY-pos.y)/32);
	ret.z = adventureInt->position.z;
	return ret;
}
int3 CTerrainRect::whichTileIsIt()
{
	return whichTileIsIt(GH.current->motion.x,GH.current->motion.y);
}

void CResDataBar::clickRight(tribool down, bool previousState)
{
}
void CResDataBar::activate()
{
	activateRClick();
}
void CResDataBar::deactivate()
{
	deactivateRClick();
}
CResDataBar::CResDataBar(const std::string &defname, int x, int y, int offx, int offy, int resdist, int datedist)
{
	bg = BitmapHandler::loadBitmap(defname);
	SDL_SetColorKey(bg,SDL_SRCCOLORKEY,SDL_MapRGB(bg->format,0,255,255));
	graphics->blueToPlayersAdv(bg,LOCPLINT->playerID);
	pos = genRect(bg->h, bg->w, pos.x+x, pos.y+y);

	txtpos.resize(8);
	for (int i = 0; i < 8 ; i++)
	{
		txtpos[i].first = pos.x + offx + resdist*i;
		txtpos[i].second = pos.y + offy;
	}
	txtpos[7].first = txtpos[6].first + datedist;
	datetext =  CGI->generaltexth->allTexts[62]+": %s, " + CGI->generaltexth->allTexts[63] 
	+ ": %s, " + CGI->generaltexth->allTexts[64] + ": %s";
}
CResDataBar::CResDataBar()
{
	bg = BitmapHandler::loadBitmap(ADVOPT.resdatabarG);
	SDL_SetColorKey(bg,SDL_SRCCOLORKEY,SDL_MapRGB(bg->format,0,255,255));
	graphics->blueToPlayersAdv(bg,LOCPLINT->playerID);
	pos = genRect(bg->h,bg->w,ADVOPT.resdatabarX,ADVOPT.resdatabarY);

	txtpos.resize(8);
	for (int i = 0; i < 8 ; i++)
	{
		txtpos[i].first = pos.x + ADVOPT.resOffsetX + ADVOPT.resDist*i;
		txtpos[i].second = pos.y + ADVOPT.resOffsetY;
	}
	txtpos[7].first = txtpos[6].first + ADVOPT.resDateDist;
	datetext =  CGI->generaltexth->allTexts[62]+": %s, " + CGI->generaltexth->allTexts[63] 
				+ ": %s, " + CGI->generaltexth->allTexts[64] + ": %s";
}

CResDataBar::~CResDataBar()
{
	SDL_FreeSurface(bg);
}
void CResDataBar::draw(SDL_Surface * to)
{
	blitAt(bg,pos.x,pos.y,to);
	char * buf = new char[15];
	for (int i=0;i<7;i++)
	{
		SDL_itoa(LOCPLINT->cb->getResourceAmount(i),buf,10);
		printAt(buf,txtpos[i].first,txtpos[i].second,FONT_SMALL,zwykly,to);
	}
	std::vector<std::string> temp;
	SDL_itoa(LOCPLINT->cb->getDate(3),buf,10); temp+=std::string(buf);
	SDL_itoa(LOCPLINT->cb->getDate(2),buf,10); temp+=std::string(buf);
	SDL_itoa(LOCPLINT->cb->getDate(1),buf,10); temp+=std::string(buf);
	printAt(processStr(datetext,temp),txtpos[7].first,txtpos[7].second,FONT_SMALL,zwykly,to);
	temp.clear();
	//updateRect(&pos,screen);
	delete[] buf;
}

void CResDataBar::show( SDL_Surface * to )
{

}

void CResDataBar::showAll( SDL_Surface * to )
{
	draw(to);
}


CInfoBar::CInfoBar()
{
	toNextTick = mode = pom = -1;
	pos.x=ADVOPT.infoboxX;
	pos.y=ADVOPT.infoboxY;
	pos.w=194;
	pos.h=186;
	day = CDefHandler::giveDef("NEWDAY.DEF");
	week1 = CDefHandler::giveDef("NEWWEEK1.DEF");
	week2 = CDefHandler::giveDef("NEWWEEK2.DEF");
	week3 = CDefHandler::giveDef("NEWWEEK3.DEF");
	week4 = CDefHandler::giveDef("NEWWEEK4.DEF");
	selInfoWin = NULL;
}
CInfoBar::~CInfoBar()
{
	delete day;
	delete week1;
	delete week2;
	delete week3;
	delete week4;

	if(selInfoWin)
		SDL_FreeSurface(selInfoWin);
}

void CInfoBar::showAll(SDL_Surface * to)
{
	if ((mode>=0) && mode<5)
	{
		blitAnim(mode);
		return;
	}
	else if (mode==5)
	{
		mode = -1;
	}

	if(selInfoWin)
	{
		blitAt(selInfoWin, pos.x, pos.y, to);
	}
}

CDefHandler * CInfoBar::getAnim(int mode)
{
	switch(mode)
	{
	case 0:
		return day;
	case 1:
		return week1;
	case 2:
		return week2;
	case 3:
		return week3;
	case 4:
		return week4;
	default:
		return NULL;
	}
}

void CInfoBar::blitAnim(int mode)//0 - day, 1 - week
{
	CDefHandler * anim = NULL;
	std::ostringstream txt;
	anim = getAnim(mode);
	if(mode) //new week animation
	{
		txt << CGI->generaltexth->allTexts[63] << " " << LOCPLINT->cb->getDate(2);
	}
	else //new day
	{
		txt << CGI->generaltexth->allTexts[64] << " " << LOCPLINT->cb->getDate(1);
	}
	blitAt(anim->ourImages[pom].bitmap,pos.x+9,pos.y+10);
	printAtMiddle(txt.str(),pos.x+95,pos.y+31,FONT_MEDIUM,zwykly);
	if (pom == anim->ourImages.size()-1)
		toNextTick+=750;
}

void CInfoBar::newDay(int Day)
{
	if(LOCPLINT->cb->getDate(1) != 1)
	{
		mode = 0; //showing day
	}
	else
	{
		switch(LOCPLINT->cb->getDate(2))
		{
		case 1:
			mode = 1;
			break;
		case 2:
			mode = 2;
			break;
		case 3:
			mode = 3;
			break;
		case 4:
			mode = 4;
			break;
		default:
			mode = -1;
			break;
		}
	}
	pom = 0;
	if(!(active & TIME))
		activateTimer();

	toNextTick = 500;
	blitAnim(mode);
}

void CInfoBar::showComp(SComponent * comp, int time)
{
	if(comp->type != SComponent::hero)
	{
		curSel = NULL;
	}

	SDL_Surface * b = BitmapHandler::loadBitmap("ADSTATOT.bmp");
	blitAt(b,pos.x+8,pos.y+11);
	blitAt(comp->getImg(),pos.x+52,pos.y+54);
	printAtMiddle(comp->subtitle,pos.x+91,pos.y+158,FONT_SMALL,zwykly);
	printAtMiddleWB(comp->description,pos.x+94,pos.y+31,FONT_SMALL,26,zwykly);
	SDL_FreeSurface(b);
	if(!(active & TIME))
		activateTimer();
	mode = 6;
	toNextTick = time;
}

void CInfoBar::tick()
{
	if(mode >= 0  &&  mode < 5)
	{
		pom++;
		if (pom >= getAnim(mode)->ourImages.size())
		{
			deactivateTimer();
			toNextTick = -1;
			mode = 5;
			showAll(screen2);
			return;
		}
		toNextTick = 150;
		blitAnim(mode);
	}
	else if(mode == 6)
	{
		deactivateTimer();
		toNextTick = -1;
		mode = 5;
		showAll(screen2);
	}
}

void CInfoBar::show( SDL_Surface * to )
{

}

void CInfoBar::activate()
{
	//CIntObject::activate();
}

void CInfoBar::deactivate()
{
	//CIntObject::deactivate();
	if(active & TIME)
		deactivateTimer();
}

void CInfoBar::updateSelection(const CGObjectInstance *obj)
{
	if(obj->ID == HEROI_TYPE)
		curSel = static_cast<const CGHeroInstance*>(obj);
	else
		curSel = NULL;
	if(selInfoWin)
		SDL_FreeSurface(selInfoWin);
	selInfoWin = LOCPLINT->infoWin(obj);
}

CAdvMapInt::CAdvMapInt()
:statusbar(ADVOPT.statusbarX,ADVOPT.statusbarY,ADVOPT.statusbarG),
kingOverview(CGI->generaltexth->zelp[293].first,CGI->generaltexth->zelp[293].second,
			 boost::bind(&CAdvMapInt::fshowOverview,this),&ADVOPT.kingOverview, SDLK_k),

underground(CGI->generaltexth->zelp[294].first,CGI->generaltexth->zelp[294].second,
			boost::bind(&CAdvMapInt::fswitchLevel,this),&ADVOPT.underground, SDLK_u),

questlog(CGI->generaltexth->zelp[295].first,CGI->generaltexth->zelp[295].second,
		 boost::bind(&CAdvMapInt::fshowQuestlog,this),&ADVOPT.questlog, SDLK_q),

sleepWake(CGI->generaltexth->zelp[296].first,CGI->generaltexth->zelp[296].second,
		  boost::bind(&CAdvMapInt::fsleepWake,this), &ADVOPT.sleepWake, SDLK_w),

moveHero(CGI->generaltexth->zelp[297].first,CGI->generaltexth->zelp[297].second,
		  boost::bind(&CAdvMapInt::fmoveHero,this), &ADVOPT.moveHero, SDLK_m),

spellbook(CGI->generaltexth->zelp[298].first,CGI->generaltexth->zelp[298].second,
		  boost::bind(&CAdvMapInt::fshowSpellbok,this), &ADVOPT.spellbook, SDLK_c),

advOptions(CGI->generaltexth->zelp[299].first,CGI->generaltexth->zelp[299].second,
		  boost::bind(&CAdvMapInt::fadventureOPtions,this), &ADVOPT.advOptions, SDLK_a),

sysOptions(CGI->generaltexth->zelp[300].first,CGI->generaltexth->zelp[300].second,
		  boost::bind(&CAdvMapInt::fsystemOptions,this), &ADVOPT.sysOptions, SDLK_o),

nextHero(CGI->generaltexth->zelp[301].first,CGI->generaltexth->zelp[301].second,
		  boost::bind(&CAdvMapInt::fnextHero,this), &ADVOPT.nextHero, SDLK_h),

endTurn(CGI->generaltexth->zelp[302].first,CGI->generaltexth->zelp[302].second,
		  boost::bind(&CAdvMapInt::fendTurn,this), &ADVOPT.endTurn, SDLK_e),

heroList(ADVOPT.hlistSize),
townList(ADVOPT.tlistSize,ADVOPT.tlistX,ADVOPT.tlistY,ADVOPT.tlistAU,ADVOPT.tlistAD)//(5,&genRect(192,48,747,196),747,196,747,372),
{
	spellBeingCasted = NULL;
	pos.x = pos.y = 0;
	pos.w = screen->w;
	pos.h = screen->h;
	selection = NULL;
	townList.fun = boost::bind(&CAdvMapInt::selectionChanged,this);
	adventureInt=this;
	bg = BitmapHandler::loadBitmap(ADVOPT.mainGraphic);
	scrollingDir = 0;
	updateScreen  = false;
	anim=0;
	animValHitCount=0; //animation frame
	heroAnim=0;
	heroAnimValHitCount=0; // hero animation frame

	heroList.init();
	heroList.genList();
	//townList.init();
	//townList.genList();


	for (int g=0; g<ADVOPT.gemG.size(); ++g)
	{
		gems.push_back(CDefHandler::giveDef(ADVOPT.gemG[g]));
	}


	setPlayer(LOCPLINT->playerID);
}

CAdvMapInt::~CAdvMapInt()
{
	SDL_FreeSurface(bg);

	for(int i=0; i<gems.size(); i++)
		delete gems[i];
}

void CAdvMapInt::fshowOverview()
{
	GH.pushInt(new CKingdomInterface);
}
void CAdvMapInt::fswitchLevel()
{
	if(!CGI->mh->map->twoLevel)
		return;
	if (position.z)
	{
		position.z--;
		underground.setIndex(0,true);
		underground.showAll(screenBuf);
	}
	else
	{
		underground.setIndex(1,true);
		position.z++;
		underground.showAll(screenBuf);
	}
	updateScreen = true;
	minimap.draw(screenBuf);
}
void CAdvMapInt::fshowQuestlog()
{
}
void CAdvMapInt::fsleepWake()
{
}
void CAdvMapInt::fmoveHero()
{	
	const CGHeroInstance *h = curHero();
	if (!h || !terrain.currentPath)
		return;

	LOCPLINT->moveHero(h, *terrain.currentPath);
}

void CAdvMapInt::fshowSpellbok()
{
	if (!curHero()) //checking necessary values
		return;

	centerOn(selection);
	CSpellWindow * spellWindow = new CSpellWindow(genRect(595, 620, (conf.cc.resx - 620)/2, (conf.cc.resy - 595)/2), curHero(), LOCPLINT, false);
	GH.pushInt(spellWindow);
}

void CAdvMapInt::fadventureOPtions()
{
	GH.pushInt(new CAdventureOptions);
}

void CAdvMapInt::fsystemOptions()
{
	CSystemOptionsWindow * sysopWindow = new CSystemOptionsWindow(Rect::createCentered(487, 481), LOCPLINT);
	GH.pushInt(sysopWindow);
}

void CAdvMapInt::fnextHero()
{
	if(!LOCPLINT->wanderingHeroes.size()) //no wandering heroes
		return; 

	int start = heroList.selected;
	int i = start;

	do 
	{
		i++;
		if(i >= LOCPLINT->wanderingHeroes.size())
			i = 0;
	} while (!LOCPLINT->wanderingHeroes[i]->movement && i!=start);
	heroList.select(i);
}

void CAdvMapInt::fendTurn()
{
	if(LOCPLINT->cingconsole->active)
		LOCPLINT->cingconsole->deactivate();
	LOCPLINT->makingTurn = false;
	LOCPLINT->cb->endTurn();
}

void CAdvMapInt::activate()
{
	if(isActive())
	{
		tlog1 << "Error: advmapint already active...\n";
		return;
	}
	screenBuf = screen;
	GH.statusbar = &statusbar;
	activateMouseMove();

	kingOverview.activate();
	underground.activate();
	questlog.activate();
	sleepWake.activate();
	moveHero.activate();
	spellbook.activate();
	sysOptions.activate();
	advOptions.activate();
	nextHero.activate();
	endTurn.activate();

	minimap.activate();
	heroList.activate();
	townList.activate();
	terrain.activate();
	infoBar.activate();

	if(!LOCPLINT->cingconsole->active)
		LOCPLINT->cingconsole->activate();
	GH.fakeMouseMove(); //to restore the cursor
}
void CAdvMapInt::deactivate()
{
	deactivateMouseMove();
	scrollingDir = 0;

	CCS->curh->changeGraphic(0,0);
	kingOverview.deactivate();
	underground.deactivate();
	questlog.deactivate();
	sleepWake.deactivate();
	moveHero.deactivate();
	spellbook.deactivate();
	advOptions.deactivate();
	sysOptions.deactivate();
	nextHero.deactivate();
	endTurn.deactivate();
	minimap.deactivate();
	heroList.deactivate();
	townList.deactivate();
	terrain.deactivate();
	infoBar.deactivate();
	infoBar.mode=-1;

	if(LOCPLINT->cingconsole->active) //TODO
		LOCPLINT->cingconsole->deactivate();
}
void CAdvMapInt::showAll(SDL_Surface *to)
{
	blitAt(bg,0,0,to);

	if(state != INGAME)
		return;

	kingOverview.showAll(to);
	underground.showAll(to);
	questlog.showAll(to);
	sleepWake.showAll(to);
	moveHero.showAll(to);
	spellbook.showAll(to);
	advOptions.showAll(to);
	sysOptions.showAll(to);
	nextHero.showAll(to);
	endTurn.showAll(to);

	minimap.draw(to);
	heroList.draw(to);
	townList.draw(to);
	updateScreen = true;
	show(to);

	resdatabar.draw(to);

	statusbar.show(to);

	infoBar.showAll(to);
	LOCPLINT->cingconsole->show(to);
}
void CAdvMapInt::show(SDL_Surface *to)
{
	if(state != INGAME)
		return;

	++animValHitCount; //for animations
	if(animValHitCount == 8)
	{
		CGI->mh->updateWater();
		animValHitCount = 0;
		++anim;
		updateScreen = true;
	}
	++heroAnim;

	//if advmap needs updating AND (no dialog is shown OR ctrl is pressed)
	if((animValHitCount % (4/LOCPLINT->sysOpts.mapScrollingSpeed)) == 0 
		&&  (
			(GH.topInt() == this)
			|| SDL_GetKeyState(NULL)[SDLK_LCTRL] 
			|| SDL_GetKeyState(NULL)[SDLK_RCTRL])
	)
	{
		if( (scrollingDir & LEFT)   &&  (position.x>-CGI->mh->frameW) )
			position.x--;

		if( (scrollingDir & RIGHT)  &&  (position.x   <   CGI->mh->map->width - CGI->mh->tilesW + CGI->mh->frameW) )
			position.x++;

		if( (scrollingDir & UP)  &&  (position.y>-CGI->mh->frameH) )
			position.y--;

		if( (scrollingDir & DOWN)  &&  (position.y  <  CGI->mh->map->height - CGI->mh->tilesH + CGI->mh->frameH) )
			position.y++;

		if(scrollingDir)
		{
			updateScreen = true;
			updateMinimap=true;
		}
	}
	if(updateScreen)
	{
		terrain.show(to);
		for(int i=0;i<4;i++)
			blitAt(gems[i]->ourImages[LOCPLINT->playerID].bitmap,ADVOPT.gemX[i],ADVOPT.gemY[i],to);
		updateScreen=false;	
		LOCPLINT->cingconsole->show(to);
	}
	if (updateMinimap)
	{
		minimap.draw(to);
		updateMinimap=false;
	}
}

void CAdvMapInt::selectionChanged()
{
	const CGTownInstance *to = LOCPLINT->towns[townList.selected];
	select(to);
}
void CAdvMapInt::centerOn(int3 on)
{
	on.x -= CGI->mh->frameW;
	on.y -= CGI->mh->frameH;
	
	on = LOCPLINT->repairScreenPos(on);

	adventureInt->position = on;
	adventureInt->updateScreen=true;
	updateMinimap=true;
	underground.setIndex(on.z,true); //change underground switch button image 
	if(GH.topInt() == this)
		underground.redraw();
}

void CAdvMapInt::centerOn(const CGObjectInstance *obj)
{
	centerOn(obj->getSightCenter());
}

void CAdvMapInt::keyPressed(const SDL_KeyboardEvent & key)
{
	ui8 Dir = 0;
	int k = key.keysym.sym;
	const CGHeroInstance *h = curHero(); //selected hero
	const CGTownInstance *t = curTown(); //selected town


	switch(k)
	{
	case SDLK_i: 
		if(isActive())
			CAdventureOptions::showScenarioInfo();
		return;
	case SDLK_s: 
		if(isActive())
			GH.pushInt(new CSavingScreen(CPlayerInterface::howManyPeople > 1));
		return;
	case SDLK_d: 
		{
			if(h && isActive() && key.state == SDL_PRESSED)
				LOCPLINT->tryDiggging(h);
			return;
		}
	case SDLK_p: 
		if(isActive())
			LOCPLINT->showPuzzleMap();
		return;
	case SDLK_SPACE: //space - try to revisit current object with selected hero
		{
			if(!isActive()) 
				return;
			if(h && key.state == SDL_PRESSED)
			{
				LOCPLINT->pim->unlock();
				LOCPLINT->cb->moveHero(h,h->pos);
				LOCPLINT->pim->lock();
			}
		}
		return;
	case SDLK_RETURN:
		{
			if(!isActive() || !selection || key.state != SDL_PRESSED) 
				return;
			if(h)
				LOCPLINT->openHeroWindow(h);
			else if(t)
				LOCPLINT->openTownWindow(t);
			return;
		}
	case SDLK_ESCAPE:
		{
			if(isActive() || GH.topInt() != this || !spellBeingCasted || key.state != SDL_PRESSED) 
				return;

			leaveCastingMode();
			return;
		}
	case SDLK_t:
		{
			//act on key down if marketplace windows is not already opened
			if(key.state != SDL_PRESSED  || GH.topInt()->type & BLOCK_ADV_HOTKEYS) return;

			//check if we have any marketplace
			const CGTownInstance *townWithMarket = NULL;
			BOOST_FOREACH(const CGTownInstance *t, LOCPLINT->cb->getTownsInfo())
			{
				if(vstd::contains(t->builtBuildings, 14))
				{
					townWithMarket = t;
					break;
				}
			}

			if(townWithMarket) //if any town has marketplace, open window
				GH.pushInt(new CMarketplaceWindow(townWithMarket)); 
			else //if not - complain
				LOCPLINT->showInfoDialog("No available marketplace!", std::vector<SComponent*>(), soundBase::sound_todo);
			return;
		}
	default: 
		{
			static const int3 directions[] = {  int3(-1, +1, 0), int3(0, +1, 0), int3(+1, +1, 0),
												int3(-1, 0, 0),  int3(0, 0, 0),  int3(+1, 0, 0),
												int3(-1, -1, 0), int3(0, -1, 0), int3(+1, -1, 0) };

			//numpad arrow
			if(isArrowKey(SDLKey(k)))
			{
				switch(k)
				{
				case SDLK_UP: 
					Dir = UP;
					break;
				case SDLK_LEFT: 
					Dir = LEFT;
					break;
				case SDLK_RIGHT: 
					Dir = RIGHT;
					break;
				case SDLK_DOWN: 
					Dir = DOWN;
					break;
				}

				k = arrowToNum(SDLKey(k));
			}

			if(!isActive() || LOCPLINT->ctrlPressed())//ctrl makes arrow move screen, not hero
				break;

			k -= SDLK_KP0 + 1;
			if(k < 0 || k > 8 || key.state != SDL_PRESSED)
				return;

			if(!h) 
				break;

			if(k == 4)
			{
				centerOn(h);
				return;
			}

			int3 dir = directions[k];

			CGPath &path = LOCPLINT->paths[h];
			terrain.currentPath = &path;
			if(!LOCPLINT->cb->getPath2(h->getPosition(false) + dir, path))
			{
				terrain.currentPath = NULL;
				return;
			}

			if(!path.nodes[0].turns)
			{
				LOCPLINT->moveHero(h, path);
			}
		}

		return;
	}
	if(Dir && key.state == SDL_PRESSED //arrow is pressed
		&& LOCPLINT->ctrlPressed()
	)
		scrollingDir |= Dir;
	else
		scrollingDir &= ~Dir;
}
void CAdvMapInt::handleRightClick(std::string text, tribool down)
{
	if(down)
	{
		CRClickPopup::createAndPush(text);
	}
}
int3 CAdvMapInt::verifyPos(int3 ver)
{
	if (ver.x<0)
		ver.x=0;
	if (ver.y<0)
		ver.y=0;
	if (ver.z<0)
		ver.z=0;
	if (ver.x>=CGI->mh->sizes.x)
		ver.x=CGI->mh->sizes.x-1;
	if (ver.y>=CGI->mh->sizes.y)
		ver.y=CGI->mh->sizes.y-1;
	if (ver.z>=CGI->mh->sizes.z)
		ver.z=CGI->mh->sizes.z-1;
	return ver;
}

void CAdvMapInt::select(const CArmedInstance *sel, bool centerView /*= true*/)
{
	assert(sel);
	LOCPLINT->cb->setSelection(sel);
	selection = sel;
	if(centerView)
		centerOn(sel);

	terrain.currentPath = NULL;
	if(sel->ID==TOWNI_TYPE)
	{
		int pos = vstd::findPos(LOCPLINT->towns,sel);
		townList.selected = pos;
		townList.fixPos();
	}
	else //hero selected
	{
		const CGHeroInstance *h = static_cast<const CGHeroInstance*>(sel);

		if(LOCPLINT->getWHero(heroList.selected) != h)
		{
			heroList.selected = heroList.getPosOfHero(h);
			heroList.fixPos();
		}

		terrain.currentPath = LOCPLINT->getAndVerifyPath(h);
	}
	townList.draw(screen);
	heroList.draw(screen);
	infoBar.updateSelection(sel);
	infoBar.showAll(screen);
}

void CAdvMapInt::mouseMoved( const SDL_MouseMotionEvent & sEvent )
{
	//adventure map scrolling with mouse
	if(!SDL_GetKeyState(NULL)[SDLK_LCTRL]  &&  isActive())
	{
		if(sEvent.x<15)
		{
			scrollingDir |= LEFT;
		}
		else
		{
			scrollingDir &= ~LEFT;
		}
		if(sEvent.x>screen->w-15)
		{
			scrollingDir |= RIGHT;
		}
		else
		{
			scrollingDir &= ~RIGHT;
		}
		if(sEvent.y<15)
		{
			scrollingDir |= UP;
		}
		else
		{
			scrollingDir &= ~UP;
		}
		if(sEvent.y>screen->h-15)
		{
			scrollingDir |= DOWN;
		}
		else
		{
			scrollingDir &= ~DOWN;
		}
	}
}

bool CAdvMapInt::isActive()
{
	return active & ~CIntObject::KEYBOARD;
}

void CAdvMapInt::startHotSeatWait(int Player)
{
	state = WAITING;
}

void CAdvMapInt::setPlayer(int Player)
{
	player = Player;
	graphics->blueToPlayersAdv(bg,player);

	kingOverview.setPlayerColor(player);
	underground.setPlayerColor(player);
	questlog.setPlayerColor(player);
	sleepWake.setPlayerColor(player);
	moveHero.setPlayerColor(player);
	spellbook.setPlayerColor(player);
	sysOptions.setPlayerColor(player);
	advOptions.setPlayerColor(player);
	nextHero.setPlayerColor(player);
	endTurn.setPlayerColor(player);
	graphics->blueToPlayersAdv(resdatabar.bg,player);

	//heroList.updateHList();
	//townList.genList();
}

void CAdvMapInt::startTurn()
{
	state = INGAME;
}

void CAdvMapInt::tileLClicked(const int3 &mp)
{
	if(!LOCPLINT->cb->isVisible(mp))
		return;

	std::vector < const CGObjectInstance * > bobjs = LOCPLINT->cb->getBlockingObjs(mp),  //blocking objects at tile
		vobjs = LOCPLINT->cb->getVisitableObjs(mp); //visitable objects
	const TerrainTile *tile = LOCPLINT->cb->getTileInfo(mp);
	const CGObjectInstance *topBlocking = bobjs.size() ? bobjs.back() : NULL;


	int3 selPos = selection->getSightCenter();
	if(spellBeingCasted && isInScreenRange(selPos, mp))
	{
		const TerrainTile *heroTile = LOCPLINT->cb->getTileInfo(selPos);

		switch(spellBeingCasted->id)
		{
		case Spells::SCUTTLE_BOAT: //Scuttle Boat 
			if(topBlocking && topBlocking->ID == 8)
				leaveCastingMode(true, mp);
			break;
		case Spells::DIMENSION_DOOR:
			if(!tile || tile->isClear(heroTile))
				leaveCastingMode(true, mp);
			break;
		}
		return;
	}
	//check if we can select this object
	bool canSelect = topBlocking && topBlocking->ID == HEROI_TYPE && topBlocking->tempOwner == LOCPLINT->playerID;
	canSelect |= topBlocking && topBlocking->ID == TOWNI_TYPE && LOCPLINT->cb->getPlayerRelations(LOCPLINT->playerID, topBlocking->tempOwner);

	if (selection->ID != HEROI_TYPE) //hero is not selected (presumably town)
	{
		assert(!terrain.currentPath); //path can be active only when hero is selected
		if(selection == topBlocking) //selected town clicked
			LOCPLINT->openTownWindow(static_cast<const CGTownInstance*>(topBlocking));
		else if ( canSelect )
				select(static_cast<const CArmedInstance*>(topBlocking), false);
		return;
	}
	else if(const CGHeroInstance * currentHero = curHero()) //hero is selected
	{
		const CGPathNode *pn = LOCPLINT->cb->getPathInfo(mp);
		if(currentHero == topBlocking) //clicked selected hero
		{
			LOCPLINT->openHeroWindow(currentHero);
			return;
		}
		else if(canSelect && pn->turns == 255 ) //selectable object at inaccessible tile
		{
			select(static_cast<const CArmedInstance*>(topBlocking), false);
			return;
		}
		else //still here? we need to move hero if we clicked end of already selected path or calculate a new path otherwise
		{
			if (terrain.currentPath  &&  terrain.currentPath->endPos() == mp)//we'll be moving
			{
				LOCPLINT->moveHero(currentHero,*terrain.currentPath);
				return;
			}
			else if(mp.z == currentHero->pos.z) //remove old path and find a new one if we clicked on the map level on which hero is present
			{
				CGPath &path = LOCPLINT->paths[currentHero];
				terrain.currentPath = &path;
				if(!LOCPLINT->cb->getPath2(mp, path)) //try getting path, erase if failed
					LOCPLINT->eraseCurrentPathOf(currentHero);
				else
					return;
			}
		}
	} //end of hero is selected "case"
	else
	{
		throw std::string("Nothing is selected...");
	}

	if(const IShipyard *shipyard = ourInaccessibleShipyard(topBlocking))
	{
		LOCPLINT->showShipyardDialogOrProblemPopup(shipyard);
	}
}

void CAdvMapInt::tileHovered(const int3 &tile)
{
	if(!LOCPLINT->cb->isVisible(tile))
	{
		CCS->curh->changeGraphic(0, 0);
		statusbar.clear();
		return;
	}

	std::vector<std::string> temp = LOCPLINT->cb->getObjDescriptions(tile);
	if (temp.size())
	{
		boost::replace_all(temp.back(),"\n"," ");
		statusbar.print(temp.back());
	}
	else
	{
		std::string hlp;
		CGI->mh->getTerrainDescr(tile, hlp, false);
		statusbar.print(hlp);
	}

	const CGPathNode *pnode = LOCPLINT->cb->getPathInfo(tile);
	std::vector<const CGObjectInstance *> objs = LOCPLINT->cb->getBlockingObjs(tile); 
	const CGObjectInstance *objAtTile = objs.size() ? objs.back() : NULL;
	bool accessible  =  pnode->turns < 255;

	int turns = pnode->turns;
	amin(turns, 3);

	if(!selection) //may occur just at the start of game (fake move before full intiialization)
		return;

	if(spellBeingCasted)
	{
		switch(spellBeingCasted->id)
		{
		case Spells::SCUTTLE_BOAT:
			if(objAtTile && objAtTile->ID == 8)
				CCS->curh->changeGraphic(0, 42);
			else
				CCS->curh->changeGraphic(0, 0);
			return;
		case Spells::DIMENSION_DOOR:
			{
				const TerrainTile *t = LOCPLINT->cb->getTileInfo(tile);
				int3 hpos = selection->getSightCenter();
				if((!t  ||  t->isClear(LOCPLINT->cb->getTileInfo(hpos)))   &&   isInScreenRange(hpos, tile))
					CCS->curh->changeGraphic(0, 41);
				else
					CCS->curh->changeGraphic(0, 0);
				return;
			}
		}
	}

	const bool guardingCreature = CGI->mh->map->isInTheMap(LOCPLINT->cb->guardingCreaturePosition(tile));

	if(selection->ID == TOWNI_TYPE)
	{
		if(objAtTile)
		{
			if(objAtTile->ID == TOWNI_TYPE && LOCPLINT->cb->getPlayerRelations(LOCPLINT->playerID, objAtTile->tempOwner))
				CCS->curh->changeGraphic(0, 3);
			else if(objAtTile->ID == HEROI_TYPE && objAtTile->tempOwner == LOCPLINT->playerID)
				CCS->curh->changeGraphic(0, 2);
		}
		else
			CCS->curh->changeGraphic(0, 0);
	}
	else if(const CGHeroInstance *h = curHero())
	{
		if(objAtTile)
		{
			if(objAtTile->ID == HEROI_TYPE)
			{
				if(!LOCPLINT->cb->getPlayerRelations( LOCPLINT->playerID, objAtTile->tempOwner)) //enemy hero
				{
					if(accessible)
						CCS->curh->changeGraphic(0, 5 + turns*6);
					else
						CCS->curh->changeGraphic(0, 0);
				}
				else //our or ally hero
				{
					if(selection == objAtTile)
						CCS->curh->changeGraphic(0, 2);
					else if(accessible)
						CCS->curh->changeGraphic(0, 8 + turns*6);
					else
						CCS->curh->changeGraphic(0, 2);
				}
			}
			else if(objAtTile->ID == TOWNI_TYPE)
			{
				if(!LOCPLINT->cb->getPlayerRelations( LOCPLINT->playerID, objAtTile->tempOwner)) //enemy town
				{
					if(accessible) 
					{
						const CGTownInstance* townObj = dynamic_cast<const CGTownInstance*>(objAtTile);

						// Show movement cursor for unguarded enemy towns, otherwise attack cursor.
						if (townObj && !townObj->armedGarrison())
							CCS->curh->changeGraphic(0, 9 + turns*6);
						else
							CCS->curh->changeGraphic(0, 5 + turns*6);

					} 
					else 
					{
						CCS->curh->changeGraphic(0, 0);
					}
				}
				else //our or ally town
				{
					if(accessible)
						CCS->curh->changeGraphic(0, 9 + turns*6);
					else
						CCS->curh->changeGraphic(0, 3);
				}
			}
			else if(objAtTile->ID == 8) //boat
			{
				if(accessible)
					CCS->curh->changeGraphic(0, 6 + turns*6);
				else
					CCS->curh->changeGraphic(0, 0);
			}
			else if (objAtTile->ID == 33 || objAtTile->ID == 219) // Garrison
			{
				if (accessible) 
				{
					const CGGarrison* garrObj = dynamic_cast<const CGGarrison*>(objAtTile); //TODO evil evil cast!

					// Show battle cursor for guarded enemy garrisons, otherwise movement cursor.
					if (garrObj  &&  garrObj->stacksCount() 
						&& !LOCPLINT->cb->getPlayerRelations( LOCPLINT->playerID, garrObj->tempOwner) )
						CCS->curh->changeGraphic(0, 5 + turns*6);
					else
						CCS->curh->changeGraphic(0, 9 + turns*6);
				} 
				else 
					CCS->curh->changeGraphic(0, 0);
			}
			else if (guardingCreature && accessible) //(objAtTile->ID == 54) //monster
			{
				CCS->curh->changeGraphic(0, 5 + turns*6);
			}
			else
			{
				if(accessible)
				{
					if(pnode->land)
						CCS->curh->changeGraphic(0, 9 + turns*6);
					else
						CCS->curh->changeGraphic(0, 28 + turns);
				}
				else
					CCS->curh->changeGraphic(0, 0);
			}
		} 
		else //no objs 
		{
			if(accessible && pnode->accessible != CGPathNode::FLYABLE)
			{
				if (guardingCreature) {
					CCS->curh->changeGraphic(0, 5 + turns*6);
				} else {
					if(pnode->land)
					{
						if(LOCPLINT->cb->getTileInfo(h->getPosition(false))->tertype != TerrainTile::water)
							CCS->curh->changeGraphic(0, 4 + turns*6);
						else
							CCS->curh->changeGraphic(0, 7 + turns*6); //anchor
					}
					else
						CCS->curh->changeGraphic(0, 6 + turns*6);
				}
			}
			else
				CCS->curh->changeGraphic(0, 0);
		}
	}

	if(ourInaccessibleShipyard(objAtTile))
	{
		CCS->curh->changeGraphic(0, 6);
	}
}

void CAdvMapInt::tileRClicked(const int3 &mp)
{
	if(spellBeingCasted)
	{
		leaveCastingMode();
		return;
	}
	if(!LOCPLINT->cb->isVisible(mp))
	{
		CRClickPopup::createAndPush(VLC->generaltexth->allTexts[61]); //Uncharted Territory
		return;
	}

	std::vector < const CGObjectInstance * > objs = LOCPLINT->cb->getBlockingObjs(mp);
	if(!objs.size()) 
	{
		// Bare or undiscovered terrain
		const TerrainTile * tile = LOCPLINT->cb->getTileInfo(mp);
		if (tile) 
		{
			std::string hlp;
			CGI->mh->getTerrainDescr(mp, hlp, true);
			CRClickPopup::createAndPush(hlp);
		}
		return;
	}

	const CGObjectInstance * obj = objs.back();
	CRClickPopup::createAndPush(obj, GH.current->motion, CENTER);
}

void CAdvMapInt::enterCastingMode(const CSpell * sp)
{
	using namespace Spells;
	assert(sp->id == SCUTTLE_BOAT  ||  sp->id == DIMENSION_DOOR);
	spellBeingCasted = sp;

	deactivate();
	terrain.activate();
	GH.fakeMouseMove();
}

void CAdvMapInt::leaveCastingMode(bool cast /*= false*/, int3 dest /*= int3(-1, -1, -1)*/)
{
	assert(spellBeingCasted);
	int id = spellBeingCasted->id;
	spellBeingCasted = NULL;
	terrain.deactivate();
	activate();

	if(cast)
		LOCPLINT->cb->castSpell(curHero(), id, dest);
	else 
		LOCPLINT->showInfoDialog(CGI->generaltexth->allTexts[731]); //Spell cancelled
}

const CGHeroInstance * CAdvMapInt::curHero() const
{
	if(selection && selection->ID == HEROI_TYPE)
		return static_cast<const CGHeroInstance *>(selection);
	else
		return NULL;
}

const CGTownInstance * CAdvMapInt::curTown() const
{
	if(selection && selection->ID == TOWNI_TYPE)
		return static_cast<const CGTownInstance *>(selection);
	else
		return NULL;
}

const IShipyard * CAdvMapInt::ourInaccessibleShipyard(const CGObjectInstance *obj) const
{
	const IShipyard *ret = IShipyard::castFrom(obj);

	if(!ret || obj->tempOwner != player || CCS->curh->mode || (CCS->curh->number != 6 && CCS->curh->number != 0))
		return NULL;

	return ret;
}

CAdventureOptions::CAdventureOptions()
{
	OBJ_CONSTRUCTION_CAPTURING_ALL;
	bg = new CPicture("ADVOPTS.bmp");
	graphics->blueToPlayersAdv(bg->bg, LOCPLINT->playerID);
	pos = bg->center();
	exit = new AdventureMapButton("","",boost::bind(&CGuiHandler::popIntTotally, &GH, this), 204, 313, "IOK6432.DEF",SDLK_RETURN);
	exit->assignedKeys.insert(SDLK_ESCAPE);

	//scenInfo = new AdventureMapButton("","", boost::bind(&CGuiHandler::popIntTotally, &GH, this), 24, 24, "ADVINFO.DEF",SDLK_i);
	scenInfo = new AdventureMapButton("","", boost::bind(&CGuiHandler::popIntTotally, &GH, this), 24, 198, "ADVINFO.DEF",SDLK_i);
	scenInfo->callback += CAdventureOptions::showScenarioInfo;
	//viewWorld = new AdventureMapButton("","",boost::bind(&CGuiHandler::popIntTotally, &GH, this), 204, 313, "IOK6432.DEF",SDLK_RETURN);

	puzzle = new AdventureMapButton("","", boost::bind(&CGuiHandler::popIntTotally, &GH, this), 24, 81, "ADVPUZ.DEF");
	puzzle->callback += boost::bind(&CPlayerInterface::showPuzzleMap, LOCPLINT);

	dig = new AdventureMapButton("","", boost::bind(&CGuiHandler::popIntTotally, &GH, this), 24, 139, "ADVDIG.DEF");
	if(const CGHeroInstance *h = adventureInt->curHero())
		dig->callback += boost::bind(&CPlayerInterface::tryDiggging, LOCPLINT, h);
	else
		dig->block(true);
}

CAdventureOptions::~CAdventureOptions()
{
}

void CAdventureOptions::showScenarioInfo()
{
	GH.pushInt(new CScenarioInfo(LOCPLINT->cb->getMapHeader(), LOCPLINT->cb->getStartInfo()));
}
