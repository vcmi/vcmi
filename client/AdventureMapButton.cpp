#include "AdventureMapButton.h"
#include "CAdvmapInterface.h"
#include "SDL_Extensions.h"
#include "../hch/CDefHandler.h"
#include "CGameInfo.h"
#include "../hch/CLodHandler.h"
#include "../hch/CGeneralTextHandler.h"
#include "../hch/CTownHandler.h"
#include "../CCallback.h"
#include "CConfigHandler.h"
#include "Graphics.h"
#include "CBattleInterface.h"
#include "CPlayerInterface.h"
#include "CMessage.h"

/*
 * AdevntureMapButton.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

CButtonBase::CButtonBase()
{
	bitmapOffset = 0;
	curimg=0;
	type=-1;
	abs=false;
	//active=false;
	notFreeButton = false;
	ourObj=NULL;
	state=0;
	text = NULL;
}

CButtonBase::~CButtonBase()
{
	delete text;
	if(notFreeButton)
		return;
	for(int i =0; i<imgs.size();i++)
		for(int j=0;j<imgs[i].size();j++)
			SDL_FreeSurface(imgs[i][j]);
}

void CButtonBase::show(SDL_Surface * to)
{
	int img = std::min(state+bitmapOffset,int(imgs[curimg].size()-1));
	img = std::max(0, img);

	if (abs)
	{
		blitAt(imgs[curimg][img],pos.x,pos.y,to);
		if(text)
		{
			CSDL_Ext::printAt(text->text, text->x + pos.x + state, text->y + pos.y + state, text->font, zwykly, to);
		}
	}
	else
	{
		blitAt(imgs[curimg][img],pos.x+ourObj->pos.x,pos.y+ourObj->pos.y,to);
	}
}

void CButtonBase::showAll( SDL_Surface * to )
{
	show(to);
}

void CButtonBase::addTextOverlay( const std::string Text, EFonts font )
{
	text = new TextOverlay;
	text->text = Text;

	const Font *f = graphics->fonts[font];
	text->x = pos.w/2 - f->getWidth(Text.c_str())/2;
	text->y = pos.h/2 - f->height/2;
	text->font = font;
}


AdventureMapButton::AdventureMapButton ()
{
	type=2;
	abs=true;
	hoverable = false;
	//active=false;
	ourObj=NULL;
	state=0;
	blocked = actOnDown = false;
}
//AdventureMapButton::AdventureMapButton( std::string Name, std::string HelpBox, boost::function<void()> Callback, int x, int y, std::string defName, bool activ,  std::vector<std::string> * add, bool playerColoredButton)
//{
//	init(Callback, Name, HelpBox, playerColoredButton, defName, add, x, y, activ);
//}
AdventureMapButton::AdventureMapButton( const std::string &Name, const std::string &HelpBox, const CFunctionList<void()> &Callback, int x, int y,  const std::string &defName,int key, std::vector<std::string> * add, bool playerColoredButton )
{
	std::map<int,std::string> pom;
	pom[0] = Name;
	init(Callback, pom, HelpBox, playerColoredButton, defName, add, x, y, key);
}

AdventureMapButton::AdventureMapButton( const std::map<int,std::string> &Name, const std::string &HelpBox, const CFunctionList<void()> &Callback, int x, int y, const std::string &defName, int key, std::vector<std::string> * add /*= NULL*/, bool playerColoredButton /*= false */ )
{
	init(Callback, Name, HelpBox, playerColoredButton, defName, add, x, y, key);
}

AdventureMapButton::AdventureMapButton( const std::string &Name, const std::string &HelpBox, const CFunctionList<void()> &Callback, config::ButtonInfo *info, int key/*=0*/ )
{
	std::map<int,std::string> pom;
	pom[0] = Name;
	init(Callback, pom, HelpBox, info->playerColoured, info->defName, &info->additionalDefs, info->x, info->y, key);
}

AdventureMapButton::AdventureMapButton( const std::pair<std::string, std::string> help, const CFunctionList<void()> &Callback, int x, int y, const std::string &defName, int key/*=0*/, std::vector<std::string> * add /*= NULL*/, bool playerColoredButton /*= false */ )
{
	std::map<int,std::string> pom;
	pom[0] = help.first;
	init(Callback, pom, help.second, playerColoredButton, defName, add, x, y, key);
}
void AdventureMapButton::clickLeft(tribool down, bool previousState)
{
	if(blocked)
		return;
	if (down)
		state=1;
	else
		state=0;
	show(screenBuf);
	if (actOnDown && down)
	{
		callback();
	}
	else if (!actOnDown && previousState && (down==false))
	{
		callback();
	}
}

void AdventureMapButton::clickRight(tribool down, bool previousState)
{
	if(down && helpBox.size()) //there is no point to show window with nothing inside...
		if(LOCPLINT)
			LOCPLINT->adventureInt->handleRightClick(helpBox,down,this);
		else
			GH.pushInt(new CRClickPopupInt(CMessage::genWindow(helpBox, 0),true));
}

void AdventureMapButton::hover (bool on)
{
	if(blocked)
		return;

	if(hoverable)
	{
		if(on)
			state = 3;
		else
			state = 0;
	}

	if(pressedL)
	{
		if(on)
			state = 1;
		else
			state = hoverable ? 3 : 0;
	}

	////Hoverable::hover(on);
	std::string *name = (vstd::contains(hoverTexts,state)) 
							? (&hoverTexts[state]) 
							: (vstd::contains(hoverTexts,0) ? (&hoverTexts[0]) : NULL);
	if(LOCPLINT && name && name->size() && blocked!=1) //if there is no name, there is nohing to display also
	{
		if (LOCPLINT->battleInt) //for battle buttons
		{
			if(on && LOCPLINT->battleInt->console->alterTxt == "")
			{
				LOCPLINT->battleInt->console->alterTxt = *name;
				LOCPLINT->battleInt->console->whoSetAlter = 1;
			}
			else if (LOCPLINT->battleInt->console->alterTxt == *name)
			{
				LOCPLINT->battleInt->console->alterTxt = "";
				LOCPLINT->battleInt->console->whoSetAlter = 0;
			}
		}
		else //for other buttons
		{
			if (on)
				LOCPLINT->statusbar->print(*name);
			else if ( LOCPLINT->statusbar->getCurrent()==(*name) )
				LOCPLINT->statusbar->clear();
		}
	}
}

void AdventureMapButton::activate()
{
// 	if (active) return;
// 	active=true;
	activateLClick();
	activateRClick();
	activateHover();
	activateKeys();
}
void AdventureMapButton::deactivate()
{
// 	if (!active) return;
// 	active=false;
	deactivateLClick();
	deactivateRClick();
	deactivateHover();
	deactivateKeys();
}

void AdventureMapButton::init(const CFunctionList<void()> &Callback, const std::map<int,std::string> &Name, const std::string &HelpBox, bool playerColoredButton, const std::string &defName, std::vector<std::string> * add, int x, int y, int key)
{
	callback = Callback;
	blocked = actOnDown = false;
	type=2;
	abs=true;
	hoverable = false;
	//active=false;
	ourObj=NULL;
	assignedKeys.insert(key);
	state=0;
	hoverTexts = Name;
	helpBox=HelpBox;

	setDef(defName, playerColoredButton);

	if (add && add->size())
	{
		imgs.resize(imgs.size()+add->size());
		for (size_t i=0; i<add->size();i++)
		{
			CDefHandler *temp = CDefHandler::giveDef((*add)[i]);
			temp->notFreeImgs = true;
			for (size_t j=0;j<temp->ourImages.size();j++)
			{
				imgs[i+1].push_back(temp->ourImages[j].bitmap);
				if(playerColoredButton)
				{
					graphics->blueToPlayersAdv(imgs[1+i][j],LOCPLINT->playerID);
				}
			}
			delete temp;
		}
		//delete add;
	}
	pos.x += x;
	pos.y += y;
	pos.w = imgs[curimg][0]->w;
	pos.h = imgs[curimg][0]->h  -1;
}

void AdventureMapButton::block( ui8 on )
{
	blocked = on;
	state = 0;
	bitmapOffset = on ? 2 : 0;
	show(screenBuf);
}

void AdventureMapButton::setDef(const std::string & defName, bool playerColoredButton)
{
	CDefHandler * temp = CDefHandler::giveDef(defName); 
	temp->notFreeImgs = true;
	for (size_t i=0;i<temp->ourImages.size();i++)
	{
		imgs.resize(1);
		imgs[0].push_back(temp->ourImages[i].bitmap);
		if(playerColoredButton)
		{
			graphics->blueToPlayersAdv(imgs[curimg][i],LOCPLINT->playerID);
		}
	}
	delete temp;
}
void CHighlightableButton::select(bool on)
{
	selected = on;
	state = selected ? 3 : 0;

	if(selected)
		callback();
	else 
		callback2();
	if(hoverTexts.size()>1)
	{
		hover(false);
		hover(true);
	}
}

void CHighlightableButton::clickLeft(tribool down, bool previousState)
{
	if(blocked)
		return;
	if (down)
		state = 1;
	else
		state = selected ? 3 : 0;
	show(screenBuf);
	if(previousState  &&  down == false)
	{
		if(!onlyOn || !selected)
			select(!selected);
	}
}

CHighlightableButton::CHighlightableButton( const CFunctionList<void()> &onSelect, const CFunctionList<void()> &onDeselect, const std::map<int,std::string> &Name, const std::string &HelpBox, bool playerColoredButton, const std::string &defName, std::vector<std::string> * add, int x, int y, int key)
: onlyOn(false), selected(false), callback2(onDeselect)
{
	init(onSelect,Name,HelpBox,playerColoredButton,defName,add,x,y,key);
}

CHighlightableButton::CHighlightableButton( const std::pair<std::string, std::string> help, const CFunctionList<void()> &onSelect, int x, int y, const std::string &defName, int myid, int key/*=0*/, std::vector<std::string> * add /*= NULL*/, bool playerColoredButton /*= false */ )
: onlyOn(false), selected(false) // TODO: callback2(???)
{
	ID = myid;
	std::map<int,std::string> pom;
	pom[0] = help.first;
	init(onSelect, pom, help.second, playerColoredButton, defName, add, x, y, key);
}

CHighlightableButton::CHighlightableButton( const std::string &Name, const std::string &HelpBox, const CFunctionList<void()> &onSelect, int x, int y, const std::string &defName, int myid, int key/*=0*/, std::vector<std::string> * add /*= NULL*/, bool playerColoredButton /*= false */ )
: onlyOn(false), selected(false) // TODO: callback2(???)
{
	ID = myid;
	std::map<int,std::string> pom;
	pom[0] = Name;
	init(onSelect, pom,HelpBox, playerColoredButton, defName, add, x, y, key);
}

void CHighlightableButtonsGroup::addButton(CHighlightableButton* bt)
{
	bt->callback += boost::bind(&CHighlightableButtonsGroup::selectionChanged,this,bt->ID);
	buttons.push_back(bt);
}

void CHighlightableButtonsGroup::addButton(const std::map<int,std::string> &tooltip, const std::string &HelpBox, const std::string &defName, int x, int y, int uid, const CFunctionList<void()> &OnSelect, int key)
{
	CHighlightableButton *bt = new CHighlightableButton(OnSelect, 0, tooltip, HelpBox, false, defName, 0, x, y, key);
	if(musicLike)
	{
		bt->bitmapOffset = buttons.size() - 3;
	}
	bt->ID = uid;
	bt->callback += boost::bind(&CHighlightableButtonsGroup::selectionChanged,this,bt->ID);
	bt->onlyOn = true;
	buttons.push_back(bt);
}	

CHighlightableButtonsGroup::CHighlightableButtonsGroup(const CFunctionList2<void(int)> &OnChange, bool musicLikeButtons)
: musicLike(musicLikeButtons), onChange(OnChange)
{}

CHighlightableButtonsGroup::~CHighlightableButtonsGroup()
{
	for(size_t i=0;i<buttons.size();i++)
	{
		delete buttons[i];
	}
}

void CHighlightableButtonsGroup::activate()
{
	for(size_t i=0;i<buttons.size();i++)
	{
		buttons[i]->activate();
	}
}

void CHighlightableButtonsGroup::deactivate()
{
	for(size_t i=0;i<buttons.size();i++)
	{
		buttons[i]->deactivate();
	}
}

void CHighlightableButtonsGroup::select(int id, bool mode)
{
	CHighlightableButton *bt = NULL;
	if(mode)
	{
		for(size_t i=0;i<buttons.size() && !bt; ++i)
			if (buttons[i]->ID == id)
				bt = buttons[i];
	}
	else
	{
		bt = buttons[id];
	}
	bt->select(true);
	selectionChanged(bt->ID);
}

void CHighlightableButtonsGroup::selectionChanged(int to)
{
	for(size_t i=0;i<buttons.size(); ++i)
		if(buttons[i]->ID!=to && buttons[i]->selected)
			buttons[i]->select(false);
	onChange(to);
}

void CHighlightableButtonsGroup::show(SDL_Surface * to )
{
	for(size_t i=0;i<buttons.size(); ++i) 
	{
		if(!musicLike || (musicLike && buttons[i]->selected)) //if musicLike, print only selected button
			buttons[i]->show(to);
	}
}

void CHighlightableButtonsGroup::showAll( SDL_Surface * to )
{
	show(to);
}

void CHighlightableButtonsGroup::block( ui8 on )
{
	for(size_t i=0;i<buttons.size(); ++i) 
	{
		buttons[i]->block(on);
	}
}

void CSlider::sliderClicked()
{
	if(!moving)
	{
		activateMouseMove();
		moving = true;
	}
}

void CSlider::mouseMoved (const SDL_MouseMotionEvent & sEvent)
{
	float v = 0;
	if(horizontal)
	{
		if(	std::abs(sEvent.y-(pos.y+pos.h/2)) > pos.h/2+40  ||  std::abs(sEvent.x-(pos.x+pos.w/2)) > pos.w/2  ) 
			return;
		v = sEvent.x - pos.x - 24;
		v *= positions;
		v /= (pos.w - 48);
	}
	else
	{
		if(std::abs(sEvent.x-(pos.x+pos.w/2)) > pos.w/2+40  ||  std::abs(sEvent.y-(pos.y+pos.h/2)) > pos.h/2  ) 
			return;
		v = sEvent.y - pos.y - 24;
		v *= positions;
		v /= (pos.h - 48);
	}
	v += 0.5f;
	if(v!=value)
	{
		moveTo(v);
		redrawSlider();
	}
}

void CSlider::redrawSlider()
{
	slider->show(screenBuf);
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
	amax(to, 0);
	amin(to, positions);

	//same, old position?
	if(value == to)
		return;

	value = to;
	if(horizontal)
	{
		if(positions)
		{
			float part = (float)to/positions;
			part*=(pos.w-48);
			slider->pos.x = part + pos.x + 16;
		}
		else
			slider->pos.x = pos.x+16;
	}
	else
	{
		if(positions)
		{
			float part = (float)to/positions;
			part*=(pos.h-48);
			slider->pos.y = part + pos.y + 16;
		}
		else
			slider->pos.y = pos.y+16;
	}
	moved(to);
}

void CSlider::clickLeft(tribool down, bool previousState)
{
	if(down)
	{
		float pw = 0;
		float rw = 0;
		if(horizontal)
		{
			pw = GH.current->motion.x-pos.x-24;
			rw = pw / ((float)(pos.w-48));
		}
		else
		{
			pw = GH.current->motion.y-pos.y-24;
			rw = pw / ((float)(pos.h-48));
		}
		if(pw < -8  ||  pw > (horizontal ? pos.w : pos.h) + 8)
			return;
// 		if (rw>1) return;
// 		if (rw<0) return;
		moveTo(rw * positions  +  0.5f);
		return;
	}
	if(moving)
	{
		deactivateMouseMove();
		moving = false;
	}
}

CSlider::~CSlider()
{
	delete imgs;
}

CSlider::CSlider(int x, int y, int totalw, boost::function<void(int)> Moved, int Capacity, int Amount, int Value, bool Horizontal, int style)
:capacity(Capacity),amount(Amount),horizontal(Horizontal), moved(Moved)
{
	OBJ_CONSTRUCTION;
	setAmount(amount);

	used = LCLICK;
	moving = false;
	strongInterest = true;


	left = new AdventureMapButton;
	right = new AdventureMapButton;
	slider = new AdventureMapButton;

	pos.x += x;
	pos.y += y;

	if(horizontal)
	{
		left->pos.y = slider->pos.y = right->pos.y = pos.y;
		left->pos.x = pos.x;
		right->pos.x = pos.x + totalw - 16;
	}
	else 
	{
		left->pos.x = slider->pos.x = right->pos.x = pos.x;
		left->pos.y = pos.y;
		right->pos.y = pos.y + totalw - 16;
	}

	left->callback = boost::bind(&CSlider::moveLeft,this);
	right->callback = boost::bind(&CSlider::moveRight,this);
	slider->callback = boost::bind(&CSlider::sliderClicked,this);
	left->pos.w = left->pos.h = right->pos.w = right->pos.h = slider->pos.w = slider->pos.h = 16;
	if(horizontal)
	{
		pos.h = 16;
		pos.w = totalw;
	}
	else
	{
		pos.w = 16;
		pos.h = totalw;
	}

	if(style == 0)
	{
		imgs = CDefHandler::giveDefEss("IGPCRDIV.DEF");
		left->imgs.resize(1); right->imgs.resize(1); slider->imgs.resize(1);
		left->imgs[0].push_back(imgs->ourImages[0].bitmap); left->imgs[0].push_back(imgs->ourImages[1].bitmap);
		right->imgs[0].push_back(imgs->ourImages[2].bitmap); right->imgs[0].push_back(imgs->ourImages[3].bitmap);
		slider->imgs[0].push_back(imgs->ourImages[4].bitmap);
		left->notFreeButton = right->notFreeButton = slider->notFreeButton = true;
	}
	else
	{
		imgs = NULL;
		left->setDef(horizontal ? "SCNRBLF.DEF" : "SCNRBUP.DEF", false);
		right->setDef(horizontal ? "SCNRBRT.DEF" : "SCNRBDN.DEF", false);
		slider->setDef("SCNRBSL.DEF", false);
	}
	slider->actOnDown = true;

	value = -1;
	moveTo(Value);
}

void CSlider::block( bool on )
{
	left->block(on);
	right->block(on);
	slider->block(on);
}

void CSlider::setAmount( int to )
{
	amount = to;
	positions = to - capacity;
	amax(positions, 0);
}
