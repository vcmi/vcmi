#include "stdafx.h"
#include "CAdvmapInterface.h"
#include "CBattleInterface.h"
#include "CCallback.h"
#include "CCastleInterface.h"
#include "CCursorHandler.h"
#include "CGameInfo.h"
#include "CHeroWindow.h"
#include "CMessage.h"
#include "CPlayerInterface.h"
//#include "SDL_Extensions.h"
#include "SDL_Extensions.h"
//#include "SDL_framerate.h"

#include "SDL_framerate.h"
#include "client/CConfigHandler.h"
#include "client/CCreatureAnimation.h"
#include "client/Graphics.h"
#include "hch/CArtHandler.h"
#include "hch/CGeneralTextHandler.h"
#include "hch/CHeroHandler.h"
#include "hch/CLodHandler.h"
#include "hch/CObjectHandler.h"
#include "lib/Connection.h"
#include "hch/CSpellHandler.h"
#include "hch/CTownHandler.h"
#include "lib/CondSh.h"
#include "lib/NetPacks.h"
#include "map.h"
#include "mapHandler.h"
#include "timeHandler.h"
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/assign/std/vector.hpp> 
#include <boost/assign/list_of.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/thread.hpp>
#include <cmath>
#include <queue>
#include <sstream>
#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif

/*
 * CPlayerInterface.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

using namespace boost::assign;
using namespace CSDL_Ext;

void processCommand(const std::string &message, CClient *&client);

extern TTF_Font * GEOR16;
extern std::queue<SDL_Event*> events;
extern boost::mutex eventsM;

CPlayerInterface * LOCPLINT;
enum  EMoveState {STOP_MOVE, WAITING_MOVE, CONTINUE_MOVE, DURING_MOVE};
CondSh<EMoveState> stillMoveHero; //used during hero movement

struct OCM_HLP_CGIN
{
	bool inline operator ()(const std::pair<const CGObjectInstance*,SDL_Rect>  & a, const std::pair<const CGObjectInstance*,SDL_Rect> & b) const
	{
		return (*a.first)<(*b.first);
	}
} ocmptwo_cgin ;


void KeyShortcut::keyPressed(const SDL_KeyboardEvent & key)
{
	if(vstd::contains(assignedKeys,key.keysym.sym))
	{
		if(key.state == SDL_PRESSED)
			clickLeft(true);
		else
			clickLeft(false);
	}
}

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
					temp = CGI->generaltexth->tcommands[4];
					boost::algorithm::replace_first(temp,"%s",creature->nameSing);
				}
				else if (owner->highlighted->creature == creature)
				{
					temp = CGI->generaltexth->tcommands[2];
					boost::algorithm::replace_first(temp,"%s",creature->nameSing);
				}
				else if (owner->highlighted->creature)
				{
					temp = CGI->generaltexth->tcommands[7];
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
					temp = CGI->generaltexth->tcommands[32];
				}
				else
				{
					temp = CGI->generaltexth->tcommands[12];
				}
				boost::algorithm::replace_first(temp,"%s",creature->nameSing);
			};
		}
		else
		{
			if(owner->highlighted)
			{
				const CArmedInstance *highl = owner->highlighted->getObj(); 
				if(  highl->needsLastStack()		//we are moving stack from hero's
				  && highl->army.slots.size() == 1	//it's only stack
				  && owner->highlighted->upg != upg	//we're moving it to the other garrison
				  )
				{
					temp = CGI->generaltexth->tcommands[5]; //cannot move last stack!
				}
				else
				{
					temp = CGI->generaltexth->tcommands[6];
					boost::algorithm::replace_first(temp,"%s",owner->highlighted->creature->nameSing);
				}
			}
			else
			{
				temp = CGI->generaltexth->tcommands[11];
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

StackState* getStackState(const CGObjectInstance *obj, int pos, bool town)
{
	const CGHeroInstance *h = dynamic_cast<const CGHeroInstance *>(obj);
	if(!h) return NULL;

	StackState *pom = new StackState();
	pom->currentHealth = 0;
	pom->attackBonus = h->getPrimSkillLevel(0);
	pom->defenseBonus = h->getPrimSkillLevel(1);
	pom->luck = h->getCurrentLuck();
	pom->morale = h->getCurrentMorale(pos,town);
	pom->speedBonus = h->valOfBonuses(HeroBonus::STACKS_SPEED);
	return pom;
}

void CGarrisonSlot::clickRight (tribool down)
{
	StackState *pom = getStackState(getObj(),ID, LOCPLINT->topInt() == LOCPLINT->castleInt);
	if(down && creature)
	{
		LOCPLINT->pushInt(new CCreInfoWindow(creature->idNumber,0,count,pom,0,0,NULL));
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
				StackState *pom2 = getStackState(getObj(), ID, LOCPLINT->topInt() == LOCPLINT->castleInt);
				UpgradeInfo pom = LOCPLINT->cb->getUpgradeInfo(getObj(), ID);

				CCreInfoWindow *creWindow = NULL;
				if(pom.oldID>=0) //upgrade is possible
				{

					creWindow = new CCreInfoWindow(
						creature->idNumber,1,count,pom2,
						boost::bind(&CCallback::upgradeCreature,LOCPLINT->cb,getObj(),ID,pom.newID[0]), //bind upgrade function
						boost::bind(&CCallback::dismissCreature,LOCPLINT->cb,getObj(),ID),&pom);
				}
				else
				{
					creWindow = new CCreInfoWindow(
						creature->idNumber,1,count,pom2,0, 
						boost::bind(&CCallback::dismissCreature,LOCPLINT->cb,getObj(),ID), NULL);
				}

				LOCPLINT->pushInt(creWindow);

				owner->highlighted = NULL;
				show(screen2);
				refr = true;
				delete pom2;
			}
			else if((owner->splitting || LOCPLINT->shiftPressed())
				&& (!creature
					|| (creature == owner->highlighted->creature))
			) //we want to split
			{
				owner->p2 = ID; //store the second stack pos
				owner->pb = upg;//store the second stack owner (up or down army)
				owner->splitting = false;

				int totalAmount = owner->highlighted->count;
				if(creature) 
					totalAmount += count;

				int last = -1;
				if(upg != owner->highlighted->upg) //not splitting within same army
				{
					if(owner->highlighted->getObj()->army.slots.size() == 1
						&& owner->highlighted->getObj()->needsLastStack() )
					{
						last = 0;
					}
					if(getObj()->army.slots.size() == 1
						&& getObj()->needsLastStack() )
					{
						last += 2;
					}
				}


				CSplitWindow * spw = new CSplitWindow(owner->highlighted->creature->idNumber, totalAmount, owner, last, count);
				LOCPLINT->pushInt(spw);
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
			show(screen2);
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
void CGarrisonSlot::show(SDL_Surface * to)
{
	if(creature)
	{
		char buf[15];
		SDL_itoa(count,buf,10);
		blitAt(graphics->bigImgs[creature->idNumber],pos,to);
		printToWR(buf,pos.x+56,pos.y+62,GEOR16,zwykly,to);

		if((owner->highlighted==this)
			|| (owner->splitting && owner->highlighted->creature == creature))
		{
			blitAt(graphics->bigImgs[-1],pos,to);
		}
	}
	else //empty slot
	{
		SDL_Rect jakis1 = genRect(pos.h,pos.w,owner->offx+ID*(pos.w+owner->interx),owner->offy+upg*(pos.h+owner->intery)), 
			jakis2 = pos;
		SDL_BlitSurface(owner->sur,&jakis1,to,&jakis2);
		if(owner->splitting)
			blitAt(graphics->bigImgs[-1],pos,to);
	}
}
CGarrisonInt::~CGarrisonInt()
{
	if(sup)
	{
		for(size_t i=0;i<sup->size();i++)
		{
			delete (*sup)[i];
		}
		delete sup;
	}
	if(sdown)
	{
		for(size_t i=0;i<sdown->size();i++)
		{
			delete (*sdown)[i]; //XXX what about smartpointers? boost or auto_ptr from std
		}
		delete sdown;
	}
}

void CGarrisonInt::show(SDL_Surface * to)
{
	if(sup)
	{
		for(size_t i = 0; i<sup->size(); i++)
		{
			if((*sup)[i])
			{
				(*sup)[i]->show(to);
			}
		}
	}
	if(sdown)
	{
		for(size_t i = 0; i<sdown->size(); i++)
		{
			if((*sdown)[i])
			{
				(*sdown)[i]->show(to);
			}
		}
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
				new CGarrisonSlot(this, pos.x + (i->first*(58+interx)), pos.y,i->first, 0, 
									&CGI->creh->creatures[i->second.first],i->second.second);
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
				new CGarrisonSlot(this, pos.x + (i->first*(58+interx)), pos.y + 64 + intery,i->first,1, 
									&CGI->creh->creatures[i->second.first],i->second.second);
		}
		for(int i=0; i<sdown->size(); i++)
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
		//show(screen2);
	}
}
void CGarrisonInt::splitClick()
{
	if(!highlighted)
		return;
	splitting = !splitting;
	show(screen2);
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
CGarrisonInt::CGarrisonInt(int x, int y, int inx, int iny, SDL_Surface *&pomsur, int OX, int OY, const CArmedInstance *s1, 
						   const CArmedInstance *s2)
	:interx(inx),intery(iny),highlighted(NULL),sur(pomsur),offx(OX),offy(OY),sup(NULL),
	 sdown(NULL),oup(s1),odown(s2)
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

CInfoWindow::CInfoWindow(std::string text, int player, int charperline, const std::vector<SComponent*> &comps, std::vector<std::pair<std::string,CFunctionList<void()> > > &Buttons, bool delComps)
{
	ID = -1;
	this->delComps = delComps;
	for(int i=0;i<Buttons.size();i++)
	{
		buttons.push_back(new AdventureMapButton("","",boost::bind(&CInfoWindow::close,this),0,0,Buttons[i].first));
		buttons[i]->callback.add(Buttons[i].second); //each button will close the window apart from call-defined actions
	}

	buttons.front()->assignedKeys.insert(SDLK_RETURN); //first button - reacts on enter
	buttons.back()->assignedKeys.insert(SDLK_ESCAPE); //last button - reacts on escape

	for(int i=0;i<comps.size();i++)
	{
		components.push_back(comps[i]);
	}
	CMessage::drawIWindow(this,text,player,charperline);
}
CInfoWindow::CInfoWindow() 
{
	ID = -1;
	delComps = false;
}
void CInfoWindow::close()
{
	LOCPLINT->popIntTotally(this);
	LOCPLINT->showingDialog->setn(false);
}
void CInfoWindow::show(SDL_Surface * to)
{
	CSimpleWindow::show(to);
	for(int i=0;i<buttons.size();i++)
		buttons[i]->show(to);
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
}

void CRClickPopup::deactivate()
{
	ClickableR::deactivate();
}

void CRClickPopup::close()
{
	LOCPLINT->popIntTotally(this);
}

CInfoPopup::CInfoPopup(SDL_Surface * Bitmap, int x, int y, bool Free)
 :free(Free),bitmap(Bitmap)
{
	pos.x = x;
	pos.y = y;
	pos.h = bitmap->h;
	pos.w = bitmap->w;
}
void CInfoPopup::close()
{
	if(free)
		SDL_FreeSurface(bitmap);
	LOCPLINT->popIntTotally(this);
}
void CInfoPopup::show(SDL_Surface * to)
{
	blitAt(bitmap,pos.x,pos.y,to);
}

void SComponent::init(Etype Type, int Subtype, int Val)
{
	std::ostringstream oss;
	switch (Type)
	{
	case artifact:
		description = CGI->arth->artifacts[Subtype].Description();
		subtitle = CGI->arth->artifacts[Subtype].Name();
		break;
	case primskill:
		oss << ((Val>0)?("+"):("-")) << Val << " ";
		if(Subtype < 4)
		{
			description = CGI->generaltexth->arraytxt[2+Subtype];
			oss << CGI->generaltexth->primarySkillNames[Subtype];
		}
		else if(Subtype == 5) //spell points
		{
			description = CGI->generaltexth->allTexts[149];
			oss <<  CGI->generaltexth->allTexts[387];
		}
		else
		{
			tlog1 << "Wrong subtype=" << Subtype << std::endl;
		}
		subtitle = oss.str();
		break;
	case secskill44: case secskill:
		subtitle += CGI->generaltexth->levels[Val-1] + " " + CGI->generaltexth->skillName[Subtype];
		description = CGI->generaltexth->skillInfoTexts[Subtype][Val-1];
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
	case creature:
		subtitle = boost::lexical_cast<std::string>(Val) + " " + CGI->creh->creatures[Subtype].*(val != 1 ? &CCreature::namePl : &CCreature::nameSing);
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
	if(!temp)
	{
		tlog1 << "Error: cannot find graphic for component with id=" << type << " subid=" << subtype << " val=" << val << std::endl;
		return;
	}
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
	else if(c.id == Component::SPELL)
		init(spell,c.subtype,c.val);
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
		return graphics->abils44->ourImages[subtype*3 + 3 + val - 1].bitmap;
		break;
	case secskill:
		return graphics->abils82->ourImages[subtype*3 + 3 + val - 1].bitmap;
		break;
	case resource:
		return graphics->resources->ourImages[subtype].bitmap;
		break;
	case experience:
		return graphics->pskillsb->ourImages[4].bitmap;
		break;
	case morale:
		return graphics->morale82->ourImages[val+3].bitmap;
		break;
	case luck:
		return graphics->luck82->ourImages[val+3].bitmap;
		break;
	case spell:
		return graphics->spellscr->ourImages[subtype].bitmap;
		break;
	case creature:
		return graphics->bigImgs[subtype];
	}
	return NULL;
}
void SComponent::clickRight (tribool down)
{
	if(description.size())
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
			SDL_PutPixelWithoutRefresh(border,i,0,239,215,123);
			SDL_PutPixelWithoutRefresh(border,i,(border->h)-1,239,215,123);
		}
		for (int i=0;i<border->h;i++)
		{
			SDL_PutPixelWithoutRefresh(border,0,i,239,215,123);
			SDL_PutPixelWithoutRefresh(border,(border->w)-1,i,239,215,123);
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
	KeyInterested::activate();
	SComponent::activate();
	ClickableL::activate();
}
void CSelectableComponent::deactivate()
{
	KeyInterested::deactivate();
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
CSelWindow::CSelWindow(const std::string &text, int player, int charperline, const std::vector<CSelectableComponent*> &comps, const std::vector<std::pair<std::string,CFunctionList<void()> > > &Buttons, int askID)
{
	ID = askID;
	for(int i=0;i<Buttons.size();i++)
	{
		buttons.push_back(new AdventureMapButton("","",Buttons[i].second,0,0,Buttons[i].first));
		if(!i  &&  askID >= 0)
			buttons.back()->callback += boost::bind(&CSelWindow::madeChoice,this);
		buttons[i]->callback += boost::bind(&CInfoWindow::close,this); //each button will close the window apart from call-defined actions
	}

	buttons.front()->assignedKeys.insert(SDLK_RETURN); //first button - reacts on enter
	buttons.back()->assignedKeys.insert(SDLK_ESCAPE); //last button - reacts on escape

	if(buttons.size() > 1  &&  askID >= 0) //cancel button functionality
		buttons.back()->callback += boost::bind(&ICallback::selectionMade,LOCPLINT->cb,0,askID);

	for(int i=0;i<comps.size();i++)
	{
		components.push_back(comps[i]);
		comps[i]->onSelect = boost::bind(&CSelWindow::selectionChange,this,i);
		if(i<9)
			comps[i]->assignedKeys.insert(SDLK_1+i);
	}
	CMessage::drawIWindow(this,text,player,charperline);
}

void CSelWindow::madeChoice()
{
	if(ID < 0)
		return;
	int ret = -1;
	for (int i=0;i<components.size();i++)
	{
		if(dynamic_cast<CSelectableComponent*>(components[i])->selected)
		{
			ret = i;
		}
	}
	LOCPLINT->cb->selectionMade(ret+1,ID);
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
	int img = std::min(state+bitmapOffset,int(imgs[curimg].size()-1));
	img = std::max(0, img);

	if (abs)
	{
		blitAt(imgs[curimg][img],pos.x,pos.y,to);
	}
	else
	{
		blitAt(imgs[curimg][img],pos.x+ourObj->pos.x,pos.y+ourObj->pos.y,to);
	}
}

ClickableL::ClickableL()
{
	pressedL=false;
}

ClickableL::~ClickableL()
{
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

ClickableR::~ClickableR()
{}

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
//ClickableR

Hoverable::~Hoverable()
{}

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
//Hoverable

KeyInterested::~KeyInterested()
{}

void KeyInterested::activate()
{
	LOCPLINT->keyinterested.push_front(this);
}
void KeyInterested::deactivate()
{
	LOCPLINT->
		keyinterested.erase(std::find(LOCPLINT->keyinterested.begin(),LOCPLINT->keyinterested.end(),this));
}
//KeyInterested

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
	curAction = NULL;
	playerID=Player;
	serialID=serial;
	human=true;
	castleInt = NULL;
	adventureInt = NULL;
	battleInt = NULL;
	pim = new boost::recursive_mutex;
	makingTurn = false;
	showingDialog = new CondSh<bool>(false);
	heroMoveSpeed = 2;
	mapScrollingSpeed = 2;
	//initializing framerate keeper
	mainFPSmng = new FPSmanager;
	SDL_initFramerate(mainFPSmng);
	SDL_setFramerate(mainFPSmng, 48);
	//framerate keeper initialized
	cingconsole = new CInGameConsole;
}
CPlayerInterface::~CPlayerInterface()
{
	delete pim;
	delete showingDialog;
	delete mainFPSmng;
	delete adventureInt;
	delete cingconsole;

	for(std::map<int,SDL_Surface*>::iterator i=graphics->heroWins.begin(); i!= graphics->heroWins.end(); i++)
		SDL_FreeSurface(i->second);
	for(std::map<int,SDL_Surface*>::iterator i=graphics->townWins.begin(); i!= graphics->townWins.end(); i++)
		SDL_FreeSurface(i->second);
}
void CPlayerInterface::init(ICallback * CB)
{
	cb = dynamic_cast<CCallback*>(CB);
	adventureInt = new CAdvMapInt(playerID);
	std::vector<const CGTownInstance*> tt = cb->getTownsInfo(false);
	for(int i=0;i<tt.size();i++)
	{
		SDL_Surface * pom = infoWin(tt[i]);
		graphics->townWins.insert(std::pair<int,SDL_Surface*>(tt[i]->id,pom));
	}
}
void CPlayerInterface::yourTurn()
{
	try
	{
		LOCPLINT = this;
		makingTurn = true;

		static int autosaveCount = 0;
		LOCPLINT->cb->save("Autosave_" + boost::lexical_cast<std::string>(autosaveCount++ + 1));
		autosaveCount %= 5;

		for(std::map<int,SDL_Surface*>::iterator i=graphics->heroWins.begin(); i!=graphics->heroWins.end();i++) //redraw hero infoboxes
			SDL_FreeSurface(i->second);
		graphics->heroWins.clear();
		std::vector <const CGHeroInstance *> hh = cb->getHeroesInfo(false);
		for(int i=0;i<hh.size();i++)
		{
			SDL_Surface * pom = infoWin(hh[i]);
			graphics->heroWins.insert(std::pair<int,SDL_Surface*>(hh[i]->subID,pom));
		}

		/* TODO: This isn't quite right. First day in game should play
		 * NEWDAY. And we don't play NEWMONTH. */
		int day = cb->getDate(1);
		if (day != 1)
			CGI->audioh->playSound(soundBase::newDay);
		else
			CGI->audioh->playSound(soundBase::newWeek);

		adventureInt->infoBar.newDay(day);

		//select first hero if available.
		//TODO: check if hero is slept
		if(adventureInt->heroList.items.size())
			adventureInt->select(adventureInt->heroList.items[0].first);
		else
			adventureInt->select(adventureInt->townList.items[0]);

		adventureInt->showAll(screen);
		pushInt(adventureInt);
		adventureInt->KeyInterested::activate();

		timeHandler th;
		th.getDif();
		while(makingTurn) // main loop
		{

			updateWater();
			pim->lock();


			//if there are any waiting dialogs, show them
			if(dialogs.size() && !showingDialog->get())
			{
				showingDialog->set(true);
				pushInt(dialogs.front());
				dialogs.pop_front();
			}

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

			while(true)
			{
				SDL_Event *ev = NULL;
				{
					boost::unique_lock<boost::mutex> lock(eventsM);
					if(!events.size())
					{
						break;
					}
					else
					{
						ev = events.front();
						events.pop();
					}
				}
				handleEvent(ev);
				delete ev;
			}

			if(!adventureInt->active && adventureInt->scrollingDir) //player force map scrolling though interface is disabled
			{
				totalRedraw();
			}
			else
			{
				//update only top interface and draw background
				if(objsToBlit.size() > 1)
					blitAt(screen2,0,0,screen); //blit background
				objsToBlit.back()->show(screen); //blit active interface/window
			}

			CGI->curh->draw1();
			CSDL_Ext::update(screen);
			CGI->curh->draw2();
			pim->unlock();
			SDL_framerateDelay(mainFPSmng);
		}

		adventureInt->KeyInterested::deactivate();
		popInt(adventureInt);

		cb->endTurn();
	} HANDLE_EXCEPTION
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

	adventureInt->centerOn(details.ho->pos); //actualizing screen pos
	adventureInt->minimap.draw(screen2);
	adventureInt->heroList.draw(screen2);

	if(details.style>0  ||  details.src == details.dst)
		return;

	//initializing objects and performing first step of move
	const CGHeroInstance * ho = details.ho; //object representing this hero
	int3 hp = details.src;
	if (!details.successful) //hero failed to move
	{
		if(ho->movement > 50)
			ho->moveDir = getDir(details.src,details.dst);
		ho->isStanding = true;

		if(ho->movement)
		{
			delete adventureInt->terrain.currentPath;
			adventureInt->terrain.currentPath = NULL;
			adventureInt->heroList.items[adventureInt->heroList.getPosOfHero(ho)].second = NULL;
		}
		stillMoveHero.setn(STOP_MOVE);
		return;
	}

	if (adventureInt->terrain.currentPath) //&& hero is moving
	{
		adventureInt->terrain.currentPath->nodes.erase(adventureInt->terrain.currentPath->nodes.end()-1);
		if(!adventureInt->terrain.currentPath->nodes.size())
		{

			delete adventureInt->terrain.currentPath;
			adventureInt->terrain.currentPath = NULL;
			adventureInt->heroList.items[adventureInt->heroList.getPosOfHero(ho)].second = NULL;
		}
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
			//setting advmap shift
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
			//setting advmap shift
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
			//setting advmap shift
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
			//setting advmap shift
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
			
			//setting advmap shift
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
			//setting advmap shift
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
			//setting advmap shift
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
			//setting advmap shift
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
		adventureInt->show(screen);
		//LOCPLINT->adventureInt->show(); //updating screen
		CSDL_Ext::update(screen);

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
	adventureInt->minimap.draw(screen2);
	adventureInt->heroList.updateMove(ho);

	//check if user cancelled movement
	{
		boost::unique_lock<boost::mutex> un(eventsM);
		while(events.size())
		{
			SDL_Event *ev = events.front();
			events.pop();
			switch(ev->type)
			{
			case SDL_MOUSEBUTTONDOWN:
				stillMoveHero.setn(STOP_MOVE);
				break;
			case SDL_KEYDOWN:
				if(ev->key.keysym.sym < SDLK_F1)
					stillMoveHero.setn(STOP_MOVE);
				break;
			}
			delete ev;
		}
	}

	if(stillMoveHero.get() == 1)
		stillMoveHero.setn(DURING_MOVE);

}
void CPlayerInterface::heroKilled(const CGHeroInstance* hero)
{
	boost::unique_lock<boost::recursive_mutex> un(*pim);
	graphics->heroWins.erase(hero->ID);
	adventureInt->heroList.updateHList(hero);
}
void CPlayerInterface::heroCreated(const CGHeroInstance * hero)
{
	boost::unique_lock<boost::recursive_mutex> un(*pim);
	if(graphics->heroWins.find(hero->subID)==graphics->heroWins.end())
		graphics->heroWins.insert(std::pair<int,SDL_Surface*>(hero->subID,infoWin(hero)));
	adventureInt->heroList.updateHList();
}
void CPlayerInterface::openTownWindow(const CGTownInstance * town)
{
	castleInt = new CCastleInterface(town);
	CGI->audioh->playMusic(castleInt->musicID, -1);
	LOCPLINT->pushInt(castleInt);
}

SDL_Surface * CPlayerInterface::infoWin(const CGObjectInstance * specific) //specific=0 => draws info about selected town/hero
{
	if (specific)
	{
		switch (specific->ID)
		{
		case HEROI_TYPE:
			return graphics->drawHeroInfoWin(dynamic_cast<const CGHeroInstance*>(specific));
			break;
		case TOWNI_TYPE:
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
			tlog1 << "Strange... selection is neither hero nor town\n";
			return NULL;
		}
	}
}

void CPlayerInterface::handleMouseMotion(SDL_Event *sEvent)
{
	//sending active, hovered hoverable objects hover() call
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

	//sending active, MotionInterested objects mouseMoved() call
	std::list<MotionInterested*> miCopy = motioninterested;
	for(std::list<MotionInterested*>::iterator i=miCopy.begin(); i != miCopy.end();i++)
	{
		if ((*i)->strongInterest || isItIn(&(*i)->pos,sEvent->motion.x,sEvent->motion.y))
		{
			(*i)->mouseMoved(sEvent->motion);
		}
	}

	//adventure map scrolling with mouse
	if(!SDL_GetKeyState(NULL)[SDLK_LCTRL])
	{
		if(sEvent->motion.x<15)
		{
			adventureInt->scrollingDir |= CAdvMapInt::LEFT;
		}
		else
		{
			adventureInt->scrollingDir &= ~CAdvMapInt::LEFT;
		}
		if(sEvent->motion.x>screen->w-15)
		{
			adventureInt->scrollingDir |= CAdvMapInt::RIGHT;
		}
		else
		{
			adventureInt->scrollingDir &= ~CAdvMapInt::RIGHT;
		}
		if(sEvent->motion.y<15)
		{
			adventureInt->scrollingDir |= CAdvMapInt::UP;
		}
		else
		{
			adventureInt->scrollingDir &= ~CAdvMapInt::UP;
		}
		if(sEvent->motion.y>screen->h-15)
		{
			adventureInt->scrollingDir |= CAdvMapInt::DOWN;
		}
		else
		{
			adventureInt->scrollingDir &= ~CAdvMapInt::DOWN;
		}
	}
}
void CPlayerInterface::handleEvent(SDL_Event *sEvent)
{
	current = sEvent;

	if (sEvent->type==SDL_KEYDOWN || sEvent->type==SDL_KEYUP)
	{
		SDL_KeyboardEvent key = sEvent->key;

		//translate numpad keys
		if (key.keysym.sym >= SDLK_KP0  && key.keysym.sym <= SDLK_KP9)
		{
			key.keysym.sym = (SDLKey) (key.keysym.sym - SDLK_KP0 + SDLK_0);
		}
		else if(key.keysym.sym == SDLK_KP_ENTER)
		{
			key.keysym.sym = (SDLKey)SDLK_RETURN;
		}

		bool keysCaptured = false;
		for(std::list<KeyInterested*>::iterator i=keyinterested.begin(); i != keyinterested.end();i++)
		{
			if((*i)->captureAllKeys)
			{
				keysCaptured = true;
				break;
			}
		}

		std::list<KeyInterested*> miCopy = keyinterested;
		for(std::list<KeyInterested*>::iterator i=miCopy.begin(); i != miCopy.end();i++)
			if(vstd::contains(keyinterested,*i) && (!keysCaptured || (*i)->captureAllKeys))
				(**i).keyPressed(key);
	}
	else if(sEvent->type==SDL_MOUSEMOTION)
	{
		CGI->curh->cursorMove(sEvent->motion.x, sEvent->motion.y);
		handleMouseMotion(sEvent);
	}
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
	if(which >= PRIMARY_SKILLS) //no need to redraw infowin if this is experience (exp is treated as prim skill with id==4)
		return;
	boost::unique_lock<boost::recursive_mutex> un(*pim);
	redrawHeroWin(hero);
}
void CPlayerInterface::heroManaPointsChanged(const CGHeroInstance * hero)
{
	boost::unique_lock<boost::recursive_mutex> un(*pim);
	redrawHeroWin(hero);
}
void CPlayerInterface::heroMovePointsChanged(const CGHeroInstance * hero)
{
	boost::unique_lock<boost::recursive_mutex> un(*pim);
	//adventureInt->heroList.draw();
}
void CPlayerInterface::receivedResource(int type, int val)
{
	boost::unique_lock<boost::recursive_mutex> un(*pim);
	LOCPLINT->totalRedraw();
}

void CPlayerInterface::heroGotLevel(const CGHeroInstance *hero, int pskill, std::vector<ui16>& skills, boost::function<void(ui32)> &callback)
{
	{
		boost::unique_lock<boost::mutex> un(showingDialog->mx);
		while(showingDialog->data)
			showingDialog->cond.wait(un);
	}

	CGI->audioh->playSound(soundBase::heroNewLevel);

	boost::unique_lock<boost::recursive_mutex> un(*pim);
	CLevelWindow *lw = new CLevelWindow(hero,pskill,skills,callback);
	LOCPLINT->pushInt(lw);
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

	CCastleInterface *c = castleInt;
	if(c)
	{
		c->garr->highlighted = NULL;
		c->hslotup.hero = town->garrisonHero;
		c->garr->odown = c->hslotdown.hero = town->visitingHero;
		c->garr->set2 = town->visitingHero ? &town->visitingHero->army : NULL;
		c->garr->recreateSlots();
	}
	LOCPLINT->totalRedraw();
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
	if(obj->ID == HEROI_TYPE) //hero
	{
		const CGHeroInstance * hh;
		if(hh = dynamic_cast<const CGHeroInstance*>(obj))
		{
			SDL_FreeSurface(graphics->heroWins[hh->subID]);
			graphics->heroWins[hh->subID] = infoWin(hh);
		}
	}
	else if (obj->ID == TOWNI_TYPE) //town
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

	}

	bool wasGarrison = false;
	for(std::list<IShowActivable*>::iterator i = listInt.begin(); i != listInt.end(); i++)
	{
		if((*i)->type & IShowActivable::WITH_GARRISON)
		{
			CWindowWithGarrison *wwg = static_cast<CWindowWithGarrison*>(*i);
			wwg->garr->recreateSlots();
			wasGarrison = true;
		}
	}

	LOCPLINT->totalRedraw();
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
	if(!castleInt)
		return;
	if(castleInt->town!=town)
		return;
	switch(what)
	{
	case 1:
		CGI->audioh->playSound(soundBase::newBuilding);
		castleInt->addBuilding(buildingID);
		break;
	case 2:
		castleInt->removeBuilding(buildingID);
		break;
	}
}

void CPlayerInterface::battleStart(CCreatureSet *army1, CCreatureSet *army2, int3 tile, CGHeroInstance *hero1, CGHeroInstance *hero2, bool side) //called by engine when battle starts; side=0 - left, side=1 - right
{
	while(showingDialog->get())
		SDL_Delay(20);

	boost::unique_lock<boost::recursive_mutex> un(*pim);
	battleInt = new CBattleInterface(army1, army2, hero1, hero2, genRect(600, 800, (conf.cc.resx - 800)/2, (conf.cc.resy - 600)/2));
	CGI->audioh->playMusicFromSet(CGI->audioh->battleMusics, -1);
	pushInt(battleInt);
}

void CPlayerInterface::battlefieldPrepared(int battlefieldType, std::vector<CObstacle*> obstacles) //called when battlefield is prepared, prior the battle beginning
{
}

void CPlayerInterface::battleNewRound(int round) //called at the beggining of each turn, round=-1 is the tactic phase, round=0 is the first "normal" turn
{
	boost::unique_lock<boost::recursive_mutex> un(*pim);
	battleInt->newRound(round);
}

void CPlayerInterface::actionStarted(const BattleAction* action)
{
	boost::unique_lock<boost::recursive_mutex> un(*pim);
	curAction = new BattleAction(*action);
	if( (action->actionType==2 || (action->actionType==6 && action->destinationTile!=cb->battleGetPos(action->stackNumber))) )
	{
		battleInt->moveStarted = true;
		if(battleInt->creAnims[action->stackNumber]->framesInGroup(20))
		{
			battleInt->creAnims[action->stackNumber]->setType(20);
		}
	}


	battleInt->deactivate();

	CStack *stack = cb->battleGetStackByID(action->stackNumber);
	char txt[400];

	if(action->actionType == 1)
	{
		if(action->side)
			battleInt->defendingHero->setPhase(4);
		else
			battleInt->attackingHero->setPhase(4);
		return;
	}
	if(!stack)
	{
		tlog1<<"Something wrong with stackNumber in actionStarted. Stack number: "<<action->stackNumber<<std::endl;
		return;
	}

	int txtid = 0;
	switch(action->actionType)
	{
	case 3: //defend
		txtid = 120;
		break;
	case 8: //wait
		txtid = 136;
		break;
	case 11: //bad morale
		txtid = -34; //negative -> no separate singular/plural form		
		battleInt->displayEffect(30,stack->position);
		break;
	}

	if(txtid > 0  &&  stack->amount != 1)
		txtid++; //move to plural text
	else if(txtid < 0)
		txtid = -txtid;

	if(txtid)
	{
		sprintf(txt, CGI->generaltexth->allTexts[txtid].c_str(),  (stack->amount != 1) ? stack->creature->namePl.c_str() : stack->creature->nameSing.c_str(), 0);
		LOCPLINT->battleInt->console->addText(txt);
	}
}

void CPlayerInterface::actionFinished(const BattleAction* action)
{
	boost::unique_lock<boost::recursive_mutex> un(*pim);
	delete curAction;
	curAction = NULL;
	//if((action->actionType==2 || (action->actionType==6 && action->destinationTile!=cb->battleGetPos(action->stackNumber)))) //activating interface when move is finished
	{
		battleInt->activate();
	}
	if(action->actionType == 1)
	{
		if(action->side)
			battleInt->defendingHero->setPhase(0);
		else
			battleInt->attackingHero->setPhase(0);
	}
}

BattleAction CPlayerInterface::activeStack(int stackID) //called when it's turn of that stack
{
	CBattleInterface *b = battleInt;
	{
		boost::unique_lock<boost::recursive_mutex> un(*pim);

		CStack *stack = cb->battleGetStackByID(stackID);
		if(vstd::contains(stack->state,MOVED)) //this stack has moved and makes second action -> high morale
		{
			std::string hlp = CGI->generaltexth->allTexts[33];
			boost::algorithm::replace_first(hlp,"%s",(stack->amount != 1) ? stack->creature->namePl : stack->creature->nameSing);
			battleInt->displayEffect(20,stack->position);
			battleInt->console->addText(hlp);
		}

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
	battleInt->battleFinished(*br);
}

void CPlayerInterface::battleStackMoved(int ID, int dest, int distance, bool end)
{
	boost::unique_lock<boost::recursive_mutex> un(*pim);
	battleInt->stackMoved(ID, dest, end, distance);
}
void CPlayerInterface::battleSpellCast(SpellCast *sc)
{
	boost::unique_lock<boost::recursive_mutex> un(*pim);
	battleInt->spellCast(sc);
}
void CPlayerInterface::battleStacksEffectsSet(SetStackEffect & sse)
{
	boost::unique_lock<boost::recursive_mutex> un(*pim);
	battleInt->battleStacksEffectsSet(sse);
}
void CPlayerInterface::battleStacksAttacked(std::set<BattleStackAttacked> & bsa)
{
	tlog5 << "CPlayerInterface::battleStackAttacked - locking...";
	boost::unique_lock<boost::recursive_mutex> un(*pim);
	tlog5 << "done!\n";


	std::vector<CBattleInterface::SStackAttackedInfo> arg;
	for(std::set<BattleStackAttacked>::iterator i = bsa.begin(); i != bsa.end(); i++)
	{
		if(i->isEffect() && i->effect != 12) //and not armageddon
		{
			battleInt->displayEffect(i->effect, cb->battleGetStackByID(i->stackAttacked)->position);
		}
		CBattleInterface::SStackAttackedInfo to_put = {i->stackAttacked, i->damageAmount, i->killedAmount, LOCPLINT->curAction->stackNumber, LOCPLINT->curAction->actionType==7, i->killed()};
		arg.push_back(to_put);
	}

	if(bsa.begin()->isEffect() && bsa.begin()->effect == 12) //for armageddon - I hope this condition is enough
	{
		battleInt->displayEffect(bsa.begin()->effect, -1);
	}

	battleInt->stacksAreAttacked(arg);
}
void CPlayerInterface::battleAttack(BattleAttack *ba)
{
	tlog5 << "CPlayerInterface::battleAttack - locking...";
	boost::unique_lock<boost::recursive_mutex> un(*pim);
	tlog5 << "done!\n";
	if(ba->lucky()) //lucky hit
	{
		CStack *stack = cb->battleGetStackByID(ba->stackAttacking);
		std::string hlp = CGI->generaltexth->allTexts[45];
		boost::algorithm::replace_first(hlp,"%s",(stack->amount != 1) ? stack->creature->namePl.c_str() : stack->creature->nameSing.c_str());
		battleInt->console->addText(hlp);
		battleInt->displayEffect(18,stack->position);
	}
	//TODO: bad luck?

	if(ba->shot())
	{
		for(std::set<BattleStackAttacked>::iterator i = ba->bsa.begin(); i != ba->bsa.end(); i++)
			battleInt->stackIsShooting(ba->stackAttacking,cb->battleGetPos(i->stackAttacked));
	}
	else
	{
		CStack * attacker = cb->battleGetStackByID(ba->stackAttacking);
		int shift = 0;		
		if(ba->counter() && BattleInfo::mutualPosition(curAction->destinationTile, attacker->position) < 0)		
		{			
			if(attacker->attackerOwned)				
				shift = 1;			
			else				
				shift = -1;		
		}		
		battleInt->stackAttacking( ba->stackAttacking, ba->counter() ? curAction->destinationTile + shift : curAction->additionalInfo );	
	}
}

void CPlayerInterface::showComp(SComponent comp)
{
	boost::unique_lock<boost::recursive_mutex> un(*pim);

	CGI->audioh->playSoundFromSet(CGI->audioh->pickupSounds);

	adventureInt->infoBar.showComp(&comp,4000);
}

void CPlayerInterface::showInfoDialog(const std::string &text, const std::vector<Component*> &components, int soundID)
{
	std::vector<SComponent*> intComps;
	for(int i=0;i<components.size();i++)
		intComps.push_back(new SComponent(*components[i]));
	showInfoDialog(text,intComps,soundID);
}

void CPlayerInterface::showInfoDialog(const std::string &text, const std::vector<SComponent*> & components, int soundID)
{
	{
		boost::unique_lock<boost::mutex> un(showingDialog->mx);
		while(showingDialog->data)
			showingDialog->cond.wait(un);
	}

	boost::unique_lock<boost::recursive_mutex> un(*pim);
	
	if(stillMoveHero.get() == DURING_MOVE)//if we are in the middle of hero movement
		stillMoveHero.setn(STOP_MOVE); //after showing dialog movement will be stopped

	std::vector<std::pair<std::string,CFunctionList<void()> > > pom;
	pom.push_back(std::pair<std::string,CFunctionList<void()> >("IOKAY.DEF",0));
	CInfoWindow * temp = new CInfoWindow(text,playerID,36,components,pom,false);

	if(makingTurn && listInt.size())
	{
		CGI->audioh->playSound(static_cast<soundBase::soundID>(soundID));
		showingDialog->set(true);
		pushInt(temp);
	}
	else
	{
		dialogs.push_back(temp);
	}
}

void CPlayerInterface::showYesNoDialog(const std::string &text, const std::vector<SComponent*> & components, CFunctionList<void()> onYes, CFunctionList<void()> onNo, bool DelComps)
{
	boost::unique_lock<boost::recursive_mutex> un(*pim);
	LOCPLINT->showingDialog->setn(true);
	std::vector<std::pair<std::string,CFunctionList<void()> > > pom;
	pom.push_back(std::pair<std::string,CFunctionList<void()> >("IOKAY.DEF",0));
	pom.push_back(std::pair<std::string,CFunctionList<void()> >("ICANCEL.DEF",0));
	CInfoWindow * temp = new CInfoWindow(text,playerID,36,components,pom,DelComps);
	temp->delComps = DelComps;
	for(int i=0;i<onYes.funcs.size();i++)
		temp->buttons[0]->callback += onYes.funcs[i];
	for(int i=0;i<onNo.funcs.size();i++)
		temp->buttons[1]->callback += onNo.funcs[i];

	LOCPLINT->pushInt(temp);
}

void CPlayerInterface::showBlockingDialog( const std::string &text, const std::vector<Component> &components, ui32 askID, int soundID, bool selection, bool cancel )
{
	boost::unique_lock<boost::recursive_mutex> un(*pim);

	CGI->audioh->playSound(static_cast<soundBase::soundID>(soundID));

	if(!selection && cancel) //simple yes/no dialog
	{
		std::vector<SComponent*> intComps;
		for(int i=0;i<components.size();i++)
			intComps.push_back(new SComponent(components[i])); //will be deleted by close in window

		showYesNoDialog(text,intComps,boost::bind(&CCallback::selectionMade,cb,1,askID),boost::bind(&CCallback::selectionMade,cb,0,askID),true);
	}
	else if(selection)
	{
		std::vector<CSelectableComponent*> intComps;
		for(int i=0;i<components.size();i++)
			intComps.push_back(new CSelectableComponent(components[i])); //will be deleted by CSelWindow::close

		std::vector<std::pair<std::string,CFunctionList<void()> > > pom;
		pom.push_back(std::pair<std::string,CFunctionList<void()> >("IOKAY.DEF",0));
		if(cancel)
		{
			pom.push_back(std::pair<std::string,CFunctionList<void()> >("ICANCEL.DEF",0));
		}

		CSelWindow * temp = new CSelWindow(text,playerID,35,intComps,pom,askID);
		pushInt(temp);
		intComps[0]->clickLeft(true);
	}

}

void CPlayerInterface::tileRevealed(const std::set<int3> &pos)
{
	boost::unique_lock<boost::recursive_mutex> un(*pim);
	for(std::set<int3>::const_iterator i=pos.begin(); i!=pos.end();i++)
		adventureInt->minimap.showTile(*i);
}

void CPlayerInterface::tileHidden(const std::set<int3> &pos)
{
	boost::unique_lock<boost::recursive_mutex> un(*pim);
	for(std::set<int3>::const_iterator i=pos.begin(); i!=pos.end();i++)
		adventureInt->minimap.hideTile(*i);
}

void CPlayerInterface::openHeroWindow(const CGHeroInstance *hero)
{
	boost::unique_lock<boost::recursive_mutex> un(*pim);
	adventureInt->heroWindow->setHero(hero);
	adventureInt->heroWindow->quitButton->callback = boost::bind(&CHeroWindow::quit,adventureInt->heroWindow);
	pushInt(adventureInt->heroWindow);
}

void CPlayerInterface::heroArtifactSetChanged(const CGHeroInstance*hero)
{
	boost::unique_lock<boost::recursive_mutex> un(*pim);
	if(adventureInt->heroWindow->curHero)
	{
		adventureInt->heroWindow->deactivate();
		adventureInt->heroWindow->setHero(adventureInt->heroWindow->curHero);
		adventureInt->heroWindow->activate();
	}
}

void CPlayerInterface::updateWater()
{

}

void CPlayerInterface::availableCreaturesChanged( const CGTownInstance *town )
{
	boost::unique_lock<boost::recursive_mutex> un(*pim);
	if(castleInt)
	{
		CFortScreen *fs = dynamic_cast<CFortScreen*>(listInt.front());
		if(fs)
			fs->draw(castleInt,false);
	}
}

void CPlayerInterface::heroBonusChanged( const CGHeroInstance *hero, const HeroBonus &bonus, bool gain )
{
	if(bonus.type == HeroBonus::NONE)	return;
	boost::unique_lock<boost::recursive_mutex> un(*pim);
	redrawHeroWin(hero);
}

template <typename Handler> void CPlayerInterface::serializeTempl( Handler &h, const int version )
{
	h & playerID & serialID;
	h & heroMoveSpeed & mapScrollingSpeed;
	h & CBattleInterface::settings;
}

void CPlayerInterface::serialize( COSer<CSaveFile> &h, const int version )
{
	serializeTempl(h,version);
}

void CPlayerInterface::serialize( CISer<CLoadFile> &h, const int version )
{
	serializeTempl(h,version);
}

void CPlayerInterface::redrawHeroWin(const CGHeroInstance * hero)
{
	if(!vstd::contains(graphics->heroWins,hero->subID))
	{
		tlog1 << "Cannot redraw infowindow for hero with subID=" << hero->subID << " - not present in our map\n";
		return;
	}
	SDL_FreeSurface(graphics->heroWins[hero->subID]);
	graphics->heroWins[hero->subID] = infoWin(hero); 
	if (adventureInt->selection == hero)
		adventureInt->infoBar.draw(screen);
}

bool CPlayerInterface::moveHero( const CGHeroInstance *h, CPath path )
{
	if (!h)
		return false; //can't find hero

	bool result = false;
	path.convert(0);
	boost::unique_lock<boost::mutex> un(stillMoveHero.mx);
	stillMoveHero.data = CONTINUE_MOVE;

	enum TerrainTile::EterrainType currentTerrain = TerrainTile::border; // not init yet
	enum TerrainTile::EterrainType newTerrain;
	int sh = -1;

	for(int i=path.nodes.size()-1; i>0 && stillMoveHero.data == CONTINUE_MOVE; i--)
	{
		// Start a new sound for the hero movement or let the existing one carry on.
#if 0
		// TODO
		if (hero is flying && sh == -1)
			sh = CGI->audioh->playSound(soundBase::horseFlying, -1);
		} 
		else if (hero is in a boat && sh = -1) {
			sh = CGI->audioh->playSound(soundBase::sound_todo, -1);
		} else
#endif
		{
			newTerrain = cb->getTileInfo(path.nodes[i].coord)->tertype;

			if (newTerrain != currentTerrain) {
				CGI->audioh->stopSound(sh);
				sh = CGI->audioh->playSound(CGI->audioh->horseSounds[newTerrain], -1);
				currentTerrain = newTerrain;
			}
		}

		stillMoveHero.data = WAITING_MOVE;

		int3 endpos(path.nodes[i-1].coord.x, path.nodes[i-1].coord.y, h->pos.z);
		cb->moveHero(h,endpos);
		while(stillMoveHero.data != STOP_MOVE  &&  stillMoveHero.data != CONTINUE_MOVE)
			stillMoveHero.cond.wait(un);
	}

	CGI->audioh->stopSound(sh);

	//stillMoveHero = false;
	return result;
}

bool CPlayerInterface::shiftPressed() const
{
	return SDL_GetKeyState(NULL)[SDLK_LSHIFT]  ||  SDL_GetKeyState(NULL)[SDLK_RSHIFT];
}

void CPlayerInterface::showGarrisonDialog( const CArmedInstance *up, const CGHeroInstance *down, boost::function<void()> &onEnd )
{
	{
		boost::unique_lock<boost::mutex> un(showingDialog->mx);
		while(showingDialog->data)
			showingDialog->cond.wait(un);
	}

	boost::unique_lock<boost::recursive_mutex> un(*pim);
	while(dialogs.size())
	{
		pim->unlock();
		SDL_Delay(20);
		pim->lock();
	}
	CGarrisonWindow *cgw = new CGarrisonWindow(up,down);
	cgw->quit->callback += onEnd;
	pushInt(cgw);
}

void CPlayerInterface::popInt( IShowActivable *top )
{
	assert(listInt.front() == top);
	top->deactivate();
	listInt.pop_front();
	objsToBlit -= top;
	if(listInt.size())
		listInt.front()->activate();
	totalRedraw();
}

void CPlayerInterface::popIntTotally( IShowActivable *top )
{
	assert(listInt.front() == top);
	popInt(top);
	delete top;
}

void CPlayerInterface::pushInt( IShowActivable *newInt )
{
	if(listInt.size())
		listInt.front()->deactivate();
	listInt.push_front(newInt);
	newInt->activate();
	objsToBlit += newInt;
	LOCPLINT->totalRedraw();
}

void CPlayerInterface::popInts( int howMany )
{
	if(!howMany) return; //senseless but who knows...

	assert(listInt.size() > howMany);
	listInt.front()->deactivate();
	for(int i=0; i < howMany; i++)
	{
		objsToBlit -= listInt.front();
		delete listInt.front();
		listInt.pop_front();
	}
	listInt.front()->activate();
	totalRedraw();
}

IShowActivable * CPlayerInterface::topInt()
{
	if(!listInt.size())
		return NULL;
	else 
		return listInt.front();
}

void CPlayerInterface::totalRedraw()
{
	for(int i=0;i<objsToBlit.size();i++)
		objsToBlit[i]->showAll(screen2);

	blitAt(screen2,0,0,screen);

	if(objsToBlit.size())
		objsToBlit.back()->showAll(screen);
}

void CPlayerInterface::requestRealized( PackageApplied *pa )
{
	if(stillMoveHero.get() == DURING_MOVE)
		stillMoveHero.setn(CONTINUE_MOVE);
}

CStatusBar::CStatusBar(int x, int y, std::string name, int maxw)
{
	bg=BitmapHandler::loadBitmap(name);
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
	if(LOCPLINT->cingconsole->enteredText == "") //for appropriate support for in-game console
	{
		current="";
		show(screen);
	}
}

void CStatusBar::print(const std::string & text)
{
	if(LOCPLINT->cingconsole->enteredText == "" || text == LOCPLINT->cingconsole->enteredText) //for appropriate support for in-game console
	{
		current=text;
		show(screen);
	}
}

void CStatusBar::show(SDL_Surface * to)
{
	SDL_Rect pom = genRect(pos.h,pos.w,pos.x,pos.y);
	SDL_BlitSurface(bg,&genRect(pos.h,pos.w,0,0),to,&pom);
	printAtMiddle(current,middlex,middley,GEOR13,zwykly,to);
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
	arrup = CDefHandler::giveDef(conf.go()->ac.hlistAU);
	arrdo = CDefHandler::giveDef(conf.go()->ac.hlistAD);
	mobile = CDefHandler::giveDef(conf.go()->ac.hlistMB);
	mana = CDefHandler::giveDef(conf.go()->ac.hlistMN);
	empty = BitmapHandler::loadBitmap("HPSXXX.bmp");
	selection = BitmapHandler::loadBitmap("HPSYYY.bmp");
	SDL_SetColorKey(selection,SDL_SRCCOLORKEY,SDL_MapRGB(selection->format,0,255,255));

	pos = genRect(32*SIZE+arrup->height+arrdo->height, std::max(arrup->width,arrdo->width), conf.go()->ac.hlistX, conf.go()->ac.hlistY);

	arrupp = genRect(arrup->height, arrup->width, pos.x, pos.y);
	arrdop = genRect(arrdo->height, arrdo->width, pos.x, pos.y+32*SIZE+arrup->height);
 //32px per hero
	posmobx = pos.x+1;
	posmoby = pos.y+arrup->height+1;
	posporx = pos.x+mobile->width+2;
	pospory = pos.y+arrup->height;
	posmanx = pos.x+1+50+mobile->width;
	posmany = pos.y+arrup->height+1;

	from = 0;
	selected = -1;
	pressed = indeterminate;
}

void CHeroList::init()
{
	int w = pos.w+1, h = pos.h+4;
	bg = CSDL_Ext::newSurface(w,h,screen);
	SDL_BlitSurface(LOCPLINT->adventureInt->bg,&genRect(w,h,pos.x,pos.y),bg,&genRect(w,h,0,0));
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
		draw(screen);
		LOCPLINT->adventureInt->infoBar.draw(screen);
	}
	if (which>=items.size())
		return;
	selected = which;

	//recalculationg path in case of something has changed on map
	if(items[which].second)
	{
		CPath * newPath = LOCPLINT->cb->getPath(items[which].second->startPos(), items[which].second->endPos(), items[which].first);
		delete items[which].second;
		LOCPLINT->adventureInt->terrain.currentPath = items[which].second = newPath;
	}
	else
	{
		LOCPLINT->adventureInt->terrain.currentPath = NULL;
	}
	LOCPLINT->adventureInt->select(items[which].first);
	//recalculated and assigned
}

void CHeroList::clickLeft(tribool down)
{
	if (down)
	{
		/***************************ARROWS*****************************************/
		if(isItIn(&arrupp,LOCPLINT->current->motion.x,LOCPLINT->current->motion.y))
		{
			if(from>0)
			{
				blitAt(arrup->ourImages[1].bitmap,arrupp.x,arrupp.y);
				pressed = true;
			}
			return;
		}
		else if(isItIn(&arrdop,LOCPLINT->current->motion.x,LOCPLINT->current->motion.y))
		{
			if(items.size()-from>SIZE)
			{
				blitAt(arrdo->ourImages[1].bitmap,arrdop.x,arrdop.y);
				pressed = false;
			}
			return;
		}
		/***************************HEROES*****************************************/
		int hx = LOCPLINT->current->motion.x, hy = LOCPLINT->current->motion.y;
		hx-=pos.x;
		hy-=pos.y; hy-=arrup->ourImages[0].bitmap->h;
		int ny = hy/32;
		if (ny>=SIZE || ny<0)
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
				draw(screen);
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

				draw(screen);
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
			LOCPLINT->adventureInt->statusbar.print(CGI->generaltexth->zelp[303].first);
		else
			LOCPLINT->adventureInt->statusbar.clear();
		return;
	}
	else if(isItIn(&arrdop,LOCPLINT->current->motion.x,LOCPLINT->current->motion.y))
	{
		if ((items.size()-from)  >  SIZE)
			LOCPLINT->adventureInt->statusbar.print(CGI->generaltexth->zelp[304].first);
		else
			LOCPLINT->adventureInt->statusbar.clear();
		return;
	}
	//if not buttons then heroes
	int hx = LOCPLINT->current->motion.x, hy = LOCPLINT->current->motion.y;
	hx-=pos.x;
	hy-=pos.y; hy-=arrup->ourImages[0].bitmap->h;
	int ny = hy/32;
	if ((ny>SIZE || ny<0) || (from+ny>=items.size()))
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
			LOCPLINT->adventureInt->handleRightClick(CGI->generaltexth->zelp[303].second,down,this);
		}
		else if(isItIn(&arrdop,LOCPLINT->current->motion.x,LOCPLINT->current->motion.y) && (items.size()-from>5))
		{
			LOCPLINT->adventureInt->handleRightClick(CGI->generaltexth->zelp[304].second,down,this);
		}
		else
		{
			//if not buttons then heroes
			int hx = LOCPLINT->current->motion.x, hy = LOCPLINT->current->motion.y;
			hx-=pos.x;
			hy-=pos.y; hy-=arrup->ourImages[0].bitmap->h;
			int ny = hy/32;
			if ((ny>SIZE || ny<0) || (from+ny>=items.size()))
			{
				return;
			}

			//show popup
			CInfoPopup * ip = new CInfoPopup(graphics->heroWins[items[from+ny].first->subID],
				LOCPLINT->current->motion.x-graphics->heroWins[items[from+ny].first->subID]->w,
				LOCPLINT->current->motion.y-graphics->heroWins[items[from+ny].first->subID]->h,
				false);
			LOCPLINT->pushInt(ip);
		}
	}
	else
	{
		LOCPLINT->adventureInt->handleRightClick(CGI->generaltexth->zelp[303].second,down,this);
		LOCPLINT->adventureInt->handleRightClick(CGI->generaltexth->zelp[304].second,down,this);
	}
}

void CHeroList::hover (bool on)
{
}

void CHeroList::keyPressed (const SDL_KeyboardEvent & key)
{
}

void CHeroList::updateHList(const CGHeroInstance *toRemove)
{
	if(toRemove) //remove specific hero
	{
		for (std::vector<std::pair<const CGHeroInstance*, CPath *> >::iterator i=items.begin(); i != items.end(); i++)
		{
			if(i->first == toRemove)
			{
				delete i->second;
				items.erase(i);
				break;
			}
		}
	}
	else
	{
		items.clear();
		genList();
	}
	if(selected>=items.size())
		select(items.size()-1);
	if(items.size()==0)
		LOCPLINT->adventureInt->townList.select(0);
	else
		select(selected);
}

void CHeroList::updateMove(const CGHeroInstance* which) //draws move points bar
{
	int ser = -1;
	for(int i=0; i<items.size() && ser<0; i++)
		if(items[i].first->subID == which->subID)
			ser = i;
	ser -= from;
	if(ser<0 || ser > SIZE) return;
	int pom = std::min((which->movement)/100,(si32)mobile->ourImages.size()-1);
	blitAt(mobile->ourImages[pom].bitmap,posmobx,posmoby+ser*32); //move point
}

void CHeroList::draw(SDL_Surface * to)
{
	for (int iT=0+from;iT<SIZE+from;iT++)
	{
		int i = iT-from;
		if (iT>=items.size())
		{
			blitAt(mobile->ourImages[0].bitmap,posmobx,posmoby+i*32,to);
			blitAt(mana->ourImages[0].bitmap,posmanx,posmany+i*32,to);
			blitAt(empty,posporx,pospory+i*32,to);
			continue;
		}
		const CGHeroInstance *cur = items[iT].first;
		int pom = cur->movement / 100;
		if (pom>25) pom=25;
		if (pom<0) pom=0;
		blitAt(mobile->ourImages[pom].bitmap,posmobx,posmoby+i*32,to); //move point
		pom = cur->mana / 5;
		if (pom>25) pom=25;
		if (pom<0) pom=0;
		blitAt(mana->ourImages[pom].bitmap,posmanx,posmany+i*32,to); //mana
		SDL_Surface * temp = graphics->portraitSmall[cur->portrait];
		blitAt(temp,posporx,pospory+i*32,to);
		if ((selected == iT) && (LOCPLINT->adventureInt->selection->ID == HEROI_TYPE))
		{
			blitAt(selection,posporx,pospory+i*32,to);
		}
		//TODO: support for custom portraits
	}
	if (from>0)
		blitAt(arrup->ourImages[0].bitmap,arrupp.x,arrupp.y,to);
	else
		blitAt(arrup->ourImages[2].bitmap,arrupp.x,arrupp.y,to);

	if (items.size()-from > SIZE)
		blitAt(arrdo->ourImages[0].bitmap,arrdop.x,arrdop.y,to);
	else
		blitAt(arrdo->ourImages[2].bitmap,arrdop.x,arrdop.y,to);
}

int CHeroList::getPosOfHero(const CArmedInstance* h)
{
	return vstd::findPos(
		items,h,
		boost::bind(vstd::equal<std::pair<const CGHeroInstance*, CPath *>,
		const CArmedInstance *,const CGHeroInstance*>,_1,
		&std::pair<const CGHeroInstance*, CPath *>::first,_2));
}


CTownList::~CTownList()
{
	delete arrup;
	delete arrdo;
}

CTownList::CTownList(int Size, int x, int y, std::string arrupg, std::string arrdog)
:CList(Size)
{
	arrup = CDefHandler::giveDef(arrupg);
	arrdo = CDefHandler::giveDef(arrdog);
	pos.x = x;
	pos.y = y;
	pos.w = std::max(arrdo->width, arrup->width);
	pos.h = arrdo->height + arrup->height + Size*32;

	arrupp.x=x;
	arrupp.y=y;
	arrupp.w=arrup->width;
	arrupp.h=arrup->height;
	arrdop.x=x;
	arrdop.y=y+arrup->height+32*SIZE;
	arrdop.w=arrdo->width;
	arrdop.h=arrdo->height;
	posporx = arrdop.x;
	pospory = arrupp.y + arrupp.h;

	pressed = indeterminate;

	from = 0;
	selected = -1;
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
			LOCPLINT->statusbar->print(CGI->generaltexth->zelp[306].first);
		else
			LOCPLINT->statusbar->clear();
		return;
	}
	else if(isItIn(&arrdop,LOCPLINT->current->motion.x,LOCPLINT->current->motion.y))
	{
		if ((items.size()-from)  >  SIZE)
			LOCPLINT->statusbar->print(CGI->generaltexth->zelp[307].first);
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
		if(isItIn(&arrupp,LOCPLINT->current->motion.x,LOCPLINT->current->motion.y))
		{
			if(from>0)
			{
				blitAt(arrup->ourImages[1].bitmap,arrupp.x,arrupp.y);
				pressed = true;
			}
			return;
		}
		else if(isItIn(&arrdop,LOCPLINT->current->motion.x,LOCPLINT->current->motion.y))
		{
			if(items.size()-from > SIZE)
			{
				blitAt(arrdo->ourImages[1].bitmap,arrdop.x,arrdop.y);
				pressed = false;
			}
			return;
		}
		/***************************TOWNS*****************************************/
		int hx = LOCPLINT->current->motion.x, hy = LOCPLINT->current->motion.y;
		hx-=pos.x;
		hy-=pos.y; hy-=arrup->ourImages[0].bitmap->h;
		int ny = hy/32;
		if (ny>SIZE || ny<0)
			return;
		if(LOCPLINT->topInt() == LOCPLINT->adventureInt
		  && (ny+from)==selected 
		  && LOCPLINT->adventureInt->selection->ID == TOWNI_TYPE
		  )
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

				draw(screen2);
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

				draw(screen2);
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
			LOCPLINT->adventureInt->handleRightClick(CGI->generaltexth->zelp[306].second,down,this);
		}
		else if(isItIn(&arrdop,LOCPLINT->current->motion.x,LOCPLINT->current->motion.y) && (items.size()-from>5))
		{
			LOCPLINT->adventureInt->handleRightClick(CGI->generaltexth->zelp[307].second,down,this);
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
		CInfoPopup * ip = new CInfoPopup(
			graphics->townWins[items[from+ny]->id],
			LOCPLINT->current->motion.x-graphics->townWins[items[from+ny]->id]->w,
			LOCPLINT->current->motion.y-graphics->townWins[items[from+ny]->id]->h,
			false);
		LOCPLINT->pushInt(ip);
	}
	else
	{
			LOCPLINT->adventureInt->handleRightClick(CGI->generaltexth->zelp[306].second,down,this);
			LOCPLINT->adventureInt->handleRightClick(CGI->generaltexth->zelp[307].second,down,this);
	}
}

void CTownList::hover (bool on)
{
}

void CTownList::keyPressed (const SDL_KeyboardEvent & key)
{
}

void CTownList::draw(SDL_Surface * to)
{
	for (int iT=0+from;iT<SIZE+from;iT++)
	{
		int i = iT-from;
		if (iT>=items.size())
		{
			blitAt(graphics->getPic(-1),posporx,pospory+i*32,to);
			continue;
		}

		blitAt(graphics->getPic(items[iT]->subID,items[iT]->hasFort(),items[iT]->builded),posporx,pospory+i*32,to);

		if ((selected == iT) && (LOCPLINT->adventureInt->selection->ID == TOWNI_TYPE))
		{
			blitAt(graphics->getPic(-2),posporx,pospory+i*32,to);
		}
	}
	if (from>0)
		blitAt(arrup->ourImages[0].bitmap,arrupp.x,arrupp.y,to);
	else
		blitAt(arrup->ourImages[2].bitmap,arrupp.x,arrupp.y,to);

	if (items.size()-from>SIZE)
		blitAt(arrdo->ourImages[0].bitmap,arrdop.x,arrdop.y,to);
	else
		blitAt(arrdo->ourImages[2].bitmap,arrdop.x,arrdop.y,to);
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
		blitAt(graphics->backgrounds[c->faction],x,y,to);//curx-50,pos.y+130-65);
		dst = genRect(130,100,x,y);
	}
	else
	{
		blitAt(graphics->backgroundsm[c->faction],x,y,to);//curx-50,pos.y+130-65);
		dst = genRect(120,100,x,y);
	}
	if(c->isDoubleWide())
		x-=15;
	return anim->nextFrameMiddle(to,x+78,y+(big ? 55 : 45),true,0,nextFrame,false,false,&dst);
}
SDL_Surface * CCreaturePic::getPic(bool nextFrame)
{
	//TODO: write
	return NULL;
}
void CRecrutationWindow::close()
{
	LOCPLINT->popIntTotally(this);
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
	buy->block(!to);
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
void CRecrutationWindow::clickRight( boost::logic::tribool down )
{
	if(down)
	{
		int curx = 192 + 51 - (102*creatures.size()/2) - (18*(creatures.size()-1)/2);
		for(int i=0;i<creatures.size();i++)
		{
			if(isItIn(&genRect(132,102,pos.x+curx,pos.y+64),LOCPLINT->current->motion.x,LOCPLINT->current->motion.y))
			{
				CCreInfoWindow *popup = new CCreInfoWindow(creatures[i].ID,0,0,NULL,NULL,NULL,NULL);
				LOCPLINT->pushInt(popup);
				break;
			}
			curx += 120;
		}
	}
}

void CRecrutationWindow::activate()
{
	ClickableL::activate();
	ClickableR::activate();
	buy->activate();
	max->activate();
	cancel->activate();
	slider->activate();
	LOCPLINT->statusbar = bar;
}

void CRecrutationWindow::deactivate()
{
	ClickableL::deactivate();
	ClickableR::deactivate();
	buy->deactivate();
	max->deactivate();
	cancel->deactivate();
	slider->deactivate();
}

void CRecrutationWindow::show(SDL_Surface * to)
{
	static char animCounter=0; //animation counter - for determining appropriate animation frame to be shown
	blitAt(bitmap,pos.x,pos.y,to);
	buy->show(to);
	max->show(to);
	cancel->show(to);
	slider->show(to);

	char pom[15];
	SDL_itoa(creatures[which].amount-slider->value,pom,10); //available
	printAtMiddle(pom,pos.x+205,pos.y+252,GEOR13,zwykly,to);
	SDL_itoa(slider->value,pom,10); //recruit
	printAtMiddle(pom,pos.x+279,pos.y+252,GEOR13,zwykly,to);
	printAtMiddle(CGI->generaltexth->allTexts[16] + " " + CGI->creh->creatures[creatures[which].ID].namePl,pos.x+243,pos.y+32,GEOR16,tytulowy,to); //eg "Recruit Dragon flies"

	int curx = pos.x+115-creatures[which].res.size()*16;
	for(int i=0;i<creatures[which].res.size();i++)
	{
		blitAt(graphics->resources32->ourImages[creatures[which].res[i].first].bitmap,curx,pos.y+243,to);
		blitAt(graphics->resources32->ourImages[creatures[which].res[i].first].bitmap,curx+258,pos.y+243,to);
		SDL_itoa(creatures[which].res[i].second,pom,10);
		printAtMiddle(pom,curx+12,pos.y+286,GEOR13,zwykly,to);
		SDL_itoa(creatures[which].res[i].second * slider->value,pom,10);
		printAtMiddle(pom,curx+12+258,pos.y+286,GEOR13,zwykly,to);
		curx+=32;
	}

	curx = pos.x + 192 + 102 - (102*creatures.size()/2) - (18*(creatures.size()-1)/2);
	for(int i=0; i<creatures.size(); ++i)
	{
		creatures[i].pic->blitPic(to, curx-50, pos.y+130-65, !(animCounter%4));
		curx += 120;
	}

	++animCounter;
	bar->show(to);
}

CRecrutationWindow::CRecrutationWindow(const std::vector<std::pair<int,int> > &Creatures, const boost::function<void(int,int)> &Recruit) //creatures - pairs<creature_ID,amount>
:recruit(Recruit), which(0)
{
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
	bitmap = SDL_ConvertSurface(hhlp,screen->format,0); //na 8bitowej mapie by sie psulo //it wouldn't work on 8bpp map
	SDL_SetColorKey(bitmap,SDL_SRCCOLORKEY,SDL_MapRGB(bitmap->format,0,255,255));
	SDL_FreeSurface(hhlp);
	pos.x = screen->w/2 - bitmap->w/2;
	pos.y = screen->h/2 - bitmap->h/2;
	pos.w = bitmap->w;
	pos.h = bitmap->h;
	bar = new CStatusBar(pos.x+8, pos.y+370, "APHLFTRT.bmp", 471);
	max = new AdventureMapButton(CGI->generaltexth->zelp[553],boost::bind(&CRecrutationWindow::Max,this),pos.x+134,pos.y+313,"IRCBTNS.DEF",SDLK_m);
	buy = new AdventureMapButton(CGI->generaltexth->zelp[554],boost::bind(&CRecrutationWindow::Buy,this),pos.x+212,pos.y+313,"IBY6432.DEF",SDLK_RETURN);
	cancel = new AdventureMapButton(CGI->generaltexth->zelp[555],boost::bind(&CRecrutationWindow::Cancel,this),pos.x+290,pos.y+313,"ICN6432.DEF",SDLK_ESCAPE);
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

	if(!creatures[0].amount ||  !amounts[0])
	{
		max->block(true);
		slider->block(true);
	}
	//buy->block(true); //not needed, will be blocked by initing slider on 0
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
	delete bar;
}

CSplitWindow::CSplitWindow(int cid, int max, CGarrisonInt *Owner, int Last, int val)
{
	last = Last;
	which = 1;
	c=cid;
	slider = NULL;
	gar = Owner;
	bitmap = BitmapHandler::loadBitmap("GPUCRDIV.bmp");
	graphics->blueToPlayersAdv(bitmap,LOCPLINT->playerID);
	SDL_SetColorKey(bitmap,SDL_SRCCOLORKEY,SDL_MapRGB(bitmap->format,0,255,255));
	pos.x = screen->w/2 - bitmap->w/2;
	pos.y = screen->h/2 - bitmap->h/2;
	pos.w = bitmap->w;
	pos.h = bitmap->h;
	ok = new AdventureMapButton("","",boost::bind(&CSplitWindow::split,this),pos.x+20,pos.y+263,"IOK6432.DEF",SDLK_RETURN);
	cancel = new AdventureMapButton("","",boost::bind(&CSplitWindow::close,this),pos.x+214,pos.y+263,"ICN6432.DEF",SDLK_ESCAPE);
	int sliderPositions = max - (last>=0) - (last==2);
	slider = new CSlider(pos.x+21,pos.y+194,257,boost::bind(&CSplitWindow::sliderMoved,this,_1),1,sliderPositions,val,true);
	a1 = max-val;
	a2 = val;
	anim = new CCreaturePic(&CGI->creh->creatures[cid]);
	anim->anim->setType(1);

	std::string title = CGI->generaltexth->allTexts[256];
	boost::algorithm::replace_first(title,"%s",CGI->creh->creatures[cid].namePl);
	printAtMiddle(title,150,34,GEOR16,tytulowy,bitmap);
}

CSplitWindow::~CSplitWindow() //d-tor
{
	SDL_FreeSurface(bitmap);
	delete ok;
	delete cancel;
	delete slider;
	delete anim;
}

void CSplitWindow::activate()
{
	ClickableL::activate();
	KeyInterested::activate();
	ok->activate();
	cancel->activate();
	slider->activate();
}

void CSplitWindow::deactivate()
{
	ClickableL::deactivate();
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
	LOCPLINT->popIntTotally(this);
}

void CSplitWindow::sliderMoved(int to)
{
	int all = a1+a2;
	a2 = to + (last==1 || last==2);
	if(slider)
		a1 = all - a2;
}

void CSplitWindow::show(SDL_Surface * to)
{
	blitAt(bitmap,pos.x,pos.y,to);
	ok->show(to);
	cancel->show(to);
	slider->show(to);
	printAtMiddle(boost::lexical_cast<std::string>(a1) + (!which ? "_" : ""),pos.x+70,pos.y+237,GEOR16,zwykly,to);
	printAtMiddle(boost::lexical_cast<std::string>(a2) + (which ? "_" : ""),pos.x+233,pos.y+237,GEOR16,zwykly,to);
	anim->blitPic(to,pos.x+20,pos.y+54,false);
	anim->blitPic(to,pos.x+177,pos.y+54,false);
}

void CSplitWindow::keyPressed (const SDL_KeyboardEvent & key)
{
	if(key.state != SDL_PRESSED)
		return;

	int &cur = (which ? a2 : a1), 
		&sec = (which ? a1 : a2), 
		ncur = cur;
	if (key.keysym.sym == SDLK_BACKSPACE)
	{
		ncur /= 10;
	}
	else if(key.keysym.sym == SDLK_TAB)
	{
		which = !which;
	}
	else
	{
		int number = key.keysym.sym - SDLK_0;
		if (number < 0   ||   number > 9) //not a number pressed
		{
			return;
		}
		ncur = cur*10 + number;
	}
	int delta = ncur - cur;
	if(delta > sec)
	{
		cur += sec;
		sec = 0;
	}
	slider->moveTo(which ? ncur : a1+a2-ncur);
}

void CSplitWindow::clickLeft( boost::logic::tribool down )
{
	if(down)
	{
		Point click(LOCPLINT->current->motion.x,LOCPLINT->current->motion.y);
		click -= pos.topLeft();
		if(Rect(19,216,105,40).isIn(click)) //left picture
			which = 0;
		else if(Rect(175,216,105,40).isIn(click)) //right picture
			which = 1;
	}
}


void CCreInfoWindow::show(SDL_Surface * to)
{
	char pom[15];
	blitAt(bitmap,pos.x,pos.y,to);
	anim->blitPic(to,pos.x+21,pos.y+48,(type) && !(anf%4));
	if(++anf==4) 
		anf=0;
	if(count.size())
		printTo(count.c_str(),pos.x+114,pos.y+174,GEOR16,zwykly,to);
	if(upgrade)
		upgrade->show(to);
	if(dismiss)
		dismiss->show(to);
	if(ok)
		ok->show(to);
}

CCreInfoWindow::CCreInfoWindow(int Cid, int Type, int creatureCount, StackState *State, boost::function<void()> Upg, boost::function<void()> Dsm, UpgradeInfo *ui)
:type(Type),dsm(Dsm),dismiss(0),upgrade(0),ok(0)
{
	//active = false;
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
	if(!type) anim->anim->setType(2);

	char pom[75];int hlp=0;

	if(creatureCount)
	{
		SDL_itoa(creatureCount,pom,10);
		count = pom;
	}

	printAtMiddle(c->namePl,149,30,GEOR13,tytulowy,bitmap); //creature name

	//atttack
	printAt(CGI->generaltexth->primarySkillNames[0],155,48,GEOR13,zwykly,bitmap);
	SDL_itoa(c->attack,pom,10);
	if(State && State->attackBonus)
	{
		int hlp;
		if(c->attack > 0)
			hlp = log10f(c->attack)+2;
		else
			hlp = 2;
		pom[hlp-1] = ' '; pom[hlp] = '(';
		SDL_itoa(c->attack+State->attackBonus,pom+hlp+1,10);
		hlp += 2+(int)log10f(State->attackBonus+c->attack);
		pom[hlp] = ')'; pom[hlp+1] = '\0';
	}
	printToWR(pom,276,61,GEOR13,zwykly,bitmap);

	//defense
	printAt(CGI->generaltexth->primarySkillNames[1],155,67,GEOR13,zwykly,bitmap);
	SDL_itoa(c->defence,pom,10);
	if(State && State->defenseBonus)
	{
		int hlp;
		if(c->defence > 0)
			hlp = log10f(c->defence)+2;
		else
			hlp = 2;
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
		if(State)
			sprintf(pom,"%d(%d)",c->shots,State->shotsLeft);
		else
			SDL_itoa(c->shots,pom,10);
		printToWR(pom,276,99,GEOR13,zwykly,bitmap);
	}

	//damage
	printAt(CGI->generaltexth->allTexts[199],155,105,GEOR13,zwykly,bitmap);
	SDL_itoa(c->damageMin,pom,10);
	if(c->damageMin > 0)
		hlp = log10f(c->damageMin)+2;
	else
		hlp = 2;
	pom[hlp-1]=' '; pom[hlp]='-'; pom[hlp+1]=' ';
	SDL_itoa(c->damageMax,pom+hlp+2,10);
	printToWR(pom,276,118,GEOR13,zwykly,bitmap);

	//health
	printAt(CGI->generaltexth->allTexts[388],155,124,GEOR13,zwykly,bitmap);
	SDL_itoa(c->hitPoints,pom,10);
	printToWR(pom,276,137,GEOR13,zwykly,bitmap);

	//remaining health
	if(State && State->currentHealth)
	{
		printAt(CGI->generaltexth->allTexts[200],155,143,GEOR13,zwykly,bitmap);
		SDL_itoa(State->currentHealth,pom,10);
		printToWR(pom,276,156,GEOR13,zwykly,bitmap);
	}

	//speed
	printAt(CGI->generaltexth->zelp[441].first,155,162,GEOR13,zwykly,bitmap);
	SDL_itoa(c->speed,pom,10);
	if(State && State->speedBonus)
	{
		int hlp;
		if(c->speed > 0)
			hlp = log10f(c->speed)+2;
		else
			hlp = 2;
		pom[hlp-1] = ' '; pom[hlp] = '(';
		SDL_itoa(c->speed + State->speedBonus, pom+hlp+1, 10);
		hlp += 2+(int)log10f(c->speed + State->speedBonus);
		pom[hlp] = ')'; pom[hlp+1] = '\0';
	}
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
				CFunctionList<void()> cfl;
				cfl = boost::bind(&CPlayerInterface::showYesNoDialog,LOCPLINT,CGI->generaltexth->allTexts[207],boost::ref(upgResCost),fs[0],fs[1],false);
				upgrade = new AdventureMapButton("",CGI->generaltexth->zelp[446].second,cfl,pos.x+76,pos.y+237,"IVIEWCR.DEF",SDLK_u);
			}
			else
			{
				upgrade = new AdventureMapButton("",CGI->generaltexth->zelp[446].second,boost::function<void()>(),pos.x+76,pos.y+237,"IVIEWCR.DEF");
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
			CFunctionList<void()> cfl;
			cfl = boost::bind(&CPlayerInterface::showYesNoDialog,LOCPLINT,CGI->generaltexth->allTexts[12],std::vector<SComponent*>(),fs[0],fs[1],false);
			dismiss = new AdventureMapButton("",CGI->generaltexth->zelp[445].second,cfl,pos.x+21,pos.y+237,"IVIEWCR2.DEF",SDLK_d);
		}
		ok = new AdventureMapButton("",CGI->generaltexth->zelp[445].second,boost::bind(&CCreInfoWindow::close,this),pos.x+216,pos.y+237,"IOKAY.DEF",SDLK_RETURN);
	}
	else
	{
		printAtWB(c->abilityText,17,231,GEOR13,35,zwykly,bitmap);
	}

	//spell effects
	if(State)
	{
		int printed=0; //how many effect pics have been printed
		for(std::set<int>::const_iterator it = State->effects.begin(); it!=State->effects.end(); ++it)
		{
			blitAt(graphics->spellEffectsPics->ourImages[*it + 1].bitmap, 127 + 52 * printed, 186, bitmap); 
			++printed;
			if(printed >= 3)
			{
				break;
			}
		}
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
	//if(active) return;
	//active = true;
	if(!type)
		ClickableR::activate();
	if(ok)
		ok->activate();
	if(dismiss)
		dismiss->activate();
	if(upgrade)
		upgrade->activate();
}

void CCreInfoWindow::close()
{
	LOCPLINT->popIntTotally(this);
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
	//if(!active) return;
	//active = false;
	if(!type)
		ClickableR::deactivate();
	if(ok)
		ok->deactivate();
	if(dismiss)
		dismiss->deactivate();
	if(upgrade)
		upgrade->deactivate();
}

void CLevelWindow::close()
{
	for(int i=0;i<comps.size();i++)
	{
		if(comps[i]->selected)
		{
			cb(i);
			break;
		}
	}
	LOCPLINT->popIntTotally(this);
	LOCPLINT->showingDialog->setn(false);
}

CLevelWindow::CLevelWindow(const CGHeroInstance *hero, int pskill, std::vector<ui16> &skills, boost::function<void(ui32)> &callback)
{
	LOCPLINT->showingDialog->setn(true);
	heroType = hero->subID;
	cb = callback;
	for(int i=0;i<skills.size();i++)
	{
		comps.push_back(new CSelectableComponent(SComponent::secskill44,skills[i],hero->getSecSkillLevel(skills[i])+1,boost::bind(&CLevelWindow::selectionChanged,this,i)));
		comps.back()->assignedKeys.insert(SDLK_1 + i);
	}
	bitmap = BitmapHandler::loadBitmap("LVLUPBKG.bmp");
	graphics->blueToPlayersAdv(bitmap,hero->tempOwner);
	SDL_SetColorKey(bitmap,SDL_SRCCOLORKEY,SDL_MapRGB(bitmap->format,0,255,255));
	pos.x = screen->w/2 - bitmap->w/2;
	pos.y = screen->h/2 - bitmap->h/2;
	pos.w = bitmap->w;
	pos.h = bitmap->h;
	ok = new AdventureMapButton("","",boost::bind(&CLevelWindow::close,this),pos.x+297,pos.y+413,"IOKAY.DEF",SDLK_RETURN);
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

	if(comps.size() > 1)
	{
		ok->block(true);
	}
	else if(comps.size() == 1)
	{
		comps[0]->select(true);
	}

	SDL_FreeSurface(ort);

}
void CLevelWindow::selectionChanged(unsigned to)
{
	if(ok->blocked)
		ok->block(false);
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
	ok->activate();
	for(int i=0;i<comps.size();i++)
		comps[i]->activate();
}
void CLevelWindow::deactivate()
{
	ok->deactivate();
	for(int i=0;i<comps.size();i++)
		comps[i]->deactivate();
}
void CLevelWindow::show(SDL_Surface * to)
{
	blitAt(bitmap,pos.x,pos.y,to);
	blitAt(graphics->portraitLarge[heroType],170+pos.x,66+pos.y,to);
	ok->show(to);
	for(int i=0;i<comps.size();i++)
		comps[i]->show(to);
}

void CMinorResDataBar::show(SDL_Surface * to)
{
	blitAt(bg,pos.x,pos.y,to);
	char buf[30];
	for (int i=0;i<7;i++)
	{
		SDL_itoa(LOCPLINT->cb->getResourceAmount(i),buf,10);
		CSDL_Ext::printAtMiddle(buf,pos.x + 50 + 76*i,pos.y+pos.h/2,GEOR13,zwykly,to);
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
		,pos.x+545+(pos.w-545)/2,pos.y+pos.h/2,GEOR13,zwykly,to);
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

void CMarketplaceWindow::CTradeableItem::show(SDL_Surface * to)
{
	SDL_Surface *hlp = getSurface();
	blitAt(hlp,pos.x+19,pos.y+9,to);
}

void CMarketplaceWindow::CTradeableItem::clickLeft( boost::logic::tribool down )
{
	CMarketplaceWindow *mw = dynamic_cast<CMarketplaceWindow *>(LOCPLINT->topInt());
	assert(mw);
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
	ok = new AdventureMapButton("","",boost::bind(&CPlayerInterface::popIntTotally,LOCPLINT,this),pos.x+516,pos.y+520,"IOK6432.DEF",SDLK_RETURN);
	deal = new AdventureMapButton("","",boost::bind(&CMarketplaceWindow::makeDeal,this),pos.x+307,pos.y+520,"TPMRKB.DEF");
	max = new AdventureMapButton("","",boost::bind(&CMarketplaceWindow::setMax,this),pos.x+229,pos.y+520,"IRCBTNS.DEF");

	max->block(true);
	deal->block(true);
}

CMarketplaceWindow::~CMarketplaceWindow()
{
	clear();
	delete slider;
}

void CMarketplaceWindow::show(SDL_Surface * to)
{
	blitAt(bg,pos,to);
	if(hRight)
		CSDL_Ext::drawBorder(to,hRight->pos.x-1,hRight->pos.y-1,hRight->pos.w+2,hRight->pos.h+2,int3(255,231,148));
	if(hLeft)
		CSDL_Ext::drawBorder(to,hLeft->pos.x-1,hLeft->pos.y-1,hLeft->pos.w+2,hLeft->pos.h+2,int3(255,231,148));
	ok->show(to);
	deal->show(to);
	max->show(to);
	slider->show(to);
	for(int i=0;i<left.size();i++)
		left[i]->show(to);
	for(int i=0;i<right.size();i++)
		right[i]->show(to);
	if(mode==0)
	{
		char buf[15];
		for(int i=0;i<left.size();i++)
		{
			SDL_itoa(LOCPLINT->cb->getResourceAmount(i),buf,10);
			printAtMiddle(buf,left[i]->pos.x+35,left[i]->pos.y+56,GEOR13,zwykly,to);
		}
		if(hLeft) //print prices
		{
			for(int i=0; i<right.size();i++)
			{
				if(right[i]->id != hLeft->id)
					printAtMiddle(rSubs[i],right[i]->pos.x+35,right[i]->pos.y+56,GEOR13,zwykly,to);
				else
					printAtMiddle(CGI->generaltexth->allTexts[164],right[i]->pos.x+35,right[i]->pos.y+56,GEOR13,zwykly,to);
			}
		}
		if(hLeft && hRight && hLeft->id!= hRight->id)
		{
			blitAt(hLeft->getSurface(),pos.x+142,pos.y+457,to);
			blitAt(hRight->getSurface(),pos.x+430,pos.y+457,to);
			SDL_itoa(slider->value * r1,buf,10);
			printAtMiddle(buf,pos.x+158,pos.y+504,GEOR13,zwykly,to);
			SDL_itoa(slider->value * r2,buf,10);
			printAtMiddle(buf,pos.x+446,pos.y+504,GEOR13,zwykly,to);
		}
	}
}

void CMarketplaceWindow::activate()
{
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
	save = new AdventureMapButton (CGI->generaltexth->zelp[321].first, CGI->generaltexth->zelp[321].second, boost::bind(&CSystemOptionsWindow::bsavef, this), 516, 354, "SOSAVE.DEF", SDLK_s);
	std::swap(save->imgs[0][0], save->imgs[0][1]);
	quitGame = new AdventureMapButton (CGI->generaltexth->zelp[324].first, CGI->generaltexth->zelp[324].second, boost::bind(&CSystemOptionsWindow::bquitf, this), 405, 471, "soquit.def", SDLK_q);
	std::swap(quitGame->imgs[0][0], quitGame->imgs[0][1]);
	backToMap = new AdventureMapButton (CGI->generaltexth->zelp[325].first, CGI->generaltexth->zelp[325].second, boost::bind(&CSystemOptionsWindow::breturnf, this), 516, 471, "soretrn.def", SDLK_RETURN);
	backToMap->assignedKeys.insert(SDLK_ESCAPE);
	std::swap(backToMap->imgs[0][0], backToMap->imgs[0][1]);

	heroMoveSpeed = new CHighlightableButtonsGroup(0);
	heroMoveSpeed->addButton(boost::assign::map_list_of(0,CGI->generaltexth->zelp[349].first),CGI->generaltexth->zelp[349].second, "sysopb1.def", 187, 134, 1);
	heroMoveSpeed->addButton(boost::assign::map_list_of(0,CGI->generaltexth->zelp[350].first),CGI->generaltexth->zelp[350].second, "sysopb2.def", 235, 134, 2);
	heroMoveSpeed->addButton(boost::assign::map_list_of(0,CGI->generaltexth->zelp[351].first),CGI->generaltexth->zelp[351].second, "sysopb3.def", 283, 134, 4);
	heroMoveSpeed->addButton(boost::assign::map_list_of(0,CGI->generaltexth->zelp[352].first),CGI->generaltexth->zelp[352].second, "sysopb4.def", 331, 134, 8);
	heroMoveSpeed->select(owner->heroMoveSpeed, 1);
	heroMoveSpeed->onChange = boost::bind(&CPlayerInterface::setHeroMoveSpeed, owner, _1);

	mapScrollSpeed = new CHighlightableButtonsGroup(0);
	mapScrollSpeed->addButton(boost::assign::map_list_of(0,CGI->generaltexth->zelp[357].first),CGI->generaltexth->zelp[357].second, "sysopb9.def", 187, 267, 1);
	mapScrollSpeed->addButton(boost::assign::map_list_of(0,CGI->generaltexth->zelp[358].first),CGI->generaltexth->zelp[358].second, "sysob10.def", 251, 267, 2);
	mapScrollSpeed->addButton(boost::assign::map_list_of(0,CGI->generaltexth->zelp[359].first),CGI->generaltexth->zelp[359].second, "sysob11.def", 315, 267, 4);
	mapScrollSpeed->select(owner->mapScrollingSpeed, 1);
	mapScrollSpeed->onChange = boost::bind(&CPlayerInterface::setMapScrollingSpeed, owner, _1);

	musicVolume = new CHighlightableButtonsGroup(0, true);
	for(int i=0; i<10; ++i)
	{
		musicVolume->addButton(boost::assign::map_list_of(0,CGI->generaltexth->zelp[326+i].first),CGI->generaltexth->zelp[326+i].second, "syslb.def", 188 + 19*i, 416, i*11);
	}
	musicVolume->select(CGI->audioh->getMusicVolume(), 1);
	musicVolume->onChange = boost::bind(&CAudioHandler::setMusicVolume, CGI->audioh, _1);

	effectsVolume = new CHighlightableButtonsGroup(0, true);
	for(int i=0; i<10; ++i)
	{
		effectsVolume->addButton(boost::assign::map_list_of(0,CGI->generaltexth->zelp[336+i].first),CGI->generaltexth->zelp[336+i].second, "syslb.def", 188 + 19*i, 482, i*11);
	}
	effectsVolume->select(CGI->audioh->getSoundVolume(), 1);
	effectsVolume->onChange = boost::bind(&CAudioHandler::setSoundVolume, CGI->audioh, _1);
}

CSystemOptionsWindow::~CSystemOptionsWindow()
{
	SDL_FreeSurface(background);

	delete save;
	delete quitGame;
	delete backToMap;
	delete heroMoveSpeed;
	delete mapScrollSpeed;
	delete musicVolume;
	delete effectsVolume;
}

void CSystemOptionsWindow::bquitf()
{
	LOCPLINT->showYesNoDialog(CGI->generaltexth->allTexts[578], std::vector<SComponent*>(), boost::bind(exit, 0), boost::bind(&CSystemOptionsWindow::activate, this), false);
}

void CSystemOptionsWindow::breturnf()
{
	LOCPLINT->popIntTotally(this);
}


void CSystemOptionsWindow::bsavef()
{
	using namespace boost::posix_time;
	std::ostringstream fnameStream;
	fnameStream << second_clock::local_time();
	std::string fname = fnameStream.str();
	boost::algorithm::replace_all(fname,":","");
	boost::algorithm::replace_all(fname," ","-");
	LOCPLINT->showYesNoDialog("Do you want to save current game as " + fname, std::vector<SComponent*>(), boost::bind(&CCallback::save, LOCPLINT->cb, fname), boost::bind(&CSystemOptionsWindow::activate, this), false);
}

void CSystemOptionsWindow::activate()
{
	save->activate();
	quitGame->activate();
	backToMap->activate();
	heroMoveSpeed->activate();
	mapScrollSpeed->activate();
	musicVolume->activate();
	effectsVolume->activate();
}

void CSystemOptionsWindow::deactivate()
{
	save->deactivate();
	quitGame->deactivate();
	backToMap->deactivate();
	heroMoveSpeed->deactivate();
	mapScrollSpeed->deactivate();
	musicVolume->deactivate();
	effectsVolume->deactivate();
}

void CSystemOptionsWindow::show(SDL_Surface *to)
{
	SDL_BlitSurface(background, NULL, to, &pos);

	save->show(to);
	quitGame->show(to);
	backToMap->show(to);
	heroMoveSpeed->show(to);
	mapScrollSpeed->show(to);
	musicVolume->show(to);
	effectsVolume->show(to);
}

CTavernWindow::CTavernWindow(const CGHeroInstance *H1, const CGHeroInstance *H2, const std::string &gossip)
:h1(selected,0,72,299,H1),h2(selected,1,162,299,H2)
{
	if(H1)
		selected = 0;
	else
		selected = -1;
	bg = BitmapHandler::loadBitmap("TPTAVERN.bmp");
	SDL_SetColorKey(bg,SDL_SRCCOLORKEY,SDL_MapRGB(bg->format,0,255,255));
	graphics->blueToPlayersAdv(bg,LOCPLINT->playerID);
	printAtMiddle(CGI->generaltexth->jktexts[37],200,35,GEOR16,tytulowy,bg);
	printAtMiddle("2500",320,328,GEOR13,zwykly,bg);
	printAtMiddle(CGI->generaltexth->jktexts[38],146,283,GEOR16,tytulowy,bg);
	printAtMiddleWB(gossip,200,220,GEOR13,50,zwykly,bg);
	pos.w = bg->w;
	pos.h = bg->h;
	pos.x = (screen->w-bg->w)/2;
	pos.y = (screen->h-bg->h)/2;
	bar = new CStatusBar(pos.x+8, pos.y+478, "APHLFTRT.bmp", 380);
	h1.pos.x += pos.x;
	h2.pos.x += pos.x;
	h1.pos.y += pos.y;
	h2.pos.y += pos.y;

	cancel = new AdventureMapButton(CGI->generaltexth->tavernInfo[7],"",boost::bind(&CTavernWindow::close,this),pos.x+310,pos.y+428,"ICANCEL.DEF",SDLK_ESCAPE);
	recruit = new AdventureMapButton("","",boost::bind(&CTavernWindow::recruitb,this),pos.x+272,pos.y+355,"TPTAV01.DEF",SDLK_RETURN);
	thiefGuild = new AdventureMapButton(CGI->generaltexth->tavernInfo[5],"",0,pos.x+22,pos.y+428,"TPTAV02.DEF",SDLK_t);

	if(LOCPLINT->cb->getResourceAmount(6) < 2500) //not enough gold
	{
		recruit->hoverTexts[0] = CGI->generaltexth->tavernInfo[0]; //Cannot afford a Hero
		recruit->block(2);
	}
	else if(LOCPLINT->cb->howManyHeroes(false) >= 8)
	{
		recruit->hoverTexts[0] = CGI->generaltexth->tavernInfo[1]; //Cannot recruit. You already have %d Heroes.
		boost::algorithm::replace_first(recruit->hoverTexts[0],"%d",boost::lexical_cast<std::string>(LOCPLINT->cb->howManyHeroes()));
		recruit->block(2);
	}
	else if(LOCPLINT->castleInt && LOCPLINT->castleInt->town->visitingHero)
	{
		recruit->hoverTexts[0] = CGI->generaltexth->tavernInfo[2]; //Cannot recruit. You already have a Hero in this town.
		recruit->block(2);
	}
	else
	{
		if(H1)
		{
			recruit->hoverTexts[0] = CGI->generaltexth->tavernInfo[3]; //Recruit %s the %s
			boost::algorithm::replace_first(recruit->hoverTexts[0],"%s",H1->name);
			boost::algorithm::replace_first(recruit->hoverTexts[0],"%s",H1->type->heroClass->name);
		}
		else
			recruit->block(1);
	}
}

void CTavernWindow::recruitb()
{
	const CGHeroInstance *toBuy = (selected ? h2 : h1).h;
	close();
	LOCPLINT->cb->recruitHero(LOCPLINT->castleInt->town,toBuy);
}

CTavernWindow::~CTavernWindow()
{
	SDL_FreeSurface(bg);
	delete cancel;
	delete thiefGuild;
	delete recruit;
	delete bar;
}

void CTavernWindow::activate()
{
	thiefGuild->activate();
	cancel->activate();
	if(h1.h)
		h1.activate();
	if(h2.h)
		h2.activate();
	recruit->activate();
	LOCPLINT->statusbar = bar;
}

void CTavernWindow::deactivate()
{
	thiefGuild->deactivate();
	cancel->deactivate();
	if(h1.h)
		h1.deactivate();
	if(h2.h)
		h2.deactivate();
	recruit->deactivate();
}

void CTavernWindow::close()
{
	LOCPLINT->popIntTotally(this);
}

void CTavernWindow::show(SDL_Surface * to)
{
	blitAt(bg,pos.x,pos.y,to);
	if(h1.h)
		h1.show(to);
	if(h2.h)
		h2.show(to);
	thiefGuild->show(to);
	cancel->show(to);
	recruit->show(to);
	bar->show(to);

	if(selected >= 0)
	{
		HeroPortrait *sel = selected ? &h2 : &h1;
		char descr[300];
		int artifs = sel->h->artifWorn.size()+sel->h->artifacts.size();
		for(int i=13; i<=17; i++) //war machines and spellbook doesn't count
			if(vstd::contains(sel->h->artifWorn,i)) 
				artifs--;
		sprintf_s(descr,300,CGI->generaltexth->allTexts[215].c_str(),
			sel->h->name.c_str(),sel->h->level,sel->h->type->heroClass->name.c_str(),artifs);
		printAtMiddleWB(descr,pos.x+146,pos.y+389,GEOR13,40,zwykly,to);
		CSDL_Ext::drawBorder(to,sel->pos.x-2,sel->pos.y-2,sel->pos.w+4,sel->pos.h+4,int3(247,223,123));
	}
}

void CTavernWindow::HeroPortrait::clickLeft(boost::logic::tribool down)
{
	if(pressedL && !down)
		as();
	ClickableL::clickLeft(down);
}
void CTavernWindow::HeroPortrait::activate()
{
	ClickableL::activate();
	ClickableR::activate();
	Hoverable::activate();
}
void CTavernWindow::HeroPortrait::deactivate()
{
	ClickableL::deactivate();
	ClickableR::deactivate();
	Hoverable::deactivate();
}
void CTavernWindow::HeroPortrait::clickRight(boost::logic::tribool down)
{
	if(down)
	{
		LOCPLINT->adventureInt->heroWindow->setHero(h);
		LOCPLINT->pushInt(new CRClickPopupInt(LOCPLINT->adventureInt->heroWindow,false));
	}
}
CTavernWindow::HeroPortrait::HeroPortrait(int &sel, int id, int x, int y, const CGHeroInstance *H)
:as(sel,id)
{
	h = H;
	pos.x = x;
	pos.y = y;
	pos.w = 58;
	pos.h = 64;
	if(H)
	{
		hoverName = CGI->generaltexth->tavernInfo[4];
		boost::algorithm::replace_first(hoverName,"%s",H->name);
	}
}
	
void CTavernWindow::HeroPortrait::show(SDL_Surface * to)
{
	blitAt(graphics->portraitLarge[h->subID],pos,to);
}

void CTavernWindow::HeroPortrait::hover( bool on )
{
	Hoverable::hover(on);
	if(on)
		LOCPLINT->statusbar->print(hoverName);
	else
		LOCPLINT->statusbar->clear();
}

void CInGameConsole::activate()
{
	KeyInterested::activate();
}

void CInGameConsole::deactivate()
{
	KeyInterested::deactivate();
}

void CInGameConsole::show(SDL_Surface * to)
{
	int number = 0;

	std::vector<std::list< std::pair< std::string, int > >::iterator> toDel;

	for(std::list< std::pair< std::string, int > >::iterator it = texts.begin(); it != texts.end(); ++it, ++number)
	{
		SDL_Color green = {0,0xff,0,0};
		Point leftBottomCorner(0, screen->h);
		if(LOCPLINT->battleInt)
		{
			leftBottomCorner = LOCPLINT->battleInt->pos.bottomLeft();
		}
		CSDL_Ext::printAt(it->first, leftBottomCorner.x + 50, leftBottomCorner.y - texts.size() * 20 - 80 + number*20, TNRB16, green);
		if(SDL_GetTicks() - it->second > defaultTimeout)
		{
			toDel.push_back(it);
		}
	}

	for(int it=0; it<toDel.size(); ++it)
	{
		texts.erase(toDel[it]);
	}
}


void CInGameConsole::print(const std::string &txt)
{
	texts.push_back(std::make_pair(txt, SDL_GetTicks()));
	if(texts.size() > maxDisplayedTexts)
	{
		texts.pop_front();
	}
}

void CInGameConsole::keyPressed (const SDL_KeyboardEvent & key)
{
	if(key.type != SDL_KEYDOWN) return;

	if(!captureAllKeys && key.keysym.sym != SDLK_TAB) return; //because user is not entering any text

	switch(key.keysym.sym)
	{
	case SDLK_TAB:
		{
			if(captureAllKeys)
			{
				captureAllKeys = false;
				endEnteringText(false);
			}
			else
			{
				captureAllKeys = true;
				startEnteringText();
			}
			break;
		}
	case SDLK_RETURN: //enter key
		{
			if(enteredText.size() > 0  &&  captureAllKeys)
			{
				captureAllKeys = false;
				endEnteringText(true);
			}
			break;
		}
	case SDLK_BACKSPACE:
		{
			if(enteredText.size() > 1)
			{
				enteredText.resize(enteredText.size()-1);
				enteredText[enteredText.size()-1] = '_';
				refreshEnteredText();
			}
			break;
		}
	case SDLK_UP: //up arrow
		{
			if(prevEntDisp == -1)
			{
				prevEntDisp = previouslyEntered.size() - 1;
				enteredText = previouslyEntered[prevEntDisp] + "_";
				refreshEnteredText();
			}
			else if( prevEntDisp > 0)
			{
				--prevEntDisp;
				enteredText = previouslyEntered[prevEntDisp] + "_";
				refreshEnteredText();
			}
			break;
		}
	case SDLK_DOWN: //down arrow
		{
			if(prevEntDisp != -1 && prevEntDisp+1 < previouslyEntered.size())
			{
				++prevEntDisp;
				enteredText = previouslyEntered[prevEntDisp] + "_";
				refreshEnteredText();
			}
			else if(prevEntDisp+1 == previouslyEntered.size()) //useful feature
			{
				prevEntDisp = -1;
				enteredText = "_";
				refreshEnteredText();
			}
			break;
		}
	default:
		{
			if(enteredText.size() > 0)
			{
				if( key.keysym.unicode < 0x80 && key.keysym.unicode > 0 )
				{
					enteredText[enteredText.size()-1] = (char)key.keysym.unicode;
					enteredText += "_";
					refreshEnteredText();
				}
			}
			break;
		}
	}
}

void CInGameConsole::startEnteringText()
{
	enteredText = "_";
	if(LOCPLINT->topInt() == LOCPLINT->adventureInt)
	{
		LOCPLINT->statusbar->print(enteredText);
	}
	else if(LOCPLINT->battleInt)
	{
		LOCPLINT->battleInt->console->ingcAlter = enteredText;
	}
}

void CInGameConsole::endEnteringText(bool printEnteredText)
{
	prevEntDisp = -1;
	if(printEnteredText)
	{
		std::string txt = enteredText.substr(0, enteredText.size()-1);
		LOCPLINT->cb->sendMessage(txt);
		previouslyEntered.push_back(txt);
		print(txt);
	}
	enteredText = "";
	if(LOCPLINT->topInt() == LOCPLINT->adventureInt)
	{
		LOCPLINT->statusbar->clear();
	}
	else if(LOCPLINT->battleInt)
	{
		LOCPLINT->battleInt->console->ingcAlter = "";
	}

}

void CInGameConsole::refreshEnteredText()
{
	if(LOCPLINT->topInt() == LOCPLINT->adventureInt)
	{
		LOCPLINT->statusbar->print(enteredText);
	}
	else if(LOCPLINT->battleInt)
	{
		LOCPLINT->battleInt->console->ingcAlter = enteredText;
	}
}

CInGameConsole::CInGameConsole() : prevEntDisp(-1), defaultTimeout(10000), maxDisplayedTexts(10)
{
}


void CGarrisonWindow::close()
{
	LOCPLINT->popIntTotally(this);
}

void CGarrisonWindow::activate()
{
	split->activate();
	quit->activate();
	garr->activate();
}

void CGarrisonWindow::deactivate()
{
	split->deactivate();
	quit->deactivate();
	garr->deactivate();
}

void CGarrisonWindow::show(SDL_Surface * to)
{
	blitAt(bg,pos,to);
	split->show(to);
	quit->show(to);
	garr->show(to);

	blitAt(graphics->flags->ourImages[garr->odown->getOwner()].bitmap,pos.x+29,pos.y+125,to);
	blitAt(graphics->portraitLarge[static_cast<const CGHeroInstance*>(garr->odown)->portrait],pos.x+29,pos.y+222,to);
	printAtMiddle(CGI->generaltexth->allTexts[709],pos.x+275,pos.y+30,GEOR16,tytulowy,to);
}

CGarrisonWindow::CGarrisonWindow( const CArmedInstance *up, const CGHeroInstance *down )
{
	bg = BitmapHandler::loadBitmap("GARRISON.bmp");
	SDL_SetColorKey(bg,SDL_SRCCOLORKEY,SDL_MapRGB(bg->format,0,255,255));
	graphics->blueToPlayersAdv(bg,LOCPLINT->playerID);
	pos.x = screen->w/2 - bg->w/2;
	pos.y = screen->h/2 - bg->h/2;
	pos.w = screen->w;
	pos.h = screen->h;

	garr = new CGarrisonInt(pos.x+92, pos.y+129, 4, 30, bg, 92, 129, up, down);
	split = new AdventureMapButton(CGI->generaltexth->tcommands[3],"",boost::bind(&CGarrisonInt::splitClick,garr),pos.x+88,pos.y+314,"IDV6432.DEF");
	quit = new AdventureMapButton(CGI->generaltexth->tcommands[8],"",boost::bind(&CGarrisonWindow::close,this),pos.x+399,pos.y+314,"IOK6432.DEF",SDLK_RETURN);
}

CGarrisonWindow::~CGarrisonWindow()
{
	SDL_FreeSurface(bg);
	delete split;
	delete quit;
	delete garr;
}

IShowActivable::IShowActivable()
{
	type = 0;
}

CWindowWithGarrison::CWindowWithGarrison()
{
	type |= WITH_GARRISON;
}


void CRClickPopupInt::show(SDL_Surface * to)
{
	inner->show(to);
}

CRClickPopupInt::CRClickPopupInt( IShowActivable *our, bool deleteInt )
{
	inner = our;
	delInner = deleteInt;
}

CRClickPopupInt::~CRClickPopupInt()
{
	if(delInner)
		delete inner;
}
