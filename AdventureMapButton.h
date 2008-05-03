#pragma once
#include "SDL_Extensions.h"
#include "hch\CDefHandler.h"
#include "CGameInfo.h"
#include "hch\CLodHandler.h"
#include "hch\CPreGameTextHandler.h"
#include "hch/CTownHandler.h"
#include "CLua.h"
#include "CPlayerInterface.h"
template <typename T=CAdvMapInt>
class AdventureMapButton 
	: public ClickableL, public ClickableR, public Hoverable, public KeyInterested, public CButtonBase
{
public:
	std::string name; //for status bar 
	std::string helpBox; //for right-click help
	char key; //key shortcut
	T* owner;
	void (T::*function)(); //function in CAdvMapInt called when this button is pressed, different for each button
	bool colorChange,
		actOnDown; //runs when mouse is pressed down over it, not when up

	void clickRight (tribool down);
	void clickLeft (tribool down);
	virtual void hover (bool on);
	void keyPressed (SDL_KeyboardEvent & key);
	void activate(); // makes button active
	void deactivate(); // makes button inactive (but doesn't delete)

	AdventureMapButton(); //c-tor
	AdventureMapButton( std::string Name, std::string HelpBox, void(T::*Function)(), int x, int y, std::string defName, T* Owner, bool activ=false,  std::vector<std::string> * add = NULL, bool playerColoredButton = false );//c-tor
};

template <typename T>
AdventureMapButton<T>::AdventureMapButton ()
{
	type=2;
	abs=true;
	active=false;
	ourObj=NULL;
	state=0;
	actOnDown = false;
}
template <typename T>
AdventureMapButton<T>::AdventureMapButton
( std::string Name, std::string HelpBox, void(T::*Function)(), int x, int y, std::string defName, T* Owner, bool activ, std::vector<std::string> * add, bool playerColoredButton )
{
	actOnDown = false;
	owner = Owner;
	type=2;
	abs=true;
	active=false;
	ourObj=NULL;
	state=0;
	name=Name;
	helpBox=HelpBox;
	colorChange = playerColoredButton;
	int est = LOCPLINT->playerID;
	CDefHandler * temp = CGI->spriteh->giveDef(defName); 
	temp->notFreeImgs = true;
	for (int i=0;i<temp->ourImages.size();i++)
	{
		imgs.resize(1);
		imgs[0].push_back(temp->ourImages[i].bitmap);
		if(playerColoredButton)
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
				if(playerColoredButton)
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
	if (actOnDown && down)
	{
		pressedL=state;
		(owner->*function)();
	}
	else if (pressedL && (down==false))
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
	if(name.size()) //if there is no name, there is nohing to display also
	{
		if (on)
			LOCPLINT->statusbar->print(name);
		else if (LOCPLINT->statusbar->getCurrent()==name)
			LOCPLINT->statusbar->clear();
	}
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
			LOCPLINT->statusbar->print(CGI->preth->zelp[306].first);
		else
			LOCPLINT->statusbar->clear();
		return;
	}
	else if(isItIn(&arrdop,LOCPLINT->current->motion.x,LOCPLINT->current->motion.y))
	{
		if ((items.size()-from)  >  SIZE)
			LOCPLINT->statusbar->print(CGI->preth->zelp[307].first);
		else
			LOCPLINT->statusbar->clear();
		return;
	}
	//if not buttons then towns
	int hx = LOCPLINT->current->motion.x, hy = LOCPLINT->current->motion.y;
	hx-=pos.x;
	hy-=pos.y; hy-=arrup->ourImages[0].bitmap->h;
	int ny = hy/32;
	if ((ny>SIZE || ny<0) || (from+ny>=items.size()))
	{
		LOCPLINT->statusbar->clear();
		return;
	};
	LOCPLINT->statusbar->print(items[from+ny]->state->hoverText(const_cast<CGTownInstance*>(items[from+ny])));
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
		int ny = hy/32;
		if (ny>SIZE || ny<0)
			return;
		if (SIZE==5 && (ny+from)==selected && (LOCPLINT->adventureInt->selection.type == TOWNI_TYPE))
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
			LOCPLINT->adventureInt->handleRightClick(CGI->preth->zelp[306].second,down,this);
		}
		else if(isItIn(&arrdop,LOCPLINT->current->motion.x,LOCPLINT->current->motion.y) && (items.size()-from>5))
		{
			LOCPLINT->adventureInt->handleRightClick(CGI->preth->zelp[307].second,down,this);
		}
		//if not buttons then towns
		int hx = LOCPLINT->current->motion.x, hy = LOCPLINT->current->motion.y;
		hx-=pos.x;
		hy-=pos.y; hy-=arrup->ourImages[0].bitmap->h;
		int ny = hy/32;
		if ((ny>5 || ny<0) || (from+ny>=items.size()))
		{
			return;
		}

		//show popup
		CInfoPopup * ip = new CInfoPopup(LOCPLINT->townWins[items[from+ny]->identifier],LOCPLINT->current->motion.x-LOCPLINT->townWins[items[from+ny]->identifier]->w,LOCPLINT->current->motion.y-LOCPLINT->townWins[items[from+ny]->identifier]->h,false);
		ip->activate();
	}
	else
	{
			LOCPLINT->adventureInt->handleRightClick(CGI->preth->zelp[306].second,down,this);
			LOCPLINT->adventureInt->handleRightClick(CGI->preth->zelp[307].second,down,this);
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


template<typename T>
class CSlider : public IShowable, public MotionInterested, public ClickableL
{
public:
	AdventureMapButton<CSlider> left, right, slider; //if vertical then left=up
	int capacity,//how many elements can be active at same time
		amount, //how many elements
		value; //first active element
	bool horizontal, moving;
	CDefEssential *imgs ;

	T* owner;
	void(T::*moved)(int to);

	void redrawSlider(); 

	void sliderClicked();
	void moveLeft();
	void clickLeft (tribool down);
	void mouseMoved (SDL_MouseMotionEvent & sEvent);
	void moveRight();
	void moveTo(int to);
	void activate(); // makes button active
	void deactivate(); // makes button inactive (but doesn't delete)
	void show(SDL_Surface * to = NULL);
	CSlider(int x, int y, int totalw, T*Owner,void(T::*Moved)(int to), int Capacity, int Amount, 
		int Value=0, bool Horizontal=true);
	~CSlider();
};	

template<typename T>
void CSlider<T>::sliderClicked()
{
	if(!moving)
	{
		MotionInterested::activate();
		moving = true;
	}
}
template<typename T>
void CSlider<T>::mouseMoved (SDL_MouseMotionEvent & sEvent)
{
	float v = sEvent.x - pos.x - 24;
	v/= (pos.w - 48);
	v*=amount;
	if(v!=value)
	{
		moveTo(v);
		redrawSlider();
	}
}
template<typename T>
void CSlider<T>::redrawSlider()
{
	slider.show();
}
template<typename T>
void CSlider<T>::moveLeft()
{
	moveTo(value-1);
}
template<typename T>
void CSlider<T>::moveRight()
{
	moveTo(value+1);
}
template<typename T>
void CSlider<T>::moveTo(int to)
{
	if(to<0)
		to=0;
	else if(to>amount)
		to=amount;
	value = to;
	float part = (float)to/amount;
	part*=(pos.w-48);
	slider.pos.x = part + pos.x + 16;
	(owner->*moved)(to);
}
template<typename T>
void CSlider<T>::activate() // makes button active
{
	left.activate();
	right.activate();
	slider.activate();
	ClickableL::activate();
}
template<typename T>
void CSlider<T>::deactivate() // makes button inactive (but doesn't delete)
{
	left.deactivate();
	right.deactivate();
	slider.deactivate();
	ClickableL::deactivate();
}
template<typename T>
void CSlider<T>::clickLeft (tribool down)
{
	if(down)
	{
		float pw = LOCPLINT->current->motion.x-pos.x-16;
		float rw = pw / ((float)(pos.w-32));
		if (rw>1) return;
		if (rw<0) return;
		moveTo(rw*amount);
		return;
	}
	if(moving)
	{
		MotionInterested::deactivate();
		moving = false;
	}
}
template<typename T>
void CSlider<T>::show(SDL_Surface * to)
{
	left.show();
	right.show();
	slider.show();
}
template<typename T>
CSlider<T>::CSlider(int x, int y, int totalw, T*Owner,void(T::*Moved)(int to), int Capacity, int Amount, int Value, bool Horizontal)
:capacity(Capacity),amount(Amount),value(Value),horizontal(Horizontal), moved(Moved), owner(Owner)
{
	moving = false;
	strongInterest = true;
	imgs = CGI->spriteh->giveDefEss("IGPCRDIV.DEF");

	left.pos.y = slider.pos.y = right.pos.y = pos.y = y;
	left.pos.x = pos.x = x;
	right.pos.x = x + totalw - 16;

	left.owner = right.owner = slider.owner = this;
	left.function = &CSlider::moveLeft;
	right.function = &CSlider::moveRight;
	slider.function = &CSlider::sliderClicked;
	left.pos.w = left.pos.h = right.pos.w = right.pos.h = slider.pos.w = slider.pos.h = pos.h = 16;
	pos.w = totalw;
	left.imgs.resize(1); right.imgs.resize(1); slider.imgs.resize(1);
	left.imgs[0].push_back(imgs->ourImages[0].bitmap); left.imgs[0].push_back(imgs->ourImages[1].bitmap);
	right.imgs[0].push_back(imgs->ourImages[2].bitmap); right.imgs[0].push_back(imgs->ourImages[3].bitmap);
	slider.imgs[0].push_back(imgs->ourImages[4].bitmap);
	left.notFreeButton = right.notFreeButton = slider.notFreeButton = true;
	slider.actOnDown = true;

	moveTo(value);
}
template<typename T>
CSlider<T>::~CSlider()
{
	delete imgs;
}