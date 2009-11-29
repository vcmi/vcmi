#include "AdventureMapButton.h"
#include "CAdvmapInterface.h"
#include "../CCallback.h"
#include "CCastleInterface.h"
#include "CCursorHandler.h"
#include "CGameInfo.h"
#include "CHeroWindow.h"
#include "CMessage.h"
#include "SDL_Extensions.h"
#include "CBitmapHandler.h"
#include "CConfigHandler.h"
#include "CSpellWindow.h"
#include "Graphics.h"
#include "../hch/CDefHandler.h"
#include "../hch/CGeneralTextHandler.h"
#include "../hch/CHeroHandler.h"
#include "../hch/CObjectHandler.h"
#include "../hch/CTownHandler.h"
#include "../lib/map.h"
#include "../mapHandler.h"
#include "../stdafx.h"
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/assign/std/vector.hpp>
#include <boost/thread.hpp>
#include <sstream>
#include "CPreGame.h"
#include "../lib/VCMI_Lib.h"

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

extern TTF_Font * TNRB16, *TNR, *GEOR13, *GEORXX; //fonts

#define ADVOPT (conf.go()->ac)
using namespace boost::logic;
using namespace boost::assign;
using namespace CSDL_Ext;

CMinimap::CMinimap(bool draw)
{
	int3 mapSizes = LOCPLINT->cb->getMapSize();
	statusbarTxt = CGI->generaltexth->zelp[291].first;
	rcText = CGI->generaltexth->zelp[291].second;
	pos.x=ADVOPT.minimapX;//630
	pos.y=ADVOPT.minimapY;//26
	pos.h=ADVOPT.minimapW;//144
	pos.w=ADVOPT.minimapH;//144

	const int tilesw=(ADVOPT.advmapW+31)/32;
	const int tilesh=(ADVOPT.advmapH+31)/32;

	int rx = (((float)tilesw)/(mapSizes.x))*((float)pos.w),
		ry = (((float)tilesh)/(mapSizes.y))*((float)pos.h);

	radar = newSurface(rx,ry);
	temps = newSurface(pos.w,pos.h);
	SDL_FillRect(radar,NULL,0x00FFFF);
	for (int i=0; i<radar->w; i++)
	{
		if (i%4 || (i==0))
		{
			SDL_PutPixelWithoutRefresh(radar,i,0,255,75,125);
			SDL_PutPixelWithoutRefresh(radar,i,radar->h-1,255,75,125);
		}
	}
	for (int i=0; i<radar->h; i++)
	{
		if ((i%4) || (i==0))
		{
			SDL_PutPixelWithoutRefresh(radar,0,i,255,75,125);
			SDL_PutPixelWithoutRefresh(radar,radar->w-1,i,255,75,125);
		}
	}
	SDL_SetColorKey(radar,SDL_SRCCOLORKEY,SDL_MapRGB(radar->format,0,255,255));

	//radar = CDefHandler::giveDef("RADAR.DEF");
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
	SDL_FreeSurface(radar);
	SDL_FreeSurface(temps);
}
void CMinimap::draw(SDL_Surface * to)
{
	int3 mapSizes = LOCPLINT->cb->getMapSize();
	//draw terrain
	blitAt(map[LOCPLINT->adventureInt->position.z],0,0,temps);

	//draw heroes
	std::vector <const CGHeroInstance *> hh = LOCPLINT->cb->getHeroesInfo(false);
	int mw = map[0]->w, mh = map[0]->h,
		wo = mw/mapSizes.x, ho = mh/mapSizes.y;

	for (size_t i=0; i < hh.size(); ++i)
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
				SDL_PutPixelWithoutRefresh(temps,maplgp.x+ii,maplgp.y+jj,graphics->playerColors[hh[i]->getOwner()].r,
						graphics->playerColors[hh[i]->getOwner()].g,graphics->playerColors[hh[i]->getOwner()].b);
			}
		}
	}

	blitAt(flObjs[LOCPLINT->adventureInt->position.z],0,0,temps);

	blitAt(FoW[LOCPLINT->adventureInt->position.z],0,0,temps);

	//draw radar
	int bx = (((float)LOCPLINT->adventureInt->position.x)/(((float)mapSizes.x)))*pos.w,
		by = (((float)LOCPLINT->adventureInt->position.y)/(((float)mapSizes.y)))*pos.h;
	blitAt(radar,bx,by,temps);
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
	LOCPLINT->adventureInt->handleRightClick(rcText,down,this);
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
	newCPos.z = LOCPLINT->adventureInt->position.z;
	LOCPLINT->adventureInt->centerOn(newCPos);
}

void CMinimap::hover (bool on)
{
	//Hoverable::hover(on);
	if (on)
		LOCPLINT->adventureInt->statusbar.print(statusbarTxt);
	else if (LOCPLINT->adventureInt->statusbar.current==statusbarTxt)
		LOCPLINT->adventureInt->statusbar.clear();
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
}

void CMinimap::show( SDL_Surface * to )
{

}

CTerrainRect::CTerrainRect()
	:currentPath(NULL), curHoveredTile(-1,-1,-1)
{
	tilesw=(ADVOPT.advmapW+31)/32;
	tilesh=(ADVOPT.advmapH+31)/32;
	pos.x=ADVOPT.advmapX;
	pos.y=ADVOPT.advmapY;
	pos.w=ADVOPT.advmapW;
	pos.h=ADVOPT.advmapH;
	moveX = moveY = 0;
	arrows = CDefHandler::giveDef("ADAG.DEF");
	for(size_t y=0; y < arrows->ourImages.size(); ++y)
	{
		CSDL_Ext::alphaTransform(arrows->ourImages[y].bitmap);
	}
}
CTerrainRect::~CTerrainRect()
{
	delete arrows;
}
void CTerrainRect::activate()
{
	activateLClick();
	activateRClick();
	activateHover();
	activateMouseMove();
};
void CTerrainRect::deactivate()
{
	deactivateLClick();
	deactivateRClick();
	deactivateHover();
	deactivateMouseMove();
	curHoveredTile = int3(-1,-1,-1); //we lost info about hovered tile when disabling
};
void CTerrainRect::clickLeft(tribool down, bool previousState)
{
	if ((down==false) || indeterminate(down))
		return;
	int3 mp = whichTileIsIt();
	if (mp.x<0 || mp.y<0 || mp.x >= LOCPLINT->cb->getMapSize().x || mp.y >= LOCPLINT->cb->getMapSize().y)
		return;

	std::vector < const CGObjectInstance * > bobjs = LOCPLINT->cb->getBlockingObjs(mp),  //blocking objects at tile
		vobjs = LOCPLINT->cb->getVisitableObjs(mp); //visitable objects

	if (LOCPLINT->adventureInt->selection->ID != HEROI_TYPE) //hero is not selected (presumably town)
	{
		if(currentPath)
		{
			tlog2<<"Warning: Lost path?" << std::endl;
			//delete currentPath;
			currentPath = NULL;
		}

		for(size_t i=0; i < bobjs.size(); ++i)
		{
			if(bobjs[i]->ID == TOWNI_TYPE && bobjs[i]->getOwner() == LOCPLINT->playerID) //our town clicked
			{
				if(LOCPLINT->adventureInt->selection == (bobjs[i])) //selected town clicked
					LOCPLINT->openTownWindow(static_cast<const CGTownInstance*>(bobjs[i]));
				else
					LOCPLINT->adventureInt->select(static_cast<const CArmedInstance*>(bobjs[i]));

				return;
			}
			else if(bobjs[i]->ID == HEROI_TYPE && bobjs[i]->tempOwner == LOCPLINT->playerID) //hero clicked - select him
			{
				LOCPLINT->adventureInt->select(static_cast<const CArmedInstance*>(bobjs[i]));
				return;
			}
		}
	}

	else //hero is selected
	{
		bool townEntrance = false; //town entrance tile has been clicked?
		const CGHeroInstance * currentHero = static_cast<const CGHeroInstance*>(LOCPLINT->adventureInt->selection);

		for(size_t i=0; i < vobjs.size(); ++i)
		{
			if(vobjs[i]->ID == TOWNI_TYPE)
				townEntrance = true;
		}

		if(!townEntrance) //not entrance - select town or open hero window
		{
			for(size_t i=0; i < bobjs.size(); ++i)
			{
				if(bobjs[i]->ID == TOWNI_TYPE && bobjs[i]->tempOwner == LOCPLINT->playerID) //town - switch selection to it
				{
					LOCPLINT->adventureInt->select(static_cast<const CArmedInstance*>(bobjs[i]));
					return;
				}
				else if(bobjs[i]->ID == HEROI_TYPE //it's a hero
					&& bobjs[i]->tempOwner == LOCPLINT->playerID  //our hero (is this condition needed?)
					&& currentHero == (bobjs[i]) ) //and selected one 
				{
					LOCPLINT->openHeroWindow(currentHero);
					return;
				}
			}
		}

		//still here? we need to move hero if we clicked end of already selected path or calculate a new path otherwise
		if (currentPath  &&  currentPath->endPos() == mp)//we'll be moving
		{
			LOCPLINT->pim->unlock();
			LOCPLINT->moveHero(currentHero,*currentPath);
			LOCPLINT->pim->lock();
		}
		else if(mp.z == currentHero->pos.z) //remove old path and find a new one if we clicked on the map level on which hero is present
		{
			int3 bufpos = currentHero->getPosition(false);
			CGPath &path = LOCPLINT->adventureInt->paths[currentHero];
			currentPath = &path;
			if(!LOCPLINT->cb->getPath2(mp, path))
			{
				LOCPLINT->adventureInt->paths.erase(currentHero);
				currentPath = NULL;
			}
		}
	} //end of hero is selected "case"
}
void CTerrainRect::clickRight(tribool down, bool previousState)
{
	int3 mp = whichTileIsIt();

	if (!CGI->mh->map->isInTheMap(mp) || down != true)
		return;

	std::vector < const CGObjectInstance * > objs = LOCPLINT->cb->getBlockingObjs(mp);
	if(!objs.size()) 
	{
		// Bare or undiscovered terrain
		const TerrainTile * tile = LOCPLINT->cb->getTileInfo(mp);
		if (tile) 
		{
			CSimpleWindow * temp = CMessage::genWindow(VLC->generaltexth->terrainNames[tile->tertype],LOCPLINT->playerID,true);
			CRClickPopupInt *rcpi = new CRClickPopupInt(temp,true);
			GH.pushInt(rcpi);
		}
		return;
	}

	const CGObjectInstance * obj = objs.back();
	switch(obj->ID)
	{
	case HEROI_TYPE:
		{
			if(!vstd::contains(graphics->heroWins,obj->subID) || obj->tempOwner != LOCPLINT->playerID)
			{
				InfoAboutHero iah;
				if(LOCPLINT->cb->getHeroInfo(obj, iah))
				{
					SDL_Surface *iwin = graphics->drawHeroInfoWin(iah);
					CInfoPopup * ip = new CInfoPopup(iwin, GH.current->motion.x-iwin->w,
													  GH.current->motion.y-iwin->h, true);
					GH.pushInt(ip);
				}
				else
				{
					tlog3 << "Warning - no infowin for hero " << obj->id << std::endl;
				}
			}
			else
			{
				CInfoPopup * ip = new CInfoPopup(graphics->heroWins[obj->subID],
					GH.current->motion.x-graphics->heroWins[obj->subID]->w,
					GH.current->motion.y-graphics->heroWins[obj->subID]->h,false
					);
				GH.pushInt(ip);
			}
			break;
		}
	case TOWNI_TYPE:
		{
			if(!vstd::contains(graphics->townWins,obj->id) || obj->tempOwner != LOCPLINT->playerID)
			{
				InfoAboutTown iah;
				if(LOCPLINT->cb->getTownInfo(obj, iah))
				{
					SDL_Surface *iwin = graphics->drawTownInfoWin(iah);
					CInfoPopup * ip = new CInfoPopup(iwin, GH.current->motion.x - iwin->w/2,
						GH.current->motion.y - iwin->h/2, true);
					GH.pushInt(ip);
				}
				else
				{
					tlog3 << "Warning - no infowin for town " << obj->id << std::endl;
				}
			}
			else
			{
				CInfoPopup * ip = new CInfoPopup(graphics->townWins[obj->id],
					GH.current->motion.x - graphics->townWins[obj->id]->w/2,
					GH.current->motion.y - graphics->townWins[obj->id]->h/2,false
					);
				GH.pushInt(ip);
			}
			break;
		}
	case 33: // Garrison
	case 219:
		{
			const CGGarrison *garr = dynamic_cast<const CGGarrison *>(obj);

			if (garr != NULL) {
				InfoAboutTown iah;

				iah.obj = garr;
				iah.fortLevel = 0;
				iah.army = garr->army;
				iah.name = VLC->generaltexth->names[33]; // "Garrison"
				iah.owner = garr->tempOwner;
				iah.built = false;
				iah.tType = NULL;

				// Show detailed info only to owning player.
				if (garr->tempOwner == LOCPLINT->playerID) {
					iah.details = new InfoAboutTown::Details;
					iah.details->customRes = false;
					iah.details->garrisonedHero = false;
					iah.details->goldIncome = -1;
					iah.details->hallLevel = -1;
				}

				SDL_Surface *iwin = graphics->drawTownInfoWin(iah);
				CInfoPopup * ip = new CInfoPopup(iwin,
					GH.current->motion.x - iwin->w/2,
					GH.current->motion.y - iwin->h/2, true);
				GH.pushInt(ip);
			}
			break;
		}
	default:
		{
			LOCPLINT->adventureInt->handleRightClick(obj->getHoverText(),down,this);
			break;
		}
	}
}
void CTerrainRect::mouseMoved (const SDL_MouseMotionEvent & sEvent)
{
	int3 tHovered = whichTileIsIt(sEvent.x,sEvent.y);
	int3 pom = LOCPLINT->adventureInt->verifyPos(tHovered);

	if(tHovered != pom) //tile outside the map
	{
		CGI->curh->changeGraphic(0, 0);
		return;
	}

	if (pom != curHoveredTile)
		curHoveredTile=pom;
	else
		return;

	std::vector<std::string> temp = LOCPLINT->cb->getObjDescriptions(pom);
	if (temp.size())
	{
		boost::replace_all(temp.back(),"\n"," ");
		LOCPLINT->adventureInt->statusbar.print(temp.back());
	}
	else
	{
		LOCPLINT->adventureInt->statusbar.clear();
	}

	const CGPathNode *pnode = LOCPLINT->cb->getPathInfo(pom);
	std::vector<const CGObjectInstance *> objs = LOCPLINT->cb->getBlockingObjs(pom); 
	const CGObjectInstance *obj = objs.size() ? objs.back() : NULL;
	bool accessible  =  pnode->turns < 255;

	int turns = pnode->turns;
	amin(turns, 4);

	if(LOCPLINT->adventureInt->selection->ID == TOWNI_TYPE)
	{
		if(obj)
		{
			if(obj->ID == TOWNI_TYPE)
			{
				CGI->curh->changeGraphic(0, 3);
			}
			else if(obj->ID == HEROI_TYPE)
			{
				CGI->curh->changeGraphic(0, 2);
			}
		}
		else
		{
			CGI->curh->changeGraphic(0, 0);
		}
	}
	else if(LOCPLINT->adventureInt->selection->ID == HEROI_TYPE)
	{
		const CGHeroInstance *h = static_cast<const CGHeroInstance *>(LOCPLINT->adventureInt->selection);
		if(obj)
		{
			if(obj->ID == HEROI_TYPE)
			{
				if(obj->tempOwner != LOCPLINT->playerID) //enemy hero TODO: allies
				{
					if(accessible)
						CGI->curh->changeGraphic(0, 5 + turns*6);
					else
						CGI->curh->changeGraphic(0, 0);
				}
				else //our hero
				{
					if(LOCPLINT->adventureInt->selection == obj)
						CGI->curh->changeGraphic(0, 2);
					else if(accessible)
						CGI->curh->changeGraphic(0, 8 + turns*6);
					else
						CGI->curh->changeGraphic(0, 2);
				}
			}
			else if(obj->ID == TOWNI_TYPE)
			{
				if(obj->tempOwner != LOCPLINT->playerID) //enemy town TODO: allies
				{
					if(accessible) {
						const CGTownInstance* townObj = dynamic_cast<const CGTownInstance*>(obj);

						// Show movement cursor for unguarded enemy towns, otherwise attack cursor.
						if (townObj && townObj->army.slots.empty())
							CGI->curh->changeGraphic(0, 9 + turns*6);
						else
							CGI->curh->changeGraphic(0, 5 + turns*6);
							
					} else {
						CGI->curh->changeGraphic(0, 0);
					}
				}
				else //our town
				{
					if(accessible)
						CGI->curh->changeGraphic(0, 9 + turns*6);
					else
						CGI->curh->changeGraphic(0, 3);
				}
			}
			else if(obj->ID == 54) //monster
			{
				if(accessible)
					CGI->curh->changeGraphic(0, 5 + turns*6);
				else
					CGI->curh->changeGraphic(0, 0);
			}
			else if(obj->ID == 8) //boat
			{
				if(accessible)
					CGI->curh->changeGraphic(0, 6 + turns*6);
				else
					CGI->curh->changeGraphic(0, 0);
			}
			else if (obj->ID == 33 || obj->ID == 219) // Garrison
			{
				if (accessible) {
					const CGGarrison* garrObj = dynamic_cast<const CGGarrison*>(obj);

					// Show battle cursor for guarded enemy garrisons, otherwise movement cursor.
					if (garrObj && garrObj->tempOwner != LOCPLINT->playerID
						&& !garrObj->army.slots.empty())
					{
						CGI->curh->changeGraphic(0, 5 + turns*6);
					}
					else
					{
						CGI->curh->changeGraphic(0, 9 + turns*6);
					}
				} else {
					CGI->curh->changeGraphic(0, 0);
				}
			}
			else
			{
				if(accessible)
				{
					if(pnode->land)
						CGI->curh->changeGraphic(0, 9 + turns*6);
					else
						CGI->curh->changeGraphic(0, 28 + turns);
				}
				else
					CGI->curh->changeGraphic(0, 0);
			}
		} 
		else //no objs 
		{
			if(accessible)
			{
				if(pnode->land)
				{
					if(LOCPLINT->cb->getTileInfo(h->getPosition(false))->tertype != TerrainTile::water)
						CGI->curh->changeGraphic(0, 4 + turns*6);
					else
						CGI->curh->changeGraphic(0, 7 + turns*6); //anchor
				}
				else
					CGI->curh->changeGraphic(0, 6 + turns*6);
			}
			else
				CGI->curh->changeGraphic(0, 0);
		}
	}

	//tlog1 << "Tile " << pom << ": Turns=" << (int)pnode->turns <<"  Move:=" << pnode->moveRemains <</* " (from  "  << ")" << */std::endl;
}
void CTerrainRect::hover(bool on)
{
	if (!on)
	{
		LOCPLINT->adventureInt->statusbar.clear();
		CGI->curh->changeGraphic(0,0);
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
			int x = 32*(currentPath->nodes[i].coord.x-LOCPLINT->adventureInt->position.x)+CGI->mh->offsetX + pos.x,
				y = 32*(currentPath->nodes[i].coord.y-LOCPLINT->adventureInt->position.y)+CGI->mh->offsetY + pos.y;
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
			int x = 32*(currentPath->nodes[i].coord.x-LOCPLINT->adventureInt->position.x)+CGI->mh->offsetX + pos.x,
				y = 32*(currentPath->nodes[i].coord.y-LOCPLINT->adventureInt->position.y)+CGI->mh->offsetY + pos.y;
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
					CSDL_Ext::blit8bppAlphaTo24bpp(arrows->ourImages[pn].bitmap, NULL, to, &genRect(32, 32, x + moveX, y + moveY));
				}
				else if(hvx<0)
				{
					CSDL_Ext::blit8bppAlphaTo24bpp
						(arrows->ourImages[pn].bitmap,&genRect(arrows->ourImages[pn].bitmap->h-hvy, arrows->ourImages[pn].bitmap->w, 0, 0),
						to, &genRect(arrows->ourImages[pn].bitmap->h-hvy, arrows->ourImages[pn].bitmap->w, x + moveX, y + moveY));
				}
				else if (hvy<0)
				{
					CSDL_Ext::blit8bppAlphaTo24bpp
						(arrows->ourImages[pn].bitmap,&genRect(arrows->ourImages[pn].bitmap->h, arrows->ourImages[pn].bitmap->w-hvx, 0, 0),
						to, &genRect(arrows->ourImages[pn].bitmap->h, arrows->ourImages[pn].bitmap->w-hvx, x + moveX, y + moveY));
				}
				else
				{
					CSDL_Ext::blit8bppAlphaTo24bpp
						(arrows->ourImages[pn].bitmap, &genRect(arrows->ourImages[pn].bitmap->h-hvy,arrows->ourImages[pn].bitmap->w-hvx, 0, 0),
						to, &genRect(arrows->ourImages[pn].bitmap->h-hvy, arrows->ourImages[pn].bitmap->w-hvx, x + moveX, y + moveY));
				}
			}
			else //standard version
			{
				if (hvx<0 && hvy<0)
				{
					CSDL_Ext::blit8bppAlphaTo24bpp(arrows->ourImages[pn].bitmap, NULL, to, &genRect(32, 32, x, y));
				}
				else if(hvx<0)
				{
					CSDL_Ext::blit8bppAlphaTo24bpp
						(arrows->ourImages[pn].bitmap,&genRect(arrows->ourImages[pn].bitmap->h-hvy, arrows->ourImages[pn].bitmap->w, 0, 0),
						to, &genRect(arrows->ourImages[pn].bitmap->h-hvy, arrows->ourImages[pn].bitmap->w, x, y));
				}
				else if (hvy<0)
				{
					CSDL_Ext::blit8bppAlphaTo24bpp
						(arrows->ourImages[pn].bitmap,&genRect(arrows->ourImages[pn].bitmap->h, arrows->ourImages[pn].bitmap->w-hvx, 0, 0),
						to, &genRect(arrows->ourImages[pn].bitmap->h, arrows->ourImages[pn].bitmap->w-hvx, x, y));
				}
				else
				{
					CSDL_Ext::blit8bppAlphaTo24bpp
						(arrows->ourImages[pn].bitmap, &genRect(arrows->ourImages[pn].bitmap->h-hvy,arrows->ourImages[pn].bitmap->w-hvx, 0, 0),
						to, &genRect(arrows->ourImages[pn].bitmap->h-hvy, arrows->ourImages[pn].bitmap->w-hvx, x, y));
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
			(LOCPLINT->adventureInt->position, LOCPLINT->adventureInt->anim,
			 &LOCPLINT->cb->getVisibilityMap(), true, LOCPLINT->adventureInt->heroAnim,
			 to, &pos, moveX, moveY, false);
	else
		CGI->mh->terrainRect
			(LOCPLINT->adventureInt->position, LOCPLINT->adventureInt->anim,
			 &LOCPLINT->cb->getVisibilityMap(), true, LOCPLINT->adventureInt->heroAnim,
			 to, &pos, 0, 0, false);
	
	//SDL_BlitSurface(teren,&genRect(pos.h,pos.w,0,0),screen,&genRect(547,594,7,6));
	//SDL_FreeSurface(teren);
	if (currentPath && LOCPLINT->adventureInt->position.z==currentPath->startPos().z) //drawing path
	{
		showPath(&pos, to);
	}
}

int3 CTerrainRect::whichTileIsIt(const int & x, const int & y)
{
	int3 ret;
	ret.x = LOCPLINT->adventureInt->position.x + ((GH.current->motion.x-CGI->mh->offsetX-pos.x)/32);
	ret.y = LOCPLINT->adventureInt->position.y + ((GH.current->motion.y-CGI->mh->offsetY-pos.y)/32);
	ret.z = LOCPLINT->adventureInt->position.z;
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
	pos = genRect(bg->h,bg->w,x,y);

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
		printAt(buf,txtpos[i].first,txtpos[i].second,GEOR13,zwykly,to);
	}
	std::vector<std::string> temp;
	SDL_itoa(LOCPLINT->cb->getDate(3),buf,10); temp+=std::string(buf);
	SDL_itoa(LOCPLINT->cb->getDate(2),buf,10); temp+=std::string(buf);
	SDL_itoa(LOCPLINT->cb->getDate(1),buf,10); temp+=std::string(buf);
	printAt(processStr(datetext,temp),txtpos[7].first,txtpos[7].second,GEOR13,zwykly,to);
	temp.clear();
	//updateRect(&pos,screen);
	delete[] buf;
}

void CResDataBar::show( SDL_Surface * to )
{

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
}
CInfoBar::~CInfoBar()
{
	delete day;
	delete week1;
	delete week2;
	delete week3;
	delete week4;
}
void CInfoBar::draw(SDL_Surface * to, const CGObjectInstance * specific)
{
	if ((mode>=0) && mode<5)
	{
		blitAnim(mode);
		return;
	}
	else if (mode==5)
	{
		mode = -1;
		draw(to,LOCPLINT->adventureInt->selection);
	}
	if (!specific)
		specific = LOCPLINT->adventureInt->selection;

	if(!specific)
		return;

	if(specific->ID == HEROI_TYPE) //hero
	{
		if(graphics->heroWins.find(specific->subID)!=graphics->heroWins.end())
			blitAt(graphics->heroWins[specific->subID],pos.x,pos.y,to);
	}
	else if (specific->ID == TOWNI_TYPE)
	{
		const CGTownInstance * t = static_cast<const CGTownInstance*>(specific);
		if(graphics->townWins.find(t->id)!=graphics->townWins.end())
			blitAt(graphics->townWins[t->id],pos.x,pos.y,to);
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
	printAtMiddle(txt.str(),pos.x+95,pos.y+31,TNRB16,zwykly);
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
	SDL_Surface * b = BitmapHandler::loadBitmap("ADSTATOT.bmp");
	blitAt(b,pos.x+8,pos.y+11);
	blitAt(comp->getImg(),pos.x+52,pos.y+54);
	printAtMiddle(comp->subtitle,pos.x+91,pos.y+158,GEOR13,zwykly);
	printAtMiddleWB(comp->description,pos.x+94,pos.y+31,GEOR13,26,zwykly);
	SDL_FreeSurface(b);
	activateTimer();
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
			deactivateTimer();
			toNextTick = -1;
			mode = 5;
			draw(screen2);
			return;
		}
		toNextTick = 150;
		blitAnim(mode);
	}
	else if (mode == 6)
	{
		deactivateTimer();
		toNextTick = -1;
		mode = 5;
		draw(screen2);
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

CAdvMapInt::CAdvMapInt(int Player)
:player(Player),
statusbar(ADVOPT.statusbarX,ADVOPT.statusbarY,ADVOPT.statusbarG),
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
	pos.x = pos.y = 0;
	pos.w = screen->w;
	pos.h = screen->h;
	active = 0;
	selection = NULL;
	townList.fun = boost::bind(&CAdvMapInt::selectionChanged,this);
	LOCPLINT->adventureInt=this;
	bg = BitmapHandler::loadBitmap(ADVOPT.mainGraphic);
	graphics->blueToPlayersAdv(bg,player);
	scrollingDir = 0;
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

	gems.push_back(CDefHandler::giveDef(ADVOPT.gemG[0]));
	gems.push_back(CDefHandler::giveDef(ADVOPT.gemG[1]));
	gems.push_back(CDefHandler::giveDef(ADVOPT.gemG[2]));
	gems.push_back(CDefHandler::giveDef(ADVOPT.gemG[3]));
}

CAdvMapInt::~CAdvMapInt()
{
	SDL_FreeSurface(bg);
	delete heroWindow;

	for(int i=0; i<gems.size(); i++)
		delete gems[i];
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
		underground.show(screenBuf);
	}
	else
	{
		underground.curimg=1;
		position.z++;
		underground.show(screenBuf);
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
	if (selection->ID!=HEROI_TYPE)
		return;
	if (!terrain.currentPath)
		return;

	LOCPLINT->pim->unlock();
	LOCPLINT->moveHero(static_cast<const CGHeroInstance*>(LOCPLINT->adventureInt->selection),*terrain.currentPath);
	LOCPLINT->pim->lock();
}

void CAdvMapInt::fshowSpellbok()
{
	if (selection->ID!=HEROI_TYPE) //checking necessary values
		return;


	CSpellWindow * spellWindow = new CSpellWindow(genRect(595, 620, (conf.cc.resx - 620)/2, (conf.cc.resy - 595)/2), (static_cast<const CGHeroInstance*>(LOCPLINT->adventureInt->selection)), false);
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
	LOCPLINT->makingTurn = false;
}

void CAdvMapInt::activate()
{
	if(active++)
	{
		tlog1 << "Error: advmapint already active...\n";
		active--;
		return;
	}
	screenBuf = screen;
	LOCPLINT->statusbar = &statusbar;
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

	LOCPLINT->cingconsole->activate();
	GH.fakeMouseMove(); //to restore the cursor
}
void CAdvMapInt::deactivate()
{
	deactivateMouseMove();
	scrollingDir = 0;

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
	infoBar.deactivate();
	infoBar.mode=-1;

	LOCPLINT->cingconsole->deactivate();

	if(--active)
	{
		tlog1 << "Error: advmapint still active...\n";
		deactivate();
	}
}
void CAdvMapInt::showAll(SDL_Surface *to)
{
	blitAt(bg,0,0,to);

	kingOverview.show(to);
	underground.show(to);
	questlog.show(to);
	sleepWake.show(to);
	moveHero.show(to);
	spellbook.show(to);
	advOptions.show(to);
	sysOptions.show(to);
	nextHero.show(to);
	endTurn.show(to);

	minimap.draw(to);
	heroList.draw(to);
	townList.draw(to);
	updateScreen = true;
	show(to);

	resdatabar.draw(to);

	statusbar.show(to);

	infoBar.draw(to);
	LOCPLINT->cingconsole->show(to);
}
void CAdvMapInt::show(SDL_Surface *to)
{
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
		&& 
			(GH.topInt() == this)
			|| SDL_GetKeyState(NULL)[SDLK_LCTRL] 
			|| SDL_GetKeyState(NULL)[SDLK_RCTRL]
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
	const CGTownInstance *to = townList.items[townList.selected];
	select(to);
}
void CAdvMapInt::centerOn(int3 on)
{
	// TODO:convertPosition should not belong to CGHeroInstance, and it
	// should be split in 2 methods.
	on = CGHeroInstance::convertPosition(on, false);

	on.x -= CGI->mh->frameW;
	on.y -= CGI->mh->frameH;
	
	on = LOCPLINT->repairScreenPos(on);

	LOCPLINT->adventureInt->position = on;
	LOCPLINT->adventureInt->updateScreen=true;
	updateMinimap=true;
	underground.curimg = on.z; //change underground switch button image 
	if(GH.topInt() == this)
		underground.redraw();
}
void CAdvMapInt::keyPressed(const SDL_KeyboardEvent & key)
{
	ui8 Dir = 0;
	int k = key.keysym.sym;
	switch(k)
	{
	case SDLK_i: 
		if(active)
			CAdventureOptions::showScenarioInfo();
		return;
	case SDLK_s: 
		if(active)
			GH.pushInt(new CSelectionScreen(saveGame));
		return;
	case SDLK_SPACE: //space - try to revisit current object with selected hero
		{
			if(!active) 
				return;
			const CGHeroInstance *h = dynamic_cast<const CGHeroInstance*>(selection);
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
			if(!active || !selection || key.state != SDL_PRESSED) 
				return;
			if(selection->ID == HEROI_TYPE)
				LOCPLINT->openHeroWindow(static_cast<const CGHeroInstance*>(selection));
			else if(selection->ID == TOWNI_TYPE)
				LOCPLINT->openTownWindow(static_cast<const CGTownInstance*>(selection));
			return;
		}
	case SDLK_t:
		{
			//act on key down if marketplace windows is not already opened
			if(key.state != SDL_PRESSED  || GH.topInt()->type & BLOCK_ADV_HOTKEYS) return;

			//check if we have aby marketplace
			std::vector<const CGTownInstance*> towns = LOCPLINT->cb->getTownsInfo();
			size_t i = 0;
			for(; i<towns.size(); i++)
				if(vstd::contains(towns[i]->builtBuildings, 14))
					break;

			if(i != towns.size()) //if any town has marketplace, open window
				GH.pushInt(new CMarketplaceWindow); 
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

			if(!active || LOCPLINT->ctrlPressed())//ctrl makes arrow move screen, not hero
				break;

			k -= SDLK_KP0 + 1;
			if(k < 0 || k > 8 || key.state != SDL_PRESSED)
				return;


			const CGHeroInstance *h = dynamic_cast<const CGHeroInstance *>(selection);
			if(!h) break;
			if(k == 4)
			{
				centerOn(h->getPosition(false));
				return;
			}

			int3 dir = directions[k];

			CGPath &path = paths[h];
			terrain.currentPath = &path;
			if(!LOCPLINT->cb->getPath2(h->getPosition(false) + dir, path))
			{
				terrain.currentPath = NULL;
				return;
			}

			if(!path.nodes[0].turns)
			{
				LOCPLINT->pim->unlock();
				LOCPLINT->moveHero(h, path);
				LOCPLINT->pim->lock();
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
void CAdvMapInt::handleRightClick(std::string text, tribool down, CIntObject * client)
{
	if (down)
	{
		CSimpleWindow * temp = CMessage::genWindow(text,LOCPLINT->playerID,true);
		CRClickPopupInt *rcpi = new CRClickPopupInt(temp,true);
		GH.pushInt(rcpi);
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
	LOCPLINT->cb->setSelection(sel);

	centerOn(sel->pos);
	selection = sel;

	terrain.currentPath = NULL;
	if(sel->ID==TOWNI_TYPE)
	{
		int pos = vstd::findPos(townList.items,sel);
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

		if(vstd::contains(paths,h)) //hero has assigned path
		{
			CGPath &path = paths[h];
			assert(h->getPosition(false) == path.startPos()); 
			//update the hero path in case of something has changed on map
			if(LOCPLINT->cb->getPath2(path.endPos(), path))
				terrain.currentPath = &path;
			else
				paths.erase(h);
		}
	}
	townList.draw(screen);
	heroList.draw(screen);
	infoBar.draw(screen);
}

void CAdvMapInt::mouseMoved( const SDL_MouseMotionEvent & sEvent )
{
	//adventure map scrolling with mouse
	if(!SDL_GetKeyState(NULL)[SDLK_LCTRL]  &&  active)
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

CAdventureOptions::CAdventureOptions()
{
	OBJ_CONSTRUCTION_CAPTURING_ALL;
	bg = new CPicture(BitmapHandler::loadBitmap("ADVOPTS.bmp"), 0, 0);
	graphics->blueToPlayersAdv(bg->bg, LOCPLINT->playerID);
	pos = bg->center();
	exit = new AdventureMapButton("","",boost::bind(&CGuiHandler::popIntTotally, &GH, this), 204, 313, "IOK6432.DEF",SDLK_RETURN);

	//scenInfo = new AdventureMapButton("","", boost::bind(&CGuiHandler::popIntTotally, &GH, this), 24, 24, "ADVINFO.DEF",SDLK_i);
	scenInfo = new AdventureMapButton("","", boost::bind(&CGuiHandler::popIntTotally, &GH, this), 24, 198, "ADVINFO.DEF",SDLK_i);
	scenInfo->callback += CAdventureOptions::showScenarioInfo;
	//viewWorld = new AdventureMapButton("","",boost::bind(&CGuiHandler::popIntTotally, &GH, this), 204, 313, "IOK6432.DEF",SDLK_RETURN);

	puzzle = new AdventureMapButton("","", boost::bind(&CGuiHandler::popIntTotally, &GH, this), 24, 81, "ADVPUZ.DEF");;
	puzzle->callback += CAdventureOptions::showPuzzleMap;
}

CAdventureOptions::~CAdventureOptions()
{
}

void CAdventureOptions::showScenarioInfo()
{
	GH.pushInt(new CScenarioInfo(LOCPLINT->cb->getMapHeader(), LOCPLINT->cb->getStartInfo()));
}

void CAdventureOptions::showPuzzleMap()
{
	GH.pushInt(new CPuzzleWindow());
}
