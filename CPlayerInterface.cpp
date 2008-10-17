#include "stdafx.h"
#include "CAdvmapInterface.h"
#include "CBattleInterface.h"
#include "CCallback.h"
#include "CCastleInterface.h"
#include "CCursorHandler.h"
#include "CGameInfo.h"
#include "CHeroWindow.h"
#include "CMessage.h"
#include "CPathfinder.h"
#include "CPlayerInterface.h"
#include "SDL_Extensions.h"
#include "SDL_Extensions.h"
#include "SDL_framerate.h"
#include "SDL_framerate.h"
#include "client/CCreatureAnimation.h"
#include "client/Graphics.h"
#include "hch/CAbilityHandler.h"
#include "hch/CArtHandler.h"
#include "hch/CGeneralTextHandler.h"
#include "hch/CHeroHandler.h"
#include "hch/CLodHandler.h"
#include "hch/CObjectHandler.h"
#include "hch/CPreGameTextHandler.h"
#include "hch/CSpellHandler.h"
#include "hch/CTownHandler.h"
#include "lib/CondSh.h"
#include "lib/NetPacks.h"
#include "map.h"
#include "mapHandler.h"
#include "timeHandler.h"
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/assign/std/vector.hpp> 
#include <boost/assign/list_of.hpp>
#include <boost/thread.hpp>
#include <cmath>
#include <queue>
#include <sstream>
using namespace boost::assign;
using namespace CSDL_Ext;

extern TTF_Font * GEOR16;
CPlayerInterface * LOCPLINT;
extern std::queue<SDL_Event> events;
extern boost::mutex eventsM;


class OCM_HLP_CGIN
{
public:
	bool operator ()(const std::pair<const CGObjectInstance*,SDL_Rect>  & a, const std::pair<const CGObjectInstance*,SDL_Rect> & b) const
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
					tlog2 << "Warning - shouldn't be - highlighted void slot "<<owner->highlighted<<std::endl;
					tlog2 << "Highlighted set to NULL"<<std::endl;
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

const CArmedInstance * CGarrisonSlot::getObj()
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
			pom->currentHealth = 0;
			pom->attackBonus = h->getPrimSkillLevel(0);
			pom->defenseBonus = h->getPrimSkillLevel(1);
			pom->luck = h->getCurrentLuck();
			pom->morale = h->getCurrentMorale();
		}
		(new CCreInfoWindow(creature->idNumber,0,count,pom,boost::function<void()>(),boost::function<void()>(),NULL))
				->activate();
		//LOCPLINT->curint->deactivate();
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
				UpgradeInfo pom = LOCPLINT->cb->getUpgradeInfo(getObj(),ID);
				if(pom.oldID>=0)
				{
					(new CCreInfoWindow
						(creature->idNumber,1,count,NULL,
						boost::bind(&CCallback::upgradeCreature,LOCPLINT->cb,getObj(),ID,pom.newID[0]), //if upgrade is possible we'll bind proper function in callback
						 boost::bind(&CCallback::dismissCreature,LOCPLINT->cb,getObj(),ID),&pom))
							->activate();
				}
				else
				{
					(new CCreInfoWindow
						(creature->idNumber,1,count,NULL,0, boost::bind(&CCallback::dismissCreature,LOCPLINT->cb,getObj(),ID),NULL) )
						->activate();
				}
				if(LOCPLINT->curint->subInt)
					LOCPLINT->curint->subInt->deactivate();
				else
					LOCPLINT->curint->deactivate();
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
		SDL_itoa(count,buf,10);
		blitAt(graphics->bigImgs[creature->idNumber],pos);
		printToWR(buf,pos.x+56,pos.y+62,GEOR16,zwykly);
		if(owner->highlighted==this)
			blitAt(graphics->bigImgs[-1],pos);
		//if(owner->update)
		//	updateRect(&pos,screen);
		delete [] buf;
	}
	else
	{
		SDL_Rect jakis1 = genRect(pos.h,pos.w,owner->offx+ID*(pos.w+owner->interx),owner->offy+upg*(pos.h+owner->intery)), jakis2 = pos;
		SDL_BlitSurface(owner->sur,&jakis1,screen,&jakis2);
		if(owner->splitting)
			blitAt(graphics->bigImgs[-1],pos);
		//if(owner->update)
		//	SDL_UpdateRect(screen,pos.x,pos.y,pos.w,pos.h);
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
			(std::map<si32,std::pair<ui32,si32> >::const_iterator i=set1->slots.begin();
			i!=set1->slots.end(); i++)
		{
			(*sup)[i->first] =
				new CGarrisonSlot(this, pos.x + (i->first*(58+interx)), pos.y,i->first, 0, &CGI->creh->creatures[i->second.first],i->second.second);
		}
		for(int i=0; i<sup->size(); i++)
			if((*sup)[i] == NULL)
				(*sup)[i] = new CGarrisonSlot(this, pos.x + (i*(58+interx)), pos.y,i,0,NULL, 0);
	}
	if(set2)
	{
		sdown = new std::vector<CGarrisonSlot*>(7,(CGarrisonSlot *)(NULL));
		for
			(std::map<si32,std::pair<ui32,si32> >::const_iterator i=set2->slots.begin();
			i!=set2->slots.end(); i++)
		{
			(*sdown)[i->first] =
				new CGarrisonSlot(this, pos.x + (i->first*(58+interx)), pos.y + 64 + intery,i->first,1, &CGI->creh->creatures[i->second.first],i->second.second);
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
		delete sup;
		sup = NULL;
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
		delete sdown;
		sdown = NULL;
	}
}
void CGarrisonInt::recreateSlots()
{
	splitting = false;
	highlighted = NULL;
	if(active)
	{
		deactiveteSlots();
	}
	deleteSlots();
	createSlots();
	if(active)
	{
		//ignoreEvent = true;
		activeteSlots();
		show();
	}
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
CGarrisonInt::CGarrisonInt(int x, int y, int inx, int iny, SDL_Surface *pomsur, int OX, int OY, const CArmedInstance *s1, const CArmedInstance *s2)
	:interx(inx),intery(iny),sur(pomsur),highlighted(NULL),sup(NULL),sdown(NULL),oup(s1),odown(s2),
	offx(OX),offy(OY)
{
	active = false;
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
	active = true;
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
	active = false;
	deactiveteSlots();
}

CInfoWindow::CInfoWindow(std::string text, int player, int charperline, const std::vector<SComponent*> &comps, std::vector<std::pair<std::string,CFunctionList<void()> > > &Buttons)
{
	for(int i=0;i<Buttons.size();i++)
	{
		buttons.push_back(new AdventureMapButton("","",Buttons[i].second,0,0,Buttons[i].first));
		if(!Buttons[i].second) //if no function, then by default we'll set it to close
			buttons[i]->callback += boost::bind(&CInfoWindow::close,this);
	}
	for(int i=0;i<comps.size();i++)
	{
		components.push_back(comps[i]);
	}
	CMessage::drawIWindow(this,text,player,charperline);
}
CInfoWindow::CInfoWindow()
{
}
void CInfoWindow::close()
{
	deactivate();
	delete this;
	LOCPLINT->showingDialog->setn(false);
}
void CInfoWindow::show(SDL_Surface * to)
{
	CSimpleWindow::show();
	for(int i=0;i<buttons.size();i++)
		buttons[i]->show();
}

CInfoWindow::~CInfoWindow()
{
	if(delComps)
	{
		for (int i=0;i<components.size();i++)
			delete components[i];
	}
	for(int i=0;i<buttons.size();i++)
		delete buttons[i];
}
void CInfoWindow::activate()
{
	for (int i=0;i<components.size();i++)
		components[i]->activate();
	for(int i=0;i<buttons.size();i++)
		buttons[i]->activate();
}
void CInfoWindow::deactivate()
{
	for (int i=0;i<components.size();i++)
		components[i]->deactivate();
	for(int i=0;i<buttons.size();i++)
		buttons[i]->deactivate();
	LOCPLINT->removeObjToBlit(this);
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
	else if((LOCPLINT->curint == LOCPLINT->castleInt) && !LOCPLINT->castleInt->subInt)
		LOCPLINT->castleInt->showAll();
}
void CInfoPopup::show(SDL_Surface * to)
{
	blitAt(bitmap,pos.x,pos.y,(to)?(to):(screen));
}

void SComponent::init(Etype Type, int Subtype, int Val)
{
	std::ostringstream oss;
	switch (Type)
	{
	case artifact:
		description = CGI->arth->artifacts[Subtype].description;
		subtitle = CGI->arth->artifacts[Subtype].name;
		break;
	case primskill:
		description = CGI->generaltexth->arraytxt[2+Subtype];
		oss << ((Val>0)?("+"):("-")) << Val << " " << CGI->heroh->pskillsn[Subtype];
		subtitle = oss.str();
		break;
	case secskill44:
		subtitle += CGI->abilh->levels[Val-1] + " " + CGI->abilh->abilities[Subtype]->name;
		description = CGI->abilh->abilities[Subtype]->infoTexts[Val-1];
		break;
	case resource:
		description = CGI->generaltexth->allTexts[242];
		oss << Val;
		subtitle = oss.str();
		break;
	case spell:
		description = CGI->spellh->spells[Subtype].descriptions[Val];
		subtitle = CGI->spellh->spells[Subtype].name;
		break;
	case experience:
		description = CGI->generaltexth->allTexts[241];
		oss << Val ;
		if(Subtype && Val==1)
		{
			subtitle = CGI->generaltexth->allTexts[442];
		}
		else
		{
			subtitle = oss.str();
		}
		break;
	}
	type = Type;
	subtype = Subtype;
	val = Val;
	SDL_Surface * temp = this->getImg();
	pos.w = temp->w;
	pos.h = temp->h;
}
SComponent::SComponent(Etype Type, int Subtype, int Val)
{
	init(Type,Subtype,Val);
}

SComponent::SComponent(const Component &c)
{
	if(c.id==5)
		init(experience,c.subtype,c.val);
	else
		init((Etype)c.id,c.subtype,c.val);

	if(c.id==2 && c.when==-1)
		subtitle += CGI->generaltexth->allTexts[3].substr(2,CGI->generaltexth->allTexts[3].length()-2);
}
void SComponent::show(SDL_Surface * to)
{
	blitAt(getImg(),pos.x,pos.y,to);
}
SDL_Surface * SComponent::getImg()
{
	switch (type)
	{
	case artifact:
		return graphics->artDefs->ourImages[subtype].bitmap;
		break;
	case primskill:
		return graphics->pskillsb->ourImages[subtype].bitmap;
		break;
	case secskill44:
		return CGI->abilh->abils44->ourImages[subtype*3 + 3 + val - 1].bitmap;
		break;
	case secskill:
		return CGI->abilh->abils82->ourImages[subtype*3 + 3 + val - 1].bitmap;
		break;
	case resource:
		return graphics->resources->ourImages[subtype].bitmap;
		break;
	case experience:
		return graphics->pskillsb->ourImages[4].bitmap;
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
		if(onSelect)
			onSelect();
	}
}
void CSelectableComponent::init(SDL_Surface * Border)
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
CSelectableComponent::CSelectableComponent(const Component &c, boost::function<void()> OnSelect, SDL_Surface * Border)
:SComponent(c),onSelect(OnSelect)
{
	init(Border);
}
CSelectableComponent::CSelectableComponent(Etype Type, int Sub, int Val, boost::function<void()> OnSelect, SDL_Surface * Border)
:SComponent(Type,Sub,Val),onSelect(OnSelect)
{
	init(Border);
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
void CSelectableComponent::show(SDL_Surface * to)
{
	blitAt(myBitmap,pos.x,pos.y,to);
	printAtMiddleWB(subtitle,pos.x+pos.w/2,pos.y+pos.h+14,GEOR13,12,zwykly,to);
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

void CSelWindow::selectionChange(unsigned to)
{
	for (unsigned i=0;i<components.size();i++)
	{
		CSelectableComponent * pom = dynamic_cast<CSelectableComponent*>(components[i]);
		if (!pom)
			continue;
		pom->select(i==to);
		blitAt(pom->getImg(),pom->pos.x-pos.x,pom->pos.y-pos.y,bitmap);
	}
}
CSelWindow::CSelWindow(std::string text, int player, int charperline, std::vector<CSelectableComponent*> &comps, std::vector<std::pair<std::string,boost::function<void()> > > &Buttons)
{
	for(int i=0;i<Buttons.size();i++)
	{
		buttons.push_back(new AdventureMapButton("","",(Buttons[i].second)?(Buttons[i].second):(boost::bind(&CInfoWindow::close,this)),0,0,Buttons[i].first));
	}
	for(int i=0;i<comps.size();i++)
	{
		components.push_back(comps[i]);
		comps[i]->onSelect = boost::bind(&CSelWindow::selectionChange,this,i);
	}
	CMessage::drawIWindow(this,text,player,charperline);
}
void CSelWindow::close()
{
	deactivate();
	int ret = -1;
	for (int i=0;i<components.size();i++)
	{
		if(dynamic_cast<CSelectableComponent*>(components[i])->selected)
		{
			ret = i;
		}
	}
	LOCPLINT->curint->activate();
	LOCPLINT->cb->selectionMade(ret,ID);
	delete this;
	//call owner with selection result
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
	LOCPLINT->lclickable.push_front(this);
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
	LOCPLINT->rclickable.push_front(this);
}
void ClickableR::deactivate()
{
	LOCPLINT->rclickable.erase(std::find(LOCPLINT->rclickable.begin(),LOCPLINT->rclickable.end(),this));
}
void Hoverable::activate()
{
	LOCPLINT->hoverable.push_front(this);
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
	LOCPLINT->keyinterested.push_front(this);
}
void KeyInterested::deactivate()
{
	LOCPLINT->
		keyinterested.erase(std::find(LOCPLINT->keyinterested.begin(),LOCPLINT->keyinterested.end(),this));
}
void MotionInterested::activate()
{
	LOCPLINT->motioninterested.push_front(this);
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
	LOCPLINT = this;
	playerID=Player;
	serialID=serial;
	human=true;
	pim = new boost::recursive_mutex;
	showingDialog = new CondSh<bool>(false);
	heroMoveSpeed = 2;
}
CPlayerInterface::~CPlayerInterface()
{
	delete pim;
	delete showingDialog;
}
void CPlayerInterface::init(ICallback * CB)
{
	cb = dynamic_cast<CCallback*>(CB);
	adventureInt = new CAdvMapInt(playerID);
	castleInt = NULL;
	std::vector <const CGHeroInstance *> hh = cb->getHeroesInfo(false);
	for(int i=0;i<hh.size();i++)
	{
		SDL_Surface * pom = infoWin(hh[i]);
		graphics->heroWins.insert(std::pair<int,SDL_Surface*>(hh[i]->subID,pom));
	}
	std::vector<const CGTownInstance*> tt = cb->getTownsInfo(false);
	for(int i=0;i<tt.size();i++)
	{
		SDL_Surface * pom = infoWin(tt[i]);
		graphics->townWins.insert(std::pair<int,SDL_Surface*>(tt[i]->id,pom));
	}
}
void CPlayerInterface::yourTurn()
{
	LOCPLINT = this;
	makingTurn = true;
	adventureInt->infoBar.newDay(cb->getDate(1));
	if(adventureInt->heroList.items.size())
		adventureInt->select(adventureInt->heroList.items[0].first);
	else
		adventureInt->select(adventureInt->townList.items[0]);
	adventureInt->activate();
	//show rest of things

	//initializing framerate keeper
	mainFPSmng = new FPSmanager;
	SDL_initFramerate(mainFPSmng);
	SDL_setFramerate(mainFPSmng, 48);
	//framerate keeper initialized
	timeHandler th;
	th.getDif();
	for(;makingTurn;) // main loop
	{
		updateWater();
		pim->lock();
		int tv = th.getDif();
		std::list<TimeInterested*> hlp = timeinterested;
		for (std::list<TimeInterested*>::iterator i=hlp.begin(); i != hlp.end();i++)
		{
			if(!vstd::contains(timeinterested,*i)) continue;
			if ((*i)->toNextTick>=0)
				(*i)->toNextTick-=tv;
			if ((*i)->toNextTick<0)
				(*i)->tick();
		}
		LOCPLINT->adventureInt->updateScreen = false;
		eventsM.lock();
		while(!events.empty())
		{
			handleEvent(&events.front());
			events.pop();
		}
		eventsM.unlock();
		if (curint == adventureInt) //stuff for advMapInt
		{
			adventureInt->update();
		}
		for(int i=0;i<objsToBlit.size();i++)
			objsToBlit[i]->show();
		CGI->curh->draw1();
		CSDL_Ext::update(screen);
		CGI->curh->draw2();
		pim->unlock();
		SDL_framerateDelay(mainFPSmng);
	}
	adventureInt->hide();
	cb->endTurn();
}

inline void subRect(const int & x, const int & y, const int & z, const SDL_Rect & r, const int & hid)
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
	boost::unique_lock<boost::recursive_mutex> un(*pim);

	int3 buff = details.ho->pos;
	buff.x-=11;
	buff.y-=9;
	buff = repairScreenPos(buff);
	LOCPLINT->adventureInt->position = buff; //actualizing screen pos

	if(adventureInt == curint)
		adventureInt->minimap.draw();

	if(details.style)
		return;

	//initializing objects and performing first step of move
	const CGHeroInstance * ho = details.ho; //object representing this hero
	int3 hp = details.src;
	if (!details.successful) //hero failed to move
	{
		ho->moveDir = getDir(details.src,details.dst);
		ho->isStanding = true;
		adventureInt->heroList.draw();
		if (adventureInt->terrain.currentPath && ho->movement>145) //TODO: better condition on movement - check cost of endtile
		{
			delete adventureInt->terrain.currentPath;
			adventureInt->terrain.currentPath = NULL;
			adventureInt->heroList.items[adventureInt->heroList.getPosOfHero(ho)].second = NULL;
		}
		return;
	}

	if (adventureInt->terrain.currentPath) //&& hero is moving
	{
		adventureInt->terrain.currentPath->nodes.erase(adventureInt->terrain.currentPath->nodes.end()-1);
	}


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
	for(int i=1; i<32; i+=2*heroMoveSpeed)
	{
		if(details.dst.x+1 == details.src.x && details.dst.y+1 == details.src.y) //tl
		{
			//seting advmap shift
			adventureInt->terrain.moveX = i-32;
			adventureInt->terrain.moveY = i-32;

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
			//seting advmap shift
			adventureInt->terrain.moveY = i-32;

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
			//seting advmap shift
			adventureInt->terrain.moveX = -i+32;
			adventureInt->terrain.moveY = i-32;

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
			//seting advmap shift
			adventureInt->terrain.moveX = -i+32;

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
			
			//seting advmap shift
			adventureInt->terrain.moveX = -i+32;
			adventureInt->terrain.moveY = -i+32;

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
			//seting advmap shift
			adventureInt->terrain.moveY = -i+32;

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
			//seting advmap shift
			adventureInt->terrain.moveX = i-32;
			adventureInt->terrain.moveY = -i+32;

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
			//seting advmap shift
			adventureInt->terrain.moveX = i-32;

			subRect(hp.x-3, hp.y-1, hp.z, genRect(32, 32, -31+i, 0), ho->id);
			subRect(hp.x-2, hp.y-1, hp.z, genRect(32, 32, 1+i, 0), ho->id);
			subRect(hp.x-1, hp.y-1, hp.z, genRect(32, 32, 33+i, 0), ho->id);
			subRect(hp.x, hp.y-1, hp.z, genRect(32, 32, 65+i, 0), ho->id);

			subRect(hp.x-3, hp.y, hp.z, genRect(32, 32, -31+i, 32), ho->id);
			subRect(hp.x-2, hp.y, hp.z, genRect(32, 32, 1+i, 32), ho->id);
			subRect(hp.x-1, hp.y, hp.z, genRect(32, 32, 33+i, 32), ho->id);
			subRect(hp.x, hp.y, hp.z, genRect(32, 32, 65+i, 32), ho->id);
		}
		adventureInt->updateScreen = true;
		LOCPLINT->adventureInt->update(); //updating screen
		CSDL_Ext::update(screen);
		//CGI->screenh->updateScreen();

		++LOCPLINT->adventureInt->animValHitCount; //for animations
		if(LOCPLINT->adventureInt->animValHitCount == 8)
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

	//restoring adventureInt->terrain.move*
	adventureInt->terrain.moveX = adventureInt->terrain.moveY = 0;

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

	//restoring good rects
	subRect(details.dst.x-2, details.dst.y-1, details.dst.z, genRect(32, 32, 0, 0), ho->id);
	subRect(details.dst.x-1, details.dst.y-1, details.dst.z, genRect(32, 32, 32, 0), ho->id);
	subRect(details.dst.x, details.dst.y-1, details.dst.z, genRect(32, 32, 64, 0), ho->id);

	subRect(details.dst.x-2, details.dst.y, details.dst.z, genRect(32, 32, 0, 32), ho->id);
	subRect(details.dst.x-1, details.dst.y, details.dst.z, genRect(32, 32, 32, 32), ho->id);
	subRect(details.dst.x, details.dst.y, details.dst.z, genRect(32, 32, 64, 32), ho->id);

	//restoring good order of objects
	std::stable_sort(CGI->mh->ttiles[details.dst.x-2][details.dst.y-1][details.dst.z].objects.begin(), CGI->mh->ttiles[details.dst.x-2][details.dst.y-1][details.dst.z].objects.end(), ocmptwo_cgin);
	std::stable_sort(CGI->mh->ttiles[details.dst.x-1][details.dst.y-1][details.dst.z].objects.begin(), CGI->mh->ttiles[details.dst.x-1][details.dst.y-1][details.dst.z].objects.end(), ocmptwo_cgin);
	std::stable_sort(CGI->mh->ttiles[details.dst.x][details.dst.y-1][details.dst.z].objects.begin(), CGI->mh->ttiles[details.dst.x][details.dst.y-1][details.dst.z].objects.end(), ocmptwo_cgin);

	std::stable_sort(CGI->mh->ttiles[details.dst.x-2][details.dst.y][details.dst.z].objects.begin(), CGI->mh->ttiles[details.dst.x-2][details.dst.y][details.dst.z].objects.end(), ocmptwo_cgin);
	std::stable_sort(CGI->mh->ttiles[details.dst.x-1][details.dst.y][details.dst.z].objects.begin(), CGI->mh->ttiles[details.dst.x-1][details.dst.y][details.dst.z].objects.end(), ocmptwo_cgin);
	std::stable_sort(CGI->mh->ttiles[details.dst.x][details.dst.y][details.dst.z].objects.begin(), CGI->mh->ttiles[details.dst.x][details.dst.y][details.dst.z].objects.end(), ocmptwo_cgin);

	ho->isStanding = true;
	//move finished
	adventureInt->minimap.draw();
	adventureInt->heroList.updateMove(ho);
}
void CPlayerInterface::heroKilled(const CGHeroInstance* hero)
{
	boost::unique_lock<boost::recursive_mutex> un(*pim);
	graphics->heroWins.erase(hero->ID);
	adventureInt->heroList.updateHList();
}
void CPlayerInterface::heroCreated(const CGHeroInstance * hero)
{
	if(graphics->heroWins.find(hero->subID)==graphics->heroWins.end())
		graphics->heroWins.insert(std::pair<int,SDL_Surface*>(hero->subID,infoWin(hero)));
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
			return graphics->drawHeroInfoWin(dynamic_cast<const CGHeroInstance*>(specific));
			break;
		case 98:
			return graphics->drawTownInfoWin(dynamic_cast<const CGTownInstance*>(specific));
			break;
		default:
			return NULL;
			break;
		}
	}
	else
	{
		switch (adventureInt->selection->ID)
		{
		case HEROI_TYPE:
			{
				const CGHeroInstance * curh = (const CGHeroInstance *)adventureInt->selection;
				return graphics->drawHeroInfoWin(curh);
			}
		case TOWNI_TYPE:
			{
				return graphics->drawTownInfoWin((const CGTownInstance *)adventureInt->selection);
			}
		default:
			return NULL;
		}
	}
	return NULL;
}

void CPlayerInterface::handleMouseMotion(SDL_Event *sEvent)
{
	std::vector<Hoverable*> hlp;
	for(std::list<Hoverable*>::iterator i=hoverable.begin(); i != hoverable.end();i++)
	{
		if (isItIn(&(*i)->pos,sEvent->motion.x,sEvent->motion.y))
		{
			if (!(*i)->hovered)
				hlp.push_back((*i));
		}
		else if ((*i)->hovered)
		{
			(*i)->hover(false);
		}
	}
	for(int i=0; i<hlp.size();i++)
		hlp[i]->hover(true);
	std::list<MotionInterested*> miCopy = motioninterested;
	for(std::list<MotionInterested*>::iterator i=miCopy.begin(); i != miCopy.end();i++)
	{
		if ((*i)->strongInterest || isItIn(&(*i)->pos,sEvent->motion.x,sEvent->motion.y))
		{
			(*i)->mouseMoved(sEvent->motion);
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
	case (SDLK_c):
		{
			if( dynamic_cast<CBattleInterface*> (curint) )
			{
				dynamic_cast<CBattleInterface*> (curint)->showStackQueue = false;
			}
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
	case (SDLK_c):
		{
			if( dynamic_cast<CBattleInterface*> (curint) )
			{
				dynamic_cast<CBattleInterface*> (curint)->showStackQueue = true;
			}
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
		std::list<ClickableL*> hlp = lclickable;
		for(std::list<ClickableL*>::iterator i=hlp.begin(); i != hlp.end();i++)
		{
			if(!vstd::contains(lclickable,*i)) continue;
			if (isItIn(&(*i)->pos,sEvent->motion.x,sEvent->motion.y))
			{
				(*i)->clickLeft(true);
			}
		}
	}
	else if ((sEvent->type==SDL_MOUSEBUTTONUP) && (sEvent->button.button == SDL_BUTTON_LEFT))
	{
		std::list<ClickableL*> hlp = lclickable;
		for(std::list<ClickableL*>::iterator i=hlp.begin(); i != hlp.end();i++)
		{
			if(!vstd::contains(lclickable,*i)) continue;
			if (isItIn(&(*i)->pos,sEvent->motion.x,sEvent->motion.y))
			{
				(*i)->clickLeft(false);
			}
			else
				(*i)->clickLeft(boost::logic::indeterminate);
		}
	}
	else if ((sEvent->type==SDL_MOUSEBUTTONDOWN) && (sEvent->button.button == SDL_BUTTON_RIGHT))
	{
		std::list<ClickableR*> hlp = rclickable;
		for(std::list<ClickableR*>::iterator i=hlp.begin(); i != hlp.end();i++)
		{
			if(!vstd::contains(rclickable,*i)) continue;
			if (isItIn(&(*i)->pos,sEvent->motion.x,sEvent->motion.y))
			{
				(*i)->clickRight(true);
			}
		}
	}
	else if ((sEvent->type==SDL_MOUSEBUTTONUP) && (sEvent->button.button == SDL_BUTTON_RIGHT))
	{
		std::list<ClickableR*> hlp = rclickable;
		for(std::list<ClickableR*>::iterator i=hlp.begin(); i != hlp.end();i++)
		{
			if(!vstd::contains(rclickable,*i)) continue;
			if (isItIn(&(*i)->pos,sEvent->motion.x,sEvent->motion.y))
			{
				(*i)->clickRight(false);
			}
			else
				(*i)->clickRight(boost::logic::indeterminate);
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
	if(pos.x>CGI->mh->map->width - this->adventureInt->terrain.tilesw + Woff)
		pos.x = CGI->mh->map->width - this->adventureInt->terrain.tilesw + Woff;
	if(pos.y>CGI->mh->map->height - this->adventureInt->terrain.tilesh + Hoff)
		pos.y = CGI->mh->map->height - this->adventureInt->terrain.tilesh + Hoff;
	return pos;
}
void CPlayerInterface::heroPrimarySkillChanged(const CGHeroInstance * hero, int which, int val)
{
	boost::unique_lock<boost::recursive_mutex> un(*pim);
	SDL_FreeSurface(graphics->heroWins[hero->subID]);//TODO: moznaby zmieniac jedynie fragment bitmapy zwiazany z dana umiejetnoscia
	graphics->heroWins[hero->subID] = infoWin(hero); //a nie przerysowywac calosc. Troche roboty, obecnie chyba nie wartej swieczki.
	if (adventureInt->selection == hero)
		adventureInt->infoBar.draw();
	return;
}

void CPlayerInterface::receivedResource(int type, int val)
{
	boost::unique_lock<boost::recursive_mutex> un(*pim);
	if(!curint->subInt)
		adventureInt->resdatabar.draw();
}

void CPlayerInterface::showSelDialog(std::string &text, const std::vector<Component*> &components, ui32 askID)
//void CPlayerInterface::showSelDialog(std::string text, std::vector<CSelectableComponent*> & components, int askID)
{
	boost::unique_lock<boost::recursive_mutex> un(*pim);
	adventureInt->hide(); //dezaktywacja starego interfejsu
	std::vector<CSelectableComponent*> intComps;
	for(int i=0;i<components.size();i++)
		intComps.push_back(new CSelectableComponent(*components[i])); //will be deleted by CSelWindow::close
	std::vector<std::pair<std::string,boost::function<void()> > > pom;
	pom.push_back(std::pair<std::string,boost::function<void()> >("IOKAY.DEF",0));

	CSelWindow * temp = new CSelWindow(text,playerID,35,intComps,pom);
	LOCPLINT->objsToBlit.push_back(temp);
	temp->activate();
	temp->ID = askID;
	intComps[0]->clickLeft(true);
}
void CPlayerInterface::heroGotLevel(const CGHeroInstance *hero, int pskill, std::vector<ui16>& skills, boost::function<void(ui32)> &callback)
{
	{
		boost::unique_lock<boost::mutex> un(showingDialog->mx);
		while(showingDialog->data)
			showingDialog->cond.wait(un);
	}
	boost::unique_lock<boost::recursive_mutex> un(*pim);
	CLevelWindow *lw = new CLevelWindow(hero,pskill,skills,callback);
	curint->deactivate();
	lw->activate();
}
void CPlayerInterface::heroInGarrisonChange(const CGTownInstance *town)
{
	boost::unique_lock<boost::recursive_mutex> un(*pim);
	//redraw infowindow
	SDL_FreeSurface(graphics->townWins[town->id]);
	graphics->townWins[town->id] = infoWin(town);
	if(town->garrisonHero)
	{
		CGI->mh->hideObject(town->garrisonHero);
	}
	if(town->visitingHero)
	{
		CGI->mh->printObject(town->visitingHero);
	}
	adventureInt->heroList.updateHList();

	CCastleInterface *c = dynamic_cast<CCastleInterface*>(curint);
	if(c)
	{
		c->garr->highlighted = NULL;
		c->hslotup.hero = town->garrisonHero;
		c->garr->odown = c->hslotdown.hero = town->visitingHero;
		c->garr->set2 = town->visitingHero ? &town->visitingHero->army : NULL;
		c->garr->recreateSlots();
		c->showAll();
	}
}
void CPlayerInterface::heroVisitsTown(const CGHeroInstance* hero, const CGTownInstance * town)
{
	if(hero->tempOwner != town->tempOwner)
		return;
	boost::unique_lock<boost::recursive_mutex> un(*pim);
	openTownWindow(town);
}
void CPlayerInterface::garrisonChanged(const CGObjectInstance * obj)
{
	boost::unique_lock<boost::recursive_mutex> un(*pim);
	if(obj->ID == 34) //hero
	{
		const CGHeroInstance * hh;
		if(hh = dynamic_cast<const CGHeroInstance*>(obj))
		{
			SDL_FreeSurface(graphics->heroWins[hh->subID]);
			graphics->heroWins[hh->subID] = infoWin(hh);
		}
		CHeroWindow * hw = dynamic_cast<CHeroWindow *>(curint->subInt);
		if(hw)
		{
			hw->garInt->recreateSlots();
			hw->garInt->show();
		}
		if(castleInt) //opened town window - redraw town garrsion slots (change is within hero garr)
		{
			castleInt->garr->highlighted = NULL;
			castleInt->garr->recreateSlots();
		}

	}
	else if (obj->ID == 98) //town
	{
		const CGTownInstance * tt;
		if(tt = static_cast<const CGTownInstance*>(obj))
		{
			SDL_FreeSurface(graphics->townWins[tt->id]);
			graphics->townWins[tt->id] = infoWin(tt);
		}
		if(tt->visitingHero)
		{
			SDL_FreeSurface(graphics->heroWins[tt->visitingHero->subID]);
			graphics->heroWins[tt->visitingHero->subID] = infoWin(tt->visitingHero);
		}
		if(LOCPLINT->castleInt)
		{
			LOCPLINT->castleInt->garr->highlighted = NULL;
			LOCPLINT->castleInt->garr->recreateSlots();
		}
	}
}
void CPlayerInterface::buildChanged(const CGTownInstance *town, int buildingID, int what) //what: 1 - built, 2 - demolished
{
	boost::unique_lock<boost::recursive_mutex> un(*pim);
	switch (buildingID)
	{
	case 7: case 8: case 9: case 10: case 11: case 12: case 13: case 15:
		{
			SDL_FreeSurface(graphics->townWins[town->id]);
			graphics->townWins[town->id] = infoWin(town);
			break;
		}
	}
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

void CPlayerInterface::battleStart(CCreatureSet *army1, CCreatureSet *army2, int3 tile, CGHeroInstance *hero1, CGHeroInstance *hero2, bool side) //called by engine when battle starts; side=0 - left, side=1 - right
{
	boost::unique_lock<boost::recursive_mutex> un(*pim);
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
	boost::unique_lock<boost::recursive_mutex> un(*pim);
	dynamic_cast<CBattleInterface*>(curint)->newRound(round);
}

void CPlayerInterface::actionStarted(const BattleAction* action)
{
	curAction = action;
	if((action->actionType==2 || (action->actionType==6 && action->destinationTile!=cb->battleGetPos(action->stackNumber)))
		&& static_cast<CBattleInterface*>(curint)->creAnims[action->stackNumber]->framesInGroup(20)
		)
	{
		static_cast<CBattleInterface*>(curint)->creAnims[action->stackNumber]->setType(20);
	if((action->actionType==2 || (action->actionType==6 && action->destinationTile!=cb->battleGetPos(action->stackNumber)))) //deactivating interface when move is started
	{
		static_cast<CBattleInterface*>(curint)->deactivate();
	}
}
}

void CPlayerInterface::actionFinished(const BattleAction* action)
{
	curAction = NULL;
	if((action->actionType==2 || (action->actionType==6 && action->destinationTile!=cb->battleGetPos(action->stackNumber)))) //activating interface when move is finished
	{
		static_cast<CBattleInterface*>(curint)->activate();
	}
}

BattleAction CPlayerInterface::activeStack(int stackID) //called when it's turn of that stack
{
	CBattleInterface *b = dynamic_cast<CBattleInterface*>(curint);
	{
		boost::unique_lock<boost::recursive_mutex> un(*pim);
		b->stackActivated(stackID);
	}
	//wait till BattleInterface sets its command
	boost::unique_lock<boost::mutex> lock(b->givenCommand->mx);
	while(!b->givenCommand->data)
		b->givenCommand->cond.wait(lock);

	//tidy up
	BattleAction ret = *(b->givenCommand->data);
	delete b->givenCommand->data;
	b->givenCommand->data = NULL;

	//return command
	return ret;
}

void CPlayerInterface::battleEnd(BattleResult *br)
{
	boost::unique_lock<boost::recursive_mutex> un(*pim);
	((CBattleInterface*)curint)->battleFinished(*br);
}

void CPlayerInterface::battleResultQuited()
{
	boost::unique_lock<boost::recursive_mutex> un(*pim);
	((CBattleInterface*)curint)->resWindow->deactivate();
	objsToBlit -= curint;
	delete curint;
	curint = adventureInt;
	adventureInt->activate();
	LOCPLINT->showingDialog->setn(false);
}

void CPlayerInterface::battleStackMoved(int ID, int dest)
{
	boost::unique_lock<boost::recursive_mutex> un(*pim);
	dynamic_cast<CBattleInterface*>(curint)->stackMoved(ID, dest, dest==curAction->destinationTile);
}
void CPlayerInterface::battleAttack(BattleAttack *ba)
{
	boost::unique_lock<boost::recursive_mutex> un(*pim);
	if(ba->shot())
		dynamic_cast<CBattleInterface*>(curint)->stackIsShooting(ba->stackAttacking,cb->battleGetPos(ba->bsa.stackAttacked));
	else
		dynamic_cast<CBattleInterface*>(curint)->stackAttacking( ba->stackAttacking, ba->counter() ? curAction->destinationTile : curAction->additionalInfo );
	if(ba->killed())
		dynamic_cast<CBattleInterface*>(curint)->stackKilled(ba->bsa.stackAttacked, ba->bsa.damageAmount, ba->bsa.killedAmount, ba->stackAttacking, ba->shot());
	else
		dynamic_cast<CBattleInterface*>(curint)->stackIsAttacked(ba->bsa.stackAttacked, ba->bsa.damageAmount, ba->bsa.killedAmount, ba->stackAttacking, ba->shot());
}
//void CPlayerInterface::battleStackKilled(int ID, int dmg, int killed, int IDby, bool byShooting)
//{
//	dynamic_cast<CBattleInterface*>(curint)->stackKilled(ID, dmg, killed, IDby, byShooting);
//}

//void CPlayerInterface::battleStackIsShooting(int ID, int dest)
//{
//	dynamic_cast<CBattleInterface*>(curint)->stackIsShooting(ID, dest);
//}

void CPlayerInterface::showComp(SComponent comp)
{
	boost::unique_lock<boost::recursive_mutex> un(*pim);
	adventureInt->infoBar.showComp(&comp,4000);
}

void CPlayerInterface::showInfoDialog(std::string &text, const std::vector<Component*> &components)
{
	std::vector<SComponent*> intComps;
	for(int i=0;i<components.size();i++)
		intComps.push_back(new SComponent(*components[i]));
	showInfoDialog(text,intComps);
}
void CPlayerInterface::showInfoDialog(std::string &text, const std::vector<SComponent*> & components)
{
	showingDialog->set(true);
	boost::unique_lock<boost::recursive_mutex> un(*pim);
	curint->deactivate(); //dezaktywacja starego interfejsu

	std::vector<std::pair<std::string,CFunctionList<void()> > > pom;
	pom.push_back(std::pair<std::string,CFunctionList<void()> >("IOKAY.DEF",0));
	
	CInfoWindow * temp = new CInfoWindow(text,playerID,32,components,pom);
	temp->buttons[0]->callback += boost::bind(&IActivable::activate,LOCPLINT->curint);
	temp->activate();
	LOCPLINT->objsToBlit.push_back(temp);
}
void CPlayerInterface::showYesNoDialog(std::string &text, const std::vector<SComponent*> & components, CFunctionList<void()> onYes, CFunctionList<void()> onNo, bool deactivateCur, bool DelComps)
{
	boost::unique_lock<boost::recursive_mutex> un(*pim);
	if(deactivateCur)
		curint->deactivate(); //dezaktywacja starego interfejsu
	std::vector<std::pair<std::string,CFunctionList<void()> > > pom;
	pom.push_back(std::pair<std::string,CFunctionList<void()> >("IOKAY.DEF",0));
	pom.push_back(std::pair<std::string,CFunctionList<void()> >("ICANCEL.DEF",0));
	CInfoWindow * temp = new CInfoWindow(text,playerID,32,components,pom);
	temp->delComps = DelComps;
	for(int i=0;i<onYes.funcs.size();i++)
		temp->buttons[0]->callback += onYes.funcs[i];
	for(int i=0;i<onNo.funcs.size();i++)
		temp->buttons[1]->callback += onNo.funcs[i];

	//if(onYes)
	//	temp->buttons[0]->callback += boost::bind(&CInfoWindow::close,temp);
	//if(onNo)
	//	temp->buttons[1]->callback += boost::bind(&CInfoWindow::close,temp);
	temp->activate();
	LOCPLINT->objsToBlit.push_back(temp);
}

void CPlayerInterface::showYesNoDialog( std::string &text, const std::vector<Component*> &components, ui32 askID )
{
	boost::unique_lock<boost::recursive_mutex> un(*pim);
	curint->deactivate(); //dezaktywacja starego interfejsu

	std::vector<SComponent*> intComps;
	for(int i=0;i<components.size();i++)
		intComps.push_back(new SComponent(*components[i])); //will be deleted by CSelWindow::close
	std::vector<std::pair<std::string,CFunctionList<void()> > > pom;
	pom.push_back(std::pair<std::string,CFunctionList<void()> >("IOKAY.DEF",0));
	pom.push_back(std::pair<std::string,CFunctionList<void()> >("ICANCEL.DEF",0));

	CInfoWindow * temp = new CInfoWindow(text,playerID,32,intComps,pom);
	temp->buttons[0]->callback += boost::bind(&IActivable::activate,curint);
	temp->buttons[1]->callback += boost::bind(&IActivable::activate,curint);
	temp->buttons[0]->callback += boost::bind(&CCallback::selectionMade,cb,0,askID);
	temp->buttons[1]->callback += boost::bind(&CCallback::selectionMade,cb,1,askID);
	temp->delComps = true;

	temp->activate();
	LOCPLINT->objsToBlit.push_back(temp);
}
void CPlayerInterface::removeObjToBlit(IShowable* obj)
{
	boost::unique_lock<boost::recursive_mutex> un(*pim);
	objsToBlit.erase
		(std::find(objsToBlit.begin(),objsToBlit.end(),obj));
	//delete obj;
}
void CPlayerInterface::tileRevealed(int3 pos)
{
	boost::unique_lock<boost::recursive_mutex> un(*pim);
	adventureInt->minimap.showTile(pos);
}
void CPlayerInterface::tileHidden(int3 pos)
{
	boost::unique_lock<boost::recursive_mutex> un(*pim);
	adventureInt->minimap.hideTile(pos);
}
void CPlayerInterface::openHeroWindow(const CGHeroInstance *hero)
{
	boost::unique_lock<boost::recursive_mutex> un(*pim);
	adventureInt->heroWindow->setHero(hero);
	this->objsToBlit.push_back(adventureInt->heroWindow);
	curint->deactivate();
	if(curint == castleInt)
		castleInt->subInt = adventureInt->heroWindow;
	adventureInt->heroWindow->quitButton->callback.funcs.clear();
	adventureInt->heroWindow->quitButton->callback += boost::bind(&CHeroWindow::deactivate,adventureInt->heroWindow);
	adventureInt->heroWindow->quitButton->callback += boost::bind(&CHeroWindow::quit,adventureInt->heroWindow);
	adventureInt->heroWindow->activate();
}

void CPlayerInterface::heroArtifactSetChanged(const CGHeroInstance*hero)
{
	boost::unique_lock<boost::recursive_mutex> un(*pim);
	if(curint->subInt == adventureInt->heroWindow)
	{
		adventureInt->heroWindow->deactivate();
		adventureInt->heroWindow->setHero(adventureInt->heroWindow->curHero);
		adventureInt->heroWindow->activate();
	}
}

void CPlayerInterface::updateWater()
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
	//CGI->screenh->updateScreen();
}

void CPlayerInterface::availableCreaturesChanged( const CGTownInstance *town )
{
	boost::unique_lock<boost::recursive_mutex> un(*pim);
	if(curint == castleInt)
	{
		CFortScreen *fs = dynamic_cast<CFortScreen*>(curint->subInt);
		if(fs)
			fs->draw(castleInt,false);
	}
}
CStatusBar::CStatusBar(int x, int y, std::string name, int maxw)
{
	bg=BitmapHandler::loadBitmap(name);
	SDL_SetColorKey(bg,SDL_SRCCOLORKEY,SDL_MapRGB(bg->format,0,255,255));
	pos.x=x;
	pos.y=y;
	if(maxw >= 0)
		pos.w = min(bg->w,maxw);
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
void CStatusBar::print(const std::string & text)
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

	arrup = CDefHandler::giveDef("IAM012.DEF");
	arrdo = CDefHandler::giveDef("IAM013.DEF");
	mobile = CDefHandler::giveDef("IMOBIL.DEF");
	mana = CDefHandler::giveDef("IMANA.DEF");
	empty = BitmapHandler::loadBitmap("HPSXXX.bmp");
	selection = BitmapHandler::loadBitmap("HPSYYY.bmp");
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
		const CGHeroInstance * h = LOCPLINT->cb->getHeroInfo(i,0);
		if(!h->inTownGarrison)
			items.push_back(std::pair<const CGHeroInstance *,CPath *>(h,NULL));
	}
}
void CHeroList::select(int which)
{
	if (which<0)
	{
		selected = which;
		LOCPLINT->adventureInt->selection = NULL;
		LOCPLINT->adventureInt->terrain.currentPath = NULL;
		draw();
		LOCPLINT->adventureInt->infoBar.draw(NULL);
	}
	if (which>=items.size())
		return;
	selected = which;
	LOCPLINT->adventureInt->centerOn(items[which].first->pos);
	LOCPLINT->adventureInt->selection = items[which].first;
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
		if ( (ny+from)==selected && (LOCPLINT->adventureInt->selection->ID == HEROI_TYPE))
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
void CHeroList::mouseMoved (const SDL_MouseMotionEvent & sEvent)
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
		CInfoPopup * ip = new CInfoPopup(graphics->heroWins[items[from+ny].first->subID],LOCPLINT->current->motion.x-graphics->heroWins[items[from+ny].first->subID]->w,LOCPLINT->current->motion.y-graphics->heroWins[items[from+ny].first->subID]->h,false);
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
void CHeroList::keyPressed (const SDL_KeyboardEvent & key)
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
		const CGHeroInstance *cur = items[iT].first;
		int pom = cur->movement / 100;
		if (pom>25) pom=25;
		if (pom<0) pom=0;
		blitAt(mobile->ourImages[pom].bitmap,posmobx,posmoby+i*32); //move point
		pom = cur->mana / 5; //bylo: .../10;
		if (pom>25) pom=25;
		if (pom<0) pom=0;
		blitAt(mana->ourImages[pom].bitmap,posmanx,posmany+i*32); //mana
		SDL_Surface * temp = graphics->portraitSmall[cur->portrait];
		blitAt(temp,posporx,pospory+i*32);
		if ((selected == iT) && (LOCPLINT->adventureInt->selection->ID == HEROI_TYPE))
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

int CHeroList::getPosOfHero(const CArmedInstance* h)
{
	return vstd::findPos(items,h,boost::bind(vstd::equal<std::pair<const CGHeroInstance*, CPath *>,const CArmedInstance *,const CGHeroInstance*>,_1,&std::pair<const CGHeroInstance*, CPath *>::first,_2));
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
	arrup = CDefHandler::giveDef("IAM014.DEF");
	arrdo = CDefHandler::giveDef("IAM015.DEF");

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

void CTownList::mouseMoved (const SDL_MouseMotionEvent & sEvent)
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
	LOCPLINT->statusbar->print(items[from+ny]->name);
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
		if (SIZE==5 && (ny+from)==selected && (LOCPLINT->adventureInt->selection->ID == TOWNI_TYPE))
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
		CInfoPopup * ip = new CInfoPopup(graphics->townWins[items[from+ny]->id],LOCPLINT->current->motion.x-graphics->townWins[items[from+ny]->id]->w,LOCPLINT->current->motion.y-graphics->townWins[items[from+ny]->id]->h,false);
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

void CTownList::keyPressed (const SDL_KeyboardEvent & key)
{
}

void CTownList::draw()
{
	for (int iT=0+from;iT<SIZE+from;iT++)
	{
		int i = iT-from;
		if (iT>=items.size())
		{
			blitAt(graphics->getPic(-1),posporx,pospory+i*32);
			continue;
		}

		blitAt(graphics->getPic(items[iT]->subID,items[iT]->hasFort(),items[iT]->builded),posporx,pospory+i*32);

		if ((selected == iT) && (LOCPLINT->adventureInt->selection->ID == TOWNI_TYPE))
		{
			blitAt(graphics->getPic(-2),posporx,pospory+i*32);
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


CCreaturePic::CCreaturePic(CCreature *cre, bool Big)
:c(cre),big(Big)
{
	anim = new CCreatureAnimation(cre->animDefName);
}
CCreaturePic::~CCreaturePic()
{
	delete anim;
}
int CCreaturePic::blitPic(SDL_Surface *to, int x, int y, bool nextFrame)
{
	SDL_Rect dst;
	if(big)
	{
		blitAt(graphics->backgrounds[c->faction],x,y);//curx-50,pos.y+130-65);
		dst = genRect(130,100,x,y);
	}
	else
	{
		blitAt(graphics->backgroundsm[c->faction],x,y);//curx-50,pos.y+130-65);
		dst = genRect(120,100,x,y);
	}
	if(c->isDoubleWide())
		x-=15;
	return anim->nextFrameMiddle(to,x+70,y+45,true,nextFrame,false,&dst);
}
SDL_Surface * CCreaturePic::getPic(bool nextFrame)
{
	//TODO: write
	return NULL;
}
void CRecrutationWindow::close()
{
	deactivate();
	delete this;
	LOCPLINT->curint->activate();
}
void CRecrutationWindow::Max()
{
	slider->moveTo(slider->amount);
}
void CRecrutationWindow::Buy()
{
	recruit(creatures[which].ID,slider->value);
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
			int newAmount = min(amounts[i],creatures[i].amount);
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
	SDL_itoa(creatures[which].amount-slider->value,pom,10); //available
	printAtMiddle(pom,pos.x+205,pos.y+252,GEOR13,zwykly,screen);
	SDL_itoa(slider->value,pom,10); //recruit
	printAtMiddle(pom,pos.x+279,pos.y+252,GEOR13,zwykly,screen);
	printAtMiddle(CGI->generaltexth->allTexts[16] + " " + CGI->creh->creatures[creatures[which].ID].namePl,pos.x+243,pos.y+32,GEOR16,tytulowy,screen); //eg "Recruit Dragon flies"
	int curx = pos.x+115-creatures[which].res.size()*16;
	for(int i=0;i<creatures[which].res.size();i++)
	{
		blitAt(graphics->resources32->ourImages[creatures[which].res[i].first].bitmap,curx,pos.y+243,screen);
		blitAt(graphics->resources32->ourImages[creatures[which].res[i].first].bitmap,curx+258,pos.y+243,screen);
		SDL_itoa(creatures[which].res[i].second,pom,10);
		printAtMiddle(pom,curx+12,pos.y+286,GEOR13,zwykly,screen);
		SDL_itoa(creatures[which].res[i].second * slider->value,pom,10);
		printAtMiddle(pom,curx+12+258,pos.y+286,GEOR13,zwykly,screen);
		curx+=32;
	}

	curx = pos.x + 192 + 102 - (102*creatures.size()/2) - (18*(creatures.size()-1)/2);
	for(int i=0;i<creatures.size();i++)
	{
		creatures[i].pic->blitPic(screen,curx-50,pos.y+130-65,!(c%4));
		curx += 120;
	}
	c++;
}
CRecrutationWindow::CRecrutationWindow(const std::vector<std::pair<int,int> > &Creatures, const boost::function<void(int,int)> &Recruit) //creatures - pairs<creature_ID,amount>
:recruit(Recruit)
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
		creatures[i].pic = new CCreaturePic(&CGI->creh->creatures[Creatures[i].first]);
		amounts[i] = CGI->creh->creatures[Creatures[i].first].maxAmount(LOCPLINT->cb->getResourceAmount());
	}
	SDL_Surface *hhlp = BitmapHandler::loadBitmap("TPRCRT.bmp");
	graphics->blueToPlayersAdv(hhlp,LOCPLINT->playerID);
	bitmap = SDL_ConvertSurface(hhlp,screen->format,0); //na 8bitowej mapie by sie psulo
	SDL_SetColorKey(bitmap,SDL_SRCCOLORKEY,SDL_MapRGB(bitmap->format,0,255,255));
	SDL_FreeSurface(hhlp);
	pos.x = screen->w/2 - bitmap->w/2;
	pos.y = screen->h/2 - bitmap->h/2;
	pos.w = bitmap->w;
	pos.h = bitmap->h;
	slider = new CSlider(pos.x+176,pos.y+279,135,boost::bind(&CRecrutationWindow::sliderMoved,this, _1),1,min(amounts[0],creatures[0].amount),0,true);
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
	if(!creatures[0].amount)
	{
		max->block(true);
		buy->block(true);
	}
}
CRecrutationWindow::~CRecrutationWindow()
{
	for(int i=0;i<creatures.size();i++)
	{
		delete creatures[i].pic;
	}
	delete max;
	delete buy;
	delete cancel;
	SDL_FreeSurface(bitmap);
	delete slider;
}

CSplitWindow::CSplitWindow(int cid, int max, CGarrisonInt *Owner)
{
	c=cid;
	slider = NULL;
	gar = Owner;
	bitmap = BitmapHandler::loadBitmap("GPUCRDIV.bmp");
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
	anim = new CCreaturePic(&CGI->creh->creatures[cid]);
	anim->anim->setType(1);

	std::string title = CGI->generaltexth->allTexts[256];
	boost::algorithm::replace_first(title,"%s",CGI->creh->creatures[cid].namePl);
	printAtMiddle(title,150,34,GEOR16,tytulowy,bitmap);
}
CSplitWindow::~CSplitWindow()
{
	SDL_FreeSurface(bitmap);
	delete ok;
	delete cancel;
	delete slider;
	delete anim;
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
	SDL_itoa(a1,pom,10);
	printAtMiddle(pom,pos.x+70,pos.y+237,GEOR16,zwykly,screen);
	SDL_itoa(a2,pom,10);
	printAtMiddle(pom,pos.x+233,pos.y+237,GEOR16,zwykly,screen);
	anim->blitPic(screen,pos.x+20,pos.y+54,false);
	anim->blitPic(screen,pos.x+177,pos.y+54,false);
}
void CSplitWindow::keyPressed (const SDL_KeyboardEvent & key)
{
	//TODO: make manual typing possible
}

void CCreInfoWindow::show(SDL_Surface * to)
{
	char pom[15];
	blitAt(bitmap,pos.x,pos.y,screen);
	anim->blitPic(screen,pos.x+21,pos.y+48,(type) && !(anf%4));
	if(++anf==4) 
		anf=0;
	if(count.size())
		printTo(count.c_str(),pos.x+114,pos.y+174,GEOR16,zwykly);
	if(upgrade)
		upgrade->show();
	if(dismiss)
		dismiss->show();
	if(ok)
		ok->show();
}

CCreInfoWindow::CCreInfoWindow(int Cid, int Type, int creatureCount, StackState *State, boost::function<void()> Upg, boost::function<void()> Dsm, UpgradeInfo *ui)
:ok(0),dismiss(0),upgrade(0),type(Type),dsm(Dsm),dependant(0)
{
	active = false;
	anf = 0;
	c = &CGI->creh->creatures[Cid];
	bitmap = BitmapHandler::loadBitmap("CRSTKPU.bmp");
	pos.x = screen->w/2 - bitmap->w/2;
	pos.y = screen->h/2 - bitmap->h/2;
	pos.w = bitmap->w;
	pos.h = bitmap->h;
	graphics->blueToPlayersAdv(bitmap,LOCPLINT->playerID);
	SDL_SetColorKey(bitmap,SDL_SRCCOLORKEY,SDL_MapRGB(bitmap->format,0,255,255));
	anim = new CCreaturePic(c);
	if(!type) anim->anim->setType(1);

	char pom[25];int hlp=0;

	if(creatureCount)
	{
		SDL_itoa(creatureCount,pom,10);
		count = pom;
	}

	printAtMiddle(c->namePl,149,30,GEOR13,zwykly,bitmap); //creature name

	//atttack
	printAt(CGI->preth->zelp[435].first,155,48,GEOR13,zwykly,bitmap);
	SDL_itoa(c->attack,pom,10);
	if(State && State->attackBonus)
	{
		int hlp = log10f(c->attack)+2;
		pom[hlp-1] = ' '; pom[hlp] = '(';
		SDL_itoa(c->attack+State->attackBonus,pom+hlp+1,10);
		hlp += 2+(int)log10f(State->attackBonus+c->attack);
		pom[hlp] = ')'; pom[hlp+1] = '\0';
	}
	printToWR(pom,276,61,GEOR13,zwykly,bitmap);

	//defense
	printAt(CGI->preth->zelp[436].first,155,67,GEOR13,zwykly,bitmap);
	SDL_itoa(c->defence,pom,10);
	if(State && State->defenseBonus)
	{
		int hlp = log10f(c->defence)+2;
		pom[hlp-1] = ' '; pom[hlp] = '(';
		SDL_itoa(c->defence+State->defenseBonus,pom+hlp+1,10);
		hlp += 2+(int)log10f(State->defenseBonus+c->defence);
		pom[hlp] = ')'; pom[hlp+1] = '\0';
	}
	printToWR(pom,276,80,GEOR13,zwykly,bitmap);

	//shots
	if(c->shots)
	{
		printAt(CGI->generaltexth->allTexts[198],155,86,GEOR13,zwykly,bitmap);
		SDL_itoa(c->shots,pom,10);
		printToWR(pom,276,99,GEOR13,zwykly,bitmap);
	}

	//damage
	printAt(CGI->generaltexth->allTexts[199],155,105,GEOR13,zwykly,bitmap);
	SDL_itoa(c->damageMin,pom,10);
	hlp=log10f(c->damageMin)+2;
	pom[hlp-1]=' '; pom[hlp]='-'; pom[hlp+1]=' ';
	SDL_itoa(c->damageMax,pom+hlp+2,10);
	printToWR(pom,276,118,GEOR13,zwykly,bitmap);

	//health
	printAt(CGI->preth->zelp[439].first,155,124,GEOR13,zwykly,bitmap);
	SDL_itoa(c->hitPoints,pom,10);
	printToWR(pom,276,137,GEOR13,zwykly,bitmap);

	//remaining health
	if(State && State->currentHealth)
	{
		printAt(CGI->preth->zelp[440].first,155,143,GEOR13,zwykly,bitmap);
		SDL_itoa(State->currentHealth,pom,10);
		printToWR(pom,276,156,GEOR13,zwykly,bitmap);
	}

	//speed
	printAt(CGI->preth->zelp[441].first,155,162,GEOR13,zwykly,bitmap);
	SDL_itoa(c->speed,pom,10);
	printToWR(pom,276,175,GEOR13,zwykly,bitmap);


	//luck and morale
	blitAt(graphics->morale42->ourImages[(State)?(State->morale+3):(3)].bitmap,24,189,bitmap);
	blitAt(graphics->luck42->ourImages[(State)?(State->luck+3):(3)].bitmap,77,189,bitmap);

	//print abilities text - if r-click popup
	if(type)
	{
		if(Upg && ui)
		{
			bool enough = true;
			for(std::set<std::pair<int,int> >::iterator i=ui->cost[0].begin(); i!=ui->cost[0].end(); i++) //calculate upgrade cost
			{
				if(LOCPLINT->cb->getResourceAmount(i->first) < i->second*creatureCount)
					enough = false;
				upgResCost.push_back(new SComponent(SComponent::resource,i->first,i->second*creatureCount)); 
			}

			if(enough)
			{
				CFunctionList<void()> fs[2];
				fs[0] += Upg;
				fs[0] += boost::bind(&CCreInfoWindow::close,this);
				fs[1] += boost::bind(&CCreInfoWindow::activate,this);
				CFunctionList<void()> cfl;
				cfl = boost::bind(&CCreInfoWindow::deactivate,this);
				cfl += boost::bind(&CPlayerInterface::showYesNoDialog,LOCPLINT,CGI->generaltexth->allTexts[207],boost::ref(upgResCost),fs[0],fs[1],false,false);
				upgrade = new AdventureMapButton("",CGI->preth->zelp[446].second,cfl,pos.x+76,pos.y+237,"IVIEWCR.DEF");
			}
			else
			{
				upgrade = new AdventureMapButton("",CGI->preth->zelp[446].second,boost::function<void()>(),pos.x+76,pos.y+237,"IVIEWCR.DEF");
				upgrade->callback.funcs.clear();
				upgrade->bitmapOffset = 2;
			}

		}
		if(Dsm)
		{
			CFunctionList<void()> fs[2];
			//on dismiss confirmed
			fs[0] += Dsm; //dismiss
			fs[0] += boost::bind(&CCreInfoWindow::close,this);//close this window
			fs[1] += boost::bind(&CCreInfoWindow::activate,this);
			CFunctionList<void()> cfl;
		        cfl = boost::bind(&CCreInfoWindow::deactivate,this);
			cfl += boost::bind(&CPlayerInterface::showYesNoDialog,LOCPLINT,CGI->generaltexth->allTexts[12],std::vector<SComponent*>(),fs[0],fs[1],false,false);
			dismiss = new AdventureMapButton("",CGI->preth->zelp[445].second,cfl,pos.x+21,pos.y+237,"IVIEWCR2.DEF");
		}
		ok = new AdventureMapButton("",CGI->preth->zelp[445].second,boost::bind(&CCreInfoWindow::close,this),pos.x+216,pos.y+237,"IOKAY.DEF");
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
	delete upgrade;
	delete ok;
	delete dismiss;
	for(int i=0; i<upgResCost.size();i++)
		delete upgResCost[i];
}
void CCreInfoWindow::activate()
{
	if(active) return;
	active = true;
	if(!type)
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
	if(dynamic_cast<CHeroWindow*>(LOCPLINT->curint->subInt))
	{
		if(type)
			LOCPLINT->curint->subInt->activate();
	}
	else
	{
		CCastleInterface *c = dynamic_cast<CCastleInterface*>(LOCPLINT->curint);
		if(c)
			c->showAll();
		if(type)
			LOCPLINT->curint->activate();
	}
	delete this;
}
void CCreInfoWindow::clickRight(boost::logic::tribool down)
{
	if(down)
		return;
	close();
}
void CCreInfoWindow::dismissF()
{
	dsm();
	close();
}
void CCreInfoWindow::keyPressed (const SDL_KeyboardEvent & key)
{
}
void CCreInfoWindow::deactivate()
{
	if(!active) return;
	active = false;
	if(!type)
		ClickableR::deactivate();
	LOCPLINT->objsToBlit.erase(std::find(LOCPLINT->objsToBlit.begin(),LOCPLINT->objsToBlit.end(),this));
	if(ok)
		ok->deactivate();
	if(dismiss)
		dismiss->deactivate();
	if(upgrade)
		upgrade->deactivate();
}

void CCreInfoWindow::onUpgradeYes()
{
	dependant->close();
	dependant->deactivate();
	delete dependant;
 
	LOCPLINT->curint->activate();
}

void CCreInfoWindow::onUpgradeNo()
{

}
void CLevelWindow::close()
{
	deactivate();
	for(int i=0;i<comps.size();i++)
	{
		if(comps[i]->selected)
		{
			cb(i);
			break;
		}
	}
	delete this;
	LOCPLINT->curint->activate();
}
CLevelWindow::CLevelWindow(const CGHeroInstance *hero, int pskill, std::vector<ui16> &skills, boost::function<void(ui32)> &callback)
{
	heroType = hero->subID;
	cb = callback;
	for(int i=0;i<skills.size();i++)
		comps.push_back(new CSelectableComponent(SComponent::secskill44,skills[i],hero->getSecSkillLevel(skills[i])+1,boost::bind(&CLevelWindow::selectionChanged,this,i)));
	bitmap = BitmapHandler::loadBitmap("LVLUPBKG.bmp");
	graphics->blueToPlayersAdv(bitmap,hero->tempOwner);
	SDL_SetColorKey(bitmap,SDL_SRCCOLORKEY,SDL_MapRGB(bitmap->format,0,255,255));
	pos.x = screen->w/2 - bitmap->w/2;
	pos.y = screen->h/2 - bitmap->h/2;
	pos.w = bitmap->w;
	pos.h = bitmap->h;
	ok = new AdventureMapButton("","",boost::bind(&CLevelWindow::close,this),pos.x+297,pos.y+413,"IOKAY.DEF");

	//draw window
	char buf[100], buf2[100];
	strcpy(buf2,CGI->generaltexth->allTexts[444].c_str()); //%s has gained a level.
	sprintf(buf,buf2,hero->name.c_str());
	printAtMiddle(buf,192,35,GEOR16,zwykly,bitmap);

	strcpy(buf2,CGI->generaltexth->allTexts[445].c_str()); //%s is now a level %d %s.
	sprintf(buf,buf2,hero->name.c_str(),hero->level,hero->type->heroClass->name.c_str());
	printAtMiddle(buf,192,162,GEOR16,zwykly,bitmap);

	blitAt(graphics->pskillsm->ourImages[pskill].bitmap,174,190,bitmap);

	printAtMiddle((CGI->generaltexth->primarySkillNames[pskill] + " +1"),192,252,GEOR16,zwykly,bitmap);

	SDL_Surface * ort = TTF_RenderText_Blended(GEOR16,CGI->generaltexth->allTexts[4].c_str(),zwykly);
	int curx = bitmap->w/2 - ( skills.size()*44   +   (skills.size()-1)*(36+ort->w) )/2;
	for(int i=0;i<comps.size();i++)
	{
		comps[i]->pos.x = curx+pos.x;
		comps[i]->pos.y = 326+pos.y;
		if( i < (comps.size()-1) )
		{
			curx += 44 + 18; //skill width + margin to "or"
			blitAt(ort,curx,346,bitmap);
			curx += ort->w + 18;
		}
	}
	SDL_FreeSurface(ort);

}
void CLevelWindow::selectionChanged(unsigned to)
{
	for(int i=0;i<comps.size();i++)
		if(i==to)
			comps[i]->select(true);
		else
			comps[i]->select(false);
}
CLevelWindow::~CLevelWindow()
{
	delete ok;
	for(int i=0;i<comps.size();i++)
		delete comps[i];
	SDL_FreeSurface(bitmap);
}
void CLevelWindow::activate()
{
	LOCPLINT->objsToBlit.push_back(this);
	ok->activate();
	for(int i=0;i<comps.size();i++)
		comps[i]->activate();
}
void CLevelWindow::deactivate()
{
	LOCPLINT->objsToBlit.erase(std::find(LOCPLINT->objsToBlit.begin(),LOCPLINT->objsToBlit.end(),this));
	ok->deactivate();
	for(int i=0;i<comps.size();i++)
		comps[i]->deactivate();
}
void CLevelWindow::show(SDL_Surface * to)
{
	blitAt(bitmap,pos.x,pos.y,screen);
	blitAt(graphics->portraitLarge[heroType],170+pos.x,66+pos.y);
	ok->show();
	for(int i=0;i<comps.size();i++)
		comps[i]->show();
}

void CMinorResDataBar::show(SDL_Surface * to)
{
	blitAt(bg,pos.x,pos.y);
	char * buf = new char[15];
	for (int i=0;i<7;i++)
	{
		SDL_itoa(LOCPLINT->cb->getResourceAmount(i),buf,10);
		CSDL_Ext::printAtMiddle(buf,pos.x + 50 + 76*i,pos.y+pos.h/2,GEOR13,zwykly);
	}
	std::vector<std::string> temp;
	SDL_itoa(LOCPLINT->cb->getDate(3),buf,10); temp.push_back(std::string(buf));
	SDL_itoa(LOCPLINT->cb->getDate(2),buf,10); temp.push_back(buf);
	SDL_itoa(LOCPLINT->cb->getDate(1),buf,10); temp.push_back(buf);
	CSDL_Ext::printAtMiddle(CSDL_Ext::processStr(
		CGI->generaltexth->allTexts[62]
	+": %s, "
		+ CGI->generaltexth->allTexts[63]
	+ ": %s, "
		+	CGI->generaltexth->allTexts[64]
	+ ": %s",temp)
		,pos.x+545+(pos.w-545)/2,pos.y+pos.h/2,GEOR13,zwykly);
	temp.clear();
	//updateRect(&pos,screen);
	delete[] buf;
}
CMinorResDataBar::CMinorResDataBar()
{
	bg = BitmapHandler::loadBitmap("Z2ESBAR.bmp");
	SDL_SetColorKey(bg,SDL_SRCCOLORKEY,SDL_MapRGB(bg->format,0,255,255));
	graphics->blueToPlayersAdv(bg,LOCPLINT->playerID);
	pos.x = 7;
	pos.y = 575;
	pos.w = bg->w;
	pos.h = bg->h;
}
CMinorResDataBar::~CMinorResDataBar()
{
	SDL_FreeSurface(bg);
}

SDL_Surface * CCustomImgComponent::getImg()
{
	return bmp;
}

CCustomImgComponent::CCustomImgComponent( Etype Type, int Subtype, int Val, SDL_Surface *sur, bool freeSur )
:bmp(sur), free(freeSur)
{
	init(Type,Subtype,Val);
}

CCustomImgComponent::~CCustomImgComponent()
{
	if(free)
		SDL_FreeSurface(bmp);
}

CMarketplaceWindow::CTradeableItem::CTradeableItem( int Type, int ID, bool Left)
{
	left = Left;
	type = Type;
	id = ID;
}

void CMarketplaceWindow::CTradeableItem::show( SDL_Surface * to/*=NULL*/ )
{
	SDL_Surface *hlp = getSurface();
	blitAt(hlp,pos.x+19,pos.y+9,to);
}

void CMarketplaceWindow::CTradeableItem::clickLeft( boost::logic::tribool down )
{
	CMarketplaceWindow *mw = static_cast<CMarketplaceWindow *>(LOCPLINT->curint->subInt);
	if(down)
	{
		if(left)
		{
			if(mw->hLeft != this)
				mw->hLeft = this;
			else
				return;
		}
		else
		{
			if(mw->hRight != this)
				mw->hRight = this;
			else
				return;
		}
		mw->selectionChanged(left);
	}
}

void CMarketplaceWindow::CTradeableItem::activate()
{
	ClickableL::activate();
}

void CMarketplaceWindow::CTradeableItem::deactivate()
{
	ClickableL::deactivate();
}

SDL_Surface * CMarketplaceWindow::CTradeableItem::getSurface()
{
	switch(type)
	{
	case 0:
		return graphics->resources32->ourImages[id].bitmap;
	case 1:
		return graphics->artDefs->ourImages[id].bitmap;
	default:
		return NULL;
	}
}
void initItems( std::vector<CMarketplaceWindow::CTradeableItem*> &i, std::vector<SDL_Rect> &p, int type, int amount, bool left, std::vector<int> *ids/*=NULL*/ )
{
	i.resize(amount);
	for(int j=0;j<amount;j++)
	{
		i[j] = new CMarketplaceWindow::CTradeableItem(type,(ids && ids->size()>j) ? (*ids)[j] : j, left);
		i[j]->pos = p[j];
	}
}

void CMarketplaceWindow::setMode( int mode )
{
	std::vector<SDL_Rect> lpos, rpos;
	clear();
	switch(mode)
	{
	case 0:
		{
			SDL_Surface *bg2 = BitmapHandler::loadBitmap("TPMRKRES.bmp");
			SDL_SetColorKey(bg2,SDL_SRCCOLORKEY,SDL_MapRGB(bg2->format,0,255,255));
			graphics->blueToPlayersAdv(bg2,LOCPLINT->playerID);
			bg = SDL_ConvertSurface(bg2,screen->format,0); 
			SDL_FreeSurface(bg2);
			lpos += genRect(66,69,39,180), genRect(66,69,122,180), genRect(66,69,204,180),
				genRect(66,69,39,259), genRect(66,69,122,259), genRect(66,69,204,259),
				genRect(66,69,122,338);
			for(int i=0;i<lpos.size();i++)
			{
				lpos[i].x += pos.x;
				lpos[i].y += pos.y;
				rpos.push_back(lpos[i]);
				rpos[rpos.size()-1].x += 288;
			}
			initItems(left,lpos,0,7,true,NULL);
			initItems(right,rpos,0,7,false,NULL);
			printAtMiddle(CGI->generaltexth->allTexts[158],303,28,GEORXX,tytulowy,bg); //title
			printAtMiddle(CGI->generaltexth->allTexts[270],158,148,GEOR13,zwykly,bg); //kingdom res.
			printAtMiddle(CGI->generaltexth->allTexts[168],450,148,GEOR13,zwykly,bg); //available for trade
		}
	}
}

void CMarketplaceWindow::clear()
{
	for(int i=0;i<left.size();i++)
		delete left[i];
	for(int i=0;i<right.size();i++)
		delete right[i];
	left.clear();
	right.clear();
	SDL_FreeSurface(bg);
}

CMarketplaceWindow::CMarketplaceWindow(int Mode)
{
	mode = Mode;
	bg = NULL;
	ok = max = deal = NULL;
	pos.x = screen->w/2 - 300;
	pos.y = screen->h/2 - 296;
	slider = new CSlider(pos.x+231,pos.y+490,137,boost::bind(&CMarketplaceWindow::sliderMoved,this,_1),0,0);
	setMode(mode);
	hLeft = hRight = NULL;
	ok = new AdventureMapButton("","",boost::bind(&CMarketplaceWindow::deactivate,this),pos.x+516,pos.y+520,"IOK6432.DEF");
	ok->callback += boost::bind(&CMarketplaceWindow::clear,this); //clear
	ok->callback += boost::bind(vstd::delObj<CMarketplaceWindow>,this); //will delete
	ok->callback += boost::bind(&CMainInterface::activate,LOCPLINT->curint);
	deal = new AdventureMapButton("","",boost::bind(&CMarketplaceWindow::makeDeal,this),pos.x+307,pos.y+520,"TPMRKB.DEF");
	max = new AdventureMapButton("","",boost::bind(&CMarketplaceWindow::setMax,this),pos.x+229,pos.y+520,"IRCBTNS.DEF");

	max->block(true);
	deal->block(true);
}

CMarketplaceWindow::~CMarketplaceWindow()
{
	delete slider;
}

void CMarketplaceWindow::show( SDL_Surface * to/*=NULL*/ )
{
	blitAt(bg,pos);
	if(hRight)
		CSDL_Ext::drawBorder(screen,hRight->pos.x-1,hRight->pos.y-1,hRight->pos.w+2,hRight->pos.h+2,int3(255,231,148));
	if(hLeft)
		CSDL_Ext::drawBorder(screen,hLeft->pos.x-1,hLeft->pos.y-1,hLeft->pos.w+2,hLeft->pos.h+2,int3(255,231,148));
	ok->show();
	deal->show();
	max->show();
	slider->show();
	for(int i=0;i<left.size();i++)
		left[i]->show();
	for(int i=0;i<right.size();i++)
		right[i]->show();
	if(mode==0)
	{
		char buf[15];
		for(int i=0;i<left.size();i++)
		{
			SDL_itoa(LOCPLINT->cb->getResourceAmount(i),buf,10);
			printAtMiddle(buf,left[i]->pos.x+35,left[i]->pos.y+56,GEOR13,zwykly);
		}
		if(hLeft) //print prices
		{
			for(int i=0; i<right.size();i++)
			{
				if(right[i]->id != hLeft->id)
					printAtMiddle(rSubs[i],right[i]->pos.x+35,right[i]->pos.y+56,GEOR13,zwykly);
				else
					printAtMiddle(CGI->generaltexth->allTexts[164],right[i]->pos.x+35,right[i]->pos.y+56,GEOR13,zwykly);
			}
		}
		if(hLeft && hRight && hLeft->id!= hRight->id)
		{
			blitAt(hLeft->getSurface(),pos.x+142,pos.y+457,screen);
			blitAt(hRight->getSurface(),pos.x+430,pos.y+457,screen);
			SDL_itoa(slider->value * r1,buf,10);
			printAtMiddle(buf,pos.x+158,pos.y+504,GEOR13,zwykly);
			SDL_itoa(slider->value * r2,buf,10);
			printAtMiddle(buf,pos.x+446,pos.y+504,GEOR13,zwykly);
		}
	}
}

void CMarketplaceWindow::activate()
{
	LOCPLINT->objsToBlit += this;
	LOCPLINT->curint->subInt = this;
	for(int i=0;i<left.size();i++)
		left[i]->activate();
	for(int i=0;i<right.size();i++)
		right[i]->activate();
	ok->activate();
	max->activate();
	deal->activate();
	slider->activate();
}

void CMarketplaceWindow::deactivate()
{
	LOCPLINT->objsToBlit -= this;
	LOCPLINT->curint->subInt = NULL;
	for(int i=0;i<left.size();i++)
		left[i]->deactivate();
	for(int i=0;i<right.size();i++)
		right[i]->deactivate();
	ok->deactivate();
	max->deactivate();
	deal->deactivate();
	slider->deactivate();
}

void CMarketplaceWindow::setMax()
{
	slider->moveTo(slider->amount);
}

void CMarketplaceWindow::makeDeal()
{
	LOCPLINT->cb->trade(mode,hLeft->id,hRight->id,slider->value*r1);
	slider->moveTo(0);
	hLeft = NULL;
	selectionChanged(true);
}

void CMarketplaceWindow::sliderMoved( int to )
{

}

void CMarketplaceWindow::selectionChanged(bool side)
{
	if(hLeft && hRight && hLeft->id!= hRight->id)
	{
		LOCPLINT->cb->getMarketOffer(hLeft->id,hRight->id,r1,r2,0);
		slider->amount = LOCPLINT->cb->getResourceAmount(hLeft->id) / r1;
		slider->moveTo(0);
		max->block(false);
		deal->block(false);
	}
	else
	{
		max->block(true);
		deal->block(true);
		slider->amount = 0;
		slider->moveTo(0);
	}
	if(side && hLeft) //left selection changed, recalculate offers
	{
		rSubs.clear();
		rSubs.resize(right.size());
		int h1, h2;
		for(int i=0;i<right.size();i++)
		{
			std::ostringstream oss;
			LOCPLINT->cb->getMarketOffer(hLeft->id,i,h1,h2,0);
			oss << h2;
			if(h1!=1)
				oss << "/" << h1;
			rSubs[i] = oss.str();
		}
	}
}

CSystemOptionsWindow::CSystemOptionsWindow(const SDL_Rect &pos, CPlayerInterface * owner)
{
	this->pos = pos;
	background = BitmapHandler::loadBitmap("SysOpbck.bmp", true);
	graphics->blueToPlayersAdv(background, LOCPLINT->playerID);

	//printing texts
	CSDL_Ext::printAtMiddle(CGI->generaltexth->allTexts[568], 240, 32, GEOR16, tytulowy, background); //window title
	CSDL_Ext::printAtMiddle(CGI->generaltexth->allTexts[569], 122, 65, GEOR16, tytulowy, background); //hero speed
	CSDL_Ext::printAtMiddle(CGI->generaltexth->allTexts[570], 122, 131, GEOR16, tytulowy, background); //enemy speed
	CSDL_Ext::printAtMiddle(CGI->generaltexth->allTexts[571], 122, 197, GEOR16, tytulowy, background); //map scroll speed
	CSDL_Ext::printAtMiddle(CGI->generaltexth->allTexts[20], 122, 263, GEOR16, tytulowy, background); //video quality
	CSDL_Ext::printAtMiddle(CGI->generaltexth->allTexts[394], 122, 348, GEOR16, tytulowy, background); //music volume
	CSDL_Ext::printAtMiddle(CGI->generaltexth->allTexts[395], 122, 414, GEOR16, tytulowy, background); //effects volume

	CSDL_Ext::printAt(CGI->generaltexth->allTexts[572], 283, 57, GEOR16, zwykly, background); //show move path
	CSDL_Ext::printAt(CGI->generaltexth->allTexts[573], 283, 89, GEOR16, zwykly, background); //show hero reminder
	CSDL_Ext::printAt(CGI->generaltexth->allTexts[574], 283, 121, GEOR16, zwykly, background); //quick combat
	CSDL_Ext::printAt(CGI->generaltexth->allTexts[575], 283, 153, GEOR16, zwykly, background); //video subtitles
	CSDL_Ext::printAt(CGI->generaltexth->allTexts[576], 283, 185, GEOR16, zwykly, background); //town building outlines
	CSDL_Ext::printAt(CGI->generaltexth->allTexts[577], 283, 217, GEOR16, zwykly, background); //spell book animation

	//setting up buttons
	quitGame = new AdventureMapButton (CGI->preth->zelp[324].first, CGI->preth->zelp[324].second, boost::bind(&CSystemOptionsWindow::bquitf, this), 405, 471, "soquit.def", false, NULL, false);
	std::swap(quitGame->imgs[0][0], quitGame->imgs[0][1]);
	backToMap = new AdventureMapButton (CGI->preth->zelp[325].first, CGI->preth->zelp[325].second, boost::bind(&CSystemOptionsWindow::breturnf, this), 516, 471, "soretrn.def", false, NULL, false);
	std::swap(backToMap->imgs[0][0], backToMap->imgs[0][1]);

	heroMoveSpeed = new CHighlightableButtonsGroup(0);
	heroMoveSpeed->addButton(boost::assign::map_list_of(0,CGI->preth->zelp[349].first),CGI->preth->zelp[349].second, "sysopb1.def", 187, 134, 1);
	heroMoveSpeed->addButton(boost::assign::map_list_of(0,CGI->preth->zelp[350].first),CGI->preth->zelp[350].second, "sysopb2.def", 235, 134, 2);
	heroMoveSpeed->addButton(boost::assign::map_list_of(0,CGI->preth->zelp[351].first),CGI->preth->zelp[351].second, "sysopb3.def", 283, 134, 4);
	heroMoveSpeed->addButton(boost::assign::map_list_of(0,CGI->preth->zelp[352].first),CGI->preth->zelp[352].second, "sysopb4.def", 331, 134, 8);
	heroMoveSpeed->select(owner->heroMoveSpeed, 1);
	heroMoveSpeed->onChange = boost::bind(&CPlayerInterface::setHeroMoveSpeed, owner, _1);
}

CSystemOptionsWindow::~CSystemOptionsWindow()
{
	SDL_FreeSurface(background);

	delete quitGame;
	delete backToMap;
	delete heroMoveSpeed;
}

void CSystemOptionsWindow::bquitf()
{
	this->deactivate();
	LOCPLINT->showYesNoDialog(CGI->generaltexth->allTexts[578], std::vector<SComponent*>(), boost::bind(exit, 0), boost::bind(&CSystemOptionsWindow::activate, this), false, false);
}

void CSystemOptionsWindow::breturnf()
{
	deactivate();

	for(int g=0; g<LOCPLINT->objsToBlit.size(); ++g)
	{
		if(dynamic_cast<CSystemOptionsWindow*>(LOCPLINT->objsToBlit[g]))
		{
			LOCPLINT->objsToBlit.erase(LOCPLINT->objsToBlit.begin()+g);
			break;
		}
	}

	delete this;
	LOCPLINT->curint->activate();
}

void CSystemOptionsWindow::activate()
{
	quitGame->activate();
	backToMap->activate();
	heroMoveSpeed->activate();
}

void CSystemOptionsWindow::deactivate()
{
	quitGame->deactivate();
	backToMap->deactivate();
	heroMoveSpeed->deactivate();
}

void CSystemOptionsWindow::show(SDL_Surface *to)
{
	//evaluating to
	if(!to)
		to = screen;

	SDL_BlitSurface(background, NULL, to, &pos);

	quitGame->show(to);
	backToMap->show(to);
	heroMoveSpeed->show(to);
}
