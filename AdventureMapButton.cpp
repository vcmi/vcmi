#include "CAdvmapInterface.h"
#include "SDL_Extensions.h"
#include "hch/CDefHandler.h"
#include "CGameInfo.h"
#include "hch/CLodHandler.h"
#include "hch/CPreGameTextHandler.h"
#include "hch/CTownHandler.h"
#include "CLua.h"
#include "CCallback.h"
AdventureMapButton::AdventureMapButton ()
{
	type=2;
	abs=true;
	active=false;
	ourObj=NULL;
	state=0;
	actOnDown = false;
}

AdventureMapButton::AdventureMapButton
( std::string Name, std::string HelpBox, boost::function<void()> Callback, int x, int y, std::string defName, bool activ,  std::vector<std::string> * add, bool playerColoredButton )
{
	callback = Callback;
	actOnDown = false;
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
	if (actOnDown && down)
	{
		pressedL=state;
		if(!callback.empty())
			callback();
	}
	else if (pressedL && (down==false))
	{
		pressedL=state;
		if(!callback.empty())
			callback();
	}
	else
	{
		pressedL=state;
	}
}

void AdventureMapButton::clickRight (tribool down)
{
	if(helpBox.size()) //there is no point to show window with nothing inside...
		LOCPLINT->adventureInt->handleRightClick(helpBox,down,this);
}

void AdventureMapButton::hover (bool on)
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

void AdventureMapButton::activate()
{
	if (active) return;
	active=true;
	ClickableL::activate();
	ClickableR::activate();
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
	ClickableR::deactivate();
	Hoverable::deactivate();
	KeyInterested::deactivate();
}


void CSlider::sliderClicked()
{
	if(!moving)
	{
		MotionInterested::activate();
		moving = true;
	}
}

void CSlider::mouseMoved (SDL_MouseMotionEvent & sEvent)
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

void CSlider::redrawSlider()
{
	slider.show();
}

void CSlider::moveLeft()
{
	moveTo(value-1);
}

void CSlider::moveRight()
{
	moveTo(value+1);
}

void CSlider::moveTo(int to)
{
	if(to<0)
		to=0;
	else if(to>amount)
		to=amount;
	value = to;
	if(amount)
	{
		float part = (float)to/amount;
		part*=(pos.w-48);
		slider.pos.x = part + pos.x + 16;
	}
	else
		slider.pos.x = pos.x+16;
	moved(to);
}

void CSlider::activate() // makes button active
{
	left.activate();
	right.activate();
	slider.activate();
	ClickableL::activate();
}

void CSlider::deactivate() // makes button inactive (but doesn't delete)
{
	left.deactivate();
	right.deactivate();
	slider.deactivate();
	ClickableL::deactivate();
}

void CSlider::clickLeft (tribool down)
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

void CSlider::show(SDL_Surface * to)
{
	left.show();
	right.show();
	slider.show();
}

CSlider::~CSlider()
{
	delete imgs;
}

CSlider::CSlider(int x, int y, int totalw, boost::function<void(int)> Moved, int Capacity, int Amount, int Value, bool Horizontal)
:capacity(Capacity),amount(Amount),value(Value),horizontal(Horizontal), moved(Moved)
{
	moving = false;
	strongInterest = true;
	imgs = CGI->spriteh->giveDefEss("IGPCRDIV.DEF");

	left.pos.y = slider.pos.y = right.pos.y = pos.y = y;
	left.pos.x = pos.x = x;
	right.pos.x = x + totalw - 16;
	left.callback = boost::bind(&CSlider::moveLeft,this);
	right.callback = boost::bind(&CSlider::moveRight,this);
	slider.callback = boost::bind(&CSlider::sliderClicked,this);
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
