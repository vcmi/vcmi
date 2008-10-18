#include "stdafx.h"
#include "CAdvmapInterface.h"
#include "client/CBitmapHandler.h"
#include "CPlayerInterface.h"
#include "CCastleInterface.h"
#include "CCursorHandler.h"
#include "hch/CPreGameTextHandler.h"
#include "hch/CGeneralTextHandler.h"
#include "hch/CDefHandler.h"
#include "hch/CTownHandler.h"
#include "CPathfinder.h"
#include "CGameInfo.h"
#include "SDL_Extensions.h"
#include "CCallback.h"
#include <boost/assign/std/vector.hpp>
#include "mapHandler.h"
#include "CMessage.h"
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/replace.hpp>
#include "hch/CHeroHandler.h"
#include <sstream>
#include "AdventureMapButton.h"
#include "CHeroWindow.h"
#include "client/Graphics.h"
#include "hch/CObjectHandler.h"
#include <boost/thread.hpp>
#include "map.h"
#include "client/CSpellWindow.h"
#include "lib/CondSh.h"
#pragma warning (disable : 4355) 
extern TTF_Font * TNRB16, *TNR, *GEOR13, *GEORXX; //fonts

using namespace boost::logic;
using namespace boost::assign;
using namespace CSDL_Ext;
CAdvMapInt::~CAdvMapInt()
{
	SDL_FreeSurface(bg);
	delete heroWindow;
}
CMinimap::CMinimap(bool draw)
{
	int3 mapSizes = LOCPLINT->cb->getMapSize();
	statusbarTxt = CGI->preth->zelp[291].first;
	rcText = CGI->preth->zelp[291].second;
	pos.x=630;
	pos.y=26;
	pos.h=pos.w=144;

	int rx = (((float)19)/(mapSizes.x))*((float)pos.w),
		ry = (((float)18)/(mapSizes.y))*((float)pos.h);

	radar = newSurface(rx,ry);
	temps = newSurface(144,144);
	SDL_FillRect(radar,NULL,0x00FFFF);
	for (int i=0; i<radar->w; i++)
	{
		if (i%4 || (i==0))
		{
			SDL_PutPixel(radar,i,0,255,75,125);
			SDL_PutPixel(radar,i,radar->h-1,255,75,125);
		}
	}
	for (int i=0; i<radar->h; i++)
	{
		if ((i%4) || (i==0))
		{
			SDL_PutPixel(radar,0,i,255,75,125);
			SDL_PutPixel(radar,radar->w-1,i,255,75,125);
		}
	}
	SDL_SetColorKey(radar,SDL_SRCCOLORKEY,SDL_MapRGB(radar->format,0,255,255));

	//radar = CDefHandler::giveDef("RADAR.DEF");
	std::ifstream is("config/minimap.txt",std::ifstream::in);
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
void CMinimap::draw()
{
	int3 mapSizes = LOCPLINT->cb->getMapSize();
	//draw terrain
	blitAt(map[LOCPLINT->adventureInt->position.z],0,0,temps);

	//draw heroes
	std::vector <const CGHeroInstance *> hh = LOCPLINT->cb->getHeroesInfo(false);
	int mw = map[0]->w, mh = map[0]->h,
		wo = mw/mapSizes.x, ho = mh/mapSizes.y;

	for (int i=0; i<hh.size();i++)
	{
		int3 hpos = hh[i]->getPosition(false);
		if(hpos.z!=LOCPLINT->adventureInt->position.z)
			continue;
		//float zawx = ((float)hpos.x/CGI->mh->sizes.x), zawy = ((float)hpos.y/CGI->mh->sizes.y);
		int3 maplgp ( (hpos.x*mw)/mapSizes.x, (hpos.y*mh)/mapSizes.y, hpos.z );
		for (int ii=0; ii<wo; ii++)
		{
			for (int jj=0; jj<ho; jj++)
			{
				SDL_PutPixel(temps,maplgp.x+ii,maplgp.y+jj,graphics->playerColors[hh[i]->getOwner()].r,graphics->playerColors[hh[i]->getOwner()].g,graphics->playerColors[hh[i]->getOwner()].b);
			}
		}
	}

	//draw flaggable objects
	for(int x=0; x<mapSizes.x; ++x)
	{
		for(int y=0; y<mapSizes.y; ++y)
		{
			std::vector < const CGObjectInstance * > oo = LOCPLINT->cb->getFlaggableObjects(int3(x, y, LOCPLINT->adventureInt->position.z));
			for(int v=0; v<oo.size(); ++v)
			{
				if(!dynamic_cast< const CGHeroInstance * >(oo[v])) //heroes have been printed
				{
					int3 maplgp ( (x*mw)/mapSizes.x, (y*mh)/mapSizes.y, LOCPLINT->adventureInt->position.z );
					for (int ii=0; ii<wo; ii++)
					{
						for (int jj=0; jj<ho; jj++)
						{
							if(oo[v]->tempOwner == 255)
								SDL_PutPixel(temps,maplgp.x+ii,maplgp.y+jj,graphics->neutralColor->r,graphics->neutralColor->g,graphics->neutralColor->b);
							else
								SDL_PutPixel(temps,maplgp.x+ii,maplgp.y+jj,graphics->playerColors[oo[v]->getOwner()].r,graphics->playerColors[oo[v]->getOwner()].g,graphics->playerColors[oo[v]->getOwner()].b);
						}
					}
				}
			}
		}
	}

	blitAt(FoW[LOCPLINT->adventureInt->position.z],0,0,temps);

	//draw radar
	int bx = (((float)LOCPLINT->adventureInt->position.x)/(((float)mapSizes.x)))*pos.w,
		by = (((float)LOCPLINT->adventureInt->position.y)/(((float)mapSizes.y)))*pos.h;
	blitAt(radar,bx,by,temps);
	blitAt(temps,pos.x,pos.y);
	//SDL_UpdateRect(screen,pos.x,pos.y,pos.w,pos.h);
}
void CMinimap::redraw(int level)// (level==-1) => redraw all levels
{
	int3 mapSizes = LOCPLINT->cb->getMapSize();
	for (int i=0; i<CGI->mh->sizes.z; i++)
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
				if (CGI->mh->ttiles[mx][my][i].tileInfo->blocked && (!CGI->mh->ttiles[mx][my][i].tileInfo->visitable))
					SDL_PutPixel(pom,x,y,colorsBlocked[CGI->mh->ttiles[mx][my][i].tileInfo->tertype].r,colorsBlocked[CGI->mh->ttiles[mx][my][i].tileInfo->tertype].g,colorsBlocked[CGI->mh->ttiles[mx][my][i].tileInfo->tertype].b);
				else SDL_PutPixel(pom,x,y,colors[CGI->mh->ttiles[mx][my][i].tileInfo->tertype].r,colors[CGI->mh->ttiles[mx][my][i].tileInfo->tertype].g,colors[CGI->mh->ttiles[mx][my][i].tileInfo->tertype].b);
			}
		}
		map.push_back(pom);

	}

	//FoW
	int mw = map[0]->w, mh = map[0]->h,
		wo = mw/mapSizes.x, ho = mh/mapSizes.y;
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
		CSDL_Ext::update(pt);
		FoW.push_back(pt);
	}
	//FoW end
}
void CMinimap::updateRadar()
{}
void CMinimap::clickRight (tribool down)
{
	LOCPLINT->adventureInt->handleRightClick(rcText,down,this);
}
void CMinimap::clickLeft (tribool down)
{
	if (down && (!pressedL))
		MotionInterested::activate();
	else if (!down)
	{
		if (std::find(LOCPLINT->motioninterested.begin(),LOCPLINT->motioninterested.end(),this)!=LOCPLINT->motioninterested.end())
			MotionInterested::deactivate();
	}
	ClickableL::clickLeft(down);
	if (!((bool)down))
		return;

	float dx=((float)(LOCPLINT->current->motion.x-pos.x))/((float)pos.w),
		dy=((float)(LOCPLINT->current->motion.y-pos.y))/((float)pos.h);

	int3 newCPos;
	newCPos.x = (CGI->mh->sizes.x*dx);
	newCPos.y = (CGI->mh->sizes.y*dy);
	newCPos.z = LOCPLINT->adventureInt->position.z;
	LOCPLINT->adventureInt->centerOn(newCPos);
}
void CMinimap::hover (bool on)
{
	Hoverable::hover(on);
	if (on)
		LOCPLINT->adventureInt->statusbar.print(statusbarTxt);
	else if (LOCPLINT->adventureInt->statusbar.current==statusbarTxt)
		LOCPLINT->adventureInt->statusbar.clear();
}
void CMinimap::mouseMoved (const SDL_MouseMotionEvent & sEvent)
{
	if (pressedL)
	{
		clickLeft(true);
	}
}
void CMinimap::activate()
{
	ClickableL::activate();
	ClickableR::activate();
	Hoverable::activate();
	if (pressedL)
		MotionInterested::activate();
}
void CMinimap::deactivate()
{
	if (pressedL)
		MotionInterested::deactivate();
	ClickableL::deactivate();
	ClickableR::deactivate();
	Hoverable::deactivate();
}
void CMinimap::showTile(int3 pos)
{
	int mw = map[0]->w, mh = map[0]->h;
	double wo = ((double)mw)/CGI->mh->sizes.x, ho = ((double)mh)/CGI->mh->sizes.y;
	for (int ii=0; ii<wo; ii++)
	{
		for (int jj=0; jj<ho; jj++)
		{
			if ((pos.x*wo+ii<this->pos.w) && (pos.y*ho+jj<this->pos.h))
				CSDL_Ext::SDL_PutPixel(FoW[pos.z],pos.x*wo+ii,pos.y*ho+jj,0,0,0,0,0);

		}
	}
}
void CMinimap::hideTile(int3 pos)
{
}
CTerrainRect::CTerrainRect():currentPath(NULL)
{
	tilesw=19;
	tilesh=18;
	pos.x=7;
	pos.y=6;
	pos.w=593;
	pos.h=547;
	moveX = moveY = 0;
	arrows = CDefHandler::giveDef("ADAG.DEF");
	for(int y=0; y<arrows->ourImages.size(); ++y)
	{
		arrows->ourImages[y].bitmap = CSDL_Ext::alphaTransform(arrows->ourImages[y].bitmap);
	}
}
void CTerrainRect::activate()
{
	ClickableL::activate();
	ClickableR::activate();
	Hoverable::activate();
	KeyInterested::activate();
	MotionInterested::activate();
};
void CTerrainRect::deactivate()
{
	ClickableL::deactivate();
	ClickableR::deactivate();
	Hoverable::deactivate();
	KeyInterested::deactivate();
	MotionInterested::deactivate();
};
void CTerrainRect::clickLeft(tribool down)
{
	if ((down==false) || indeterminate(down))
		return;
	int3 mp = whichTileIsIt();
	if ((mp.x<0) || (mp.y<0))
		return;
	std::vector < const CGObjectInstance * > objs;
	if (LOCPLINT->adventureInt->selection->ID != HEROI_TYPE)
	{
		if (currentPath)
		{
			tlog2<<"Warning: Lost path?" << std::endl;
			delete currentPath;
			currentPath = NULL;
		}
		objs = LOCPLINT->cb->getBlockingObjs(mp);
		for(int i=0; i<objs.size();i++)
		{
			if(objs[i]->ID == 98 && objs[i]->tempOwner == LOCPLINT->playerID) //town
			{
				if(LOCPLINT->adventureInt->selection == (objs[i]))
				{
					LOCPLINT->openTownWindow(static_cast<const CGTownInstance*>(objs[i]));
				}
				else
				{
					LOCPLINT->adventureInt->select(static_cast<const CArmedInstance*>(objs[i]));
					return;
				}
			}
			else if(objs[i]->ID == 34 && objs[i]->tempOwner == LOCPLINT->playerID)
			{
				LOCPLINT->adventureInt->select(static_cast<const CArmedInstance*>(objs[i]));
				return;
			}
		}
		return;
	}
	else
	{
		objs = LOCPLINT->cb->getVisitableObjs(mp);
		for(int i=0; i<objs.size();i++)
		{
			if(objs[i]->ID == 98)
				goto endchkpt;
		}
		objs = LOCPLINT->cb->getBlockingObjs(mp);
		for(int i=0; i<objs.size();i++)
		{
			if(objs[i]->ID == 98 && objs[i]->tempOwner == LOCPLINT->playerID) //town
			{
				LOCPLINT->adventureInt->select(static_cast<const CArmedInstance*>(objs[i]));
				return;
			}
			else if(objs[i]->ID == 34 && objs[i]->tempOwner == LOCPLINT->playerID && LOCPLINT->adventureInt->selection == (objs[i]))
			{
				LOCPLINT->openHeroWindow(static_cast<const CGHeroInstance*>(objs[i]));
				return;
			}
		}
	}
endchkpt:
	bool mres =true;
	if (currentPath)
	{
		if ( (currentPath->endPos()) == mp)
		{ //move
			CPath sended(*currentPath); //temporary path - engine will operate on it
			LOCPLINT->pim->unlock();
			mres = LOCPLINT->cb->moveHero( ((const CGHeroInstance*)LOCPLINT->adventureInt->selection)->type->ID,&sended,1,0);
			LOCPLINT->pim->lock();
			if(mres)
			{
				delete currentPath;
				currentPath = NULL;
				LOCPLINT->adventureInt->heroList.items[LOCPLINT->adventureInt->heroList.getPosOfHero(LOCPLINT->adventureInt->selection)].second = NULL;
			}
		}
		else
		{
			delete currentPath;
			currentPath=NULL;
			LOCPLINT->adventureInt->heroList.items[LOCPLINT->adventureInt->heroList.getPosOfHero(LOCPLINT->adventureInt->selection)].second = NULL;
		}
	}
	const CGHeroInstance * currentHero = (LOCPLINT->adventureInt->heroList.items.size())?(LOCPLINT->adventureInt->heroList.items[LOCPLINT->adventureInt->heroList.selected].first):(NULL);
	if(currentHero)
	{
		int3 bufpos = currentHero->getPosition(false);
		if (mres)
		{
			vector<Coordinate>* p;
			p = CGI->pathf->GetPath(Coordinate(bufpos),Coordinate(mp),currentHero);
			//Convert to old format.
			currentPath = LOCPLINT->adventureInt->heroList.items[LOCPLINT->adventureInt->heroList.selected].second = CGI->pathf->ConvertToOldFormat(p);
			delete p;
		}
		return;
	}
	
}
void CTerrainRect::clickRight(tribool down)
{
}
void CTerrainRect::mouseMoved (const SDL_MouseMotionEvent & sEvent)
{
	int3 pom=LOCPLINT->adventureInt->verifyPos(whichTileIsIt(sEvent.x,sEvent.y));
	if (pom!=curHoveredTile)
		curHoveredTile=pom;
	else
		return;
	std::vector<std::string> temp = LOCPLINT->cb->getObjDescriptions(pom);
	if (temp.size())
	{
		LOCPLINT->adventureInt->statusbar.print((*((temp.end())-1)));
	}
	else
	{
		LOCPLINT->adventureInt->statusbar.clear();
	}
	std::vector<const CGObjectInstance *> objs = LOCPLINT->cb->getVisitableObjs(pom); 
	for(int i=0; i<objs.size();i++)
	{
		if(objs[i]->ID == 98) //town
		{
			CGI->curh->changeGraphic(0,0);
			return;
		}
	}
	objs = LOCPLINT->cb->getBlockingObjs(pom);
	for(int i=0; i<objs.size();i++)
	{
		if(objs[i]->ID == 98 && objs[i]->tempOwner == LOCPLINT->playerID) //town
		{
			CGI->curh->changeGraphic(0,3);
			return;
		}
		else if(objs[i]->ID == 34 //mouse over hero
			&& (objs[i]==LOCPLINT->adventureInt->selection  ||  LOCPLINT->adventureInt->selection->ID==98)
			&& objs[i]->tempOwner == LOCPLINT->playerID) //this hero is selected or we've selected a town
		{
			CGI->curh->changeGraphic(0,2);
			return;
		}
	}
	CGI->curh->changeGraphic(0,0);
}
void CTerrainRect::keyPressed (const SDL_KeyboardEvent & key){}
void CTerrainRect::hover(bool on)
{
	if (!on)
		LOCPLINT->adventureInt->statusbar.clear();
}
void CTerrainRect::showPath()
{
	for (int i=0;i<currentPath->nodes.size()-1;i++)
	{
		int pn=-1;//number of picture
		if (i==0) //last tile
		{
			int x = 32*(currentPath->nodes[i].coord.x-LOCPLINT->adventureInt->position.x)+7,
				y = 32*(currentPath->nodes[i].coord.y-LOCPLINT->adventureInt->position.y)+6;
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
			std::vector<CPathNode> & cv = currentPath->nodes;
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
				if(cv[i-1].coord.x == cv[i].coord.x && cv[i-1].coord.y == cv[i].coord.y+1)
				{
					pn = 5;
				}
				else if(cv[i-1].coord.x == cv[i].coord.x-1 && cv[i-1].coord.y == cv[i].coord.y+1)
				{
					pn = 14;
				}
				else if(cv[i-1].coord.x+1 == cv[i].coord.x && cv[i-1].coord.y == cv[i].coord.y)
				{
					pn = 23;
				}
				else if(cv[i-1].coord.x == cv[i].coord.x-1 && cv[i-1].coord.y == cv[i].coord.y-1)
				{
					pn = 24;
				}
				else if(cv[i-1].coord.x == cv[i].coord.x+1 && cv[i-1].coord.y == cv[i].coord.y+1)
				{
					pn = 4;
				}
			}
			else if (cv[i+1].coord.x == cv[i].coord.x+1 && cv[i+1].coord.y == cv[i].coord.y) //65x
			{
				if(cv[i-1].coord.x == cv[i].coord.x-1 && cv[i-1].coord.y == cv[i].coord.y+1)
				{
					pn = 6;
				}
				else if(cv[i-1].coord.x == cv[i].coord.x-1 && cv[i-1].coord.y == cv[i].coord.y)
				{
					pn = 15;
				}
				else if(cv[i-1].coord.x == cv[i].coord.x-1 && cv[i-1].coord.y == cv[i].coord.y-1)
				{
					pn = 24;
				}
			}
			else if (cv[i+1].coord.x == cv[i].coord.x+1 && cv[i+1].coord.y == cv[i].coord.y+1) //95x
			{
				if(cv[i-1].coord.x == cv[i].coord.x-1 && cv[i-1].coord.y == cv[i].coord.y)
				{
					pn = 7;
				}
				else if(cv[i-1].coord.x == cv[i].coord.x-1 && cv[i-1].coord.y == cv[i].coord.y-1)
				{
					pn = 16;
				}
				else if(cv[i-1].coord.x == cv[i].coord.x && cv[i-1].coord.y == cv[i].coord.y-1)
				{
					pn = 17;
				}
				else if(cv[i-1].coord.x == cv[i].coord.x-1 && cv[i-1].coord.y == cv[i].coord.y+1)
				{
					pn = 6;
				}
				else if(cv[i-1].coord.x == cv[i].coord.x+1 && cv[i-1].coord.y == cv[i].coord.y-1)
				{
					pn = 18;
				}
			}
			else if (cv[i+1].coord.x == cv[i].coord.x && cv[i+1].coord.y == cv[i].coord.y+1) //85x
			{
				if(cv[i-1].coord.x == cv[i].coord.x-1 && cv[i-1].coord.y == cv[i].coord.y+1)
				{
					pn = 6;
				}
				if(cv[i-1].coord.x == cv[i].coord.x-1 && cv[i-1].coord.y == cv[i].coord.y)
				{
					pn = 7;
				}
				if(cv[i-1].coord.x == cv[i].coord.x-1 && cv[i-1].coord.y == cv[i].coord.y-1)
				{
					pn = 8;
				}
				else if(cv[i-1].coord.x == cv[i].coord.x && cv[i-1].coord.y == cv[i].coord.y-1)
				{
					pn = 9;
				}
				else if(cv[i-1].coord.x == cv[i].coord.x+1 && cv[i-1].coord.y == cv[i].coord.y-1)
				{
					pn = 18;
				}
				else if(cv[i-1].coord.x == cv[i].coord.x+1 && cv[i-1].coord.y == cv[i].coord.y)
				{
					pn = 19;
				}
				else if(cv[i-1].coord.x == cv[i].coord.x+1 && cv[i-1].coord.y == cv[i].coord.y+1)
				{
					pn = 20;
				}
			}
			else if (cv[i+1].coord.x == cv[i].coord.x-1 && cv[i+1].coord.y == cv[i].coord.y+1) //75x
			{
				if(cv[i-1].coord.x == cv[i].coord.x && cv[i-1].coord.y == cv[i].coord.y-1)
				{
					pn = 1;
				}
				else if(cv[i-1].coord.x == cv[i].coord.x+1 && cv[i-1].coord.y == cv[i].coord.y-1)
				{
					pn = 10;
				}
				else if(cv[i-1].coord.x == cv[i].coord.x+1 && cv[i-1].coord.y == cv[i].coord.y)
				{
					pn = 19;
				}
				else if(cv[i-1].coord.x == cv[i].coord.x-1 && cv[i-1].coord.y == cv[i].coord.y-1)
				{
					pn = 8;
				}
				else if(cv[i-1].coord.x == cv[i].coord.x+1 && cv[i-1].coord.y == cv[i].coord.y+1)
				{
					pn = 20;
				}
			}
			else if (cv[i+1].coord.x == cv[i].coord.x-1 && cv[i+1].coord.y == cv[i].coord.y) //45x
			{
				if(cv[i-1].coord.x == cv[i].coord.x+1 && cv[i-1].coord.y == cv[i].coord.y-1)
				{
					pn = 2;
				}
				else if(cv[i-1].coord.x == cv[i].coord.x+1 && cv[i-1].coord.y == cv[i].coord.y)
				{
					pn = 11;
				}
				else if(cv[i-1].coord.x == cv[i].coord.x+1 && cv[i-1].coord.y == cv[i].coord.y+1)
				{
					pn = 20;
				}
			}

		}
		if (  ((currentPath->nodes[i].dist)-(*(currentPath->nodes.end()-1)).dist) > ((const CGHeroInstance*)(LOCPLINT->adventureInt->selection))->movement)
			pn+=25;
		if (pn>=0)
		{
			int x = 32*(currentPath->nodes[i].coord.x-LOCPLINT->adventureInt->position.x)+7,
				y = 32*(currentPath->nodes[i].coord.y-LOCPLINT->adventureInt->position.y)+6;
			if (x<0 || y<0 || x>pos.w || y>pos.h)
				continue;
			int hvx = (x+arrows->ourImages[pn].bitmap->w)-(pos.x+pos.w),
				hvy = (y+arrows->ourImages[pn].bitmap->h)-(pos.y+pos.h);
			if (hvx<0 && hvy<0)
			{
				CSDL_Ext::blit8bppAlphaTo24bpp(arrows->ourImages[pn].bitmap, NULL, screen, &genRect(32, 32, x, y));
			}
			else if(hvx<0)
			{
				CSDL_Ext::blit8bppAlphaTo24bpp
					(arrows->ourImages[pn].bitmap,&genRect(arrows->ourImages[pn].bitmap->h-hvy,arrows->ourImages[pn].bitmap->w,0,0),
					screen,&genRect(arrows->ourImages[pn].bitmap->h-hvy,arrows->ourImages[pn].bitmap->w,x,y));
			}
			else if (hvy<0)
			{
				CSDL_Ext::blit8bppAlphaTo24bpp
					(arrows->ourImages[pn].bitmap,&genRect(arrows->ourImages[pn].bitmap->h,arrows->ourImages[pn].bitmap->w-hvx,0,0),
					screen,&genRect(arrows->ourImages[pn].bitmap->h,arrows->ourImages[pn].bitmap->w-hvx,x,y));
			}
			else
			{
				CSDL_Ext::blit8bppAlphaTo24bpp
					(arrows->ourImages[pn].bitmap,&genRect(arrows->ourImages[pn].bitmap->h-hvy,arrows->ourImages[pn].bitmap->w-hvx,0,0),
					screen,&genRect(arrows->ourImages[pn].bitmap->h-hvy,arrows->ourImages[pn].bitmap->w-hvx,x,y));
			}

		}
	} //for (int i=0;i<currentPath->nodes.size()-1;i++)
}
void CTerrainRect::show()
{
	SDL_Surface * teren = CGI->mh->terrainRect
		(LOCPLINT->adventureInt->position.x,LOCPLINT->adventureInt->position.y,
		tilesw,tilesh,LOCPLINT->adventureInt->position.z,LOCPLINT->adventureInt->anim,
		&LOCPLINT->cb->getVisibilityMap(), true, LOCPLINT->adventureInt->heroAnim,
		screen, &genRect(547, 594, 7, 6)
		);
	//SDL_BlitSurface(teren,&genRect(pos.h,pos.w,0,0),screen,&genRect(547,594,7,6));
	//SDL_FreeSurface(teren);
	if (currentPath && LOCPLINT->adventureInt->position.z==currentPath->startPos().z) //drawing path
	{
		showPath();
	}
}

int3 CTerrainRect::whichTileIsIt(const int & x, const int & y)
{
	int3 ret;
	ret.x = LOCPLINT->adventureInt->position.x + ((LOCPLINT->current->motion.x-pos.x)/32);
	ret.y = LOCPLINT->adventureInt->position.y + ((LOCPLINT->current->motion.y-pos.y)/32);
	ret.z = LOCPLINT->adventureInt->position.z;
	return ret;
}
int3 CTerrainRect::whichTileIsIt()
{
	return whichTileIsIt(LOCPLINT->current->motion.x,LOCPLINT->current->motion.y);
}
void CResDataBar::clickRight (tribool down)
{
}
void CResDataBar::activate()
{
	ClickableR::activate();
}
void CResDataBar::deactivate()
{
	ClickableR::deactivate();
}
CResDataBar::CResDataBar()
{
	bg = BitmapHandler::loadBitmap("ZRESBAR.bmp");
	SDL_SetColorKey(bg,SDL_SRCCOLORKEY,SDL_MapRGB(bg->format,0,255,255));
	graphics->blueToPlayersAdv(bg,LOCPLINT->playerID);
	pos = genRect(bg->h,bg->w,3,575);

	txtpos  +=  (std::pair<int,int>(35,577)),(std::pair<int,int>(120,577)),(std::pair<int,int>(205,577)),
		(std::pair<int,int>(290,577)),(std::pair<int,int>(375,577)),(std::pair<int,int>(460,577)),
		(std::pair<int,int>(545,577)),(std::pair<int,int>(620,577));
	datetext =  CGI->generaltexth->allTexts[62]+": %s, " + CGI->generaltexth->allTexts[63] + ": %s, " +
		CGI->generaltexth->allTexts[64] + ": %s";

}
CResDataBar::~CResDataBar()
{
	SDL_FreeSurface(bg);
}
void CResDataBar::draw()
{
	blitAt(bg,pos.x,pos.y);
	char * buf = new char[15];
	for (int i=0;i<7;i++)
	{
		SDL_itoa(LOCPLINT->cb->getResourceAmount(i),buf,10);
		printAt(buf,txtpos[i].first,txtpos[i].second,GEOR13,zwykly);
	}
	std::vector<std::string> temp;
	SDL_itoa(LOCPLINT->cb->getDate(3),buf,10); temp+=std::string(buf);
	SDL_itoa(LOCPLINT->cb->getDate(2),buf,10); temp+=std::string(buf);
	SDL_itoa(LOCPLINT->cb->getDate(1),buf,10); temp+=std::string(buf);
	printAt(processStr(datetext,temp),txtpos[7].first,txtpos[7].second,GEOR13,zwykly);
	temp.clear();
	//updateRect(&pos,screen);
	delete[] buf;
}
CInfoBar::CInfoBar()
{
	toNextTick = mode = pom = -1;
	pos.x=605;
	pos.y=389;
	pos.w=194;
	pos.h=186;
	day = CDefHandler::giveDef("NEWDAY.DEF");
	week1 = CDefHandler::giveDef("NEWWEEK1.DEF");
	week2 = CDefHandler::giveDef("NEWWEEK2.DEF");
	week3 = CDefHandler::giveDef("NEWWEEK3.DEF");
	week4 = CDefHandler::giveDef("NEWWEEK4.DEF");
}
CInfoBar::~CInfoBar()
{
	delete day;
	delete week1;
	delete week2;
	delete week3;
	delete week4;
}
void CInfoBar::draw(const CGObjectInstance * specific)
{
	if ((mode>=0) && mode<5)
	{
		blitAnim(mode);
		return;
	}
	else if (mode==5)
	{
		mode = -1;
		draw(LOCPLINT->adventureInt->selection);
	}
	if (!specific)
		specific = LOCPLINT->adventureInt->selection;

	if(!specific)
		return;

	if(specific->ID == 34) //hero
	{
		if(graphics->heroWins.find(specific->subID)!=graphics->heroWins.end())
			blitAt(graphics->heroWins[specific->subID],pos.x,pos.y);
	}
	else if (specific->ID == 98)
	{
		const CGTownInstance * t = static_cast<const CGTownInstance*>(specific);
		if(graphics->townWins.find(t->id)!=graphics->townWins.end())
			blitAt(graphics->townWins[t->id],pos.x,pos.y);
	}

	//SDL_Surface * todr = LOCPLINT->infoWin(specific);
	//if (!todr)
	//	return;
	//blitAt(todr,pos.x,pos.y);
	//SDL_FreeSurface(todr);
}

CDefHandler * CInfoBar::getAnim(int mode)
{
	switch(mode)
	{
	case 0:
		return day;
		break;
	case 1:
		return week1;
		break;
	case 2:
		return week2;
		break;
	case 3:
		return week3;
		break;
	case 4:
		return week4;
		break;
	default:
		return NULL;
		break;
	}
}
void CInfoBar::blitAnim(int mode)//0 - day, 1 - week
{
	CDefHandler * anim = NULL;
	std::stringstream txt;
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
	printAtMiddle(txt.str(),700,420,TNRB16,zwykly);
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
	TimeInterested::activate();
	toNextTick = 500;
	blitAnim(mode);
	//blitAt(day->ourImages[pom].bitmap,pos.x+10,pos.y+10);
}

void CInfoBar::showComp(SComponent * comp, int time)
{
	SDL_Surface * b = BitmapHandler::loadBitmap("ADSTATOT.bmp");
	blitAt(b,pos.x+8,pos.y+11);
	blitAt(comp->getImg(),pos.x+52,pos.y+54);
	printAtMiddle(comp->subtitle,pos.x+91,pos.y+158,GEOR13,zwykly);
	printAtMiddleWB(comp->description,pos.x+94,pos.y+31,GEOR13,26,zwykly);
	SDL_FreeSurface(b);
	TimeInterested::activate();
	mode = 6;
	toNextTick = time;
}

void CInfoBar::tick()
{
	if((mode >= 0) && (mode < 5))
	{
		pom++;
		if (pom >= getAnim(mode)->ourImages.size())
		{
			TimeInterested::deactivate();
			toNextTick = -1;
			mode = 5;
			draw();
			return;
		}
		toNextTick = 150;
		blitAnim(mode);
	}
	else if (mode == 6)
	{
		TimeInterested::deactivate();
		toNextTick = -1;
		mode = 5;
		draw();
	}

}

CAdvMapInt::CAdvMapInt(int Player)
:player(Player),
statusbar(7,556),
kingOverview(CGI->preth->zelp[293].first,CGI->preth->zelp[293].second,
			 boost::bind(&CAdvMapInt::fshowOverview,this), 679, 196, "IAM002.DEF", false,NULL,true),

underground(CGI->preth->zelp[294].first,CGI->preth->zelp[294].second,
			boost::bind(&CAdvMapInt::fswitchLevel,this), 711, 196, "IAM010.DEF", false, new std::vector<std::string>(1,std::string("IAM003.DEF")),true),

questlog(CGI->preth->zelp[295].first,CGI->preth->zelp[295].second,
		 boost::bind(&CAdvMapInt::fshowQuestlog,this), 679, 228, "IAM004.DEF", false,NULL,true),

sleepWake(CGI->preth->zelp[296].first,CGI->preth->zelp[296].second,
		  boost::bind(&CAdvMapInt::fsleepWake,this), 711, 228, "IAM005.DEF", false,NULL,true),

moveHero(CGI->preth->zelp[297].first,CGI->preth->zelp[297].second,
		  boost::bind(&CAdvMapInt::fmoveHero,this), 679, 260, "IAM006.DEF", false,NULL,true),

spellbook(CGI->preth->zelp[298].first,CGI->preth->zelp[298].second,
		  boost::bind(&CAdvMapInt::fshowSpellbok,this), 711, 260, "IAM007.DEF", false,NULL,true),

advOptions(CGI->preth->zelp[299].first,CGI->preth->zelp[299].second,
		  boost::bind(&CAdvMapInt::fadventureOPtions,this), 679, 292, "IAM008.DEF", false,NULL,true),

sysOptions(CGI->preth->zelp[300].first,CGI->preth->zelp[300].second,
		  boost::bind(&CAdvMapInt::fsystemOptions,this), 711, 292, "IAM009.DEF", false,NULL,true),

nextHero(CGI->preth->zelp[301].first,CGI->preth->zelp[301].second,
		  boost::bind(&CAdvMapInt::fnextHero,this), 679, 324, "IAM000.DEF", false,NULL,true),

endTurn(CGI->preth->zelp[302].first,CGI->preth->zelp[302].second,
		  boost::bind(&CAdvMapInt::fendTurn,this), 679, 356, "IAM001.DEF", false,NULL,true),

townList(5,&genRect(192,48,747,196),747,196,747,372)
{
	selection = NULL;
	townList.fun = boost::bind(&CAdvMapInt::selectionChanged,this);
	LOCPLINT->adventureInt=this;
	bg = BitmapHandler::loadBitmap("ADVMAP.bmp");
	graphics->blueToPlayersAdv(bg,player);
	scrollingLeft = false;
	scrollingRight  = false;
	scrollingUp = false ;
	scrollingDown = false ;
	updateScreen  = false;
	anim=0;
	animValHitCount=0; //animation frame
	heroAnim=0;
	heroAnimValHitCount=0; // hero animation frame

	heroList.init();
	heroList.genList();
	//townList.init();
	townList.genList();

	heroWindow = new CHeroWindow(this->player);

	gems.push_back(CDefHandler::giveDef("agemLL.def"));
	gems.push_back(CDefHandler::giveDef("agemLR.def"));
	gems.push_back(CDefHandler::giveDef("agemUL.def"));
	gems.push_back(CDefHandler::giveDef("agemUR.def"));
}

void CAdvMapInt::fshowOverview()
{
}
void CAdvMapInt::fswitchLevel()
{
	if(!CGI->mh->map->twoLevel)
		return;
	if (position.z)
	{
		position.z--;
		underground.curimg=0;
		underground.show();
	}
	else
	{
		underground.curimg=1;
		position.z++;
		underground.show();
	}
	updateScreen = true;
	minimap.draw();
}
void CAdvMapInt::fshowQuestlog()
{
}
void CAdvMapInt::fsleepWake()
{
}
void CAdvMapInt::fmoveHero()
{
	if (selection->ID!=HEROI_TYPE)
		return;
	if (!terrain.currentPath)
		return;
	CPath sended(*(terrain.currentPath)); //temporary path - engine will operate on it
	LOCPLINT->pim->unlock();
	LOCPLINT->cb->moveHero( ((const CGHeroInstance*)LOCPLINT->adventureInt->selection)->type->ID,&sended,1,0);
	LOCPLINT->pim->lock();
}
void CAdvMapInt::fshowSpellbok()
{
	if (selection->ID!=HEROI_TYPE) //checking necessary values
		return;

	LOCPLINT->curint->deactivate();

	CSpellWindow * spellWindow = new CSpellWindow(genRect(595, 620, 90, 2), ((const CGHeroInstance*)LOCPLINT->adventureInt->selection));
	spellWindow->activate();
	LOCPLINT->objsToBlit.push_back(spellWindow);
}
void CAdvMapInt::fadventureOPtions()
{
}
void CAdvMapInt::fsystemOptions()
{
	LOCPLINT->curint->deactivate();

	CSystemOptionsWindow * sysopWindow = new CSystemOptionsWindow(genRect(487, 481, 159, 57), LOCPLINT);
	sysopWindow->activate();
	LOCPLINT->objsToBlit.push_back(sysopWindow);
}
void CAdvMapInt::fnextHero()
{
}
void CAdvMapInt::fendTurn()
{
	LOCPLINT->makingTurn = false;
}

void CAdvMapInt::activate()
{
	if(subInt == heroWindow)
	{
		heroWindow->activate();
		return;
	}
	LOCPLINT->curint = this;
	LOCPLINT->statusbar = &statusbar;
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
	show();
}
void CAdvMapInt::deactivate()
{
	if(subInt == heroWindow)
	{
		heroWindow->deactivate();
		return;
	}
	hide();
}
void CAdvMapInt::show(SDL_Surface *to)
{
	blitAt(bg,0,0);

	kingOverview.show();
	underground.show();
	questlog.show();
	sleepWake.show();
	moveHero.show();
	spellbook.show();
	advOptions.show();
	sysOptions.show();
	nextHero.show();
	endTurn.show();

	minimap.draw();
	heroList.draw();
	townList.draw();
	updateScreen = true;
	update();

	resdatabar.draw();

	statusbar.show();

	infoBar.draw();
}
void CAdvMapInt::hide()
{
	CGI->curh->changeGraphic(0,0);
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
	if(std::find(LOCPLINT->timeinterested.begin(),LOCPLINT->timeinterested.end(),&infoBar)!=LOCPLINT->timeinterested.end())
		LOCPLINT->timeinterested.erase(std::find(LOCPLINT->timeinterested.begin(),LOCPLINT->timeinterested.end(),&infoBar));
	infoBar.mode=-1;

}
void CAdvMapInt::update()
{
	++animValHitCount; //for animations
	if(animValHitCount == 8)
	{
		animValHitCount = 0;
		++anim;
		updateScreen = true;

	}
	++heroAnim;
	if((animValHitCount % 4) && !LOCPLINT->showingDialog->get())
	{
		if(scrollingLeft)
		{
			if(position.x>-Woff)
			{
				position.x--;
				updateScreen = true;
				updateMinimap=true;
			}
		}
		if(scrollingRight)
		{
			if(position.x<CGI->mh->map->width-19+4)
			{
				position.x++;
				updateScreen = true;
				updateMinimap=true;
			}
		}
		if(scrollingUp)
		{
			if(position.y>-Hoff)
			{
				position.y--;
				updateScreen = true;
				updateMinimap=true;
			}
		}
		if(scrollingDown)
		{
			if(position.y<CGI->mh->map->height-18+4)
			{
				position.y++;
				updateScreen = true;
				updateMinimap=true;
			}
		}
	}
	if(updateScreen)
	{	
		terrain.show();
		blitAt(gems[2]->ourImages[LOCPLINT->playerID].bitmap,6,6);
		blitAt(gems[0]->ourImages[LOCPLINT->playerID].bitmap,6,508);
		blitAt(gems[1]->ourImages[LOCPLINT->playerID].bitmap,556,508);
		blitAt(gems[3]->ourImages[LOCPLINT->playerID].bitmap,556,6);
		updateScreen=false;
	}
	if (updateMinimap)
	{
		minimap.draw();
		updateMinimap=false;
	}
}

void CAdvMapInt::selectionChanged()
{
	const CGTownInstance *to = townList.items[townList.selected];
	select(to);
}
void CAdvMapInt::centerOn(int3 on)
{
	on.x -= (LOCPLINT->adventureInt->terrain.tilesw/2);
	on.y -= (LOCPLINT->adventureInt->terrain.tilesh/2);

	if (on.x<0)
		on.x=-(Woff/2);
	else if((on.x+LOCPLINT->adventureInt->terrain.tilesw)  >  (CGI->mh->sizes.x))
		on.x=CGI->mh->sizes.x-LOCPLINT->adventureInt->terrain.tilesw+(Woff/2);

	if (on.y<0)
		on.y = -(Hoff/2);
	else if((on.y+LOCPLINT->adventureInt->terrain.tilesh)  >  (CGI->mh->sizes.y))
		on.y = CGI->mh->sizes.y-LOCPLINT->adventureInt->terrain.tilesh+(Hoff/2);

	LOCPLINT->adventureInt->position.x=on.x;
	LOCPLINT->adventureInt->position.y=on.y;
	LOCPLINT->adventureInt->position.z=on.z;
	LOCPLINT->adventureInt->updateScreen=true;
	updateMinimap=true;
}
void CAdvMapInt::handleRightClick(std::string text, tribool down, CIntObject * client)
{
	if (down)
	{
		boost::algorithm::erase_all(text,"\"");
		CSimpleWindow * temp = CMessage::genWindow(text,LOCPLINT->playerID);
		temp->pos.x=300-(temp->pos.w/2);
		temp->pos.y=300-(temp->pos.h/2);
		temp->owner = client;
		LOCPLINT->objsToBlit.push_back(temp);
	}
	else
	{
		for (int i=0;i<LOCPLINT->objsToBlit.size();i++)
		{
			//TODO: pewnie da sie to zrobic lepiej, ale nie chce mi sie. Wolajacy obiekt powinien informowac kogo spodziewa sie odwolac (null jesli down)
			CSimpleWindow * pom = dynamic_cast<CSimpleWindow*>(LOCPLINT->objsToBlit[i]);
			if (!pom)
				continue;
			if (pom->owner==client)
			{
				LOCPLINT->objsToBlit.erase(LOCPLINT->objsToBlit.begin()+(i));
				delete pom;
				if((LOCPLINT->curint == LOCPLINT->castleInt) && !LOCPLINT->castleInt->subInt)
					LOCPLINT->castleInt->showAll(0,true);
			}
		}
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

void CAdvMapInt::select(const CArmedInstance *sel )
{
	centerOn(sel->pos);
	selection = sel;
	if(sel->ID==98)
	{
		int pos = vstd::findPos(townList.items,sel);
		townList.selected = pos;
		terrain.currentPath = NULL;
	}
	else
	{
		int pos = heroList.getPosOfHero(sel);
		heroList.selected = pos;
		terrain.currentPath = heroList.items[pos].second;
	}
	townList.draw();
	heroList.draw();
	infoBar.draw(NULL);
}