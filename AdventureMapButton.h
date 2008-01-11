#pragma once
#include "SDL_Extensions.h"
#include "hch\CDefHandler.h"
#include "CGameInfo.h"
#include "hch\CLodHandler.h"
template <typename T>
AdventureMapButton<T>::AdventureMapButton ()
{
	type=2;
	abs=true;
	active=false;
	ourObj=NULL;
	state=0;
}
template <typename T>
AdventureMapButton<T>::AdventureMapButton
( std::string Name, std::string HelpBox, void(T::*Function)(), int x, int y, std::string defName, T* Owner, bool activ, std::vector<std::string> * add )
{
	owner = Owner;
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
		CSDL_Ext::blueToPlayersAdv(imgs[curimg][i],LOCPLINT->playerID);
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
				CSDL_Ext::blueToPlayersAdv(imgs[1+i][j],LOCPLINT->playerID);
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

template <typename T>
void AdventureMapButton<T>::clickLeft (tribool down)
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
	{
		pressedL=state;
		(owner->*function)();
	}
	else
	{
		pressedL=state;
	}
}
template <typename T>
void AdventureMapButton<T>::clickRight (tribool down)
{
	LOCPLINT->adventureInt->handleRightClick(helpBox,down,this);
}
template <typename T>
void AdventureMapButton<T>::hover (bool on)
{
	Hoverable::hover(on);
	if (on)
		LOCPLINT->adventureInt->statusbar.print(name);
	else if (LOCPLINT->adventureInt->statusbar.current==name)
		LOCPLINT->adventureInt->statusbar.clear();
}
template <typename T>
void AdventureMapButton<T>::activate()
{
	if (active) return;
	active=true;
	ClickableL::activate();
	ClickableR::activate();
	Hoverable::activate();
	KeyInterested::activate();
}
template <typename T>
void AdventureMapButton<T>::keyPressed (SDL_KeyboardEvent & key)
{
	//TODO: check if it's shortcut
}
template <typename T>
void AdventureMapButton<T>::deactivate()
{
	if (!active) return;
	active=false;
	ClickableL::deactivate();
	ClickableR::deactivate();
	Hoverable::deactivate();
	KeyInterested::deactivate();
}
