#include "stdafx.h"
#include "CAdvmapInterface.h"
#include "hch\CLodHandler.h"
#include "hch\CPreGameTextHandler.h"
#include "hch\CGeneralTextHandler.h"
#include "hch\CTownHandler.h"
#include "CPathfinder.h"
#include "CGameInfo.h"
#include "SDL_Extensions.h"
#include "CCallback.h"
#include <boost/assign/std/vector.hpp>
#include "mapHandler.h"
extern TTF_Font * TNRB16, *TNR, *GEOR13, *GEORXX; //fonts

using namespace boost::logic;
using namespace boost::assign;
using namespace CSDL_Ext;
CAdvMapInt::~CAdvMapInt()
{
	SDL_FreeSurface(bg);
}

AdventureMapButton::AdventureMapButton ()
{
	type=2;
	abs=true;
	active=false;
	ourObj=NULL;
	state=0;
}
AdventureMapButton::AdventureMapButton
( std::string Name, std::string HelpBox, void(CAdvMapInt::*Function)(), int x, int y, std::string defName, bool activ, std::vector<std::string> * add )
{
	type=2;
	abs=true;
	active=false;
	ourObj=NULL;
	state=0;
	name=Name;
	helpBox=HelpBox;
	int est = LOCPLINT->playerID;
	CDefHandler * temp = CGI->spriteh->giveDef(defName); //todo: moze cieknac
	for (int i=0;i<temp->ourImages.size();i++)
	{
		imgs.resize(1);
		imgs[0].push_back(temp->ourImages[i].bitmap);
		blueToPlayersAdv(imgs[curimg][i],LOCPLINT->playerID);
	}
	if (add)
	{
		imgs.resize(imgs.size()+add->size());
		for (int i=0; i<add->size();i++)
		{
			temp = CGI->spriteh->giveDef((*add)[i]);
			for (int j=0;j<temp->ourImages.size();j++)
			{
				imgs[i+1].push_back(temp->ourImages[j].bitmap);
				blueToPlayersAdv(imgs[1+i][j],LOCPLINT->playerID);
			}
		}
		delete add;
	}
	function = Function;
	pos.x=x;
	pos.y=y;
	pos.w = imgs[curimg][0]->w;
	pos.h = imgs[curimg][0]->h  -1;
	if (activ)
		activate();
}

void AdventureMapButton::clickLeft (tribool down)
{
	if (down)
	{
		state=1;
	}
	else 
	{
		state=0;
	}
	show();
	if (pressedL && (down==false))
		(LOCPLINT->adventureInt->*function)();
	pressedL=state;
}
void AdventureMapButton::clickRight (tribool down)
{
	//TODO: show/hide infobox
}
void AdventureMapButton::hover (bool on)
{
	Hoverable::hover(on);
	if (on)
		LOCPLINT->adventureInt->statusbar.print(name);
	else if (LOCPLINT->adventureInt->statusbar.current==name)
		LOCPLINT->adventureInt->statusbar.clear();
}
void AdventureMapButton::activate()
{
	if (active) return;
	active=true;
	ClickableL::activate();
	Hoverable::activate();
	KeyInterested::activate();
}
void AdventureMapButton::keyPressed (SDL_KeyboardEvent & key)
{
	//TODO: check if it's shortcut
}
void AdventureMapButton::deactivate()
{
	if (!active) return;
	active=false;
	ClickableL::deactivate();
	Hoverable::deactivate();
	KeyInterested::deactivate();
}

void CList::activate()
{
	ClickableL::activate();
	ClickableR::activate();
	Hoverable::activate();
	KeyInterested::activate();
	MotionInterested::activate();
}; 
void CList::deactivate()
{
	ClickableL::deactivate();
	ClickableR::deactivate();
	Hoverable::deactivate();
	KeyInterested::deactivate();
	MotionInterested::deactivate();
}; 
void CList::clickLeft(tribool down)
{
};
CHeroList::CHeroList()
{
	pos = genRect(192,64,609,196);
	
	arrupp = genRect(16,64,609,196);
	arrdop = genRect(16,64,609,372);
 //32px per hero
	posmobx = 610;
	posmoby = 213;
	posporx = 617;
	pospory = 212;
	posmanx = 666;
	posmany = 213;
	
	arrup = CGI->spriteh->giveDef("IAM012.DEF");
	arrdo = CGI->spriteh->giveDef("IAM013.DEF");
	mobile = CGI->spriteh->giveDef("IMOBIL.DEF");
	mana = CGI->spriteh->giveDef("IMANA.DEF");
	empty = CGI->bitmaph->loadBitmap("HPSXXX.bmp");
	selection = CGI->bitmaph->loadBitmap("HPSYYY.bmp");
	SDL_SetColorKey(selection,SDL_SRCCOLORKEY,SDL_MapRGB(selection->format,0,255,255));
	from = 0;
	pressed = indeterminate;
}

void CHeroList::init()
{
	bg = CSDL_Ext::newSurface(68,193,ekran);
	SDL_BlitSurface(LOCPLINT->adventureInt->bg,&genRect(193,68,607,196),bg,&genRect(193,68,0,0));
}
void CHeroList::genList()
{
	int howMany = LOCPLINT->cb->howManyHeroes(LOCPLINT->playerID);
	for (int i=0;i<howMany;i++)
	{
		items.push_back(std::pair<const CHeroInstance *,CPath *>(LOCPLINT->cb->getHeroInfo(LOCPLINT->playerID,i,0),NULL));
	}
}
void CHeroList::select(int which)
{
	selected = which;
	if (which>=items.size()) 
		return;
	LOCPLINT->adventureInt->centerOn(items[which].first->pos);
	LOCPLINT->adventureInt->selection.type = HEROI_TYPE;
	LOCPLINT->adventureInt->selection.selected = items[which].first;
	LOCPLINT->adventureInt->terrain.currentPath = items[which].second;
	draw();
	LOCPLINT->adventureInt->townList.draw();
}
void CHeroList::clickLeft(tribool down)
{
	if (down)
	{
		/***************************ARROWS*****************************************/
		if(isItIn(&arrupp,LOCPLINT->current->motion.x,LOCPLINT->current->motion.y) && from>0)
		{
			blitAtWR(arrup->ourImages[1].bitmap,arrupp.x,arrupp.y);
			pressed = true;
			return;
		}
		else if(isItIn(&arrdop,LOCPLINT->current->motion.x,LOCPLINT->current->motion.y) && (items.size()-from>5))
		{
			blitAtWR(arrdo->ourImages[1].bitmap,arrdop.x,arrdop.y);
			pressed = false;
			return;
		}
		/***************************HEROES*****************************************/
		int hx = LOCPLINT->current->motion.x, hy = LOCPLINT->current->motion.y;
		hx-=pos.x;
		hy-=pos.y; hy-=arrup->ourImages[0].bitmap->h;
		float ny = (float)hy/(float)32;
		if (ny>5 || ny<0)
			return;
		select(ny+from);
	}
	else
	{
		if (indeterminate(pressed))
			return;
		if (pressed) //up
		{
			blitAtWR(arrup->ourImages[0].bitmap,arrupp.x,arrupp.y);
			pressed = indeterminate;
			if (!down)
			{
				from--;
				if (from<0)
					from=0;
				draw();
			}
		}
		else if (!pressed) //down
		{
			blitAtWR(arrdo->ourImages[0].bitmap,arrdop.x,arrdop.y);
			pressed = indeterminate;
			if (!down)
			{
				from++;
				if (from<items.size()-5)
					from=items.size()-5;
				draw();
			}
		}
		else
			throw 0;

	}
}
void CHeroList::mouseMoved (SDL_MouseMotionEvent & sEvent)
{
	if(isItIn(&arrupp,LOCPLINT->current->motion.x,LOCPLINT->current->motion.y))
	{
		if (from>0)
			LOCPLINT->adventureInt->statusbar.print(CGI->preth->advHListUp.first);
		else
			LOCPLINT->adventureInt->statusbar.clear();
		return;
	}
	else if(isItIn(&arrdop,LOCPLINT->current->motion.x,LOCPLINT->current->motion.y))
	{
		if ((items.size()-from)  >  5)
			LOCPLINT->adventureInt->statusbar.print(CGI->preth->advHListDown.first);
		else
			LOCPLINT->adventureInt->statusbar.clear();
		return;
	}
	//if not buttons then heroes
	int hx = LOCPLINT->current->motion.x, hy = LOCPLINT->current->motion.y;
	hx-=pos.x;
	hy-=pos.y; hy-=arrup->ourImages[0].bitmap->h;
	float ny = (float)hy/(float)32;
	if ((ny>5 || ny<0) || (from+ny>=items.size()))
	{
		LOCPLINT->adventureInt->statusbar.clear();
		return;
	}
	std::vector<std::string> temp;
	temp+=(items[from+ny].first->name),(items[from+ny].first->type->heroClass->name);
	LOCPLINT->adventureInt->statusbar.print( processStr(CGI->generaltexth->allTexts[15],temp) );
	//select(ny+from);
}
void CHeroList::clickRight(tribool down)
{
}
void CHeroList::hover (bool on)
{
}
void CHeroList::keyPressed (SDL_KeyboardEvent & key)
{
}
void CHeroList::draw()
{	
	for (int iT=0+from;iT<5+from;iT++)
	{
		int i = iT-from;
		if (iT>=items.size())
		{
			blitAtWR(mobile->ourImages[0].bitmap,posmobx,posmoby+i*32);
			blitAtWR(mana->ourImages[0].bitmap,posmanx,posmany+i*32);
			blitAtWR(empty,posporx,pospory+i*32);
			continue;
		}
		int pom = (LOCPLINT->cb->getHeroInfo(LOCPLINT->playerID,iT,0)->movement)/100;
		if (pom>25) pom=25;
		if (pom<0) pom=0;
		blitAtWR(mobile->ourImages[pom].bitmap,posmobx,posmoby+i*32); //move point
		pom = (LOCPLINT->cb->getHeroInfo(LOCPLINT->playerID,iT,0)->mana)/5; //bylo: .../10;
		if (pom>25) pom=25;
		if (pom<0) pom=0;
		blitAtWR(mana->ourImages[pom].bitmap,posmanx,posmany+i*32); //mana
		SDL_Surface * temp = LOCPLINT->cb->getHeroInfo(LOCPLINT->playerID,iT,0)->type->portraitSmall;
		blitAtWR(temp,posporx,pospory+i*32);
		if ((selected == iT) && (LOCPLINT->adventureInt->selection.type == HEROI_TYPE))
		{
			blitAtWR(selection,posporx,pospory+i*32);
		}
		//TODO: support for custom portraits
	}
	if (from>0)
		blitAtWR(arrup->ourImages[0].bitmap,arrupp.x,arrupp.y);
	else
		blitAtWR(arrup->ourImages[2].bitmap,arrupp.x,arrupp.y);

	if (items.size()-from>5)
		blitAtWR(arrdo->ourImages[0].bitmap,arrdop.x,arrdop.y);
	else
		blitAtWR(arrdo->ourImages[2].bitmap,arrdop.x,arrdop.y);
}
CTownList::CTownList()
{
	pos = genRect(192,48,747,196);
	arrup = CGI->spriteh->giveDef("IAM014.DEF");
	arrdo = CGI->spriteh->giveDef("IAM015.DEF");

	arrupp.x=747;
	arrupp.y=196;
	arrupp.w=arrup->ourImages[0].bitmap->w;
	arrupp.h=arrup->ourImages[0].bitmap->h;
	arrdop.x=747;
	arrdop.y=372;
	arrdop.w=arrdo->ourImages[0].bitmap->w;
	arrdop.h=arrdo->ourImages[0].bitmap->h;
	posporx = 747;
	pospory = 212;

	pressed = indeterminate;

	from = 0;
	
}
void CTownList::genList()
{
	int howMany = LOCPLINT->cb->howManyTowns();
	for (int i=0;i<howMany;i++)
	{
		items.push_back(LOCPLINT->cb->getTownInfo(i,0));
	}
}
void CTownList::select(int which)
{
	selected = which;
	if (which>=items.size()) 
		return;
	LOCPLINT->adventureInt->centerOn(items[which]->pos);
	LOCPLINT->adventureInt->selection.type = TOWNI_TYPE;
	LOCPLINT->adventureInt->selection.selected = items[which];
	LOCPLINT->adventureInt->terrain.currentPath = NULL;
	draw();
	LOCPLINT->adventureInt->heroList.draw();
}
void CTownList::mouseMoved (SDL_MouseMotionEvent & sEvent)
{
}
void CTownList::clickLeft(tribool down)
{
	if (down)
	{
		/***************************ARROWS*****************************************/
		if(isItIn(&arrupp,LOCPLINT->current->motion.x,LOCPLINT->current->motion.y) && from>0)
		{
			blitAtWR(arrup->ourImages[1].bitmap,arrupp.x,arrupp.y);
			pressed = true;
			return;
		}
		else if(isItIn(&arrdop,LOCPLINT->current->motion.x,LOCPLINT->current->motion.y) && (items.size()-from>5))
		{
			blitAtWR(arrdo->ourImages[1].bitmap,arrdop.x,arrdop.y);
			pressed = false;
			return;
		}
		/***************************HEROES*****************************************/
		int hx = LOCPLINT->current->motion.x, hy = LOCPLINT->current->motion.y;
		hx-=pos.x;
		hy-=pos.y; hy-=arrup->ourImages[0].bitmap->h;
		float ny = (float)hy/(float)32;
		if (ny>5 || ny<0)
			return;
		select(ny+from);
	}
	else
	{
		if (indeterminate(pressed))
			return;
		if (pressed) //up
		{
			blitAtWR(arrup->ourImages[0].bitmap,arrupp.x,arrupp.y);
			pressed = indeterminate;
			if (!down)
			{
				from--;
				if (from<0)
					from=0;
				draw();
			}
		}
		else if (!pressed) //down
		{
			blitAtWR(arrdo->ourImages[0].bitmap,arrdop.x,arrdop.y);
			pressed = indeterminate;
			if (!down)
			{
				from++;
				if (from<items.size()-5)
					from=items.size()-5;
				draw();
			}
		}
		else
			throw 0;

	}
}
void CTownList::clickRight(tribool down)
{
}
void CTownList::hover (bool on)
{
}
void CTownList::keyPressed (SDL_KeyboardEvent & key)
{
}
void CTownList::draw()
{	
	for (int iT=0+from;iT<5+from;iT++)
	{
		int i = iT-from;
		if (iT>=items.size())
		{
			blitAtWR(CGI->townh->getPic(-1),posporx,pospory+i*32);
			continue;
		}

		blitAtWR(CGI->townh->getPic(items[i]->type),posporx,pospory+i*32);

		if ((selected == iT) && (LOCPLINT->adventureInt->selection.type == TOWNI_TYPE))
		{
			blitAtWR(CGI->townh->getPic(-2),posporx,pospory+i*32);
		}
		//TODO: dodac oznaczanie zbudowania w danej turze i posiadania fortu
	}
	if (from>0)
		blitAtWR(arrup->ourImages[0].bitmap,arrupp.x,arrupp.y);
	else
		blitAtWR(arrup->ourImages[2].bitmap,arrupp.x,arrupp.y);

	if (items.size()-from>5)
		blitAtWR(arrdo->ourImages[0].bitmap,arrdop.x,arrdop.y);
	else
		blitAtWR(arrdo->ourImages[2].bitmap,arrdop.x,arrdop.y);
}
CStatusBar::CStatusBar(int x, int y)
{
	bg=CGI->bitmaph->loadBitmap("ADROLLVR.bmp");
	SDL_SetColorKey(bg,SDL_SRCCOLORKEY,SDL_MapRGB(bg->format,0,255,255));
	pos.x=x;
	pos.y=y;
	pos.w=bg->w;
	pos.h=bg->h;
	middlex=(bg->w/2)+x;
	middley=(bg->h/2)+y;
}
CStatusBar::~CStatusBar()
{
	SDL_FreeSurface(bg);
}
void CStatusBar::clear()
{
	current="";
	blitAtWR(bg,pos.x,pos.y);
}
void CStatusBar::print(std::string text)
{
	current=text;
	blitAtWR(bg,pos.x,pos.y);
	printAtMiddle(current,middlex,middley,GEOR13,zwykly);
}
void CStatusBar::show()
{
	blitAtWR(bg,pos.x,pos.y);
	printAtMiddle(current,middlex,middley,GEOR13,zwykly);
}
CMinimap::CMinimap(bool draw)
{
	statusbarTxt = CGI->preth->advWorldMap.first;
	pos.x=630;
	pos.y=26;
	pos.h=pos.w=144;
	radar = CGI->spriteh->giveDef("RADAR.DEF");
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
	blitAtWR(map[LOCPLINT->adventureInt->position.z],pos.x,pos.y);
}
void CMinimap::redraw(int level)// (level==-1) => redraw all levels
{
	(CGameInfo::mainObj);
	for (int i=0; i<CGI->mh->sizes.z; i++)
	{
		SDL_Surface * pom ;
		if ((level>=0) && (i!=level))
			continue;
		if (map.size()<i+1)
			pom = CSDL_Ext::newSurface(pos.w,pos.h,ekran);
		else pom = map[i];
		for (int x=0;x<pos.w;x++)
		{
			for (int y=0;y<pos.h;y++)
			{
				int mx=(CGI->mh->sizes.x*x)/pos.w;
				int my=(CGI->mh->sizes.y*y)/pos.h;
				if (CGI->mh->ttiles[mx][my][i].blocked && (!CGI->mh->ttiles[mx][my][i].visitable))
					SDL_PutPixel(pom,x,y,colorsBlocked[CGI->mh->ttiles[mx][my][i].terType].r,colorsBlocked[CGI->mh->ttiles[mx][my][i].terType].g,colorsBlocked[CGI->mh->ttiles[mx][my][i].terType].b);
				else SDL_PutPixel(pom,x,y,colors[CGI->mh->ttiles[mx][my][i].terType].r,colors[CGI->mh->ttiles[mx][my][i].terType].g,colors[CGI->mh->ttiles[mx][my][i].terType].b);
			}
		}
		map.push_back(pom);
	}
}
void CMinimap::updateRadar()
{}
void CMinimap::clickRight (tribool down)
{}
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
void CMinimap::mouseMoved (SDL_MouseMotionEvent & sEvent)
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
CTerrainRect::CTerrainRect():currentPath(NULL)
{
	tilesw=19;
	tilesh=18;
	pos.x=7;
	pos.y=6;
	pos.w=593;
	pos.h=547;
	arrows = CGI->spriteh->giveDef("ADAG.DEF");
	for(int y=0; y<arrows->ourImages.size(); ++y)
	{
		CSDL_Ext::fullAlphaTransform(arrows->ourImages[y].bitmap);
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
	if (LOCPLINT->adventureInt->selection.type != HEROI_TYPE)
	{
		if (currentPath)
		{
			delete currentPath;
			currentPath = NULL;
		}
		return;
	}
	int3 mp = whichTileIsIt();
	if ((mp.x<0) || (mp.y<0))
		return;
	if (currentPath)
	{
		if ( (currentPath->endPos()) == mp)
		{ //move
			CPath sended(*currentPath); //temporary path - engine will operate on it
			LOCPLINT->cb->moveHero( ((const CHeroInstance*)LOCPLINT->adventureInt->selection.selected)->type->ID,&sended,1,0);
		}
		else
		{
			delete currentPath;
			currentPath=NULL;
		}
	}
	const CHeroInstance * currentHero = LOCPLINT->adventureInt->heroList.items[LOCPLINT->adventureInt->heroList.selected].first;
	int3 bufpos = currentHero->getPosition(false);
	//bufpos.x-=1;
	currentPath = LOCPLINT->adventureInt->heroList.items[LOCPLINT->adventureInt->heroList.selected].second = CGI->pathf->getPath(bufpos,mp,currentHero,1);
}
void CTerrainRect::clickRight(tribool down)
{
}
void CTerrainRect::mouseMoved (SDL_MouseMotionEvent & sEvent)
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

}
void CTerrainRect::keyPressed (SDL_KeyboardEvent & key){}
void CTerrainRect::hover(bool on)
{
	if (!on)
		LOCPLINT->adventureInt->statusbar.clear();
}
void CTerrainRect::show()
{
	SDL_Surface * teren = CGI->mh->terrainRect
		(LOCPLINT->adventureInt->position.x,LOCPLINT->adventureInt->position.y,
		tilesw,tilesh,LOCPLINT->adventureInt->position.z,LOCPLINT->adventureInt->anim);
	SDL_BlitSurface(teren,&genRect(pos.h,pos.w,0,0),ekran,&genRect(547,594,7,6));
	SDL_FreeSurface(teren);
	if (currentPath) //drawing path
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
				std::vector<CPathNode> & cv = currentPath->nodes;
				if (cv[i+1].coord.x == cv[i].coord.x-1 && cv[i+1].coord.y == cv[i].coord.y-1)
				{
					if(cv[i-1].coord.x == cv[i].coord.x+1 && cv[i-1].coord.y == cv[i].coord.y)
					{
						pn = 3;
					}
					else if(cv[i-1].coord.x == cv[i].coord.x+1 && cv[i-1].coord.y == cv[i].coord.y+1)
					{
						pn = 12;
					}
					else if(cv[i-1].coord.x == cv[i].coord.x && cv[i-1].coord.y == cv[i].coord.y+1)
					{
						pn = 21;
					}
					else if(cv[i-1].coord.x == cv[i].coord.x-1 && cv[i-1].coord.y == cv[i].coord.y+1)
					{
						pn = 22;
					}
					else if(cv[i-1].coord.x == cv[i].coord.x+1 && cv[i-1].coord.y == cv[i].coord.y-1)
					{
						pn = 2;
					}
				}
				else if (cv[i+1].coord.x == cv[i].coord.x && cv[i+1].coord.y == cv[i].coord.y-1)
				{
					if(cv[i-1].coord.x == cv[i].coord.x+1 && cv[i-1].coord.y == cv[i].coord.y+1)
					{
						pn = 4;
					}
					else if(cv[i-1].coord.x == cv[i].coord.x && cv[i-1].coord.y == cv[i].coord.y+1)
					{
						pn = 13;
					}
					else if(cv[i-1].coord.x == cv[i].coord.x-1 && cv[i-1].coord.y == cv[i].coord.y+1)
					{
						pn = 22;
					}
				}
				else if (cv[i+1].coord.x == cv[i].coord.x+1 && cv[i+1].coord.y == cv[i].coord.y-1)
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
				else if (cv[i+1].coord.x == cv[i].coord.x+1 && cv[i+1].coord.y == cv[i].coord.y)
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
				else if (cv[i+1].coord.x == cv[i].coord.x+1 && cv[i+1].coord.y == cv[i].coord.y+1)
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
				else if (cv[i+1].coord.x == cv[i].coord.x && cv[i+1].coord.y == cv[i].coord.y+1)
				{
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
				}
				else if (cv[i+1].coord.x == cv[i].coord.x-1 && cv[i+1].coord.y == cv[i].coord.y+1)
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
				else if (cv[i+1].coord.x == cv[i].coord.x-1 && cv[i+1].coord.y == cv[i].coord.y)
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
			if (pn>=0)
			{				
				int x = 32*(currentPath->nodes[i].coord.x-LOCPLINT->adventureInt->position.x)+7,
					y = 32*(currentPath->nodes[i].coord.y-LOCPLINT->adventureInt->position.y)+6;
				if (x<0 || y<0 || x>pos.w || y>pos.h)
					continue;
				int hvx = (x+arrows->ourImages[pn].bitmap->w)-(pos.x+pos.w),
					hvy = (y+arrows->ourImages[pn].bitmap->h)-(pos.y+pos.h);
				if (hvx<0 && hvy<0)
					blitAtWR(arrows->ourImages[pn].bitmap,x,y);
				else if(hvx<0)
					SDL_BlitSurface
						(arrows->ourImages[pn].bitmap,&genRect(arrows->ourImages[pn].bitmap->h-hvy,arrows->ourImages[pn].bitmap->w,0,0),
						ekran,&genRect(arrows->ourImages[pn].bitmap->h-hvy,arrows->ourImages[pn].bitmap->w,x,y));
				else if (hvy<0)
				{
					SDL_BlitSurface
						(arrows->ourImages[pn].bitmap,&genRect(arrows->ourImages[pn].bitmap->h,arrows->ourImages[pn].bitmap->w-hvx,0,0),
						ekran,&genRect(arrows->ourImages[pn].bitmap->h,arrows->ourImages[pn].bitmap->w-hvx,x,y));
				}
				else
					SDL_BlitSurface
						(arrows->ourImages[pn].bitmap,&genRect(arrows->ourImages[pn].bitmap->h-hvy,arrows->ourImages[pn].bitmap->w-hvx,0,0),
						ekran,&genRect(arrows->ourImages[pn].bitmap->h-hvy,arrows->ourImages[pn].bitmap->w-hvx,x,y));

			}
		} //for (int i=0;i<currentPath->nodes.size()-1;i++)
	} // if (currentPath)
}

int3 CTerrainRect::whichTileIsIt(int x, int y)
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
	bg = CGI->bitmaph->loadBitmap("ZRESBAR.bmp");
	SDL_SetColorKey(bg,SDL_SRCCOLORKEY,SDL_MapRGB(bg->format,0,255,255));
	blueToPlayersAdv(bg,LOCPLINT->playerID);
	//blueToPlayersNice(bg,LOCPLINT->playerID);
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
		itoa(LOCPLINT->cb->getResourceAmount(i),buf,10);
		printAt(buf,txtpos[i].first,txtpos[i].second,GEOR13,zwykly);
	}
	std::vector<std::string> temp;
	itoa(LOCPLINT->cb->getDate(3),buf,10); temp+=std::string(buf);
	itoa(LOCPLINT->cb->getDate(2),buf,10); temp+=std::string(buf);
	itoa(LOCPLINT->cb->getDate(1),buf,10); temp+=std::string(buf);
	printAt(processStr(datetext,temp),txtpos[7].first,txtpos[7].second,GEOR13,zwykly);
	temp.clear();
	updateRect(&pos,ekran);
	delete buf;
}
CInfoBar::CInfoBar()
{
	pos.x=604;
	pos.y=389;
	pos.w=194;
	pos.h=186;
}
void CInfoBar::draw(void * specific)
{
	SDL_Surface * todr = LOCPLINT->infoWin(specific);
	blitAt(todr,pos.x,pos.y);
	SDL_FreeSurface(todr);
}
CAdvMapInt::CAdvMapInt(int Player)
:player(Player),
statusbar(7,556),
kingOverview(CGI->preth->advKingdomOverview.first,CGI->preth->advKingdomOverview.second,
			 &CAdvMapInt::fshowOverview, 679, 196, "IAM002.DEF"),

underground(CGI->preth->advSurfaceSwitch.first,CGI->preth->advSurfaceSwitch.second,
		   &CAdvMapInt::fswitchLevel, 711, 196, "IAM010.DEF", false, new std::vector<std::string>(1,std::string("IAM003.DEF"))),

questlog(CGI->preth->advQuestlog.first,CGI->preth->advQuestlog.second,
		 &CAdvMapInt::fshowQuestlog, 679, 228, "IAM004.DEF"),

sleepWake(CGI->preth->advSleepWake.first,CGI->preth->advSleepWake.second,
		  &CAdvMapInt::fsleepWake, 711, 228, "IAM005.DEF"),

moveHero(CGI->preth->advMoveHero.first,CGI->preth->advMoveHero.second,
		  &CAdvMapInt::fmoveHero, 679, 260, "IAM006.DEF"),

spellbook(CGI->preth->advCastSpell.first,CGI->preth->advCastSpell.second,
		  &CAdvMapInt::fshowSpellbok, 711, 260, "IAM007.DEF"),

advOptions(CGI->preth->advAdvOptions.first,CGI->preth->advAdvOptions.second,
		  &CAdvMapInt::fadventureOPtions, 679, 292, "IAM008.DEF"),

sysOptions(CGI->preth->advSystemOptions.first,CGI->preth->advSystemOptions.second,
		  &CAdvMapInt::fsystemOptions, 711, 292, "IAM009.DEF"),

nextHero(CGI->preth->advNextHero.first,CGI->preth->advNextHero.second,
		  &CAdvMapInt::fnextHero, 679, 324, "IAM000.DEF"),

endTurn(CGI->preth->advEndTurn.first,CGI->preth->advEndTurn.second,
		  &CAdvMapInt::fendTurn, 679, 356, "IAM001.DEF")
{
	LOCPLINT->adventureInt=this;
	bg = CGI->bitmaph->loadBitmap("ADVMAP.bmp");
	blueToPlayersAdv(bg,player);
	scrollingLeft = false;
	scrollingRight  = false;
	scrollingUp = false ;
	scrollingDown = false ;
	updateScreen  = false;
	anim=0;
	animValHitCount=0; //animation frame

	heroList.init();
	heroList.genList();
	//townList.init();
	townList.genList();
	
	gems.push_back(CGI->spriteh->giveDef("agemLL.def"));
	gems.push_back(CGI->spriteh->giveDef("agemLR.def"));
	gems.push_back(CGI->spriteh->giveDef("agemUL.def"));
	gems.push_back(CGI->spriteh->giveDef("agemUR.def"));
}

void CAdvMapInt::fshowOverview()
{
}
void CAdvMapInt::fswitchLevel()
{
	if(!CGI->ac->map.twoLevel)
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
}
void CAdvMapInt::fshowSpellbok()
{
}
void CAdvMapInt::fadventureOPtions()
{
}
void CAdvMapInt::fsystemOptions()
{
}
void CAdvMapInt::fnextHero()
{
}
void CAdvMapInt::fendTurn()
{
}

void CAdvMapInt::show()
{
	blitAt(bg,0,0);

	kingOverview.show();
	kingOverview.activate();
	underground.show();
	underground.activate();
	questlog.show();
	questlog.activate();
	sleepWake.show();
	sleepWake.activate();
	moveHero.show();
	moveHero.activate();
	spellbook.show();
	spellbook.activate();
	advOptions.show();
	advOptions.activate();
	sysOptions.show();
	sysOptions.activate();
	nextHero.show();
	nextHero.activate();
	endTurn.show();
	endTurn.activate();

	minimap.activate();
	minimap.draw();
	heroList.activate();
	heroList.draw();
	townList.activate();
	townList.draw();
	terrain.activate();

	resdatabar.draw();

	statusbar.show();

	SDL_Flip(ekran);
}
void CAdvMapInt::update()
{
	terrain.show();
	blitAt(gems[2]->ourImages[LOCPLINT->playerID].bitmap,6,6);
	blitAt(gems[0]->ourImages[LOCPLINT->playerID].bitmap,6,508);
	blitAt(gems[1]->ourImages[LOCPLINT->playerID].bitmap,556,508);
	blitAt(gems[3]->ourImages[LOCPLINT->playerID].bitmap,556,6);
	updateRect(&genRect(550,600,6,6));
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
}
CAdvMapInt::CurrentSelection::CurrentSelection()
{
	type=-1;
	selected=NULL;
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