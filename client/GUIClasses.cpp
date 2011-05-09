#include "GUIClasses.h"
#include "SDL_Extensions.h"

#include "../stdafx.h"
#include "CAdvmapInterface.h"
#include "CBattleInterface.h"
#include "../CCallback.h"
#include "CCastleInterface.h"
#include "CCreatureWindow.h"
#include "CCursorHandler.h"
#include "CGameInfo.h"
#include "CHeroWindow.h"
#include "CMessage.h"
#include "CConfigHandler.h"
#include "SDL_framerate.h"
#include "CConfigHandler.h"
#include "CCreatureAnimation.h"
#include "CPlayerInterface.h"
#include "Graphics.h"
#include "CAnimation.h"
#include "../lib/CArtHandler.h"
#include "../lib/CBuildingHandler.h"
#include "../lib/CGeneralTextHandler.h"
#include "../lib/CHeroHandler.h"
#include "../lib/CLodHandler.h"
#include "../lib/CObjectHandler.h"
#include "../lib/CSpellHandler.h"
#include "../lib/CTownHandler.h"
#include "../lib/CondSh.h"
#include "../lib/map.h"
#include "mapHandler.h"
#include "../timeHandler.h"
#include <boost/foreach.hpp>
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
#include "CVideoHandler.h"
#include "../StartInfo.h"
#include "CPreGame.h"
#include "../lib/HeroBonus.h"
#include "../lib/CCreatureHandler.h"
#include "CMusicHandler.h"
#include "../lib/BattleState.h"
#include "../lib/CGameState.h"

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

std::list<CFocusable*> CFocusable::focusables;
CFocusable * CFocusable::inputWithFocus;

#undef min
#undef max


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
				else if(owner->armedObjs[0] && owner->armedObjs[0]->ID == TOWNI_TYPE)
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
				  && highl->stacksCount() == 1	//it's only stack
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
		GH.statusbar->print(temp);
	}
	else
	{
		GH.statusbar->clear();
	}
}

const CArmedInstance * CGarrisonSlot::getObj()
{
	return 	(!upg)?(owner->armedObjs[0]):(owner->armedObjs[1]);
}

bool CGarrisonSlot::our()
{
	return 	upg?(owner->owned[1]):(owner->owned[0]);
}

void CGarrisonSlot::clickRight(tribool down, bool previousState)
{
	if(down && creature)
	{
		//GH.pushInt(new CCreInfoWindow(*myStack));
		GH.pushInt(new CCreatureWindow(*myStack, 2));
	}
}
void CGarrisonSlot::clickLeft(tribool down, bool previousState)
{
	if(down)
	{
		bool refr = false;
		if(owner->highlighted)
		{
			if(owner->highlighted == this) //view info
			{
				UpgradeInfo pom;
				LOCPLINT->cb->getUpgradeInfo(getObj(), ID, pom);

				bool canUpgrade = getObj()->tempOwner == LOCPLINT->playerID && pom.oldID>=0; //upgrade is possible
				bool canDismiss = getObj()->tempOwner == LOCPLINT->playerID && (getObj()->stacksCount()>1  || !getObj()->needsLastStack());
				boost::function<void()> upgr = NULL;
				boost::function<void()> dism = NULL;
				if (canUpgrade) upgr = boost::bind(&CCallback::upgradeCreature, LOCPLINT->cb, getObj(), ID, pom.newID[0]);
				if (canDismiss) dism = boost::bind(&CCallback::dismissCreature, LOCPLINT->cb, getObj(), ID);

				//CCreInfoWindow *creWindow = new CCreInfoWindow( *myStack, 1, upgr, dism, &pom);
				CCreatureWindow *creWindow = new CCreatureWindow( *myStack, 3, upgr, dism, &pom);
				
				GH.pushInt(creWindow);

				owner->highlighted = NULL;
				owner->splitting = false;

				for(size_t i = 0; i<owner->splitButtons.size(); i++)
					owner->splitButtons[i]->block(true);

				redraw();
				refr = true;
			}
			else 
			{
				// Only allow certain moves if troops aren't removable or not ours.
				if (  ( owner->highlighted->our()//our creature is selected
				     || owner->highlighted->creature == creature )//or we are rebalancing army
				   && ( owner->removableUnits
				     || (upg == 0 &&  ( owner->highlighted->upg == 1 && !creature ) )
					 || (upg == 1 &&    owner->highlighted->upg == 1 ) ) )
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
							if(owner->highlighted->getObj()->stacksCount() == 1 //we're splitting away the last stack
								&& owner->highlighted->getObj()->needsLastStack() )
							{
								last = 0;
							}
							if(getObj()->stacksCount() == 1 //destination army can't be emptied, unless we're rebalancing two stacks of same creature
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
							(!upg)?(owner->armedObjs[0]):(owner->armedObjs[1]),
							(!owner->highlighted->upg)?(owner->armedObjs[0]):(owner->armedObjs[1]),
							ID,owner->highlighted->ID);
					}
					else //merge
					{
						LOCPLINT->cb->mergeStacks(
							(!owner->highlighted->upg)?(owner->armedObjs[0]):(owner->armedObjs[1]),
							(!upg)?(owner->armedObjs[0]):(owner->armedObjs[1]),
							owner->highlighted->ID,ID);
					}
				}
				else // Highlight
				{ 
					if(creature)
						owner->highlighted = this;
					redraw();
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
			redraw();
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
CGarrisonSlot::CGarrisonSlot(CGarrisonInt *Owner, int x, int y, int IID, int Upg, const CStackInstance * Creature)
{
	//assert(Creature == CGI->creh->creatures[Creature->idNumber]);
	active = false;
	upg = Upg;
	ID = IID;
	myStack = Creature;
	creature = Creature ? Creature->type : NULL;
	count = Creature ? Creature->count : 0;
	pos.x += x;
	pos.y += y;
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
void CGarrisonSlot::showAll(SDL_Surface * to)
{
	std::map<int,SDL_Surface*> &imgs = (owner->smallIcons ? graphics->smallImgs : graphics->bigImgs);
	if(creature)
	{
		char buf[15];
		SDL_itoa(count,buf,10);
		blitAt(imgs[creature->idNumber],pos,to);
		printTo(buf, pos.x+pos.w, pos.y+pos.h+1, owner->smallIcons ? FONT_TINY : FONT_MEDIUM, zwykly, to);

		if((owner->highlighted==this)
			|| (owner->splitting && owner->highlighted->creature == creature))
		{
			blitAt(imgs[-1],pos,to);
		}
	}
	else//empty slot
	{
		if(owner->splitting && owner->highlighted->our())
			blitAt(imgs[-1],pos,to);
	}
}

CGarrisonInt::~CGarrisonInt()
{/*
	for(size_t i = 0; i<splitButtons.size(); i++)
		delete splitButtons[i];*/
}

void CGarrisonInt::addSplitBtn(AdventureMapButton * button)
{
	addChild(button);
	button->recActions = defActions;
	splitButtons.push_back(button);
}

void CGarrisonInt::createSet(std::vector<CGarrisonSlot*> &ret, const CCreatureSet * set, int posX, int posY, int distance, int Upg )
{
	ret.resize(7);
	
	for(TSlots::const_iterator i=set->Slots().begin(); i!=set->Slots().end(); i++)
	{
		ret[i->first] = new CGarrisonSlot(this, posX + (i->first*distance), posY, i->first, Upg, i->second);
	}

	for(int i=0; i<ret.size(); i++)
		if(!ret[i])
			ret[i] = new CGarrisonSlot(this, posX + (i*distance), posY,i,Upg,NULL);

	if (twoRows)
		for (int i=4; i<ret.size(); i++)
		{
			ret[i]->pos.x -= 126;
			ret[i]->pos.y += 37;
		};
}

void CGarrisonInt::createSlots()
{
	OBJ_CONSTRUCTION_CAPTURING_ALL;
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

	if(armedObjs[0])
		createSet(slotsUp, armedObjs[0], 0, 0, w+interx, 0);

	if(armedObjs[1])
		createSet (slotsDown, armedObjs[1], garOffset.x, garOffset.y, w+interx, 1);
}

void CGarrisonInt::deleteSlots()
{
	for (int i=0; i<slotsUp.size(); i++)
		delChildNUll(slotsUp[i]);
		
	for (int i=0; i<slotsDown.size(); i++)
		delChildNUll(slotsDown[i]);
}

void CGarrisonInt::recreateSlots()
{

	splitting = false;
	highlighted = NULL;

	for(size_t i = 0; i<splitButtons.size(); i++)
		splitButtons[i]->block(true);

	bool wasActive = active;
	if(active)
	{
		deactivate();
	}
	
	deleteSlots();
	createSlots();
	
	if(wasActive)
	{
		activate();
		showAll(screen2);
	}
}

void CGarrisonInt::splitClick()
{
	if(!highlighted)
		return;
	splitting = !splitting;
	redraw();
}
void CGarrisonInt::splitStacks(int am2)
{
	LOCPLINT->cb->splitStack(armedObjs[highlighted->upg], armedObjs[pb], highlighted->ID, p2, am2);
}

CGarrisonInt::CGarrisonInt(int x, int y, int inx, const Point &garsOffset, 
                            SDL_Surface *pomsur, const Point& SurOffset, 
                            const CArmedInstance *s1, const CArmedInstance *s2, 
                            bool _removableUnits, bool smallImgs, bool _twoRows )
	: interx(inx), garOffset(garsOffset), highlighted(NULL), splitting(false),
	smallIcons(smallImgs), removableUnits (_removableUnits), twoRows(_twoRows)
{
	setArmy(s1, false);
	setArmy(s2, true);
	pos.x += x;
	pos.y += y;
	createSlots();
}

void CGarrisonInt::activate()
{
	for(size_t i = 0; i<splitButtons.size(); i++)
		if( (splitButtons[i]->isBlocked()) != !highlighted)
			splitButtons[i]->block(!highlighted);

	CIntObject::activate();
}

void CGarrisonInt::setArmy(const CArmedInstance *army, bool bottomGarrison)
{
	owned[bottomGarrison] =  army ? (army->tempOwner == LOCPLINT->playerID || army->tempOwner == 254) : false; //254 - neutral objects (pandora, banks)
	armedObjs[bottomGarrison] = army;
}

CInfoWindow::CInfoWindow(std::string Text, int player, const TCompsInfo &comps, const TButtonsInfo &Buttons, bool delComps)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL;

	type |= BLOCK_ADV_HOTKEYS;
	ID = -1;
	for(int i=0;i<Buttons.size();i++)
	{
		AdventureMapButton *button = new AdventureMapButton("","",boost::bind(&CInfoWindow::close,this),0,0,Buttons[i].first);
		button->borderColor = Colors::MetallicGold;
		button->borderEnabled = true;
		button->callback.add(Buttons[i].second); //each button will close the window apart from call-defined actions
		buttons.push_back(button);
	}

	text = new CTextBox(Text, Rect(0, 0, 250, 100), 0, FONT_MEDIUM, CENTER, zwykly);
	text->redrawParentOnScrolling = true;

	if(buttons.size())
	{
		buttons.front()->assignedKeys.insert(SDLK_RETURN); //first button - reacts on enter
		buttons.back()->assignedKeys.insert(SDLK_ESCAPE); //last button - reacts on escape
	}

	for(int i=0;i<comps.size();i++)
	{
		comps[i]->recActions = 0xff;
		addChild(comps[i]);
		comps[i]->recActions &= ~(SHOWALL | UPDATE);
		components.push_back(comps[i]);
	}
	setDelComps(delComps);
	CMessage::drawIWindow(this,Text,player);
}

CInfoWindow::CInfoWindow() 
{
	ID = -1;
	setDelComps(false);
	text = NULL;
}

void CInfoWindow::close()
{
	GH.popIntTotally(this);
	if(LOCPLINT)
		LOCPLINT->showingDialog->setn(false);
}

void CInfoWindow::show(SDL_Surface * to)
{
	CIntObject::show(to);
}

CInfoWindow::~CInfoWindow()
{
	if(!delComps)
	{
		for (int i=0;i<components.size();i++)
			removeChild(components[i]);
	}
}

void CInfoWindow::showAll( SDL_Surface * to )
{
	CSimpleWindow::show(to);
	CIntObject::showAll(to);
}

void CInfoWindow::showYesNoDialog(const std::string & text, const std::vector<SComponent*> *components, const CFunctionList<void( ) > &onYes, const CFunctionList<void()> &onNo, bool DelComps, int player)
{
	assert(!LOCPLINT || LOCPLINT->showingDialog->get());
	std::vector<std::pair<std::string,CFunctionList<void()> > > pom;
	pom.push_back(std::pair<std::string,CFunctionList<void()> >("IOKAY.DEF",0));
	pom.push_back(std::pair<std::string,CFunctionList<void()> >("ICANCEL.DEF",0));
	CInfoWindow * temp = new CInfoWindow(text, player, components ? *components : std::vector<SComponent*>(), pom, DelComps);
	for(int i=0;i<onYes.funcs.size();i++)
		temp->buttons[0]->callback += onYes.funcs[i];
	for(int i=0;i<onNo.funcs.size();i++)
		temp->buttons[1]->callback += onNo.funcs[i];

	GH.pushInt(temp);
}

CInfoWindow * CInfoWindow::create(const std::string &text, int playerID /*= 1*/, const std::vector<SComponent*> *components /*= NULL*/, bool DelComps)
{
	std::vector<std::pair<std::string,CFunctionList<void()> > > pom;
	pom.push_back(std::pair<std::string,CFunctionList<void()> >("IOKAY.DEF",0));
	CInfoWindow * ret = new CInfoWindow(text, playerID, components ? *components : std::vector<SComponent*>(), pom, DelComps);
	return ret;
}

void CInfoWindow::setDelComps(bool DelComps)
{
	delComps = DelComps;
	BOOST_FOREACH(SComponent *comp, components)
	{
		if(delComps)
			comp->recActions |= DISPOSE;
		else
			comp->recActions &= ~DISPOSE;
	}
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

void CRClickPopup::createAndPush(const std::string &txt, const CInfoWindow::TCompsInfo &comps)
{
	int player = LOCPLINT ? LOCPLINT->playerID : 1; //if no player, then use blue

	CSimpleWindow * temp = new CInfoWindow(txt, player, comps);
	temp->center(Point(GH.current->motion)); //center on mouse
	temp->fitToScreen(10);
	CRClickPopupInt *rcpi = new CRClickPopupInt(temp,true);
	GH.pushInt(rcpi);
}

void CRClickPopup::createAndPush(const CGObjectInstance *obj, const Point &p, EAlignment alignment /*= BOTTOMRIGHT*/)
{
	SDL_Surface *iWin = LOCPLINT->infoWin(obj); //try get custom infowindow for this obj
	if(iWin)
		GH.pushInt(new CInfoPopup(iWin, p, alignment, true));
	else
		CRClickPopup::createAndPush(obj->getHoverText());
}

CRClickPopup::CRClickPopup()
{
}

CRClickPopup::~CRClickPopup()
{
}

CInfoPopup::CInfoPopup(SDL_Surface * Bitmap, int x, int y, bool Free)
 :free(Free),bitmap(Bitmap)
{
	init(x, y);
}


CInfoPopup::CInfoPopup(SDL_Surface * Bitmap, const Point &p, EAlignment alignment, bool Free/*=false*/)
 : free(Free),bitmap(Bitmap)
{
	switch(alignment)
	{
	case BOTTOMRIGHT:
		init(p.x - Bitmap->w, p.y - Bitmap->h);
		break;
	case CENTER:
		init(p.x - Bitmap->w/2, p.y - Bitmap->h/2);
		break;
	case TOPLEFT:
		init(p.x, p.y);
		break;
	default:
		assert(0); //not implemented
	}
}

CInfoPopup::CInfoPopup(SDL_Surface *Bitmap, bool Free)
{
	CCS->curh->hide();

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
	CCS->curh->show();
}

void CInfoPopup::init(int x, int y)
{
	CCS->curh->hide();

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

void SComponent::init(Etype Type, int Subtype, int Val)
{
	std::ostringstream oss;
	switch (Type)
	{
	case artifact:
		description = CGI->arth->artifacts[Subtype]->Description();
		subtitle = CGI->arth->artifacts[Subtype]->Name();
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
	case morale:
		description = CGI->generaltexth->heroscrn[ 4 - (val>0) + (val<0)];
		break;
	case luck:
		description = CGI->generaltexth->heroscrn[ 7 - (val>0) + (val<0)];
		break;
	case resource:
		description = CGI->generaltexth->allTexts[242];
		oss << Val;
		subtitle = oss.str();
		break;
	case spell:
		description = CGI->spellh->spells[Subtype]->descriptions[Val];
		subtitle = CGI->spellh->spells[Subtype]->name;
		break;
	case creature:
		subtitle = (Val? boost::lexical_cast<std::string>(Val) + " " : "") + CGI->creh->creatures[Subtype]->*(Val != 1 ? &CCreature::namePl : &CCreature::nameSing);
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
	img = NULL;
	free = false;
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
SComponent::SComponent(Etype Type, int Subtype, int Val, SDL_Surface *sur, bool freeSur):img(sur),free(freeSur)
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

SComponent::SComponent()
{
	img = NULL;
}

SComponent::~SComponent()
{
	if (free && img)
		SDL_FreeSurface(img);
}

SDL_Surface * SComponent::setSurface(std:: string defname, int imagepos)
{
	if (img)
		tlog1<<"SComponent::setSurface: Warning - surface is already set!\n";
	CDefEssential * def = CDefHandler::giveDefEss(defname);
	
	free = true;
	img = def->ourImages[imagepos].bitmap;
	img->refcount++;//to preserve surface whed def is deleted
	delete def;
	return img;
}

void SComponent::show(SDL_Surface * to)
{
	blitAt(getImg(),pos.x,pos.y,to);
}

SDL_Surface * SComponent::getImg()
{
	if (img)
		return img;
	switch (type)
	{
	case artifact:
		return graphics->artDefs->ourImages[subtype].bitmap;
	case primskill:
		return graphics->pskillsb->ourImages[subtype].bitmap;
	case secskill44:
		return graphics->abils44->ourImages[subtype*3 + 3 + val - 1].bitmap;
	case secskill:
		return graphics->abils82->ourImages[subtype*3 + 3 + val - 1].bitmap;
	case resource:
		return graphics->resources->ourImages[subtype].bitmap;
	case experience:
		return graphics->pskillsb->ourImages[4].bitmap;
	case morale:
		return graphics->morale82->ourImages[val+3].bitmap;
	case luck:
		return graphics->luck82->ourImages[val+3].bitmap;
	case spell:
		return graphics->spellscr->ourImages[subtype].bitmap;
	case building:
		return setSurface(graphics->buildingPics[subtype],val);
	case creature:
		return graphics->bigImgs[subtype];
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
		adventureInt->handleRightClick(description,down);
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
void CSelectableComponent::init()
{
	selected = false;
}
CSelectableComponent::CSelectableComponent(const Component &c, boost::function<void()> OnSelect)
:SComponent(c),onSelect(OnSelect)
{
	init();
}
CSelectableComponent::CSelectableComponent(Etype Type, int Sub, int Val, boost::function<void()> OnSelect)
:SComponent(Type,Sub,Val),onSelect(OnSelect)
{
	init();
}
CSelectableComponent::~CSelectableComponent()
{
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
void CSelectableComponent::select(bool on)
{
	if(on != selected)
	{
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
	blitAt(getImg(),pos.x,pos.y,to);
	if(selected)
	{
		CSDL_Ext::drawBorder(to, Rect::around(Rect(pos.x, pos.y, getImg()->w, getImg()->h)), int3(239,215,123));
	}
	
	printAtMiddleWB(subtitle,pos.x+pos.w/2,pos.y+pos.h+25,FONT_SMALL,12,zwykly,to);
}

void CSimpleWindow::show(SDL_Surface * to)
{
	if(bitmap)
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
CSelWindow::CSelWindow(const std::string &Text, int player, int charperline, const std::vector<CSelectableComponent*> &comps, const std::vector<std::pair<std::string,CFunctionList<void()> > > &Buttons, int askID)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL;
	ID = askID;
	for(int i=0;i<Buttons.size();i++)
	{
		buttons.push_back(new AdventureMapButton("","",Buttons[i].second,0,0,Buttons[i].first));
		if(!i  &&  askID >= 0)
			buttons.back()->callback += boost::bind(&CSelWindow::madeChoice,this);
		buttons[i]->callback += boost::bind(&CInfoWindow::close,this); //each button will close the window apart from call-defined actions
	}

	text = new CTextBox(Text, Rect(0, 0, 250, 100), 0, FONT_MEDIUM, CENTER, zwykly);
	text->redrawParentOnScrolling = true;

	buttons.front()->assignedKeys.insert(SDLK_RETURN); //first button - reacts on enter
	buttons.back()->assignedKeys.insert(SDLK_ESCAPE); //last button - reacts on escape

	if(buttons.size() > 1  &&  askID >= 0) //cancel button functionality
		buttons.back()->callback += boost::bind(&CCallback::selectionMade,LOCPLINT->cb,0,askID);

	for(int i=0;i<comps.size();i++)
	{
		comps[i]->recActions = 255;
		addChild(comps[i]);
		components.push_back(comps[i]);
		comps[i]->onSelect = boost::bind(&CSelWindow::selectionChange,this,i);
		if(i<9)
			comps[i]->assignedKeys.insert(SDLK_1+i);
	}
	CMessage::drawIWindow(this, Text, player);

	BOOST_FOREACH(SComponent *c, components)
		c->subtitle = "";//workaround - erase subtitles since they were hard-blitted by function drawing window
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
	pos.x += x;
	pos.y += y;
	if(maxw >= 0)
		pos.w = std::min(bg->w,maxw);
	else
		pos.w=bg->w;
	pos.h=bg->h;
	middlex=(pos.w/2)+pos.x;
	middley=(bg->h/2)+pos.y;
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
		redraw();
	}
}

void CStatusBar::print(const std::string & text)
{
	if(LOCPLINT->cingconsole->enteredText == "" || text == LOCPLINT->cingconsole->enteredText) //for appropriate support for in-game console
	{
		current=text;
		redraw();
	}
}

void CStatusBar::show(SDL_Surface * to)
{
	SDL_Rect srcRect = genRect(pos.h,pos.w,0,0);
	SDL_Rect dstRect = genRect(pos.h,pos.w,pos.x,pos.y);
	CSDL_Ext::blitSurface(bg,&srcRect,to,&dstRect);
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
	if(selected < 0) //no selection, do nothing
		return;
	if(selected < from) //scroll up
		from = selected;
	else if(from + SIZE <= selected)
		from = selected - SIZE + 1; 

	amin(from, size() - SIZE);
	amax(from, 0);
	draw(screen);
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
	Rect srcRect = genRect(w, h, pos.x, pos.y);
	Rect dstRect = genRect(w, h, 0, 0);
	CSDL_Ext::blitSurface(adventureInt->bg, &srcRect, bg, &dstRect);
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
		adventureInt->selection = NULL;
		adventureInt->terrain.currentPath = NULL;
		draw(screen);
		adventureInt->infoBar.showAll(screen);
	}
	if (which>=LOCPLINT->wanderingHeroes.size())
		return;

	selected = which;
	adventureInt->select(LOCPLINT->wanderingHeroes[which]);
	fixPos();
	draw(screen);
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
			if(LOCPLINT->wanderingHeroes.size()-from>SIZE)
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
		if ( (ny+from)==selected && (adventureInt->selection->ID == HEROI_TYPE))
			LOCPLINT->openHeroWindow(LOCPLINT->wanderingHeroes[selected]);//print hero screen
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
			adventureInt->statusbar.print(CGI->generaltexth->zelp[303].first);
		else
			adventureInt->statusbar.clear();
		return;
	}
	else if(isItIn(&arrdop,GH.current->motion.x,GH.current->motion.y))
	{
		if ((LOCPLINT->wanderingHeroes.size()-from)  >  SIZE)
			adventureInt->statusbar.print(CGI->generaltexth->zelp[304].first);
		else
			adventureInt->statusbar.clear();
		return;
	}
	//if not buttons then heroes
	int hx = GH.current->motion.x, hy = GH.current->motion.y;
	hx-=pos.x;
	hy-=pos.y; hy-=arrup->ourImages[0].bitmap->h;
	int ny = hy/32;
	if ((ny>SIZE || ny<0) || (from+ny>=LOCPLINT->wanderingHeroes.size()))
	{
		adventureInt->statusbar.clear();
		return;
	}
	std::vector<std::string> temp;
	temp.push_back(LOCPLINT->wanderingHeroes[from+ny]->name);
	temp.push_back(LOCPLINT->wanderingHeroes[from+ny]->type->heroClass->name);
	adventureInt->statusbar.print( processStr(CGI->generaltexth->allTexts[15],temp) );
	//select(ny+from);
}

void CHeroList::clickRight(tribool down, bool previousState)
{
	if (down)
	{
		/***************************ARROWS*****************************************/
		if(isItIn(&arrupp,GH.current->motion.x,GH.current->motion.y) && from>0)
		{
			adventureInt->handleRightClick(CGI->generaltexth->zelp[303].second,down);
		}
		else if(isItIn(&arrdop,GH.current->motion.x,GH.current->motion.y) && (LOCPLINT->wanderingHeroes.size()-from>5))
		{
			adventureInt->handleRightClick(CGI->generaltexth->zelp[304].second,down);
		}
		else
		{
			//if not buttons then heroes
			int hx = GH.current->motion.x, hy = GH.current->motion.y;
			hx-=pos.x;
			hy-=pos.y; hy-=arrup->ourImages[0].bitmap->h;
			int ny = hy/32;
			if ((ny>SIZE || ny<0) || (from+ny>=LOCPLINT->wanderingHeroes.size()))
			{
				return;
			}			//show popup

			CRClickPopup::createAndPush(LOCPLINT->wanderingHeroes[from+ny], GH.current->motion);
		}
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
		LOCPLINT->wanderingHeroes -= toRemove;
	else
		LOCPLINT->recreateHeroTownList();


	if(selected >= LOCPLINT->wanderingHeroes.size())
		select(LOCPLINT->wanderingHeroes.size()-1);

	if(toRemove)
	{
		if(LOCPLINT->wanderingHeroes.size() == 0)
			adventureInt->townList.select(0);
		else
			select(selected);
	}
}

void CHeroList::updateMove(const CGHeroInstance* which) //draws move points bar
{
	int ser = -1;
	for(int i=0; i<LOCPLINT->wanderingHeroes.size() && ser<0; i++)
		if(LOCPLINT->wanderingHeroes[i]->subID == which->subID)
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
		if (iT>=LOCPLINT->wanderingHeroes.size())
		{
			blitAt(mobile->ourImages[0].bitmap,posmobx,posmoby+i*32,to);
			blitAt(mana->ourImages[0].bitmap,posmanx,posmany+i*32,to);
			blitAt(empty,posporx,pospory+i*32,to);
			continue;
		}
		const CGHeroInstance *cur = LOCPLINT->wanderingHeroes[iT];
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
		if (adventureInt->selection && (selected == iT) && (adventureInt->selection->ID == HEROI_TYPE))
		{
			blitAt(selection,posporx,pospory+i*32,to);
		}
		//TODO: support for custom portraits
	}
	if (from>0)
		blitAt(arrup->ourImages[0].bitmap,arrupp.x,arrupp.y,to);
	else
		blitAt(arrup->ourImages[2].bitmap,arrupp.x,arrupp.y,to);

	if (LOCPLINT->wanderingHeroes.size()-from > SIZE)
		blitAt(arrdo->ourImages[0].bitmap,arrdop.x,arrdop.y,to);
	else
		blitAt(arrdo->ourImages[2].bitmap,arrdop.x,arrdop.y,to);
}

int CHeroList::getPosOfHero(const CGHeroInstance* h)
{
	return vstd::findPos(LOCPLINT->wanderingHeroes, h, std::equal_to<const CGHeroInstance*>());
}

void CHeroList::show( SDL_Surface * to )
{

}

void CHeroList::showAll( SDL_Surface * to )
{
	draw(to);
}

int CHeroList::size()
{
	return LOCPLINT->wanderingHeroes.size();
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
	pos.x += x;
	pos.y += y;
	pos.w = std::max(arrdo->width, arrup->width);
	pos.h = arrdo->height + arrup->height + Size*32;

	arrupp.x=pos.x;
	arrupp.y=pos.y;
	arrupp.w=arrup->width;
	arrupp.h=arrup->height;
	arrdop.x=pos.x;
	arrdop.y=pos.y+arrup->height+32*SIZE;
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
// 	LOCPLINT->towns.clear();
// 	int howMany = LOCPLINT->cb->howManyTowns();
// 	for (int i=0;i<howMany;i++)
// 	{
// 		LOCPLINT->towns.push_back(LOCPLINT->cb->getTownInfo(i,0));
// 	}
}

void CTownList::select(int which)
{
	if (which>=LOCPLINT->towns.size())
		return;
	selected = which;
	fixPos();
	if(!fun.empty())
		fun();
}

void CTownList::mouseMoved (const SDL_MouseMotionEvent & sEvent)
{
	if(isItIn(&arrupp,GH.current->motion.x,GH.current->motion.y))
	{
		if (from>0)
			GH.statusbar->print(CGI->generaltexth->zelp[306].first);
		else
			GH.statusbar->clear();
		return;
	}
	else if(isItIn(&arrdop,GH.current->motion.x,GH.current->motion.y))
	{
		if ((LOCPLINT->towns.size()-from)  >  SIZE)
			GH.statusbar->print(CGI->generaltexth->zelp[307].first);
		else
			GH.statusbar->clear();
		return;
	}
	//if not buttons then towns
	int hx = GH.current->motion.x, hy = GH.current->motion.y;
	hx-=pos.x;
	hy-=pos.y; hy-=arrup->ourImages[0].bitmap->h;
	int ny = hy/32;
	if ((ny>=SIZE || ny<0) || (from+ny>=LOCPLINT->towns.size()))
	{
		GH.statusbar->clear();
		return;
	};
	std::string temp = CGI->generaltexth->tcommands[4];
	boost::algorithm::replace_first(temp,"%s",LOCPLINT->towns[from+ny]->name);
	temp += ", "+LOCPLINT->towns[from+ny]->town->Name();
	GH.statusbar->print(temp);
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
			if(LOCPLINT->towns.size()-from > SIZE)
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
		if (ny>=SIZE || ny<0)
			return;
		if(GH.topInt() == adventureInt
		  && (ny+from)==selected 
		  && adventureInt->selection->ID == TOWNI_TYPE
		  )
			LOCPLINT->openTownWindow(LOCPLINT->towns[selected]);//print town screen
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
				//if (from<LOCPLINT->towns.size()-5)
				//	from=LOCPLINT->towns.size()-5;

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
			adventureInt->handleRightClick(CGI->generaltexth->zelp[306].second,down);
		}
		else if(isItIn(&arrdop,GH.current->motion.x,GH.current->motion.y) && (LOCPLINT->towns.size()-from>5))
		{
			adventureInt->handleRightClick(CGI->generaltexth->zelp[307].second,down);
		}
		//if not buttons then towns
		int hx = GH.current->motion.x, hy = GH.current->motion.y;
		hx-=pos.x;
		hy-=pos.y; hy-=arrup->ourImages[0].bitmap->h;
		int ny = hy/32;
		if ((ny>=SIZE || ny<0) || (from+ny>=LOCPLINT->towns.size()))
		{
			return;
		}

		//show popup
		CRClickPopup::createAndPush(LOCPLINT->towns[from+ny], GH.current->motion);
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
		if (iT>=LOCPLINT->towns.size())
		{
			blitAt(graphics->getPic(-1),posporx,pospory+i*32,to);
			continue;
		}

		blitAt(graphics->getPic(LOCPLINT->towns[iT]->subID,LOCPLINT->towns[iT]->hasFort(),LOCPLINT->towns[iT]->builded),posporx,pospory+i*32,to);

		if (adventureInt->selection && (selected == iT) && (adventureInt->selection->ID == TOWNI_TYPE))
		{
			blitAt(graphics->getPic(-2),posporx,pospory+i*32,to);
		}
	}
	if (from>0)
		blitAt(arrup->ourImages[0].bitmap,arrupp.x,arrupp.y,to);
	else
		blitAt(arrup->ourImages[2].bitmap,arrupp.x,arrupp.y,to);

	if (LOCPLINT->towns.size()-from>SIZE)
		blitAt(arrdo->ourImages[0].bitmap,arrdop.x,arrdop.y,to);
	else
		blitAt(arrdo->ourImages[2].bitmap,arrdop.x,arrdop.y,to);
}

void CTownList::show( SDL_Surface * to )
{

}

void CTownList::showAll( SDL_Surface * to )
{
	draw(to);
}

int CTownList::size()
{
	return LOCPLINT->towns.size();
}

CCreaturePic::CCreaturePic(int x, int y, const CCreature *cre, bool Big, bool Animated)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL;
	pos.x+=x;
	pos.y+=y;
	
	if(Big)
		bg = new CPicture(graphics->backgrounds[cre->faction],0,0,false);
	else
		bg = new CPicture(graphics->backgroundsm[cre->faction],0,0,false);
	bg->needRefresh = true;
	anim = new CCreatureAnim(0, 0, cre->animDefName, Rect());
	anim->clipRect(cre->doubleWide?170:150, 155, bg->pos.w, bg->pos.h);
	anim->startPreview();
}

CCreaturePic::~CCreaturePic()
{
	
}

void CRecruitmentWindow::close()
{
	GH.popIntTotally(this);
}
void CRecruitmentWindow::Max()
{
	slider->moveToMax();
}
void CRecruitmentWindow::Buy()
{
	int crid = creatures[which].ID,
		dstslot = dst-> getSlotFor(crid);

	if(dstslot < 0 && !vstd::contains(CGI->arth->bigArtifacts,CGI->arth->convertMachineID(crid, true))) //no available slot
	{
		std::string txt;
		if(dst->ID == HEROI_TYPE)
		{
			txt = CGI->generaltexth->allTexts[425]; //The %s would join your hero, but there aren't enough provisions to support them.
			boost::algorithm::replace_first(txt, "%s", slider->value > 1 ? CGI->creh->creatures[crid]->namePl : CGI->creh->creatures[crid]->nameSing);
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
	redraw();
}
void CRecruitmentWindow::clickLeft(tribool down, bool previousState)
{
	for(int i=0;i<creatures.size();i++)
	{
		Rect creaPos = pos + creatures[i].pos;
		if(isItIn(&creaPos, GH.current->motion.x, GH.current->motion.y))
		{
			which = i;
			int newAmount = std::min(amounts[i],creatures[i].amount);
			slider->setAmount(newAmount);
			max->block(!newAmount);

			if(slider->value > newAmount)
				slider->moveTo(newAmount);
			else
				slider->moveTo(slider->value);
			redraw();
			break;
		}
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
			Rect creatureRect = genRect(132, sCREATURE_WIDTH, pos.x+curx, pos.y+64);
			if(isItIn(&creatureRect, GH.current->motion.x, GH.current->motion.y))
			{
				CCreatureWindow *popup = new CCreatureWindow(creatures[i].ID, 0, 0);
				GH.pushInt(popup);
				break;
			}
			curx += TOTAL_CREATURE_WIDTH;
		}
	}
}

void CRecruitmentWindow::showAll( SDL_Surface * to )
{
	CIntObject::showAll(to);
	
	char pom[15];
	SDL_itoa(creatures[which].amount-slider->value,pom,10); //available
	printAtMiddleLoc(pom,205,253,FONT_SMALL,zwykly,to);
	SDL_itoa(slider->value,pom,10); //recruit
	printAtMiddleLoc(pom,279,253,FONT_SMALL,zwykly,to);
	printAtMiddleLoc(CGI->generaltexth->allTexts[16] + " " + CGI->creh->creatures[creatures[which].ID]->namePl,243,32,FONT_BIG,tytulowy,to); //eg "Recruit Dragon flies"

	int curx = 122-creatures[which].res.size()*24;
	for(int i=creatures[which].res.size()-1; i>=0; i--)// decrement used to make gold displayed as first res
	{
		blitAtLoc(graphics->resources32->ourImages[creatures[which].res[i].first].bitmap,curx,243,to);
		blitAtLoc(graphics->resources32->ourImages[creatures[which].res[i].first].bitmap,curx+258,243,to);
		SDL_itoa(creatures[which].res[i].second,pom,10);
		printAtMiddleLoc(pom,curx+15,287,FONT_SMALL,zwykly,to);
		SDL_itoa(creatures[which].res[i].second * slider->value,pom,10);
		printAtMiddleLoc(pom,curx+15+258,287,FONT_SMALL,zwykly,to);
		curx+=32+16;//size of bitmap + distance between them
	}

	for(int j=0;j<creatures.size();j++)
	{
		if(which==j)
			drawBorder(*bitmap,creatures[j].pos,int3(255,0,0));
		else
			drawBorder(*bitmap,creatures[j].pos,int3(239,215,123));
	}
}

CRecruitmentWindow::CRecruitmentWindow(const CGDwelling *Dwelling, int Level, const CArmedInstance *Dst, const boost::function<void(int,int)> &Recruit, int y_offset)
:recruit(Recruit), dwelling(Dwelling), level(Level), dst(Dst)
{
	used = LCLICK | RCLICK;
	OBJ_CONSTRUCTION_CAPTURING_ALL;

	which = 0;

	bitmap = new CPicture("TPRCRT.bmp");
	bitmap->colorizeAndConvert(LOCPLINT->playerID);
	bitmap->center();
	pos = (bitmap->pos += Point(0, y_offset));

	bar = new CGStatusBar(8, 370, "APHLFTRT.bmp", 471);
	max = new AdventureMapButton(CGI->generaltexth->zelp[553],boost::bind(&CRecruitmentWindow::Max,this),134,313,"IRCBTNS.DEF",SDLK_m);
	buy = new AdventureMapButton(CGI->generaltexth->zelp[554],boost::bind(&CRecruitmentWindow::Buy,this),212,313,"IBY6432.DEF",SDLK_RETURN);
	cancel = new AdventureMapButton(CGI->generaltexth->zelp[555],boost::bind(&CRecruitmentWindow::Cancel,this),290,313,"ICN6432.DEF",SDLK_ESCAPE);
	slider = new CSlider(176,279,135,0,0,0,0,true);
	slider->moved = boost::bind(&CRecruitmentWindow::sliderMoved,this, _1);

	initCres();

	printAtMiddle(CGI->generaltexth->allTexts[346],113,232,FONT_SMALL,zwykly,*bitmap); //cost per troop t
	printAtMiddle(CGI->generaltexth->allTexts[465],205,233,FONT_SMALL,zwykly,*bitmap); //available t
	printAtMiddle(CGI->generaltexth->allTexts[16],279,233,FONT_SMALL,zwykly,*bitmap); //recruit t
	printAtMiddle(CGI->generaltexth->allTexts[466],371,232,FONT_SMALL,zwykly,*bitmap); //total cost t
	drawBorder(*bitmap,172,222,67,42,int3(239,215,123));
	drawBorder(*bitmap,246,222,67,42,int3(239,215,123));
	drawBorder(*bitmap,64,222,99,76,int3(239,215,123));
	drawBorder(*bitmap,322,222,99,76,int3(239,215,123));
	drawBorder(*bitmap,133,312,66,34,int3(173,142,66));
	drawBorder(*bitmap,211,312,66,34,int3(173,142,66));
	drawBorder(*bitmap,289,312,66,34,int3(173,142,66));

	//border for creatures
	int curx = 192 + 50 - (CREATURE_WIDTH*creatures.size()/2) - (SPACE_BETWEEN*(creatures.size()-1)/2);
	for(int i=0;i<creatures.size();i++)
	{
		creatures[i].pos.x = curx-1;
		creatures[i].pos.y = 65 - 1;
		creatures[i].pos.w = 100 + 2;
		creatures[i].pos.h = 130 + 2;
// 		if(which==i)
// 			drawBorder(*bitmap,curx-1,64,CREATURE_WIDTH,132,int3(255,0,0));
// 		else
// 			drawBorder(*bitmap,curx-1,64,CREATURE_WIDTH,132,int3(239,215,123));
		creatures[i].pic = new CCreaturePic(curx, 65, CGI->creh->creatures[creatures[i].ID]);
		curx += TOTAL_CREATURE_WIDTH;
	}

	if(!creatures[0].amount ||  !amounts[0])
	{
		max->block(true);
		slider->block(true);
	}
	buy->block(true);
}

CRecruitmentWindow::~CRecruitmentWindow()
{
}

void CRecruitmentWindow::initCres()
{
	creatures.clear();
	amounts.clear();

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
			const CCreature * cre= CGI->creh->creatures[cur.ID];

			for(int k=0; k<cre->cost.size(); k++)
				if(cre->cost[k])
					cur.res.push_back(std::make_pair(k,cre->cost[k]));
			amounts.push_back(cre->maxAmount(LOCPLINT->cb->getResourceAmount()));
		}
	}

	slider->setAmount(std::min(amounts[which],creatures[which].amount));
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
	animLeft = new CCreaturePic(pos.x+20,  pos.y+54, CGI->creh->creatures[cid], true, false);
	animRight = new CCreaturePic(pos.x+177, pos.y+54, CGI->creh->creatures[cid], true, false);

	std::string title = CGI->generaltexth->allTexts[256];
	boost::algorithm::replace_first(title,"%s",CGI->creh->creatures[cid]->namePl);
	printAtMiddle(title,150,34,FONT_BIG,tytulowy,bitmap);
}

CSplitWindow::~CSplitWindow() //d-tor
{
	SDL_FreeSurface(bitmap);
	delete ok;
	delete cancel;
	delete slider;
	delete animLeft;
	delete animRight;
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
	ok->showAll(to);
	cancel->showAll(to);
	slider->showAll(to);
	printAtMiddle(boost::lexical_cast<std::string>(a1) + (!which ? "_" : ""),pos.x+70,pos.y+237,FONT_BIG,zwykly,to);
	printAtMiddle(boost::lexical_cast<std::string>(a2) + (which ? "_" : ""),pos.x+233,pos.y+237,FONT_BIG,zwykly,to);
	animLeft->show(to);
	animRight->show(to);
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
	else if (k == SDLK_LEFT)
	{
		ncur--;
	}
	else if (k == SDLK_RIGHT)
	{
		ncur++;
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
	blitAt(*bitmap,pos.x,pos.y,to);
	anim->show(to);
	if(count.size())
		printTo(count.c_str(),pos.x+114,pos.y+174,FONT_TIMES,zwykly,to);
	if(upgrade)
		upgrade->showAll(to);
	if(dismiss)
		dismiss->showAll(to);
	if(ok)
		ok->showAll(to);
}

CCreInfoWindow::CCreInfoWindow(const CStackInstance &st, int Type, boost::function<void()> Upg, boost::function<void()> Dsm, UpgradeInfo *ui)
	: type(Type), dsm(Dsm), dismiss(0), upgrade(0), ok(0)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL;
	init(st.type, &st, dynamic_cast<const CGHeroInstance*>(st.armyObj), st.count);

	//print abilities text - if r-click popup
	if(type)
	{
		if(Upg && ui)
		{
			bool enough = true;
			for(std::set<std::pair<int,int> >::iterator i=ui->cost[0].begin(); i!=ui->cost[0].end(); i++) //calculate upgrade cost
			{
				BLOCK_CAPTURING;
				if(LOCPLINT->cb->getResourceAmount(i->first) < i->second*st.count)
					enough = false;
				upgResCost.push_back(new SComponent(SComponent::resource,i->first,i->second*st.count)); 
			}

			if(enough)
			{
				CFunctionList<void()> fs;
				fs += Upg;
				fs += boost::bind(&CCreInfoWindow::close,this);
				CFunctionList<void()> cfl;
				cfl = boost::bind(&CPlayerInterface::showYesNoDialog, LOCPLINT, CGI->generaltexth->allTexts[207], boost::ref(upgResCost), fs, 0, false);
				upgrade = new AdventureMapButton("",CGI->generaltexth->zelp[446].second,cfl,76,237,"IVIEWCR.DEF",SDLK_u);
			}
			else
			{
				upgrade = new AdventureMapButton("",CGI->generaltexth->zelp[446].second,boost::function<void()>(),76,237,"IVIEWCR.DEF");
				upgrade->callback.funcs.clear();
				upgrade->setOffset(2);
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
			dismiss = new AdventureMapButton("",CGI->generaltexth->zelp[445].second,cfl,21,237,"IVIEWCR2.DEF",SDLK_d);
		}
		ok = new AdventureMapButton("",CGI->generaltexth->zelp[445].second,boost::bind(&CCreInfoWindow::close,this),216,237,"IOKAY.DEF",SDLK_RETURN);
	}
}



CCreInfoWindow::CCreInfoWindow(int Cid, int Type, int creatureCount)
	: type(Type), dismiss(0), upgrade(0), ok(0)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL;
	const CCreature *cre = CGI->creh->creatures[Cid];
	init(cre, NULL, NULL, creatureCount);
}

CCreInfoWindow::CCreInfoWindow(const CStack &st, int Type /*= 0*/)
	: type(Type), dismiss(0), upgrade(0), ok(0)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL;
	init(st.getCreature(), &st, st.getMyHero(), st.count);
}

void CCreInfoWindow::printLine(int nr, const std::string &text, int baseVal, int val/*=-1*/, bool range/*=false*/)
{
	printAt(text, 155, 48 + nr*19, FONT_SMALL, zwykly, *bitmap);

	std::string hlp;
	if(range && baseVal != val)
		hlp = boost::str(boost::format("%d - %d") % baseVal % val);
	else if(baseVal != val && val>=0)
		hlp = boost::str(boost::format("%d (%d)") % baseVal % val);
	else
		hlp = boost::lexical_cast<std::string>(baseVal);

	printTo(hlp, 276, 61 + nr*19, FONT_SMALL, zwykly, *bitmap);
}

//void CCreInfoWindow::init(const CCreature *cre, const CStackInstance *stack, int creatureCount)
void CCreInfoWindow::init(const CCreature *cre, const CBonusSystemNode *stackNode, const CGHeroInstance *heroOwner, int creatureCount)
{
	c = cre;
	if(!stackNode) stackNode = c;

	bitmap = new CPicture("CRSTKPU.bmp");
	bitmap->colorizeAndConvert(LOCPLINT->playerID);
	pos = bitmap->center();

	anim = new CCreaturePic(21, 48, c);

	count = boost::lexical_cast<std::string>(creatureCount);

	printAtMiddle(c->namePl,149,30,FONT_SMALL,tytulowy,*bitmap); //creature name
	
	printLine(0, CGI->generaltexth->primarySkillNames[0], cre->valOfBonuses(Bonus::PRIMARY_SKILL, PrimarySkill::ATTACK), stackNode->valOfBonuses(Bonus::PRIMARY_SKILL, PrimarySkill::ATTACK));
	printLine(1, CGI->generaltexth->primarySkillNames[1], cre->valOfBonuses(Bonus::PRIMARY_SKILL, PrimarySkill::DEFENSE), stackNode->valOfBonuses(Bonus::PRIMARY_SKILL, PrimarySkill::DEFENSE));
	//if(c->shots)
	//	printLine(2, CGI->generaltexth->allTexts[198], c->shots);
	if(stackNode->valOfBonuses(Bonus::SHOTS))
		printLine(2, CGI->generaltexth->allTexts[198], stackNode->valOfBonuses(Bonus::SHOTS));

	//TODO
	int dmgMultiply = 1;
	if(heroOwner && stackNode->hasBonusOfType(Bonus::SIEGE_WEAPON))
		dmgMultiply += heroOwner->Attack(); 

	printLine(3, CGI->generaltexth->allTexts[199], stackNode->getMinDamage() * dmgMultiply, stackNode->getMaxDamage() * dmgMultiply, true);
	printLine(4, CGI->generaltexth->allTexts[388], cre->valOfBonuses(Bonus::STACK_HEALTH), stackNode->valOfBonuses(Bonus::STACK_HEALTH));
	printLine(6, CGI->generaltexth->zelp[441].first, cre->valOfBonuses(Bonus::STACKS_SPEED), stackNode->valOfBonuses(Bonus::STACKS_SPEED));

	//setting morale
	morale = new MoraleLuckBox(true, genRect(42, 42, 24, 189));
	morale->set(stackNode);
	//setting luck
	luck = new MoraleLuckBox(false, genRect(42, 42, 77, 189));
	luck->set(stackNode);

	//luck and morale
	int luck = 3, morale = 3;
	if(stackNode)
	{
		//add modifiers
		luck += stackNode->LuckVal();
		morale += stackNode->MoraleVal();
	}

	blitAt(graphics->morale42->ourImages[morale].bitmap, 24, 189, *bitmap);
	blitAt(graphics->luck42->ourImages[luck].bitmap, 77, 189, *bitmap);


	if(!type)
	{
		printAtWB(c->abilityText,17,231,FONT_SMALL,35,zwykly,*bitmap);
	}

	//if we are displying window fo r stack in battle, there are several more things that we need to display
	if(const CStack *battleStack = dynamic_cast<const CStack*>(stackNode))
	{
		//spell effects
		int printed=0; //how many effect pics have been printed
		std::vector<si32> spells = battleStack->activeSpells();
		BOOST_FOREACH(si32 effect, spells)
		{
			blitAt(graphics->spellEffectsPics->ourImages[effect + 1].bitmap, 127 + 52 * printed, 186, *bitmap); 
			++printed;
			if(printed >= 3) //we can fit only 3 effects
				break;
		}
		//print current health
		printLine(5, CGI->generaltexth->allTexts[200], battleStack->firstHPleft);
	}
}


CCreInfoWindow::~CCreInfoWindow()
{
 	for(int i=0; i<upgResCost.size();i++)
 		delete upgResCost[i];
}

void CCreInfoWindow::activate()
{
	CIntObject::activate();
	if(!type)
		activateRClick();
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
	if(!type)
		deactivateRClick();
	CIntObject::deactivate();
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
		comps.push_back(new CSelectableComponent(SComponent::secskill44, skills[i],
			hero->getSecSkillLevel( static_cast<CGHeroInstance::SecondarySkill>(skills[i]) )+1,
			boost::bind(&CLevelWindow::selectionChanged,this,i)));
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

	if(comps.size())
	{
		comps[0]->select(true);
	}
}
void CLevelWindow::selectionChanged(unsigned to)
{
	if(ok->isBlocked())
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
	ok->showAll(to);
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

void CMinorResDataBar::showAll(SDL_Surface * to)
{
	show(to);
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

CObjectListWindow::CObjectListWindow(const std::vector<int> &_items, CPicture * titlePic, std::string _title, std::string _descr,
				boost::function<void(int)> Callback, int initState)
				:title(_title), descr(_descr),items(_items),selected(initState)
{
	init = false;
	OBJ_CONSTRUCTION_CAPTURING_ALL;
	defActions = ACTIVATE | DEACTIVATE | UPDATE | SHOWALL | DISPOSE | SHARE_POS;
	used = LCLICK | KEYBOARD;
	
	onSelect = Callback;
	length = 9;
	pos.x = (screen->w-306)/2;
	pos.y = (screen->h-468)/2;
	
	bg = new CPicture("TPGATE.pcx");//x=0, y=0
	bg->colorizeAndConvert(LOCPLINT->playerID);
	
	slider = new CSlider(277, 120, 256, boost::bind(&CObjectListWindow::moveList,this, _1), length, items.size(), 0, false, 0);
	ok = new AdventureMapButton("","",boost::bind(&CObjectListWindow::elementSelected, this),15,402,"IOKAY.DEF", SDLK_RETURN);
	exit = new AdventureMapButton("","",boost::bind(&CGuiHandler::popIntTotally,&GH, this),228,402,"ICANCEL.DEF",SDLK_ESCAPE);
	pos.w = bg->pos.w;
	pos.h = bg->pos.h;
	titleImage = titlePic;
	titleImage->pos.x =153+pos.x-titleImage->pos.w/2;
	titleImage->pos.y =75 +pos.y-titleImage->pos.h/2;
	
	for (int i=0; i<length; i++)
		areas.push_back(genRect(22, 260, pos.x+15, pos.y+152+i*25 ));//rects for selecting\printing items
	init = true;
}

CObjectListWindow::~CObjectListWindow()
{
	delete titleImage;
}

void CObjectListWindow::elementSelected()
{
	boost::function<void(int)> toCall = onSelect;//save
	int where = items[selected];      //required variables
	GH.popIntTotally(this);//then destroy window
	toCall(where);//and send selected object
}

void CObjectListWindow::moveList(int which)
{
	if (init)//TODO: is there a way to disable running this when CSlider is created?
		showAll(screen2);
}

void CObjectListWindow::clickLeft(tribool down, bool previousState)
{
	if (previousState && (!down))
	{
		for (int i=0; i<areas.size(); i++)
			if(slider->value+i < items.size() && isItIn(&areas[i],GH.current->motion.x,GH.current->motion.y))
			{//check all areas to find element which was clicked
				selected = i+slider->value;
				showAll(screen2);
				return;
			}
	}
}

void CObjectListWindow::keyPressed (const SDL_KeyboardEvent & key)
{
	if(key.state != SDL_PRESSED) return;

	int sel = selected;

	switch(key.keysym.sym)
	{
	case SDLK_UP:	
					sel -=1;
					break;
	case SDLK_DOWN:
					sel +=1;
					break;
	case SDLK_PAGEUP:
					sel -=length;
					break;
	case SDLK_PAGEDOWN:
					sel +=length;
					break;
	case SDLK_HOME:
					sel = 0;
					break;
	case SDLK_END:
					sel = slider->amount;
					break;
	default:
		return;
	}
	if (sel<-1)//nothing was selected & list was moved up
		return;
	if (sel<0)//start of list reached
		sel = 0;
	if ( sel >= slider->amount )//end of list reached
		sel = slider->amount-1;
	if ( sel >= items.size() )
		sel = items.size()-1;
	if ( sel < slider->value )//need to move list up
		slider->moveTo(sel);
	else 
	if ( sel >= slider->value+length )//move to bottom
		slider->moveTo(sel-length+1);
	selected = sel;
	showAll(screen2);
}

void CObjectListWindow::show(SDL_Surface * to)
{
	
}

void CObjectListWindow::showAll(SDL_Surface * to)
{
	ok->block((selected<0)?2:0);
	CIntObject::showAll(to);
	CSDL_Ext::printAtMiddle(title,pos.x+152,pos.y+27,FONT_BIG,tytulowy,to);//"castle gate"
	CSDL_Ext::printAtMiddle(descr,pos.x+145,pos.y+133,FONT_SMALL,zwykly,to);//"select destination"
	titleImage->showAll(to);
	if ( selected >= slider->value && selected < slider->value+length )//if selected item is visible 
	{
		SDL_Rect a = areas[selected-slider->value];
		CSDL_Ext::drawBorder(to, a.x,   a.y,   a.w,   a.h,   int3(255, 231, 148));
		CSDL_Ext::drawBorder(to, a.x-1, a.y-1, a.w+2, a.h+2, int3(255, 231, 148));//border shoul be 2 pixels width
	}
	int position = slider->value;
	for ( int i = 0; i<9 && i<items.size()-position; i++)
		CSDL_Ext::printAtMiddle(CGI->mh->map->objects[items[i+position]]->hoverName,pos.x+145,pos.y+163+25*i,
			FONT_SMALL, zwykly, to);//print item names in list
}

CTradeWindow::CTradeableItem::CTradeableItem( EType Type, int ID, bool Left, int Serial)
{
	serial = Serial;
	left = Left;
	type = Type;
	id = ID;
	used = LCLICK | HOVER | RCLICK;
	downSelection = false;
	hlp = NULL;
}

void CTradeWindow::CTradeableItem::showAll(SDL_Surface * to)
{
	Point posToBitmap;
	Point posToSubCenter;

	switch(type)
	{
	case RESOURCE:
		posToBitmap = Point(19,9);
		posToSubCenter = Point(36, 59);
		break;
	case CREATURE_PLACEHOLDER:
	case CREATURE:
		posToSubCenter = Point(29, 76);
		if(downSelection)
			posToSubCenter.y += 5;
		break;
	case PLAYER:
		posToSubCenter = Point(31, 76);
		break;
	case ARTIFACT_PLACEHOLDER:
	case ARTIFACT_INSTANCE:
		posToSubCenter = Point(19, 55);
		if(downSelection)
			posToSubCenter.y += 8;
		break;
	case ARTIFACT_TYPE:
		posToSubCenter = Point(19, 58);
		break;
	}

	if(SDL_Surface *hlp = getSurface())
		blitAt(hlp, pos + posToBitmap, to);

	printAtMiddleLoc(subtitle, posToSubCenter, FONT_SMALL, zwykly, to);
}

void CTradeWindow::CTradeableItem::clickLeft(tribool down, bool previousState)
{
	CTradeWindow *mw = dynamic_cast<CTradeWindow *>(parent);
	assert(mw);
	if(down)
	{

		if(type == ARTIFACT_PLACEHOLDER)
		{
			CAltarWindow *aw = static_cast<CAltarWindow *>(mw);
			if(const CArtifactInstance *movedArt = aw->arts->commonInfo->src.art)
			{
				aw->moveFromSlotToAltar(aw->arts->commonInfo->src.slotID, this, movedArt);
			}
			else if(const CArtifactInstance *art = getArtInstance())
			{
				aw->arts->commonInfo->src.AOH = aw->arts;
				aw->arts->commonInfo->src.art = art;
				aw->arts->commonInfo->src.slotID = aw->hero->getArtPos(art);
				aw->arts->markPossibleSlots(art);

				//aw->arts->commonInfo->dst.AOH = aw->arts;
				CCS->curh->dragAndDropCursor(graphics->artDefs->ourImages[art->artType->id].bitmap);

				aw->arts->artifactsOnAltar.erase(art);
				id = -1;
				subtitle = "";
				aw->deal->block(!aw->arts->artifactsOnAltar.size());
			}

			aw->calcTotalExp();
			return;
		}
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

SDL_Surface * CTradeWindow::CTradeableItem::getSurface()
{
	switch(type)
	{
	case RESOURCE:
		return graphics->resources32->ourImages[id].bitmap;
	case PLAYER:
		return graphics->flags->ourImages[id].bitmap;
	case ARTIFACT_TYPE:
	case ARTIFACT_PLACEHOLDER:
	case ARTIFACT_INSTANCE:
		return id >= 0 ? graphics->artDefs->ourImages[id].bitmap : NULL;
	case CREATURE:
		return graphics->bigImgs[id];
	default:
		return NULL;
	}
}

void CTradeWindow::CTradeableItem::showAllAt(const Point &dstPos, const std::string &customSub, SDL_Surface * to)
{
	Rect oldPos = pos;
	std::string oldSub = subtitle;
	downSelection = true;

	pos = dstPos;
	subtitle = customSub;
	showAll(to);

	downSelection = false;
	pos = oldPos;
	subtitle = oldSub;
}

void CTradeWindow::CTradeableItem::hover(bool on)
{
	if(!on)
	{
		GH.statusbar->clear();
		return;
	}

	switch(type)
	{
	case CREATURE:
	case CREATURE_PLACEHOLDER:
		GH.statusbar->print(boost::str(boost::format(CGI->generaltexth->allTexts[481]) % CGI->creh->creatures[id]->namePl));
		break;
	case ARTIFACT_PLACEHOLDER:
		if(id < 0)
			GH.statusbar->print(CGI->generaltexth->zelp[582].first);
		else
			GH.statusbar->print(CGI->arth->artifacts[id]->Name());
		break;
	}
}

void CTradeWindow::CTradeableItem::clickRight(tribool down, bool previousState)
{
	if(down)
	{
		switch(type)
		{
		case CREATURE:
		case CREATURE_PLACEHOLDER:
			//GH.statusbar->print(boost::str(boost::format(CGI->generaltexth->allTexts[481]) % CGI->creh->creatures[id]->namePl));
			break;
		case ARTIFACT_TYPE:
		case ARTIFACT_PLACEHOLDER:
			if(id >= 0)
				adventureInt->handleRightClick(CGI->arth->artifacts[id]->Description(), down);
			break;
		}
	}
}

std::string CTradeWindow::CTradeableItem::getName(int number /*= -1*/) const
{
	switch(type)
	{
	case PLAYER:
		return CGI->generaltexth->capColors[id];
	case RESOURCE:
		return CGI->generaltexth->restypes[id];
	case CREATURE:
		if(number == 1)
			return CGI->creh->creatures[id]->nameSing;
		else
			return CGI->creh->creatures[id]->namePl;
	case ARTIFACT_TYPE:
	case ARTIFACT_INSTANCE:
		return CGI->arth->artifacts[id]->Name();
	}
	assert(0);
	return "";
}

const CArtifactInstance * CTradeWindow::CTradeableItem::getArtInstance() const
{
	switch(type)
	{
	case ARTIFACT_PLACEHOLDER:
	case ARTIFACT_INSTANCE:
		return (const CArtifactInstance *)hlp;
	default:
		return NULL;
	}
}

void CTradeWindow::CTradeableItem::setArtInstance(const CArtifactInstance *art)
{
	assert(type == ARTIFACT_PLACEHOLDER || type == ARTIFACT_INSTANCE);
	hlp = art;
	if(art)
		id = art->artType->id;
	else
		id = -1;
}

CTradeWindow::CTradeWindow(const IMarket *Market, const CGHeroInstance *Hero, EMarketMode Mode)
	: market(Market), hero(Hero),  arts(NULL), hLeft(NULL), hRight(NULL), readyToTrade(false)
{
	type |= BLOCK_ADV_HOTKEYS;
	mode = Mode;
	initTypes();
}

void CTradeWindow::initTypes()
{
	switch(mode)
	{
	case RESOURCE_RESOURCE:
		itemsType[1] = RESOURCE;
		itemsType[0] = RESOURCE;
		break;
	case RESOURCE_PLAYER:
		itemsType[1] = RESOURCE;
		itemsType[0] = PLAYER;
		break;
	case CREATURE_RESOURCE:
		itemsType[1] = CREATURE;
		itemsType[0] = RESOURCE;
		break;
	case RESOURCE_ARTIFACT:
		itemsType[1] = RESOURCE;
		itemsType[0] = ARTIFACT_TYPE;
		break;
	case ARTIFACT_RESOURCE:
		itemsType[1] = ARTIFACT_INSTANCE;
		itemsType[0] = RESOURCE;
		break;
	case CREATURE_EXP:
		itemsType[1] = CREATURE;
		itemsType[0] = CREATURE_PLACEHOLDER;
		break;
	case ARTIFACT_EXP:
		itemsType[1] = ARTIFACT_TYPE;
		itemsType[0] = ARTIFACT_PLACEHOLDER;
		break;
	}
}

void CTradeWindow::initItems(bool Left)
{
	if(Left && (itemsType[1] == ARTIFACT_TYPE || itemsType[1] == ARTIFACT_INSTANCE))
	{
		int xOffset = 0, yOffset = 0;
		if(mode == ARTIFACT_RESOURCE)
		{
			xOffset = -361;
			yOffset = +46;

			CTradeableItem *hlp = new CTradeableItem(itemsType[Left], -1, 1, 0);
			hlp->recActions &= ~(UPDATE | SHOWALL);
			hlp->pos += Rect(137, 469, 42, 42);
			items[Left].push_back(hlp);
		}
		else //ARTIFACT_EXP
		{
			xOffset = -363;
			yOffset = -12;
		}

		BLOCK_CAPTURING;
		arts = new CArtifactsOfHero(Point(pos.x+xOffset, pos.y+yOffset));
		arts->commonInfo = new CArtifactsOfHero::SCommonPart;
		arts->commonInfo->participants.insert(arts);
		arts->recActions = 255;
		arts->setHero(hero);
		arts->allowedAssembling = false;
		addChild(arts);
		artSets.push_back(arts);

		if(mode == ARTIFACT_RESOURCE)
			arts->highlightModeCallback = boost::bind(&CTradeWindow::artifactSelected, this, _1);
		return;
	}

	std::vector<int> *ids = getItemsIds(Left);
	std::vector<Rect> pos;
	int amount = -1;

	getPositionsFor(pos, Left, itemsType[Left]);

	if(Left || !ids)
		amount = 7;
	else 
		amount = ids->size();

	if(ids)
		amin(amount, ids->size());

	for(int j=0; j<amount; j++)
	{
		int id = (ids && ids->size()>j) ? (*ids)[j] : j;
		if(id < 0 && mode != ARTIFACT_EXP)  //when sacrificing artifacts we need to prepare empty slots
			continue;

		CTradeableItem *hlp = new CTradeableItem(itemsType[Left], id, Left, j);
		hlp->pos = pos[j] + hlp->pos;
		items[Left].push_back(hlp);
	}

	initSubs(Left);
}

std::vector<int> *CTradeWindow::getItemsIds(bool Left)
{
	std::vector<int> *ids = NULL;

	if(mode == ARTIFACT_EXP)
		return new std::vector<int>(22, -1);

	if(Left)
	{
		switch(itemsType[1])
		{
		case CREATURE:
			ids = new std::vector<int>;
			for(int i = 0; i < 7; i++)
			{
				if(const CCreature *c = hero->getCreature(i))
					ids->push_back(c->idNumber);
				else
					ids->push_back(-1);
			}
			break;
		}
	}
	else
	{
		switch(itemsType[0])
		{
		case PLAYER:
			ids = new std::vector<int>;
			for(int i = 0; i < PLAYER_LIMIT; i++)
				if(i != LOCPLINT->playerID && LOCPLINT->cb->getPlayerStatus(i) == PlayerState::INGAME)
					ids->push_back(i);
			break;

		case ARTIFACT_TYPE:
			ids = new std::vector<int>(market->availableItemsIds(mode));
			break;
		}
	}

	return ids;
}

void CTradeWindow::getPositionsFor(std::vector<Rect> &poss, bool Left, EType type) const
{
	if(mode == ARTIFACT_EXP && !Left)
	{
		//22 boxes, 5 in row, last row: two boxes centered
		int h, w, x, y, dx, dy;
		h = w = 44;
		x = 317;
		y = 53;
		dx = 54;
		dy = 70;
		for (int i = 0; i < 4 ; i++)
			for (int j = 0; j < 5 ; j++)
				poss += Rect(x + dx*j, y + dy*i, w, h);

		poss += Rect(x + dx*1.5, y + dy*4, w, h);
		poss += Rect(x + dx*2.5, y + dy*4, w, h);
	}
	else
	{
		//seven boxes:
		//  X  X  X
		//  X  X  X
		//     X   
		int h, w, x, y, dx, dy;
		int leftToRightOffset;
		getBaseForPositions(type, dx, dy, x, y, h, w, !Left, leftToRightOffset);

		poss += genRect(h, w, x, y), genRect(h, w, x + dx, y), genRect(h, w, x + 2*dx, y),
			genRect(h, w, x, y + dy), genRect(h, w, x + dx, y + dy), genRect(h, w, x + 2*dx, y + dy),
			genRect(h, w, x + dx, y + 2*dy);

		if(!Left)
		{
			BOOST_FOREACH(Rect &r, poss)
				r.x += leftToRightOffset;
		}
	}
}

void CTradeWindow::initSubs(bool Left)
{
	BOOST_FOREACH(CTradeableItem *t, items[Left])
	{
		if(Left)
		{
			switch(itemsType[1])
			{
			case CREATURE:
				t->subtitle = boost::lexical_cast<std::string>(hero->getStackCount(t->serial));
				break;
			case RESOURCE:
				t->subtitle = boost::lexical_cast<std::string>(LOCPLINT->cb->getResourceAmount(t->serial));
				break;
			}
		}
		else //right side
		{
			if(itemsType[0] == PLAYER)
			{
				t->subtitle = CGI->generaltexth->capColors[t->id];
			}
			else if(hLeft)//artifact, creature
			{
				int h1, h2; //hlp variables for getting offer
				market->getOffer(hLeft->id, t->id, h1, h2, mode);
				if(t->id != hLeft->id || mode != RESOURCE_RESOURCE) //don't allow exchanging same resources
				{
					std::ostringstream oss;
					oss << h2;
					if(h1!=1)
						oss << "/" << h1;
					t->subtitle = oss.str();
				}
				else
					t->subtitle = CGI->generaltexth->allTexts[164]; // n/a
			}
			else
				t->subtitle = "";
		}
	}
}

void CTradeWindow::showAll(SDL_Surface * to)
{
	CIntObject::showAll(to);

	if(hRight)
		CSDL_Ext::drawBorder(to,hRight->pos.x-1,hRight->pos.y-1,hRight->pos.w+2,hRight->pos.h+2,int3(255,231,148));
	if(hLeft && hLeft->type != ARTIFACT_INSTANCE)
		CSDL_Ext::drawBorder(to,hLeft->pos.x-1,hLeft->pos.y-1,hLeft->pos.w+2,hLeft->pos.h+2,int3(255,231,148));

	if(readyToTrade)
	{
		hLeft->showAllAt(pos.topLeft() + selectionOffset(true), selectionSubtitle(true), to);
		hRight->showAllAt(pos.topLeft() + selectionOffset(false), selectionSubtitle(false), to);
	}
}

void CTradeWindow::removeItems(const std::set<CTradeableItem *> &toRemove)
{
	BOOST_FOREACH(CTradeableItem *t, toRemove)
		removeItem(t);
}

void CTradeWindow::removeItem(CTradeableItem * t)
{
	if(active)
		t->deactivate();
	items[t->left] -= t;
	delChild(t);

	if(hRight == t)
	{
		hRight = NULL;
		selectionChanged(false);
	}
}

void CTradeWindow::getEmptySlots(std::set<CTradeableItem *> &toRemove)
{
	BOOST_FOREACH(CTradeableItem *t, items[1])
		if(!hero->getStackCount(t->serial))
			toRemove.insert(t);
}

void CTradeWindow::setMode(EMarketMode Mode)
{
	const IMarket *m = market;
	const CGHeroInstance *h = hero;
	CTradeWindow *nwindow = NULL;

	GH.popIntTotally(this);
	
	switch(Mode)
	{
	case CREATURE_EXP:
	case ARTIFACT_EXP:
		nwindow = new CAltarWindow(m, h, Mode);
		break;
	default:
		nwindow = new CMarketplaceWindow(m, h, Mode);
		break;
	}

	GH.pushInt(nwindow);
}

void CTradeWindow::artifactSelected(CArtPlace *slot)
{
	assert(mode == ARTIFACT_RESOURCE);
	items[1][0]->setArtInstance(slot->ourArt);
	if(slot->ourArt)
		hLeft = items[1][0];
	else
		hLeft = NULL;

	selectionChanged(true);
}

CMarketplaceWindow::CMarketplaceWindow(const IMarket *Market, const CGHeroInstance *Hero, EMarketMode Mode)
	: CTradeWindow(Market, Hero, Mode)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL;
	madeTransaction = false;
	std::string bgName;
	bool sliderNeeded = true;

	switch(Mode)
	{
	case RESOURCE_RESOURCE:
		bgName = "TPMRKRES.bmp";
		break;

	case RESOURCE_PLAYER:
		bgName = "TPMRKPTS.bmp";
		break;

	case CREATURE_RESOURCE:
		bgName = "TPMRKCRS.bmp";
		break;

	case RESOURCE_ARTIFACT:
		bgName = "TPMRKABS.bmp";
		sliderNeeded = false;
		break;

	case ARTIFACT_RESOURCE:
		bgName = "TPMRKASS.bmp";
		sliderNeeded = false;
		break;
	}

	bg = new CPicture(bgName);
	bg->colorizeAndConvert(LOCPLINT->playerID);
	pos = bg->center();

	new CGStatusBar(302, 576);

	if(market->o->ID == 7) //black market
	{
		printAtMiddle(CGI->generaltexth->allTexts[349],300,27,FONT_BIG,tytulowy,*bg); //title
	}
	else if(market->o->ID == 99 || market->o->ID == 221) //trading post
	{
		printAtMiddle(CGI->generaltexth->allTexts[159],300,27,FONT_BIG,tytulowy,*bg); //title
	}
	else if(mode == CREATURE_RESOURCE)
	{
		if(market->o->ID == TOWNI_TYPE)
			printAtMiddle(CGI->buildh->buildings[6][21]->Name(), 300, 27, FONT_BIG, tytulowy, *bg); //title
		else
			printAtMiddle(market->o->getHoverText(), 300, 27, FONT_BIG, tytulowy, *bg); //title
	}
	else if(mode == RESOURCE_ARTIFACT || mode == ARTIFACT_RESOURCE)
	{
		const std::string &title = market->o->ID == TOWNI_TYPE 
									? CGI->buildh->buildings[market->o->subID][17]->Name()
									: market->o->getHoverText();
		
		printAtMiddle(title, 300, 27, FONT_BIG, tytulowy, *bg); //title
	}
	else
	{
		printAtMiddle(CGI->generaltexth->allTexts[158],300,27,FONT_BIG,tytulowy,*bg); //marketplace
	}

	initItems(false);
	initItems(true);
	
	ok = new AdventureMapButton(CGI->generaltexth->zelp[600],boost::bind(&CGuiHandler::popIntTotally,&GH,this),516,520,"IOK6432.DEF",SDLK_RETURN);
	ok->assignedKeys.insert(SDLK_ESCAPE);
	deal = new AdventureMapButton(CGI->generaltexth->zelp[595],boost::bind(&CMarketplaceWindow::makeDeal,this),307,520,"TPMRKB.DEF");
	deal->block(true);


	//slider and buttons must be created after bg
	if(sliderNeeded)
	{
		slider = new CSlider(231,490,137,0,0,0);
		slider->moved = boost::bind(&CMarketplaceWindow::sliderMoved,this,_1);
		max = new AdventureMapButton(CGI->generaltexth->zelp[596],boost::bind(&CMarketplaceWindow::setMax,this),229,520,"IRCBTNS.DEF");
		max->block(true);
	}
	else
	{
		slider = NULL;
		max = NULL;
		deal->moveBy(Point(-30, 0));
	}

	Rect traderTextRect;

	//left side
	switch(Mode)
	{
	case RESOURCE_RESOURCE:
	case RESOURCE_PLAYER:
	case RESOURCE_ARTIFACT:
		printAtMiddle(CGI->generaltexth->allTexts[270],154,148,FONT_SMALL,zwykly,*bg); //kingdom res.
		break;
	case CREATURE_RESOURCE: 
		printAtMiddle(boost::str(boost::format(CGI->generaltexth->allTexts[272]) % hero->name), 152, 102, FONT_SMALL, zwykly, *bg); //%s's Creatures
		break;
	case ARTIFACT_RESOURCE: 
		printAtMiddle(boost::str(boost::format(CGI->generaltexth->allTexts[271]) % hero->name), 152, 57, FONT_SMALL, zwykly, *bg); //%s's Artifacts
		break;
	}

	//right side
	switch(Mode)
	{
	case RESOURCE_RESOURCE:
	case CREATURE_RESOURCE:
	case RESOURCE_ARTIFACT:
	case ARTIFACT_RESOURCE:
		printAtMiddle(CGI->generaltexth->allTexts[168],445,148,FONT_SMALL,zwykly,*bg); //available for trade
		traderTextRect = Rect(316, 48, 260, 75);
		break;
	case RESOURCE_PLAYER:
		printAtMiddle(CGI->generaltexth->allTexts[169],445,55,FONT_SMALL,zwykly,*bg); //players
		traderTextRect = Rect(28, 48, 260, 75);
		break;
	}

	traderText = new CTextBox("", traderTextRect, 0, FONT_SMALL, CENTER);
	int specialOffset = mode == ARTIFACT_RESOURCE ? 35 : 0; //in selling artifacts mode we need to move res-res and art-res buttons down

	if(printButtonFor(RESOURCE_PLAYER))
		new AdventureMapButton(CGI->generaltexth->zelp[612],boost::bind(&CMarketplaceWindow::setMode,this, RESOURCE_PLAYER), 18, 520,"TPMRKBU1.DEF");
	if(printButtonFor(RESOURCE_RESOURCE))
		new AdventureMapButton(CGI->generaltexth->zelp[605],boost::bind(&CMarketplaceWindow::setMode,this, RESOURCE_RESOURCE), 516, 450 + specialOffset,"TPMRKBU5.DEF");
	if(printButtonFor(CREATURE_RESOURCE))
		new AdventureMapButton(CGI->generaltexth->zelp[599],boost::bind(&CMarketplaceWindow::setMode,this, CREATURE_RESOURCE), 516, 485,"TPMRKBU4.DEF"); //was y=450, changed to not overlap res-res in some conditions
	if(printButtonFor(RESOURCE_ARTIFACT))
		new AdventureMapButton(CGI->generaltexth->zelp[598],boost::bind(&CMarketplaceWindow::setMode,this, RESOURCE_ARTIFACT), 18, 450 + specialOffset,"TPMRKBU2.DEF");
	if(printButtonFor(ARTIFACT_RESOURCE))																				
		new AdventureMapButton(CGI->generaltexth->zelp[613],boost::bind(&CMarketplaceWindow::setMode,this, ARTIFACT_RESOURCE), 18, 485,"TPMRKBU3.DEF"); //was y=450, changed to not overlap res-art in some conditions

	updateTraderText();
}

CMarketplaceWindow::~CMarketplaceWindow()
{
	hLeft = hRight = NULL;
	for(int i=0;i<items[1].size();i++)
		delChild(items[1][i]);
	for(int i=0;i<items[0].size();i++)
		delChild(items[0][i]);

	items[1].clear();
	items[0].clear();
	delChild(bg);
	bg = NULL;
}



void CMarketplaceWindow::setMax()
{
	slider->moveToMax();
}

void CMarketplaceWindow::makeDeal()
{
	int sliderValue = 0;
	if(slider)
		sliderValue = slider->value;
	else	
		sliderValue = !deal->isBlocked(); //should always be 1

	if(!sliderValue)
		return;

	int leftIdToSend = -1;
	if(mode == CREATURE_RESOURCE)
		leftIdToSend = hLeft->serial;
	else if(mode == ARTIFACT_RESOURCE)
		leftIdToSend = hLeft->getArtInstance()->id;
	else
		leftIdToSend = hLeft->id;

	if(slider)
	{
		LOCPLINT->cb->trade(market->o, mode, leftIdToSend, hRight->id, slider->value*r1, hero);
		slider->moveTo(0);
	}
	else
	{
		LOCPLINT->cb->trade(market->o, mode, leftIdToSend, hRight->id, r2, hero);
	}
	madeTransaction = true;

	hLeft = NULL;
	hRight = NULL;
	selectionChanged(true);
}

void CMarketplaceWindow::sliderMoved( int to )
{
	redraw();
}

void CMarketplaceWindow::selectionChanged(bool side)
{
	readyToTrade = hLeft && hRight;
	if(mode == RESOURCE_RESOURCE)
		readyToTrade = readyToTrade && (hLeft->id != hRight->id); //for resource trade, two DIFFERENT resources must be selected 

 	if(mode == ARTIFACT_RESOURCE && !hLeft) 
		arts->unmarkSlots(false);
 
	if(readyToTrade)
	{
		int soldItemId = hLeft->id;
		market->getOffer(soldItemId, hRight->id, r1, r2, mode);

		if(slider)
		{
			int newAmount = -1;
			if(itemsType[1] == RESOURCE)
				newAmount = LOCPLINT->cb->getResourceAmount(soldItemId);
			else if(itemsType[1] ==  CREATURE)
				newAmount = hero->getStackCount(hLeft->serial) - (hero->Slots().size() == 1  &&  hero->needsLastStack());
			else
				assert(0);

			slider->setAmount(newAmount / r1);
			slider->moveTo(0);
			max->block(false);
			deal->block(false);
		}
		else if(itemsType[1] == RESOURCE) //buying -> check if we can afford transaction
		{
			deal->block(LOCPLINT->cb->getResourceAmount(soldItemId) < r1);
		}
		else
			deal->block(false);
	}
	else
	{
		if(slider)
		{
			max->block(true);
			slider->setAmount(0);
			slider->moveTo(0);
		}
		deal->block(true);
	}

	if(side && itemsType[0] != PLAYER) //items[1] selection changed, recalculate offers
		initSubs(false);

	updateTraderText();
	redraw();
}

bool CMarketplaceWindow::printButtonFor(EMarketMode M) const
{
	return market->allowsTrade(M) && M != mode && (hero || ( M != CREATURE_RESOURCE && M != RESOURCE_ARTIFACT && M != ARTIFACT_RESOURCE ));
}

void CMarketplaceWindow::garrisonChanged()
{
	if(mode != CREATURE_RESOURCE)
		return;

	std::set<CTradeableItem *> toRemove;
	getEmptySlots(toRemove);


	removeItems(toRemove);
	initSubs(true);
}

void CMarketplaceWindow::artifactsChanged(bool Left)
{
	assert(!Left);
	if(mode != RESOURCE_ARTIFACT)
		return;

	std::vector<int> available = market->availableItemsIds(mode);
	std::set<CTradeableItem *> toRemove;
	BOOST_FOREACH(CTradeableItem *t, items[0])
		if(!vstd::contains(available, t->id))
			toRemove.insert(t);

	removeItems(toRemove);
	redraw();
}

std::string CMarketplaceWindow::selectionSubtitle(bool Left) const
{
	if(Left)
	{
		switch(itemsType[1])
		{
		case RESOURCE:
		case CREATURE:
			{
				int val = slider 
					? slider->value * r1 
					: (((deal->isBlocked())) ? 0 : r1);

				return boost::lexical_cast<std::string>(val);
			}
		case ARTIFACT_INSTANCE:
			return ((deal->isBlocked()) ? "0" : "1");
		}
	}
	else
	{
		switch(itemsType[0])
		{
		case RESOURCE:
			if(slider)
				return boost::lexical_cast<std::string>( slider->value * r2 );
			else
				return boost::lexical_cast<std::string>(r2);
		case ARTIFACT_TYPE:
			return ((deal->isBlocked()) ? "0" : "1");
		case PLAYER:
			return (hRight ? CGI->generaltexth->capColors[hRight->id] : "");
		}
	}

	return "???";
}

Point CMarketplaceWindow::selectionOffset(bool Left) const
{
	if(Left)
	{
		switch(itemsType[1])
		{
		case RESOURCE:
			return Point(122, 446);
		case CREATURE:
			return Point(128, 450);
		case ARTIFACT_INSTANCE:
			return Point(134, 466);
		}
	}
	else
	{
		switch(itemsType[0])
		{
		case RESOURCE:
			if(mode == ARTIFACT_RESOURCE)
				return Point(410, 469);
			else
				return Point(410, 446);
		case ARTIFACT_TYPE:
			return Point(425, 447);
		case PLAYER:
			return Point(417, 451);
		}
	}

	assert(0);
	return Point(0,0);
}

void CMarketplaceWindow::resourceChanged(int type, int val)
{
	initSubs(true);
}

void CMarketplaceWindow::getBaseForPositions(EType type, int &dx, int &dy, int &x, int &y, int &h, int &w, bool Right, int &leftToRightOffset) const
{
	switch(type)
	{
	case RESOURCE:
		dx = 82;
		dy = 79;
		x = 39;
		y = 180;
		h = 66;
		w = 74;
		break;
	case PLAYER:
		dx = 83;
		dy = 118;
		h = 64;
		w = 58;
		x = 44;
		y = 83;
		assert(Right);
		break;
	case CREATURE://45,123
		x = 45;
		y = 123;
		w = 58;
		h = 64;
		dx = 83;
		dy = 98;
		assert(!Right);
		break;
	case ARTIFACT_TYPE://45,123
		x = 340-289;
		y = 180;
		w = 44;
		h = 44;
		dx = 83;
		dy = 79;
		break;
	}

	leftToRightOffset = 289;
}

void CMarketplaceWindow::updateTraderText()
{
	if(readyToTrade)
	{
		if(mode == RESOURCE_PLAYER)
		{
			//I can give %s to the %s player.
			traderText->setTxt(boost::str(boost::format(CGI->generaltexth->allTexts[165]) % hLeft->getName() % hRight->getName()));
		}
		else if(mode == RESOURCE_ARTIFACT)
		{
			//I can offer you the %s for %d %s of %s.
			traderText->setTxt(boost::str(boost::format(CGI->generaltexth->allTexts[267]) % hRight->getName() % r1 % CGI->generaltexth->allTexts[160 + (r1==1)] % hLeft->getName()));
		}
		else if(mode == RESOURCE_RESOURCE)
		{
			//I can offer you %d %s of %s for %d %s of %s.
			traderText->setTxt(boost::str(boost::format(CGI->generaltexth->allTexts[157]) % r2 % CGI->generaltexth->allTexts[160 + (r2==1)] % hRight->getName() % r1 % CGI->generaltexth->allTexts[160 + (r1==1)] % hLeft->getName()));
		}
		else if(mode == CREATURE_RESOURCE)
		{
			//I can offer you %d %s of %s for %d %s.
			traderText->setTxt(boost::str(boost::format(CGI->generaltexth->allTexts[269]) % r2 % CGI->generaltexth->allTexts[160 + (r2==1)] % hRight->getName() % r1 % hLeft->getName(r1)));
		}
		else if(mode == ARTIFACT_RESOURCE)
		{
			//I can offer you %d %s of %s for your %s.
			traderText->setTxt(boost::str(boost::format(CGI->generaltexth->allTexts[268]) % r2 % CGI->generaltexth->allTexts[160 + (r2==1)] % hRight->getName() % hLeft->getName(r1)));
		}
		return;
	}

	int gnrtxtnr = -1;
	if(madeTransaction)
	{
		if(mode == RESOURCE_PLAYER)
			gnrtxtnr = 166; //Are there any other resources you'd like to give away?
		else
			gnrtxtnr = 162; //You have received quite a bargain.  I expect to make no profit on the deal.  Can I interest you in any of my other wares?
	}
	else
	{
		if(mode == RESOURCE_PLAYER)
			gnrtxtnr = 167; //If you'd like to give any of your resources to another player, click on the item you wish to give and to whom.
		else
			gnrtxtnr = 163; //Please inspect our fine wares.  If you feel like offering a trade, click on the items you wish to trade with and for.
	}
	traderText->setTxt(CGI->generaltexth->allTexts[gnrtxtnr]);
}

CAltarWindow::CAltarWindow(const IMarket *Market, const CGHeroInstance *Hero /*= NULL*/, EMarketMode Mode)
	:CTradeWindow(Market, Hero, Mode)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL;
	bg = new CPicture(Mode == CREATURE_EXP ? "ALTARMON.bmp" : "ALTRART2.bmp");
	bg->colorizeAndConvert(LOCPLINT->playerID);
	pos = bg->center();




	if(Mode == CREATURE_EXP)
	{
		printAtMiddle(boost::str(boost::format(CGI->generaltexth->allTexts[272]) % hero->name), 155, 30, FONT_SMALL, tytulowy, *bg); //%s's Creatures
		printAtMiddle(CGI->generaltexth->allTexts[479], 450, 30, FONT_SMALL, tytulowy, *bg); //Altar of Sacrifice
		printAtMiddleWB(CGI->generaltexth->allTexts[480], 450, 70, FONT_SMALL, 45, tytulowy, *bg); //To sacrifice creatures, move them from your army on to the Altar and click Sacrifice

		slider = new CSlider(231,481,137,0,0,0);
		slider->moved = boost::bind(&CAltarWindow::sliderMoved,this,_1);
		max = new AdventureMapButton(CGI->generaltexth->zelp[578],boost::bind(&CSlider::moveToMax, slider),147,520,"IRCBTNS.DEF");

		sacrificedUnits.resize(ARMY_SIZE, 0);
		sacrificeAll = new AdventureMapButton(CGI->generaltexth->zelp[579],boost::bind(&CAltarWindow::SacrificeAll,this),393,520,"ALTARMY.DEF");
		sacrificeBackpack = NULL;

		initItems(true);
		mimicCres();
	}
	else
	{
		printAtMiddle(CGI->generaltexth->allTexts[477], 450, 34, FONT_SMALL, tytulowy, *bg); //Sacrifice artifacts for experience
		printAtMiddle(CGI->generaltexth->allTexts[478], 302, 423, FONT_SMALL, tytulowy, *bg); //%s's Creatures

		sacrificeAll = new AdventureMapButton(CGI->generaltexth->zelp[571],boost::bind(&CAltarWindow::SacrificeAll,this),393,520,"ALTFILL.DEF");
		sacrificeAll->block(!hero->artifactsInBackpack.size() && !hero->artifactsWorn.size());
		sacrificeBackpack = new AdventureMapButton(CGI->generaltexth->zelp[570],boost::bind(&CAltarWindow::SacrificeBackpack,this),147,520,"ALTEMBK.DEF");
		sacrificeBackpack->block(!hero->artifactsInBackpack.size());

		slider = NULL;
		max = NULL;
		
		initItems(true);
		initItems(false);
	}

	printAtMiddleWB(CGI->generaltexth->allTexts[475], 72, 437, FONT_SMALL, 17, tytulowy, *bg); //Experience needed to reach next level
	printAtMiddleWB(CGI->generaltexth->allTexts[476], 72, 505, FONT_SMALL, 17, tytulowy, *bg); //Total experience on the Altar

	new CGStatusBar(302, 576);
	ok = new AdventureMapButton(CGI->generaltexth->zelp[568],boost::bind(&CGuiHandler::popIntTotally,&GH,this),516,520,"IOK6432.DEF",SDLK_RETURN);
	ok->assignedKeys.insert(SDLK_ESCAPE);

	deal = new AdventureMapButton(CGI->generaltexth->zelp[585],boost::bind(&CAltarWindow::makeDeal,this),269,520,"ALTSACR.DEF");

	if(Hero->getAlignment() != EVIL && Mode == CREATURE_EXP)
		new AdventureMapButton(CGI->generaltexth->zelp[580], boost::bind(&CTradeWindow::setMode,this, ARTIFACT_EXP), 516, 421, "ALTART.DEF");
	if(Hero->getAlignment() != GOOD && Mode == ARTIFACT_EXP)
		new AdventureMapButton(CGI->generaltexth->zelp[572], boost::bind(&CTradeWindow::setMode,this, CREATURE_EXP), 516, 421, "ALTSACC.DEF");

	expPerUnit.resize(ARMY_SIZE, 0);
	getExpValues();

	expToLevel = new CLabel(73, 475, FONT_SMALL, CENTER);
	expOnAltar = new CLabel(73, 543, FONT_SMALL, CENTER);

	setExpToLevel();
	calcTotalExp();
	blockTrade();
}

CAltarWindow::~CAltarWindow()
{

}

void CAltarWindow::getBaseForPositions(EType type, int &dx, int &dy, int &x, int &y, int &h, int &w, bool Right, int &leftToRightOffset) const
{
	leftToRightOffset = 289;
	x = 45;
	y = 110;
	w = 58;
	h = 64;
	dx = 83;
	dy = 98;
}

void CAltarWindow::sliderMoved(int to)
{
	sacrificedUnits[hLeft->serial] = to;
	updateRight(hRight);
	deal->block(!to);
	calcTotalExp();
	redraw();
}

void CAltarWindow::makeDeal()
{
	if(mode == CREATURE_EXP)
	{
		blockTrade();
		slider->value = 0;

		std::vector<int> toSacrifice = sacrificedUnits;
		for (int i = 0; i < toSacrifice.size(); i++)
		{
			if(toSacrifice[i])
				LOCPLINT->cb->trade(market->o, mode, i, 0, toSacrifice[i], hero);
		}

		BOOST_FOREACH(int& val, sacrificedUnits)
			val = 0;

		BOOST_FOREACH(CTradeableItem *t, items[0])
		{
			t->type = CREATURE_PLACEHOLDER;
			t->subtitle = "";
		}
	}
	else
	{
		BOOST_FOREACH(const CArtifactInstance *art, arts->artifactsOnAltar) //sacrifice each artifact on the list
		{
			LOCPLINT->cb->trade(market->o, mode, hero->getArtPos(art), -1, 1, hero);
		}
		arts->artifactsOnAltar.clear();

		BOOST_FOREACH(CTradeableItem *t, items[0])
		{
			t->id = -1;
			t->subtitle = "";
		}

		arts->commonInfo->reset();
		//arts->scrollBackpack(0);
		deal->block(true);
	}

	calcTotalExp();
}

void CAltarWindow::SacrificeAll()
{
	if(mode == CREATURE_EXP)
	{
		bool movedAnything = false;
		BOOST_FOREACH(CTradeableItem *t, items[1])
			sacrificedUnits[t->serial] = hero->getStackCount(t->serial);

		sacrificedUnits[items[1].front()->serial]--;

		BOOST_FOREACH(CTradeableItem *t, items[0])
		{
			updateRight(t);
			if(t->type == CREATURE)
				movedAnything = true;
		}
		
		deal->block(!movedAnything);
		calcTotalExp();
	}
	else
	{
		for(std::map<ui16, ArtSlotInfo>::const_iterator i = hero->artifactsWorn.begin(); i != hero->artifactsWorn.end(); i++)
		{
			if(i->second.artifact->artType->id != 145) //ignore locks from assembled artifacts
				moveFromSlotToAltar(i->first, NULL, i->second.artifact);
		}

		SacrificeBackpack();
	}
	redraw();
}

void CAltarWindow::selectionChanged(bool side)
{
	if(mode != CREATURE_EXP)
		return;

	CTradeableItem *&selected = side ? hLeft : hRight;
	CTradeableItem *&theOther = side ? hRight : hLeft;

	theOther = *std::find_if(items[!side].begin(), items[!side].end(), boost::bind(&CTradeableItem::serial, _1) == selected->serial);

	int stackCount = 0;
	for (int i = 0; i < ARMY_SIZE; i++)
		if(hero->getStackCount(i) > sacrificedUnits[i])
			stackCount++;

	slider->setAmount(hero->getStackCount(hLeft->serial) - (stackCount == 1));
	slider->block(!slider->amount);
	slider->value = sacrificedUnits[hLeft->serial];
	max->block(!slider->amount);
	readyToTrade = true;
	redraw();
}

void CAltarWindow::mimicCres()
{
	std::vector<Rect> positions;
	getPositionsFor(positions, false, CREATURE);

	BOOST_FOREACH(CTradeableItem *t, items[1])
	{
		CTradeableItem *hlp = new CTradeableItem(CREATURE_PLACEHOLDER, t->id, false, t->serial);
		hlp->pos = positions[t->serial] + hlp->pos;
		items[0].push_back(hlp);
	}
}

Point CAltarWindow::selectionOffset(bool Left) const
{
	if(Left)
		return Point(150, 421);
	else
		return Point(396, 421);
}

std::string CAltarWindow::selectionSubtitle(bool Left) const
{
	if(Left && slider && hLeft)
		return boost::lexical_cast<std::string>(slider->value);
	else if(!Left && hRight)
		return hRight->subtitle;
	else
		return "";
}

void CAltarWindow::artifactsChanged(bool left)
{

}

void CAltarWindow::garrisonChanged()
{
	if(mode != CREATURE_EXP)
		return;

	std::set<CTradeableItem *> empty;
	getEmptySlots(empty);

	BOOST_FOREACH(CTradeableItem *t, empty)
	{
		removeItem(*std::find_if(items[0].begin(), items[0].end(), boost::bind(&CTradeableItem::serial, _1) == t->serial));
		removeItem(t);
	}

	initSubs(true);
	getExpValues();
}

void CAltarWindow::getExpValues()
{
	int dump;
	BOOST_FOREACH(CTradeableItem *t, items[1])
		if(t->id >= 0)
			market->getOffer(t->id, 0, dump, expPerUnit[t->serial], CREATURE_EXP);
}

void CAltarWindow::calcTotalExp()
{
	int val = 0;
	if(mode == CREATURE_EXP)
	{
		for (int i = 0; i < sacrificedUnits.size(); i++)
		{
			val += expPerUnit[i] * sacrificedUnits[i];
		}
	}
	else
	{
		BOOST_FOREACH(const CArtifactInstance *art, arts->artifactsOnAltar)
		{
			int dmp, valOfArt;
			market->getOffer(art->artType->id, 0, dmp, valOfArt, mode);
			val += valOfArt; //WAS val += valOfArt * arts->artifactsOnAltar.count(*i);
		}
	}
	val *=(100+hero->getSecSkillLevel(CGHeroInstance::LEARNING)*5)/100.0f;
	expOnAltar->setTxt(boost::lexical_cast<std::string>(val));
}

void CAltarWindow::setExpToLevel()
{
	expToLevel->setTxt(boost::lexical_cast<std::string>(CGI->heroh->reqExp(CGI->heroh->level(hero->exp)+1) - hero->exp));
}

void CAltarWindow::blockTrade()
{
	hLeft = hRight = NULL;
	readyToTrade = false;
	if(slider)
	{
		slider->block(true);
		max->block(true);
	}
	deal->block(true);
}

void CAltarWindow::updateRight(CTradeableItem *toUpdate)
{
	int val = sacrificedUnits[toUpdate->serial];
	toUpdate->type = val ? CREATURE : CREATURE_PLACEHOLDER;
	toUpdate->subtitle = val ? boost::str(boost::format(CGI->generaltexth->allTexts[122]) % boost::lexical_cast<std::string>(val * expPerUnit[toUpdate->serial])) : ""; //%s exp
}

int CAltarWindow::firstFreeSlot()
{
	int ret = -1;
	while(items[0][++ret]->id >= 0  &&  ret + 1 < items[0].size());
	return ret < items[0].size() ? ret : -1;
}

void CAltarWindow::SacrificeBackpack()
{
	std::multiset<const CArtifactInstance *> toOmmit = arts->artifactsOnAltar;

	for (int i = 0; i < hero->artifactsInBackpack.size(); i++)
	{

		if(vstd::contains(toOmmit, hero->artifactsInBackpack[i].artifact))
		{
			toOmmit -= hero->artifactsInBackpack[i].artifact;
			continue;
		}

		putOnAltar(NULL, hero->artifactsInBackpack[i].artifact);
	}

	arts->scrollBackpack(0);
	calcTotalExp();
}

void CAltarWindow::artifactPicked()
{
	redraw();
}

void CAltarWindow::showAll(SDL_Surface * to)
{
	CTradeWindow::showAll(to);
	if(mode == ARTIFACT_EXP && arts && arts->commonInfo->src.art)
	{
		blitAtLoc(graphics->artDefs->ourImages[arts->commonInfo->src.art->artType->id].bitmap, 281, 442, to);

		int dmp, val;
		market->getOffer(arts->commonInfo->src.art->artType->id, 0, dmp, val, ARTIFACT_EXP);
		printAtMiddleLoc(boost::lexical_cast<std::string>(val), 304, 498, FONT_SMALL, zwykly, to);
	}
}

bool CAltarWindow::putOnAltar(CTradeableItem* altarSlot, const CArtifactInstance *art)
{
	int artID = art->artType->id;
	if(artID != 1 && artID < 7) //special art
	{
		tlog2 << "Cannot put special artifact on altar!\n";
		return false;
	}

	if(!altarSlot)
	{
		int slotIndex = firstFreeSlot();
		if(slotIndex < 0)
		{
			tlog2 << "No free slots on altar!\n";
			return false;
		}
		altarSlot = items[0][slotIndex];
	}

	int dmp, val;
	market->getOffer(artID, 0, dmp, val, ARTIFACT_EXP);

	arts->artifactsOnAltar.insert(art);
	altarSlot->setArtInstance(art);
	altarSlot->subtitle = boost::lexical_cast<std::string>(val);

	deal->block(false);
	return true;
}

void CAltarWindow::moveFromSlotToAltar(int slotID, CTradeableItem* altarSlot, const CArtifactInstance *art)
{
	int freeBackpackSlot = hero->artifactsInBackpack.size() + Arts::BACKPACK_START;
	if(arts->commonInfo->src.art)
	{
		arts->commonInfo->dst.slotID = freeBackpackSlot;
		arts->commonInfo->dst.AOH = arts;
	}

	if(putOnAltar(altarSlot, art))
	{
		if(slotID < Arts::BACKPACK_START)
			LOCPLINT->cb->swapArtifacts(hero, slotID, hero, freeBackpackSlot);
		else
		{
			arts->commonInfo->src.clear();
			arts->commonInfo->dst.clear();
			CCS->curh->dragAndDropCursor(NULL);
			arts->unmarkSlots(false);
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
	save->swappedImages = true;
	save->update();

	// restart = new AdventureMapButton (CGI->generaltexth->zelp[323].first, CGI->generaltexth->zelp[323].second, boost::bind(&CSystemOptionsWindow::bmainmenuf, this), pos.x+346, pos.y+357, "SORSTRT", SDLK_r);
	// std::swap(save->imgs[0][0], restart->imgs[0][1]);

	mainMenu = new AdventureMapButton (CGI->generaltexth->zelp[320].first, CGI->generaltexth->zelp[320].second, boost::bind(&CSystemOptionsWindow::bmainmenuf, this), pos.x+357, pos.y+357, "SOMAIN.DEF", SDLK_m);
	mainMenu->swappedImages = true;
	mainMenu->update();

	quitGame = new AdventureMapButton (CGI->generaltexth->zelp[324].first, CGI->generaltexth->zelp[324].second, boost::bind(&CSystemOptionsWindow::bquitf, this), pos.x+246, pos.y+415, "soquit.def", SDLK_q);
	quitGame->swappedImages = true;
	quitGame->update();
	backToMap = new AdventureMapButton (CGI->generaltexth->zelp[325].first, CGI->generaltexth->zelp[325].second, boost::bind(&CSystemOptionsWindow::breturnf, this), pos.x+357, pos.y+415, "soretrn.def", SDLK_RETURN);
	backToMap->swappedImages = true;
	backToMap->update();
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
	musicVolume->select(CCS->musich->getVolume(), 1);
	musicVolume->onChange = boost::bind(&SystemOptions::setMusicVolume, &owner->sysOpts, _1);

	effectsVolume = new CHighlightableButtonsGroup(0, true);
	for(int i=0; i<10; ++i)
	{
		effectsVolume->addButton(boost::assign::map_list_of(0,CGI->generaltexth->zelp[336+i].second),CGI->generaltexth->zelp[336+i].second, "syslb.def", pos.x+29 + 19*i, pos.y+425, i*11);
	}
	effectsVolume->select(CCS->soundh->getVolume(), 1);
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
	GH.pushInt(new CSavingScreen(CPlayerInterface::howManyPeople > 1));
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
	CSDL_Ext::blitSurface(background, NULL, to, &pos);

	save->showAll(to);
	quitGame->showAll(to);
	backToMap->showAll(to);
	mainMenu->showAll(to);
	heroMoveSpeed->showAll(to);
	mapScrollSpeed->showAll(to);
	musicVolume->showAll(to);
	effectsVolume->showAll(to);
}

CTavernWindow::CTavernWindow(const CGObjectInstance *TavernObj)
 : tavernObj(TavernObj)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL;
	std::vector<const CGHeroInstance*> h = LOCPLINT->cb->getAvailableHeroes(TavernObj);
	assert(h.size() == 2);

	h1 = new HeroPortrait(selected,0,72,299,h[0]);
	h2 = new HeroPortrait(selected,1,162,299,h[1]);
	if(h[0])
		selected = 0;
	else
		selected = -1;
	oldSelected = -1;

	bg = new CPicture("TPTAVERN.bmp");
	bg->colorizeAndConvert(LOCPLINT->playerID);
	pos = center(bg->pos);


	printAtMiddle(CGI->generaltexth->jktexts[37],200,35,FONT_BIG,tytulowy,*bg);
	printAtMiddle("2500",320,328,FONT_SMALL,zwykly,*bg);
//	printAtMiddle(CGI->generaltexth->jktexts[38],146,283,FONT_BIG,tytulowy,bg); //what is this???
	printAtMiddleWB(LOCPLINT->cb->getTavernGossip(tavernObj), 200, 220, FONT_SMALL, 50, zwykly, *bg);


	bar = new CGStatusBar(8, 478, "APHLFTRT.bmp", 380);
	cancel = new AdventureMapButton(CGI->generaltexth->tavernInfo[7],"", boost::bind(&CTavernWindow::close, this), 310, 428, "ICANCEL.DEF", SDLK_ESCAPE);
	recruit = new AdventureMapButton("", "", boost::bind(&CTavernWindow::recruitb, this), 272, 355, "TPTAV01.DEF", SDLK_RETURN);
	thiefGuild = new AdventureMapButton(CGI->generaltexth->tavernInfo[5],"", boost::bind(&CTavernWindow::thievesguildb, this), 22, 428, "TPTAV02.DEF", SDLK_t);

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
		if(!h[0])
			recruit->block(1);
	}

#ifdef _WIN32
	CCS->videoh->open("TAVERN.BIK");
#else
	CCS->videoh->open("tavern.mjpg", true, false);
#endif
}

void CTavernWindow::recruitb()
{
	const CGHeroInstance *toBuy = (selected ? h2 : h1)->h;
	const CGObjectInstance *obj = tavernObj;
	close();
	LOCPLINT->cb->recruitHero(obj, toBuy);
}

void CTavernWindow::thievesguildb()
{
	GH.pushInt( new CThievesGuildWindow(tavernObj) );
}

CTavernWindow::~CTavernWindow()
{
	CCS->videoh->close();
}

void CTavernWindow::close()
{
	GH.popIntTotally(this);
}

void CTavernWindow::show(SDL_Surface * to)
{
	CIntObject::show(to);

	CCS->videoh->update(pos.x+70, pos.y+56, to, true, false);
	if(selected >= 0)
	{
		HeroPortrait *sel = selected ? h2 : h1;

		if (selected != oldSelected  &&  !recruit->isBlocked()) 
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
	if(previousState && !down && h)
		as();
	//ClickableL::clickLeft(down);
}

void CTavernWindow::HeroPortrait::clickRight(tribool down, bool previousState)
{
	if(down && h)
	{
		GH.pushInt(new CRClickPopupInt(new CHeroWindow(h), true));
	}
}

CTavernWindow::HeroPortrait::HeroPortrait(int &sel, int id, int x, int y, const CGHeroInstance *H)
:as(sel,id), h(H)
{
	used = LCLICK | RCLICK | HOVER;
	h = H;
	pos.x = x;
	pos.y = y;
	pos.w = 58;
	pos.h = 64;

	if(H)
	{
		hoverName = CGI->generaltexth->tavernInfo[4];
		boost::algorithm::replace_first(hoverName,"%s",H->name);

		int artifs = h->artifactsWorn.size() + h->artifactsInBackpack.size();
		for(int i=13; i<=17; i++) //war machines and spellbook don't count
			if(vstd::contains(h->artifactsWorn,i)) 
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
		GH.statusbar->print(hoverName);
	else
		GH.statusbar->clear();
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
	if(GH.topInt() == adventureInt)
	{
		GH.statusbar->print(enteredText);
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
	if(GH.topInt() == adventureInt)
	{
		GH.statusbar->clear();
	}
	else if(LOCPLINT->battleInt)
	{
		LOCPLINT->battleInt->console->ingcAlter = "";
	}

}

void CInGameConsole::refreshEnteredText()
{
	if(GH.topInt() == adventureInt)
	{
		GH.statusbar->print(enteredText);
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

CGarrisonWindow::CGarrisonWindow( const CArmedInstance *up, const CGHeroInstance *down, bool removableUnits )
{
	OBJ_CONSTRUCTION_CAPTURING_ALL;
	bg = new CPicture("GARRISON.bmp");
	bg->colorizeAndConvert(LOCPLINT->playerID);
	pos = bg->center();

	garr = new CGarrisonInt(92, 127, 4, Point(0,96), bg->bg, Point(93,127), up, down, removableUnits);
	{
		AdventureMapButton *split = new AdventureMapButton(CGI->generaltexth->tcommands[3],"",boost::bind(&CGarrisonInt::splitClick,garr),88,314,"IDV6432.DEF");
		removeChild(split);
		garr->addSplitBtn(split);
	}
	quit = new AdventureMapButton(CGI->generaltexth->tcommands[8],"",boost::bind(&CGarrisonWindow::close,this),399,314,"IOK6432.DEF",SDLK_RETURN);
}

CGarrisonWindow::~CGarrisonWindow()
{
}

void CGarrisonWindow::showAll(SDL_Surface * to)
{
	CIntObject::showAll(to);

	std::string title;
	if (garr->armedObjs[1]->tempOwner == garr->armedObjs[0]->tempOwner)
		title = CGI->generaltexth->allTexts[709];
	else
	{
		title = CGI->generaltexth->allTexts[35];
		boost::algorithm::replace_first(title, "%s", garr->armedObjs[0]->Slots().begin()->second->type->namePl);
	}

	blitAtLoc(graphics->flags->ourImages[garr->armedObjs[1]->getOwner()].bitmap,28,124,to);
	blitAtLoc(graphics->portraitLarge[static_cast<const CGHeroInstance*>(garr->armedObjs[1])->portrait],29,222,to);
	printAtMiddleLoc(title,275,30,FONT_BIG,tytulowy,to);
}

IShowActivable::IShowActivable()
{
	type = 0;
}

CGarrisonHolder::CGarrisonHolder()
{
	type |= WITH_GARRISON;
}

void CWindowWithGarrison::updateGarrisons()
{
	garr->recreateSlots();
}

void CRClickPopupInt::show(SDL_Surface * to)
{
	inner->show(to);
}

CRClickPopupInt::CRClickPopupInt( IShowActivable *our, bool deleteInt )
{
	CCS->curh->hide();
	inner = our;
	delInner = deleteInt;
}

CRClickPopupInt::~CRClickPopupInt()
{
// 	//workaround for hero window issue - if it's our interface, call dispose to properly reset it's state 
// 	//TODO? it might be better to rewrite hero window so it will bee newed/deleted on opening / closing (not effort-worthy now, but on some day...?)
// 	if(LOCPLINT && inner == adventureInt->heroWindow)
// 		adventureInt->heroWindow->dispose();

	if(delInner)
		delete inner;

	CCS->curh->show();
}

void CRClickPopupInt::showAll(SDL_Surface * to)
{
	inner->showAll(to);
}

CArtPlace::CArtPlace(const CArtifactInstance* Art)
	:picked(false), marked(false), locked(false), ourArt(Art)
{
}

CArtPlace::CArtPlace(Point position, const CArtifactInstance * Art):
	picked(false), marked(false), locked(false), ourArt(Art)
{
	pos += position;
	pos.w = pos.h = 44;
}

void CArtPlace::activate()
{
	if(!active)
	{
		LRClickableAreaWTextComp::activate();
	}
}

void CArtPlace::clickLeft(tribool down, bool previousState)
{
	//LRClickableAreaWTextComp::clickLeft(down);
	bool inBackpack = slotID >= Arts::BACKPACK_START, 
		srcInBackpack = ourOwner->commonInfo->src.slotID >= Arts::BACKPACK_START,
		srcInSameHero = ourOwner->commonInfo->src.AOH == ourOwner;
	
	if(ourOwner->highlightModeCallback && ourArt)
	{
		if(down)
		{
			if(ourArt->artType->id < 7) //War Machine or Spellbook
			{
				LOCPLINT->showInfoDialog(CGI->generaltexth->allTexts[21]); //This item can't be traded.
			}
			else
			{
				ourOwner->unmarkSlots(false);
				marked = true;
				ourOwner->highlightModeCallback(this);
			}
		}
		return;
	}

	// If clicked on spellbook, open it only if no artifact is held at the moment.
	if(ourArt && !down && previousState && !ourOwner->commonInfo->src.AOH)
	{
		if(ourArt->artType->id == 0)
		{
			CSpellWindow * spellWindow = new CSpellWindow(genRect(595, 620, (conf.cc.resx - 620)/2, (conf.cc.resy - 595)/2), ourOwner->curHero, LOCPLINT, LOCPLINT->battleInt);
			GH.pushInt(spellWindow);
		}
	}

	if (!down && previousState)
	{
		if(ourArt && ourArt->artType->id == 0) //spellbook
			return; //this is handled separately

		if(!ourOwner->commonInfo->src.AOH) //nothing has been clicked
		{
			if(ourArt  //to prevent selecting empty slots (bugfix to what GrayFace reported)
				&&  ourOwner->curHero->tempOwner == LOCPLINT->playerID)//can't take art from another player
			{
				if(ourArt->artType->id == 3) //catapult cannot be highlighted
				{
					std::vector<SComponent *> catapult(1, new SComponent(SComponent::artifact, 3, 0));
					LOCPLINT->showInfoDialog(CGI->generaltexth->allTexts[312], catapult); //The Catapult must be equipped.
					return;
				}
				select();
			}
		}
		else if(ourArt == ourOwner->commonInfo->src.art) //restore previously picked artifact
		{
			deselect();
		}
		else //perform artifact transition
		{
			if(inBackpack) // Backpack destination.
			{
				if(srcInBackpack && slotID == ourOwner->commonInfo->src.slotID + 1) //next slot (our is not visible, so visually same as "old" place) to the art -> make nothing, return artifact to slot
				{
					deselect();
				}
				else
				{
					const CArtifact * const cur = ourOwner->commonInfo->src.art->artType;

					switch(cur->id)
					{
					case 3:
						//should not happen, catapult cannot be selected
						assert(cur->id != 3);
						break;
					case 4: case 5: case 6: //war machines cannot go to backpack
						LOCPLINT->showInfoDialog(boost::str(boost::format(CGI->generaltexth->allTexts[153]) % cur->Name()));
						break;
					default:
						setMeAsDest();
						amin(ourOwner->commonInfo->dst.slotID, ourOwner->curHero->artifactsInBackpack.size() + Arts::BACKPACK_START);
						if(srcInBackpack && srcInSameHero)
						{
							if(!ourArt								//cannot move from backpack to AFTER backpack -> combined with amin above it will guarantee that dest is at most the last artifact
							  || ourOwner->commonInfo->src.slotID < ourOwner->commonInfo->dst.slotID) //rearranging arts in backpack after taking src artifact, the dest id will be shifted
								ourOwner->commonInfo->dst.slotID--;
						}
						if(srcInSameHero && ourOwner->commonInfo->dst.slotID == ourOwner->commonInfo->src.slotID) //we came to src == dst
							deselect();
						else
							ourOwner->realizeCurrentTransaction();
						break;
					}
				}
			}
			//check if swap is possible
			else if (fitsHere(ourOwner->commonInfo->src.art) &&
				(!ourArt || ourOwner->curHero->tempOwner == LOCPLINT->playerID))
			{
				setMeAsDest();
// 
// 				// Special case when the dest artifact can't be fit into the src slot.
// 				//CGI->arth->unequipArtifact(ourOwner->curHero->artifWorn, slotID);
// 				const CArtifactsOfHero* srcAOH = ourOwner->commonInfo->src.AOH;
// 				ui16 srcSlotID = ourOwner->commonInfo->src.slotID;
// 				if (ourArt && srcSlotID < 19 && !ourArt->canBePutAt(ArtifactLocation(srcAOH->curHero, srcSlotID))) 
// 				{
// 					// Put dest artifact into owner's backpack.
// 					ourOwner->commonInfo->src.AOH = ourOwner;
// 					ourOwner->commonInfo->src.slotID = ourOwner->curHero->artifacts.size() + 19;
// 				}

				ourOwner->realizeCurrentTransaction();
			}
		}
	}
}

void CArtPlace::clickRight(tribool down, bool previousState)
{
	if(down && ourArt && !locked && text.size())  //if there is no description or it's a lock, do nothing ;]
	{
		if (slotID < 19) 
		{
			if(ourOwner->allowedAssembling)
			{
				std::vector<const CArtifact *> assemblyPossibilities = ourArt->assemblyPossibilities(ourOwner->curHero);

				// If the artifact can be assembled, display dialog.
				BOOST_FOREACH(const CArtifact *combination, assemblyPossibilities) 
				{
					LOCPLINT->showArtifactAssemblyDialog(
						ourArt->artType->id,
						combination->id,
						true,
						boost::bind(&CCallback::assembleArtifacts, LOCPLINT->cb, ourOwner->curHero, slotID, true, combination->id),
						0);

					if(assemblyPossibilities.size() > 2)
					{
						tlog3 << "More than one possibility of assembling... taking only first\n";
						break;
					}
				}

				// Otherwise if the artifact can be diasassembled, display dialog.
				if(ourArt->canBeDisassembled())
				{
					LOCPLINT->showArtifactAssemblyDialog(
						ourArt->artType->id,
						0,
						false,
						boost::bind(&CCallback::assembleArtifacts, LOCPLINT->cb, ourOwner->curHero, slotID, false, 0),
						0);
				}
			}
		}

		// Lastly just show the artifact description.
		LRClickableAreaWTextComp::clickRight(down, previousState);
	}
}

/**
 * Selects artifact slot so that the containing artifact looks like it's picked up.
 */
void CArtPlace::select ()
{
	if (locked)
		return;

	picked = true;
	//int backpackCorrection = -(slotID - Arts::BACKPACK_START < ourOwner->backpackPos);

	CCS->curh->dragAndDropCursor(graphics->artDefs->ourImages[ourArt->artType->id].bitmap);
	ourOwner->commonInfo->src.setTo(this, false);
	ourOwner->markPossibleSlots(ourArt);

	if(slotID >= Arts::BACKPACK_START)
		ourOwner->scrollBackpack(0); //will update slots

	ourOwner->updateParentWindow();
	ourOwner->safeRedraw();
}

/**
 * Deselects the artifact slot. FIXME: Not used. Maybe it should?
 */
void CArtPlace::deselect ()
{
	picked = false;
	CCS->curh->dragAndDropCursor(NULL);
	ourOwner->unmarkSlots();
	ourOwner->commonInfo->src.clear();
	if(slotID >= Arts::BACKPACK_START)
		ourOwner->scrollBackpack(0); //will update slots


	ourOwner->updateParentWindow();
	ourOwner->safeRedraw();
}

void CArtPlace::deactivate()
{
	if(active)
	{
		LRClickableAreaWTextComp::deactivate();
	}
}

void CArtPlace::showAll(SDL_Surface *to)
{
	if (ourArt && !picked && ourArt == ourOwner->curHero->getArt(slotID, false)) //last condition is needed for disassembling -> artifact may be gone, but we don't know yet TODO: real, nice solution
	{
		int graphic = locked ? 145 : ourArt->artType->id;
		blitAt(graphics->artDefs->ourImages[graphic].bitmap, pos.x, pos.y, to);
	}

	if(marked && active)
	{
		// Draw vertical bars.
		for (int i = 0; i < pos.h; ++i) 
		{
			CSDL_Ext::SDL_PutPixelWithoutRefresh(to, pos.x,             pos.y + i, 240, 220, 120);
			CSDL_Ext::SDL_PutPixelWithoutRefresh(to, pos.x + pos.w - 1, pos.y + i, 240, 220, 120);
		}

		// Draw horizontal bars.
		for (int i = 0; i < pos.w; ++i) 
		{
			CSDL_Ext::SDL_PutPixelWithoutRefresh(to, pos.x + i, pos.y,             240, 220, 120);
			CSDL_Ext::SDL_PutPixelWithoutRefresh(to, pos.x + i, pos.y + pos.h - 1, 240, 220, 120);
		}
	}
}

bool CArtPlace::fitsHere(const CArtifactInstance * art) const
{
	// You can place 'no artifact' anywhere.
	if(!art)
		return true;

	// Anything can but War Machines can be placed in backpack.
	if (slotID >= 19)
		return !CGI->arth->isBigArtifact(art->id);

	return art->canBePutAt(ArtifactLocation(ourOwner->curHero, slotID), true);
}

CArtPlace::~CArtPlace()
{
	deactivate();
}

void CArtPlace::setMeAsDest(bool backpackAsVoid /*= true*/)
{
	ourOwner->commonInfo->dst.setTo(this, backpackAsVoid);
}

void CArtPlace::setArtifact(const CArtifactInstance *art)
{
	baseType = -1; //by default we don't store any component
	ourArt = art;
	if(!art)
	{
		text = std::string();
		hoverText = CGI->generaltexth->allTexts[507];
	}
	else
	{
		text = ourArt->artType->Description();
		if(art->artType->id == 1) //spell scroll
		{
			// we expect scroll description to be like this: This scroll contains the [spell name] spell which is added into your spell book for as long as you carry the scroll.
			// so we want to replace text in [...] with a spell name
			// however other language versions don't have name placeholder at all, so we have to be careful
			int spellID = art->getGivenSpellID();
			int nameStart = text.find_first_of('[');
			int nameEnd = text.find_first_of(']', nameStart);
			if(spellID >= 0)
			{
				if(nameStart != std::string::npos  &&  nameEnd != std::string::npos)
					text = text.replace(nameStart, nameEnd - nameStart + 1, CGI->spellh->spells[spellID]->name);

				//add spell component info (used to provide a pic in r-click popup)
				baseType = SComponent::spell;
				type = spellID;
				bonusValue = 0;
			}
		}

		if (locked) // Locks should appear as empty.
			hoverText = CGI->generaltexth->allTexts[507];
		else
			hoverText = boost::str(boost::format(CGI->generaltexth->heroscrn[1]) % ourArt->artType->Name());
	}
}

void HoverableArea::hover (bool on)
{
	if (on)
		GH.statusbar->print(hoverText);
	else if (GH.statusbar->getCurrent()==hoverText)
		GH.statusbar->clear();
}

HoverableArea::HoverableArea()
{
	used |= HOVER;
}

HoverableArea::~HoverableArea()
{
}

void LRClickableAreaWText::clickLeft(tribool down, bool previousState)
{
	if(!down && previousState)
	{
		LOCPLINT->showInfoDialog(text);
	}
}
void LRClickableAreaWText::clickRight(tribool down, bool previousState)
{
	adventureInt->handleRightClick(text, down);
}

LRClickableAreaWText::LRClickableAreaWText()
{
	init();
}

LRClickableAreaWText::LRClickableAreaWText(const Rect &Pos, const std::string &HoverText /*= ""*/, const std::string &ClickText /*= ""*/)
{
	init();
	pos = Pos + pos;
	hoverText = HoverText;
	text = ClickText;
}

LRClickableAreaWText::~LRClickableAreaWText()
{
}

void LRClickableAreaWText::init()
{
	used = LCLICK | RCLICK | HOVER;
}

void LRClickableAreaWTextComp::clickLeft(tribool down, bool previousState)
{
	if((!down) && previousState)
	{
		std::vector<SComponent*> comp(1, createComponent());
		LOCPLINT->showInfoDialog(text, comp);
	}
}

LRClickableAreaWTextComp::LRClickableAreaWTextComp(const Rect &Pos, int BaseType)
	: LRClickableAreaWText(Pos), baseType(BaseType), bonusValue(-1)
{
}

SComponent * LRClickableAreaWTextComp::createComponent() const
{
	if(baseType >= 0)
		return new SComponent(SComponent::Etype(baseType), type, bonusValue);
	else
		return NULL;
}

void LRClickableAreaWTextComp::clickRight(tribool down, bool previousState)
{
	if(down)
	{
		if(SComponent *comp = createComponent())
		{
			CRClickPopup::createAndPush(text, CInfoWindow::TCompsInfo(1, comp));
			return;
		}
	}

	LRClickableAreaWText::clickRight(down, previousState); //only if with-component variant not occured
}

CHeroArea::CHeroArea(int x, int y, const CGHeroInstance * _hero):hero(_hero)
{
	used = LCLICK | RCLICK | HOVER;
	pos.x += x;	pos.w = 58;
	pos.y += y;	pos.h = 64;
}

void CHeroArea::clickLeft(tribool down, bool previousState)
{
	if((!down) && previousState && hero)
		LOCPLINT->openHeroWindow(hero);
}

void CHeroArea::clickRight(tribool down, bool previousState)
{
	if((!down) && previousState && hero)
		LOCPLINT->openHeroWindow(hero);
}

void CHeroArea::hover(bool on)
{
	if (on && hero)
		GH.statusbar->print(hero->hoverName);
	else
		GH.statusbar->clear();
}

void CHeroArea::showAll(SDL_Surface * to)
{
	if (hero)
		blitAtLoc(graphics->portraitLarge[hero->portrait],0,0,to);
}

void LRClickableAreaOpenTown::clickLeft(tribool down, bool previousState)
{
	if((!down) && previousState && town)
		{
		LOCPLINT->openTownWindow(town);
		if ( type == 2 )
			LOCPLINT->castleInt->builds->buildingClicked(10);
		else if ( type == 3 && town->fortLevel() )
			LOCPLINT->castleInt->builds->buildingClicked(7);
		}
}

void LRClickableAreaOpenTown::clickRight(tribool down, bool previousState)
{
	if((!down) && previousState && town)
		LOCPLINT->openTownWindow(town);//TODO: popup?
}

LRClickableAreaOpenTown::LRClickableAreaOpenTown()
	: LRClickableAreaWTextComp(Rect(0,0,0,0), -1)
{
}

void CArtifactsOfHero::SCommonPart::reset()
{
	src.clear();
	dst.clear();
	CCS->curh->dragAndDropCursor(NULL);
}

void CArtifactsOfHero::setHero(const CGHeroInstance * hero)
{
// 	// An update is made, rather than initialization.
// 	if (curHero && curHero->id == hero->id) 
// 	{
// 		if(curHero != hero)
// 		{
// 			//delete	curHero;
// 			curHero = hero; //was: creating a copy
// 		}
// 
// 		// Compensate backpack pos if an artifact was insertad before it.
// 		if (commonInfo->dst.slotID >= 19 && commonInfo->destAOH == this
// 			&& commonInfo->dst.slotID - 19 < backpackPos)
// 		{
// 			backpackPos++;
// 		}
// 
// 		if (updateState && commonInfo->srcAOH == this) 
// 		{
// 			// A swap was made, make the replaced artifact the current selected.
// 			if (commonInfo->dst.slotID < 19 && commonInfo->destArtifact) 
// 			{
// // 				// Temporarily remove artifact from hero.
// // 				if (commonInfo->srcSlotID < 19)
// // 					CGI->arth->unequipArtifact(curHero->artifWorn, commonInfo->srcSlotID);
// // 				else
// // 					curHero->artifacts.erase(curHero->artifacts.begin() + (commonInfo->srcSlotID - 19));
// 
// 				updateParentWindow(); //TODO: evil! but does the thing
// 
// 				// Source <- Dest
// 				commonInfo->srcArtifact = commonInfo->destArtifact;
// 
// 				// Reset destination parameters.
// 				commonInfo->dst.clear();
// 
// 				CCS->curh->dragAndDropCursor(graphics->artDefs->ourImages[commonInfo->srcArtifact->id].bitmap);
// 				markPossibleSlots(commonInfo->srcArtifact);
// 			} 
// 			else if (commonInfo->destAOH != NULL) 
// 			{
// 				// Reset all parameters.
// 				commonInfo->reset();
// 				unmarkSlots();
// 			}
// 		}
// 	} 
// 	else 
// 	{
// 		commonInfo->reset();
// 	}
// 
// 	if(hero != curHero)
// 	{
// // 		delete curHero;
// 		// 		curHero = new CGHeroInstance(*hero);
// 		curHero = hero; //was: creating a copy
// 	}

	curHero = hero;
	if (curHero->artifactsInBackpack.size() > 0)
		backpackPos %= curHero->artifactsInBackpack.size();
	else
		backpackPos = 0;

	// Fill the slots for worn artifacts and backpack.
	for (int g = 0; g < artWorn.size() ; g++)
		setSlotData(artWorn[g], g);
	scrollBackpack(0);
}

void CArtifactsOfHero::dispose()
{
	//delNull(curHero);
	unmarkSlots(false);
	CCS->curh->dragAndDropCursor(NULL);
}

void CArtifactsOfHero::scrollBackpack(int dir)
{
	int artsInBackpack = curHero->artifactsInBackpack.size();
	backpackPos += dir;
	if(backpackPos < 0)// No guarantee of modulus behavior with negative operands -> we keep it positive
		backpackPos += artsInBackpack;

	if(artsInBackpack)
		backpackPos %= artsInBackpack;

	std::multiset<const CArtifactInstance *> toOmit = artifactsOnAltar;
	if(commonInfo->src.art) //if we picked an art from backapck, its slot has to be omitted
		toOmit.insert(commonInfo->src.art);
	
	int omitedSoFar = 0;

	//set new data
	size_t s = 0;
	for( ; s < artsInBackpack; ++s) 
	{

		if (s < artsInBackpack)
		{
			int slotID = 19 + (s + backpackPos)%artsInBackpack;
			const CArtifactInstance *art = curHero->getArt(slotID);
			assert(art);
			if(!vstd::contains(toOmit, art))
			{ 
				if(s - omitedSoFar < backpack.size())
					setSlotData(backpack[s-omitedSoFar], slotID);
			}
			else
			{
				toOmit -= art;
				omitedSoFar++;
				continue;
			}
		}
	}
	for( ; s - omitedSoFar < backpack.size(); s++)
		eraseSlotData(backpack[s-omitedSoFar], 19 + s);

	//in artifact merchant selling artifacts we may have highlight on one of backpack artifacts -> market needs update, cause artifact under highlight changed
	if(highlightModeCallback)
	{
		for(int i = 0; i < backpack.size(); i++)
		{
			if(backpack[i]->marked)
			{
				highlightModeCallback(backpack[i]);
				break;
			}
		}
	}

	//blocking scrolling if there is not enough artifacts to scroll
	bool scrollingPossible = artsInBackpack - omitedSoFar > backpack.size();
	leftArtRoll->block(!scrollingPossible);
	rightArtRoll->block(!scrollingPossible);

	safeRedraw();

}

/**
 * Marks possible slots where a given artifact can be placed, except backpack.
 *
 * @param art Artifact checked against.
 */
void CArtifactsOfHero::markPossibleSlots(const CArtifactInstance* art)
{
	BOOST_FOREACH(CArtifactsOfHero *aoh, commonInfo->participants)
		BOOST_FOREACH(CArtPlace *place, aoh->artWorn)
			place->marked = art->canBePutAt(ArtifactLocation(aoh->curHero, place->slotID), true);
	
	safeRedraw();
}

/**
 * Unamarks all slots.
 */
void CArtifactsOfHero::unmarkSlots(bool withRedraw /*= true*/)
{
	if(commonInfo)
		BOOST_FOREACH(CArtifactsOfHero *aoh, commonInfo->participants)
			aoh->unmarkLocalSlots(false);
	else
		unmarkLocalSlots(false);\

	if(withRedraw)
		safeRedraw();
}

void CArtifactsOfHero::unmarkLocalSlots(bool withRedraw /*= true*/)
{
	BOOST_FOREACH(CArtPlace *place, artWorn)
		place->marked = false;
	BOOST_FOREACH(CArtPlace *place, backpack)
		place->marked = false;

	if(withRedraw)
		safeRedraw();
}

/**
 * Assigns an artifacts to an artifact place depending on it's new slot ID.
 */
void CArtifactsOfHero::setSlotData(CArtPlace* artPlace, int slotID)
{
	if(!artPlace && slotID >= Arts::BACKPACK_START) //spurious call from artifactMoved in attempt to update hidden backpack slot
	{
		return;
	}

	artPlace->picked = false;
	artPlace->slotID = slotID;
	
	if(const ArtSlotInfo *asi = curHero->getSlot(slotID))
	{
		artPlace->setArtifact(asi->artifact);
		artPlace->locked = asi->locked;
	}
	else
		artPlace->setArtifact(NULL);
} 

/**
 * Makes given artifact slot appear as empty with a certain slot ID.
 */
void CArtifactsOfHero::eraseSlotData (CArtPlace* artPlace, int slotID)
{
	artPlace->picked = false;
	artPlace->slotID = slotID;
	artPlace->setArtifact(NULL);
}

CArtifactsOfHero::CArtifactsOfHero(std::vector<CArtPlace *> ArtWorn, std::vector<CArtPlace *> Backpack,
	AdventureMapButton *leftScroll, AdventureMapButton *rightScroll, bool createCommonPart):

	curHero(NULL),
	artWorn(ArtWorn), backpack(Backpack),
	backpackPos(0), commonInfo(NULL), updateState(false),
	leftArtRoll(leftScroll), rightArtRoll(rightScroll),
	allowedAssembling(true), highlightModeCallback(0)
{
	if(createCommonPart)
	{
		commonInfo = new CArtifactsOfHero::SCommonPart;
		commonInfo->participants.insert(this);
	}
	
	// Init slots for worn artifacts.
	for (size_t g = 0; g < artWorn.size() ; g++)
	{
		artWorn[g]->ourOwner = this;
		eraseSlotData(artWorn[g], g);
	}

	// Init slots for the backpack.
	for(size_t s=0; s<backpack.size(); ++s)
	{
		backpack[s]->ourOwner = this;
		eraseSlotData(backpack[s], 19 + s);
	}

	leftArtRoll->callback  += boost::bind(&CArtifactsOfHero::scrollBackpack,this,-1);
	rightArtRoll->callback += boost::bind(&CArtifactsOfHero::scrollBackpack,this,+1);
}

CArtifactsOfHero::CArtifactsOfHero(const Point& position, bool createCommonPart /*= false*/)
 : curHero(NULL), backpackPos(0), commonInfo(NULL), updateState(false), allowedAssembling(true), highlightModeCallback(0)
{
	if(createCommonPart)
	{
		commonInfo = new CArtifactsOfHero::SCommonPart;
		commonInfo->participants.insert(this);
	}

	OBJ_CONSTRUCTION_CAPTURING_ALL;
	pos += position;
	artWorn.resize(19);
	
	std::vector<Rect> slotPos;
	slotPos += genRect(44,44,509,30), genRect(44,44,567,240), genRect(44,44,509,80), 
		genRect(44,44,383,68), genRect(44,44,564,183), genRect(44,44,509,130), 
		genRect(44,44,431,68), genRect(44,44,610,183), genRect(44,44,515,295), 
		genRect(44,44,383,143), genRect(44,44,399,194), genRect(44,44,415,245),
		genRect(44,44,431,296), genRect(44,44,564,30), genRect(44,44,610,30), 
		genRect(44,44,610,76), genRect(44,44,610,122), genRect(44,44,610,310),	
		genRect(44,44,381,296);

	// Create slots for worn artifacts.
	for (size_t g = 0; g < 19 ; g++)
	{
		artWorn[g] = new CArtPlace(NULL);
		artWorn[g]->pos = slotPos[g] + pos;
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

	leftArtRoll = new AdventureMapButton(std::string(), std::string(), boost::bind(&CArtifactsOfHero::scrollBackpack,this,-1), 379, 364, "hsbtns3.def", SDLK_LEFT);
	rightArtRoll = new AdventureMapButton(std::string(), std::string(), boost::bind(&CArtifactsOfHero::scrollBackpack,this,+1), 632, 364, "hsbtns5.def", SDLK_RIGHT);
}

CArtifactsOfHero::~CArtifactsOfHero()
{
	dispose();
}

void CArtifactsOfHero::updateParentWindow()
{
	if (CHeroWindow* chw = dynamic_cast<CHeroWindow*>(GH.topInt())) 
	{
		if(updateState)
			chw->curHero = curHero;
		else
			chw->update(curHero, true);
	} 
	else if(CExchangeWindow* cew = dynamic_cast<CExchangeWindow*>(GH.topInt()))
	{

		//use our copy of hero to draw window
		if(cew->heroInst[0]->id == curHero->id)
			cew->heroInst[0] = curHero;
		else
			cew->heroInst[1] = curHero;

		if(!updateState)
		{
			cew->deactivate();
// 			for(int g=0; g<ARRAY_COUNT(cew->heroInst); ++g)
// 			{
// 				if(cew->heroInst[g] == curHero)
// 				{
// 					cew->artifs[g]->setHero(curHero);
// 				}
// 			}


			cew->prepareBackground();
			cew->redraw();
			cew->activate();
		}
	}
}

void CArtifactsOfHero::safeRedraw()
{
	if(parent)
		parent->redraw();
	else 
		redraw();
}

void CArtifactsOfHero::realizeCurrentTransaction()
{
	assert(commonInfo->src.AOH);
	assert(commonInfo->dst.AOH);
	LOCPLINT->cb->swapArtifacts(commonInfo->src.AOH->curHero, commonInfo->src.slotID, 
								commonInfo->dst.AOH->curHero, commonInfo->dst.slotID);
}

void CArtifactsOfHero::artifactMoved(const ArtifactLocation &src, const ArtifactLocation &dst)
{
	if(src.hero == curHero && src.slot >= Arts::BACKPACK_START)
		updateSlot(src.slot);
	if(dst.hero == curHero && dst.slot >= Arts::BACKPACK_START)
		updateSlot(dst.slot);
	if(src.hero == curHero  ||  dst.hero == curHero) //we need to update all slots, artifact might be combined and affect more slots
		updateWornSlots(false);

	if(commonInfo->src == src) //artifact was taken from us
	{
		assert(commonInfo->dst == dst  ||  dst.slot == dst.hero->artifactsInBackpack.size() + Arts::BACKPACK_START);
		commonInfo->reset();
		unmarkSlots();
		updateParentWindow();
	}
	else if(commonInfo->dst == src) //the dest artifact was moved -> we are picking it
	{
		assert(dst.slot >= Arts::BACKPACK_START);
		commonInfo->reset();

		CArtPlace *ap = NULL;
		BOOST_FOREACH(CArtifactsOfHero *aoh, commonInfo->participants)
		{
			if(aoh->curHero == dst.hero)
			{
				commonInfo->src.AOH = aoh;
				if((ap = aoh->getArtPlace(dst.slot)))
					break;
			}
		}

		if(ap)
		{
			ap->select();
		}
		else
		{
			commonInfo->src.art = dst.getArt();
			commonInfo->src.slotID = dst.slot;
			assert(commonInfo->src.AOH);
			CCS->curh->dragAndDropCursor(graphics->artDefs->ourImages[dst.getArt()->artType->id].bitmap);
			markPossibleSlots(dst.getArt());
			updateParentWindow();
		}
	}
	else if(src.slot >= Arts::BACKPACK_START && src.slot < commonInfo->src.slotID && src.hero == commonInfo->src.AOH->curHero) //artifact taken from before currently picked one
	{
		//int fixedSlot = src.hero->getArtPos(commonInfo->src.art);
		commonInfo->src.slotID--;
		assert(commonInfo->src.valid());
		updateParentWindow();
	}
	else
	{
		tlog1 << "Unexpected artifact movement...\n";
	}

 	int shift = 0;
// 	if(dst.slot >= Arts::BACKPACK_START && dst.slot - Arts::BACKPACK_START < backpackPos)
// 		shift++;
// 
 	if(src.slot < Arts::BACKPACK_START  &&  dst.slot - Arts::BACKPACK_START < backpackPos)
		shift++;
	if(dst.slot < Arts::BACKPACK_START  &&  src.slot - Arts::BACKPACK_START < backpackPos)
 		shift--;

	if( (src.hero == curHero && src.slot >= Arts::BACKPACK_START)
	 || (dst.hero == curHero && dst.slot >= Arts::BACKPACK_START) )
		scrollBackpack(shift); //update backpack slots
}

void CArtifactsOfHero::artifactRemoved(const ArtifactLocation &al)
{
	if(al.hero == curHero)
	{
		if(al.slot < Arts::BACKPACK_START)
			updateWornSlots(0);
		else
			scrollBackpack(0); //update backpack slots
	}
}

CArtPlace * CArtifactsOfHero::getArtPlace(int slot)
{
	if(slot < Arts::BACKPACK_START)
	{
		return artWorn[slot];
	}
	else
	{
		BOOST_FOREACH(CArtPlace *ap, backpack)
			if(ap->slotID == slot)
				return ap;
	}

	return NULL;
}

void CArtifactsOfHero::artifactAssembled(const ArtifactLocation &al)
{
	if(al.hero == curHero)
		updateWornSlots();
}

void CArtifactsOfHero::artifactDisassembled(const ArtifactLocation &al)
{
	if(al.hero == curHero)
		updateWornSlots();
}

void CArtifactsOfHero::updateWornSlots(bool redrawParent /*= true*/)
{
	for(int i = 0; i < Arts::BACKPACK_START; i++)
		updateSlot(i);


	if(redrawParent)
		updateParentWindow();
}

const CGHeroInstance * CArtifactsOfHero::getHero() const
{
	return curHero;
}

void CArtifactsOfHero::updateSlot(int slotID)
{
	setSlotData(getArtPlace(slotID), slotID);
}

void CExchangeWindow::close()
{
	GH.popIntTotally(this);
}

void CExchangeWindow::activate()
{
	quit->activate();
	garr->activate();

	//artifs[0]->setHero(heroInst[0]);
	artifs[0]->activate();
	//artifs[1]->setHero(heroInst[1]);
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

	GH.statusbar = ourBar;

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

	quit->showAll(to);

	//printing border around window
	if(screen->w != 800 || screen->h !=600)
		CMessage::drawBorder(LOCPLINT->playerID,to,828,628,pos.x-14,pos.y-15);

	artifs[0]->showAll(to);
	artifs[1]->showAll(to);

	ourBar->showAll(to);

	for(int g=0; g<ARRAY_COUNT(secSkillAreas); g++)
	{
		questlogButton[g]->showAll(to);//FIXME: for array count(secondary skill) show quest log button? WTF?
	}

	garr->showAll(to);
}

void CExchangeWindow::questlog(int whichHero)
{
	CCS->curh->dragAndDropCursor(NULL);
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
		CHeroWithMaybePickedArtifact heroWArt = CHeroWithMaybePickedArtifact(this, heroInst[b]);
		//printing primary skills' amounts
		for(int m=0; m<4; ++m)
		{
			std::ostringstream primarySkill;
			primarySkill << heroWArt.getPrimSkillLevel(m);
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
		blitAt(graphics->morale30->ourImages[heroWArt.MoraleVal()+3].bitmap, 177 + 490*b, 45, bg);

		//setting luck
		blitAt(graphics->luck30->ourImages[heroWArt.LuckVal()+3].bitmap, 213 + 490*b, 45, bg);
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
	heroInst[0] = LOCPLINT->cb->getHero(hero1);
	heroInst[1] = LOCPLINT->cb->getHero(hero2);

	prepareBackground();
	pos.x = screen->w/2 - bg->w/2;
	pos.y = screen->h/2 - bg->h/2;
	pos.w = screen->w;
	pos.h = screen->h;

	
	artifs[0] = new CArtifactsOfHero(Point(pos.x + -334, pos.y + 150));
	artifs[0]->commonInfo = new CArtifactsOfHero::SCommonPart;
	artifs[0]->commonInfo->participants.insert(artifs[0]);
	artifs[0]->setHero(heroInst[0]);
	artifs[1] = new CArtifactsOfHero(Point(pos.x + 96, pos.y + 150));
	artifs[1]->commonInfo = artifs[0]->commonInfo;
	artifs[1]->commonInfo->participants.insert(artifs[1]);
	artifs[1]->setHero(heroInst[1]);

	artSets.push_back(artifs[0]);
	artSets.push_back(artifs[1]);

	//primary skills
	for(int g=0; g<4; ++g)
	{
		//primary skill's clickable areas
		primSkillAreas.push_back(new LRClickableAreaWTextComp());
		primSkillAreas[g]->pos = genRect(32, 140, pos.x+329, pos.y + 19 + 36 * g);
		primSkillAreas[g]->text = CGI->generaltexth->arraytxt[2+g];
		primSkillAreas[g]->type = g;
		primSkillAreas[g]->bonusValue = -1;
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
			secSkillAreas[b][g]->bonusValue = level;
			secSkillAreas[b][g]->text = CGI->generaltexth->skillInfoTexts[skill][level-1];

			sprintf(bufor, CGI->generaltexth->heroscrn[21].c_str(), CGI->generaltexth->levels[level - 1].c_str(), CGI->generaltexth->skillName[skill].c_str());
			secSkillAreas[b][g]->hoverText = std::string(bufor);
		}

		portrait[b] = new CHeroArea(pos.x + 257 + 228*b, pos.y + 13, heroInst[b]);

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
		morale[b] = new MoraleLuckBox(true, genRect(32, 32, pos.x + 177 + 490*b, pos.y + 45));
		morale[b]->set(heroInst[b]);
		//setting luck
		luck[b] = new MoraleLuckBox(false, genRect(32, 32, pos.x + 213 + 490*b, pos.y + 45));
		luck[b]->set(heroInst[b]);
	}

	//buttons
	quit = new AdventureMapButton(CGI->generaltexth->tcommands[8], "", boost::bind(&CExchangeWindow::close, this), pos.x+732, pos.y+567, "IOKAY.DEF", SDLK_RETURN);
	questlogButton[0] = new AdventureMapButton(CGI->generaltexth->heroscrn[0], std::string(), boost::bind(&CExchangeWindow::questlog,this, 0), pos.x+10, pos.y+44, "hsbtns4.def");
	questlogButton[1] = new AdventureMapButton(CGI->generaltexth->heroscrn[0], std::string(), boost::bind(&CExchangeWindow::questlog,this, 1), pos.x+740, pos.y+44, "hsbtns4.def");

	//statusbar 
	//FIXME - this image is a bit bigger than required - part of background should be used instead
	ourBar = new CGStatusBar(pos.x + 3, pos.y + 577, "KSTATBAR");

	//garrison interface
	garr = new CGarrisonInt(pos.x + 69, pos.y + 131, 4, Point(418,0), bg, Point(69,131), heroInst[0],heroInst[1], true, true);

	garr->addSplitBtn(new AdventureMapButton(CGI->generaltexth->tcommands[3],"",boost::bind(&CGarrisonInt::splitClick,garr),pos.x+10,pos.y+132,"TSBTNS.DEF"));
	garr->addSplitBtn(new AdventureMapButton(CGI->generaltexth->tcommands[3],"",boost::bind(&CGarrisonInt::splitClick,garr),pos.x+740,pos.y+132,"TSBTNS.DEF"));
}

CExchangeWindow::~CExchangeWindow() //d-tor
{
	SDL_FreeSurface(bg);
	delete quit;

	//warning: don't experiment with these =NULL lines, they prevent heap corruption!
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
	Rect clipRect = genRect(64, 96, pos.x+110, pos.y+85);
	CSDL_Ext::blit8bppAlphaTo24bpp(graphics->boatAnims[boat]->ourImages[21 + frame++/8%8].bitmap, NULL, to, &clipRect);
	build->showAll(to);
	quit->showAll(to);
}

CShipyardWindow::~CShipyardWindow()
{
	delete build;
	delete quit;
}

CShipyardWindow::CShipyardWindow(const std::vector<si32> &cost, int state, int boatType, const boost::function<void()> &onBuy)
{
	boat = boatType;
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

CPuzzleWindow::CPuzzleWindow(const int3 &grailPos, float discoveredRatio)
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
	quitb->assignedKeys.insert(SDLK_ESCAPE);
	quitb->borderColor = Colors::MetallicGold;
	quitb->borderEnabled = true;

	resdatabar = new CResDataBar("ZRESBAR.bmp", pos.x+3, pos.y+575, 32, 2, 85, 85);
	resdatabar->pos.x = pos.x+3; resdatabar->pos.y = pos.y+575;

	//printing necessary things to background
	int3 moveInt = int3(8, 9, 0);
	Rect mapRect = genRect(544, 591, 8, 8);
	CGI->mh->terrainRect
		(grailPos - moveInt, adventureInt->anim,
		 &LOCPLINT->cb->getVisibilityMap(), true, adventureInt->heroAnim,
		 background, &mapRect, 0, 0, true, moveInt);

	int faction = LOCPLINT->cb->getStartInfo()->playerInfos.find(LOCPLINT->playerID)->second.castle;

	std::vector<SPuzzleInfo> puzzlesToPrint;

	for(int g=0; g<PUZZLES_PER_FACTION; ++g)
	{
		if(CGI->heroh->puzzleInfo[faction][g].whenUncovered > PUZZLES_PER_FACTION * discoveredRatio)
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
	quitb->showAll(to);
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

void CTransformerWindow::CItem::showAll(SDL_Surface * to)
{
	SDL_Surface * backgr = graphics->bigImgs[parent->army->getCreature(id)->idNumber];
	blitAt(backgr, pos.x, pos.y, to);
	printAtMiddle(boost::lexical_cast<std::string>(size),pos.x+28, pos.y+76,FONT_SMALL,zwykly,to);//stack size
}

void CTransformerWindow::CItem::move()
{
	if (left)
		pos.x += 289;
	else
		pos.x -= 289;
	left = !left;
}

void CTransformerWindow::CItem::clickLeft(tribool down, bool previousState)
{
	if(previousState && (!down))
	{
		move();
		parent->showAll(screen2);
	}
}

CTransformerWindow::CItem::CItem(CTransformerWindow * _parent, int _size, int _id):
	id(_id), size(_size), parent(_parent)
{
	used = LCLICK;
	left = true;
	pos.w = 58;
	pos.h = 64;
	
	pos.x += 45  + (id%3)*83 + id/6*83;
	pos.y += 109 + (id/3)*98;
}

CTransformerWindow::CItem::~CItem()
{
	
}

void CTransformerWindow::showAll(SDL_Surface * to)
{
	CIntObject::showAll(to);
	printAtMiddleLoc(  CGI->generaltexth->allTexts[485], 153,     29,FONT_SMALL,     tytulowy,to);//holding area
	printAtMiddleLoc(  CGI->generaltexth->allTexts[486], 153+295, 29,FONT_SMALL,     tytulowy,to);//transformer
	printAtMiddleWBLoc(CGI->generaltexth->allTexts[487], 153,     75,FONT_MEDIUM, 32,tytulowy,to);//move creatures to create skeletons
	printAtMiddleWBLoc(CGI->generaltexth->allTexts[488], 153+295, 75,FONT_MEDIUM, 32,tytulowy,to);//creatures here will become skeletons
}

void CTransformerWindow::makeDeal()
{
	for (int i=0; i<items.size(); i++)
		if (!items[i]->left)
			LOCPLINT->cb->trade(town, CREATURE_UNDEAD, items[i]->id, 0, 0, hero);
}

void CTransformerWindow::addAll()
{
	for (int i=0; i<items.size(); i++)
		if (items[i]->left)
			items[i]->move();
	showAll(screen2);
}

CTransformerWindow::CTransformerWindow(const CGHeroInstance * _hero, const CGTownInstance * _town):hero(_hero),town(_town)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL;
	bg = new CPicture ("SKTRNBK.PCX");
	bg->colorizeAndConvert(LOCPLINT->playerID);
	pos = center(bg->pos);
	
	if (hero)
		army = hero;
	else
		army = town;
	
	for (int i=0; i<7; i++ )
		if ( army->getCreature(i) )
			items.push_back(new CItem(this, army->getStackCount(i), i));
			
	all    = new AdventureMapButton(CGI->generaltexth->zelp[590],boost::bind(&CTransformerWindow::addAll,this),     146,416,"ALTARMY.DEF",SDLK_a);
	convert= new AdventureMapButton(CGI->generaltexth->zelp[591],boost::bind(&CTransformerWindow::makeDeal,this),   269,416,"ALTSACR.DEF",SDLK_RETURN);
	cancel = new AdventureMapButton(CGI->generaltexth->zelp[592],boost::bind(&CGuiHandler::popIntTotally,&GH, this),392,416,"ICANCEL.DEF",SDLK_ESCAPE);
	bar    = new CGStatusBar(304, 469);
}

CTransformerWindow::~CTransformerWindow()
{
	
}

void CUniversityWindow::CItem::clickLeft(tribool down, bool previousState)
{
	if(previousState && (!down))
	{
		if ( state() != 2 )
			return;
		CUnivConfirmWindow *win = new CUnivConfirmWindow(parent, ID, LOCPLINT->cb->getResourceAmount(6) >= 2000);
		GH.pushInt(win);
	}
}

void CUniversityWindow::CItem::clickRight(tribool down, bool previousState)
{
	if(down)
	{
		CInfoPopup *message = new CInfoPopup();
		message->free = true;
		message->bitmap = CMessage::drawBoxTextBitmapSub
		                            (LOCPLINT->playerID,
		                             CGI->generaltexth->skillInfoTexts[ID][0],
		                             graphics->abils82->ourImages[ID*3+3].bitmap,
		                             CGI->generaltexth->skillName[ID]);
		message->pos.x = screen->w/2 - message->bitmap->w/2;
		message->pos.y = screen->h/2 - message->bitmap->h/2;
		GH.pushInt(message);
	}
}

void CUniversityWindow::CItem::hover(bool on)
{
	if (on)
		GH.statusbar->print(CGI->generaltexth->skillName[ID]);
	else
		GH.statusbar->clear();
}

int CUniversityWindow::CItem::state()
{
	if (parent->hero->getSecSkillLevel(static_cast<CGHeroInstance::SecondarySkill>(ID)))//hero know this skill
		return 1;
	if (parent->hero->secSkills.size() >= SKILL_PER_HERO)//can't learn more skills
		return 0;
	if (parent->hero->type->heroClass->proSec[ID]==0)//can't learn this skill (like necromancy for most of non-necros)
		return 0;
/*	if (LOCPLINT->cb->getResourceAmount(6) < 2000 )//no gold - allowed in H3, confirm button is blocked instead
		return 0;*/
	return 2;
}

void CUniversityWindow::CItem::showAll(SDL_Surface * to)
{
	CPicture * bar;
	switch (state())
	{
		case 0: bar = parent->red;
		        break;
		case 1: bar = parent->yellow;
		        break;
		case 2: bar = parent->green;
		        break;
		default:bar = NULL;
		        break;
	}
	assert(bar);
	
	blitAtLoc(bar->bg, -28, -22, to);
	blitAtLoc(bar->bg, -28,  48, to);
	printAtMiddleLoc  (CGI->generaltexth->skillName[ID], 22, -13, FONT_SMALL, zwykly,to);//Name
	printAtMiddleLoc  (CGI->generaltexth->levels[0], 22, 57, FONT_SMALL, zwykly,to);//Level(always basic)
	
	CPicture::showAll(to);
}

CUniversityWindow::CItem::CItem(CUniversityWindow * _parent, int _ID, int X, int Y):
	CPicture (graphics->abils44->ourImages[_ID*3+3].bitmap,X,Y,false),ID(_ID), parent(_parent)
{
	used = LCLICK | RCLICK | HOVER;
}

CUniversityWindow::CUniversityWindow(const CGHeroInstance * _hero, const IMarket * _market):hero(_hero), market(_market)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL;
	bg = new CPicture("UNIVERS1.PCX");
	bg->colorizeAndConvert(LOCPLINT->playerID);
	
	green  = new CPicture("UNIVGREN.PCX");
	yellow = new CPicture("UNIVGOLD.PCX");//bars
	red    = new CPicture("UNIVRED.PCX");

	green->recActions  =
	yellow->recActions =
	red->recActions    = DISPOSE;

	if ( market->o->ID == 104 ) // this is adventure map university
	{
		SDL_Surface * titleImage = BitmapHandler::loadBitmap("UNIVBLDG.PCX");
		blitAtLoc(titleImage, 232-titleImage->w/2, 76-titleImage->h/2, bg->bg);
		SDL_FreeSurface(titleImage);
	}
	else if (LOCPLINT->castleInt && LOCPLINT->castleInt->town->subID == 8)// this is town university
	{
		SDL_Surface * titleImage = LOCPLINT->castleInt->bicons->ourImages[21].bitmap;
		blitAtLoc(titleImage, 232-titleImage->w/2, 76-titleImage->h/2, bg->bg);
	}
	else
		tlog0<<"Error: Image for university was not found!\n";//This should not happen

	printAtMiddleWBLoc(CGI->generaltexth->allTexts[603], 232, 153, FONT_SMALL, 70,zwykly,bg->bg);//Clerk speech
	printAtMiddleLoc  (CGI->generaltexth->allTexts[602], 231, 26 , FONT_MEDIUM ,tytulowy,bg->bg);//University

	std::vector<int> list = market->availableItemsIds(RESOURCE_SKILL);
	if (list.size()!=4)
		tlog0<<"\t\tIncorrect size of available items vector!\n";
	for (int i=0; i<list.size(); i++)//prepare clickable items
		items.push_back(new CItem(this, list[i], pos.x+54+i*104, pos.y+234));
		
	pos = center(bg->pos);
	cancel = new AdventureMapButton(CGI->generaltexth->zelp[632],
		boost::bind(&CGuiHandler::popIntTotally,&GH, this),200,313,"IOKAY.DEF",SDLK_RETURN);
		
	bar = new CGStatusBar(232, 371);
}

CUniversityWindow::~CUniversityWindow()
{
	
}

CUnivConfirmWindow::CUnivConfirmWindow(CUniversityWindow * PARENT, int SKILL, bool available ):parent(PARENT)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL;
	bg = new CPicture("UNIVERS2.PCX");
	bg->colorizeAndConvert(LOCPLINT->playerID);

	std::string text = CGI->generaltexth->allTexts[608];
	boost::replace_first(text, "%s", CGI->generaltexth->levels[0]);
	boost::replace_first(text, "%s", CGI->generaltexth->skillName[SKILL]);
	boost::replace_first(text, "%d", "2000");

	printAtMiddleWBLoc(text, 230, 163, FONT_SMALL, 65,zwykly,bg->bg);//Clerk speech
	printAtMiddleLoc  (CGI->generaltexth-> skillName[SKILL], 230, 37,  FONT_SMALL,    zwykly,bg->bg);//Skill name
	printAtMiddleLoc  (CGI->generaltexth->levels[1], 230, 107, FONT_SMALL,    zwykly,bg->bg);//Skill level
	printAtMiddleLoc  ("2000", 230, 267, FONT_SMALL,    zwykly,bg->bg);//Cost
	blitAtLoc(graphics->abils44->ourImages[SKILL*3+3].bitmap, 211, 51,  bg->bg);//skill
	blitAtLoc(graphics->resources32->ourImages[6].bitmap, 210, 210, bg->bg);//gold

	pos = center(bg->pos);

	std::string hoverText = CGI->generaltexth->allTexts[609];
	boost::replace_first(hoverText, "%s", CGI->generaltexth->levels[0]+ " " + CGI->generaltexth->skillName[SKILL]);
	
	text = CGI->generaltexth->zelp[633].second;
	boost::replace_first(text, "%s", CGI->generaltexth->levels[0]);
	boost::replace_first(text, "%s", CGI->generaltexth->skillName[SKILL]);
	boost::replace_first(text, "%d", "2000");

	confirm= new AdventureMapButton(hoverText, text, boost::bind(&CUnivConfirmWindow::makeDeal, this, SKILL),
	         148,299,"IBY6432.DEF",SDLK_RETURN);
	confirm->block(!available);
	
	cancel = new AdventureMapButton(CGI->generaltexth->zelp[631],boost::bind(&CGuiHandler::popIntTotally, &GH, this),
	         252,299,"ICANCEL.DEF",SDLK_ESCAPE);
	bar = new CGStatusBar(232, 371);
}

void CUnivConfirmWindow::makeDeal(int skill)
{
	LOCPLINT->cb->trade(parent->market->o, RESOURCE_SKILL, 6, skill, 1, parent->hero);
	GH.popIntTotally(this);
}

CHillFortWindow::CHillFortWindow(const CGHeroInstance *visitor, const CGObjectInstance *object):
	fort(object),hero(visitor)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL;
	
	slotsCount=7;
	resources =  CDefHandler::giveDefEss("SMALRES.DEF");
	bg = new CPicture("APHLFTBK.PCX");
	bg->colorizeAndConvert(LOCPLINT->playerID);
	
	printAtMiddleLoc  (fort->hoverName, 325, 32, FONT_BIG, tytulowy, bg->bg);//Hill Fort
	pos = center(bg->pos);

	heroPic = new CHeroArea(30, 60, hero);
	
	currState.resize(slotsCount+1);
	costs.resize(slotsCount);
	totalSumm.resize(RESOURCE_QUANTITY);
	std::vector<std::string> files;
	files += "APHLF1R.DEF", "APHLF1Y.DEF", "APHLF1G.DEF";
	for (int i=0; i<slotsCount; i++)
	{
		currState[i] = getState(i);
		upgrade[i] = new AdventureMapButton(getTextForSlot(i),"",boost::bind(&CHillFortWindow::makeDeal, this, i),
		                                    107+i*76, 171, "", SDLK_1+i, &files);
		upgrade[i]->block(currState[i] == -1);
	}
	files.clear();
	files += "APHLF4R.DEF", "APHLF4Y.DEF", "APHLF4G.DEF";
	currState[slotsCount] = getState(slotsCount);
	upgradeAll = new AdventureMapButton(CGI->generaltexth->allTexts[432],"",boost::bind(&CHillFortWindow::makeDeal, this, slotsCount),
	                                    30, 231, "", SDLK_0, &files);
	quit = new AdventureMapButton("","",boost::bind(&CGuiHandler::popIntTotally, &GH, this), 294, 275, "IOKAY.DEF", SDLK_RETURN);
	bar = new CGStatusBar(327, 332);

	garr = new CGarrisonInt(108, 60, 18, Point(),bg->bg,Point(108,60),hero,NULL);
	updateGarrisons();
}

CHillFortWindow::~CHillFortWindow()
{
	
}

void CHillFortWindow::activate()
{
	CIntObject::activate();
	GH.statusbar = bar;
}

void CHillFortWindow::updateGarrisons()
{
	
	for (int i=0; i<RESOURCE_QUANTITY; i++)
		totalSumm[i]=0;
	
	for (int i=0; i<slotsCount; i++)
	{
		costs[i].clear();
		int newState = getState(i);
		if (newState != -1)
		{
			UpgradeInfo info;
			LOCPLINT->cb->getUpgradeInfo(hero, i, info);
			if (info.newID.size())//we have upgrades here - update costs
				for(std::set<std::pair<int,int> >::iterator it=info.cost[0].begin(); it!=info.cost[0].end(); it++)
				{
					std::pair<int, int> pair = std::make_pair(it->first, it->second * hero->getStackCount(i) );
					costs[i].insert(pair);
					totalSumm[pair.first] += pair.second;
				}
		}
		
		currState[i] = newState;
		upgrade[i]->setIndex(newState);
		upgrade[i]->block(currState[i] == -1);
		upgrade[i]->hoverTexts[0] = getTextForSlot(i);
	}
	
	int newState = getState(slotsCount);
	currState[slotsCount] = newState;
	upgradeAll->setIndex(newState);
	garr->recreateSlots();
}

void CHillFortWindow::makeDeal(int slot)
{
	int offset = (slot == slotsCount)?2:0;
	switch (currState[slot])
	{
		case 0:
			LOCPLINT->showInfoDialog(CGI->generaltexth->allTexts[314 + offset], 
			          std::vector<SComponent*>(), soundBase::sound_todo);
			break;
		case 1:
			LOCPLINT->showInfoDialog(CGI->generaltexth->allTexts[313 + offset], 
			          std::vector<SComponent*>(), soundBase::sound_todo);
			break;
		case 2:
			for (int i=0; i<slotsCount; i++)
				if ( slot ==i || ( slot == slotsCount && currState[i] == 2 ) )//this is activated slot or "upgrade all"
				{
					UpgradeInfo info;
					LOCPLINT->cb->getUpgradeInfo(hero, i, info);
					LOCPLINT->cb->upgradeCreature(hero, i, info.newID[0]);
				}
			break;
		
	}
}

void CHillFortWindow::showAll (SDL_Surface *to)
{
	CIntObject::showAll(to);
	
	for ( int i=0; i<slotsCount; i++)
	{
		if ( currState[i] == 0 || currState[i] == 2 )
		{
			if ( costs[i].size() )//we have several elements
			{
				int curY = 128;//reverse iterator is used to display gold as first element
				for( std::map<int,int>::reverse_iterator rit=costs[i].rbegin(); rit!=costs[i].rend(); rit++)
				{
					blitAtLoc(resources->ourImages[rit->first].bitmap, 104+76*i, curY, to);
					printToLoc(boost::lexical_cast<std::string>(rit->second), 168+76*i, curY+16, FONT_SMALL, zwykly, to);
					curY += 20;
				}
			}
			else//free upgrade - print gold image and "Free" text
			{
				blitAtLoc(resources->ourImages[6].bitmap, 104+76*i, 128, to);
				printToLoc(CGI->generaltexth->allTexts[344], 168+76*i, 144, FONT_SMALL, zwykly, to);
			}
		}
	}
	for (int i=0; i<RESOURCE_QUANTITY; i++)
	{
		if (totalSumm[i])//this resource is used - display it
		{
			blitAtLoc(resources->ourImages[i].bitmap, 104+76*i, 237, to);
			printToLoc(boost::lexical_cast<std::string>(totalSumm[i]), 166+76*i, 253, FONT_SMALL, zwykly, to);
		}
	}
}

std::string CHillFortWindow::getTextForSlot(int slot)
{
	if ( !hero->getCreature(slot) )//we dont have creature here
		return "";

	std::string str = CGI->generaltexth->allTexts[318];
	int amount = hero->getStackCount(slot);
	if ( amount == 1 )
		boost::algorithm::replace_first(str,"%s",hero->getCreature(slot)->nameSing);
	else
		boost::algorithm::replace_first(str,"%s",hero->getCreature(slot)->namePl);
	
	return str;
}

int CHillFortWindow::getState(int slot)
{
	if ( slot == slotsCount )//"Upgrade all" slot
	{
		bool allUpgraded = true;//All creatures are upgraded?
		for (int i=0; i<slotsCount; i++)
			allUpgraded &=  currState[i] == 1 || currState[i] == -1;
			
		if (allUpgraded)
			return 1;

		for ( int i=0; i<RESOURCE_QUANTITY; i++)//if we need more resources
			if(LOCPLINT->cb->getResourceAmount(i) < totalSumm[i])
				return 0;
				
		return 2;
	}

	if (hero->slotEmpty(slot))//no creature here
		return -1;
		
	UpgradeInfo info;
	LOCPLINT->cb->getUpgradeInfo(hero, slot, info);
	if (!info.newID.size())//already upgraded
		return 1;

	for(std::set<std::pair<int,int> >::iterator it=info.cost[0].begin(); it!=info.cost[0].end(); it++)
		if(LOCPLINT->cb->getResourceAmount(it->first) < it->second * hero->getStackCount(slot))
			return 0;
	return 2;//can upgrade
}

void CThievesGuildWindow::activate()
{
	CIntObject::activate();
	GH.statusbar = statusBar;
}

void CThievesGuildWindow::show(SDL_Surface * to)
{
	blitAt(background, pos.x, pos.y, to);

	statusBar->show(to);
	exitb->showAll(to);
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
	OBJ_CONSTRUCTION_CAPTURING_ALL;

	SThievesGuildInfo tgi; //info to be displayed
	LOCPLINT->cb->getThievesGuildInfo(tgi, owner);

	pos = center(Rect(0,0,800,600));// Rect( (conf.cc.resx - 800) / 2, (conf.cc.resy - 600) / 2, 800, 600 );

	//loading backround and converting to more bpp form
	SDL_Surface * bg = background = BitmapHandler::loadBitmap("TpRank.bmp", false);
	graphics->blueToPlayersAdv(bg,LOCPLINT->playerID);
	background = newSurface(bg->w, bg->h);
	blitAt(bg, 0, 0, background);
	SDL_FreeSurface(bg);

	exitb = new AdventureMapButton (std::string(), std::string(), boost::bind(&CThievesGuildWindow::bexitf,this), 748, 556, "TPMAGE1.def", SDLK_RETURN);
	statusBar = new CGStatusBar(3, 555, "TStatBar.bmp", 742);

	resdatabar = new CMinorResDataBar();
	resdatabar->pos.x += pos.x;
	resdatabar->pos.y += pos.y;

	static std::vector< std::list< ui8 > > SThievesGuildInfo::* fields[] = { &SThievesGuildInfo::numOfTowns, &SThievesGuildInfo::numOfHeroes, &SThievesGuildInfo::gold,
		&SThievesGuildInfo::woodOre, &SThievesGuildInfo::mercSulfCrystGems, &SThievesGuildInfo::obelisks, &SThievesGuildInfo::artifacts, &SThievesGuildInfo::army,
		&SThievesGuildInfo::income};

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
		std::string text = CGI->generaltexth->jktexts[24+g];
		boost::algorithm::trim_if(text,boost::algorithm::is_any_of("\""));
		printAtMiddle(text, 135, y, FONT_MEDIUM, tytulowy, background);
	}

	CDefHandler * strips = CDefHandler::giveDef("PRSTRIPS.DEF");

	static const std::string colorToBox[] = {"PRRED.BMP", "PRBLUE.BMP", "PRTAN.BMP", "PRGREEN.BMP", "PRORANGE.BMP", "PRPURPLE.BMP", "PRTEAL.BMP", "PRROSE.bmp"};

	for(int g=0; g<tgi.playerColors.size(); ++g)
	{
		if(g > 0)
		{
			blitAt(strips->ourImages[g-1].bitmap, 250 + 66*g, 7, background);
		}
		printAtMiddle(CGI->generaltexth->jktexts[16+g], 283 + 66*g, 24, FONT_BIG, tytulowy, background);
		SDL_Surface * box = BitmapHandler::loadBitmap(colorToBox[tgi.playerColors[g]]);
		blitAt(box, 253 + 66*g, 334, background);
		SDL_FreeSurface(box);
	}

	delete strips;

	CDefHandler * flagPictures = CDefHandler::giveDef("itgflags.def");

	//printing flags
	for(int g=0; g<ARRAY_COUNT(fields); ++g) //by lines
	{
		for(int b=0; b<(tgi .* fields[g]).size(); ++b) //by places (1st, 2nd, ...)
		{
			std::list<ui8> players = (tgi .* fields[g])[b]; //get players with this place in this line
			//std::sort(players.begin(), players.end());

			int counter = 0;
			for(std::list<ui8>::const_iterator it = players.begin(); it != players.end(); ++it)
			{
				int xpos = 259 + 66 * b + 12 * (counter % 4) + 6 * (counter / 4);
				int ypos = 41 + 32 * g + 4 * (counter / 4);
				blitAt(flagPictures->ourImages[*it].bitmap, xpos, ypos, background);
				counter++;
			}
		}
	}
	delete flagPictures;
	flagPictures = NULL;

	//printing best hero

	int counter = 0;
	for(std::map<ui8, InfoAboutHero>::const_iterator it = tgi.colorToBestHero.begin(); it !=  tgi.colorToBestHero.end(); ++it)
	{
		if(it->second.portrait >= 0)
			blitAt(graphics->portraitSmall[it->second.portrait], 260 + 66 * counter, 360, background);
		counter++;

		//printing stats
		if(it->second.details)
		{
			printAtWB(CGI->generaltexth->allTexts[184], 191 + 66*counter, 396, FONT_TINY, 10, zwykly, background);
			for (int i=0; i<it->second.details->primskills.size(); ++i)
			{
				std::ostringstream skill;
				skill << it->second.details->primskills[i];
				printTo(skill.str(), 244 + 66 * counter, 407 + 11*i, FONT_TINY, zwykly, background);
			}
		}

	}

	//printing best creature
	counter = 0;
	for(std::map<ui8, si32>::const_iterator it = tgi.bestCreature.begin(); it !=  tgi.bestCreature.end(); ++it)
	{
		if(it->second >= 0)
			blitAt(graphics->bigImgs[it->second], 255 + 66 * counter, 479, background);
		counter++;
	}

	//printing personality
	counter = 0;
	for(std::map<ui8, si8>::const_iterator it = tgi.personality.begin(); it !=  tgi.personality.end(); ++it)
	{
		int toPrint = 0;
		if(it->second == -1)
		{
			toPrint = 172;
		}
		else
		{
			toPrint = 168 + it->second;
		}

		printAtMiddle(CGI->generaltexth->arraytxt[toPrint], 283 + 66*counter, 459, FONT_SMALL, zwykly, background);

		counter++;
	}

}

CThievesGuildWindow::~CThievesGuildWindow()
{
	SDL_FreeSurface(background);
// 	delete exitb;
// 	delete statusBar;
// 	delete resdatabar;
}

void MoraleLuckBox::set(const IBonusBearer *node)
{
	const int textId[] = {62, 88}; //eg %s \n\n\n {Current Luck Modifiers:}
	const int noneTxtId = 108; //Russian version uses same text for neutral morale\luck
	const int neutralDescr[] = {60, 86}; //eg {Neutral Morale} \n\n Neutral morale means your armies will neither be blessed with extra attacks or freeze in combat.
	const int componentType[] = {SComponent::luck, SComponent::morale};
	const int hoverTextBase[] = {7, 4};
	const Bonus::BonusType bonusType[] = {Bonus::LUCK, Bonus::MORALE};
	int (IBonusBearer::*getValue[])() const = {&IBonusBearer::LuckVal, &IBonusBearer::MoraleVal};

	int mrlt = -9;
	TModDescr mrl;

	
	if (node)
	{
		node->getModifiersWDescr(mrl, bonusType[morale]);
		bonusValue = (node->*getValue[morale])();
	}
	else
		bonusValue = 0;

	mrlt = (bonusValue>0)-(bonusValue<0); //signum: -1 - bad luck / morale, 0 - neutral, 1 - good
	hoverText = CGI->generaltexth->heroscrn[hoverTextBase[morale] - mrlt];
	baseType = componentType[morale];
	text = CGI->generaltexth->arraytxt[textId[morale]];
	boost::algorithm::replace_first(text,"%s",CGI->generaltexth->arraytxt[neutralDescr[morale]-mrlt]);
	if (!mrl.size())
		text += CGI->generaltexth->arraytxt[noneTxtId];
	else
	{
		if (node->hasBonusOfType (Bonus::UNDEAD) || node->hasBonusOfType(Bonus::BLOCK_MORALE) || node->hasBonusOfType(Bonus::NON_LIVING)) //it's a creature window
		{
			text += CGI->generaltexth->arraytxt[113]; //unaffected by morale
		}
		else
		{
			for(int it=0; it < mrl.size(); it++)
				text += "\n" + mrl[it].second;
		}
	}
}

void MoraleLuckBox::showAll(SDL_Surface * to)
{
	CDefEssential *def;
	if (small)
		def = morale ? graphics->morale30 : graphics->luck30;
	else
		def = morale ? graphics->morale42 : graphics->luck42;
	SDL_Surface *img = def->ourImages[bonusValue + 3].bitmap;
	
	blitAt(img, Rect(img).centerIn(pos), to); //put img in the center of our pos
}

MoraleLuckBox::MoraleLuckBox(bool Morale, const Rect &r, bool Small)
	:morale(Morale),
	 small(Small)
{
	bonusValue = 0;
	pos = r + pos;
}

MoraleLuckBox::~MoraleLuckBox()
{

}

void CLabel::showAll(SDL_Surface * to)
{
	CIntObject::showAll(to);
	std::string *hlpText = NULL;  //if NULL, text field will be used
	if(ignoreLeadingWhitespace)
	{
		hlpText = new std::string(text);
		boost::trim_left(*hlpText);
	}

	std::string &toPrint = hlpText ? *hlpText : text;
	if(!toPrint.length())
		return;

	static void (*printer[3])(const std::string &, int, int, EFonts, SDL_Color, SDL_Surface *) = {&CSDL_Ext::printAt, &CSDL_Ext::printAtMiddle, &CSDL_Ext::printTo}; //array of printing functions
	printer[alignment](toPrint, pos.x + textOffset.x, pos.y + textOffset.y, font, color, to);
}

CLabel::CLabel(int x, int y, EFonts Font /*= FONT_SMALL*/, EAlignment Align, const SDL_Color &Color /*= zwykly*/, const std::string &Text /*= ""*/)
:alignment(Align), font(Font), color(Color), text(Text)
{
	autoRedraw = true;
	pos.x += x;
	pos.y += y;
	pos.w = pos.h = 0;
	bg = NULL;
}

void CLabel::setTxt(const std::string &Txt)
{
	text = Txt;
	if(autoRedraw)
	{
		if(bg || !parent)
			redraw();
		else
			parent->redraw();
	}
}

CTextBox::CTextBox(std::string Text, const Rect &rect, int SliderStyle, EFonts Font /*= FONT_SMALL*/, EAlignment Align /*= TOPLEFT*/, const SDL_Color &Color /*= zwykly*/)
	:CLabel(rect.x, rect.y, Font, Align, Color, Text), sliderStyle(SliderStyle), slider(NULL)
{
	redrawParentOnScrolling = false;
	autoRedraw = false;
	pos.h = rect.h;
	pos.w = rect.w;
	assert(Align == TOPLEFT || Align == CENTER); //TODO: support for other alignments
	assert(pos.w >= 80); //we need some space
	setTxt(Text);
}

void CTextBox::showAll(SDL_Surface * to)
{
	CIntObject::showAll(to);

	const Font &f = *graphics->fonts[font];
	int dy = f.height; //line height
	int base_y = pos.y;
	if(alignment == CENTER)
		base_y += std::max((pos.h - maxH)/2,0);

	int howManyLinesToPrint = slider ? slider->capacity : lines.size();
	int firstLineToPrint = slider ? slider->value : 0;

	for (int i = 0; i < howManyLinesToPrint; i++)
	{
		const std::string &line = lines[i + firstLineToPrint];
		if(!line.size()) continue;

		int x = pos.x;
		if(alignment == CENTER)
		{
			x += (pos.w - f.getWidth(line.c_str())) / 2;
			if(slider)
				x -= slider->pos.w / 2 + 5;
		}

		if(line[0] == '{' && line[line.size()-1] == '}')
			printAt(line, x, base_y + i*dy, font, tytulowy, to);
		else
			printAt(line, x, base_y + i*dy, font, color, to);
	}

}

void CTextBox::setTxt(const std::string &Txt)
{
	recalculateLines(Txt);
	CLabel::setTxt(Txt);
}

void CTextBox::sliderMoved(int to)
{
	if(!slider)
		return;

	if(redrawParentOnScrolling)
		parent->redraw();
	redraw();
}

void CTextBox::setBounds(int limitW, int limitH)
{
	pos.h = limitH;
	pos.w = limitW;
	recalculateLines(text);
}

void CTextBox::recalculateLines(const std::string &Txt)
{
	delChildNUll(slider, true);
	lines.clear();

	const Font &f = *graphics->fonts[font];
	int lineHeight =  f.height; 
	int lineCapacity = pos.h / lineHeight;

	lines = CMessage::breakText(Txt, pos.w, font);
	if(lines.size() > lineCapacity) //we need to add a slider
	{
		lines = CMessage::breakText(Txt, pos.w - 32 - 10, font);
		OBJ_CONSTRUCTION_CAPTURING_ALL;
		slider = new CSlider(pos.w - 32, 0, pos.h, boost::bind(&CTextBox::sliderMoved, this, _1), lineCapacity, lines.size(), 0, false, sliderStyle);
		if(active)
			slider->activate();
	}

	maxH = lineHeight * lines.size();
	maxW = 0;
	BOOST_FOREACH(const std::string &line, lines)
		amax(maxW, f.getWidth(line.c_str()));
}

void CGStatusBar::print(const std::string & Text)
{
	setTxt(Text);
}

void CGStatusBar::clear()
{
	setTxt("");
}

std::string CGStatusBar::getCurrent()
{
	return text;
}

CGStatusBar::CGStatusBar(int x, int y, EFonts Font /*= FONT_SMALL*/, EAlignment Align, const SDL_Color &Color /*= zwykly*/, const std::string &Text /*= ""*/)
	: CLabel(x, y, Font, Align, Color, Text)
{
	init();
}

CGStatusBar::CGStatusBar(CPicture *BG, EFonts Font /*= FONT_SMALL*/, EAlignment Align /*= CENTER*/, const SDL_Color &Color /*= zwykly*/)
	: CLabel(BG->pos.x, BG->pos.y, Font, Align, Color, "")
{
	init();
	bg = BG;
	moveChild(bg, bg->parent, this);
	pos = bg->pos;
	calcOffset();
}

CGStatusBar::CGStatusBar(int x, int y, std::string name/*="ADROLLVR.bmp"*/, int maxw/*=-1*/)
	: CLabel(x, y, FONT_SMALL, CENTER)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL;
	init();
	bg = new CPicture(name);
	pos = bg->pos;
	if(maxw < pos.w)
	{
		amin(pos.w, maxw);
		bg->srcRect = new Rect(0, 0, maxw, pos.h);
	}
	calcOffset();
}

CGStatusBar::~CGStatusBar()
{
	GH.statusbar = oldStatusBar;
}

void CGStatusBar::show(SDL_Surface * to)
{

}

void CGStatusBar::init()
{
	oldStatusBar = GH.statusbar;
	GH.statusbar = this;
}

void CGStatusBar::calcOffset()
{
	switch(alignment)
	{
	case CENTER:
		textOffset = Point(pos.w/2, pos.h/2);
		break;
	case BOTTOMRIGHT:
		textOffset = Point(pos.w, pos.h);
		break;
	}
}

CTextInput::CTextInput( const Rect &Pos, const Point &bgOffset, const std::string &bgName, const CFunctionList<void(const std::string &)> &CB )
:cb(CB)
{
	focus = false;
	pos += Pos;
	captureAllKeys = true;
	OBJ_CONSTRUCTION;
	bg = new CPicture(bgName, bgOffset.x, bgOffset.y);
	used = LCLICK | KEYBOARD;
	giveFocus();
}

CTextInput::CTextInput(const Rect &Pos, SDL_Surface *srf)
{
	focus = false;
	pos += Pos;
	captureAllKeys = true;
	OBJ_CONSTRUCTION;
	bg = new CPicture(Pos, 0, true);
	Rect hlp = Pos;
	if(srf)
		CSDL_Ext::blitSurface(srf, &hlp, *bg, NULL);
	else
		SDL_FillRect(*bg, NULL, 0);
	pos.w = bg->pos.w;
	pos.h = bg->pos.h;
	bg->pos = pos;
	used = LCLICK | KEYBOARD;
	giveFocus();
}

void CTextInput::showAll( SDL_Surface * to )
{
	CIntObject::showAll(to);
	const std::string toPrint = focus ? text + "_" : text;
	CSDL_Ext::printAt(toPrint, pos.x, pos.y, FONT_SMALL, zwykly, to);
}

void CTextInput::clickLeft( tribool down, bool previousState )
{
	if(down && !focus)
		giveFocus();
}

void CTextInput::keyPressed( const SDL_KeyboardEvent & key )
{
	if(!focus || key.state != SDL_PRESSED) 
		return;

	if(key.keysym.sym == SDLK_TAB)
	{
		moveFocus();
		GH.breakEventHandling();
		return;
	}

	switch(key.keysym.sym)
	{
	case SDLK_BACKSPACE:
		if(text.size())
			text.resize(text.size()-1);
		break;
	default:
		char c = key.keysym.unicode; //TODO 16-/>8
		static const std::string forbiddenChars = "<>:\"/\\|?*"; //if we are entering a filename, some special characters won't be allowed
		if(!vstd::contains(forbiddenChars,c) && std::isprint(c))
			text += c;
		break;
	}
	redraw();
	cb(text);
}

void CTextInput::setText( const std::string &nText, bool callCb )
{
	text = nText;
	redraw();
	if(callCb)
		cb(text);
}

CTextInput::~CTextInput()
{
}

CFocusable::CFocusable()
{
	focusables.push_back(this);
}

CFocusable::~CFocusable()
{
	if(inputWithFocus == this)
		inputWithFocus = NULL;

	focusables -= this;
}
void CFocusable::giveFocus()
{
	if(inputWithFocus)
	{
		inputWithFocus->focus = false;
		inputWithFocus->redraw();
	}

	focus = true;
	inputWithFocus = this;
	redraw();
}

void CFocusable::moveFocus()
{
	std::list<CFocusable*>::iterator i = vstd::find(focusables, this),
									ourIt = i;
	for(i++; i != ourIt; i++)
	{
		if(i == focusables.end())
			i = focusables.begin();

		if((*i)->active)
		{
			(*i)->giveFocus();
			break;;
		}
	}
}

CArtifactHolder::CArtifactHolder()
{
	type |= WITH_ARTIFACTS;
}

void CWindowWithArtifacts::artifactRemoved(const ArtifactLocation &artLoc)
{
	BOOST_FOREACH(CArtifactsOfHero *aoh, artSets)
		aoh->artifactRemoved(artLoc);
}

void CWindowWithArtifacts::artifactMoved(const ArtifactLocation &artLoc, const ArtifactLocation &destLoc)
{
	BOOST_FOREACH(CArtifactsOfHero *aoh, artSets)
		aoh->artifactMoved(artLoc, destLoc);
}

void CWindowWithArtifacts::artifactDisassembled(const ArtifactLocation &artLoc)
{
	BOOST_FOREACH(CArtifactsOfHero *aoh, artSets)
		aoh->artifactDisassembled(artLoc);
}

void CWindowWithArtifacts::artifactAssembled(const ArtifactLocation &artLoc)
{
	BOOST_FOREACH(CArtifactsOfHero *aoh, artSets)
		aoh->artifactAssembled(artLoc);
}

void CArtifactsOfHero::SCommonPart::Artpos::clear()
{
	slotID = -1;
	AOH = NULL;
	art = NULL;
}

CArtifactsOfHero::SCommonPart::Artpos::Artpos()
{
	clear();
}

void CArtifactsOfHero::SCommonPart::Artpos::setTo(const CArtPlace *place, bool dontTakeBackpack)
{
	slotID = place->slotID;
	AOH = place->ourOwner;

	if(slotID >= 19 && dontTakeBackpack)
		art = NULL;
	else
		art = place->ourArt;
}

bool CArtifactsOfHero::SCommonPart::Artpos::operator==(const ArtifactLocation &al) const
{
	if(!AOH)
		return false;
	bool ret = al.hero == AOH->curHero  &&  al.slot == slotID;

	//assert(al.getArt() == art);
	return ret;
}

bool CArtifactsOfHero::SCommonPart::Artpos::valid()
{
	assert(AOH && art);
	return art == AOH->curHero->getArt(slotID);
}
