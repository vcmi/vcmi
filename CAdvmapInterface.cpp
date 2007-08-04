#include "stdafx.h"
#include "CAdvmapInterface.h"

CAdvMapInt::CAdvMapInt(int Player)
:player(Player)
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
CAdvMapInt::~CAdvMapInt()
{
	SDL_FreeSurface(bg);
}

void AdventureMapButton::clickLeft (tribool down)
{
	if (down)
		state=1;
	else state=0;
	show();
	int i;
}
void AdventureMapButton::clickRight (tribool down)
{
	//TODO: show/hide infobox
}
void AdventureMapButton::hover (bool on)
{
	//TODO: print info in statusbar
}
void AdventureMapButton::activate()
{
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
	ClickableL::deactivate();
	Hoverable::deactivate();
	KeyInterested::deactivate();
}
AdventureMapButton::AdventureMapButton ()
{
	type=2;
	abs=true;
	active=false;
	ourObj=NULL;
	state=0;
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
		(CURPLINT->adventureInt->position.x,CURPLINT->adventureInt->position.y,
		19,18,CURPLINT->adventureInt->position.z,CURPLINT->adventureInt->anim);
	SDL_BlitSurface(teren,&genRect(547,594,0,0),ekran,&genRect(547,594,7,6));
	SDL_FreeSurface(teren);
}
void CAdvMapInt::show()
{
	blitAt(bg,0,0);
	SDL_Flip(ekran);
}
void CAdvMapInt::update()
{
	terrain.show();
	blitAt(gems[2]->ourImages[CURPLINT->playerID].bitmap,6,6);
	blitAt(gems[0]->ourImages[CURPLINT->playerID].bitmap,6,508);
	blitAt(gems[1]->ourImages[CURPLINT->playerID].bitmap,556,508);
	blitAt(gems[3]->ourImages[CURPLINT->playerID].bitmap,556,6);
	updateRect(&genRect(550,600,6,6));
}