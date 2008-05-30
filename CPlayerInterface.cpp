#include "stdafx.h"
#include "CPlayerInterface.h"
#include "CAdvmapInterface.h"
#include "CMessage.h"
#include "mapHandler.h"
#include "SDL_Extensions.h"
#include "SDL_framerate.h"
#include "CScreenHandler.h"
#include "CCursorHandler.h"
#include "CCallback.h"
#include "SDL_Extensions.h"
#include "hch/CLodHandler.h"
#include "CPathfinder.h"
#include <sstream>
#include "hch/CHeroHandler.h"
#include "hch/CTownHandler.h"
#include "SDL_framerate.h"
#include "hch/CGeneralTextHandler.h"
#include "CCastleInterface.h"
#include "CHeroWindow.h"
#include "timeHandler.h"
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/replace.hpp>
#include "hch\CPreGameTextHandler.h"
#include "CBattleInterface.h"
#include "CLua.h"
#include <cmath>
using namespace CSDL_Ext;

extern TTF_Font * GEOR16;

class OCM_HLP_CGIN
{
public:
	bool operator ()(const std::pair<CGObjectInstance*,SDL_Rect>  & a, const std::pair<CGObjectInstance*,SDL_Rect> & b) const
	{
		return (*a.first)<(*b.first);
	}
} ocmptwo_cgin ;

void CGarrisonSlot::hover (bool on)
{
	Hoverable::hover(on);
	if(on)
	{
		std::string temp;
		if(creature)
		{
			if(owner->highlighted)
			{
				if(owner->highlighted == this)
				{
					temp = CGI->townh->tcommands[4];
					boost::algorithm::replace_first(temp,"%s",creature->nameSing);
				}
				else if (owner->highlighted->creature == creature)
				{
					temp = CGI->townh->tcommands[2];
					boost::algorithm::replace_first(temp,"%s",creature->nameSing);
				}
				else if (owner->highlighted->creature)
				{
					temp = CGI->townh->tcommands[7];
					boost::algorithm::replace_first(temp,"%s",owner->highlighted->creature->nameSing);
					boost::algorithm::replace_first(temp,"%s",creature->nameSing);
				}
				else
				{
					std::cout << "Warning - shouldn't be - highlighted void slot "<<owner->highlighted<<std::endl;
					std::cout << "Highlighted set to NULL"<<std::endl;
					owner->highlighted = NULL;
				}
			}
			else
			{
				if(upg)
				{
					temp = CGI->townh->tcommands[32];
				}
				else
				{
					temp = CGI->townh->tcommands[12];
				}
				boost::algorithm::replace_first(temp,"%s",creature->nameSing);
			};
		}
		else
		{
			if(owner->highlighted)
			{
					temp = CGI->townh->tcommands[6];
					boost::algorithm::replace_first(temp,"%s",owner->highlighted->creature->nameSing);
			}
			else
			{
				temp = CGI->townh->tcommands[11];
			}
		}
		LOCPLINT->statusbar->print(temp);
	}
	else
	{
		LOCPLINT->statusbar->clear();
	}
}

const CGObjectInstance * CGarrisonSlot::getObj()
{
	return 	(!upg)?(owner->oup):(owner->odown);
}

void CGarrisonSlot::clickRight (tribool down)
{
	StackState *pom = NULL;
	if(down && creature)
	{
		if(getObj()->ID == 34)
		{
			pom = new StackState();
			const CGHeroInstance *h = static_cast<const CGHeroInstance *>(getObj());
			pom->attackBonus = h->primSkills[0];
			pom->defenseBonus = h->primSkills[1];
			pom->luck = h->getCurrentLuck();
			pom->morale = h->getCurrentMorale();
		}
		(new CCreInfoWindow(creature->idNumber,0,pom,boost::function<void()>(),boost::function<void()>()))
				->activate();
	}
	delete pom;
}
void CGarrisonSlot::clickLeft(tribool down)
{
	if(owner->ignoreEvent)
	{
		owner->ignoreEvent = false;
		return;
	}
	if(down)
	{
		bool refr = false;
		if(owner->highlighted)
		{
			if(owner->highlighted == this) //view info
			{
				//TODO: view creature info
				owner->highlighted = NULL;
				show();
				refr = true;
			}
			else if( !creature && owner->splitting)//split
			{
				owner->p2 = ID;
				owner->pb = upg;
				owner->splitting = false;
				LOCPLINT->curint->deactivate();
				CSplitWindow * spw = new CSplitWindow(owner->highlighted->creature->idNumber,owner->highlighted->count, owner);
				spw->activate();
				refr = true;
			}
			else if(creature != owner->highlighted->creature) //swap
			{
				LOCPLINT->cb->swapCreatures(
					(!upg)?(owner->oup):(owner->odown),
					(!owner->highlighted->upg)?(owner->oup):(owner->odown),
					ID,owner->highlighted->ID);
			}
			else //merge
			{
				LOCPLINT->cb->mergeStacks(
					(!owner->highlighted->upg)?(owner->oup):(owner->odown),
					(!upg)?(owner->oup):(owner->odown),
					owner->highlighted->ID,ID);
			}
		}
		else //highlight
		{
			if(creature)
				owner->highlighted = this;
			show();
			refr = true;
		}
		if(refr) {hover(false);	hover(true); } //to refresh statusbar
	}
}
void CGarrisonSlot::activate()
{
	if(!active) active=true;
	else return;
	ClickableL::activate();
	ClickableR::activate();
	Hoverable::activate();
}
void CGarrisonSlot::deactivate()
{
	if(active) active=false;
	else return;
	ClickableL::deactivate();
	ClickableR::deactivate();
	Hoverable::deactivate();
}
CGarrisonSlot::CGarrisonSlot(CGarrisonInt *Owner, int x, int y, int IID, int Upg, const CCreature * Creature, int Count)
{
	active = false;
	upg = Upg;
	count = Count;
	ID = IID;
	creature = Creature;
	pos.x = x;
	pos.y = y;
	pos.w = 58;
	pos.h = 64;
	owner = Owner;
}
CGarrisonSlot::~CGarrisonSlot()
{
	if(active)
		deactivate();
}
void CGarrisonSlot::show()
{
	if(creature)
	{
		char* buf = new char[15];
		itoa(count,buf,10);
		blitAt(CGI->creh->bigImgs[creature->idNumber],pos);
		printTo(buf,pos.x+56,pos.y+62,GEOR16,zwykly);
		if(owner->highlighted==this)
			blitAt(CGI->creh->bigImgs[-1],pos);
		if(owner->update)
			updateRect(&pos,screen);
		delete [] buf;
	}
	else
	{
		SDL_Rect jakis1 = genRect(pos.h,pos.w,owner->offx+ID*(pos.w+owner->interx),owner->offy+upg*(pos.h+owner->intery)), jakis2 = pos;
		SDL_BlitSurface(owner->sur,&jakis1,screen,&jakis2);
		if(owner->splitting)
			blitAt(CGI->creh->bigImgs[-1],pos);
		if(owner->update)
			SDL_UpdateRect(screen,pos.x,pos.y,pos.w,pos.h);
	}
}
CGarrisonInt::~CGarrisonInt()
{
	if(sup)
	{
		for(int i=0;i<sup->size();i++)
			delete (*sup)[i];
		delete sup;
	}
	if(sdown)
	{
		for(int i=0;i<sdown->size();i++)
			delete (*sdown)[i];
		delete sdown;
	}
}

void CGarrisonInt::show()
{
	if(sup)
	{
		for(int i = 0; i<sup->size(); i++)
			if((*sup)[i])
				(*sup)[i]->show();
	}
	if(sdown)
	{
		for(int i = 0; i<sdown->size(); i++)
			if((*sdown)[i])
				(*sdown)[i]->show();
	}
}
void CGarrisonInt::deactiveteSlots()
{
	if(sup)
	{
		for(int i = 0; i<sup->size(); i++)
		{
			if((*sup)[i])
			{
				(*sup)[i]->deactivate();
			}
		}
	}
	if(sdown)
	{
		for(int i = 0; i<sdown->size(); i++)
		{
			if((*sdown)[i])
			{
				(*sdown)[i]->deactivate();
			}
		}
	}
}
void CGarrisonInt::activeteSlots()
{
	if(sup)
	{
		for(int i = 0; i<sup->size(); i++)
		{
			if((*sup)[i])
			{
				(*sup)[i]->activate();
			}
		}
	}
	if(sdown)
	{
		for(int i = 0; i<sdown->size(); i++)
		{
			if((*sdown)[i])
			{
				(*sdown)[i]->activate();
			}
		}
	}
}
void CGarrisonInt::createSlots()
{
	if(set1)
	{
		sup = new std::vector<CGarrisonSlot*>(7,(CGarrisonSlot *)(NULL));
		for
			(std::map<int,std::pair<CCreature*,int> >::const_iterator i=set1->slots.begin();
			i!=set1->slots.end(); i++)
		{
			(*sup)[i->first] = 
				new CGarrisonSlot(this, pos.x + (i->first*(58+interx)), pos.y,i->first, 0, i->second.first,i->second.second);
		}
		for(int i=0; i<sup->size(); i++)
			if((*sup)[i] == NULL)
				(*sup)[i] = new CGarrisonSlot(this, pos.x + (i*(58+interx)), pos.y,i,0,NULL, 0);
	}
	if(set2)
	{	
		sdown = new std::vector<CGarrisonSlot*>(7,(CGarrisonSlot *)(NULL));
		for
			(std::map<int,std::pair<CCreature*,int> >::const_iterator i=set2->slots.begin();
			i!=set2->slots.end(); i++)
		{
			(*sdown)[i->first] = 
				new CGarrisonSlot(this, pos.x + (i->first*(58+interx)), pos.y + 64 + intery,i->first,1, i->second.first,i->second.second);
		}
		for(int i=0; i<sup->size(); i++)
			if((*sdown)[i] == NULL)
				(*sdown)[i] = new CGarrisonSlot(this, pos.x + (i*(58+interx)), pos.y + 64 + intery,i,1, NULL, 0);
	}
}
void CGarrisonInt::deleteSlots()
{
	if(sup)
	{
		for(int i = 0; i<sup->size(); i++)
		{
			if((*sup)[i])
			{
				delete (*sup)[i];
			}
		}
	}
	if(sdown)
	{
		for(int i = 0; i<sdown->size(); i++)
		{
			if((*sdown)[i])
			{
				delete (*sdown)[i];
			}
		}
	}
}
void CGarrisonInt::recreateSlots()
{
	splitting = false;
	highlighted = NULL;
	deactiveteSlots();
	deleteSlots();
	createSlots();
	ignoreEvent = true;
	activeteSlots();
	show();
}
void CGarrisonInt::splitClick()
{
	if(!highlighted)
		return;
	splitting = !splitting;
	show();
}
void CGarrisonInt::splitStacks(int am2)
{
	LOCPLINT->cb->splitStack(
		(highlighted->upg)?(odown):(oup),
		(pb)?(odown):(oup),
		highlighted->ID,
		p2,
		am2);

}
CGarrisonInt::CGarrisonInt(int x, int y, int inx, int iny, SDL_Surface *pomsur, int OX, int OY, const CGObjectInstance *s1, const CGObjectInstance *s2)
	:interx(inx),intery(iny),sur(pomsur),highlighted(NULL),sup(NULL),sdown(NULL),oup(s1),odown(s2),
	offx(OX),offy(OY)
{
	splitting = false;
	set1 = LOCPLINT->cb->getGarrison(s1);
	set2 = LOCPLINT->cb->getGarrison(s2);
	ignoreEvent = false;
	update = true;
	pos.x=(x);
	pos.y=(y);
	pos.w=(58);
	pos.h=(64);
	createSlots();
}

void CGarrisonInt::activate()
{
	if(sup)
	{
		for(int i = 0; i<sup->size(); i++)
			if((*sup)[i])
				(*sup)[i]->activate();
	}
	if(sdown)
	{
		for(int i = 0; i<sdown->size(); i++)
			if((*sdown)[i])
				(*sdown)[i]->activate();
	}
}
void CGarrisonInt::deactivate()
{
	deactiveteSlots();
}

CInfoWindow::CInfoWindow()
:okb(NMessage::ok,NULL,&CInfoWindow::okClicked)
{
	okb.ourObj = this;
	okb.delg = this;
	okb.notFreeButton=true;
}

void CInfoWindow::okClicked(tribool down)
{
	if (!down)
		close();
}

void CInfoWindow::close()
{
	for (int i=0;i<components.size();i++)
	{
		components[i]->deactivate();
		delete components[i];
	}
	components.clear();
	okb.deactivate();
	SDL_FreeSurface(bitmap);
	bitmap = NULL;
	LOCPLINT->removeObjToBlit(this);
	LOCPLINT->curint->activate();
	delete this;
}
CInfoWindow::~CInfoWindow()
{
}

void CRClickPopup::clickRight (tribool down)
{
	if(down)
		return;
	close();
}

void CRClickPopup::activate()
{
	ClickableR::activate();
	LOCPLINT->objsToBlit.push_back(this);
}

void CRClickPopup::deactivate()
{
	ClickableR::deactivate();
	LOCPLINT->objsToBlit.erase(std::find(LOCPLINT->objsToBlit.begin(),LOCPLINT->objsToBlit.end(),this));
}

CInfoPopup::CInfoPopup(SDL_Surface * Bitmap, int x, int y, bool Free)
:bitmap(Bitmap),free(Free)
{
	pos.x = x;
	pos.y = y;
	pos.h = bitmap->h;
	pos.w = bitmap->w;
}
void CInfoPopup::close()
{
	deactivate();
	if(free)
		SDL_FreeSurface(bitmap);
	delete this;
	if(LOCPLINT->curint == LOCPLINT->adventureInt)
		LOCPLINT->adventureInt->show();
	else if(LOCPLINT->curint == LOCPLINT->castleInt)
		LOCPLINT->castleInt->showAll();
}
void CInfoPopup::show(SDL_Surface * to)
{
	blitAt(bitmap,pos.x,pos.y,(to)?(to):(screen));
}

SComponent::SComponent(Etype Type, int Subtype, int Val)
{
	std::ostringstream oss;
	switch (Type)
	{
	case primskill:
		description = CGI->generaltexth->arraytxt[2+Subtype];
		oss << ((Val>0)?("+"):("-")) << Val << " " << CGI->heroh->pskillsn[Subtype];
		subtitle = oss.str();
		break;
	case resource:
		description = CGI->generaltexth->allTexts[242];
		oss << Val;
		subtitle = oss.str();
		break;
	case experience:
		description = CGI->generaltexth->allTexts[241];
		oss << Val ;
		subtitle = oss.str();
		break;
	}
	type = Type;
	subtype = Subtype;
	val = Val;
	SDL_Surface * temp = getImg();
	pos.w = temp->w;
	pos.h = temp->h;
}

SDL_Surface * SComponent::getImg()
{
	switch (type)
	{
	case primskill:
		return CGI->heroh->pskillsb->ourImages[subtype].bitmap;
		break;
	case secskill:
		return CGI->abilh->abils82->ourImages[subtype*3 + 3 + val].bitmap;
		break;
	case resource:
		return CGI->heroh->resources->ourImages[subtype].bitmap;
		break;
	case experience:
		return CGI->heroh->pskillsb->ourImages[4].bitmap;
		break;
	}
	return NULL;
}
void SComponent::clickRight (tribool down)
{
	LOCPLINT->adventureInt->handleRightClick(description,down,this);
}
void SComponent::activate()
{
	ClickableR::activate();
}
void SComponent::deactivate()
{
	ClickableR::deactivate();
}

void CSelectableComponent::clickLeft(tribool down)
{
	if (down)
	{
		select(true);
		owner->selectionChange(this);
	}
}
CSelectableComponent::CSelectableComponent(Etype Type, int Sub, int Val, CSelWindow * Owner, SDL_Surface * Border)
:SComponent(Type,Sub,Val),owner(Owner)
{
	SDL_Surface * symb = SComponent::getImg();
	myBitmap = CSDL_Ext::newSurface(symb->w+2,symb->h+2,screen);
	SDL_SetColorKey(myBitmap,SDL_SRCCOLORKEY,SDL_MapRGB(myBitmap->format,0,255,255));	
	blitAt(symb,1,1,myBitmap);
	if (Border) //use custom border
	{
		border = Border;
		customB = true;
	}
	else //we need to draw border
	{
		customB = false;
		border = CSDL_Ext::newSurface(symb->w+2,symb->h+2,screen);
		SDL_FillRect(border,NULL,0x00FFFF);
		for (int i=0;i<border->w;i++)
		{
			SDL_PutPixel(border,i,0,239,215,123);
			SDL_PutPixel(border,i,(border->h)-1,239,215,123);
		}
		for (int i=0;i<border->h;i++)
		{
			SDL_PutPixel(border,0,i,239,215,123);
			SDL_PutPixel(border,(border->w)-1,i,239,215,123);
		}
		SDL_SetColorKey(border,SDL_SRCCOLORKEY,SDL_MapRGB(border->format,0,255,255));	
	}
	selected = false;
}
CSelectableComponent::~CSelectableComponent()
{
	SDL_FreeSurface(myBitmap);
	if (!customB)
		SDL_FreeSurface(border);
}
void CSelectableComponent::activate()
{
	SComponent::activate();
	ClickableL::activate();
}
void CSelectableComponent::deactivate()
{
	SComponent::deactivate();
	ClickableL::deactivate();
}
SDL_Surface * CSelectableComponent::getImg()
{
	return myBitmap;
}
void CSelectableComponent::select(bool on)
{
	if(on != selected)
	{
		SDL_FillRect(myBitmap,NULL,0x000000);
		blitAt(SComponent::getImg(),1,1,myBitmap);
		if (on)
		{
			blitAt(border,0,0,myBitmap);
		}
		selected = on;
		return;
	}
	else 
	{
		return;
	}
}

void CSimpleWindow::show(SDL_Surface * to)
{
	if(!to)
		to=screen;
	blitAt(bitmap,pos.x,pos.y,to);
}
CSimpleWindow::~CSimpleWindow()
{
	if (bitmap)
	{
		SDL_FreeSurface(bitmap);
		bitmap=NULL;
	}
}

void CSelWindow::selectionChange(CSelectableComponent * to)
{
	blitAt(to->getImg(),to->pos.x-pos.x,to->pos.y-pos.y,bitmap);
	for (int i=0;i<components.size();i++)
	{
		if(components[i]==to)
		{
			if (to->selected)
				continue;
			else
				to->select(true);
		}
		CSelectableComponent * pom = dynamic_cast<CSelectableComponent*>(components[i]);
		if (!pom)
			continue;
		pom->select(false);
		blitAt(pom->getImg(),pom->pos.x-pos.x,pom->pos.y-pos.y,bitmap);
	}
}
void CSelWindow::okClicked(tribool down)
{
	if(!down)
		close();
}
void CSelWindow::close()
{
	int ret = -1;
	for (int i=0;i<components.size();i++)
	{
		if(dynamic_cast<CSelectableComponent*>(components[i])->selected)
		{
			ret = i;
		}
		components[i]->deactivate();
	}	
	components.clear();
	okb.deactivate();
	SDL_FreeSurface(bitmap);
	bitmap = NULL;
	LOCPLINT->removeObjToBlit(this);
	LOCPLINT->curint->activate();
	LOCPLINT->cb->selectionMade(ret,ID);
	delete this;
	//call owner with selection result
}
template <typename T>CSCButton<T>::CSCButton(CDefHandler * img, CIntObject * obj, void(T::*poin)(tribool), T* Delg)
{
	ourObj = obj;
	delg = Delg;
	func = poin;
	imgs.resize(1);
	for (int i =0; i<img->ourImages.size();i++)
	{
		imgs[0].push_back(img->ourImages[i].bitmap);
	}
	pos.w = imgs[0][0]->w;
	pos.h = imgs[0][0]->h;
	state = 0;
}
template <typename T> void CSCButton<T>::clickLeft (tribool down)
{
	if (down)
	{
		state=1;
	}
	else 
	{
		state=0;
	}
	pressedL=state;
	show();
	if (delg)
		(delg->*func)(down);
}
template <typename T> void CSCButton<typename T>::activate()
{
	ClickableL::activate();
}
template <typename T> void CSCButton<typename T>::deactivate()
{
	ClickableL::deactivate();
}

template <typename T> void CSCButton<typename T>::show(SDL_Surface * to)
{
	if (delg) //we blit on our owner's bitmap
	{
		blitAt(imgs[curimg][state],posr.x,posr.y,delg->bitmap);
		//updateRect(&genRect(pos.h,pos.w,posr.x,posr.y),delg->bitmap);
	}
	else
	{
		CButtonBase::show(to);
	}
}
CButtonBase::CButtonBase()
{
	bitmapOffset = 0;
	curimg=0;
	type=-1;
	abs=false;
	active=false;
	notFreeButton = false;
	ourObj=NULL;
	state=0;
}
CButtonBase::~CButtonBase()
{
	if(notFreeButton)
		return;
	for(int i =0; i<imgs.size();i++)
		for(int j=0;j<imgs[i].size();j++)
			SDL_FreeSurface(imgs[i][j]);
}
void CButtonBase::show(SDL_Surface * to)
{
	if(!to)
		to=screen;
	if (abs)
	{
		blitAt(imgs[curimg]
			[( (state+bitmapOffset) < (imgs[curimg].size()) )	?	
				(state+bitmapOffset)	:
				(imgs[curimg].size()-1)			]
													,pos.x,pos.y,to);
		//updateRect(&pos,to);
	}
	else
	{
		blitAt(imgs[curimg]
			[( (state+bitmapOffset) < (imgs[curimg].size()) )	?	
				(state+bitmapOffset)	:
				(imgs[curimg].size()-1)			],pos.x+ourObj->pos.x,pos.y+ourObj->pos.y,to);
		//updateRect(&genRect(pos.h,pos.w,pos.x+ourObj->pos.x,pos.y+ourObj->pos.y),to);
		
	}
}
ClickableL::ClickableL()
{
	pressedL=false;
}
void ClickableL::clickLeft(tribool down)
{
	if (down)
		pressedL=true;
	else
		pressedL=false;
}
void ClickableL::activate()
{
	LOCPLINT->lclickable.push_back(this);
}
void ClickableL::deactivate()
{
	LOCPLINT->lclickable.erase
		(std::find(LOCPLINT->lclickable.begin(),LOCPLINT->lclickable.end(),this));
}
ClickableR::ClickableR()
{
	pressedR=false;
}
void ClickableR::clickRight(tribool down)
{
	if (down)
		pressedR=true;
	else
		pressedR=false;
}
void ClickableR::activate()
{
	LOCPLINT->rclickable.push_back(this);
}
void ClickableR::deactivate()
{
	LOCPLINT->rclickable.erase(std::find(LOCPLINT->rclickable.begin(),LOCPLINT->rclickable.end(),this));
}
void Hoverable::activate()
{
	LOCPLINT->hoverable.push_back(this);
}
void Hoverable::deactivate()
{
	LOCPLINT->hoverable.erase(std::find(LOCPLINT->hoverable.begin(),LOCPLINT->hoverable.end(),this));
}
void Hoverable::hover(bool on)
{
	hovered=on;
}
void KeyInterested::activate()
{
	LOCPLINT->keyinterested.push_back(this);
}
void KeyInterested::deactivate()
{
	LOCPLINT->
		keyinterested.erase(std::find(LOCPLINT->keyinterested.begin(),LOCPLINT->keyinterested.end(),this));
}
void MotionInterested::activate()
{
	LOCPLINT->motioninterested.push_back(this);
}
void MotionInterested::deactivate()
{
	LOCPLINT->
		motioninterested.erase(std::find(LOCPLINT->motioninterested.begin(),LOCPLINT->motioninterested.end(),this));
}
void TimeInterested::activate()
{
	LOCPLINT->timeinterested.push_back(this);
}
void TimeInterested::deactivate()
{
	LOCPLINT->
		timeinterested.erase(std::find(LOCPLINT->timeinterested.begin(),LOCPLINT->timeinterested.end(),this));
}
CPlayerInterface::CPlayerInterface(int Player, int serial)
{
	playerID=Player;
	serialID=serial;
	CGI->localPlayer = playerID;
	human=true;
	hInfo = CGI->bitmaph->loadBitmap("HEROQVBK.bmp");
	SDL_SetColorKey(hInfo,SDL_SRCCOLORKEY,SDL_MapRGB(hInfo->format,0,255,255));
	tInfo = CGI->bitmaph->loadBitmap("TOWNQVBK.bmp");
	SDL_SetColorKey(tInfo,SDL_SRCCOLORKEY,SDL_MapRGB(tInfo->format,0,255,255));
	slotsPos.push_back(std::pair<int,int>(44,82));
	slotsPos.push_back(std::pair<int,int>(80,82));
	slotsPos.push_back(std::pair<int,int>(116,82));
	slotsPos.push_back(std::pair<int,int>(26,131));
	slotsPos.push_back(std::pair<int,int>(62,131));
	slotsPos.push_back(std::pair<int,int>(98,131));
	slotsPos.push_back(std::pair<int,int>(134,131));

	luck22 = CGI->spriteh->giveDefEss("ILCK22.DEF");
	luck30 = CGI->spriteh->giveDefEss("ILCK30.DEF");
	luck42 = CGI->spriteh->giveDefEss("ILCK42.DEF");
	luck82 = CGI->spriteh->giveDefEss("ILCK82.DEF");
	morale22 = CGI->spriteh->giveDefEss("IMRL22.DEF");
	morale30 = CGI->spriteh->giveDefEss("IMRL30.DEF");
	morale42 = CGI->spriteh->giveDefEss("IMRL42.DEF");
	morale82 = CGI->spriteh->giveDefEss("IMRL82.DEF");
	halls = CGI->spriteh->giveDefEss("ITMTLS.DEF");
	forts = CGI->spriteh->giveDefEss("ITMCLS.DEF");
	bigTownPic =  CGI->spriteh->giveDefEss("ITPT.DEF");

}
void CPlayerInterface::init(ICallback * CB)
{
	cb = dynamic_cast<CCallback*>(CB);
	CGI->localPlayer = serialID;
	adventureInt = new CAdvMapInt(playerID);
	castleInt = NULL;
	std::vector <const CGHeroInstance *> hh = cb->getHeroesInfo(false);
	for(int i=0;i<hh.size();i++)
	{
		SDL_Surface * pom = infoWin(hh[i]);
		heroWins.insert(std::pair<int,SDL_Surface*>(hh[i]->subID,pom));
	}
	std::vector<const CGTownInstance*> tt = cb->getTownsInfo(false);
	for(int i=0;i<tt.size();i++)
	{
		SDL_Surface * pom = infoWin(tt[i]);
		townWins.insert(std::pair<int,SDL_Surface*>(tt[i]->identifier,pom));
	}
}
void CPlayerInterface::yourTurn()
{
	makingTurn = true;
	CGI->localPlayer = serialID;
	unsigned char & animVal = LOCPLINT->adventureInt->anim; //for animations handling
	unsigned char & heroAnimVal = LOCPLINT->adventureInt->heroAnim;
	adventureInt->infoBar.newDay(cb->getDate(1));
	adventureInt->activate();
	//show rest of things

	//initializing framerate keeper
	mainFPSmng = new FPSmanager;
	SDL_initFramerate(mainFPSmng);
	SDL_setFramerate(mainFPSmng, 24);
	SDL_Event sEvent;
	//framerate keeper initialized
	timeHandler th;
	th.getDif();
	for(;makingTurn;) // main loop
	{
		//updating water tiles
		//int wnumber = -1;
		//for(int s=0; s<CGI->mh->reader->defs.size(); ++s)
		//{
		//	if(CGI->mh->reader->defs[s]->defName==std::string("WATRTL.DEF"))
		//	{
		//		wnumber = s;
		//		break;
		//	}
		//}
		//if(wnumber>=0)
		//{
		//	for(int g=0; g<CGI->mh->reader->defs[wnumber]->ourImages.size(); ++g)
		//	{
		//		SDL_Color tab[32];
		//		for(int i=0; i<32; ++i)
		//		{
		//			tab[i] = CGI->mh->reader->defs[wnumber]->ourImages[g].bitmap->format->palette->colors[160 + (i+1)%32];
		//		}
		//		//SDL_SaveBMP(CGI->mh->reader->defs[wnumber]->ourImages[g].bitmap,"t1.bmp");
		//		for(int i=0; i<32; ++i)
		//		{
		//			CGI->mh->reader->defs[wnumber]->ourImages[g].bitmap->format->palette->colors[160 + i] = tab[i];
		//		}
		//		//SDL_SaveBMP(CGI->mh->reader->defs[wnumber]->ourImages[g].bitmap,"t2.bmp");
		//		CSDL_Ext::update(CGI->mh->reader->defs[wnumber]->ourImages[g].bitmap); 
		//	}
		//}
		//water tiles updated
		CGI->screenh->updateScreen();
		int tv = th.getDif();
		for (int i=0;i<timeinterested.size();i++)
		{
			if (timeinterested[i]->toNextTick>=0)
				timeinterested[i]->toNextTick-=tv;
			if (timeinterested[i]->toNextTick<0)
				timeinterested[i]->tick();
		}
		LOCPLINT->adventureInt->updateScreen = false;
		while (SDL_PollEvent(&sEvent))  //wait for event...
		{
			handleEvent(&sEvent);
		}
		if (!castleInt) //stuff for advMapInt
		{
			++LOCPLINT->adventureInt->animValHitCount; //for animations
			if(LOCPLINT->adventureInt->animValHitCount == 4)
			{
				LOCPLINT->adventureInt->animValHitCount = 0;
				++animVal;
				LOCPLINT->adventureInt->updateScreen = true;

			}
			++heroAnimVal;
			if(LOCPLINT->adventureInt->scrollingLeft)
			{
				if(LOCPLINT->adventureInt->position.x>-Woff)
				{
					LOCPLINT->adventureInt->position.x--;
					LOCPLINT->adventureInt->updateScreen = true;
					adventureInt->updateMinimap=true;
				}
			}
			if(LOCPLINT->adventureInt->scrollingRight)
			{
				if(LOCPLINT->adventureInt->position.x<CGI->ac->map.width-19+4)
				{
					LOCPLINT->adventureInt->position.x++;
					LOCPLINT->adventureInt->updateScreen = true;
					adventureInt->updateMinimap=true;
				}
			}
			if(LOCPLINT->adventureInt->scrollingUp)
			{
				if(LOCPLINT->adventureInt->position.y>-Hoff)
				{
					LOCPLINT->adventureInt->position.y--;
					LOCPLINT->adventureInt->updateScreen = true;
					adventureInt->updateMinimap=true;
				}
			}
			if(LOCPLINT->adventureInt->scrollingDown)
			{
				if(LOCPLINT->adventureInt->position.y<CGI->ac->map.height-18+4)
				{
					LOCPLINT->adventureInt->position.y++;
					LOCPLINT->adventureInt->updateScreen = true;
					adventureInt->updateMinimap=true;
				}
			}
			if(LOCPLINT->adventureInt->updateScreen)
			{
				adventureInt->update();
				adventureInt->updateScreen=false;
			}
			if (LOCPLINT->adventureInt->updateMinimap)
			{
				adventureInt->minimap.draw();
				adventureInt->updateMinimap=false;
			}
		}
		for(int i=0;i<objsToBlit.size();i++)
			objsToBlit[i]->show();
		//SDL_Flip(screen);
		CSDL_Ext::update(screen);
		SDL_Delay(5); //give time for other apps
		SDL_framerateDelay(mainFPSmng);
	}
	adventureInt->hide();
}

inline void subRect(const int & x, const int & y, const int & z, SDL_Rect & r, const int & hid)
{
	TerrainTile2 & hlp = CGI->mh->ttiles[x][y][z];
	for(int h=0; h<hlp.objects.size(); ++h)
		if(hlp.objects[h].first->id==hid)
		{
			hlp.objects[h].second = r;
			return;
		}
}

inline void delObjRect(const int & x, const int & y, const int & z, const int & hid)
{
	TerrainTile2 & hlp = CGI->mh->ttiles[x][y][z];
	for(int h=0; h<hlp.objects.size(); ++h)
		if(hlp.objects[h].first->id==hid)
		{
			hlp.objects.erase(hlp.objects.begin()+h);
			return;
		}
}
int getDir(int3 src, int3 dst)
{	
	int ret = -1;
	if(dst.x+1 == src.x && dst.y+1 == src.y) //tl
	{
		ret = 1;
	}
	else if(dst.x == src.x && dst.y+1 == src.y) //t
	{
		ret = 2;
	}
	else if(dst.x-1 == src.x && dst.y+1 == src.y) //tr
	{
		ret = 3;
	}
	else if(dst.x-1 == src.x && dst.y == src.y) //r
	{
		ret = 4;
	}
	else if(dst.x-1 == src.x && dst.y-1 == src.y) //br
	{
		ret = 5;
	}
	else if(dst.x == src.x && dst.y-1 == src.y) //b
	{
		ret = 6;
	}
	else if(dst.x+1 == src.x && dst.y-1 == src.y) //bl
	{
		ret = 7;
	}
	else if(dst.x+1 == src.x && dst.y == src.y) //l
	{
		ret = 8;
	}
	return ret;
}
void CPlayerInterface::heroMoved(const HeroMoveDetails & details)
{
	//initializing objects and performing first step of move
	CGHeroInstance * ho = details.ho; //object representing this hero
	int3 hp = details.src;
	if (!details.successful)
	{
		ho->moveDir = getDir(details.src,details.dst);
		ho->isStanding = true;
		adventureInt->heroList.draw();
		if (adventureInt->terrain.currentPath)
		{
			delete adventureInt->terrain.currentPath;
			adventureInt->terrain.currentPath = NULL;
		}
		return;
	}

	if (adventureInt->terrain.currentPath) //&& hero is moving
	{
		adventureInt->terrain.currentPath->nodes.erase(adventureInt->terrain.currentPath->nodes.end()-1);
	}

	int3 buff = details.ho->pos;
	buff.x-=11;
	buff.y-=9;
	buff = repairScreenPos(buff);
	LOCPLINT->adventureInt->position = buff; //actualizing screen pos

	if(details.dst.x+1 == details.src.x && details.dst.y+1 == details.src.y) //tl
	{
		ho->moveDir = 1;
		ho->isStanding = false;
		CGI->mh->ttiles[hp.x-3][hp.y-2][hp.z].objects.push_back(std::make_pair(ho, genRect(32, 32, -31, -31)));
		CGI->mh->ttiles[hp.x-2][hp.y-2][hp.z].objects.push_back(std::make_pair(ho, genRect(32, 32, 1, -31)));
		CGI->mh->ttiles[hp.x-1][hp.y-2][hp.z].objects.push_back(std::make_pair(ho, genRect(32, 32, 33, -31)));
		CGI->mh->ttiles[hp.x][hp.y-2][hp.z].objects.push_back(std::make_pair(ho, genRect(32, 32, 65, -31)));

		CGI->mh->ttiles[hp.x-3][hp.y-1][hp.z].objects.push_back(std::make_pair(ho, genRect(32, 32, -31, 1)));
		subRect(hp.x-2, hp.y-1, hp.z, genRect(32, 32, 1, 1), ho->id);
		subRect(hp.x-1, hp.y-1, hp.z, genRect(32, 32, 33, 1), ho->id);
		subRect(hp.x, hp.y-1, hp.z, genRect(32, 32, 65, 1), ho->id);

		CGI->mh->ttiles[hp.x-3][hp.y][hp.z].objects.push_back(std::make_pair(ho, genRect(32, 32, -31, 33)));
		subRect(hp.x-2, hp.y, hp.z, genRect(32, 32, 1, 33), ho->id);
		subRect(hp.x-1, hp.y, hp.z, genRect(32, 32, 33, 33), ho->id);
		subRect(hp.x, hp.y, hp.z, genRect(32, 32, 65, 33), ho->id);

		std::stable_sort(CGI->mh->ttiles[hp.x-3][hp.y-2][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-3][hp.y-2][hp.z].objects.end(), ocmptwo_cgin);
		std::stable_sort(CGI->mh->ttiles[hp.x-2][hp.y-2][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-2][hp.y-2][hp.z].objects.end(), ocmptwo_cgin);
		std::stable_sort(CGI->mh->ttiles[hp.x-1][hp.y-2][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-1][hp.y-2][hp.z].objects.end(), ocmptwo_cgin);
		std::stable_sort(CGI->mh->ttiles[hp.x][hp.y-2][hp.z].objects.begin(), CGI->mh->ttiles[hp.x][hp.y-2][hp.z].objects.end(), ocmptwo_cgin);

		std::stable_sort(CGI->mh->ttiles[hp.x-3][hp.y-1][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-3][hp.y-1][hp.z].objects.end(), ocmptwo_cgin);

		std::stable_sort(CGI->mh->ttiles[hp.x-3][hp.y][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-3][hp.y][hp.z].objects.end(), ocmptwo_cgin);
	}
	else if(details.dst.x == details.src.x && details.dst.y+1 == details.src.y) //t
	{
		ho->moveDir = 2;
		ho->isStanding = false;
		CGI->mh->ttiles[hp.x-2][hp.y-2][hp.z].objects.push_back(std::make_pair(ho, genRect(32, 32, 0, -31)));
		CGI->mh->ttiles[hp.x-1][hp.y-2][hp.z].objects.push_back(std::make_pair(ho, genRect(32, 32, 32, -31)));
		CGI->mh->ttiles[hp.x][hp.y-2][hp.z].objects.push_back(std::make_pair(ho, genRect(32, 32, 64, -31)));

		subRect(hp.x-2, hp.y-1, hp.z, genRect(32, 32, 0, 1), ho->id);
		subRect(hp.x-1, hp.y-1, hp.z, genRect(32, 32, 32, 1), ho->id);
		subRect(hp.x, hp.y-1, hp.z, genRect(32, 32, 64, 1), ho->id);

		subRect(hp.x-2, hp.y, hp.z, genRect(32, 32, 0, 33), ho->id);
		subRect(hp.x-1, hp.y, hp.z, genRect(32, 32, 32, 33), ho->id);
		subRect(hp.x, hp.y, hp.z, genRect(32, 32, 64, 33), ho->id);

		std::stable_sort(CGI->mh->ttiles[hp.x-2][hp.y-2][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-2][hp.y-2][hp.z].objects.end(), ocmptwo_cgin);
		std::stable_sort(CGI->mh->ttiles[hp.x-1][hp.y-2][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-1][hp.y-2][hp.z].objects.end(), ocmptwo_cgin);
		std::stable_sort(CGI->mh->ttiles[hp.x][hp.y-2][hp.z].objects.begin(), CGI->mh->ttiles[hp.x][hp.y-2][hp.z].objects.end(), ocmptwo_cgin);
	}
	else if(details.dst.x-1 == details.src.x && details.dst.y+1 == details.src.y) //tr
	{
		ho->moveDir = 3;
		ho->isStanding = false;
		CGI->mh->ttiles[hp.x-2][hp.y-2][hp.z].objects.push_back(std::make_pair(ho, genRect(32, 32, -1, -31)));
		CGI->mh->ttiles[hp.x-1][hp.y-2][hp.z].objects.push_back(std::make_pair(ho, genRect(32, 32, 31, -31)));
		CGI->mh->ttiles[hp.x][hp.y-2][hp.z].objects.push_back(std::make_pair(ho, genRect(32, 32, 63, -31)));
		CGI->mh->ttiles[hp.x+1][hp.y-2][hp.z].objects.push_back(std::make_pair(ho, genRect(32, 32, 95, -31)));

		subRect(hp.x-2, hp.y-1, hp.z, genRect(32, 32, -1, 1), ho->id);
		subRect(hp.x-1, hp.y-1, hp.z, genRect(32, 32, 31, 1), ho->id);
		subRect(hp.x, hp.y-1, hp.z, genRect(32, 32, 63, 1), ho->id);
		CGI->mh->ttiles[hp.x+1][hp.y-1][hp.z].objects.push_back(std::make_pair(ho, genRect(32, 32, 95, 1)));

		subRect(hp.x-2, hp.y, hp.z, genRect(32, 32, -1, 33), ho->id);
		subRect(hp.x-1, hp.y, hp.z, genRect(32, 32, 31, 33), ho->id);
		subRect(hp.x, hp.y, hp.z, genRect(32, 32, 63, 33), ho->id);
		CGI->mh->ttiles[hp.x+1][hp.y][hp.z].objects.push_back(std::make_pair(ho, genRect(32, 32, 95, 33)));

		std::stable_sort(CGI->mh->ttiles[hp.x-2][hp.y-2][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-2][hp.y-2][hp.z].objects.end(), ocmptwo_cgin);
		std::stable_sort(CGI->mh->ttiles[hp.x-1][hp.y-2][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-1][hp.y-2][hp.z].objects.end(), ocmptwo_cgin);
		std::stable_sort(CGI->mh->ttiles[hp.x][hp.y-2][hp.z].objects.begin(), CGI->mh->ttiles[hp.x][hp.y-2][hp.z].objects.end(), ocmptwo_cgin);
		std::stable_sort(CGI->mh->ttiles[hp.x+1][hp.y-2][hp.z].objects.begin(), CGI->mh->ttiles[hp.x+1][hp.y-2][hp.z].objects.end(), ocmptwo_cgin);

		std::stable_sort(CGI->mh->ttiles[hp.x+1][hp.y-1][hp.z].objects.begin(), CGI->mh->ttiles[hp.x+1][hp.y-1][hp.z].objects.end(), ocmptwo_cgin);

		std::stable_sort(CGI->mh->ttiles[hp.x+1][hp.y][hp.z].objects.begin(), CGI->mh->ttiles[hp.x+1][hp.y][hp.z].objects.end(), ocmptwo_cgin);
	}
	else if(details.dst.x-1 == details.src.x && details.dst.y == details.src.y) //r
	{
		ho->moveDir = 4;
		ho->isStanding = false;
		subRect(hp.x-2, hp.y-1, hp.z, genRect(32, 32, -1, 0), ho->id);
		subRect(hp.x-1, hp.y-1, hp.z, genRect(32, 32, 31, 0), ho->id);
		subRect(hp.x, hp.y-1, hp.z, genRect(32, 32, 63, 0), ho->id);
		CGI->mh->ttiles[hp.x+1][hp.y-1][hp.z].objects.push_back(std::make_pair(ho, genRect(32, 32, 95, 0)));

		subRect(hp.x-2, hp.y, hp.z, genRect(32, 32, -1, 32), ho->id);
		subRect(hp.x-1, hp.y, hp.z, genRect(32, 32, 31, 32), ho->id);
		subRect(hp.x, hp.y, hp.z, genRect(32, 32, 63, 32), ho->id);
		CGI->mh->ttiles[hp.x+1][hp.y][hp.z].objects.push_back(std::make_pair(ho, genRect(32, 32, 95, 32)));

		std::stable_sort(CGI->mh->ttiles[hp.x+1][hp.y-1][hp.z].objects.begin(), CGI->mh->ttiles[hp.x+1][hp.y-1][hp.z].objects.end(), ocmptwo_cgin);

		std::stable_sort(CGI->mh->ttiles[hp.x+1][hp.y][hp.z].objects.begin(), CGI->mh->ttiles[hp.x+1][hp.y][hp.z].objects.end(), ocmptwo_cgin);
	}
	else if(details.dst.x-1 == details.src.x && details.dst.y-1 == details.src.y) //br
	{
		ho->moveDir = 5;
		ho->isStanding = false;
		subRect(hp.x-2, hp.y-1, hp.z, genRect(32, 32, -1, -1), ho->id);
		subRect(hp.x-1, hp.y-1, hp.z, genRect(32, 32, 31, -1), ho->id);
		subRect(hp.x, hp.y-1, hp.z, genRect(32, 32, 63, -1), ho->id);
		CGI->mh->ttiles[hp.x+1][hp.y-1][hp.z].objects.push_back(std::make_pair(ho, genRect(32, 32, 95, -1)));

		subRect(hp.x-2, hp.y, hp.z, genRect(32, 32, -1, 31), ho->id);
		subRect(hp.x-1, hp.y, hp.z, genRect(32, 32, 31, 31), ho->id);
		subRect(hp.x, hp.y, hp.z, genRect(32, 32, 63, 31), ho->id);
		CGI->mh->ttiles[hp.x+1][hp.y][hp.z].objects.push_back(std::make_pair(ho, genRect(32, 32, 95, 31)));

		CGI->mh->ttiles[hp.x-2][hp.y+1][hp.z].objects.push_back(std::make_pair(ho, genRect(32, 32, -1, 63)));
		CGI->mh->ttiles[hp.x-1][hp.y+1][hp.z].objects.push_back(std::make_pair(ho, genRect(32, 32, 31, 63)));
		CGI->mh->ttiles[hp.x][hp.y+1][hp.z].objects.push_back(std::make_pair(ho, genRect(32, 32, 63, 63)));
		CGI->mh->ttiles[hp.x+1][hp.y+1][hp.z].objects.push_back(std::make_pair(ho, genRect(32, 32, 95, 63)));

		std::stable_sort(CGI->mh->ttiles[hp.x+1][hp.y-1][hp.z].objects.begin(), CGI->mh->ttiles[hp.x+1][hp.y-1][hp.z].objects.end(), ocmptwo_cgin);

		std::stable_sort(CGI->mh->ttiles[hp.x+1][hp.y][hp.z].objects.begin(), CGI->mh->ttiles[hp.x+1][hp.y][hp.z].objects.end(), ocmptwo_cgin);

		std::stable_sort(CGI->mh->ttiles[hp.x-2][hp.y+1][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-2][hp.y+1][hp.z].objects.end(), ocmptwo_cgin);
		std::stable_sort(CGI->mh->ttiles[hp.x-1][hp.y+1][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-1][hp.y+1][hp.z].objects.end(), ocmptwo_cgin);
		std::stable_sort(CGI->mh->ttiles[hp.x][hp.y+1][hp.z].objects.begin(), CGI->mh->ttiles[hp.x][hp.y+1][hp.z].objects.end(), ocmptwo_cgin);
		std::stable_sort(CGI->mh->ttiles[hp.x+1][hp.y+1][hp.z].objects.begin(), CGI->mh->ttiles[hp.x+1][hp.y+1][hp.z].objects.end(), ocmptwo_cgin);
	}
	else if(details.dst.x == details.src.x && details.dst.y-1 == details.src.y) //b
	{
		ho->moveDir = 6;
		ho->isStanding = false;
		subRect(hp.x-2, hp.y-1, hp.z, genRect(32, 32, 0, -1), ho->id);
		subRect(hp.x-1, hp.y-1, hp.z, genRect(32, 32, 32, -1), ho->id);
		subRect(hp.x, hp.y-1, hp.z, genRect(32, 32, 64, -1), ho->id);

		subRect(hp.x-2, hp.y, hp.z, genRect(32, 32, 0, 31), ho->id);
		subRect(hp.x-1, hp.y, hp.z, genRect(32, 32, 32, 31), ho->id);
		subRect(hp.x, hp.y, hp.z, genRect(32, 32, 64, 31), ho->id);

		CGI->mh->ttiles[hp.x-2][hp.y+1][hp.z].objects.push_back(std::make_pair(ho, genRect(32, 32, 0, 63)));
		CGI->mh->ttiles[hp.x-1][hp.y+1][hp.z].objects.push_back(std::make_pair(ho, genRect(32, 32, 32, 63)));
		CGI->mh->ttiles[hp.x][hp.y+1][hp.z].objects.push_back(std::make_pair(ho, genRect(32, 32, 64, 63)));

		std::stable_sort(CGI->mh->ttiles[hp.x-2][hp.y+1][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-2][hp.y+1][hp.z].objects.end(), ocmptwo_cgin);
		std::stable_sort(CGI->mh->ttiles[hp.x-1][hp.y+1][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-1][hp.y+1][hp.z].objects.end(), ocmptwo_cgin);
		std::stable_sort(CGI->mh->ttiles[hp.x][hp.y+1][hp.z].objects.begin(), CGI->mh->ttiles[hp.x][hp.y+1][hp.z].objects.end(), ocmptwo_cgin);
	}
	else if(details.dst.x+1 == details.src.x && details.dst.y-1 == details.src.y) //bl
	{
		ho->moveDir = 7;
		ho->isStanding = false;
		CGI->mh->ttiles[hp.x-3][hp.y-1][hp.z].objects.push_back(std::make_pair(ho, genRect(32, 32, -31, -1)));
		subRect(hp.x-2, hp.y-1, hp.z, genRect(32, 32, 1, -1), ho->id);
		subRect(hp.x-1, hp.y-1, hp.z, genRect(32, 32, 33, -1), ho->id);
		subRect(hp.x, hp.y-1, hp.z, genRect(32, 32, 65, -1), ho->id);

		CGI->mh->ttiles[hp.x-3][hp.y][hp.z].objects.push_back(std::make_pair(ho, genRect(32, 32, -31, 31)));
		subRect(hp.x-2, hp.y, hp.z, genRect(32, 32, 1, 31), ho->id);
		subRect(hp.x-1, hp.y, hp.z, genRect(32, 32, 33, 31), ho->id);
		subRect(hp.x, hp.y, hp.z, genRect(32, 32, 65, 31), ho->id);

		CGI->mh->ttiles[hp.x-3][hp.y+1][hp.z].objects.push_back(std::make_pair(ho, genRect(32, 32, -31, 63)));
		CGI->mh->ttiles[hp.x-2][hp.y+1][hp.z].objects.push_back(std::make_pair(ho, genRect(32, 32, 1, 63)));
		CGI->mh->ttiles[hp.x-1][hp.y+1][hp.z].objects.push_back(std::make_pair(ho, genRect(32, 32, 33, 63)));
		CGI->mh->ttiles[hp.x][hp.y+1][hp.z].objects.push_back(std::make_pair(ho, genRect(32, 32, 65, 63)));

		std::stable_sort(CGI->mh->ttiles[hp.x-3][hp.y-1][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-3][hp.y-1][hp.z].objects.end(), ocmptwo_cgin);

		std::stable_sort(CGI->mh->ttiles[hp.x-3][hp.y][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-3][hp.y][hp.z].objects.end(), ocmptwo_cgin);

		std::stable_sort(CGI->mh->ttiles[hp.x-3][hp.y+1][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-3][hp.y+1][hp.z].objects.end(), ocmptwo_cgin);
		std::stable_sort(CGI->mh->ttiles[hp.x-2][hp.y+1][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-2][hp.y+1][hp.z].objects.end(), ocmptwo_cgin);
		std::stable_sort(CGI->mh->ttiles[hp.x-1][hp.y+1][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-1][hp.y+1][hp.z].objects.end(), ocmptwo_cgin);
		std::stable_sort(CGI->mh->ttiles[hp.x][hp.y+1][hp.z].objects.begin(), CGI->mh->ttiles[hp.x][hp.y+1][hp.z].objects.end(), ocmptwo_cgin);
	}
	else if(details.dst.x+1 == details.src.x && details.dst.y == details.src.y) //l
	{
		ho->moveDir = 8;
		ho->isStanding = false;
		CGI->mh->ttiles[hp.x-3][hp.y-1][hp.z].objects.push_back(std::make_pair(ho, genRect(32, 32, -31, 0)));
		subRect(hp.x-2, hp.y-1, hp.z, genRect(32, 32, 1, 0), ho->id);
		subRect(hp.x-1, hp.y-1, hp.z, genRect(32, 32, 33, 0), ho->id);
		subRect(hp.x, hp.y-1, hp.z, genRect(32, 32, 65, 0), ho->id);

		CGI->mh->ttiles[hp.x-3][hp.y][hp.z].objects.push_back(std::make_pair(ho, genRect(32, 32, -31, 32)));
		subRect(hp.x-2, hp.y, hp.z, genRect(32, 32, 1, 32), ho->id);
		subRect(hp.x-1, hp.y, hp.z, genRect(32, 32, 33, 32), ho->id);
		subRect(hp.x, hp.y, hp.z, genRect(32, 32, 65, 32), ho->id);

		std::stable_sort(CGI->mh->ttiles[hp.x-3][hp.y-1][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-3][hp.y-1][hp.z].objects.end(), ocmptwo_cgin);

		std::stable_sort(CGI->mh->ttiles[hp.x-3][hp.y][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-3][hp.y][hp.z].objects.end(), ocmptwo_cgin);
	}
	//first initializing done
	SDL_framerateDelay(mainFPSmng); // after first move
	//main moving
	for(int i=1; i<32; i+=4)
	{
		if(details.dst.x+1 == details.src.x && details.dst.y+1 == details.src.y) //tl
		{
			subRect(hp.x-3, hp.y-2, hp.z, genRect(32, 32, -31+i, -31+i), ho->id);
			subRect(hp.x-2, hp.y-2, hp.z, genRect(32, 32, 1+i, -31+i), ho->id);
			subRect(hp.x-1, hp.y-2, hp.z, genRect(32, 32, 33+i, -31+i), ho->id);
			subRect(hp.x, hp.y-2, hp.z, genRect(32, 32, 65+i, -31+i), ho->id);

			subRect(hp.x-3, hp.y-1, hp.z, genRect(32, 32, -31+i, 1+i), ho->id);
			subRect(hp.x-2, hp.y-1, hp.z, genRect(32, 32, 1+i, 1+i), ho->id);
			subRect(hp.x-1, hp.y-1, hp.z, genRect(32, 32, 33+i, 1+i), ho->id);
			subRect(hp.x, hp.y-1, hp.z, genRect(32, 32, 65+i, 1+i), ho->id);

			subRect(hp.x-3, hp.y, hp.z, genRect(32, 32, -31+i, 33+i), ho->id);
			subRect(hp.x-2, hp.y, hp.z, genRect(32, 32, 1+i, 33+i), ho->id);
			subRect(hp.x-1, hp.y, hp.z, genRect(32, 32, 33+i, 33+i), ho->id);
			subRect(hp.x, hp.y, hp.z, genRect(32, 32, 65+i, 33+i), ho->id);
		}
		else if(details.dst.x == details.src.x && details.dst.y+1 == details.src.y) //t
		{
			subRect(hp.x-2, hp.y-2, hp.z, genRect(32, 32, 0, -31+i), ho->id);
			subRect(hp.x-1, hp.y-2, hp.z, genRect(32, 32, 32, -31+i), ho->id);
			subRect(hp.x, hp.y-2, hp.z, genRect(32, 32, 64, -31+i), ho->id);

			subRect(hp.x-2, hp.y-1, hp.z, genRect(32, 32, 0, 1+i), ho->id);
			subRect(hp.x-1, hp.y-1, hp.z, genRect(32, 32, 32, 1+i), ho->id);
			subRect(hp.x, hp.y-1, hp.z, genRect(32, 32, 64, 1+i), ho->id);

			subRect(hp.x-2, hp.y, hp.z, genRect(32, 32, 0, 33+i), ho->id);
			subRect(hp.x-1, hp.y, hp.z, genRect(32, 32, 32, 33+i), ho->id);
			subRect(hp.x, hp.y, hp.z, genRect(32, 32, 64, 33+i), ho->id);
		}
		else if(details.dst.x-1 == details.src.x && details.dst.y+1 == details.src.y) //tr
		{
			subRect(hp.x-2, hp.y-2, hp.z, genRect(32, 32, -1-i, -31+i), ho->id);
			subRect(hp.x-1, hp.y-2, hp.z, genRect(32, 32, 31-i, -31+i), ho->id);
			subRect(hp.x, hp.y-2, hp.z, genRect(32, 32, 63-i, -31+i), ho->id);
			subRect(hp.x+1, hp.y-2, hp.z, genRect(32, 32, 95-i, -31+i), ho->id);

			subRect(hp.x-2, hp.y-1, hp.z, genRect(32, 32, -1-i, 1+i), ho->id);
			subRect(hp.x-1, hp.y-1, hp.z, genRect(32, 32, 31-i, 1+i), ho->id);
			subRect(hp.x, hp.y-1, hp.z, genRect(32, 32, 63-i, 1+i), ho->id);
			subRect(hp.x+1, hp.y-1, hp.z, genRect(32, 32, 95-i, 1+i), ho->id);

			subRect(hp.x-2, hp.y, hp.z, genRect(32, 32, -1-i, 33+i), ho->id);
			subRect(hp.x-1, hp.y, hp.z, genRect(32, 32, 31-i, 33+i), ho->id);
			subRect(hp.x, hp.y, hp.z, genRect(32, 32, 63-i, 33+i), ho->id);
			subRect(hp.x+1, hp.y, hp.z, genRect(32, 32, 95-i, 33+i), ho->id);
		}
		else if(details.dst.x-1 == details.src.x && details.dst.y == details.src.y) //r
		{
			subRect(hp.x-2, hp.y-1, hp.z, genRect(32, 32, -1-i, 0), ho->id);
			subRect(hp.x-1, hp.y-1, hp.z, genRect(32, 32, 31-i, 0), ho->id);
			subRect(hp.x, hp.y-1, hp.z, genRect(32, 32, 63-i, 0), ho->id);
			subRect(hp.x+1, hp.y-1, hp.z, genRect(32, 32, 95-i, 0), ho->id);

			subRect(hp.x-2, hp.y, hp.z, genRect(32, 32, -1-i, 32), ho->id);
			subRect(hp.x-1, hp.y, hp.z, genRect(32, 32, 31-i, 32), ho->id);
			subRect(hp.x, hp.y, hp.z, genRect(32, 32, 63-i, 32), ho->id);
			subRect(hp.x+1, hp.y, hp.z, genRect(32, 32, 95-i, 32), ho->id);
		}
		else if(details.dst.x-1 == details.src.x && details.dst.y-1 == details.src.y) //br
		{
			subRect(hp.x-2, hp.y-1, hp.z, genRect(32, 32, -1-i, -1-i), ho->id);
			subRect(hp.x-1, hp.y-1, hp.z, genRect(32, 32, 31-i, -1-i), ho->id);
			subRect(hp.x, hp.y-1, hp.z, genRect(32, 32, 63-i, -1-i), ho->id);
			subRect(hp.x+1, hp.y-1, hp.z, genRect(32, 32, 95-i, -1-i), ho->id);

			subRect(hp.x-2, hp.y, hp.z, genRect(32, 32, -1-i, 31-i), ho->id);
			subRect(hp.x-1, hp.y, hp.z, genRect(32, 32, 31-i, 31-i), ho->id);
			subRect(hp.x, hp.y, hp.z, genRect(32, 32, 63-i, 31-i), ho->id);
			subRect(hp.x+1, hp.y, hp.z, genRect(32, 32, 95-i, 31-i), ho->id);

			subRect(hp.x-2, hp.y+1, hp.z, genRect(32, 32, -1-i, 63-i), ho->id);
			subRect(hp.x-1, hp.y+1, hp.z, genRect(32, 32, 31-i, 63-i), ho->id);
			subRect(hp.x, hp.y+1, hp.z, genRect(32, 32, 63-i, 63-i), ho->id);
			subRect(hp.x+1, hp.y+1, hp.z, genRect(32, 32, 95-i, 63-i), ho->id);
		}
		else if(details.dst.x == details.src.x && details.dst.y-1 == details.src.y) //b
		{
			subRect(hp.x-2, hp.y-1, hp.z, genRect(32, 32, 0, -1-i), ho->id);
			subRect(hp.x-1, hp.y-1, hp.z, genRect(32, 32, 32, -1-i), ho->id);
			subRect(hp.x, hp.y-1, hp.z, genRect(32, 32, 64, -1-i), ho->id);

			subRect(hp.x-2, hp.y, hp.z, genRect(32, 32, 0, 31-i), ho->id);
			subRect(hp.x-1, hp.y, hp.z, genRect(32, 32, 32, 31-i), ho->id);
			subRect(hp.x, hp.y, hp.z, genRect(32, 32, 64, 31-i), ho->id);

			subRect(hp.x-2, hp.y+1, hp.z, genRect(32, 32, 0, 63-i), ho->id);
			subRect(hp.x-1, hp.y+1, hp.z, genRect(32, 32, 32, 63-i), ho->id);
			subRect(hp.x, hp.y+1, hp.z, genRect(32, 32, 64, 63-i), ho->id);
		}
		else if(details.dst.x+1 == details.src.x && details.dst.y-1 == details.src.y) //bl
		{
			subRect(hp.x-3, hp.y-1, hp.z, genRect(32, 32, -31+i, -1-i), ho->id);
			subRect(hp.x-2, hp.y-1, hp.z, genRect(32, 32, 1+i, -1-i), ho->id);
			subRect(hp.x-1, hp.y-1, hp.z, genRect(32, 32, 33+i, -1-i), ho->id);
			subRect(hp.x, hp.y-1, hp.z, genRect(32, 32, 65+i, -1-i), ho->id);

			subRect(hp.x-3, hp.y, hp.z, genRect(32, 32, -31+i, 31-i), ho->id);
			subRect(hp.x-2, hp.y, hp.z, genRect(32, 32, 1+i, 31-i), ho->id);
			subRect(hp.x-1, hp.y, hp.z, genRect(32, 32, 33+i, 31-i), ho->id);
			subRect(hp.x, hp.y, hp.z, genRect(32, 32, 65+i, 31-i), ho->id);

			subRect(hp.x-3, hp.y+1, hp.z, genRect(32, 32, -31+i, 63-i), ho->id);
			subRect(hp.x-2, hp.y+1, hp.z, genRect(32, 32, 1+i, 63-i), ho->id);
			subRect(hp.x-1, hp.y+1, hp.z, genRect(32, 32, 33+i, 63-i), ho->id);
			subRect(hp.x, hp.y+1, hp.z, genRect(32, 32, 65+i, 63-i), ho->id);
		}
		else if(details.dst.x+1 == details.src.x && details.dst.y == details.src.y) //l
		{
			subRect(hp.x-3, hp.y-1, hp.z, genRect(32, 32, -31+i, 0), ho->id);
			subRect(hp.x-2, hp.y-1, hp.z, genRect(32, 32, 1+i, 0), ho->id);
			subRect(hp.x-1, hp.y-1, hp.z, genRect(32, 32, 33+i, 0), ho->id);
			subRect(hp.x, hp.y-1, hp.z, genRect(32, 32, 65+i, 0), ho->id);

			subRect(hp.x-3, hp.y, hp.z, genRect(32, 32, -31+i, 32), ho->id);
			subRect(hp.x-2, hp.y, hp.z, genRect(32, 32, 1+i, 32), ho->id);
			subRect(hp.x-1, hp.y, hp.z, genRect(32, 32, 33+i, 32), ho->id);
			subRect(hp.x, hp.y, hp.z, genRect(32, 32, 65+i, 32), ho->id);
		}
		LOCPLINT->adventureInt->update(); //updating screen
		CSDL_Ext::update(screen);
		CGI->screenh->updateScreen();

		++LOCPLINT->adventureInt->animValHitCount; //for animations
		if(LOCPLINT->adventureInt->animValHitCount == 4)
		{
			LOCPLINT->adventureInt->animValHitCount = 0;
			++LOCPLINT->adventureInt->anim;
			LOCPLINT->adventureInt->updateScreen = true;
		}
		++LOCPLINT->adventureInt->heroAnim;

		SDL_Delay(5);
		SDL_framerateDelay(mainFPSmng); //for animation purposes
	} //for(int i=1; i<32; i+=4)
	//main moving done
	//finishing move
	if(details.dst.x+1 == details.src.x && details.dst.y+1 == details.src.y) //tl
	{
		delObjRect(hp.x, hp.y-2, hp.z, ho->id);
		delObjRect(hp.x, hp.y-1, hp.z, ho->id);
		delObjRect(hp.x, hp.y, hp.z, ho->id);
		delObjRect(hp.x-1, hp.y, hp.z, ho->id);
		delObjRect(hp.x-2, hp.y, hp.z, ho->id);
		delObjRect(hp.x-3, hp.y, hp.z, ho->id);
	}
	else if(details.dst.x == details.src.x && details.dst.y+1 == details.src.y) //t
	{
		delObjRect(hp.x, hp.y, hp.z, ho->id);
		delObjRect(hp.x-1, hp.y, hp.z, ho->id);
		delObjRect(hp.x-2, hp.y, hp.z, ho->id);
	}
	else if(details.dst.x-1 == details.src.x && details.dst.y+1 == details.src.y) //tr
	{
		delObjRect(hp.x-2, hp.y-2, hp.z, ho->id);
		delObjRect(hp.x-2, hp.y-1, hp.z, ho->id);
		delObjRect(hp.x+1, hp.y, hp.z, ho->id);
		delObjRect(hp.x, hp.y, hp.z, ho->id);
		delObjRect(hp.x-1, hp.y, hp.z, ho->id);
		delObjRect(hp.x-2, hp.y, hp.z, ho->id);
	}
	else if(details.dst.x-1 == details.src.x && details.dst.y == details.src.y) //r
	{
		delObjRect(hp.x-2, hp.y-1, hp.z, ho->id);
		delObjRect(hp.x-2, hp.y, hp.z, ho->id);
	}
	else if(details.dst.x-1 == details.src.x && details.dst.y-1 == details.src.y) //br
	{
		delObjRect(hp.x-2, hp.y+1, hp.z, ho->id);
		delObjRect(hp.x-2, hp.y, hp.z, ho->id);
		delObjRect(hp.x+1, hp.y-1, hp.z, ho->id);
		delObjRect(hp.x, hp.y-1, hp.z, ho->id);
		delObjRect(hp.x-1, hp.y-1, hp.z, ho->id);
		delObjRect(hp.x-2, hp.y-1, hp.z, ho->id);
	}
	else if(details.dst.x == details.src.x && details.dst.y-1 == details.src.y) //b
	{
		delObjRect(hp.x, hp.y-1, hp.z, ho->id);
		delObjRect(hp.x-1, hp.y-1, hp.z, ho->id);
		delObjRect(hp.x-2, hp.y-1, hp.z, ho->id);
	}
	else if(details.dst.x+1 == details.src.x && details.dst.y-1 == details.src.y) //bl
	{
		delObjRect(hp.x, hp.y-1, hp.z, ho->id);
		delObjRect(hp.x-1, hp.y-1, hp.z, ho->id);
		delObjRect(hp.x-2, hp.y-1, hp.z, ho->id);
		delObjRect(hp.x-3, hp.y-1, hp.z, ho->id);
		delObjRect(hp.x, hp.y, hp.z, ho->id);
		delObjRect(hp.x, hp.y+1, hp.z, ho->id);
	}
	else if(details.dst.x+1 == details.src.x && details.dst.y == details.src.y) //l
	{
		delObjRect(hp.x, hp.y-1, hp.z, ho->id);
		delObjRect(hp.x, hp.y, hp.z, ho->id);
	}
	ho->pos = details.dst; //copy of hero's position
	//ho->moveDir = 0; //move ended
	ho->isStanding = true;
	//move finished
	CGI->mh->recalculateHideVisPosUnderObj(details.ho, true);
	adventureInt->minimap.draw();
	adventureInt->heroList.updateMove(ho);
}
void CPlayerInterface::heroKilled(const CGHeroInstance* hero)
{
	heroWins.erase(hero->ID);
}
void CPlayerInterface::heroCreated(const CGHeroInstance * hero)
{
	if(heroWins.find(hero->subID)==heroWins.end())
		heroWins.insert(std::pair<int,SDL_Surface*>(hero->subID,infoWin(hero)));
}

SDL_Surface * CPlayerInterface::drawPrimarySkill(const CGHeroInstance *curh, SDL_Surface *ret, int from, int to)
{
	char * buf = new char[10];
	for (int i=from;i<to;i++)
	{
		itoa(curh->primSkills[i],buf,10);
		printAtMiddle(buf,84+28*i,68,GEOR13,zwykly,ret);
	}
	delete[] buf;
	return ret;
}
SDL_Surface * CPlayerInterface::drawHeroInfoWin(const CGHeroInstance * curh)
{
	char * buf = new char[10];
	blueToPlayersAdv(hInfo,playerID,1);
	SDL_Surface * ret = SDL_DisplayFormat(hInfo);
	SDL_SetColorKey(ret,SDL_SRCCOLORKEY,SDL_MapRGB(ret->format,0,255,255));
	printAt(curh->name,75,15,GEOR13,zwykly,ret);
	drawPrimarySkill(curh, ret);
	for (std::map<int,std::pair<CCreature*,int> >::const_iterator i=curh->army.slots.begin(); i!=curh->army.slots.end();i++)
	{
		blitAt(CGI->creh->smallImgs[(*i).second.first->idNumber],slotsPos[(*i).first].first+1,slotsPos[(*i).first].second+1,ret);
		itoa((*i).second.second,buf,10);
		printAtMiddle(buf,slotsPos[(*i).first].first+17,slotsPos[(*i).first].second+39,GEORM,zwykly,ret);
	}
	blitAt(curh->type->portraitLarge,11,12,ret);
	itoa(curh->mana,buf,10);
	printAtMiddle(buf,166,109,GEORM,zwykly,ret); //mana points
	delete[] buf;
	blitAt(morale22->ourImages[curh->getCurrentMorale()+3].bitmap,14,84,ret);
	blitAt(luck22->ourImages[curh->getCurrentLuck()+3].bitmap,14,101,ret);
	//SDL_SaveBMP(ret,"inf1.bmp");
	return ret;
}

SDL_Surface * CPlayerInterface::drawTownInfoWin(const CGTownInstance * curh)
{
	char * buf = new char[10];
	blueToPlayersAdv(tInfo,playerID,1);
	SDL_Surface * ret = SDL_DisplayFormat(tInfo);
	SDL_SetColorKey(ret,SDL_SRCCOLORKEY,SDL_MapRGB(ret->format,0,255,255));
	printAt(curh->name,75,15,GEOR13,zwykly,ret);

	int pom = curh->fortLevel() - 1; if(pom<0) pom = 3;
	blitAt(forts->ourImages[pom].bitmap,115,42,ret);
	if((pom=curh->hallLevel())>=0)
		blitAt(halls->ourImages[pom].bitmap,77,42,ret);
	itoa(curh->dailyIncome(),buf,10);
	printAtMiddle(buf,167,70,GEORM,zwykly,ret);
	for (std::map<int,std::pair<CCreature*,int> >::const_iterator i=curh->army.slots.begin(); i!=curh->army.slots.end();i++)
	{
		if(!i->second.first)
			continue;
		blitAt(CGI->creh->smallImgs[(*i).second.first->idNumber],slotsPos[(*i).first].first+1,slotsPos[(*i).first].second+1,ret);
		itoa((*i).second.second,buf,10);
		printAtMiddle(buf,slotsPos[(*i).first].first+17,slotsPos[(*i).first].second+39,GEORM,zwykly,ret);
	}

	//blit town icon
	pom = curh->subID*2;
	if (!curh->hasFort())
		pom += F_NUMBER*2;
	if(curh->builded >= MAX_BUILDING_PER_TURN)
		pom++;
	blitAt(bigTownPic->ourImages[pom].bitmap,13,13,ret);
	delete[] buf;
	return ret;
}

void CPlayerInterface::openTownWindow(const CGTownInstance * town)
{
	adventureInt->hide();
	//timeHandler t;
	//t.getDif();
	castleInt = new CCastleInterface(town,true);
	//std::cout << "Loading town screen: " << t.getDif() <<std::endl;
}

SDL_Surface * CPlayerInterface::infoWin(const CGObjectInstance * specific) //specific=0 => draws info about selected town/hero
{
	if (specific)
	{
		switch (specific->ID)
		{
		case 34:
			return drawHeroInfoWin(dynamic_cast<const CGHeroInstance*>(specific));
			break;
		case 98:
			return drawTownInfoWin(dynamic_cast<const CGTownInstance*>(specific));
			break;
		default:
			return NULL;
			break;
		}
	}
	else
	{
		switch (adventureInt->selection.type)
		{
		case HEROI_TYPE:
			{
				const CGHeroInstance * curh = (const CGHeroInstance *)adventureInt->selection.selected;
				return drawHeroInfoWin(curh);
			}
		case TOWNI_TYPE:
			{
				return drawTownInfoWin((const CGTownInstance *)adventureInt->selection.selected);
			}
		default:
			return NULL;
		}
	}
	return NULL;
}

void CPlayerInterface::handleMouseMotion(SDL_Event *sEvent)
{
	for (int i=0; i<hoverable.size();i++)
	{
		if (isItIn(&hoverable[i]->pos,sEvent->motion.x,sEvent->motion.y))
		{
			if (!hoverable[i]->hovered)
				hoverable[i]->hover(true);
		}
		else if (hoverable[i]->hovered)
		{
			hoverable[i]->hover(false);
		}
	}
	for(int i=0; i<motioninterested.size();i++)
	{
		if (motioninterested[i]->strongInterest || isItIn(&motioninterested[i]->pos,sEvent->motion.x,sEvent->motion.y))
		{
			motioninterested[i]->mouseMoved(sEvent->motion);
		}
	}
	if(sEvent->motion.x<15)
	{
		LOCPLINT->adventureInt->scrollingLeft = true;
	}
	else
	{
		LOCPLINT->adventureInt->scrollingLeft = false;
	}
	if(sEvent->motion.x>screen->w-15)
	{
		LOCPLINT->adventureInt->scrollingRight = true;
	}
	else
	{
		LOCPLINT->adventureInt->scrollingRight = false;
	}
	if(sEvent->motion.y<15)
	{
		LOCPLINT->adventureInt->scrollingUp = true;
	}
	else
	{
		LOCPLINT->adventureInt->scrollingUp = false;
	}
	if(sEvent->motion.y>screen->h-15)
	{
		LOCPLINT->adventureInt->scrollingDown = true;
	}
	else
	{
		LOCPLINT->adventureInt->scrollingDown = false;
	}
}
void CPlayerInterface::handleKeyUp(SDL_Event *sEvent)
{
	switch (sEvent->key.keysym.sym)
	{
	case SDLK_LEFT:
		{
			LOCPLINT->adventureInt->scrollingLeft = false;
			break;
		}
	case (SDLK_RIGHT):
		{
			LOCPLINT->adventureInt->scrollingRight = false;
			break;
		}
	case (SDLK_UP):
		{
			LOCPLINT->adventureInt->scrollingUp = false;
			break;
		}
	case (SDLK_DOWN):
		{
			LOCPLINT->adventureInt->scrollingDown = false;
			break;
		}
	case (SDLK_u):
		{
			adventureInt->underground.clickLeft(false);
			break;
		}
	case (SDLK_m):
		{
			adventureInt->moveHero.clickLeft(false);
			break;
		}
	case (SDLK_e):
		{
			adventureInt->endTurn.clickLeft(false);
			break;
		}
	}
}
void CPlayerInterface::handleKeyDown(SDL_Event *sEvent)
{
	switch (sEvent->key.keysym.sym)
	{
	case SDLK_LEFT:
		{
			LOCPLINT->adventureInt->scrollingLeft = true;
			break;
		}
	case (SDLK_RIGHT):
		{
			LOCPLINT->adventureInt->scrollingRight = true;
			break;
		}
	case (SDLK_UP):
		{
			LOCPLINT->adventureInt->scrollingUp = true;
			break;
		}
	case (SDLK_DOWN):
		{
			LOCPLINT->adventureInt->scrollingDown = true;
			break;
		}
	case (SDLK_q):
		{
			exit(0);
			break;
		}
	case (SDLK_u):
		{
			adventureInt->underground.clickLeft(true);
			break;
		}
	case (SDLK_m):
		{
			adventureInt->moveHero.clickLeft(true);
			break;
		}
	case (SDLK_e):
		{
			adventureInt->endTurn.clickLeft(true);
			break;
		}
	}
}
void CPlayerInterface::handleEvent(SDL_Event *sEvent)
{
	current = sEvent;

	if(sEvent->type == SDL_MOUSEMOTION)
	{
		CGI->curh->cursorMove(sEvent->motion.x, sEvent->motion.y);
	}

	if(sEvent->type==SDL_QUIT) 
		exit(0);
	else if (sEvent->type==SDL_KEYDOWN)
	{
		handleKeyDown(sEvent);
	} //keydown end
	else if(sEvent->type==SDL_KEYUP) 
	{
		handleKeyUp(sEvent);
	}//keyup end
	else if(sEvent->type==SDL_MOUSEMOTION)
	{
		handleMouseMotion(sEvent);
	} //mousemotion end

	else if ((sEvent->type==SDL_MOUSEBUTTONDOWN) && (sEvent->button.button == SDL_BUTTON_LEFT))
	{
		LOGE("Left mouse button down1");
		for(int i=0; i<lclickable.size();i++)
		{
			if (isItIn(&lclickable[i]->pos,sEvent->motion.x,sEvent->motion.y))
			{
				lclickable[i]->clickLeft(true);
			}
		}
	}
	else if ((sEvent->type==SDL_MOUSEBUTTONUP) && (sEvent->button.button == SDL_BUTTON_LEFT))
	{
		for(int i=0; i<lclickable.size();i++)
		{
			if (isItIn(&lclickable[i]->pos,sEvent->motion.x,sEvent->motion.y))
			{
				lclickable[i]->clickLeft(false);
			}
			else
				lclickable[i]->clickLeft(boost::logic::indeterminate);
		}
	}
	else if ((sEvent->type==SDL_MOUSEBUTTONDOWN) && (sEvent->button.button == SDL_BUTTON_RIGHT))
	{
		for(int i=0; i<rclickable.size();i++)
		{
			if (isItIn(&rclickable[i]->pos,sEvent->motion.x,sEvent->motion.y))
			{
				rclickable[i]->clickRight(true);
			}
		}
	}
	else if ((sEvent->type==SDL_MOUSEBUTTONUP) && (sEvent->button.button == SDL_BUTTON_RIGHT))
	{
		for(int i=0; i<rclickable.size();i++)
		{
			if (isItIn(&rclickable[i]->pos,sEvent->motion.x,sEvent->motion.y))
			{
				rclickable[i]->clickRight(false);
			}
			else
				rclickable[i]->clickRight(boost::logic::indeterminate);
		}
	}
	current = NULL;

} //event end

int3 CPlayerInterface::repairScreenPos(int3 pos)
{
	if(pos.x<=-Woff)
		pos.x = -Woff+1;
	if(pos.y<=-Hoff)
		pos.y = -Hoff+1;
	if(pos.x>CGI->mh->reader->map.width - this->adventureInt->terrain.tilesw + Woff)
		pos.x = CGI->mh->reader->map.width - this->adventureInt->terrain.tilesw + Woff;
	if(pos.y>CGI->mh->reader->map.height - this->adventureInt->terrain.tilesh + Hoff)
		pos.y = CGI->mh->reader->map.height - this->adventureInt->terrain.tilesh + Hoff;
	return pos;
}
void CPlayerInterface::heroPrimarySkillChanged(const CGHeroInstance * hero, int which, int val)
{
	SDL_FreeSurface(heroWins[hero->subID]);//TODO: moznaby zmieniac jedynie fragment bitmapy zwiazany z dana umiejetnoscia
	heroWins[hero->subID] = infoWin(hero); //a nie przerysowywac calosc. Troche roboty, obecnie chyba nie wartej swieczki.
	if (adventureInt->selection.selected == hero)
		adventureInt->infoBar.draw();
	return;
}

void CPlayerInterface::receivedResource(int type, int val)
{
	adventureInt->resdatabar.draw();
}

void CPlayerInterface::showSelDialog(std::string text, std::vector<CSelectableComponent*> & components, int askID)
{
	adventureInt->hide(); //dezaktywacja starego interfejsu
	CSelWindow * temp = CMessage::genSelWindow(text,LOCPLINT->playerID,35,components,playerID);
	LOCPLINT->objsToBlit.push_back(temp);
	temp->pos.x=300-(temp->pos.w/2);
	temp->pos.y=300-(temp->pos.h/2);
	temp->okb.pos.x = temp->okb.posr.x + temp->pos.x;
	temp->okb.pos.y = temp->okb.posr.y + temp->pos.y;
	temp->okb.activate();
	for (int i=0;i<temp->components.size();i++)
	{
		temp->components[i]->activate();
		temp->components[i]->pos.x += temp->pos.x;
		temp->components[i]->pos.y += temp->pos.y;
	}
	temp->ID = askID;
	components[0]->clickLeft(true);
}
void CPlayerInterface::heroVisitsTown(const CGHeroInstance* hero, const CGTownInstance * town)
{
	openTownWindow(town);
}
void CPlayerInterface::garrisonChanged(const CGObjectInstance * obj)
{
	if(obj->ID == 34) //hero
	{
		const CGHeroInstance * hh;
		if(hh = dynamic_cast<const CGHeroInstance*>(obj))
		{
			SDL_FreeSurface(heroWins[hh->subID]);
			heroWins[hh->subID] = infoWin(hh);
		}
		CHeroWindow * hw = dynamic_cast<CHeroWindow *>(curint);
		if(hw)
		{
			hw->garInt->recreateSlots();
			hw->garInt->show();
		}
		else if(castleInt == curint) //opened town window - redraw town garrsion slots (change is within hero garr) 
		{
			castleInt->garr->highlighted = NULL;
			castleInt->garr->recreateSlots();
		}
		
	}
	else if (obj->ID == 98) //town
	{
		const CGTownInstance * tt;
		if(tt = dynamic_cast<const CGTownInstance*>(obj))
		{
			SDL_FreeSurface(townWins[tt->identifier]);
			townWins[tt->identifier] = infoWin(tt);
		}
		
		const CCastleInterface *ci = dynamic_cast<CCastleInterface*>(curint);
		if(ci)
		{
			ci->garr->highlighted = NULL;
			ci->garr->recreateSlots();
		}
	}
}
void CPlayerInterface::buildChanged(const CGTownInstance *town, int buildingID, int what) //what: 1 - built, 2 - demolished
{
	if(curint!=castleInt)
		return;
	if(castleInt->town!=town)
		return;
	switch(what)
	{
	case 1:
		castleInt->addBuilding(buildingID);
		break;
	case 2:
		castleInt->removeBuilding(buildingID);
		break;
	}
}

void CPlayerInterface::battleStart(CCreatureSet * army1, CCreatureSet * army2, int3 tile, CGHeroInstance *hero1, CGHeroInstance *hero2, tribool side) //called by engine when battle starts; side=0 - left, side=1 - right
{
	curint->deactivate();
	curint = new CBattleInterface(army1,army2,hero1,hero2);
	curint->activate();
	LOCPLINT->objsToBlit.push_back(dynamic_cast<IShowable*>(curint));
}

void CPlayerInterface::battlefieldPrepared(int battlefieldType, std::vector<CObstacle*> obstacles) //called when battlefield is prepared, prior the battle beginning
{
}

void CPlayerInterface::battleNewRound(int round) //called at the beggining of each turn, round=-1 is the tactic phase, round=0 is the first "normal" turn
{
	dynamic_cast<CBattleInterface*>(curint)->newRound(round);
}

void CPlayerInterface::actionStarted(BattleAction action)//occurs BEFORE every action taken by any stack or by the hero
{
}

void CPlayerInterface::actionFinished(BattleAction action)//occurs AFTER every action taken by any stack or by the hero
{
	//dynamic_cast<CBattleInterface*>(curint)->givenCommand = -1;
}

BattleAction CPlayerInterface::activeStack(int stackID) //called when it's turn of that stack
{
	unsigned char showCount = 0;
	dynamic_cast<CBattleInterface*>(curint)->stackActivated(stackID);
	while(!dynamic_cast<CBattleInterface*>(curint)->givenCommand) //while current unit can perform an action
	{
		++showCount;
		SDL_Event sEvent;
		while (SDL_PollEvent(&sEvent))  //wait for event...
		{
			LOCPLINT->handleEvent(&sEvent);
		}
		if(showCount%2==0)
			for(int i=0;i<objsToBlit.size();i++)
				objsToBlit[i]->show();
		//SDL_Flip(screen);
		CSDL_Ext::update(screen);
		
		/*timeHandler th;
		th.getDif();
		int tv = th.getDif();
		for (int i=0;i<((CBattleInterface*)this->curint)->timeinterested.size();i++)
		{
			if (timeinterested[i]->toNextTick>=0)
				timeinterested[i]->toNextTick-=tv;
			if (timeinterested[i]->toNextTick<0)
				timeinterested[i]->tick();
		}*/

		SDL_Delay(1); //give time for other apps
		SDL_framerateDelay(mainFPSmng);
	}
	BattleAction ret = *(dynamic_cast<CBattleInterface*>(curint)->givenCommand);
	delete dynamic_cast<CBattleInterface*>(curint)->givenCommand;
	dynamic_cast<CBattleInterface*>(curint)->givenCommand = NULL;
	return ret;
}

void CPlayerInterface::battleEnd(CCreatureSet * army1, CCreatureSet * army2, CArmedInstance *hero1, CArmedInstance *hero2, std::vector<int> capturedArtifacts, int expForWinner, bool winner)
{
	dynamic_cast<CBattleInterface*>(curint)->deactivate();
	LOCPLINT->objsToBlit.erase(std::find(LOCPLINT->objsToBlit.begin(),LOCPLINT->objsToBlit.end(),dynamic_cast<IShowable*>(curint)));
	delete dynamic_cast<CBattleInterface*>(curint);
	curint = adventureInt;
	adventureInt->activate();
}

void CPlayerInterface::battleStackMoved(int ID, int dest, bool startMoving, bool endMoving)
{
	dynamic_cast<CBattleInterface*>(curint)->stackMoved(ID, dest, startMoving, endMoving);
}

void CPlayerInterface::battleStackAttacking(int ID, int dest)
{
	dynamic_cast<CBattleInterface*>(curint)->stackAttacking(ID, dest);
}

void CPlayerInterface::battleStackIsAttacked(int ID)
{
	dynamic_cast<CBattleInterface*>(curint)->stackIsAttacked(ID);
}

void CPlayerInterface::battleStackKilled(int ID)
{
	dynamic_cast<CBattleInterface*>(curint)->stackKilled(ID);
}

void CPlayerInterface::showComp(SComponent comp)
{
	adventureInt->infoBar.showComp(&comp,4000);
}

void CPlayerInterface::showInfoDialog(std::string text, std::vector<SComponent*> & components)
{
	curint->deactivate(); //dezaktywacja starego interfejsu
	CInfoWindow * temp = CMessage::genIWindow(text,LOCPLINT->playerID,32,components);
	LOCPLINT->objsToBlit.push_back(temp);
	temp->pos.x=300-(temp->pos.w/2);
	temp->pos.y=300-(temp->pos.h/2);
	temp->okb.pos.x = temp->okb.posr.x + temp->pos.x;
	temp->okb.pos.y = temp->okb.posr.y + temp->pos.y;
	temp->okb.activate();
	for (int i=0;i<temp->components.size();i++)
	{
		temp->components[i]->activate();
		temp->components[i]->pos.x += temp->pos.x;
		temp->components[i]->pos.y += temp->pos.y;
	}
}
void CPlayerInterface::removeObjToBlit(IShowable* obj)
{
	objsToBlit.erase
		(std::find(objsToBlit.begin(),objsToBlit.end(),obj));
	//delete obj;
}
void CPlayerInterface::tileRevealed(int3 pos)
{
	adventureInt->minimap.showTile(pos);
}
void CPlayerInterface::tileHidden(int3 pos)
{
	adventureInt->minimap.hideTile(pos);
}
void CPlayerInterface::openHeroWindow(const CGHeroInstance *hero)
{
	adventureInt->heroWindow->setHero(hero);
	this->objsToBlit.push_back(adventureInt->heroWindow);
	adventureInt->hide();
	adventureInt->heroWindow->activate();
}
CStatusBar::CStatusBar(int x, int y, std::string name, int maxw)
{
	bg=CGI->bitmaph->loadBitmap(name);
	SDL_SetColorKey(bg,SDL_SRCCOLORKEY,SDL_MapRGB(bg->format,0,255,255));
	pos.x=x;
	pos.y=y;
	if(maxw >= 0)
		pos.w = std::min(bg->w,maxw);
	else
		pos.w=bg->w;
	pos.h=bg->h;
	middlex=(pos.w/2)+x;
	middley=(bg->h/2)+y;
}
CStatusBar::~CStatusBar()
{
	SDL_FreeSurface(bg);
}
void CStatusBar::clear()
{
	current="";
	SDL_Rect pom = genRect(pos.h,pos.w,pos.x,pos.y);
	SDL_BlitSurface(bg,&genRect(pos.h,pos.w,0,0),screen,&pom);
}
void CStatusBar::print(std::string text)
{
	current=text;
	SDL_Rect pom = genRect(pos.h,pos.w,pos.x,pos.y);
	SDL_BlitSurface(bg,&genRect(pos.h,pos.w,0,0),screen,&pom);
	printAtMiddle(current,middlex,middley,GEOR13,zwykly);
}
void CStatusBar::show()
{
	SDL_Rect pom = genRect(pos.h,pos.w,pos.x,pos.y);
	SDL_BlitSurface(bg,&genRect(pos.h,pos.w,0,0),screen,&pom);
	printAtMiddle(current,middlex,middley,GEOR13,zwykly);
}
std::string CStatusBar::getCurrent()
{
	return current;
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
CList::CList(int Size)
:SIZE(Size)
{
}
CHeroList::CHeroList(int Size)
:CList(Size)
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
	bg = CSDL_Ext::newSurface(68,193,screen);
	SDL_BlitSurface(LOCPLINT->adventureInt->bg,&genRect(193,68,607,196),bg,&genRect(193,68,0,0));
}
void CHeroList::genList()
{
	int howMany = LOCPLINT->cb->howManyHeroes();
	for (int i=0;i<howMany;i++)
	{
		items.push_back(std::pair<const CGHeroInstance *,CPath *>(LOCPLINT->cb->getHeroInfo(LOCPLINT->playerID,i,0),NULL));
	}
}
void CHeroList::select(int which)
{
	if (which<0)
	{
		selected = which;
		LOCPLINT->adventureInt->selection.selected = LOCPLINT->adventureInt->terrain.currentPath = NULL;
		draw();
		LOCPLINT->adventureInt->infoBar.draw(NULL);
	}
	if (which>=items.size()) 
		return;
	selected = which;
	LOCPLINT->adventureInt->centerOn(items[which].first->pos);
	LOCPLINT->adventureInt->selection.type = HEROI_TYPE;
	LOCPLINT->adventureInt->selection.selected = items[which].first;
	LOCPLINT->adventureInt->terrain.currentPath = items[which].second;
	draw();
	LOCPLINT->adventureInt->townList.draw();
	
	LOCPLINT->adventureInt->infoBar.draw(NULL);
}
void CHeroList::clickLeft(tribool down)
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
		else if(isItIn(&arrdop,LOCPLINT->current->motion.x,LOCPLINT->current->motion.y) && (items.size()-from>5))
		{
			blitAt(arrdo->ourImages[1].bitmap,arrdop.x,arrdop.y);
			pressed = false;
			return;
		}
		/***************************HEROES*****************************************/
		int hx = LOCPLINT->current->motion.x, hy = LOCPLINT->current->motion.y;
		hx-=pos.x;
		hy-=pos.y; hy-=arrup->ourImages[0].bitmap->h;
		int ny = hy/32;
		if (ny>=5 || ny<0)
			return;
		if ( (ny+from)==selected && (LOCPLINT->adventureInt->selection.type == HEROI_TYPE))
			LOCPLINT->openHeroWindow(items[selected].first);//print hero screen
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
void CHeroList::mouseMoved (SDL_MouseMotionEvent & sEvent)
{
	if(isItIn(&arrupp,LOCPLINT->current->motion.x,LOCPLINT->current->motion.y))
	{
		if (from>0)
			LOCPLINT->adventureInt->statusbar.print(CGI->preth->zelp[303].first);
		else
			LOCPLINT->adventureInt->statusbar.clear();
		return;
	}
	else if(isItIn(&arrdop,LOCPLINT->current->motion.x,LOCPLINT->current->motion.y))
	{
		if ((items.size()-from)  >  5)
			LOCPLINT->adventureInt->statusbar.print(CGI->preth->zelp[304].first);
		else
			LOCPLINT->adventureInt->statusbar.clear();
		return;
	}
	//if not buttons then heroes
	int hx = LOCPLINT->current->motion.x, hy = LOCPLINT->current->motion.y;
	hx-=pos.x;
	hy-=pos.y; hy-=arrup->ourImages[0].bitmap->h;
	int ny = hy/32;
	if ((ny>5 || ny<0) || (from+ny>=items.size()))
	{
		LOCPLINT->adventureInt->statusbar.clear();
		return;
	}
	std::vector<std::string> temp;
	temp.push_back(items[from+ny].first->name);
	temp.push_back(items[from+ny].first->type->heroClass->name);
	LOCPLINT->adventureInt->statusbar.print( processStr(CGI->generaltexth->allTexts[15],temp) );
	//select(ny+from);
}
void CHeroList::clickRight(tribool down)
{
	if (down)
	{
		/***************************ARROWS*****************************************/
		if(isItIn(&arrupp,LOCPLINT->current->motion.x,LOCPLINT->current->motion.y) && from>0)
		{
			LOCPLINT->adventureInt->handleRightClick(CGI->preth->zelp[303].second,down,this);
		}
		else if(isItIn(&arrdop,LOCPLINT->current->motion.x,LOCPLINT->current->motion.y) && (items.size()-from>5))
		{
			LOCPLINT->adventureInt->handleRightClick(CGI->preth->zelp[304].second,down,this);
		}
		//if not buttons then heroes
		int hx = LOCPLINT->current->motion.x, hy = LOCPLINT->current->motion.y;
		hx-=pos.x;
		hy-=pos.y; hy-=arrup->ourImages[0].bitmap->h;
		int ny = hy/32;
		if ((ny>5 || ny<0) || (from+ny>=items.size()))
		{
			return;
		}

		//show popup
		CInfoPopup * ip = new CInfoPopup(LOCPLINT->heroWins[items[from+ny].first->subID],LOCPLINT->current->motion.x-LOCPLINT->heroWins[items[from+ny].first->subID]->w,LOCPLINT->current->motion.y-LOCPLINT->heroWins[items[from+ny].first->subID]->h,false);
		ip->activate();
	}
	else
	{
			LOCPLINT->adventureInt->handleRightClick(CGI->preth->zelp[303].second,down,this);
			LOCPLINT->adventureInt->handleRightClick(CGI->preth->zelp[304].second,down,this);
	}
}
void CHeroList::hover (bool on)
{
}
void CHeroList::keyPressed (SDL_KeyboardEvent & key)
{
}
void CHeroList::updateHList()
{
	items.clear();
	genList();
	if(selected>=items.size())
		select(items.size()-1);
	if(items.size()==0)
		LOCPLINT->adventureInt->townList.select(0);
	else
		select(selected);
}
void CHeroList::updateMove(const CGHeroInstance* which) //draws move points bar
{
	int ser = LOCPLINT->cb->getHeroSerial(which);
	ser -= from;
	int pom = (which->movement)/100;
	blitAt(mobile->ourImages[pom].bitmap,posmobx,posmoby+ser*32); //move point
}
void CHeroList::draw()
{	
	for (int iT=0+from;iT<5+from;iT++)
	{
		int i = iT-from;
		if (iT>=items.size())
		{
			blitAt(mobile->ourImages[0].bitmap,posmobx,posmoby+i*32);
			blitAt(mana->ourImages[0].bitmap,posmanx,posmany+i*32);
			blitAt(empty,posporx,pospory+i*32);
			continue;
		}
		int pom = (LOCPLINT->cb->getHeroInfo(LOCPLINT->playerID,iT,0)->movement)/100;
		if (pom>25) pom=25;
		if (pom<0) pom=0;
		blitAt(mobile->ourImages[pom].bitmap,posmobx,posmoby+i*32); //move point
		pom = (LOCPLINT->cb->getHeroInfo(LOCPLINT->playerID,iT,0)->mana)/5; //bylo: .../10;
		if (pom>25) pom=25;
		if (pom<0) pom=0;
		blitAt(mana->ourImages[pom].bitmap,posmanx,posmany+i*32); //mana
		SDL_Surface * temp = LOCPLINT->cb->getHeroInfo(LOCPLINT->playerID,iT,0)->type->portraitSmall;
		blitAt(temp,posporx,pospory+i*32);
		if ((selected == iT) && (LOCPLINT->adventureInt->selection.type == HEROI_TYPE))
		{
			blitAt(selection,posporx,pospory+i*32);
		}
		//TODO: support for custom portraits
	}
	if (from>0)
		blitAt(arrup->ourImages[0].bitmap,arrupp.x,arrupp.y);
	else
		blitAt(arrup->ourImages[2].bitmap,arrupp.x,arrupp.y);

	if (items.size()-from>5)
		blitAt(arrdo->ourImages[0].bitmap,arrdop.x,arrdop.y);
	else
		blitAt(arrdo->ourImages[2].bitmap,arrdop.x,arrdop.y);
}



CTownList::~CTownList()
{
	delete arrup;
	delete arrdo;
}

CTownList::CTownList(int Size, SDL_Rect * Pos, int arupx, int arupy, int ardox, int ardoy)
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
	if (which>=items.size()) 
		return;
	selected = which;
	if(!fun.empty())
		fun();
}

void CTownList::mouseMoved (SDL_MouseMotionEvent & sEvent)
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

void CTownList::clickLeft(tribool down)
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

void CTownList::clickRight(tribool down)
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

void CTownList::hover (bool on)
{
}

void CTownList::keyPressed (SDL_KeyboardEvent & key)
{
}

void CTownList::draw()
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


void CRecrutationWindow::close()
{
	deactivate();
	delete this;
	LOCPLINT->curint->activate();
	CCastleInterface *pom;
	if(pom=dynamic_cast<CCastleInterface*>(LOCPLINT->curint))
		pom->showAll();
}
void CRecrutationWindow::Max()
{
	slider->moveTo(slider->amount);
}
void CRecrutationWindow::Buy()
{
	rec->recruit(creatures[which].ID,slider->value);
	close();
}
void CRecrutationWindow::Cancel()
{
	close();
}
void CRecrutationWindow::sliderMoved(int to)
{
}
void CRecrutationWindow::clickLeft(tribool down)
{
	int curx = 192 + 51 - (102*creatures.size()/2) - (18*(creatures.size()-1)/2);
	for(int i=0;i<creatures.size();i++)
	{
		if(isItIn(&genRect(132,102,pos.x+curx,pos.y+64),LOCPLINT->current->motion.x,LOCPLINT->current->motion.y))
		{
			which = i;	
			int newAmount = std::min(amounts[i],creatures[i].amount);
			slider->amount = newAmount;
			if(slider->value > newAmount)
				slider->moveTo(newAmount);
			else
				slider->moveTo(slider->value);
			curx = 192 + 51 - (102*creatures.size()/2) - (18*(creatures.size()-1)/2);
			for(int j=0;j<creatures.size();j++)
			{
				if(which==j)
					drawBorder(bitmap,curx,64,102,132,int3(255,0,0));
				else
					drawBorder(bitmap,curx,64,102,132,int3(239,215,123));
				curx += 120;
			}
			break;
		}
		curx += 120;
	}
}
void CRecrutationWindow::activate()
{
	LOCPLINT->objsToBlit.push_back(this);
	ClickableL::activate();
	buy->activate();
	max->activate();
	cancel->activate();
	slider->activate();
}
void CRecrutationWindow::deactivate()
{
	LOCPLINT->objsToBlit.erase(std::find(LOCPLINT->objsToBlit.begin(),LOCPLINT->objsToBlit.end(),this));
	ClickableL::deactivate();
	buy->deactivate();
	max->deactivate();
	cancel->deactivate();
	slider->deactivate();
}
void CRecrutationWindow::show(SDL_Surface * to)
{
	static char c=0;
	blitAt(bitmap,pos.x,pos.y,to?to:screen);
	buy->show();
	max->show();
	cancel->show();
	slider->show();
	char pom[15];
	itoa(creatures[which].amount,pom,10); //available
	printAtMiddle(pom,pos.x+205,pos.y+252,GEOR13,zwykly,screen);
	itoa(slider->value,pom,10); //recruit
	printAtMiddle(pom,pos.x+279,pos.y+252,GEOR13,zwykly,screen);
	printAtMiddle(CGI->generaltexth->allTexts[16] + " " + CGI->creh->creatures[creatures[which].ID].namePl,pos.x+243,pos.y+32,GEOR16,tytulowy,screen); //eg "Recruit Dragon flies"
	int curx = pos.x+115-creatures[which].res.size()*16;
	for(int i=0;i<creatures[which].res.size();i++)
	{
		blitAt(CGI->townh->resources->ourImages[creatures[which].res[i].first].bitmap,curx,pos.y+243,screen);
		blitAt(CGI->townh->resources->ourImages[creatures[which].res[i].first].bitmap,curx+258,pos.y+243,screen);
		itoa(creatures[which].res[i].second,pom,10);
		printAtMiddle(pom,curx+12,pos.y+286,GEOR13,zwykly,screen);
		itoa(creatures[which].res[i].second * slider->value,pom,10);
		printAtMiddle(pom,curx+12+258,pos.y+286,GEOR13,zwykly,screen);
		curx+=32;
	}

	curx = pos.x + 192 + 102 - (102*creatures.size()/2) - (18*(creatures.size()-1)/2);
	for(int i=0;i<creatures.size();i++)
	{
		blitAt(CGI->creh->backgrounds[CGI->creh->creatures[creatures[i].ID].faction],curx-50,pos.y+130-65);
		SDL_Rect dst = genRect(130,100,curx-50,pos.y+130-65);
		creatures[i].anim->nextFrameMiddle(screen,curx+20,pos.y+110,true,!(c%2),false,&dst);
		curx += 120;
	}
	c++;
}
CRecrutationWindow::CRecrutationWindow(std::vector<std::pair<int,int> > &Creatures, IRecruit *irec) //creatures - pairs<creature_ID,amount>
:rec(irec)
{
	which = 0;
	creatures.resize(Creatures.size());
	amounts.resize(Creatures.size());
	for(int i=0;i<creatures.size();i++)
	{
		creatures[i].amount = Creatures[i].second;
		creatures[i].ID = Creatures[i].first;
		for(int j=0;j<CGI->creh->creatures[Creatures[i].first].cost.size();j++)
			if(CGI->creh->creatures[Creatures[i].first].cost[j])
				creatures[i].res.push_back(std::make_pair(j,CGI->creh->creatures[Creatures[i].first].cost[j]));
		creatures[i].anim = new CCreatureAnimation(CGI->creh->creatures[Creatures[i].first].animDefName);
		amounts[i] = CGI->creh->creatures[Creatures[i].first].maxAmount(LOCPLINT->cb->getResourceAmount());
	}
	SDL_Surface *hhlp = CGI->bitmaph->loadBitmap("TPRCRT.bmp");
	blueToPlayersAdv(hhlp,LOCPLINT->playerID);
	bitmap = SDL_ConvertSurface(hhlp,screen->format,0); //na 8bitowej mapie by sie psulo
	SDL_SetColorKey(bitmap,SDL_SRCCOLORKEY,SDL_MapRGB(bitmap->format,0,255,255));
	SDL_FreeSurface(hhlp);
	pos.x = screen->w/2 - bitmap->w/2;
	pos.y = screen->h/2 - bitmap->h/2;
	pos.w = bitmap->w;
	pos.h = bitmap->h;
	slider = new CSlider(pos.x+176,pos.y+279,135,boost::bind(&CRecrutationWindow::sliderMoved,this, _1),1,std::min(amounts[0],creatures[0].amount),0,true);
	std::string pom;
	printAtMiddle(CGI->generaltexth->allTexts[346],113,231,GEOR13,zwykly,bitmap); //cost per troop t
	printAtMiddle(CGI->generaltexth->allTexts[465],205,231,GEOR13,zwykly,bitmap); //available t
	printAtMiddle(CGI->generaltexth->allTexts[16],279,231,GEOR13,zwykly,bitmap); //recruit t
	printAtMiddle(CGI->generaltexth->allTexts[466],373,231,GEOR13,zwykly,bitmap); //total cost t
	drawBorder(bitmap,172,222,67,42,int3(239,215,123));
	drawBorder(bitmap,246,222,67,42,int3(239,215,123));
	drawBorder(bitmap,64,222,99,76,int3(239,215,123));
	drawBorder(bitmap,322,222,99,76,int3(239,215,123));
	drawBorder(bitmap,133,312,66,34,int3(173,142,66));
	drawBorder(bitmap,211,312,66,34,int3(173,142,66));
	drawBorder(bitmap,289,312,66,34,int3(173,142,66));

	//border for creatures
	int curx = 192 + 51 - (102*creatures.size()/2) - (18*(creatures.size()-1)/2);
	for(int i=0;i<creatures.size();i++)
	{
		creatures[i].pos.x = curx+1;
		creatures[i].pos.y = 65;
		creatures[i].pos.w = 100;
		creatures[i].pos.h = 130;
		if(which==i)
			drawBorder(bitmap,curx,64,102,132,int3(255,0,0));
		else
			drawBorder(bitmap,curx,64,102,132,int3(239,215,123));
		curx += 120;
	}

	max = new AdventureMapButton("","",boost::bind(&CRecrutationWindow::Max,this),pos.x+134,pos.y+313,"IRCBTNS.DEF");
	buy = new AdventureMapButton("","",boost::bind(&CRecrutationWindow::Buy,this),pos.x+212,pos.y+313,"IBY6432.DEF");
	cancel = new AdventureMapButton("","",boost::bind(&CRecrutationWindow::Cancel,this),pos.x+290,pos.y+313,"ICN6432.DEF");
	LOCPLINT->curint->deactivate();
	//AdventureMapButton( std::string Name, std::string HelpBox, void(T::*Function)(), 
	//int x, int y, std::string defName, T* Owner, bool activ=false,  std::vector<std::string> * add = NULL, bool playerColoredButton = true );//c-tor
}//(int x, int y, int totalw, T*Owner,void(T::*Moved)(int to), int Capacity, int Amount, int Value, bool Horizontal)
CRecrutationWindow::~CRecrutationWindow()
{
	SDL_FreeSurface(bitmap);
	delete slider;
}

CSplitWindow::CSplitWindow(int cid, int max, CGarrisonInt *Owner)
{
	c=cid;
	slider = NULL;
	gar = Owner;
	bitmap = CGI->bitmaph->loadBitmap("GPUCRDIV.bmp");
	SDL_SetColorKey(bitmap,SDL_SRCCOLORKEY,SDL_MapRGB(bitmap->format,0,255,255));
	pos.x = screen->w/2 - bitmap->w/2;
	pos.y = screen->h/2 - bitmap->h/2;
	pos.w = bitmap->w;
	pos.h = bitmap->h;
	ok = new AdventureMapButton("","",boost::bind(&CSplitWindow::split,this),pos.x+20,pos.y+263,"IOK6432.DEF");
	cancel = new AdventureMapButton("","",boost::bind(&CSplitWindow::close,this),pos.x+214,pos.y+263,"ICN6432.DEF");
	slider = new CSlider(pos.x+21,pos.y+194,257,boost::bind(&CSplitWindow::sliderMoved,this,_1),1,max,0,true);
	a1 = max;
	a2 = 0;
	anim = new CCreatureAnimation(CGI->creh->creatures[cid].animDefName);
	anim->setType(1);

	std::string title = CGI->generaltexth->allTexts[256];
	boost::algorithm::replace_first(title,"%s",CGI->creh->creatures[cid].namePl);
	printAtMiddle(title,150,34,GEOR16,tytulowy,bitmap);
}
CSplitWindow::~CSplitWindow()
{
	SDL_FreeSurface(bitmap);
}
void CSplitWindow::activate()
{
	LOCPLINT->objsToBlit.push_back(this);
	KeyInterested::activate();
	ok->activate();
	cancel->activate();
	slider->activate();
}
void CSplitWindow::deactivate()
{
	LOCPLINT->objsToBlit.erase(std::find(LOCPLINT->objsToBlit.begin(),LOCPLINT->objsToBlit.end(),this));
	KeyInterested::deactivate();
	ok->deactivate();
	cancel->deactivate();
	slider->deactivate();
}
void CSplitWindow::split()
{
	gar->splitStacks(a2);
	close();
}
void CSplitWindow::close()
{
	deactivate();
	delete this;
	LOCPLINT->curint->activate();
	
	CCastleInterface *c = dynamic_cast<CCastleInterface*>(LOCPLINT->curint);
	if(c) c->showAll();
}
void CSplitWindow::sliderMoved(int to)
{
	a2 = to;
	if(slider)
		a1 = slider->amount - to;
}
void CSplitWindow::show(SDL_Surface * to)
{
	char pom[15];
	blitAt(bitmap,pos.x,pos.y,screen);
	ok->show();
	cancel->show();
	slider->show();
	itoa(a1,pom,10);
	printAtMiddle(pom,pos.x+70,pos.y+237,GEOR16,zwykly,screen);
	itoa(a2,pom,10);
	printAtMiddle(pom,pos.x+233,pos.y+237,GEOR16,zwykly,screen);


	blitAt(CGI->creh->backgrounds[CGI->creh->creatures[c].faction],pos.x+20,pos.y+54);
	anim->nextFrameMiddle(screen,pos.x+20+70,pos.y+54+55,true,false,false);

	blitAt(CGI->creh->backgrounds[CGI->creh->creatures[c].faction],pos.x+177,pos.y+54);
	anim->nextFrameMiddle(screen,pos.x+177+70,pos.y+54+55,true,false,false);
}
void CSplitWindow::keyPressed (SDL_KeyboardEvent & key)
{
	//TODO: zeby sie dalo recznie wpisywac
}



void CCreInfoWindow::show(SDL_Surface * to)
{
	char pom[15];
	blitAt(bitmap,pos.x,pos.y,screen);
	blitAt(CGI->creh->backgrounds[c->faction],pos.x+21,pos.y+48,screen);
	anim->nextFrameMiddle(screen,pos.x+90,pos.y+95,true,false,false);
}

CCreInfoWindow::CCreInfoWindow(int Cid, int Type, StackState *State, boost::function<void()> Upg, boost::function<void()> Dsm)
:ok(0),dismiss(0),upgrade(0),type(Type)
{
	c = &CGI->creh->creatures[Cid];
	bitmap = CGI->bitmaph->loadBitmap("CRSTKPU.bmp");
	pos.x = screen->w/2 - bitmap->w/2;
	pos.y = screen->h/2 - bitmap->h/2;
	pos.w = bitmap->w;
	pos.h = bitmap->h;
	blueToPlayersAdv(bitmap,LOCPLINT->playerID);
	SDL_SetColorKey(bitmap,SDL_SRCCOLORKEY,SDL_MapRGB(bitmap->format,0,255,255));
	anim = new CCreatureAnimation(c->animDefName);
	anim->setType(1);

	char pom[25];int hlp=0;
	printAtMiddle(c->namePl,149,30,GEOR13,zwykly,bitmap); //creature name

	//atttack
	printAt(CGI->preth->zelp[435].first,155,48,GEOR13,zwykly,bitmap);
	itoa(c->attack,pom,10);
	if(State && State->attackBonus)
	{
		int hlp = log10f(c->attack)+2;
		pom[hlp-1] = ' '; pom[hlp] = '(';
		itoa(c->attack+State->attackBonus,pom+hlp+1,10);
		hlp += 2+(int)log10f(State->attackBonus+c->attack);
		pom[hlp] = ')'; pom[hlp+1] = '\0';
	}
	printToWR(pom,276,61,GEOR13,zwykly,bitmap);

	//defense
	printAt(CGI->preth->zelp[436].first,155,67,GEOR13,zwykly,bitmap);
	itoa(c->defence,pom,10);
	if(State && State->defenseBonus)
	{
		int hlp = log10f(c->defence)+2;
		pom[hlp-1] = ' '; pom[hlp] = '(';
		itoa(c->defence+State->defenseBonus,pom+hlp+1,10);
		pom[hlp+2+(int)log10f(State->defenseBonus+c->defence)] = ')';
	}
	printToWR(pom,276,80,GEOR13,zwykly,bitmap);

	//shots
	if(c->shots)
	{
		printAt(CGI->generaltexth->allTexts[198],155,86,GEOR13,zwykly,bitmap);
		itoa(c->shots,pom,10);
		printToWR(pom,276,99,GEOR13,zwykly,bitmap);
	}

	//damage
	printAt(CGI->generaltexth->allTexts[199],155,105,GEOR13,zwykly,bitmap);
	itoa(c->damageMin,pom,10);
	hlp=log10f(c->damageMin)+2;
	pom[hlp-1]=' '; pom[hlp]='-'; pom[hlp+1]=' ';
	itoa(c->damageMax,pom+hlp+2,10);
	printToWR(pom,276,118,GEOR13,zwykly,bitmap);

	//health
	printAt(CGI->preth->zelp[439].first,155,124,GEOR13,zwykly,bitmap);
	itoa(c->hitPoints,pom,10);
	printToWR(pom,276,137,GEOR13,zwykly,bitmap);

	//remaining health - TODO: show during the battles
	//printAt(CGI->preth->zelp[440].first,155,143,GEOR13,zwykly,bitmap);

	//speed
	printAt(CGI->preth->zelp[441].first,155,162,GEOR13,zwykly,bitmap);
	itoa(c->speed,pom,10);
	printToWR(pom,276,175,GEOR13,zwykly,bitmap);


	//luck and morale
	blitAt(LOCPLINT->morale42->ourImages[(State)?(State->morale+3):(3)].bitmap,24,189,bitmap);
	blitAt(LOCPLINT->luck42->ourImages[(State)?(State->luck+3):(3)].bitmap,77,189,bitmap);

	//print abilities text - if r-click popup
	if(type)
	{
		if(Upg)
			upgrade = new AdventureMapButton("",CGI->preth->zelp[446].second,Upg,76,237,"IVIEWCR.DEF");
		if(Dsm)
			dismiss = new AdventureMapButton("",CGI->preth->zelp[445].second,Upg,21,237,"IVIEWCR2.DEF");
	}
	else
	{
		printAtWB(c->abilityText,17,231,GEOR13,35,zwykly,bitmap);
	}
}
CCreInfoWindow::~CCreInfoWindow()
{
	SDL_FreeSurface(bitmap);
	delete anim;
}
void CCreInfoWindow::activate()
{
	ClickableR::activate();
	LOCPLINT->objsToBlit.push_back(this);
	if(ok)
		ok->activate();
	if(dismiss)
		dismiss->activate();
	if(upgrade)
		upgrade->activate();
}
void CCreInfoWindow::close()
{
	deactivate();
	CCastleInterface *c = dynamic_cast<CCastleInterface*>(LOCPLINT->curint);
	if(c) c->showAll();
	if(type)
		LOCPLINT->curint->activate();
	delete this;

}
void CCreInfoWindow::clickRight(boost::logic::tribool down)
{
	if(down)
		return;
	close();
}
void CCreInfoWindow::keyPressed (SDL_KeyboardEvent & key)
{
}
void CCreInfoWindow::deactivate()
{
	ClickableR::deactivate(); 
	LOCPLINT->objsToBlit.erase(std::find(LOCPLINT->objsToBlit.begin(),LOCPLINT->objsToBlit.end(),this));
	if(ok)
		ok->deactivate();
	if(dismiss)
		dismiss->deactivate();
	if(upgrade)
		upgrade->deactivate();
}
