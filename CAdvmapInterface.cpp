#include "stdafx.h"
#include "CAdvmapInterface.h"
#include "hch\CLodHandler.h"
#include "hch\CPreGameTextHandler.h"

extern TTF_Font * TNRB16, *TNR, *GEOR13, *GEORXX; //fonts

using namespace boost::logic;
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
}; 
void CList::deactivate()
{
	ClickableL::deactivate();
	ClickableR::deactivate();
	Hoverable::deactivate();
	KeyInterested::deactivate();
}; 
void CList::clickLeft(tribool down)
{
};
CHeroList::CHeroList()
{
	pos = genRect(192,64,609,196);

	arrup = CGI->spriteh->giveDef("IAM012.DEF");
	arrdo = CGI->spriteh->giveDef("IAM013.DEF");
	mobile = CGI->spriteh->giveDef("IMOBIL.DEF");
	mana = CGI->spriteh->giveDef("IMANA.DEF");
}
void CHeroList::select(int which)
{
}
void CHeroList::clickLeft(tribool down)
{
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
CTownList::CTownList()
{
	pos = genRect(192,48,747,196);
	arrup = CGI->spriteh->giveDef("IAM014.DEF");
	arrdo = CGI->spriteh->giveDef("IAM015.DEF");
}
void CTownList::select(int which)
{
}
void CTownList::clickLeft(tribool down)
{
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
	std::ifstream is("minimap.txt",std::ifstream::in);
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
			pom = SDL_CreateRGBSurface(ekran->flags,pos.w,pos.h,ekran->format->BitsPerPixel,ekran->format->Rmask,ekran->format->Gmask,ekran->format->Bmask,ekran->format->Amask);
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
		MotionInterested::deactivate();
	ClickableL::clickLeft(down);
	if (!((bool)down))
		return;
	
	float dx=((float)(LOCPLINT->current->motion.x-pos.x))/((float)pos.w),
		dy=((float)(LOCPLINT->current->motion.y-pos.y))/((float)pos.h);

	int dxa = (CGI->mh->sizes.x*dx)-(LOCPLINT->adventureInt->terrain.tilesw/2);
	int dya = (CGI->mh->sizes.y*dy)-(LOCPLINT->adventureInt->terrain.tilesh/2);

	if (dxa<0)
		dxa=-(Woff/2);
	else if((dxa+LOCPLINT->adventureInt->terrain.tilesw)  >  (CGI->mh->sizes.x))
		dxa=CGI->mh->sizes.x-LOCPLINT->adventureInt->terrain.tilesw+(Woff/2);

	if (dya<0)
		dya = -(Hoff/2);
	else if((dya+LOCPLINT->adventureInt->terrain.tilesh)  >  (CGI->mh->sizes.y))
		dya = CGI->mh->sizes.y-LOCPLINT->adventureInt->terrain.tilesh+(Hoff/2);

	LOCPLINT->adventureInt->position.x=dxa;
	LOCPLINT->adventureInt->position.y=dya;
	LOCPLINT->adventureInt->updateScreen=true;
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
	pos.w=594;
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
}; 
void CTerrainRect::deactivate()
{
	ClickableL::deactivate();
	ClickableR::deactivate();
	Hoverable::deactivate();
	KeyInterested::deactivate();
}; 
void CTerrainRect::clickLeft(tribool down){}
void CTerrainRect::clickRight(tribool down){}
void CTerrainRect::hover(bool on){}
void CTerrainRect::keyPressed (SDL_KeyboardEvent & key){}
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
				int x = 32*(currentPath->nodes[i].x-LOCPLINT->adventureInt->position.x)+7,
					y = 32*(currentPath->nodes[i].y-LOCPLINT->adventureInt->position.y)+6;
				if (x<0 || y<0 || x>pos.w || y>pos.h)
					continue;
				pn=0;
			}
			else
			{
				std::vector<CPathNode> & cv = currentPath->nodes;
				if (cv[i+1].x == cv[i].x-1 && cv[i+1].y == cv[i].y-1)
				{
					if(cv[i-1].x == cv[i].x+1 && cv[i-1].y == cv[i].y)
					{
						pn = 3;
					}
					else if(cv[i-1].x == cv[i].x+1 && cv[i-1].y == cv[i].y+1)
					{
						pn = 12;
					}
					else if(cv[i-1].x == cv[i].x && cv[i-1].y == cv[i].y+1)
					{
						pn = 21;
					}
					else if(cv[i-1].x == cv[i].x-1 && cv[i-1].y == cv[i].y+1)
					{
						pn = 22;
					}
					else if(cv[i-1].x == cv[i].x+1 && cv[i-1].y == cv[i].y-1)
					{
						pn = 2;
					}
				}
				else if (cv[i+1].x == cv[i].x && cv[i+1].y == cv[i].y-1)
				{
					if(cv[i-1].x == cv[i].x+1 && cv[i-1].y == cv[i].y+1)
					{
						pn = 4;
					}
					else if(cv[i-1].x == cv[i].x && cv[i-1].y == cv[i].y+1)
					{
						pn = 13;
					}
					else if(cv[i-1].x == cv[i].x-1 && cv[i-1].y == cv[i].y+1)
					{
						pn = 22;
					}
				}
				else if (cv[i+1].x == cv[i].x+1 && cv[i+1].y == cv[i].y-1)
				{
					if(cv[i-1].x == cv[i].x && cv[i-1].y == cv[i].y+1)
					{
						pn = 5;
					}
					else if(cv[i-1].x == cv[i].x-1 && cv[i-1].y == cv[i].y+1)
					{
						pn = 14;
					}
					else if(cv[i-1].x-1 == cv[i].x && cv[i-1].y == cv[i].y)
					{
						pn = 23;
					}
					else if(cv[i-1].x == cv[i].x-1 && cv[i-1].y == cv[i].y-1)
					{
						pn = 24;
					}
					else if(cv[i-1].x == cv[i].x+1 && cv[i-1].y == cv[i].y+1)
					{
						pn = 4;
					}
				}
				else if (cv[i+1].x == cv[i].x+1 && cv[i+1].y == cv[i].y)
				{
					if(cv[i-1].x == cv[i].x-1 && cv[i-1].y == cv[i].y+1)
					{
						pn = 6;
					}
					else if(cv[i-1].x == cv[i].x-1 && cv[i-1].y == cv[i].y)
					{
						pn = 15;
					}
					else if(cv[i-1].x == cv[i].x-1 && cv[i-1].y == cv[i].y-1)
					{
						pn = 24;
					}
				}
				else if (cv[i+1].x == cv[i].x+1 && cv[i+1].y == cv[i].y+1)
				{
					if(cv[i-1].x == cv[i].x-1 && cv[i-1].y == cv[i].y)
					{
						pn = 7;
					}
					else if(cv[i-1].x == cv[i].x-1 && cv[i-1].y == cv[i].y-1)
					{
						pn = 16;
					}
					else if(cv[i-1].x == cv[i].x && cv[i-1].y == cv[i].y-1)
					{
						pn = 17;
					}
					else if(cv[i-1].x == cv[i].x-1 && cv[i-1].y == cv[i].y+1)
					{
						pn = 6;
					}
					else if(cv[i-1].x == cv[i].x+1 && cv[i-1].y == cv[i].y-1)
					{
						pn = 18;
					}
				}
				else if (cv[i+1].x == cv[i].x && cv[i+1].y == cv[i].y+1)
				{
					if(cv[i-1].x == cv[i].x-1 && cv[i-1].y == cv[i].y-1)
					{
						pn = 8;
					}
					else if(cv[i-1].x == cv[i].x && cv[i-1].y == cv[i].y-1)
					{
						pn = 9;
					}
					else if(cv[i-1].x == cv[i].x+1 && cv[i-1].y == cv[i].y-1)
					{
						pn = 18;
					}
				}
				else if (cv[i+1].x == cv[i].x-1 && cv[i+1].y == cv[i].y+1)
				{
					if(cv[i-1].x == cv[i].x && cv[i-1].y == cv[i].y-1)
					{
						pn = 1;
					}
					else if(cv[i-1].x == cv[i].x+1 && cv[i-1].y == cv[i].y-1)
					{
						pn = 10;
					}
					else if(cv[i-1].x == cv[i].x+1 && cv[i-1].y == cv[i].y)
					{
						pn = 19;
					}
					else if(cv[i-1].x == cv[i].x-1 && cv[i-1].y == cv[i].y-1)
					{
						pn = 8;
					}
					else if(cv[i-1].x == cv[i].x+1 && cv[i-1].y == cv[i].y+1)
					{
						pn = 20;
					}
				}
				else if (cv[i+1].x == cv[i].x-1 && cv[i+1].y == cv[i].y)
				{
					if(cv[i-1].x == cv[i].x+1 && cv[i-1].y == cv[i].y-1)
					{
						pn = 2;
					}
					else if(cv[i-1].x == cv[i].x+1 && cv[i-1].y == cv[i].y)
					{
						pn = 11;
					}
					else if(cv[i-1].x == cv[i].x+1 && cv[i-1].y == cv[i].y+1)
					{
						pn = 20;
					}
				}
			}
			if (pn>=0)
			{				
				int x = 32*(currentPath->nodes[i].x-LOCPLINT->adventureInt->position.x)+7,
					y = 32*(currentPath->nodes[i].y-LOCPLINT->adventureInt->position.y)+6;
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
	bg = CGI->bitmaph->loadBitmap("ADVMAP.bmp");
	blueToPlayersAdv(bg,player);
	scrollingLeft = false;
	scrollingRight  = false;
	scrollingUp = false ;
	scrollingDown = false ;
	updateScreen  = false;
	anim=0;
	animValHitCount=0; //animation frame
	
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