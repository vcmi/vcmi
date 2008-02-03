#pragma once
#include "SDL_Extensions.h"
#include "hch\CDefHandler.h"
#include "CGameInfo.h"
#include "hch\CLodHandler.h"
#include "hch\CPreGameTextHandler.h"
#include "hch/CTownHandler.h"
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
	CDefHandler * temp = CGI->spriteh->giveDef(defName); 
	temp->notFreeImgs = true;
	for (int i=0;i<temp->ourImages.size();i++)
	{
		imgs.resize(1);
		imgs[0].push_back(temp->ourImages[i].bitmap);
		CSDL_Ext::blueToPlayersAdv(imgs[curimg][i],LOCPLINT->playerID);
	}
	delete temp;
	if (add)
	{
		imgs.resize(imgs.size()+add->size());
		for (int i=0; i<add->size();i++)
		{
			temp = CGI->spriteh->giveDef((*add)[i]);
			temp->notFreeImgs = true;
			for (int j=0;j<temp->ourImages.size();j++)
			{
				imgs[i+1].push_back(temp->ourImages[j].bitmap);
				CSDL_Ext::blueToPlayersAdv(imgs[1+i][j],LOCPLINT->playerID);
			}
			delete temp;
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
	if(helpBox.size()) //there is no point to show window with nothing inside...
		LOCPLINT->adventureInt->handleRightClick(helpBox,down,this);
}
template <typename T>
void AdventureMapButton<T>::hover (bool on)
{
	Hoverable::hover(on);
	if (on)
		LOCPLINT->statusbar->print(name);
	else if (LOCPLINT->statusbar->getCurrent()==name)
		LOCPLINT->statusbar->clear();
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
template <typename T>
CTownList<T>::~CTownList()
{
	delete arrup;
	delete arrdo;
}
template <typename T>
CTownList<T>::CTownList(int Size, SDL_Rect * Pos, int arupx, int arupy, int ardox, int ardoy)
:CList(Size)
{
	pos = *Pos;
	arrup = CGI->spriteh->giveDef("IAM014.DEF");
	arrdo = CGI->spriteh->giveDef("IAM015.DEF");

	arrupp.x=arupx;
	arrupp.y=arupy;
	arrupp.w=arrup->ourImages[0].bitmap->w;
	arrupp.h=arrup->ourImages[0].bitmap->h;
	arrdop.x=ardox;
	arrdop.y=ardoy;
	arrdop.w=arrdo->ourImages[0].bitmap->w;
	arrdop.h=arrdo->ourImages[0].bitmap->h;
	posporx = arrdop.x;
	pospory = arrupp.y + arrupp.h;

	pressed = indeterminate;

	from = 0;
	
}
template<typename T>
void CTownList<T>::genList()
{
	int howMany = LOCPLINT->cb->howManyTowns();
	for (int i=0;i<howMany;i++)
	{
		items.push_back(LOCPLINT->cb->getTownInfo(i,0));
	}
}
template<typename T>
void CTownList<T>::select(int which)
{
	if (which>=items.size()) 
		return;
	selected = which;
	if(owner)
		(owner->*fun)();
}
template<typename T>
void CTownList<T>::mouseMoved (SDL_MouseMotionEvent & sEvent)
{
	if(isItIn(&arrupp,LOCPLINT->current->motion.x,LOCPLINT->current->motion.y))
	{
		if (from>0)
			LOCPLINT->adventureInt->statusbar.print(CGI->preth->advTListUp.first);
		else
			LOCPLINT->adventureInt->statusbar.clear();
		return;
	}
	else if(isItIn(&arrdop,LOCPLINT->current->motion.x,LOCPLINT->current->motion.y))
	{
		if ((items.size()-from)  >  SIZE)
			LOCPLINT->adventureInt->statusbar.print(CGI->preth->advTListDown.first);
		else
			LOCPLINT->adventureInt->statusbar.clear();
		return;
	}
	//if not buttons then heroes
	int hx = LOCPLINT->current->motion.x, hy = LOCPLINT->current->motion.y;
	hx-=pos.x;
	hy-=pos.y; hy-=arrup->ourImages[0].bitmap->h;
	float ny = (float)hy/(float)32;
	if ((ny>SIZE || ny<0) || (from+ny>=items.size()))
	{
		LOCPLINT->adventureInt->statusbar.clear();
		return;
	};
	//LOCPLINT->adventureInt->statusbar.print( items[from+ny]->name + ", " + items[from+ny]->town->name ); //TODO - uncomment when pointer to the town type is initialized
}
template<typename T>
void CTownList<T>::clickLeft(tribool down)
{
	if (down)
	{
		/***************************ARROWS*****************************************/
		if(isItIn(&arrupp,LOCPLINT->current->motion.x,LOCPLINT->current->motion.y) && from>0)
		{
			blitAt(arrup->ourImages[1].bitmap,arrupp.x,arrupp.y);
			pressed = true;
			return;
		}
		else if(isItIn(&arrdop,LOCPLINT->current->motion.x,LOCPLINT->current->motion.y) && (items.size()-from>SIZE))
		{
			blitAt(arrdo->ourImages[1].bitmap,arrdop.x,arrdop.y);
			pressed = false;
			return;
		}
		/***************************TOWNS*****************************************/
		int hx = LOCPLINT->current->motion.x, hy = LOCPLINT->current->motion.y;
		hx-=pos.x;
		hy-=pos.y; hy-=arrup->ourImages[0].bitmap->h;
		float ny = (float)hy/(float)32;
		if (ny>SIZE || ny<0)
			return;
		if (SIZE==5 && ((int)(ny+from))==selected && (LOCPLINT->adventureInt->selection.type == TOWNI_TYPE))
			LOCPLINT->openTownWindow(items[selected]);//print town screen
		else
			select(ny+from);
	}
	else
	{
		if (indeterminate(pressed))
			return;
		if (pressed) //up
		{
			blitAt(arrup->ourImages[0].bitmap,arrupp.x,arrupp.y);
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
			blitAt(arrdo->ourImages[0].bitmap,arrdop.x,arrdop.y);
			pressed = indeterminate;
			if (!down)
			{
				from++;
				//if (from<items.size()-5)
				//	from=items.size()-5;
				draw();
			}
		}
		else
			throw 0;

	}
}
template<typename T>
void CTownList<T>::clickRight(tribool down)
{	
	if (down)
	{
		/***************************ARROWS*****************************************/
		if(isItIn(&arrupp,LOCPLINT->current->motion.x,LOCPLINT->current->motion.y) && from>0)
		{
			LOCPLINT->adventureInt->handleRightClick(CGI->preth->advTListUp.second,down,this);
		}
		else if(isItIn(&arrdop,LOCPLINT->current->motion.x,LOCPLINT->current->motion.y) && (items.size()-from>5))
		{
			LOCPLINT->adventureInt->handleRightClick(CGI->preth->advTListDown.second,down,this);
		}
	}
	else
	{
			LOCPLINT->adventureInt->handleRightClick(CGI->preth->advTListUp.second,down,this);
			LOCPLINT->adventureInt->handleRightClick(CGI->preth->advTListDown.second,down,this);
	}
}
template<typename T>
void CTownList<T>::hover (bool on)
{
}
template<typename T>
void CTownList<T>::keyPressed (SDL_KeyboardEvent & key)
{
}
template<typename T>
void CTownList<T>::draw()
{	
	for (int iT=0+from;iT<SIZE+from;iT++)
	{
		int i = iT-from;
		if (iT>=items.size())
		{
			blitAt(CGI->townh->getPic(-1),posporx,pospory+i*32);
			continue;
		}

		blitAt(CGI->townh->getPic(items[iT]->subID,items[iT]->hasFort(),items[iT]->builded),posporx,pospory+i*32);

		if ((selected == iT) && (LOCPLINT->adventureInt->selection.type == TOWNI_TYPE))
		{
			blitAt(CGI->townh->getPic(-2),posporx,pospory+i*32);
		}
	}
	if (from>0)
		blitAt(arrup->ourImages[0].bitmap,arrupp.x,arrupp.y);
	else
		blitAt(arrup->ourImages[2].bitmap,arrupp.x,arrupp.y);

	if (items.size()-from>SIZE)
		blitAt(arrdo->ourImages[0].bitmap,arrdop.x,arrdop.y);
	else
		blitAt(arrdo->ourImages[2].bitmap,arrdop.x,arrdop.y);
}