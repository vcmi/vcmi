#include "stdafx.h"
#include "CPlayerInterface.h"
#include "CAdvMapInterface.h"
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
#include "SDL_framerate.h"
#include "hch/CGeneralTextHandler.h"
#include "CCastleInterface.h"
#include "CHeroWindow.h"
#include "timeHandler.h"
using namespace CSDL_Ext;

class OCM_HLP_CGIN
{
public:
	bool operator ()(const std::pair<CGObjectInstance*,std::pair<SDL_Rect, std::vector<std::list<int3>>>>  & a, const std::pair<CGObjectInstance*,std::pair<SDL_Rect, std::vector<std::list<int3>>>> & b) const
	{
		return (*a.first)<(*b.first);
	}
} ocmptwo_cgin ;

void CGarrisonSlot::hover (bool on)
{
}
void CGarrisonSlot::clickRight (tribool down)
{
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
		if(owner->highlighted)
		{
			LOCPLINT->cb->swapCreatures(
				(!upg)?(owner->set1):(owner->set2),
				(!owner->highlighted->upg)?(owner->set1):(owner->set2),
				ID,owner->highlighted->ID);
			owner->highlighted = NULL;
			owner->recreateSlots();
		}
		else
		{
			owner->highlighted = this;
		}
	}
}
void CGarrisonSlot::activate()
{
	ClickableL::activate();
	ClickableR::activate();
	Hoverable::activate();
}
void CGarrisonSlot::deactivate()
{
	ClickableL::deactivate();
	ClickableR::deactivate();
	Hoverable::deactivate();
}
CGarrisonSlot::CGarrisonSlot(CGarrisonInt *Owner, int x, int y, int IID, const CCreature * Creature, int Count)
{
	upg = 0;
	count = Count;
	ID = IID;
	creature = Creature;
	pos.x = x;
	pos.y = y;
	pos.w = 58;
	pos.h = 64;
	owner = Owner;
}
void CGarrisonSlot::show()
{
	if(creature)
	{
		blitAtWR(CGI->creh->bigImgs[creature->idNumber],pos);
	}
	else
	{
		SDL_Rect jakis1 = genRect(pos.w,pos.h,owner->offx+ID*(pos.w+owner->interx),owner->offy+upg*(pos.h+owner->intery)), jakis2 = pos;
		SDL_BlitSurface(owner->sur,&jakis1,ekran,&jakis2);
		SDL_UpdateRect(ekran,pos.x,pos.y,pos.w,pos.h);
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
	if(highlighted)
	{
		blitAt(CGI->creh->bigImgs[-1],highlighted->pos);
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
				new CGarrisonSlot(this, pos.x + (i->first*(58+interx)), pos.y,i->first, i->second.first,i->second.second);
		}
		for(int i=0; i<sup->size(); i++)
			if((*sup)[i] == NULL)
				(*sup)[i] = new CGarrisonSlot(this, pos.x + (i*(58+interx)), pos.y,i, NULL, 0);
	}
	if(set2)
	{	
		sdown = new std::vector<CGarrisonSlot*>(7,(CGarrisonSlot *)(NULL));
		for
			(std::map<int,std::pair<CCreature*,int> >::const_iterator i=set2->slots.begin();
			i!=set2->slots.end(); i++)
		{
			(*sdown)[i->first] = 
				new CGarrisonSlot(this, pos.x + (i->first*(58+interx)), pos.y + 64 + intery,i->first, i->second.first,i->second.second);
		}
		for(int i=0; i<sup->size(); i++)
			if((*sdown)[i] == NULL)
				(*sdown)[i] = new CGarrisonSlot(this, pos.x + (i*(58+interx)), pos.y,i, NULL, 0);
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
	deactiveteSlots();
	deleteSlots();
	createSlots();
	activeteSlots();
	show();
}
CGarrisonInt::CGarrisonInt(int x, int y, int inx, int iny, SDL_Surface *pomsur, int OX, int OY, const CCreatureSet * s1, const CCreatureSet *s2)
	:interx(inx),intery(iny),sur(pomsur),highlighted(NULL),sup(NULL),sdown(NULL),set1(s1),set2(s2),
	offx(OX),offy(OY)
{
	
	ignoreEvent = false;
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
	myBitmap = CSDL_Ext::newSurface(symb->w+2,symb->h+2,ekran);
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
		border = CSDL_Ext::newSurface(symb->w+2,symb->h+2,ekran);
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
		to=ekran;
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
	ourObj=NULL;
	state=0;
}
CButtonBase::~CButtonBase()
{
	for(int i =0; i<imgs.size();i++)
		for(int j=0;j<imgs[i].size();j++)
			SDL_FreeSurface(imgs[i][j]);
}
void CButtonBase::show(SDL_Surface * to)
{
	if(!to)
		to=ekran;
	if (abs)
	{
		blitAt(imgs[curimg][state+bitmapOffset],pos.x,pos.y,to);
		//updateRect(&pos,to);
	}
	else
	{
		blitAt(imgs[curimg][state+bitmapOffset],pos.x+ourObj->pos.x,pos.y+ourObj->pos.y,to);
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
}
void CPlayerInterface::yourTurn()
{
	makingTurn = true;
	CGI->localPlayer = serialID;
	unsigned char & animVal = LOCPLINT->adventureInt->anim; //for animations handling
	unsigned char & heroAnimVal = LOCPLINT->adventureInt->heroAnim;
	adventureInt->infoBar.newDay(cb->getDate(1));
	adventureInt->show();
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
		//SDL_Flip(ekran);
		CSDL_Ext::update(ekran);
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
			hlp.objects[h].second.first = r;
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
		CGI->mh->ttiles[hp.x-3][hp.y-2][hp.z].objects.push_back(std::make_pair(ho, std::make_pair(genRect(32, 32, -31, -31), std::vector<std::list<int3>>())));
		CGI->mh->ttiles[hp.x-2][hp.y-2][hp.z].objects.push_back(std::make_pair(ho, std::make_pair(genRect(32, 32, 1, -31), std::vector<std::list<int3>>())));
		CGI->mh->ttiles[hp.x-1][hp.y-2][hp.z].objects.push_back(std::make_pair(ho, std::make_pair(genRect(32, 32, 33, -31), std::vector<std::list<int3>>())));
		CGI->mh->ttiles[hp.x][hp.y-2][hp.z].objects.push_back(std::make_pair(ho, std::make_pair(genRect(32, 32, 65, -31), std::vector<std::list<int3>>())));

		CGI->mh->ttiles[hp.x-3][hp.y-1][hp.z].objects.push_back(std::make_pair(ho, std::make_pair(genRect(32, 32, -31, 1), std::vector<std::list<int3>>())));
		subRect(hp.x-2, hp.y-1, hp.z, genRect(32, 32, 1, 1), ho->id);
		subRect(hp.x-1, hp.y-1, hp.z, genRect(32, 32, 33, 1), ho->id);
		subRect(hp.x, hp.y-1, hp.z, genRect(32, 32, 65, 1), ho->id);

		CGI->mh->ttiles[hp.x-3][hp.y][hp.z].objects.push_back(std::make_pair(ho, std::make_pair(genRect(32, 32, -31, 33), std::vector<std::list<int3>>())));
		subRect(hp.x-2, hp.y, hp.z, genRect(32, 32, 1, 33), ho->id);
		subRect(hp.x-1, hp.y, hp.z, genRect(32, 32, 33, 33), ho->id);
		subRect(hp.x, hp.y, hp.z, genRect(32, 32, 65, 33), ho->id);

		std::stable_sort(CGI->mh->ttiles[hp.x-3][hp.y-2][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-3][hp.y-2][hp.z].objects.end(), ocmptwo_cgin);
		std::stable_sort(CGI->mh->ttiles[hp.x-2][hp.y-2][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-2][hp.y-2][hp.z].objects.end(), ocmptwo_cgin);
		std::stable_sort(CGI->mh->ttiles[hp.x-1][hp.y-2][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-1][hp.y-2][hp.z].objects.end(), ocmptwo_cgin);
		std::stable_sort(CGI->mh->ttiles[hp.x][hp.y-2][hp.z].objects.begin(), CGI->mh->ttiles[hp.x][hp.y-2][hp.z].objects.end(), ocmptwo_cgin);

		std::stable_sort(CGI->mh->ttiles[hp.x-3][hp.y-1][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-3][hp.y-1][hp.z].objects.end(), ocmptwo_cgin);
		//std::stable_sort(CGI->mh->ttiles[hp.x-2][hp.y-1][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-2][hp.y-1][hp.z].objects.end(), ocmptwo_cgin);
		//std::stable_sort(CGI->mh->ttiles[hp.x-1][hp.y-1][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-1][hp.y-1][hp.z].objects.end(), ocmptwo_cgin);
		//std::stable_sort(CGI->mh->ttiles[hp.x][hp.y-1][hp.z].objects.begin(), CGI->mh->ttiles[hp.x][hp.y-1][hp.z].objects.end(), ocmptwo_cgin);

		std::stable_sort(CGI->mh->ttiles[hp.x-3][hp.y][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-3][hp.y][hp.z].objects.end(), ocmptwo_cgin);
		//std::stable_sort(CGI->mh->ttiles[hp.x-2][hp.y][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-2][hp.y][hp.z].objects.end(), ocmptwo_cgin);
		//std::stable_sort(CGI->mh->ttiles[hp.x-1][hp.y][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-1][hp.y][hp.z].objects.end(), ocmptwo_cgin);
		//std::stable_sort(CGI->mh->ttiles[hp.x][hp.y][hp.z].objects.begin(), CGI->mh->ttiles[hp.x][hp.y][hp.z].objects.end(), ocmptwo_cgin);
	}
	else if(details.dst.x == details.src.x && details.dst.y+1 == details.src.y) //t
	{
		ho->moveDir = 2;
		ho->isStanding = false;
		CGI->mh->ttiles[hp.x-2][hp.y-2][hp.z].objects.push_back(std::make_pair(ho, std::make_pair(genRect(32, 32, 0, -31), std::vector<std::list<int3>>())));
		CGI->mh->ttiles[hp.x-1][hp.y-2][hp.z].objects.push_back(std::make_pair(ho, std::make_pair(genRect(32, 32, 32, -31), std::vector<std::list<int3>>())));
		CGI->mh->ttiles[hp.x][hp.y-2][hp.z].objects.push_back(std::make_pair(ho, std::make_pair(genRect(32, 32, 64, -31), std::vector<std::list<int3>>())));

		subRect(hp.x-2, hp.y-1, hp.z, genRect(32, 32, 0, 1), ho->id);
		subRect(hp.x-1, hp.y-1, hp.z, genRect(32, 32, 32, 1), ho->id);
		subRect(hp.x, hp.y-1, hp.z, genRect(32, 32, 64, 1), ho->id);

		subRect(hp.x-2, hp.y, hp.z, genRect(32, 32, 0, 33), ho->id);
		subRect(hp.x-1, hp.y, hp.z, genRect(32, 32, 32, 33), ho->id);
		subRect(hp.x, hp.y, hp.z, genRect(32, 32, 64, 33), ho->id);

		std::stable_sort(CGI->mh->ttiles[hp.x-2][hp.y-2][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-2][hp.y-2][hp.z].objects.end(), ocmptwo_cgin);
		std::stable_sort(CGI->mh->ttiles[hp.x-1][hp.y-2][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-1][hp.y-2][hp.z].objects.end(), ocmptwo_cgin);
		std::stable_sort(CGI->mh->ttiles[hp.x][hp.y-2][hp.z].objects.begin(), CGI->mh->ttiles[hp.x][hp.y-2][hp.z].objects.end(), ocmptwo_cgin);

		//std::stable_sort(CGI->mh->ttiles[hp.x-2][hp.y-1][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-2][hp.y-1][hp.z].objects.end(), ocmptwo_cgin);
		//std::stable_sort(CGI->mh->ttiles[hp.x-1][hp.y-1][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-1][hp.y-1][hp.z].objects.end(), ocmptwo_cgin);
		//std::stable_sort(CGI->mh->ttiles[hp.x][hp.y-1][hp.z].objects.begin(), CGI->mh->ttiles[hp.x][hp.y-1][hp.z].objects.end(), ocmptwo_cgin);

		//std::stable_sort(CGI->mh->ttiles[hp.x-2][hp.y][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-2][hp.y][hp.z].objects.end(), ocmptwo_cgin);
		//std::stable_sort(CGI->mh->ttiles[hp.x-1][hp.y][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-1][hp.y][hp.z].objects.end(), ocmptwo_cgin);
		//std::stable_sort(CGI->mh->ttiles[hp.x][hp.y][hp.z].objects.begin(), CGI->mh->ttiles[hp.x][hp.y][hp.z].objects.end(), ocmptwo_cgin);
	}
	else if(details.dst.x-1 == details.src.x && details.dst.y+1 == details.src.y) //tr
	{
		ho->moveDir = 3;
		ho->isStanding = false;
		CGI->mh->ttiles[hp.x-2][hp.y-2][hp.z].objects.push_back(std::make_pair(ho, std::make_pair(genRect(32, 32, -1, -31), std::vector<std::list<int3>>())));
		CGI->mh->ttiles[hp.x-1][hp.y-2][hp.z].objects.push_back(std::make_pair(ho, std::make_pair(genRect(32, 32, 31, -31), std::vector<std::list<int3>>())));
		CGI->mh->ttiles[hp.x][hp.y-2][hp.z].objects.push_back(std::make_pair(ho, std::make_pair(genRect(32, 32, 63, -31), std::vector<std::list<int3>>())));
		CGI->mh->ttiles[hp.x+1][hp.y-2][hp.z].objects.push_back(std::make_pair(ho, std::make_pair(genRect(32, 32, 95, -31), std::vector<std::list<int3>>())));

		subRect(hp.x-2, hp.y-1, hp.z, genRect(32, 32, -1, 1), ho->id);
		subRect(hp.x-1, hp.y-1, hp.z, genRect(32, 32, 31, 1), ho->id);
		subRect(hp.x, hp.y-1, hp.z, genRect(32, 32, 63, 1), ho->id);
		CGI->mh->ttiles[hp.x+1][hp.y-1][hp.z].objects.push_back(std::make_pair(ho, std::make_pair(genRect(32, 32, 95, 1), std::vector<std::list<int3>>())));

		subRect(hp.x-2, hp.y, hp.z, genRect(32, 32, -1, 33), ho->id);
		subRect(hp.x-1, hp.y, hp.z, genRect(32, 32, 31, 33), ho->id);
		subRect(hp.x, hp.y, hp.z, genRect(32, 32, 63, 33), ho->id);
		CGI->mh->ttiles[hp.x+1][hp.y][hp.z].objects.push_back(std::make_pair(ho, std::make_pair(genRect(32, 32, 95, 33), std::vector<std::list<int3>>())));

		std::stable_sort(CGI->mh->ttiles[hp.x-2][hp.y-2][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-2][hp.y-2][hp.z].objects.end(), ocmptwo_cgin);
		std::stable_sort(CGI->mh->ttiles[hp.x-1][hp.y-2][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-1][hp.y-2][hp.z].objects.end(), ocmptwo_cgin);
		std::stable_sort(CGI->mh->ttiles[hp.x][hp.y-2][hp.z].objects.begin(), CGI->mh->ttiles[hp.x][hp.y-2][hp.z].objects.end(), ocmptwo_cgin);
		std::stable_sort(CGI->mh->ttiles[hp.x+1][hp.y-2][hp.z].objects.begin(), CGI->mh->ttiles[hp.x+1][hp.y-2][hp.z].objects.end(), ocmptwo_cgin);

		//std::stable_sort(CGI->mh->ttiles[hp.x-2][hp.y-1][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-2][hp.y-1][hp.z].objects.end(), ocmptwo_cgin);
		//std::stable_sort(CGI->mh->ttiles[hp.x-1][hp.y-1][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-1][hp.y-1][hp.z].objects.end(), ocmptwo_cgin);
		//std::stable_sort(CGI->mh->ttiles[hp.x][hp.y-1][hp.z].objects.begin(), CGI->mh->ttiles[hp.x][hp.y-1][hp.z].objects.end(), ocmptwo_cgin);
		std::stable_sort(CGI->mh->ttiles[hp.x+1][hp.y-1][hp.z].objects.begin(), CGI->mh->ttiles[hp.x+1][hp.y-1][hp.z].objects.end(), ocmptwo_cgin);

		//std::stable_sort(CGI->mh->ttiles[hp.x-2][hp.y][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-2][hp.y][hp.z].objects.end(), ocmptwo_cgin);
		//std::stable_sort(CGI->mh->ttiles[hp.x-1][hp.y][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-1][hp.y][hp.z].objects.end(), ocmptwo_cgin);
		//std::stable_sort(CGI->mh->ttiles[hp.x][hp.y][hp.z].objects.begin(), CGI->mh->ttiles[hp.x][hp.y][hp.z].objects.end(), ocmptwo_cgin);
		std::stable_sort(CGI->mh->ttiles[hp.x+1][hp.y][hp.z].objects.begin(), CGI->mh->ttiles[hp.x+1][hp.y][hp.z].objects.end(), ocmptwo_cgin);
	}
	else if(details.dst.x-1 == details.src.x && details.dst.y == details.src.y) //r
	{
		ho->moveDir = 4;
		ho->isStanding = false;
		subRect(hp.x-2, hp.y-1, hp.z, genRect(32, 32, -1, 0), ho->id);
		subRect(hp.x-1, hp.y-1, hp.z, genRect(32, 32, 31, 0), ho->id);
		subRect(hp.x, hp.y-1, hp.z, genRect(32, 32, 63, 0), ho->id);
		CGI->mh->ttiles[hp.x+1][hp.y-1][hp.z].objects.push_back(std::make_pair(ho, std::make_pair(genRect(32, 32, 95, 0), std::vector<std::list<int3>>())));

		subRect(hp.x-2, hp.y, hp.z, genRect(32, 32, -1, 32), ho->id);
		subRect(hp.x-1, hp.y, hp.z, genRect(32, 32, 31, 32), ho->id);
		subRect(hp.x, hp.y, hp.z, genRect(32, 32, 63, 32), ho->id);
		CGI->mh->ttiles[hp.x+1][hp.y][hp.z].objects.push_back(std::make_pair(ho, std::make_pair(genRect(32, 32, 95, 32), std::vector<std::list<int3>>())));

		//std::stable_sort(CGI->mh->ttiles[hp.x-2][hp.y-1][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-2][hp.y-1][hp.z].objects.end(), ocmptwo_cgin);
		//std::stable_sort(CGI->mh->ttiles[hp.x-1][hp.y-1][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-1][hp.y-1][hp.z].objects.end(), ocmptwo_cgin);
		//std::stable_sort(CGI->mh->ttiles[hp.x][hp.y-1][hp.z].objects.begin(), CGI->mh->ttiles[hp.x][hp.y-1][hp.z].objects.end(), ocmptwo_cgin);
		std::stable_sort(CGI->mh->ttiles[hp.x+1][hp.y-1][hp.z].objects.begin(), CGI->mh->ttiles[hp.x+1][hp.y-1][hp.z].objects.end(), ocmptwo_cgin);

		//std::stable_sort(CGI->mh->ttiles[hp.x-2][hp.y][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-2][hp.y][hp.z].objects.end(), ocmptwo_cgin);
		//std::stable_sort(CGI->mh->ttiles[hp.x-1][hp.y][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-1][hp.y][hp.z].objects.end(), ocmptwo_cgin);
		//std::stable_sort(CGI->mh->ttiles[hp.x][hp.y][hp.z].objects.begin(), CGI->mh->ttiles[hp.x][hp.y][hp.z].objects.end(), ocmptwo_cgin);
		std::stable_sort(CGI->mh->ttiles[hp.x+1][hp.y][hp.z].objects.begin(), CGI->mh->ttiles[hp.x+1][hp.y][hp.z].objects.end(), ocmptwo_cgin);
	}
	else if(details.dst.x-1 == details.src.x && details.dst.y-1 == details.src.y) //br
	{
		ho->moveDir = 5;
		ho->isStanding = false;
		subRect(hp.x-2, hp.y-1, hp.z, genRect(32, 32, -1, -1), ho->id);
		subRect(hp.x-1, hp.y-1, hp.z, genRect(32, 32, 31, -1), ho->id);
		subRect(hp.x, hp.y-1, hp.z, genRect(32, 32, 63, -1), ho->id);
		CGI->mh->ttiles[hp.x+1][hp.y-1][hp.z].objects.push_back(std::make_pair(ho, std::make_pair(genRect(32, 32, 95, -1), std::vector<std::list<int3>>())));

		subRect(hp.x-2, hp.y, hp.z, genRect(32, 32, -1, 31), ho->id);
		subRect(hp.x-1, hp.y, hp.z, genRect(32, 32, 31, 31), ho->id);
		subRect(hp.x, hp.y, hp.z, genRect(32, 32, 63, 31), ho->id);
		CGI->mh->ttiles[hp.x+1][hp.y][hp.z].objects.push_back(std::make_pair(ho, std::make_pair(genRect(32, 32, 95, 31), std::vector<std::list<int3>>())));

		CGI->mh->ttiles[hp.x-2][hp.y+1][hp.z].objects.push_back(std::make_pair(ho, std::make_pair(genRect(32, 32, -1, 63), std::vector<std::list<int3>>())));
		CGI->mh->ttiles[hp.x-1][hp.y+1][hp.z].objects.push_back(std::make_pair(ho, std::make_pair(genRect(32, 32, 31, 63), std::vector<std::list<int3>>())));
		CGI->mh->ttiles[hp.x][hp.y+1][hp.z].objects.push_back(std::make_pair(ho, std::make_pair(genRect(32, 32, 63, 63), std::vector<std::list<int3>>())));
		CGI->mh->ttiles[hp.x+1][hp.y+1][hp.z].objects.push_back(std::make_pair(ho, std::make_pair(genRect(32, 32, 95, 63), std::vector<std::list<int3>>())));

		//std::stable_sort(CGI->mh->ttiles[hp.x-2][hp.y-1][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-2][hp.y-1][hp.z].objects.end(), ocmptwo_cgin);
		//std::stable_sort(CGI->mh->ttiles[hp.x-1][hp.y-1][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-1][hp.y-1][hp.z].objects.end(), ocmptwo_cgin);
		//std::stable_sort(CGI->mh->ttiles[hp.x][hp.y-1][hp.z].objects.begin(), CGI->mh->ttiles[hp.x][hp.y-1][hp.z].objects.end(), ocmptwo_cgin);
		std::stable_sort(CGI->mh->ttiles[hp.x+1][hp.y-1][hp.z].objects.begin(), CGI->mh->ttiles[hp.x+1][hp.y-1][hp.z].objects.end(), ocmptwo_cgin);

		//std::stable_sort(CGI->mh->ttiles[hp.x-2][hp.y][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-2][hp.y][hp.z].objects.end(), ocmptwo_cgin);
		//std::stable_sort(CGI->mh->ttiles[hp.x-1][hp.y][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-1][hp.y][hp.z].objects.end(), ocmptwo_cgin);
		//std::stable_sort(CGI->mh->ttiles[hp.x][hp.y][hp.z].objects.begin(), CGI->mh->ttiles[hp.x][hp.y][hp.z].objects.end(), ocmptwo_cgin);
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

		CGI->mh->ttiles[hp.x-2][hp.y+1][hp.z].objects.push_back(std::make_pair(ho, std::make_pair(genRect(32, 32, 0, 63), std::vector<std::list<int3>>())));
		CGI->mh->ttiles[hp.x-1][hp.y+1][hp.z].objects.push_back(std::make_pair(ho, std::make_pair(genRect(32, 32, 32, 63), std::vector<std::list<int3>>())));
		CGI->mh->ttiles[hp.x][hp.y+1][hp.z].objects.push_back(std::make_pair(ho, std::make_pair(genRect(32, 32, 64, 63), std::vector<std::list<int3>>())));

		//std::stable_sort(CGI->mh->ttiles[hp.x-2][hp.y-1][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-2][hp.y-1][hp.z].objects.end(), ocmptwo_cgin);
		//std::stable_sort(CGI->mh->ttiles[hp.x-1][hp.y-1][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-1][hp.y-1][hp.z].objects.end(), ocmptwo_cgin);
		//std::stable_sort(CGI->mh->ttiles[hp.x][hp.y-1][hp.z].objects.begin(), CGI->mh->ttiles[hp.x][hp.y-1][hp.z].objects.end(), ocmptwo_cgin);

		//std::stable_sort(CGI->mh->ttiles[hp.x-2][hp.y][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-2][hp.y][hp.z].objects.end(), ocmptwo_cgin);
		//std::stable_sort(CGI->mh->ttiles[hp.x-1][hp.y][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-1][hp.y][hp.z].objects.end(), ocmptwo_cgin);
		//std::stable_sort(CGI->mh->ttiles[hp.x][hp.y][hp.z].objects.begin(), CGI->mh->ttiles[hp.x][hp.y][hp.z].objects.end(), ocmptwo_cgin);

		std::stable_sort(CGI->mh->ttiles[hp.x-2][hp.y+1][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-2][hp.y+1][hp.z].objects.end(), ocmptwo_cgin);
		std::stable_sort(CGI->mh->ttiles[hp.x-1][hp.y+1][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-1][hp.y+1][hp.z].objects.end(), ocmptwo_cgin);
		std::stable_sort(CGI->mh->ttiles[hp.x][hp.y+1][hp.z].objects.begin(), CGI->mh->ttiles[hp.x][hp.y+1][hp.z].objects.end(), ocmptwo_cgin);
	}
	else if(details.dst.x+1 == details.src.x && details.dst.y-1 == details.src.y) //bl
	{
		ho->moveDir = 7;
		ho->isStanding = false;
		CGI->mh->ttiles[hp.x-3][hp.y-1][hp.z].objects.push_back(std::make_pair(ho, std::make_pair(genRect(32, 32, -31, -1), std::vector<std::list<int3>>())));
		subRect(hp.x-2, hp.y-1, hp.z, genRect(32, 32, 1, -1), ho->id);
		subRect(hp.x-1, hp.y-1, hp.z, genRect(32, 32, 33, -1), ho->id);
		subRect(hp.x, hp.y-1, hp.z, genRect(32, 32, 65, -1), ho->id);

		CGI->mh->ttiles[hp.x-3][hp.y][hp.z].objects.push_back(std::make_pair(ho, std::make_pair(genRect(32, 32, -31, 31), std::vector<std::list<int3>>())));
		subRect(hp.x-2, hp.y, hp.z, genRect(32, 32, 1, 31), ho->id);
		subRect(hp.x-1, hp.y, hp.z, genRect(32, 32, 33, 31), ho->id);
		subRect(hp.x, hp.y, hp.z, genRect(32, 32, 65, 31), ho->id);

		CGI->mh->ttiles[hp.x-3][hp.y+1][hp.z].objects.push_back(std::make_pair(ho, std::make_pair(genRect(32, 32, -31, 63), std::vector<std::list<int3>>())));
		CGI->mh->ttiles[hp.x-2][hp.y+1][hp.z].objects.push_back(std::make_pair(ho, std::make_pair(genRect(32, 32, 1, 63), std::vector<std::list<int3>>())));
		CGI->mh->ttiles[hp.x-1][hp.y+1][hp.z].objects.push_back(std::make_pair(ho, std::make_pair(genRect(32, 32, 33, 63), std::vector<std::list<int3>>())));
		CGI->mh->ttiles[hp.x][hp.y+1][hp.z].objects.push_back(std::make_pair(ho, std::make_pair(genRect(32, 32, 65, 63), std::vector<std::list<int3>>())));

		std::stable_sort(CGI->mh->ttiles[hp.x-3][hp.y-1][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-3][hp.y-1][hp.z].objects.end(), ocmptwo_cgin);
		//std::stable_sort(CGI->mh->ttiles[hp.x-2][hp.y-1][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-2][hp.y-1][hp.z].objects.end(), ocmptwo_cgin);
		//std::stable_sort(CGI->mh->ttiles[hp.x-1][hp.y-1][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-1][hp.y-1][hp.z].objects.end(), ocmptwo_cgin);
		//std::stable_sort(CGI->mh->ttiles[hp.x][hp.y-1][hp.z].objects.begin(), CGI->mh->ttiles[hp.x][hp.y-1][hp.z].objects.end(), ocmptwo_cgin);

		std::stable_sort(CGI->mh->ttiles[hp.x-3][hp.y][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-3][hp.y][hp.z].objects.end(), ocmptwo_cgin);
		//std::stable_sort(CGI->mh->ttiles[hp.x-2][hp.y][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-2][hp.y][hp.z].objects.end(), ocmptwo_cgin);
		//std::stable_sort(CGI->mh->ttiles[hp.x-1][hp.y][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-1][hp.y][hp.z].objects.end(), ocmptwo_cgin);
		//std::stable_sort(CGI->mh->ttiles[hp.x][hp.y][hp.z].objects.begin(), CGI->mh->ttiles[hp.x][hp.y][hp.z].objects.end(), ocmptwo_cgin);

		std::stable_sort(CGI->mh->ttiles[hp.x-3][hp.y+1][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-3][hp.y+1][hp.z].objects.end(), ocmptwo_cgin);
		std::stable_sort(CGI->mh->ttiles[hp.x-2][hp.y+1][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-2][hp.y+1][hp.z].objects.end(), ocmptwo_cgin);
		std::stable_sort(CGI->mh->ttiles[hp.x-1][hp.y+1][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-1][hp.y+1][hp.z].objects.end(), ocmptwo_cgin);
		std::stable_sort(CGI->mh->ttiles[hp.x][hp.y+1][hp.z].objects.begin(), CGI->mh->ttiles[hp.x][hp.y+1][hp.z].objects.end(), ocmptwo_cgin);
	}
	else if(details.dst.x+1 == details.src.x && details.dst.y == details.src.y) //l
	{
		ho->moveDir = 8;
		ho->isStanding = false;
		CGI->mh->ttiles[hp.x-3][hp.y-1][hp.z].objects.push_back(std::make_pair(ho, std::make_pair(genRect(32, 32, -31, 0), std::vector<std::list<int3>>())));
		subRect(hp.x-2, hp.y-1, hp.z, genRect(32, 32, 1, 0), ho->id);
		subRect(hp.x-1, hp.y-1, hp.z, genRect(32, 32, 33, 0), ho->id);
		subRect(hp.x, hp.y-1, hp.z, genRect(32, 32, 65, 0), ho->id);

		CGI->mh->ttiles[hp.x-3][hp.y][hp.z].objects.push_back(std::make_pair(ho, std::make_pair(genRect(32, 32, -31, 32), std::vector<std::list<int3>>())));
		subRect(hp.x-2, hp.y, hp.z, genRect(32, 32, 1, 32), ho->id);
		subRect(hp.x-1, hp.y, hp.z, genRect(32, 32, 33, 32), ho->id);
		subRect(hp.x, hp.y, hp.z, genRect(32, 32, 65, 32), ho->id);

		std::stable_sort(CGI->mh->ttiles[hp.x-3][hp.y-1][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-3][hp.y-1][hp.z].objects.end(), ocmptwo_cgin);
		//std::stable_sort(CGI->mh->ttiles[hp.x-2][hp.y-1][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-2][hp.y-1][hp.z].objects.end(), ocmptwo_cgin);
		//std::stable_sort(CGI->mh->ttiles[hp.x-1][hp.y-1][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-1][hp.y-1][hp.z].objects.end(), ocmptwo_cgin);
		//std::stable_sort(CGI->mh->ttiles[hp.x][hp.y-1][hp.z].objects.begin(), CGI->mh->ttiles[hp.x][hp.y-1][hp.z].objects.end(), ocmptwo_cgin);

		std::stable_sort(CGI->mh->ttiles[hp.x-3][hp.y][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-3][hp.y][hp.z].objects.end(), ocmptwo_cgin);
		//std::stable_sort(CGI->mh->ttiles[hp.x-2][hp.y][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-2][hp.y][hp.z].objects.end(), ocmptwo_cgin);
		//std::stable_sort(CGI->mh->ttiles[hp.x-1][hp.y][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-1][hp.y][hp.z].objects.end(), ocmptwo_cgin);
		//std::stable_sort(CGI->mh->ttiles[hp.x][hp.y][hp.z].objects.begin(), CGI->mh->ttiles[hp.x][hp.y][hp.z].objects.end(), ocmptwo_cgin);
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

			/*std::stable_sort(CGI->mh->ttiles[hp.x-3][hp.y-2][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-3][hp.y-2][hp.z].objects.end(), ocmptwo_cgin);
			std::stable_sort(CGI->mh->ttiles[hp.x-2][hp.y-2][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-2][hp.y-2][hp.z].objects.end(), ocmptwo_cgin);
			std::stable_sort(CGI->mh->ttiles[hp.x-1][hp.y-2][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-1][hp.y-2][hp.z].objects.end(), ocmptwo_cgin);
			std::stable_sort(CGI->mh->ttiles[hp.x][hp.y-2][hp.z].objects.begin(), CGI->mh->ttiles[hp.x][hp.y-2][hp.z].objects.end(), ocmptwo_cgin);

			std::stable_sort(CGI->mh->ttiles[hp.x-3][hp.y-1][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-3][hp.y-1][hp.z].objects.end(), ocmptwo_cgin);
			std::stable_sort(CGI->mh->ttiles[hp.x-2][hp.y-1][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-2][hp.y-1][hp.z].objects.end(), ocmptwo_cgin);
			std::stable_sort(CGI->mh->ttiles[hp.x-1][hp.y-1][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-1][hp.y-1][hp.z].objects.end(), ocmptwo_cgin);
			std::stable_sort(CGI->mh->ttiles[hp.x][hp.y-1][hp.z].objects.begin(), CGI->mh->ttiles[hp.x][hp.y-1][hp.z].objects.end(), ocmptwo_cgin);

			std::stable_sort(CGI->mh->ttiles[hp.x-3][hp.y][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-3][hp.y][hp.z].objects.end(), ocmptwo_cgin);
			std::stable_sort(CGI->mh->ttiles[hp.x-2][hp.y][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-2][hp.y][hp.z].objects.end(), ocmptwo_cgin);
			std::stable_sort(CGI->mh->ttiles[hp.x-1][hp.y][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-1][hp.y][hp.z].objects.end(), ocmptwo_cgin);
			std::stable_sort(CGI->mh->ttiles[hp.x][hp.y][hp.z].objects.begin(), CGI->mh->ttiles[hp.x][hp.y][hp.z].objects.end(), ocmptwo_cgin);*/
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

			/*std::stable_sort(CGI->mh->ttiles[hp.x-2][hp.y-2][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-2][hp.y-2][hp.z].objects.end(), ocmptwo_cgin);
			std::stable_sort(CGI->mh->ttiles[hp.x-1][hp.y-2][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-1][hp.y-2][hp.z].objects.end(), ocmptwo_cgin);
			std::stable_sort(CGI->mh->ttiles[hp.x][hp.y-2][hp.z].objects.begin(), CGI->mh->ttiles[hp.x][hp.y-2][hp.z].objects.end(), ocmptwo_cgin);

			std::stable_sort(CGI->mh->ttiles[hp.x-2][hp.y-1][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-2][hp.y-1][hp.z].objects.end(), ocmptwo_cgin);
			std::stable_sort(CGI->mh->ttiles[hp.x-1][hp.y-1][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-1][hp.y-1][hp.z].objects.end(), ocmptwo_cgin);
			std::stable_sort(CGI->mh->ttiles[hp.x][hp.y-1][hp.z].objects.begin(), CGI->mh->ttiles[hp.x][hp.y-1][hp.z].objects.end(), ocmptwo_cgin);

			std::stable_sort(CGI->mh->ttiles[hp.x-2][hp.y][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-2][hp.y][hp.z].objects.end(), ocmptwo_cgin);
			std::stable_sort(CGI->mh->ttiles[hp.x-1][hp.y][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-1][hp.y][hp.z].objects.end(), ocmptwo_cgin);
			std::stable_sort(CGI->mh->ttiles[hp.x][hp.y][hp.z].objects.begin(), CGI->mh->ttiles[hp.x][hp.y][hp.z].objects.end(), ocmptwo_cgin);*/
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

			/*std::stable_sort(CGI->mh->ttiles[hp.x-2][hp.y-2][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-2][hp.y-2][hp.z].objects.end(), ocmptwo_cgin);
			std::stable_sort(CGI->mh->ttiles[hp.x-1][hp.y-2][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-1][hp.y-2][hp.z].objects.end(), ocmptwo_cgin);
			std::stable_sort(CGI->mh->ttiles[hp.x][hp.y-2][hp.z].objects.begin(), CGI->mh->ttiles[hp.x][hp.y-2][hp.z].objects.end(), ocmptwo_cgin);
			std::stable_sort(CGI->mh->ttiles[hp.x+1][hp.y-2][hp.z].objects.begin(), CGI->mh->ttiles[hp.x+1][hp.y-2][hp.z].objects.end(), ocmptwo_cgin);

			std::stable_sort(CGI->mh->ttiles[hp.x-2][hp.y-1][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-2][hp.y-1][hp.z].objects.end(), ocmptwo_cgin);
			std::stable_sort(CGI->mh->ttiles[hp.x-1][hp.y-1][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-1][hp.y-1][hp.z].objects.end(), ocmptwo_cgin);
			std::stable_sort(CGI->mh->ttiles[hp.x][hp.y-1][hp.z].objects.begin(), CGI->mh->ttiles[hp.x][hp.y-1][hp.z].objects.end(), ocmptwo_cgin);
			std::stable_sort(CGI->mh->ttiles[hp.x+1][hp.y-1][hp.z].objects.begin(), CGI->mh->ttiles[hp.x+1][hp.y-1][hp.z].objects.end(), ocmptwo_cgin);

			std::stable_sort(CGI->mh->ttiles[hp.x-2][hp.y][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-2][hp.y][hp.z].objects.end(), ocmptwo_cgin);
			std::stable_sort(CGI->mh->ttiles[hp.x-1][hp.y][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-1][hp.y][hp.z].objects.end(), ocmptwo_cgin);
			std::stable_sort(CGI->mh->ttiles[hp.x][hp.y][hp.z].objects.begin(), CGI->mh->ttiles[hp.x][hp.y][hp.z].objects.end(), ocmptwo_cgin);
			std::stable_sort(CGI->mh->ttiles[hp.x+1][hp.y][hp.z].objects.begin(), CGI->mh->ttiles[hp.x+1][hp.y][hp.z].objects.end(), ocmptwo_cgin);*/
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

			/*std::stable_sort(CGI->mh->ttiles[hp.x-2][hp.y-1][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-2][hp.y-1][hp.z].objects.end(), ocmptwo_cgin);
			std::stable_sort(CGI->mh->ttiles[hp.x-1][hp.y-1][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-1][hp.y-1][hp.z].objects.end(), ocmptwo_cgin);
			std::stable_sort(CGI->mh->ttiles[hp.x][hp.y-1][hp.z].objects.begin(), CGI->mh->ttiles[hp.x][hp.y-1][hp.z].objects.end(), ocmptwo_cgin);
			std::stable_sort(CGI->mh->ttiles[hp.x+1][hp.y-1][hp.z].objects.begin(), CGI->mh->ttiles[hp.x+1][hp.y-1][hp.z].objects.end(), ocmptwo_cgin);

			std::stable_sort(CGI->mh->ttiles[hp.x-2][hp.y][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-2][hp.y][hp.z].objects.end(), ocmptwo_cgin);
			std::stable_sort(CGI->mh->ttiles[hp.x-1][hp.y][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-1][hp.y][hp.z].objects.end(), ocmptwo_cgin);
			std::stable_sort(CGI->mh->ttiles[hp.x][hp.y][hp.z].objects.begin(), CGI->mh->ttiles[hp.x][hp.y][hp.z].objects.end(), ocmptwo_cgin);
			std::stable_sort(CGI->mh->ttiles[hp.x+1][hp.y][hp.z].objects.begin(), CGI->mh->ttiles[hp.x+1][hp.y][hp.z].objects.end(), ocmptwo_cgin);*/
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

			/*std::stable_sort(CGI->mh->ttiles[hp.x-2][hp.y-1][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-2][hp.y-1][hp.z].objects.end(), ocmptwo_cgin);
			std::stable_sort(CGI->mh->ttiles[hp.x-1][hp.y-1][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-1][hp.y-1][hp.z].objects.end(), ocmptwo_cgin);
			std::stable_sort(CGI->mh->ttiles[hp.x][hp.y-1][hp.z].objects.begin(), CGI->mh->ttiles[hp.x][hp.y-1][hp.z].objects.end(), ocmptwo_cgin);
			std::stable_sort(CGI->mh->ttiles[hp.x+1][hp.y-1][hp.z].objects.begin(), CGI->mh->ttiles[hp.x+1][hp.y-1][hp.z].objects.end(), ocmptwo_cgin);

			std::stable_sort(CGI->mh->ttiles[hp.x-2][hp.y][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-2][hp.y][hp.z].objects.end(), ocmptwo_cgin);
			std::stable_sort(CGI->mh->ttiles[hp.x-1][hp.y][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-1][hp.y][hp.z].objects.end(), ocmptwo_cgin);
			std::stable_sort(CGI->mh->ttiles[hp.x][hp.y][hp.z].objects.begin(), CGI->mh->ttiles[hp.x][hp.y][hp.z].objects.end(), ocmptwo_cgin);
			std::stable_sort(CGI->mh->ttiles[hp.x+1][hp.y][hp.z].objects.begin(), CGI->mh->ttiles[hp.x+1][hp.y][hp.z].objects.end(), ocmptwo_cgin);

			std::stable_sort(CGI->mh->ttiles[hp.x-2][hp.y+1][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-2][hp.y+1][hp.z].objects.end(), ocmptwo_cgin);
			std::stable_sort(CGI->mh->ttiles[hp.x-1][hp.y+1][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-1][hp.y+1][hp.z].objects.end(), ocmptwo_cgin);
			std::stable_sort(CGI->mh->ttiles[hp.x][hp.y+1][hp.z].objects.begin(), CGI->mh->ttiles[hp.x][hp.y+1][hp.z].objects.end(), ocmptwo_cgin);
			std::stable_sort(CGI->mh->ttiles[hp.x+1][hp.y+1][hp.z].objects.begin(), CGI->mh->ttiles[hp.x+1][hp.y+1][hp.z].objects.end(), ocmptwo_cgin);*/
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

			/*std::stable_sort(CGI->mh->ttiles[hp.x-2][hp.y-1][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-2][hp.y-1][hp.z].objects.end(), ocmptwo_cgin);
			std::stable_sort(CGI->mh->ttiles[hp.x-1][hp.y-1][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-1][hp.y-1][hp.z].objects.end(), ocmptwo_cgin);
			std::stable_sort(CGI->mh->ttiles[hp.x][hp.y-1][hp.z].objects.begin(), CGI->mh->ttiles[hp.x][hp.y-1][hp.z].objects.end(), ocmptwo_cgin);

			std::stable_sort(CGI->mh->ttiles[hp.x-2][hp.y][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-2][hp.y][hp.z].objects.end(), ocmptwo_cgin);
			std::stable_sort(CGI->mh->ttiles[hp.x-1][hp.y][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-1][hp.y][hp.z].objects.end(), ocmptwo_cgin);
			std::stable_sort(CGI->mh->ttiles[hp.x][hp.y][hp.z].objects.begin(), CGI->mh->ttiles[hp.x][hp.y][hp.z].objects.end(), ocmptwo_cgin);

			std::stable_sort(CGI->mh->ttiles[hp.x-2][hp.y+1][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-2][hp.y+1][hp.z].objects.end(), ocmptwo_cgin);
			std::stable_sort(CGI->mh->ttiles[hp.x-1][hp.y+1][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-1][hp.y+1][hp.z].objects.end(), ocmptwo_cgin);
			std::stable_sort(CGI->mh->ttiles[hp.x][hp.y+1][hp.z].objects.begin(), CGI->mh->ttiles[hp.x][hp.y+1][hp.z].objects.end(), ocmptwo_cgin);*/
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

			/*std::stable_sort(CGI->mh->ttiles[hp.x-3][hp.y-1][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-3][hp.y-1][hp.z].objects.end(), ocmptwo_cgin);
			std::stable_sort(CGI->mh->ttiles[hp.x-2][hp.y-1][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-2][hp.y-1][hp.z].objects.end(), ocmptwo_cgin);
			std::stable_sort(CGI->mh->ttiles[hp.x-1][hp.y-1][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-1][hp.y-1][hp.z].objects.end(), ocmptwo_cgin);
			std::stable_sort(CGI->mh->ttiles[hp.x][hp.y-1][hp.z].objects.begin(), CGI->mh->ttiles[hp.x][hp.y-1][hp.z].objects.end(), ocmptwo_cgin);

			std::stable_sort(CGI->mh->ttiles[hp.x-3][hp.y][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-3][hp.y][hp.z].objects.end(), ocmptwo_cgin);
			std::stable_sort(CGI->mh->ttiles[hp.x-2][hp.y][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-2][hp.y][hp.z].objects.end(), ocmptwo_cgin);
			std::stable_sort(CGI->mh->ttiles[hp.x-1][hp.y][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-1][hp.y][hp.z].objects.end(), ocmptwo_cgin);
			std::stable_sort(CGI->mh->ttiles[hp.x][hp.y][hp.z].objects.begin(), CGI->mh->ttiles[hp.x][hp.y][hp.z].objects.end(), ocmptwo_cgin);

			std::stable_sort(CGI->mh->ttiles[hp.x-3][hp.y+1][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-3][hp.y+1][hp.z].objects.end(), ocmptwo_cgin);
			std::stable_sort(CGI->mh->ttiles[hp.x-2][hp.y+1][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-2][hp.y+1][hp.z].objects.end(), ocmptwo_cgin);
			std::stable_sort(CGI->mh->ttiles[hp.x-1][hp.y+1][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-1][hp.y+1][hp.z].objects.end(), ocmptwo_cgin);
			std::stable_sort(CGI->mh->ttiles[hp.x][hp.y+1][hp.z].objects.begin(), CGI->mh->ttiles[hp.x][hp.y+1][hp.z].objects.end(), ocmptwo_cgin);*/
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

			/*std::stable_sort(CGI->mh->ttiles[hp.x-3][hp.y-1][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-3][hp.y-1][hp.z].objects.end(), ocmptwo_cgin);
			std::stable_sort(CGI->mh->ttiles[hp.x-2][hp.y-1][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-2][hp.y-1][hp.z].objects.end(), ocmptwo_cgin);
			std::stable_sort(CGI->mh->ttiles[hp.x-1][hp.y-1][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-1][hp.y-1][hp.z].objects.end(), ocmptwo_cgin);
			std::stable_sort(CGI->mh->ttiles[hp.x][hp.y-1][hp.z].objects.begin(), CGI->mh->ttiles[hp.x][hp.y-1][hp.z].objects.end(), ocmptwo_cgin);

			std::stable_sort(CGI->mh->ttiles[hp.x-3][hp.y][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-3][hp.y][hp.z].objects.end(), ocmptwo_cgin);
			std::stable_sort(CGI->mh->ttiles[hp.x-2][hp.y][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-2][hp.y][hp.z].objects.end(), ocmptwo_cgin);
			std::stable_sort(CGI->mh->ttiles[hp.x-1][hp.y][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-1][hp.y][hp.z].objects.end(), ocmptwo_cgin);
			std::stable_sort(CGI->mh->ttiles[hp.x][hp.y][hp.z].objects.begin(), CGI->mh->ttiles[hp.x][hp.y][hp.z].objects.end(), ocmptwo_cgin);*/
		}
		LOCPLINT->adventureInt->update(); //updating screen
		CSDL_Ext::update(ekran);
		CGI->screenh->updateScreen();

		++LOCPLINT->adventureInt->animValHitCount; //for animations
		if(LOCPLINT->adventureInt->animValHitCount == 4)
		{
			LOCPLINT->adventureInt->animValHitCount = 0;
			++LOCPLINT->adventureInt->anim;
			LOCPLINT->adventureInt->updateScreen = true;
		}
		++LOCPLINT->adventureInt->heroAnim;

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
	SDL_Surface * ret = copySurface(hInfo);
	SDL_SetColorKey(ret,SDL_SRCCOLORKEY,SDL_MapRGB(ret->format,0,255,255));
	blueToPlayersAdv(ret,playerID,1);
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
	return NULL;
}

void CPlayerInterface::openTownWindow(const CGTownInstance * town)
{
	adventureInt->hide();
	timeHandler t;
	t.getDif();
	castleInt = new CCastleInterface(town,true);
	std::cout << "Loading town screen: " << t.getDif() <<std::endl;
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
		if (isItIn(&motioninterested[i]->pos,sEvent->motion.x,sEvent->motion.y))
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
	if(sEvent->motion.x>ekran->w-15)
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
	if(sEvent->motion.y>ekran->h-15)
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