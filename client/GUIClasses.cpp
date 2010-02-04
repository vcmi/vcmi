#include "GUIClasses.h"
#include "SDL_Extensions.h"

#include "../stdafx.h"
#include "CAdvmapInterface.h"
#include "CBattleInterface.h"
#include "../CCallback.h"
#include "CCastleInterface.h"
#include "CCursorHandler.h"
#include "CGameInfo.h"
#include "CHeroWindow.h"
#include "CMessage.h"
#include "CConfigHandler.h"
#include "SDL_framerate.h"
#include "CConfigHandler.h"
#include "CCreatureAnimation.h"
#include "Graphics.h"
#include "../hch/CArtHandler.h"
#include "../hch/CBuildingHandler.h"
#include "../hch/CGeneralTextHandler.h"
#include "../hch/CHeroHandler.h"
#include "../hch/CLodHandler.h"
#include "../hch/CObjectHandler.h"
#include "../hch/CSpellHandler.h"
#include "../hch/CTownHandler.h"
#include "../lib/CondSh.h"
#include "../lib/map.h"
#include "../mapHandler.h"
#include "../timeHandler.h"
#include <boost/lexical_cast.hpp>
#include <boost/format.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/assign/std/vector.hpp> 
#include <boost/assign/list_of.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
//#include <boost/thread.hpp>
#include <cmath>
#include <queue>
#include <sstream>
#include "../lib/NetPacks.h"
#include "CSpellWindow.h"
#include "CHeroWindow.h"
#include "../hch/CVideoHandler.h"
#include "../StartInfo.h"
#include "CPreGame.h"

/*
 * GUIClasses.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

using namespace boost::assign;
using namespace CSDL_Ext;

extern std::queue<SDL_Event*> events;
extern boost::mutex eventsM;

#undef min
#undef max

static StackState* getStackState(const CGObjectInstance *obj, int pos, bool town)
{
	const CGHeroInstance *h = dynamic_cast<const CGHeroInstance *>(obj);
	if(!h) return NULL;

	StackState *pom = new StackState();
	pom->shotsLeft = -1;
	pom->healthBonus = h->valOfBonuses(HeroBonus::STACK_HEALTH);
	pom->currentHealth = 0;
	pom->attackBonus = h->getPrimSkillLevel(0);
	pom->defenseBonus = h->getPrimSkillLevel(1);
	pom->luck = h->getCurrentLuck();
	pom->morale = h->getCurrentMorale(pos,town);
	pom->speedBonus = h->valOfBonuses(HeroBonus::STACKS_SPEED);
	pom->dmgMultiplier = 1;
	return pom;
}


void CGarrisonSlot::hover (bool on)
{
	////Hoverable::hover(on);
	if(on)
	{
		std::string temp;
		if(creature)
		{
			if(owner->highlighted)
			{
				if(owner->highlighted == this)
				{
					temp = CGI->generaltexth->tcommands[4]; //View %s
					boost::algorithm::replace_first(temp,"%s",creature->nameSing);
				}
				else if (owner->highlighted->creature == creature)
				{
					temp = CGI->generaltexth->tcommands[2]; //Combine %s armies
					boost::algorithm::replace_first(temp,"%s",creature->nameSing);
				}
				else if (owner->highlighted->creature)
				{
					temp = CGI->generaltexth->tcommands[7]; //Exchange %s with %s
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
					temp = CGI->generaltexth->tcommands[32]; //Select %s (visiting)
				}
				else if(owner->oup && owner->oup->ID == TOWNI_TYPE)
				{
					temp = CGI->generaltexth->tcommands[12]; //Select %s (in garrison)
				}
				else
				{
					temp = CGI->generaltexth->allTexts[481]; //Select %s
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
					temp = CGI->generaltexth->tcommands[5]; //Cannot move last army to garrison
				}
				else
				{
					temp = CGI->generaltexth->tcommands[6]; //Move %s
					boost::algorithm::replace_first(temp,"%s",owner->highlighted->creature->nameSing);
				}
			}
			else
			{
				temp = CGI->generaltexth->tcommands[11]; //Empty
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

void CGarrisonSlot::clickRight(tribool down, bool previousState)
{
	StackState *pom = getStackState(getObj(),ID, GH.topInt() == LOCPLINT->castleInt);
	if(down && creature)
	{
		GH.pushInt(new CCreInfoWindow(creature->idNumber, 0, count, pom, 0, 0, NULL));
	}
	delete pom;
}
void CGarrisonSlot::clickLeft(tribool down, bool previousState)
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
				StackState *pom2 = getStackState(getObj(), ID, GH.topInt() == LOCPLINT->castleInt);
				UpgradeInfo pom = LOCPLINT->cb->getUpgradeInfo(getObj(), ID);

				CCreInfoWindow *creWindow = NULL;
				if(pom.oldID>=0) //upgrade is possible
				{

					creWindow = new CCreInfoWindow(
						creature->idNumber, 1, count, pom2,
						boost::bind(&CCallback::upgradeCreature, LOCPLINT->cb, getObj(), ID, pom.newID[0]), //bind upgrade function
						boost::bind(&CCallback::dismissCreature, LOCPLINT->cb, getObj(), ID), &pom);
				}
				else
				{
					creWindow = new CCreInfoWindow(
						creature->idNumber, 1, count, pom2, 0, 
						boost::bind(&CCallback::dismissCreature, LOCPLINT->cb, getObj(), ID), NULL);
				}

				GH.pushInt(creWindow);

				owner->highlighted = NULL;
				owner->splitting = false;

				for(size_t i = 0; i<owner->splitButtons.size(); i++)
					owner->splitButtons[i]->block(true);

				show(screen2);
				refr = true;
				delete pom2;
			}
			else 
			{
				// Only allow certain moves if troops aren't removable.
				if (owner->removableUnits
					|| (upg == 0 && (owner->highlighted->upg == 1 && !creature))
					|| (upg == 1 && owner->highlighted->upg == 1))
				{
					//we want to split
					if((owner->splitting || LOCPLINT->shiftPressed())
						&& (!creature
							|| (creature == owner->highlighted->creature)))
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
							if(owner->highlighted->getObj()->army.slots.size() == 1 //we're splitting away the last stack
								&& owner->highlighted->getObj()->needsLastStack() )
							{
								last = 0;
							}
							if(getObj()->army.slots.size() == 1 //destination army can't be emptied, unless we're rebalancing two stacks of same creature
								&& owner->highlighted->creature == creature
								&& getObj()->needsLastStack() )
							{
								last += 2;
							}
						}


						CSplitWindow * spw = new CSplitWindow(owner->highlighted->creature->idNumber, totalAmount, owner, last, count);
						GH.pushInt(spw);
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
				else // Highlight
				{ 
					if(creature)
						owner->highlighted = this;
					show(screen2);
					refr = true;
				}
			}
		}
		else //highlight
		{
			if(creature)
			{
				owner->highlighted = this;

				for(size_t i = 0; i<owner->splitButtons.size(); i++)
					owner->splitButtons[i]->block(false);
			}
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
	activateLClick();
	activateRClick();
	activateHover();
}
void CGarrisonSlot::deactivate()
{
	if(active) active=false;
	else return;
	deactivateLClick();
	deactivateRClick();
	deactivateHover();
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
	if(Owner->smallIcons)
	{
		pos.w = 32;
		pos.h = 32;
	}
	else
	{
		pos.w = 58;
		pos.h = 64;
	}
	owner = Owner;
}
CGarrisonSlot::~CGarrisonSlot()
{
	if(active)
		deactivate();
}
void CGarrisonSlot::show(SDL_Surface * to)
{
	std::map<int,SDL_Surface*> &imgs = (owner->smallIcons ? graphics->smallImgs : graphics->bigImgs);
	if(creature)
	{
		char buf[15];
		SDL_itoa(count,buf,10);
		blitAt(imgs[creature->idNumber],pos,to);
		printTo(buf, pos.x+pos.w, pos.y+pos.h+1, owner->smallIcons ? FONT_TINY : FONT_VERD, zwykly, to);

		if((owner->highlighted==this)
			|| (owner->splitting && owner->highlighted->creature == creature))
		{
			blitAt(imgs[-1],pos,to);
		}
	}
	else//empty slot
	{
		Rect pos1 = pos, pos2 = pos; //positions on the garr bg sur and scren
		pos1.x = owner->surOffset.x+ pos.x-owner->pos.x;
		pos1.y = owner->surOffset.y+ pos.y-owner->pos.y;

		SDL_BlitSurface(owner->sur,&pos1,to,&pos2);
		if(owner->splitting)
			blitAt(imgs[-1],pos,to);
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

	for(size_t i = 0; i<splitButtons.size(); i++)
		delete splitButtons[i];
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

	for(size_t i = 0; i<splitButtons.size(); i++)
		splitButtons[i]->show(to);
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
	int h, w; //height and width of slot
	if(smallIcons)
	{
		h = w = 32;
	}
	else
	{
		h = 64;
		w = 58;
	}

	if(set1)
	{
		sup = new std::vector<CGarrisonSlot*>(7,(CGarrisonSlot *)(NULL));
		for
			(std::map<si32,std::pair<ui32,si32> >::const_iterator i=set1->slots.begin();
			i!=set1->slots.end(); i++)
		{
			(*sup)[i->first] =
				new CGarrisonSlot(this, pos.x + (i->first*(w+interx)), pos.y,i->first, 0, 
									&CGI->creh->creatures[i->second.first],i->second.second);
		}
		for(int i=0; i<sup->size(); i++)
			if((*sup)[i] == NULL)
				(*sup)[i] = new CGarrisonSlot(this, pos.x + (i*(w+interx)), pos.y,i,0,NULL, 0);
		if (shiftPos)
			for (int i=shiftPos; i<sup->size(); i++)
			{
				(*sup)[i]->pos.x += shiftPoint.x;
				(*sup)[i]->pos.y += shiftPoint.y;
			};
	}
	if(set2)
	{
		sdown = new std::vector<CGarrisonSlot*>(7,(CGarrisonSlot *)(NULL));
		for
			(std::map<si32,std::pair<ui32,si32> >::const_iterator i=set2->slots.begin();
			i!=set2->slots.end(); i++)
		{
			(*sdown)[i->first] =
				new CGarrisonSlot(this, pos.x + (i->first*(w+interx)) + garOffset.x, pos.y + garOffset.y,i->first,1, 
									&CGI->creh->creatures[i->second.first],i->second.second);
		}
		for(int i=0; i<sdown->size(); i++)
			if((*sdown)[i] == NULL)
				(*sdown)[i] = new CGarrisonSlot(this, pos.x + (i*(w+interx)) + garOffset.x,	pos.y + garOffset.y,i,1, NULL, 0);
		if (shiftPos)
			for (int i=shiftPos; i<sup->size(); i++)
			{
				(*sdown)[i]->pos.x += shiftPoint.x;
				(*sdown)[i]->pos.y += shiftPoint.y;
			};
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

	for(size_t i = 0; i<splitButtons.size(); i++)
		splitButtons[i]->block(true);

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
CGarrisonInt::CGarrisonInt(int x, int y, int inx, const Point &garsOffset, SDL_Surface *&pomsur, const Point& SurOffset, 
						   const CArmedInstance *s1, const CArmedInstance *s2, bool _removableUnits, bool smallImgs, int _shiftPos, const Point &_shiftPoint)
	 :interx(inx),garOffset(garsOffset),highlighted(NULL),sur(pomsur),surOffset(SurOffset),sup(NULL),
	 sdown(NULL),oup(s1),odown(s2), removableUnits(_removableUnits), smallIcons(smallImgs), shiftPos(_shiftPos), shiftPoint(_shiftPoint)
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
	for(size_t i = 0; i<splitButtons.size(); i++)
		if(splitButtons[i]->blocked != !highlighted)
			splitButtons[i]->block(!highlighted);

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

	for(size_t i = 0; i<splitButtons.size(); i++)
		splitButtons[i]->activate();
}
void CGarrisonInt::deactivate()
{
	active = false;
	deactiveteSlots();
	for(size_t i = 0; i<splitButtons.size(); i++)
		splitButtons[i]->deactivate();
}

CInfoWindow::CInfoWindow(std::string text, int player, int charperline, const std::vector<SComponent*> &comps, std::vector<std::pair<std::string,CFunctionList<void()> > > &Buttons, bool delComps)
{
	slider = NULL;
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
	GH.popIntTotally(this);
	if(LOCPLINT)
		LOCPLINT->showingDialog->setn(false);
}
void CInfoWindow::show(SDL_Surface * to)
{
	CSimpleWindow::show(to);
	for(int i=0;i<buttons.size();i++)
		buttons[i]->show(to);
	if (slider)
		slider->show(to);
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
	if (slider)
		delete slider;
}
void CInfoWindow::activate()
{
	for (int i=0;i<components.size();i++)
		components[i]->activate();
	for(int i=0;i<buttons.size();i++)
		buttons[i]->activate();
	if (slider)
		slider->activate();
}
void CInfoWindow::sliderMoved(int to)
{
	/*slider->moveTo(to);
		if(!slider) return; //ignore spurious call when slider is being created
	*/
	redraw();
}
void CInfoWindow::deactivate()
{
	for (int i=0;i<components.size();i++)
		components[i]->deactivate();
	for(int i=0;i<buttons.size();i++)
		buttons[i]->deactivate();
	if (slider)
		slider->deactivate();
}

void CInfoWindow::showAll( SDL_Surface * to )
{
	show(to);
}

void CInfoWindow::showYesNoDialog(const std::string & text, const std::vector<SComponent*> *components, const CFunctionList<void( ) > &onYes, const CFunctionList<void()> &onNo, bool DelComps, int player)
{
	assert(!LOCPLINT || LOCPLINT->showingDialog->get());
	std::vector<std::pair<std::string,CFunctionList<void()> > > pom;
	pom.push_back(std::pair<std::string,CFunctionList<void()> >("IOKAY.DEF",0));
	pom.push_back(std::pair<std::string,CFunctionList<void()> >("ICANCEL.DEF",0));
	CInfoWindow * temp = new CInfoWindow(text, player, 0, components ? *components : std::vector<SComponent*>(), pom, DelComps);
	temp->delComps = DelComps;
	for(int i=0;i<onYes.funcs.size();i++)
		temp->buttons[0]->callback += onYes.funcs[i];
	for(int i=0;i<onNo.funcs.size();i++)
		temp->buttons[1]->callback += onNo.funcs[i];

	GH.pushInt(temp);
}

void CRClickPopup::clickRight(tribool down, bool previousState)
{
	if(down)
		return;
	close();
}

void CRClickPopup::activate()
{
	activateRClick();
}

void CRClickPopup::deactivate()
{
	deactivateRClick();
}

void CRClickPopup::close()
{
	GH.popIntTotally(this);
}

CInfoPopup::CInfoPopup(SDL_Surface * Bitmap, int x, int y, bool Free)
 :free(Free),bitmap(Bitmap)
{
	CGI->curh->hide();

	pos.x = x;
	pos.y = y;
	pos.h = bitmap->h;
	pos.w = bitmap->w;

	// Put the window back on screen if necessary
	amax(pos.x, 0);
	amax(pos.y, 0);
	amin(pos.x, conf.cc.resx - bitmap->w);
	amin(pos.y, conf.cc.resy - bitmap->h);
}

CInfoPopup::CInfoPopup(SDL_Surface *Bitmap, bool Free)
{
	CGI->curh->hide();

	free=Free;
	bitmap=Bitmap;

	if(bitmap)
	{
		pos.x = screen->w/2 - bitmap->w/2;
		pos.y = screen->h/2 - bitmap->h/2;
		pos.h = bitmap->h;
		pos.w = bitmap->w;
	}
}

void CInfoPopup::close()
{
	if(free)
		SDL_FreeSurface(bitmap);
	GH.popIntTotally(this);
}
void CInfoPopup::show(SDL_Surface * to)
{
	blitAt(bitmap,pos.x,pos.y,to);
}
CInfoPopup::~CInfoPopup()
{
	CGI->curh->show();
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
		oss << std::showpos << Val << " ";
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
	case building:
		description = CGI->buildh->buildings[Subtype][Val]->Description();
		subtitle = CGI->buildh->buildings[Subtype][Val]->Name();
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
		subtitle = boost::lexical_cast<std::string>(Val) + " " + CGI->creh->creatures[Subtype].*(Val != 1 ? &CCreature::namePl : &CCreature::nameSing);
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
	case hero:
		subtitle = description = CGI->heroh->heroes[Subtype]->name;
		break;
	case flag:
		subtitle = CGI->generaltexth->capColors[Subtype];
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
		break;
	case hero:
		return graphics->portraitLarge[subtype];
	case flag:
		return graphics->flags->ourImages[subtype].bitmap;
	}
	return NULL;
}
void SComponent::clickRight(tribool down, bool previousState)
{
	if(description.size())
		LOCPLINT->adventureInt->handleRightClick(description,down,this);
}
void SComponent::activate()
{
	activateRClick();
}
void SComponent::deactivate()
{
	deactivateRClick();
}

void CSelectableComponent::clickLeft(tribool down, bool previousState)
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
	activateKeys();
	SComponent::activate();
	activateLClick();
}
void CSelectableComponent::deactivate()
{
	deactivateKeys();
	SComponent::deactivate();
	deactivateLClick();
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
	printAtMiddleWB(subtitle,pos.x+pos.w/2,pos.y+pos.h+25,FONT_SMALL,12,zwykly,to);
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
		show(GH.topInt()==LOCPLINT->adventureInt ? screen : screen2); //if there are now windows opened, update statusbar on screen, else to cache surface
	}
}

void CStatusBar::show(SDL_Surface * to)
{
	SDL_Rect pom = genRect(pos.h,pos.w,pos.x,pos.y);
	SDL_BlitSurface(bg,&genRect(pos.h,pos.w,0,0),to,&pom);
	printAtMiddle(current,middlex,middley,FONT_SMALL,zwykly,to);
}

std::string CStatusBar::getCurrent()
{
	return current;
}

void CList::activate()
{
	activateLClick();
	activateRClick();
	activateHover();
	activateKeys();
	activateMouseMove();
};

void CList::deactivate()
{
	deactivateLClick();
	deactivateRClick();
	deactivateHover();
	deactivateKeys();
	deactivateMouseMove();
};

void CList::clickLeft(tribool down, bool previousState)
{
};

CList::CList(int Size)
:SIZE(Size)
{
}

void CList::fixPos()
{
	int oldFrom = from;
	if(selected < 0) //no selection, do nothing
		return;
	if(selected < from) //scroll up
		from = selected;
	else if(from + SIZE <= selected)
		from = selected - SIZE + 1; 

	amin(from, size() - SIZE);
	amax(from, 0);
}

CHeroList::CHeroList(int Size)
:CList(Size), heroes(LOCPLINT->wanderingHeroes)
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
	//int howMany = LOCPLINT->cb->howManyHeroes();
	//for (int i=0;i<howMany;i++)
	//{
	//	const CGHeroInstance * h = LOCPLINT->cb->getHeroInfo(i,0);
	//	if(!h->inTownGarrison)
	//		items.push_back(std::pair<const CGHeroInstance *,CPath *>(h,NULL));
	//}
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
	if (which>=heroes.size())
		return;

	selected = which;
	LOCPLINT->adventureInt->select(heroes[which]);
}

void CHeroList::clickLeft(tribool down, bool previousState)
{
	if (down)
	{
		/***************************ARROWS*****************************************/
		if(isItIn(&arrupp,GH.current->motion.x,GH.current->motion.y))
		{
			if(from>0)
			{
				blitAt(arrup->ourImages[1].bitmap,arrupp.x,arrupp.y);
				pressed = true;
			}
			return;
		}
		else if(isItIn(&arrdop,GH.current->motion.x,GH.current->motion.y))
		{
			if(heroes.size()-from>SIZE)
			{
				blitAt(arrdo->ourImages[1].bitmap,arrdop.x,arrdop.y);
				pressed = false;
			}
			return;
		}
		/***************************HEROES*****************************************/
		int hx = GH.current->motion.x, hy = GH.current->motion.y;
		hx-=pos.x;
		hy-=pos.y; hy-=arrup->ourImages[0].bitmap->h;
		int ny = hy/32;
		if (ny>=SIZE || ny<0)
			return;
		if ( (ny+from)==selected && (LOCPLINT->adventureInt->selection->ID == HEROI_TYPE))
			LOCPLINT->openHeroWindow(heroes[selected]);//print hero screen
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
	if(isItIn(&arrupp,GH.current->motion.x,GH.current->motion.y))
	{
		if (from>0)
			LOCPLINT->adventureInt->statusbar.print(CGI->generaltexth->zelp[303].first);
		else
			LOCPLINT->adventureInt->statusbar.clear();
		return;
	}
	else if(isItIn(&arrdop,GH.current->motion.x,GH.current->motion.y))
	{
		if ((heroes.size()-from)  >  SIZE)
			LOCPLINT->adventureInt->statusbar.print(CGI->generaltexth->zelp[304].first);
		else
			LOCPLINT->adventureInt->statusbar.clear();
		return;
	}
	//if not buttons then heroes
	int hx = GH.current->motion.x, hy = GH.current->motion.y;
	hx-=pos.x;
	hy-=pos.y; hy-=arrup->ourImages[0].bitmap->h;
	int ny = hy/32;
	if ((ny>SIZE || ny<0) || (from+ny>=heroes.size()))
	{
		LOCPLINT->adventureInt->statusbar.clear();
		return;
	}
	std::vector<std::string> temp;
	temp.push_back(heroes[from+ny]->name);
	temp.push_back(heroes[from+ny]->type->heroClass->name);
	LOCPLINT->adventureInt->statusbar.print( processStr(CGI->generaltexth->allTexts[15],temp) );
	//select(ny+from);
}

void CHeroList::clickRight(tribool down, bool previousState)
{
	if (down)
	{
		/***************************ARROWS*****************************************/
		if(isItIn(&arrupp,GH.current->motion.x,GH.current->motion.y) && from>0)
		{
			LOCPLINT->adventureInt->handleRightClick(CGI->generaltexth->zelp[303].second,down,this);
		}
		else if(isItIn(&arrdop,GH.current->motion.x,GH.current->motion.y) && (heroes.size()-from>5))
		{
			LOCPLINT->adventureInt->handleRightClick(CGI->generaltexth->zelp[304].second,down,this);
		}
		else
		{
			//if not buttons then heroes
			int hx = GH.current->motion.x, hy = GH.current->motion.y;
			hx-=pos.x;
			hy-=pos.y; hy-=arrup->ourImages[0].bitmap->h;
			int ny = hy/32;
			if ((ny>SIZE || ny<0) || (from+ny>=heroes.size()))
			{
				return;
			}

			//show popup
			CInfoPopup * ip = new CInfoPopup(graphics->heroWins[heroes[from+ny]->subID],
				GH.current->motion.x-graphics->heroWins[heroes[from+ny]->subID]->w,
				GH.current->motion.y-graphics->heroWins[heroes[from+ny]->subID]->h,
				false);
			GH.pushInt(ip);
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
		heroes -= toRemove;
	else
		LOCPLINT->recreateWanderingHeroes();


	if(selected >= heroes.size())
		select(heroes.size()-1);

	if(toRemove)
	{
		if(heroes.size() == 0)
			LOCPLINT->adventureInt->townList.select(0);
		else
			select(selected);
	}
}

void CHeroList::updateMove(const CGHeroInstance* which) //draws move points bar
{
	int ser = -1;
	for(int i=0; i<heroes.size() && ser<0; i++)
		if(heroes[i]->subID == which->subID)
			ser = i;
	ser -= from;
	if(ser<0 || ser >= SIZE) return;
	int pom = std::min((which->movement)/100,(si32)mobile->ourImages.size()-1);
	blitAt(mobile->ourImages[pom].bitmap,posmobx,posmoby+ser*32); //move point
}

void CHeroList::draw(SDL_Surface * to)
{
	for (int iT=0+from;iT<SIZE+from;iT++)
	{
		int i = iT-from;
		if (iT>=heroes.size())
		{
			blitAt(mobile->ourImages[0].bitmap,posmobx,posmoby+i*32,to);
			blitAt(mana->ourImages[0].bitmap,posmanx,posmany+i*32,to);
			blitAt(empty,posporx,pospory+i*32,to);
			continue;
		}
		const CGHeroInstance *cur = heroes[iT];
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

	if (heroes.size()-from > SIZE)
		blitAt(arrdo->ourImages[0].bitmap,arrdop.x,arrdop.y,to);
	else
		blitAt(arrdo->ourImages[2].bitmap,arrdop.x,arrdop.y,to);
}

int CHeroList::getPosOfHero(const CGHeroInstance* h)
{
	return vstd::findPos(heroes, h, std::equal_to<const CGHeroInstance*>());
}

void CHeroList::show( SDL_Surface * to )
{

}

int CHeroList::size()
{
	return heroes.size();
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
	items.clear();
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
	if(isItIn(&arrupp,GH.current->motion.x,GH.current->motion.y))
	{
		if (from>0)
			LOCPLINT->statusbar->print(CGI->generaltexth->zelp[306].first);
		else
			LOCPLINT->statusbar->clear();
		return;
	}
	else if(isItIn(&arrdop,GH.current->motion.x,GH.current->motion.y))
	{
		if ((items.size()-from)  >  SIZE)
			LOCPLINT->statusbar->print(CGI->generaltexth->zelp[307].first);
		else
			LOCPLINT->statusbar->clear();
		return;
	}
	//if not buttons then towns
	int hx = GH.current->motion.x, hy = GH.current->motion.y;
	hx-=pos.x;
	hy-=pos.y; hy-=arrup->ourImages[0].bitmap->h;
	int ny = hy/32;
	if ((ny>SIZE || ny<0) || (from+ny>=items.size()))
	{
		LOCPLINT->statusbar->clear();
		return;
	};
	std::string temp = CGI->generaltexth->tcommands[4];
	boost::algorithm::replace_first(temp,"%s",items[from+ny]->name);
	temp += ", "+items[from+ny]->town->Name();
	LOCPLINT->statusbar->print(temp);
}

void CTownList::clickLeft(tribool down, bool previousState)
{
	if (down)
	{
		/***************************ARROWS*****************************************/
		if(isItIn(&arrupp,GH.current->motion.x,GH.current->motion.y))
		{
			if(from>0)
			{
				blitAt(arrup->ourImages[1].bitmap,arrupp.x,arrupp.y,screenBuf);
				pressed = true;
			}
			return;
		}
		else if(isItIn(&arrdop,GH.current->motion.x,GH.current->motion.y))
		{
			if(items.size()-from > SIZE)
			{
				blitAt(arrdo->ourImages[1].bitmap,arrdop.x,arrdop.y,screenBuf);
				pressed = false;
			}
			return;
		}
		/***************************TOWNS*****************************************/
		int hx = GH.current->motion.x, hy = GH.current->motion.y;
		hx-=pos.x;
		hy-=pos.y; hy-=arrup->ourImages[0].bitmap->h;
		int ny = hy/32;
		if (ny>SIZE || ny<0)
			return;
		if(GH.topInt() == LOCPLINT->adventureInt
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
			blitAt(arrup->ourImages[0].bitmap,arrupp.x,arrupp.y,screenBuf);
			pressed = indeterminate;
			if (!down)
			{
				from--;
				if (from<0)
					from=0;

				draw(screenBuf);
			}
		}
		else if (!pressed) //down
		{
			blitAt(arrdo->ourImages[0].bitmap,arrdop.x,arrdop.y,screenBuf);
			pressed = indeterminate;
			if (!down)
			{
				from++;
				//if (from<items.size()-5)
				//	from=items.size()-5;

				draw(screenBuf);
			}
		}
		else
			throw 0;

	}
}

void CTownList::clickRight(tribool down, bool previousState)
{
	if (down)
	{
		/***************************ARROWS*****************************************/
		if(isItIn(&arrupp,GH.current->motion.x,GH.current->motion.y) && from>0)
		{
			LOCPLINT->adventureInt->handleRightClick(CGI->generaltexth->zelp[306].second,down,this);
		}
		else if(isItIn(&arrdop,GH.current->motion.x,GH.current->motion.y) && (items.size()-from>5))
		{
			LOCPLINT->adventureInt->handleRightClick(CGI->generaltexth->zelp[307].second,down,this);
		}
		//if not buttons then towns
		int hx = GH.current->motion.x, hy = GH.current->motion.y;
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
			GH.current->motion.x-graphics->townWins[items[from+ny]->id]->w,
			GH.current->motion.y-graphics->townWins[items[from+ny]->id]->h,
			false);
		GH.pushInt(ip);
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

void CTownList::show( SDL_Surface * to )
{

}

int CTownList::size()
{
	return items.size();
}

CCreaturePic::CCreaturePic(const CCreature *cre, bool Big)
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
void CRecruitmentWindow::close()
{
	GH.popIntTotally(this);
}
void CRecruitmentWindow::Max()
{
	slider->moveTo(slider->amount);
}
void CRecruitmentWindow::Buy()
{
	int crid = creatures[which].ID,
		dstslot = dst->army.getSlotFor(crid);

	if(dstslot < 0) //no available slot
	{
		std::string txt;
		if(dst->ID == HEROI_TYPE)
		{
			txt = CGI->generaltexth->allTexts[425]; //The %s would join your hero, but there aren't enough provisions to support them.
			boost::algorithm::replace_first(txt, "%s", slider->value > 1 ? CGI->creh->creatures[crid].namePl : CGI->creh->creatures[crid].nameSing);
		}
		else
		{
			txt = CGI->generaltexth->allTexts[17]; //There is no room in the garrison for this army.
		}

		LOCPLINT->showInfoDialog(txt);
		return;
	}

	recruit(crid, slider->value);
	if(level >= 0)
		close();
	else
		slider->moveTo(0);

}
void CRecruitmentWindow::Cancel()
{
	close();
}
void CRecruitmentWindow::sliderMoved(int to)
{
	buy->block(!to);
}
void CRecruitmentWindow::clickLeft(tribool down, bool previousState)
{
	int curx = 192 + 51 - (CREATURE_WIDTH*creatures.size()/2) - (SPACE_BETWEEN*(creatures.size()-1)/2);
	for(int i=0;i<creatures.size();i++)
	{
		const int sCREATURE_WIDTH = CREATURE_WIDTH; // gcc -O0 workaround
		if(isItIn(&genRect(132,sCREATURE_WIDTH,pos.x+curx,pos.y+64),GH.current->motion.x,GH.current->motion.y))
		{
			which = i;
			int newAmount = std::min(amounts[i],creatures[i].amount);
			slider->setAmount(newAmount);
			if(slider->value > newAmount)
				slider->moveTo(newAmount);
			else
				slider->moveTo(slider->value);
			curx = 192 + 51 - (CREATURE_WIDTH*creatures.size()/2) - (SPACE_BETWEEN*(creatures.size()-1)/2);
			for(int j=0;j<creatures.size();j++)
			{
				if(which==j)
					drawBorder(bitmap,curx,64,CREATURE_WIDTH,132,int3(255,0,0));
				else
					drawBorder(bitmap,curx,64,CREATURE_WIDTH,132,int3(239,215,123));
				curx += TOTAL_CREATURE_WIDTH;
			}
			break;
		}
		curx += TOTAL_CREATURE_WIDTH;
	}
}
void CRecruitmentWindow::clickRight(tribool down, bool previousState)
{
	if(down)
	{
		int curx = 192 + 51 - (CREATURE_WIDTH*creatures.size()/2) - (SPACE_BETWEEN*(creatures.size()-1)/2);
		for(int i=0;i<creatures.size();i++)
		{
			const int sCREATURE_WIDTH = CREATURE_WIDTH; // gcc -O0 workaround
			if(isItIn(&genRect(132,sCREATURE_WIDTH,pos.x+curx,pos.y+64),GH.current->motion.x,GH.current->motion.y))
			{
				CCreInfoWindow *popup = new CCreInfoWindow(creatures[i].ID, 0, 0, NULL, NULL, NULL, NULL);
				GH.pushInt(popup);
				break;
			}
			curx += TOTAL_CREATURE_WIDTH;
		}
	}
}

void CRecruitmentWindow::activate()
{
	activateLClick();
	activateRClick();
	buy->activate();
	max->activate();
	cancel->activate();
	slider->activate();
	LOCPLINT->statusbar = bar;
}

void CRecruitmentWindow::deactivate()
{
	deactivateLClick();
	deactivateRClick();
	buy->deactivate();
	max->deactivate();
	cancel->deactivate();
	slider->deactivate();
}

void CRecruitmentWindow::show(SDL_Surface * to)
{
	static char animCounter=0; //animation counter - for determining appropriate animation frame to be shown
	blitAt(bitmap,pos.x,pos.y,to);
	buy->show(to);
	max->show(to);
	cancel->show(to);
	slider->show(to);

	char pom[15];
	SDL_itoa(creatures[which].amount-slider->value,pom,10); //available
	printAtMiddle(pom,pos.x+205,pos.y+254,FONT_SMALL,zwykly,to);
	SDL_itoa(slider->value,pom,10); //recruit
	printAtMiddle(pom,pos.x+279,pos.y+254,FONT_SMALL,zwykly,to);
	printAtMiddle(CGI->generaltexth->allTexts[16] + " " + CGI->creh->creatures[creatures[which].ID].namePl,pos.x+243,pos.y+32,FONT_BIG,tytulowy,to); //eg "Recruit Dragon flies"

	int curx = pos.x+115-creatures[which].res.size()*16;
	for(int i=0;i<creatures[which].res.size();i++)
	{
		blitAt(graphics->resources32->ourImages[creatures[which].res[i].first].bitmap,curx,pos.y+243,to);
		blitAt(graphics->resources32->ourImages[creatures[which].res[i].first].bitmap,curx+258,pos.y+243,to);
		SDL_itoa(creatures[which].res[i].second,pom,10);
		printAtMiddle(pom,curx+14,pos.y+288,FONT_SMALL,zwykly,to);
		SDL_itoa(creatures[which].res[i].second * slider->value,pom,10);
		printAtMiddle(pom,curx+12+258,pos.y+286,FONT_SMALL,zwykly,to);
		curx+=32;
	}

	curx = pos.x + 192 + CREATURE_WIDTH - (CREATURE_WIDTH*creatures.size()/2) - (SPACE_BETWEEN*(creatures.size()-1)/2);
	for(int i=0; i<creatures.size(); ++i)
	{
		creatures[i].pic->blitPic(to, curx-50, pos.y+130-65, !(animCounter%4));
		curx += TOTAL_CREATURE_WIDTH;
	}

	++animCounter;
	bar->show(to);
}

CRecruitmentWindow::CRecruitmentWindow(const CGDwelling *Dwelling, int Level, const CArmedInstance *Dst, const boost::function<void(int,int)> &Recruit)
:recruit(Recruit), dwelling(Dwelling), dst(Dst), level(Level)
{
	which = 0;
	SDL_Surface *hhlp = BitmapHandler::loadBitmap("TPRCRT.bmp");
	graphics->blueToPlayersAdv(hhlp,LOCPLINT->playerID);
	bitmap = SDL_ConvertSurface(hhlp,screen->format,0);
	SDL_SetColorKey(bitmap,SDL_SRCCOLORKEY,SDL_MapRGB(bitmap->format,0,255,255));
	SDL_FreeSurface(hhlp);
	pos.x = screen->w/2 - bitmap->w/2;
	pos.y = screen->h/2 - bitmap->h/2;
	pos.w = bitmap->w;
	pos.h = bitmap->h;
	bar = new CStatusBar(pos.x+8, pos.y+370, "APHLFTRT.bmp", 471);
	max = new AdventureMapButton(CGI->generaltexth->zelp[553],boost::bind(&CRecruitmentWindow::Max,this),pos.x+134,pos.y+313,"IRCBTNS.DEF",SDLK_m);
	buy = new AdventureMapButton(CGI->generaltexth->zelp[554],boost::bind(&CRecruitmentWindow::Buy,this),pos.x+212,pos.y+313,"IBY6432.DEF",SDLK_RETURN);
	cancel = new AdventureMapButton(CGI->generaltexth->zelp[555],boost::bind(&CRecruitmentWindow::Cancel,this),pos.x+290,pos.y+313,"ICN6432.DEF",SDLK_ESCAPE);
	slider = new CSlider(pos.x+176,pos.y+279,135,boost::bind(&CRecruitmentWindow::sliderMoved,this, _1),0,0,0,true);

	initCres();

	printAtMiddle(CGI->generaltexth->allTexts[346],113,233,FONT_SMALL,zwykly,bitmap); //cost per troop t
	printAtMiddle(CGI->generaltexth->allTexts[465],205,233,FONT_SMALL,zwykly,bitmap); //available t
	printAtMiddle(CGI->generaltexth->allTexts[16],279,233,FONT_SMALL,zwykly,bitmap); //recruit t
	printAtMiddle(CGI->generaltexth->allTexts[466],373,233,FONT_SMALL,zwykly,bitmap); //total cost t
	drawBorder(bitmap,172,222,67,42,int3(239,215,123));
	drawBorder(bitmap,246,222,67,42,int3(239,215,123));
	drawBorder(bitmap,64,222,99,76,int3(239,215,123));
	drawBorder(bitmap,322,222,99,76,int3(239,215,123));
	drawBorder(bitmap,133,312,66,34,int3(173,142,66));
	drawBorder(bitmap,211,312,66,34,int3(173,142,66));
	drawBorder(bitmap,289,312,66,34,int3(173,142,66));

	//border for creatures
	int curx = 192 + 51 - (CREATURE_WIDTH*creatures.size()/2) - (SPACE_BETWEEN*(creatures.size()-1)/2);
	for(int i=0;i<creatures.size();i++)
	{
		creatures[i].pos.x = curx+1;
		creatures[i].pos.y = 65;
		creatures[i].pos.w = 100;
		creatures[i].pos.h = 130;
		if(which==i)
			drawBorder(bitmap,curx,64,CREATURE_WIDTH,132,int3(255,0,0));
		else
			drawBorder(bitmap,curx,64,CREATURE_WIDTH,132,int3(239,215,123));
		curx += TOTAL_CREATURE_WIDTH;
	}

	if(!creatures[0].amount ||  !amounts[0])
	{
		max->block(true);
		slider->block(true);
	}
	//buy->block(true); //not needed, will be blocked by initing slider on 0
}

CRecruitmentWindow::~CRecruitmentWindow()
{
	cleanCres();
	delete max;
	delete buy;
	delete cancel;
	SDL_FreeSurface(bitmap);
	delete slider;
	delete bar;
}

void CRecruitmentWindow::initCres()
{
	cleanCres();
	for(int i=0; i<dwelling->creatures.size(); i++)
	{
		if(level >= 0 && i != level) 
			continue;

		for(int j = dwelling->creatures[i].second.size() - 1; j >= 0 ; j--)
		{
			creatures.resize(creatures.size()+1);
			creinfo &cur = creatures.back();

			cur.amount = dwelling->creatures[i].first;
			cur.ID = dwelling->creatures[i].second[j];
			const CCreature *cre = &CGI->creh->creatures[cur.ID];
			cur.pic = new CCreaturePic(cre);

			for(int k=0; k<cre->cost.size(); k++)
				if(cre->cost[k])
					cur.res.push_back(std::make_pair(k,cre->cost[k]));
			amounts.push_back(cre->maxAmount(LOCPLINT->cb->getResourceAmount()));
		}
	}

	slider->setAmount(std::min(amounts[which],creatures[which].amount));
}

void CRecruitmentWindow::cleanCres()
{
	for(int i=0;i<creatures.size();i++)
	{
		delete creatures[i].pic;
	}
	creatures.clear();
	amounts.clear();
}

CSplitWindow::CSplitWindow(int cid, int max, CGarrisonInt *Owner, int Last, int val)
{
	last = Last;
	which = 1;
	c=cid;
	slider = NULL;
	gar = Owner;

	SDL_Surface *hhlp = BitmapHandler::loadBitmap("GPUCRDIV.bmp");
	graphics->blueToPlayersAdv(hhlp,LOCPLINT->playerID);
	bitmap = SDL_ConvertSurface(hhlp,screen->format,0);
	SDL_SetColorKey(bitmap,SDL_SRCCOLORKEY,SDL_MapRGB(bitmap->format,0,255,255));
	SDL_FreeSurface(hhlp);
	pos.x = screen->w/2 - bitmap->w/2;
	pos.y = screen->h/2 - bitmap->h/2;
	pos.w = bitmap->w;
	pos.h = bitmap->h;
	ok = new AdventureMapButton("","",boost::bind(&CSplitWindow::split,this),pos.x+20,pos.y+263,"IOK6432.DEF",SDLK_RETURN);
	cancel = new AdventureMapButton("","",boost::bind(&CSplitWindow::close,this),pos.x+214,pos.y+263,"ICN6432.DEF",SDLK_ESCAPE);
	int sliderPositions = max - (last>=0) - (last==2);
	slider = new CSlider(pos.x+21,pos.y+194,257,boost::bind(&CSplitWindow::sliderMoved,this,_1),0,sliderPositions,val,true);
	a1 = max-val;
	a2 = val;
	anim = new CCreaturePic(&CGI->creh->creatures[cid]);
	anim->anim->setType(1);

	std::string title = CGI->generaltexth->allTexts[256];
	boost::algorithm::replace_first(title,"%s",CGI->creh->creatures[cid].namePl);
	printAtMiddle(title,150,34,FONT_BIG,tytulowy,bitmap);
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
	activateLClick();
	activateKeys();
	ok->activate();
	cancel->activate();
	slider->activate();
}

void CSplitWindow::deactivate()
{
	deactivateLClick();
	deactivateKeys();
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
	GH.popIntTotally(this);
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
	printAtMiddle(boost::lexical_cast<std::string>(a1) + (!which ? "_" : ""),pos.x+70,pos.y+237,FONT_BIG,zwykly,to);
	printAtMiddle(boost::lexical_cast<std::string>(a2) + (which ? "_" : ""),pos.x+233,pos.y+237,FONT_BIG,zwykly,to);
	anim->blitPic(to,pos.x+20,pos.y+54,false);
	anim->blitPic(to,pos.x+177,pos.y+54,false);
}

void CSplitWindow::keyPressed (const SDL_KeyboardEvent & key)
{
	SDLKey k = key.keysym.sym;
	if (isNumKey(k)) //convert numpad number to normal digit
		k = numToDigit(k); 

	if(key.state != SDL_PRESSED)
		return;

	int &cur = (which ? a2 : a1), 
		&sec = (which ? a1 : a2), 
		ncur = cur;
	if (k == SDLK_BACKSPACE)
	{
		ncur /= 10;
	}
	else if(k == SDLK_TAB)
	{
		which = !which;
	}
	else
	{
		int number = k - SDLK_0;
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

void CSplitWindow::clickLeft(tribool down, bool previousState)
{
	if(down)
	{
		Point click(GH.current->motion.x,GH.current->motion.y);
		click = click - pos.topLeft();
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
		printTo(count.c_str(),pos.x+114,pos.y+174,FONT_TIMES,zwykly,to);
	if(upgrade)
		upgrade->show(to);
	if(dismiss)
		dismiss->show(to);
	if(ok)
		ok->show(to);
}

CCreInfoWindow::CCreInfoWindow(int Cid, int Type, int creatureCount, StackState *State, boost::function<void()> Upg, boost::function<void()> Dsm, UpgradeInfo *ui)
	: type(Type), dsm(Dsm), dismiss(0), upgrade(0), ok(0)
{
	//active = false;
	anf = 0;
	c = &CGI->creh->creatures[Cid];
	SDL_Surface *hhlp = BitmapHandler::loadBitmap("CRSTKPU.bmp");
	graphics->blueToPlayersAdv(hhlp,LOCPLINT->playerID);
	bitmap = SDL_ConvertSurface(hhlp,screen->format,0);
	SDL_SetColorKey(bitmap,SDL_SRCCOLORKEY,SDL_MapRGB(bitmap->format,0,255,255));
	SDL_FreeSurface(hhlp);
	pos.x = screen->w/2 - bitmap->w/2;
	pos.y = screen->h/2 - bitmap->h/2;
	pos.w = bitmap->w;
	pos.h = bitmap->h;
	anim = new CCreaturePic(c);
	if(!type) anim->anim->setType(2);

	char pom[75];int hlp=0;

	if(creatureCount)
	{
		SDL_itoa(creatureCount,pom,10);
		count = pom;
	}

	printAtMiddle(c->namePl,149,30,FONT_SMALL,tytulowy,bitmap); //creature name

	//atttack
	printAt(CGI->generaltexth->primarySkillNames[0],155,48,FONT_SMALL,zwykly,bitmap);
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
	printTo(pom,276,61,FONT_SMALL,zwykly,bitmap);

	//defense
	printAt(CGI->generaltexth->primarySkillNames[1],155,67,FONT_SMALL,zwykly,bitmap);
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
	printTo(pom,276,80,FONT_SMALL,zwykly,bitmap);

	//shots
	if(c->shots)
	{
		printAt(CGI->generaltexth->allTexts[198], 155, 86, FONT_SMALL, zwykly, bitmap);
		if(State  &&  State->shotsLeft >= 0)
			sprintf(pom,"%d (%d)", c->shots, State->shotsLeft);
		else
			SDL_itoa(c->shots, pom, 10);
		printTo(pom, 276, 99, FONT_SMALL, zwykly, bitmap);
	}

	//damage
	int dmgMin = c->damageMin * (State ? State->dmgMultiplier : 1);
	int dmgMax = c->damageMax * (State ? State->dmgMultiplier : 1);

	printAt(CGI->generaltexth->allTexts[199], 155, 105, FONT_SMALL, zwykly, bitmap);
	SDL_itoa(dmgMin, pom, 10);
	if(dmgMin > 0)
		hlp = log10f(dmgMin) + 2;
	else
		hlp = 2;
	pom[hlp-1]=' '; pom[hlp]='-'; pom[hlp+1]=' ';
	SDL_itoa(dmgMax, pom+hlp+2, 10);
	printTo(pom, 276, 118, FONT_SMALL, zwykly, bitmap);

	//health
	printAt(CGI->generaltexth->allTexts[388],155,124,FONT_SMALL,zwykly,bitmap);
	if(State  &&  State->healthBonus)
		sprintf(pom,"%d (%d)",c->hitPoints, c->hitPoints + State->healthBonus);
	else
		SDL_itoa(c->hitPoints,pom,10);
	printTo(pom,276,137,FONT_SMALL,zwykly,bitmap);

	//remaining health
	if(State && State->currentHealth)
	{
		printAt(CGI->generaltexth->allTexts[200],155,143,FONT_SMALL,zwykly,bitmap);
		SDL_itoa(State->currentHealth,pom,10);
		printTo(pom,276,156,FONT_SMALL,zwykly,bitmap);
	}

	//speed
	printAt(CGI->generaltexth->zelp[441].first,155,162,FONT_SMALL,zwykly,bitmap);
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
	printTo(pom,276,175,FONT_SMALL,zwykly,bitmap);


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
		printAtWB(c->abilityText,17,231,FONT_SMALL,35,zwykly,bitmap);
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
		activateRClick();
	if(ok)
		ok->activate();
	if(dismiss)
		dismiss->activate();
	if(upgrade)
		upgrade->activate();
}

void CCreInfoWindow::close()
{
	GH.popIntTotally(this);
}

void CCreInfoWindow::clickRight(tribool down, bool previousState)
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
		deactivateRClick();
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
	GH.popIntTotally(this);
	LOCPLINT->showingDialog->setn(false);
}

CLevelWindow::CLevelWindow(const CGHeroInstance *hero, int pskill, std::vector<ui16> &skills, boost::function<void(ui32)> &callback)
{
	LOCPLINT->showingDialog->setn(true);
	heroPortrait = hero->portrait;
	cb = callback;
	for(int i=0;i<skills.size();i++)
	{
		comps.push_back(new CSelectableComponent(SComponent::secskill44,skills[i],hero->getSecSkillLevel(skills[i])+1,boost::bind(&CLevelWindow::selectionChanged,this,i)));
		comps.back()->assignedKeys.insert(SDLK_1 + i);
	}
	SDL_Surface *hhlp = BitmapHandler::loadBitmap("LVLUPBKG.bmp");
	graphics->blueToPlayersAdv(hhlp,LOCPLINT->playerID);
	bitmap = SDL_ConvertSurface(hhlp,screen->format,0);
	SDL_SetColorKey(bitmap,SDL_SRCCOLORKEY,SDL_MapRGB(bitmap->format,0,255,255));
	SDL_FreeSurface(hhlp);

	pos.x = screen->w/2 - bitmap->w/2;
	pos.y = screen->h/2 - bitmap->h/2;
	pos.w = bitmap->w;
	pos.h = bitmap->h;
	ok = new AdventureMapButton("","",boost::bind(&CLevelWindow::close,this),pos.x+297,pos.y+413,"IOKAY.DEF",SDLK_RETURN);
	//draw window
	char buf[100], buf2[100];
	strcpy(buf2,CGI->generaltexth->allTexts[444].c_str()); //%s has gained a level.
	sprintf(buf,buf2,hero->name.c_str());
	printAtMiddle(buf,192,33,FONT_MEDIUM,zwykly,bitmap);

	strcpy(buf2,CGI->generaltexth->allTexts[445].c_str()); //%s is now a level %d %s.
	sprintf(buf,buf2,hero->name.c_str(),hero->level,hero->type->heroClass->name.c_str());
	printAtMiddle(buf,192,162,FONT_MEDIUM,zwykly,bitmap);

	blitAt(graphics->pskillsm->ourImages[pskill].bitmap,174,190,bitmap);

	printAtMiddle((CGI->generaltexth->primarySkillNames[pskill] + " +1"),192,253,FONT_MEDIUM,zwykly,bitmap);

	const Font *f = graphics->fonts[FONT_MEDIUM];
	std::string text = CGI->generaltexth->allTexts[4];
	int fontWidth = f->getWidth(text.c_str())/2;
	int curx = bitmap->w/2 - ( skills.size()*44 + (skills.size()-1)*(36+fontWidth) )/2;

	for(int i=0;i<comps.size();i++)
	{
		comps[i]->pos.x = curx+pos.x;
		comps[i]->pos.y = 326+pos.y;
		if( i < (comps.size()-1) )
		{
			curx += 44+21; //skill width + margin to "or"
			printAtMiddle(text ,curx,346,FONT_MEDIUM,zwykly,bitmap);
			curx += fontWidth+15;
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
	blitAt(graphics->portraitLarge[heroPortrait],170+pos.x,66+pos.y,to);
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
		CSDL_Ext::printAtMiddle(buf,pos.x + 50 + 76*i,pos.y+pos.h/2,FONT_SMALL,zwykly,to);
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
		,pos.x+545+(pos.w-545)/2,pos.y+pos.h/2,FONT_SMALL,zwykly,to);
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

void CMarketplaceWindow::CTradeableItem::clickLeft(tribool down, bool previousState)
{
	CMarketplaceWindow *mw = dynamic_cast<CMarketplaceWindow *>(GH.topInt());
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
	activateLClick();
}

void CMarketplaceWindow::CTradeableItem::deactivate()
{
	deactivateLClick();
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
static void initItems( std::vector<CMarketplaceWindow::CTradeableItem*> &i, std::vector<SDL_Rect> &p, int type, int amount, bool left, std::vector<int> *ids/*=NULL*/ )
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
			lpos += genRect(66,74,39,180), genRect(66,74,122,180), genRect(66,74,204,180),
				genRect(66,74,39,259), genRect(66,74,122,259), genRect(66,74,204,259),
				genRect(66,74,122,338);
			for(int i=0;i<lpos.size();i++)
			{
				lpos[i].x += pos.x;
				lpos[i].y += pos.y;
				rpos.push_back(lpos[i]);
				rpos[rpos.size()-1].x += 288;
			}
			initItems(left,lpos,0,7,true,NULL);
			initItems(right,rpos,0,7,false,NULL);
			printAtMiddle(CGI->generaltexth->allTexts[158],300,27,FONT_BIG,tytulowy,bg); //title
			printAtMiddle(CGI->generaltexth->allTexts[270],154,148,FONT_SMALL,zwykly,bg); //kingdom res.
			printAtMiddle(CGI->generaltexth->allTexts[168],445,147,FONT_SMALL,zwykly,bg); //available for trade
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
	type = BLOCK_ADV_HOTKEYS;
	mode = Mode;
	bg = NULL;
	ok = max = deal = NULL;
	pos.x = screen->w/2 - 300;
	pos.y = screen->h/2 - 296;
	slider = new CSlider(pos.x+231,pos.y+490,137,boost::bind(&CMarketplaceWindow::sliderMoved,this,_1),0,0);
	setMode(mode);
	hLeft = hRight = NULL;
	ok = new AdventureMapButton("","",boost::bind(&CGuiHandler::popIntTotally,&GH,this),pos.x+516,pos.y+520,"IOK6432.DEF",SDLK_RETURN);
	ok->assignedKeys.insert(SDLK_ESCAPE);
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
			printAtMiddle(buf,left[i]->pos.x+36,left[i]->pos.y+57,FONT_SMALL,zwykly,to);
		}
		if(hLeft) //print prices
		{
			for(int i=0; i<right.size();i++)
			{
				if(right[i]->id != hLeft->id)
					printAtMiddle(rSubs[i],right[i]->pos.x+36,right[i]->pos.y+57,FONT_SMALL,zwykly,to);
				else
					printAtMiddle(CGI->generaltexth->allTexts[164],right[i]->pos.x+36,right[i]->pos.y+57,FONT_SMALL,zwykly,to);
			}
		}
		if(hLeft && hRight && hLeft->id!= hRight->id)
		{
			blitAt(hLeft->getSurface(),pos.x+141,pos.y+457,to);
			blitAt(hRight->getSurface(),pos.x+429,pos.y+457,to);
			SDL_itoa(slider->value * r1,buf,10);
			printAtMiddle(buf,pos.x+156,pos.y+505,FONT_SMALL,zwykly,to);
			SDL_itoa(slider->value * r2,buf,10);
			printAtMiddle(buf,pos.x+443,pos.y+505,FONT_SMALL,zwykly,to);
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
		slider->setAmount(LOCPLINT->cb->getResourceAmount(hLeft->id) / r1);
		slider->moveTo(0);
		max->block(false);
		deal->block(false);
	}
	else
	{
		max->block(true);
		deal->block(true);
		slider->setAmount(0);
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
	SDL_Surface *hhlp = BitmapHandler::loadBitmap("SysOpbck.bmp", true);
	graphics->blueToPlayersAdv(hhlp,LOCPLINT->playerID);
	background = SDL_ConvertSurface(hhlp,screen->format,0);
	SDL_SetColorKey(background,SDL_SRCCOLORKEY,SDL_MapRGB(background->format,0,255,255));
	SDL_FreeSurface(hhlp);

	//printing texts
	CSDL_Ext::printAtMiddle(CGI->generaltexth->allTexts[568], 242, 32, FONT_BIG, tytulowy, background); //window title
	CSDL_Ext::printAtMiddle(CGI->generaltexth->allTexts[569], 122, 64, FONT_MEDIUM, tytulowy, background); //hero speed
	CSDL_Ext::printAtMiddle(CGI->generaltexth->allTexts[570], 122, 130, FONT_MEDIUM, tytulowy, background); //enemy speed
	CSDL_Ext::printAtMiddle(CGI->generaltexth->allTexts[571], 122, 196, FONT_MEDIUM, tytulowy, background); //map scroll speed
	CSDL_Ext::printAtMiddle(CGI->generaltexth->allTexts[20], 122, 262, FONT_MEDIUM, tytulowy, background); //video quality
	CSDL_Ext::printAtMiddle(CGI->generaltexth->allTexts[394], 122, 347, FONT_MEDIUM, tytulowy, background); //music volume
	CSDL_Ext::printAtMiddle(CGI->generaltexth->allTexts[395], 122, 412, FONT_MEDIUM, tytulowy, background); //effects volume

	CSDL_Ext::printAt(CGI->generaltexth->allTexts[572], 282, 57, FONT_MEDIUM, zwykly, background); //show move path
	CSDL_Ext::printAt(CGI->generaltexth->allTexts[573], 282, 89, FONT_MEDIUM, zwykly, background); //show hero reminder
	CSDL_Ext::printAt(CGI->generaltexth->allTexts[574], 282, 121, FONT_MEDIUM, zwykly, background); //quick combat
	CSDL_Ext::printAt(CGI->generaltexth->allTexts[575], 282, 153, FONT_MEDIUM, zwykly, background); //video subtitles
	CSDL_Ext::printAt(CGI->generaltexth->allTexts[576], 282, 185, FONT_MEDIUM, zwykly, background); //town building outlines
	CSDL_Ext::printAt(CGI->generaltexth->allTexts[577], 282, 217, FONT_MEDIUM, zwykly, background); //spell book animation

	//setting up buttons
	// load = new AdventureMapButton (CGI->generaltexth->zelp[321].first, CGI->generaltexth->zelp[321].second, boost::bind(&CSystemOptionsWindow::loadf, this), pos.x+246, pos.y+298, "SOLOAD.DEF", SDLK_l);
	// std::swap(save->imgs[0][0], load->imgs[0][1]);

	save = new AdventureMapButton (CGI->generaltexth->zelp[322].first, CGI->generaltexth->zelp[322].second, boost::bind(&CSystemOptionsWindow::bsavef, this), pos.x+357, pos.y+298, "SOSAVE.DEF", SDLK_s);
	std::swap(save->imgs[0][0], save->imgs[0][1]);

	// restart = new AdventureMapButton (CGI->generaltexth->zelp[323].first, CGI->generaltexth->zelp[323].second, boost::bind(&CSystemOptionsWindow::bmainmenuf, this), pos.x+346, pos.y+357, "SORSTRT", SDLK_r);
	// std::swap(save->imgs[0][0], restart->imgs[0][1]);

	mainMenu = new AdventureMapButton (CGI->generaltexth->zelp[320].first, CGI->generaltexth->zelp[320].second, boost::bind(&CSystemOptionsWindow::bmainmenuf, this), pos.x+357, pos.y+357, "SOMAIN.DEF", SDLK_m);
	std::swap(mainMenu->imgs[0][0], mainMenu->imgs[0][1]);

	quitGame = new AdventureMapButton (CGI->generaltexth->zelp[324].first, CGI->generaltexth->zelp[324].second, boost::bind(&CSystemOptionsWindow::bquitf, this), pos.x+246, pos.y+415, "soquit.def", SDLK_q);
	std::swap(quitGame->imgs[0][0], quitGame->imgs[0][1]);

	backToMap = new AdventureMapButton (CGI->generaltexth->zelp[325].first, CGI->generaltexth->zelp[325].second, boost::bind(&CSystemOptionsWindow::breturnf, this), pos.x+357, pos.y+415, "soretrn.def", SDLK_RETURN);
	std::swap(backToMap->imgs[0][0], backToMap->imgs[0][1]);
	backToMap->assignedKeys.insert(SDLK_ESCAPE);

	heroMoveSpeed = new CHighlightableButtonsGroup(0);
	heroMoveSpeed->addButton(boost::assign::map_list_of(0,CGI->generaltexth->zelp[349].second),CGI->generaltexth->zelp[349].second, "sysopb1.def", pos.x+28, pos.y+77, 1);
	heroMoveSpeed->addButton(boost::assign::map_list_of(0,CGI->generaltexth->zelp[350].second),CGI->generaltexth->zelp[350].second, "sysopb2.def", pos.x+76, pos.y+77, 2);
	heroMoveSpeed->addButton(boost::assign::map_list_of(0,CGI->generaltexth->zelp[351].second),CGI->generaltexth->zelp[351].second, "sysopb3.def", pos.x+124, pos.y+77, 4);
	heroMoveSpeed->addButton(boost::assign::map_list_of(0,CGI->generaltexth->zelp[352].second),CGI->generaltexth->zelp[352].second, "sysopb4.def", pos.x+172, pos.y+77, 8);
	heroMoveSpeed->select(owner->sysOpts.heroMoveSpeed, 1);
	heroMoveSpeed->onChange = boost::bind(&SystemOptions::setHeroMoveSpeed, &owner->sysOpts, _1);

	mapScrollSpeed = new CHighlightableButtonsGroup(0);
	mapScrollSpeed->addButton(boost::assign::map_list_of(0,CGI->generaltexth->zelp[357].second),CGI->generaltexth->zelp[357].second, "sysopb9.def", pos.x+28, pos.y+210, 1);
	mapScrollSpeed->addButton(boost::assign::map_list_of(0,CGI->generaltexth->zelp[358].second),CGI->generaltexth->zelp[358].second, "sysob10.def", pos.x+92, pos.y+210, 2);
	mapScrollSpeed->addButton(boost::assign::map_list_of(0,CGI->generaltexth->zelp[359].second),CGI->generaltexth->zelp[359].second, "sysob11.def", pos.x+156, pos.y+210, 4);
	mapScrollSpeed->select(owner->sysOpts.mapScrollingSpeed, 1);
	mapScrollSpeed->onChange = boost::bind(&SystemOptions::setMapScrollingSpeed, &owner->sysOpts, _1);

	musicVolume = new CHighlightableButtonsGroup(0, true);
	for(int i=0; i<10; ++i)
	{
		musicVolume->addButton(boost::assign::map_list_of(0,CGI->generaltexth->zelp[326+i].second),CGI->generaltexth->zelp[326+i].second, "syslb.def", pos.x+29 + 19*i, pos.y+359, i*11);
	}
	musicVolume->select(CGI->musich->getVolume(), 1);
	musicVolume->onChange = boost::bind(&SystemOptions::setMusicVolume, &owner->sysOpts, _1);

	effectsVolume = new CHighlightableButtonsGroup(0, true);
	for(int i=0; i<10; ++i)
	{
		effectsVolume->addButton(boost::assign::map_list_of(0,CGI->generaltexth->zelp[336+i].second),CGI->generaltexth->zelp[336+i].second, "syslb.def", pos.x+29 + 19*i, pos.y+425, i*11);
	}
	effectsVolume->select(CGI->soundh->getVolume(), 1);
	effectsVolume->onChange = boost::bind(&SystemOptions::setSoundVolume, &owner->sysOpts, _1);
}

CSystemOptionsWindow::~CSystemOptionsWindow()
{
	SDL_FreeSurface(background);

	delete save;
	delete quitGame;
	delete backToMap;
	delete mainMenu;
	delete heroMoveSpeed;
	delete mapScrollSpeed;
	delete musicVolume;
	delete effectsVolume;
}

void CSystemOptionsWindow::pushSDLEvent(int type, int usercode)
{
	GH.popIntTotally(this);

	SDL_Event event;
	event.type = type;
	event.user.code = usercode;	// not necessarily used
	SDL_PushEvent(&event);
}

void CSystemOptionsWindow::bquitf()
{
	LOCPLINT->showYesNoDialog(CGI->generaltexth->allTexts[578], std::vector<SComponent*>(), boost::bind(&CSystemOptionsWindow::pushSDLEvent, this, SDL_QUIT, 0), 0, false);
}

void CSystemOptionsWindow::breturnf()
{
	GH.popIntTotally(this);
}

void CSystemOptionsWindow::bmainmenuf()
{
	LOCPLINT->showYesNoDialog(CGI->generaltexth->allTexts[578], std::vector<SComponent*>(), boost::bind(&CSystemOptionsWindow::pushSDLEvent, this, SDL_USEREVENT, 2), 0, false);
}

void CSystemOptionsWindow::bsavef()
{
	GH.popIntTotally(this);
	GH.pushInt(new CSelectionScreen(saveGame));
	/*using namespace boost::posix_time;
	std::ostringstream fnameStream;
	fnameStream << second_clock::local_time();
	std::string fname = fnameStream.str();
	boost::algorithm::replace_all(fname,":","");
	boost::algorithm::replace_all(fname," ","-");
	LOCPLINT->showYesNoDialog("Do you want to save current game as " + fname, std::vector<SComponent*>(), boost::bind(&CCallback::save, LOCPLINT->cb, fname), boost::bind(&CSystemOptionsWindow::activate, this), false);*/
}

void CSystemOptionsWindow::activate()
{
	save->activate();
	quitGame->activate();
	backToMap->activate();
	mainMenu->activate();
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
	mainMenu->deactivate();
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
	mainMenu->show(to);
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
	oldSelected = -1;

	SDL_Surface *hhlp = BitmapHandler::loadBitmap("TPTAVERN.bmp");
	graphics->blueToPlayersAdv(hhlp,LOCPLINT->playerID);
	bg = SDL_ConvertSurface(hhlp,screen->format,0);
	SDL_SetColorKey(bg,SDL_SRCCOLORKEY,SDL_MapRGB(bg->format,0,255,255));
	SDL_FreeSurface(hhlp);

	printAtMiddle(CGI->generaltexth->jktexts[37],200,35,FONT_BIG,tytulowy,bg);
	printAtMiddle("2500",320,328,FONT_SMALL,zwykly,bg);
//	printAtMiddle(CGI->generaltexth->jktexts[38],146,283,FONT_BIG,tytulowy,bg); //what is this???
	printAtMiddleWB(gossip,200,220,FONT_SMALL,50,zwykly,bg);
	pos.w = bg->w;
	pos.h = bg->h;
	pos.x = (screen->w-bg->w)/2;
	pos.y = (screen->h-bg->h)/2;
	bar = new CStatusBar(pos.x+8, pos.y+478, "APHLFTRT.bmp", 380);
	h1.pos.x += pos.x;
	h2.pos.x += pos.x;
	h1.pos.y += pos.y;
	h2.pos.y += pos.y;

	cancel = new AdventureMapButton(CGI->generaltexth->tavernInfo[7],"", boost::bind(&CTavernWindow::close, this), pos.x+310,pos.y+428, "ICANCEL.DEF", SDLK_ESCAPE);
	recruit = new AdventureMapButton("", "", boost::bind(&CTavernWindow::recruitb, this), pos.x+272, pos.y+355, "TPTAV01.DEF", SDLK_RETURN);
	thiefGuild = new AdventureMapButton(CGI->generaltexth->tavernInfo[5],"", boost::bind(&CTavernWindow::thievesguildb, this), pos.x+22, pos.y+428, "TPTAV02.DEF", SDLK_t);

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
		if(!H1)
			recruit->block(1);
	}

#ifdef _WIN32
	CGI->videoh->open("TAVERN.BIK");
#else
	CGI->videoh->open("tavern.mjpg", true, false);
#endif
}

void CTavernWindow::recruitb()
{
	const CGHeroInstance *toBuy = (selected ? h2 : h1).h;
	close();
	LOCPLINT->cb->recruitHero(LOCPLINT->castleInt->town,toBuy);
}

void CTavernWindow::thievesguildb()
{
	GH.pushInt( new CThievesGuildWindow(LOCPLINT->castleInt->town) );
}

CTavernWindow::~CTavernWindow()
{
	CGI->videoh->close();
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
	GH.popIntTotally(this);
}

void CTavernWindow::show(SDL_Surface * to)
{
	blitAt(bg,pos.x,pos.y,to);
	CGI->videoh->update(pos.x+70, pos.y+56, to, true, false);
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

		if (selected != oldSelected  &&  !recruit->blocked) 
		{
			// Selected hero just changed. Update RECRUIT button hover text if recruitment is allowed.
			oldSelected = selected;

			recruit->hoverTexts[0] = CGI->generaltexth->tavernInfo[3]; //Recruit %s the %s
			boost::algorithm::replace_first(recruit->hoverTexts[0],"%s",sel->h->name);
			boost::algorithm::replace_first(recruit->hoverTexts[0],"%s",sel->h->type->heroClass->name);
		}

		printAtMiddleWB(sel->descr,pos.x+146,pos.y+389,FONT_SMALL,40,zwykly,to);
		CSDL_Ext::drawBorder(to,sel->pos.x-2,sel->pos.y-2,sel->pos.w+4,sel->pos.h+4,int3(247,223,123));
	}
}

void CTavernWindow::HeroPortrait::clickLeft(tribool down, bool previousState)
{
	if(previousState && !down)
		as();
	//ClickableL::clickLeft(down);
}

void CTavernWindow::HeroPortrait::activate()
{
	activateLClick();
	activateRClick();
	activateHover();
}

void CTavernWindow::HeroPortrait::deactivate()
{
	deactivateLClick();
	deactivateRClick();
	deactivateHover();
}

void CTavernWindow::HeroPortrait::clickRight(tribool down, bool previousState)
{
	if(down)
	{
		LOCPLINT->adventureInt->heroWindow->setHero(h);
		GH.pushInt(new CRClickPopupInt(LOCPLINT->adventureInt->heroWindow,false));
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

		int artifs = h->artifWorn.size() + h->artifacts.size();
		for(int i=13; i<=17; i++) //war machines and spellbook don't count
			if(vstd::contains(h->artifWorn,i)) 
				artifs--;
		sprintf_s(descr, sizeof(descr),CGI->generaltexth->allTexts[215].c_str(),
				  h->name.c_str(), h->level, h->type->heroClass->name.c_str(), artifs);
		descr[sizeof(descr)-1] = '\0';
	}

}
	
void CTavernWindow::HeroPortrait::show(SDL_Surface * to)
{
	blitAt(graphics->portraitLarge[h->subID],pos,to);
}

void CTavernWindow::HeroPortrait::hover( bool on )
{
	//Hoverable::hover(on);
	if(on)
		LOCPLINT->statusbar->print(hoverName);
	else
		LOCPLINT->statusbar->clear();
}

void CInGameConsole::activate()
{
	activateKeys();
}

void CInGameConsole::deactivate()
{
	deactivateKeys();
}

void CInGameConsole::show(SDL_Surface * to)
{
	int number = 0;

	std::vector<std::list< std::pair< std::string, int > >::iterator> toDel;

	texts_mx.lock();

	for(std::list< std::pair< std::string, int > >::iterator it = texts.begin(); it != texts.end(); ++it, ++number)
	{
		SDL_Color green = {0,0xff,0,0};
		Point leftBottomCorner(0, screen->h);
		if(LOCPLINT->battleInt)
		{
			leftBottomCorner = LOCPLINT->battleInt->pos.bottomLeft();
		}
		CSDL_Ext::printAt(it->first, leftBottomCorner.x + 50, leftBottomCorner.y - texts.size() * 20 - 80 + number*20, FONT_MEDIUM, green);
		if(SDL_GetTicks() - it->second > defaultTimeout)
		{
			toDel.push_back(it);
		}
	}

	for(int it=0; it<toDel.size(); ++it)
	{
		texts.erase(toDel[it]);
	}

	texts_mx.unlock();
}

void CInGameConsole::print(const std::string &txt)
{
	texts_mx.lock();
	int lineLen = conf.go()->ac.outputLineLength;

	if(txt.size() < lineLen)
	{
		texts.push_back(std::make_pair(txt, SDL_GetTicks()));
		if(texts.size() > maxDisplayedTexts)
		{
			texts.pop_front();
		}
	}
	else
	{
		assert(lineLen);
		for(int g=0; g<txt.size() / lineLen + 1; ++g)
		{
			std::string part = txt.substr(g * lineLen, lineLen);
			if(part.size() == 0)
				break;

			texts.push_back(std::make_pair(part, SDL_GetTicks()));
			if(texts.size() > maxDisplayedTexts)
			{
				texts.pop_front();
			}
		}
	}

	texts_mx.unlock();
}

void CInGameConsole::keyPressed (const SDL_KeyboardEvent & key)
{
	if(key.type != SDL_KEYDOWN) return;

	if(!captureAllKeys && key.keysym.sym != SDLK_TAB) return; //because user is not entering any text

	switch(key.keysym.sym)
	{
	case SDLK_TAB:
	case SDLK_ESCAPE:
		{
			if(captureAllKeys)
			{
				captureAllKeys = false;
				endEnteringText(false);
			}
			else if(SDLK_TAB)
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
			if(previouslyEntered.size() == 0)
				break;

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
			if(enteredText.size() > 0 && enteredText.size() < conf.go()->ac.inputLineLength)
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
	if(GH.topInt() == LOCPLINT->adventureInt)
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
	if(GH.topInt() == LOCPLINT->adventureInt)
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
	if(GH.topInt() == LOCPLINT->adventureInt)
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
	GH.popIntTotally(this);
}

void CGarrisonWindow::activate()
{
	quit->activate();
	garr->activate();
}

void CGarrisonWindow::deactivate()
{
	quit->deactivate();
	garr->deactivate();
}

void CGarrisonWindow::show(SDL_Surface * to)
{
	blitAt(bg,pos,to);
	quit->show(to);
	garr->show(to);

	blitAt(graphics->flags->ourImages[garr->odown->getOwner()].bitmap,pos.x+28,pos.y+124,to);
	blitAt(graphics->portraitLarge[static_cast<const CGHeroInstance*>(garr->odown)->portrait],pos.x+29,pos.y+222,to);
	printAtMiddle(CGI->generaltexth->allTexts[709],pos.x+275,pos.y+30,FONT_BIG,tytulowy,to);
}

CGarrisonWindow::CGarrisonWindow( const CArmedInstance *up, const CGHeroInstance *down, bool removableUnits )
{
	bg = BitmapHandler::loadBitmap("GARRISON.bmp");
	SDL_SetColorKey(bg,SDL_SRCCOLORKEY,SDL_MapRGB(bg->format,0,255,255));
	graphics->blueToPlayersAdv(bg,LOCPLINT->playerID);
	pos.x = screen->w/2 - bg->w/2;
	pos.y = screen->h/2 - bg->h/2;
	pos.w = screen->w;
	pos.h = screen->h;

	garr = new CGarrisonInt(pos.x+92, pos.y+127, 4, Point(0,96), bg, Point(93,127), up, down, removableUnits);
	garr->splitButtons.push_back(new AdventureMapButton(CGI->generaltexth->tcommands[3],"",boost::bind(&CGarrisonInt::splitClick,garr),pos.x+88,pos.y+314,"IDV6432.DEF"));
	quit = new AdventureMapButton(CGI->generaltexth->tcommands[8],"",boost::bind(&CGarrisonWindow::close,this),pos.x+399,pos.y+314,"IOK6432.DEF",SDLK_RETURN);
}

CGarrisonWindow::~CGarrisonWindow()
{
	SDL_FreeSurface(bg);
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
	CGI->curh->hide();
	inner = our;
	delInner = deleteInt;
}

CRClickPopupInt::~CRClickPopupInt()
{
	//workaround for hero window issue - if it's our interface, call dispose to properly reset it's state 
	//TODO? it might be better to rewrite hero window so it will bee newed/deleted on opening / closing (not effort-worthy now, but on some day...?)
	if(LOCPLINT && inner == LOCPLINT->adventureInt->heroWindow)
		LOCPLINT->adventureInt->heroWindow->dispose();

	if(delInner)
		delete inner;

	CGI->curh->show();
}

CArtPlace::CArtPlace(const CArtifact* Art): active(false), marked(false), ourArt(Art)/*,
	spellBook(false), warMachine1(false), warMachine2(false), warMachine3(false),
	warMachine4(false),misc1(false), misc2(false), misc3(false), misc4(false),
	misc5(false), feet(false), lRing(false), rRing(false), torso(false),
	lHand(false), rHand(false), neck(false), shoulders(false), head(false) */
{
}

void CArtPlace::activate()
{
	if(!active)
	{
		//activateLClick();
		LRClickableAreaWTextComp::activate();
		active = true;
	}
}

void CArtPlace::clickLeft(tribool down, bool previousState)
{
	//LRClickableAreaWTextComp::clickLeft(down);
	
	// If clicked on spellbook, open it only if no artifact is held at the moment.
	if(ourArt && !down && previousState && !ourOwner->commonInfo->srcAOH)
	{
		if(ourArt->id == 0)
		{
			CSpellWindow * spellWindow = new CSpellWindow(genRect(595, 620, (conf.cc.resx - 620)/2, (conf.cc.resy - 595)/2), ourOwner->curHero);
			GH.pushInt(spellWindow);
		}
	}
	if (!down && previousState)
	{
		if(ourArt && ourArt->id == 0)
			return; //this is handled separately
		if(!ourOwner->commonInfo->srcAOH) //nothing has been clicked
		{
			if(ourArt) //to prevent selecting empty slots (bugfix to what GrayFace reported)
			{
				if(ourArt->id == 3) //catapult cannot be highlighted
				{
					std::vector<SComponent *> catapult(1, new SComponent(SComponent::artifact, 3, 0));
					LOCPLINT->showInfoDialog(CGI->generaltexth->allTexts[312], catapult); //The Catapult must be equipped.
					return;
				}
				select();
			}
		}
		else //perform artifact substitution
		{
			if (slotID >= 19) // Backpack slot - Remove active artifact and insert it into the designated position in backpack.
			{
				const CArtifact * cur = ourOwner->commonInfo->srcArtifact;

				switch(cur->id)
				{
				case 3:
					//should not happen, catapult cannot be selected
					assert(cur->id != 3);
					break;
				case 4: case 5: case 6:
					{
						std::string text = CGI->generaltexth->allTexts[153];
						boost::algorithm::replace_first(text, "%s", cur->Name());
						LOCPLINT->showInfoDialog(text);
					}
					break;
				default:
					ourOwner->commonInfo->destAOH = ourOwner;
					ourOwner->commonInfo->destSlotID = slotID;
					ourOwner->commonInfo->destArtifact = NULL;

					LOCPLINT->cb->setArtifact(
						ourOwner->curHero,
						slotID,
						ourOwner->commonInfo->srcArtifact->id);

					break;
				}
			}
			//check if swap is possible
			else if (this->fitsHere(ourOwner->commonInfo->srcArtifact))
			{
				ourOwner->commonInfo->destAOH = ourOwner;
				ourOwner->commonInfo->destSlotID = slotID;
				ourOwner->commonInfo->destArtifact = ourArt;

				LOCPLINT->cb->setArtifact(
					ourOwner->curHero,
					slotID,
					ourOwner->commonInfo->srcArtifact->id);
			}
		}
	}
	/*else if(!down && clicked)
	{
		if(ourArt && ourArt->id == 0)
			return; //this is handled separately
		deselect();
	}*/
	//ClickableL::clickLeft(down);
}

void CArtPlace::clickRight(tribool down, bool previousState)
{
	if(text.size()) //if there is no description, do nothing ;]
		LRClickableAreaWTextComp::clickRight(down, previousState);
}

/**
 * Selects artifact slot so that the containing artifact looks like it's picked up.
 */
void CArtPlace::select ()
{
	CGI->curh->dragAndDropCursor(graphics->artDefs->ourImages[ourArt->id].bitmap);
	ourOwner->markPossibleSlots(ourArt);

	ourOwner->commonInfo->srcArtifact = ourArt;
	ourOwner->commonInfo->srcSlotID = slotID;
	ourOwner->commonInfo->srcAOH = ourOwner;

	if (slotID >= 19) {
		// Correcting position in backpack.
		ourOwner->scrollBackpack(-(slotID - 19 < ourOwner->backpackPos));
	} else {
		ourOwner->eraseSlotData(this, slotID);
	}
	LOCPLINT->cb->setArtifact(ourOwner->curHero, slotID, -1);
}

/**
 * Deselects the artifact slot.
 */
void CArtPlace::deselect ()
{
	CGI->curh->dragAndDropCursor(NULL);
	ourOwner->unmarkSlots();
}

void CArtPlace::deactivate()
{
	if(active)
	{
		active = false;
		//deactivateLClick();
		LRClickableAreaWTextComp::deactivate();
	}
}

void CArtPlace::show(SDL_Surface *to)
{
	if (ourArt)
		blitAt(graphics->artDefs->ourImages[ourArt->id].bitmap, pos.x, pos.y, to);

	if(marked && active)
	{
		// Draw vertical bars.
		for (int i = 0; i < pos.h; ++i) {
			CSDL_Ext::SDL_PutPixelWithoutRefresh(to, pos.x,             pos.y + i, 240, 220, 120);
			CSDL_Ext::SDL_PutPixelWithoutRefresh(to, pos.x + pos.w - 1, pos.y + i, 240, 220, 120);
		}

		// Draw horizontal bars.
		for (int i = 0; i < pos.w; ++i) {
			CSDL_Ext::SDL_PutPixelWithoutRefresh(to, pos.x + i, pos.y,             240, 220, 120);
			CSDL_Ext::SDL_PutPixelWithoutRefresh(to, pos.x + i, pos.y + pos.h - 1, 240, 220, 120);
		}
	}
}

bool CArtPlace::fitsHere(const CArtifact * art)
{
	// You can place 'no artifact' anywhere.
	if(!art)
		return true;

	// Anything can but War Machines can be placed in backpack.
	if (slotID >= 19) {
		return !CGI->arth->isBigArtifact(art->id);
	} else if (vstd::contains(art->possibleSlots, slotID)) {
		// TODO: Deal with combinational at dest and as src.
		return true;
	}

	return false;
}

CArtPlace::~CArtPlace()
{
	deactivate();
}

void HoverableArea::hover (bool on)
{
	if (on)
		LOCPLINT->statusbar->print(hoverText);
	else if (LOCPLINT->statusbar->getCurrent()==hoverText)
		LOCPLINT->statusbar->clear();
}

void HoverableArea::activate()
{
	activateHover();
}

void HoverableArea::deactivate()
{
	deactivateHover();
}

void LClickableArea::activate()
{
	activateLClick();
}
void LClickableArea::deactivate()
{
	deactivateLClick();
}
void LClickableArea::clickLeft(tribool down, bool previousState)
{
	//if(!down)
	//{
	//	LOCPLINT->showInfoDialog("TEST TEST AAA", std::vector<SComponent*>());
	//}
}

void RClickableArea::activate()
{
	activateRClick();
}
void RClickableArea::deactivate()
{
	deactivateRClick();
}
void RClickableArea::clickRight(tribool down, bool previousState)
{
	//if(!down)
	//{
	//	LOCPLINT->showInfoDialog("TEST TEST AAA", std::vector<SComponent*>());
	//}
}

void LRClickableAreaWText::clickLeft(tribool down, bool previousState)
{
	if(!down && previousState)
	{
		LOCPLINT->showInfoDialog(text, std::vector<SComponent*>(), soundBase::sound_todo);
	}
	//ClickableL::clickLeft(down);
}
void LRClickableAreaWText::clickRight(tribool down, bool previousState)
{
	LOCPLINT->adventureInt->handleRightClick(text, down, this);
}
void LRClickableAreaWText::activate()
{
	LClickableArea::activate();
	RClickableArea::activate();
	activateHover();
}
void LRClickableAreaWText::deactivate()
{
	LClickableArea::deactivate();
	RClickableArea::deactivate();
	deactivateHover();
}

void LClickableAreaHero::clickLeft(tribool down, bool previousState)
{
	if(!down)
	{
		owner->deactivate();
		const CGHeroInstance * buf = LOCPLINT->getWHero(id);
		owner->setHero(buf);
		owner->redrawCurBack();
		owner->activate();
	}
}

void LRClickableAreaWTextComp::clickLeft(tribool down, bool previousState)
{
	if((!down) && previousState)
	{
		std::vector<SComponent*> comp(1, new SComponent(SComponent::Etype(baseType), type, bonus));
		LOCPLINT->showInfoDialog(text, comp, soundBase::sound_todo);
	}
	//ClickableL::clickLeft(down);
}
void LRClickableAreaWTextComp::clickRight(tribool down, bool previousState)
{
	LOCPLINT->adventureInt->handleRightClick(text, down, this);
}
void LRClickableAreaWTextComp::activate()
{
	LClickableArea::activate();
	RClickableArea::activate();
	activateHover();
}
void LRClickableAreaWTextComp::deactivate()
{
	LClickableArea::deactivate();
	RClickableArea::deactivate();
	deactivateHover();
}

void LRClickableAreaOpenHero::clickLeft(tribool down, bool previousState)
{
	if((!down) && previousState && hero)
		LOCPLINT->openHeroWindow(hero);
}

void LRClickableAreaOpenHero::clickRight(tribool down, bool previousState)
{
	if((!down) && previousState && hero)
		LOCPLINT->openHeroWindow(hero);
}

void LRClickableAreaOpenTown::clickLeft(tribool down, bool previousState)
{
	if((!down) && previousState && town)
		LOCPLINT->openTownWindow(town);
}

void LRClickableAreaOpenTown::clickRight(tribool down, bool previousState)
{
	if((!down) && previousState && town)
		LOCPLINT->openTownWindow(town);
}

void CArtifactsOfHero::activate()
{
	for(size_t f=0; f<artWorn.size(); ++f)
	{
		if(artWorn[f])
			artWorn[f]->activate();
	}
	for(size_t f=0; f<backpack.size(); ++f)
	{
		if(backpack[f])
			backpack[f]->activate();
	}
	
	leftArtRoll->activate();
	rightArtRoll->activate();
}

void CArtifactsOfHero::deactivate()
{
	for(size_t f=0; f<artWorn.size(); ++f)
	{
		if(artWorn[f])
			artWorn[f]->deactivate();
	}
	for(size_t f=0; f<backpack.size(); ++f)
	{
		if(backpack[f])
			backpack[f]->deactivate();
	}
	
	leftArtRoll->deactivate();
	rightArtRoll->deactivate();
}

void CArtifactsOfHero::show(SDL_Surface * to)
{
	for(size_t d=0; d<artWorn.size(); ++d)
	{
		artWorn[d]->show(to);
	}
	for(size_t d=0; d<backpack.size(); ++d)
	{
		backpack[d]->show(to);
	}

	leftArtRoll->show(to);
	rightArtRoll->show(to);
}

void CArtifactsOfHero::setHero(const CGHeroInstance * hero)
{
	// An update is made, rather than initialization.
	if (curHero == hero) {
		// Compensate backpack pos if an artifact was insertad before it.
		if (commonInfo->destSlotID >= 19 && commonInfo->destAOH == this
			&& commonInfo->destSlotID - 19 < backpackPos)
		{
			backpackPos++;
		}

		// A swap was made, make the replaced artifact into current selected.
		if (commonInfo->destSlotID < 19 && commonInfo->destArtifact) {
			// Source <- Dest
			commonInfo->srcAOH = commonInfo->destAOH;
			commonInfo->srcArtifact = commonInfo->destArtifact;
			commonInfo->srcSlotID = -1; // The artifact's original place is taken now.

			// Reset destination parameters.
			commonInfo->destAOH = NULL;
			commonInfo->destArtifact = NULL;
			commonInfo->destSlotID = -1;

			CGI->curh->dragAndDropCursor(
				graphics->artDefs->ourImages[commonInfo->srcArtifact->id].bitmap);
			markPossibleSlots(commonInfo->srcArtifact);
		} else if (commonInfo->destAOH != NULL) {
			// Reset all parameters.
			commonInfo->srcAOH = NULL;
			commonInfo->srcArtifact = NULL;
			commonInfo->srcSlotID = -1;
			commonInfo->destAOH = NULL;
			commonInfo->destArtifact = NULL;
			commonInfo->destSlotID = -1;

			CGI->curh->dragAndDropCursor(NULL);
			unmarkSlots();
		}
	} else {
		rollback();
	}

	curHero = hero;

	if (curHero->artifacts.size() > 0)
		backpackPos %= curHero->artifacts.size();
	else
		backpackPos = 0;

	// Fill the slots for worn artifacts and backpack.
	for (int g = 0; g < 19 ; g++)
		setSlotData(artWorn[g], g);
	scrollBackpack(0);

	//blocking scrolling if there is not enough artifacts to scroll
	leftArtRoll->block(curHero->artifacts.size() <= backpack.size());
	rightArtRoll->block(curHero->artifacts.size() <= backpack.size());
}

/**
 * Any held artifacts, marked slots etc. will be restored to it's original way.
 */
void CArtifactsOfHero::rollback()
{
	if (curHero != NULL) {
		// Restore any held artifact to it's original position.
		if (commonInfo->destArtifact && commonInfo->destAOH == this) {
			// For an unlikely race condition scenario, put swapped artifact into backpack.
			LOCPLINT->cb->setArtifact(
				curHero,
				19 + curHero->artifacts.size(),
				commonInfo->destArtifact->id);
		}
		else if (commonInfo->srcArtifact && commonInfo->srcAOH == this) {
			if (commonInfo->srcSlotID != -1) { // Held artifact, just put it back to it's spot.
				LOCPLINT->cb->setArtifact(
					curHero,
					commonInfo->srcSlotID,
					commonInfo->srcArtifact->id);
			} else { // Swapped artifact.
				// Wear the artifact in a suitable spot.
				ui16 i = 0;
				for (; i < 19; i++) {
					if (artWorn[i]->fitsHere(commonInfo->srcArtifact)
						&& curHero->artifWorn.find(i) == curHero->artifWorn.end())
					{
						LOCPLINT->cb->setArtifact(
							curHero, i, commonInfo->srcArtifact->id);
						break;
					}
				}

				// If it can't be worn, put it in the backpack.
				if (i == 19)
					LOCPLINT->cb->setArtifact(
						curHero,
						19 + curHero->artifacts.size(),
						commonInfo->srcArtifact->id);
			}
		}
	}
		unmarkSlots();
	backpackPos = 0;

	commonInfo->srcAOH = NULL;
	commonInfo->srcArtifact = NULL;
	commonInfo->srcSlotID = -1;
	commonInfo->destAOH = NULL;
	commonInfo->destArtifact = NULL;
	commonInfo->destSlotID = -1;

	CGI->curh->dragAndDropCursor(NULL);
}

void CArtifactsOfHero::dispose()
{
	curHero = NULL;
}

void CArtifactsOfHero::scrollBackpack(int dir)
{
	backpackPos += dir;
	if (curHero->artifacts.size() > 0) {
		if (backpackPos < 0) { // No guarantee of modulus behavior with negative operands.
			do {
				backpackPos += curHero->artifacts.size();
			} while (backpackPos < 0);
		} else {
			backpackPos %= curHero->artifacts.size();
		}
	}

	//set new data
	for (size_t s = 0; s < backpack.size(); ++s) {
		if (s < curHero->artifacts.size())
			setSlotData(backpack[s], 19 + (s + backpackPos)%curHero->artifacts.size());
		else
			eraseSlotData(backpack[s], 19 + s);
	}
}

/**
 * Marks possible slots where a given artifact can be placed, except backpack.
 *
 * @param art Artifact checked against.
 */
void CArtifactsOfHero::markPossibleSlots (const CArtifact* art)
{
	for (std::set<CArtifactsOfHero *>::iterator it = commonInfo->participants.begin();
		it != commonInfo->participants.end();
		++it)
	{
		for (int i = 0; i < (*it)->artWorn.size(); i++) {
			if ((*it)->artWorn[i]->fitsHere(art))
				(*it)->artWorn[i]->marked = true;
		}
	}
}

/**
 * Unamarks all slots.
 */
void CArtifactsOfHero::unmarkSlots ()
{
	for (std::set<CArtifactsOfHero *>::iterator it = commonInfo->participants.begin();
		it != commonInfo->participants.end();
		++it)
	{
		for (int i = 0; i < (*it)->artWorn.size(); i++) {
			(*it)->artWorn[i]->marked = false;
		}
	}
}

/**
 * Assigns an artifacts to an artifact place depending on it's new slot ID.
 */
void CArtifactsOfHero::setSlotData (CArtPlace* artPlace, int slotID)
{
	artPlace->slotID = slotID;
	artPlace->ourArt = curHero->getArt(slotID);

	if (artPlace->ourArt) {
		artPlace->text = artPlace->ourArt->Description();
		artPlace->hoverText = boost::str(boost::format(CGI->generaltexth->heroscrn[1].c_str()) % artPlace->ourArt->Name().c_str());
	} else {
		eraseSlotData(artPlace, slotID);
	}
}

/**
 * Makes given artifact slot appear as empty with a certain slot ID.
 */
void CArtifactsOfHero::eraseSlotData (CArtPlace* artPlace, int slotID)
{
	artPlace->slotID = slotID;
	artPlace->ourArt = NULL;
	artPlace->text = std::string();
	artPlace->hoverText = CGI->generaltexth->allTexts[507];
}

CArtifactsOfHero::CArtifactsOfHero(const SDL_Rect & position) :
	backpackPos(0)
{
	pos = position;
	artWorn.resize(19);
	
	std::vector<SDL_Rect> slotPos;
	slotPos += genRect(44,44,pos.x+509,pos.y+30), genRect(44,44,pos.x+567,pos.y+240), genRect(44,44,pos.x+509,pos.y+80), 
		genRect(44,44,pos.x+383,pos.y+68), genRect(44,44,pos.x+564,pos.y+183), genRect(44,44,pos.x+509,pos.y+130), 
		genRect(44,44,pos.x+431,pos.y+68), genRect(44,44,pos.x+610,pos.y+183), genRect(44,44,pos.x+515,pos.y+295), 
		genRect(44,44,pos.x+383,pos.y+143), genRect(44,44,pos.x+399,pos.y+194), genRect(44,44,pos.x+415,pos.y+245),
		genRect(44,44,pos.x+431,pos.y+296), genRect(44,44,pos.x+564,pos.y+30), genRect(44,44,pos.x+610,pos.y+30), 
		genRect(44,44,pos.x+610,pos.y+76), genRect(44,44,pos.x+610,pos.y+122), genRect(44,44,pos.x+610,pos.y+310),	
		genRect(44,44,pos.x+381,pos.y+296);

	// Create slots for worn artifacts.
	for (int g = 0; g < 19 ; g++)
	{	
		artWorn[g] = new CArtPlace(NULL);
		artWorn[g]->pos = slotPos[g];
		artWorn[g]->ourOwner = this;
		eraseSlotData(artWorn[g], g);
	}

	// Create slots for the backpack.
	for(size_t s=0; s<5; ++s)
	{
		CArtPlace * add = new CArtPlace(NULL);

		add->ourOwner = this;
		add->pos.x = pos.x + 403 + 46*s;
		add->pos.y = pos.y + 365;
		add->pos.h = add->pos.w = 44;
		eraseSlotData(add, 19 + s);

		backpack.push_back(add);
	}

	leftArtRoll = new AdventureMapButton(std::string(), std::string(), boost::bind(&CArtifactsOfHero::scrollBackpack,this,-1), pos.x+379, pos.y+364, "hsbtns3.def", SDLK_LEFT);
	rightArtRoll = new AdventureMapButton(std::string(), std::string(), boost::bind(&CArtifactsOfHero::scrollBackpack,this,+1), pos.x+632, pos.y+364, "hsbtns5.def", SDLK_RIGHT);
}

CArtifactsOfHero::~CArtifactsOfHero()
{
	dispose();
	for(size_t g=0; g<artWorn.size(); ++g)
	{
		delete artWorn[g];
		artWorn[g] = NULL;
	}
	for(size_t g=0; g<backpack.size(); ++g)
	{
		delete backpack[g];
		backpack[g] = NULL;
	}
	backpack.clear();
	artWorn.clear();

	delete leftArtRoll;
	delete rightArtRoll;
}

void CExchangeWindow::close()
{
	GH.popIntTotally(this);
}

void CExchangeWindow::activate()
{
	quit->activate();
	garr->activate();

	artifs[0]->setHero(heroInst[0]);
	artifs[0]->activate();
	artifs[1]->setHero(heroInst[1]);
	artifs[1]->activate();

	for(int g=0; g<ARRAY_COUNT(secSkillAreas); g++)
	{
		for(int b=0; b<secSkillAreas[g].size(); ++b)
		{
			secSkillAreas[g][b]->activate();
		}
	}

	for(int b=0; b<primSkillAreas.size(); ++b)
		primSkillAreas[b]->activate();

	LOCPLINT->statusbar = ourBar;

	for(int g=0; g<ARRAY_COUNT(questlogButton); g++)
		questlogButton[g]->activate();

	for(int g=0; g<ARRAY_COUNT(morale); g++)
		morale[g]->activate();

	for(int g=0; g<ARRAY_COUNT(luck); g++)
		luck[g]->activate();

	for(int g=0; g<ARRAY_COUNT(portrait); g++)
		portrait[g]->activate();

	for(int g=0; g<ARRAY_COUNT(spellPoints); g++)
		spellPoints[g]->activate();

	for(int g=0; g<ARRAY_COUNT(experience); g++)
		experience[g]->activate();

	for(int g=0; g<ARRAY_COUNT(speciality); g++)
		speciality[g]->activate();
}

void CExchangeWindow::deactivate()
{
	quit->deactivate();
	garr->deactivate();

	artifs[0]->deactivate();
	artifs[1]->deactivate();

	for(int g=0; g<ARRAY_COUNT(secSkillAreas); g++)
	{
		for(int b=0; b<secSkillAreas[g].size(); ++b)
		{
			secSkillAreas[g][b]->deactivate();
		}
	}

	for(int b=0; b<primSkillAreas.size(); ++b)
		primSkillAreas[b]->deactivate();

	for(int g=0; g<ARRAY_COUNT(questlogButton); g++)
		questlogButton[g]->deactivate();

	for(int g=0; g<ARRAY_COUNT(morale); g++)
		morale[g]->deactivate();

	for(int g=0; g<ARRAY_COUNT(luck); g++)
		luck[g]->deactivate();

	for(int g=0; g<ARRAY_COUNT(portrait); g++)
		portrait[g]->deactivate();

	for(int g=0; g<ARRAY_COUNT(spellPoints); g++)
		spellPoints[g]->deactivate();

	for(int g=0; g<ARRAY_COUNT(experience); g++)
		experience[g]->deactivate();

	for(int g=0; g<ARRAY_COUNT(speciality); g++)
		speciality[g]->deactivate();
}

void CExchangeWindow::show(SDL_Surface * to)
{
	blitAt(bg, pos, to);

	quit->show(to);

	//printing border around window
	if(screen->w != 800 || screen->h !=600)
		CMessage::drawBorder(LOCPLINT->playerID,to,828,628,pos.x-14,pos.y-15);

	artifs[0]->show(to);
	artifs[1]->show(to);

	ourBar->show(to);

	for(int g=0; g<ARRAY_COUNT(secSkillAreas); g++)
	{
		questlogButton[g]->show(to);
	}

	garr->show(to);
}

void CExchangeWindow::questlog(int whichHero)
{
	CGI->curh->dragAndDropCursor(NULL);
}

void CExchangeWindow::prepareBackground()
{
	if(bg)
		SDL_FreeSurface(bg);

	SDL_Surface * bgtemp; //loaded as 8bpp surface
	bgtemp = BitmapHandler::loadBitmap("TRADE2.BMP");
	graphics->blueToPlayersAdv(bgtemp, heroInst[0]->tempOwner);
	bg = SDL_ConvertSurface(bgtemp, screen->format, screen->flags); //to 24 bpp
	SDL_FreeSurface(bgtemp);

	//printing heroes' names and levels
	std::ostringstream os, os2;
	os<<heroInst[0]->name<<", Level "<<heroInst[0]->level<<" "<<heroInst[0]->type->heroClass->name;
	CSDL_Ext::printAtMiddle(os.str(), 147, 25, FONT_SMALL, zwykly, bg);
	os2<<heroInst[1]->name<<", Level "<<heroInst[1]->level<<" "<<heroInst[1]->type->heroClass->name;
	CSDL_Ext::printAtMiddle(os2.str(), 653, 25, FONT_SMALL, zwykly, bg);

	//printing primary skills
	CDefHandler * skilldef = CDefHandler::giveDef("PSKIL32.DEF");
	for(int g=0; g<4; ++g)
	{
		//graphics
		blitAt(skilldef->ourImages[g].bitmap, genRect(32, 32, 385, 19 + 36 * g), bg);
	}

	CDefHandler * un32 = CDefHandler::giveDef("UN32.DEF");
	//heroes related thing
	for(int b=0; b<ARRAY_COUNT(heroInst); b++)
	{
		//printing primary skills' amounts
		for(int m=0; m<4; ++m)
		{
			std::ostringstream primarySkill;
			primarySkill<<heroInst[b]->getPrimSkillLevel(m);
			CSDL_Ext::printAtMiddle(primarySkill.str(), 352 + 93 * b, 35 + 36 * m, FONT_SMALL, zwykly, bg);
		}

		//printing secondary skills
		for(int m=0; m<heroInst[b]->secSkills.size(); ++m)
		{
			blitAt(graphics->abils32->ourImages[heroInst[b]->secSkills[m].first * 3 + heroInst[b]->secSkills[m].second + 2].bitmap, genRect(32, 32, 32 + 36 * m + 454 * b, 88), bg);
		}

		//hero's specialty
		blitAt(un32->ourImages[heroInst[b]->subID].bitmap, 67 + 490*b, 45, bg);

		//experience
		blitAt(skilldef->ourImages[4].bitmap, 103 + 490*b, 45, bg);
		printAtMiddle( makeNumberShort(heroInst[b]->exp), 119 + 490*b, 71, FONT_SMALL, zwykly, bg );

		//mana points
		blitAt(skilldef->ourImages[5].bitmap, 139 + 490*b, 45, bg);
		printAtMiddle( makeNumberShort(heroInst[b]->mana), 155 + 490*b, 71, FONT_SMALL, zwykly, bg );

		//setting morale
		blitAt(graphics->morale30->ourImages[heroInst[b]->getCurrentMorale()+3].bitmap, 177 + 490*b, 45, bg);

		//setting luck
		blitAt(graphics->luck30->ourImages[heroInst[b]->getCurrentLuck()+3].bitmap, 213 + 490*b, 45, bg);
	}

	//printing portraits
	blitAt(graphics->portraitLarge[heroInst[0]->portrait], 257, 13, bg);
	blitAt(graphics->portraitLarge[heroInst[1]->portrait], 485, 13, bg);

	delete un32;
	delete skilldef;
}

CExchangeWindow::CExchangeWindow(si32 hero1, si32 hero2) : bg(NULL)
{
	char bufor[400];
	heroInst[0] = LOCPLINT->cb->getHeroInfo(hero1, 2);
	heroInst[1] = LOCPLINT->cb->getHeroInfo(hero2, 2);

	prepareBackground();
	pos.x = screen->w/2 - bg->w/2;
	pos.y = screen->h/2 - bg->h/2;
	pos.w = screen->w;
	pos.h = screen->h;


	artifs[0] = new CArtifactsOfHero(genRect(600, 800, pos.x + -334, pos.y + 150));
	artifs[0]->commonInfo = new CArtifactsOfHero::SCommonPart;
	artifs[0]->commonInfo->participants.insert(artifs[0]);
	artifs[0]->setHero(heroInst[0]);
	artifs[1] = new CArtifactsOfHero(genRect(600, 800, pos.x + 96, pos.y + 150));
	artifs[1]->commonInfo = artifs[0]->commonInfo;
	artifs[1]->commonInfo->participants.insert(artifs[1]);
	artifs[1]->setHero(heroInst[1]);


	//primary skills
	for(int g=0; g<4; ++g)
	{
		//primary skill's clickable areas
		primSkillAreas.push_back(new LRClickableAreaWTextComp());
		primSkillAreas[g]->pos = genRect(32, 140, pos.x+329, pos.y + 19 + 36 * g);
		primSkillAreas[g]->text = CGI->generaltexth->arraytxt[2+g];
		primSkillAreas[g]->type = g;
		primSkillAreas[g]->bonus = -1;
		primSkillAreas[g]->baseType = 0;
		sprintf(bufor, CGI->generaltexth->heroscrn[1].c_str(), CGI->generaltexth->primarySkillNames[g].c_str());
		primSkillAreas[g]->hoverText = std::string(bufor);
	}

	//heroes related thing
	for(int b=0; b<ARRAY_COUNT(heroInst); b++)
	{
		//secondary skill's clickable areas
		for(int g=0; g<heroInst[b]->secSkills.size(); ++g)
		{
			int skill = heroInst[b]->secSkills[g].first,
				level = heroInst[b]->secSkills[g].second; // <1, 3>
			secSkillAreas[b].push_back(new LRClickableAreaWTextComp());
			secSkillAreas[b][g]->pos = genRect(32, 32, pos.x + 32 + g*36 + b*454 , pos.y + 88);
			secSkillAreas[b][g]->baseType = 1;

			secSkillAreas[b][g]->type = skill;
			secSkillAreas[b][g]->bonus = level;
			secSkillAreas[b][g]->text = CGI->generaltexth->skillInfoTexts[skill][level-1];

			sprintf(bufor, CGI->generaltexth->heroscrn[21].c_str(), CGI->generaltexth->levels[level - 1].c_str(), CGI->generaltexth->skillName[skill].c_str());
			secSkillAreas[b][g]->hoverText = std::string(bufor);
		}

		portrait[b] = new LRClickableAreaOpenHero();
		portrait[b]->pos = genRect(64, 58, pos.x + 257 + 228*b, pos.y + 13);
		portrait[b]->hero = heroInst[b];
		sprintf(bufor, CGI->generaltexth->allTexts[15].c_str(), heroInst[b]->name.c_str(), heroInst[b]->type->heroClass->name.c_str());
		portrait[b]->hoverText = std::string(bufor);

		speciality[b] = new LRClickableAreaWText();
		speciality[b]->pos = genRect(32, 32, pos.x + 69 + 490*b, pos.y + 45);
		speciality[b]->hoverText = CGI->generaltexth->heroscrn[27];
		speciality[b]->text = CGI->generaltexth->hTxts[heroInst[b]->subID].longBonus;

		experience[b] = new LRClickableAreaWText();
		experience[b]->pos = genRect(32, 32, pos.x + 105 + 490*b, pos.y + 45);
		experience[b]->hoverText = CGI->generaltexth->heroscrn[9];
		experience[b]->text = CGI->generaltexth->allTexts[2].c_str();
		boost::replace_first(experience[b]->text, "%d", boost::lexical_cast<std::string>(heroInst[b]->level));
		boost::replace_first(experience[b]->text, "%d", boost::lexical_cast<std::string>(CGI->heroh->reqExp(heroInst[b]->level+1)));
		boost::replace_first(experience[b]->text, "%d", boost::lexical_cast<std::string>(heroInst[b]->exp));

		spellPoints[b] = new LRClickableAreaWText();
		spellPoints[b]->pos = genRect(32, 32, pos.x + 141 + 490*b, pos.y + 45);
		spellPoints[b]->hoverText = CGI->generaltexth->heroscrn[22];
		sprintf(bufor, CGI->generaltexth->allTexts[205].c_str(), heroInst[b]->name.c_str(), heroInst[b]->mana, heroInst[b]->manaLimit());
		spellPoints[b]->text = std::string(bufor);

		//setting morale
		morale[b] = new LRClickableAreaWTextComp();
		morale[b]->pos = genRect(32, 32, pos.x + 177 + 490*b, pos.y + 45);

		std::vector<std::pair<int,std::string> > mrl = heroInst[b]->getCurrentMoraleModifiers();
		int mrlv = heroInst[b]->getCurrentMorale();
		int mrlt = (mrlv>0)-(mrlv<0); //signum: -1 - bad morale[b], 0 - neutral, 1 - good
		morale[b]->hoverText = CGI->generaltexth->heroscrn[4 - mrlt];
		morale[b]->baseType = SComponent::morale;
		morale[b]->bonus = mrlv;
		morale[b]->text = CGI->generaltexth->arraytxt[88];
		boost::algorithm::replace_first(morale[b]->text,"%s",CGI->generaltexth->arraytxt[86-mrlt]);
		if (!mrl.size())
			morale[b]->text += CGI->generaltexth->arraytxt[108];
		else
			for(int it=0; it < mrl.size(); it++)
				morale[b]->text += mrl[it].second;

		//setting luck
		luck[b] = new LRClickableAreaWTextComp();
		luck[b]->pos = genRect(32, 32, pos.x + 213 + 490*b, pos.y + 45);

		mrl = heroInst[b]->getCurrentLuckModifiers();
		mrlv = heroInst[b]->getCurrentLuck();
		mrlt = (mrlv>0)-(mrlv<0); //signum: -1 - bad luck[b], 0 - neutral, 1 - good
		luck[b]->hoverText = CGI->generaltexth->heroscrn[7 - mrlt];
		luck[b]->baseType = SComponent::luck;
		luck[b]->bonus = mrlv;
		luck[b]->text = CGI->generaltexth->arraytxt[62];
		boost::algorithm::replace_first(luck[b]->text,"%s",CGI->generaltexth->arraytxt[60-mrlt]);
		if (!mrl.size())
			luck[b]->text += CGI->generaltexth->arraytxt[77];
		else
			for(int it=0; it < mrl.size(); it++)
				luck[b]->text += mrl[it].second;
	}

	//buttons
	quit = new AdventureMapButton(CGI->generaltexth->tcommands[8], "", boost::bind(&CExchangeWindow::close, this), pos.x+732, pos.y+567, "IOKAY.DEF", SDLK_RETURN);
	questlogButton[0] = new AdventureMapButton(CGI->generaltexth->heroscrn[0], std::string(), boost::bind(&CExchangeWindow::questlog,this, 0), pos.x+10, pos.y+44, "hsbtns4.def");
	questlogButton[1] = new AdventureMapButton(CGI->generaltexth->heroscrn[0], std::string(), boost::bind(&CExchangeWindow::questlog,this, 1), pos.x+740, pos.y+44, "hsbtns4.def");

	//statusbar
	ourBar = new CStatusBar(pos.x + 3, pos.y + 577, "TSTATBAR.bmp", 726);

	//garrison interface
	garr = new CGarrisonInt(pos.x + 69, pos.y + 131, 4, Point(418,0), bg, Point(69,131), heroInst[0],heroInst[1], true, true);

	garr->splitButtons.push_back(new AdventureMapButton(CGI->generaltexth->tcommands[3],"",boost::bind(&CGarrisonInt::splitClick,garr),pos.x+10,pos.y+132,"TSBTNS.DEF"));
	garr->splitButtons.push_back(new AdventureMapButton(CGI->generaltexth->tcommands[3],"",boost::bind(&CGarrisonInt::splitClick,garr),pos.x+740,pos.y+132,"TSBTNS.DEF"));
}

CExchangeWindow::~CExchangeWindow() //d-tor
{
	SDL_FreeSurface(bg);
	delete quit;

	//warning: don't experiment with these =NULL lines, they prevent heap corruption!
	artifs[0]->rollback();
	artifs[1]->rollback();
	delete artifs[0]->commonInfo;
	artifs[0]->commonInfo = NULL;
	delete artifs[0];
	artifs[1]->commonInfo = NULL;
	delete artifs[1];

	delete garr;
	delete ourBar;

	for(int g=0; g<ARRAY_COUNT(secSkillAreas); g++)
	{
		for(int b=0; b<secSkillAreas[g].size(); ++b)
		{
			delete secSkillAreas[g][b];
		}
	}

	for(int b=0; b<primSkillAreas.size(); ++b)
	{
		delete primSkillAreas[b];
	}

	
	for(int g=0; g<ARRAY_COUNT(questlogButton); g++)
	{
		delete questlogButton[g];
	}

	for(int g=0; g<ARRAY_COUNT(morale); g++)
		delete morale[g];

	for(int g=0; g<ARRAY_COUNT(luck); g++)
		delete luck[g];

	for(int g=0; g<ARRAY_COUNT(portrait); g++)
		delete portrait[g];

	for(int g=0; g<ARRAY_COUNT(spellPoints); g++)
		delete spellPoints[g];

	for(int g=0; g<ARRAY_COUNT(experience); g++)
		delete experience[g];

	for(int g=0; g<ARRAY_COUNT(speciality); g++)
		delete speciality[g];
}

void CShipyardWindow::activate()
{
	build->activate();
	quit->activate();
}

void CShipyardWindow::deactivate()
{
	build->deactivate();
	quit->deactivate();
}

void CShipyardWindow::show( SDL_Surface * to )
{
	blitAt(bg,pos,to);
	CSDL_Ext::blit8bppAlphaTo24bpp(graphics->boatAnims[1]->ourImages[21 + frame++/8%8].bitmap, NULL, to, &genRect(64, 96, pos.x+110, pos.y+85));
	build->show(to);
	quit->show(to);
}

CShipyardWindow::~CShipyardWindow()
{
	delete build;
	delete quit;
}

CShipyardWindow::CShipyardWindow(const std::vector<si32> &cost, int state, const boost::function<void()> &onBuy)
{
	frame = 0;
	SDL_Surface * bgtemp; //loaded as 8bpp surface
	bgtemp = BitmapHandler::loadBitmap("TPSHIP.bmp");
	pos.x = screen->w/2 - bgtemp->w/2;
	pos.y = screen->h/2 - bgtemp->h/2;
	pos.w = bgtemp->w;
	pos.h = bgtemp->h;
	SDL_SetColorKey(bgtemp,SDL_SRCCOLORKEY,SDL_MapRGB(bgtemp->format,0,255,255));
	graphics->blueToPlayersAdv(bgtemp, LOCPLINT->playerID);
	bg = SDL_ConvertSurface(bgtemp, screen->format, screen->flags); //to 24 bpp
	SDL_FreeSurface(bgtemp);

	bgtemp = BitmapHandler::loadBitmap("TPSHIPBK.bmp");
	blitAt(bgtemp, 100, 69, bg);
	SDL_FreeSurface(bgtemp);

	// Draw resource icons and costs.
	std::string goldCost = boost::lexical_cast<std::string>(1000);
	std::string woodCost = boost::lexical_cast<std::string>(10);
	blitAt(graphics->resources32->ourImages[6].bitmap, 100, 244, bg);
	printAtMiddle(goldCost.c_str(), 118, 294, FONT_SMALL, zwykly, bg);
	blitAt(graphics->resources32->ourImages[0].bitmap, 196, 244, bg);
	printAtMiddle(woodCost.c_str(), 212, 294, FONT_SMALL, zwykly, bg);

	bool affordable = true;
	for(int i = 0; i < cost.size(); i++)
	{
		if(cost[i] > LOCPLINT->cb->getResourceAmount(i))
		{
			affordable = false;
			break;
		}
	}

	quit = new AdventureMapButton(CGI->generaltexth->allTexts[599], "", boost::bind(&CGuiHandler::popIntTotally, &GH, this), pos.x+224, pos.y+312, "ICANCEL.DEF", SDLK_RETURN);
	build = new AdventureMapButton(CGI->generaltexth->allTexts[598], "", boost::bind(&CGuiHandler::popIntTotally, &GH, this), pos.x+42, pos.y+312, "IBY6432.DEF", SDLK_RETURN);
	build->callback += onBuy;

	if(!affordable)
		build->block(true);

	printAtMiddle(CGI->generaltexth->jktexts[13], 164, 27, FONT_BIG, tytulowy, bg);  //Build A New Ship
	printAtMiddle(CGI->generaltexth->jktexts[14], 164, 220, FONT_MEDIUM, zwykly, bg); //Resource cost:
}

CPuzzleWindow::CPuzzleWindow()
:animCount(0)
{
	SDL_Surface * back = BitmapHandler::loadBitmap("PUZZLE.BMP", false);
	graphics->blueToPlayersAdv(back, LOCPLINT->playerID);
	//make transparency black
	back->format->palette->colors[0].b = back->format->palette->colors[0].r = back->format->palette->colors[0].g = 0;
	//the rest
	background = SDL_ConvertSurface(back, screen->format, back->flags);
	SDL_FreeSurface(back);
	pos = genRect(background->h, background->w, (conf.cc.resx - background->w) / 2, (conf.cc.resy - background->h) / 2);
	quitb = new AdventureMapButton(CGI->generaltexth->allTexts[599], "", boost::bind(&CGuiHandler::popIntTotally, &GH, this), pos.x+670, pos.y+538, "IOK6432.DEF", SDLK_RETURN);
	resdatabar = new CResDataBar("ZRESBAR.bmp", pos.x+3, pos.y+575, 32, 2, 85, 85);
	resdatabar->pos.x = pos.x+3; resdatabar->pos.y = pos.y+575;

	//printing necessary thinks to background
	
	CGI->mh->terrainRect
		(int3(14, 15, 0), LOCPLINT->adventureInt->anim,
		 &LOCPLINT->cb->getVisibilityMap(), true, LOCPLINT->adventureInt->heroAnim,
		 background, &genRect(544, 591, 8, 8), 0, 0, true);

	
	float discoveryRatio = 0.5f;
	int faction = LOCPLINT->cb->getStartInfo()->playerInfos[LOCPLINT->serialID].castle;

	std::vector<SPuzzleInfo> puzzlesToPrint;

	for(int g=0; g<PUZZLES_PER_FACTION; ++g)
	{
		if(CGI->heroh->puzzleInfo[faction][g].whenUncovered >= PUZZLES_PER_FACTION * discoveryRatio)
		{
			puzzlesToPrint.push_back(CGI->heroh->puzzleInfo[faction][g]);
		}
		else
		{
			SDL_Surface *buf = BitmapHandler::loadBitmap(CGI->heroh->puzzleInfo[faction][g].filename);
			puzzlesToPullBack.push_back( std::make_pair(buf, &CGI->heroh->puzzleInfo[faction][g]) );
		}
	}

	for(int b = 0; b < puzzlesToPrint.size(); ++b)
	{
		SDL_Surface * puzzle = BitmapHandler::loadBitmap(puzzlesToPrint[b].filename);

		blitAt(puzzle, puzzlesToPrint[b].x, puzzlesToPrint[b].y, background);

		SDL_FreeSurface(puzzle);
	}

}

CPuzzleWindow::~CPuzzleWindow()
{
	delete quitb;
	delete resdatabar;
	SDL_FreeSurface(background);
	for(int g = 0; g < puzzlesToPullBack.size(); ++g)
		SDL_FreeSurface( puzzlesToPullBack[g].first );
}

void CPuzzleWindow::activate()
{
	quitb->activate();
}

void CPuzzleWindow::deactivate()
{
	quitb->deactivate();
}

void CPuzzleWindow::show(SDL_Surface * to)
{
	blitAt(background, pos.x, pos.y, to);
	quitb->show(to);
	resdatabar->draw(to);

	//blitting disappearing puzzles
	for(int g=0; g<2; ++g)
		if(animCount != 255)
			++animCount;

	if(animCount != 255)
	{
		for(int b = 0; b < puzzlesToPullBack.size(); ++b)
		{
			int xPos = puzzlesToPullBack[b].second->x + pos.x,
				yPos = puzzlesToPullBack[b].second->y + pos.y;
			SDL_Surface *from = puzzlesToPullBack[b].first;

			SDL_SetAlpha(from, SDL_SRCALPHA, 255 - animCount);
			blitAt(from, xPos, yPos, to);
		}
	}
	//disappearing puzzles blitted

	//printing border around window
	if(screen->w != 800 || screen->h !=600)
		CMessage::drawBorder(LOCPLINT->playerID,to,828,628,pos.x-14,pos.y-15);
}

bool CShopWindow::swapItem (ui16 which, bool choose)
{
	bool itemFound = false;
	if (choose == true) //choose item
	{
		if (chosen.count(which))
		{
			itemFound = true;
			chosen[which] = available[which];
			available.erase(which);
		}
	}
	else //return item to available list
	{
		if (available.count(which))
		{
			itemFound = true;
			available[which] = chosen[which];
			chosen.erase(which);
		}
	}
	return itemFound;
}

CArtMerchantWindow::CArtMerchantWindow()
{
}
CArtMerchantWindow::~CArtMerchantWindow(){}
void CArtMerchantWindow::activate() {}
void CArtMerchantWindow::deactivate() {}
void CArtMerchantWindow::show(SDL_Surface * to) {}
void CArtMerchantWindow::Buy() {}


void CThievesGuildWindow::activate()
{
	statusBar->activate();
	exitb->activate();
	resdatabar->activate();

	LOCPLINT->statusbar = statusBar;
}

void CThievesGuildWindow::deactivate()
{
	statusBar->deactivate();
	exitb->deactivate();
	resdatabar->deactivate();
}

void CThievesGuildWindow::show(SDL_Surface * to)
{
	blitAt(background, pos.x, pos.y, to);

	statusBar->show(to);
	exitb->show(to);
	resdatabar->show(to);

	//showing border around window
	if(screen->w != 800 || screen->h !=600)
	{
		CMessage::drawBorder(LOCPLINT->playerID, to, pos.w + 28, pos.h + 28, pos.x-14, pos.y-15);
	}
}

void CThievesGuildWindow::bexitf()
{
	GH.popIntTotally(this);
}

CThievesGuildWindow::CThievesGuildWindow(const CGObjectInstance * _owner)
	:owner(_owner)
{
	SThievesGuildInfo tgi; //info to be displayed
	LOCPLINT->cb->getThievesGuildInfo(tgi, owner);

	pos = Rect( (conf.cc.resx - 800) / 2, (conf.cc.resy - 600) / 2, 800, 600 );

	//loading backround and converting to more bpp form
	SDL_Surface * bg = background = BitmapHandler::loadBitmap("TpRank.bmp", false);
	background = newSurface(bg->w, bg->h);
	blitAt(bg, 0, 0, background);
	SDL_FreeSurface(bg);

	exitb = new AdventureMapButton (std::string(), std::string(), boost::bind(&CThievesGuildWindow::bexitf,this), 748 + pos.x, 556 + pos.y, "HSBTNS.def", SDLK_RETURN);
	statusBar = new CStatusBar(pos.x + 3, pos.y + 555, "TStatBar.bmp", 742);

	resdatabar = new CResDataBar("ZRESBAR.bmp", pos.x+3, pos.y+575, 32, 2, 85, 85);

	//printing texts & descriptions to background

	for(int g=0; g<12; ++g)
	{
		int y;
		if(g == 9) //best hero
		{
			y = 400;
		}
		else if(g == 10) //personality
		{
			y = 460;
		}
		else if(g == 11) //best monster
		{
			y = 510;
		}
		else
		{
			y = 52 + 32*g;
		}
		printAtMiddle(CGI->generaltexth->jktexts[24+g], 135, y, FONT_MEDIUM, tytulowy, background);
	}

	CDefHandler * strips = CDefHandler::giveDef("PRSTRIPS.DEF");

	static const std::string colorToBox[] = {"PRRED.BMP", "PRBLUE.BMP", "PRTAN.BMP", "PRGREEN.BMP", "PRORANGE.BMP", "PRPURPLE.BMP", "PRTEAL.BMP", "PRPINK.BMP"};

	for(int g=0; g<tgi.playerColors.size(); ++g)
	{
		if(g > 0)
		{
			blitAt(strips->ourImages[g-1].bitmap, 250 + 66*g, 7, background);
		}
		printAtMiddle(CGI->generaltexth->jktexts[16+g], 283 + 66*g, 20, FONT_MEDIUM, tytulowy, background);
		SDL_Surface * box = BitmapHandler::loadBitmap(colorToBox[tgi.playerColors[g]]);
		blitAt(box, 253 + 66*g, 334, background);
		SDL_FreeSurface(box);
	}

	delete strips;

}

CThievesGuildWindow::~CThievesGuildWindow()
{
	SDL_FreeSurface(background);
	delete exitb;
	delete statusBar;
	delete resdatabar;
}





