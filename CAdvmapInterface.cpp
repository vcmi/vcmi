#include "stdafx.h"
#include "CAdvmapInterface.h"

CAdvMapInt::CAdvMapInt(int Player)
:player(Player)
{
	bg = CGI->bitmaph->loadBitmap("ADVMAP.bmp");
	blueToPlayersAdv(bg,player);
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