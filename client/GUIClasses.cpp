#include "StdInc.h"
#include "GUIClasses.h"
#include "UIFramework/SDL_Extensions.h"

#include "CAdvmapInterface.h"
#include "BattleInterface/CBattleInterface.h"
#include "BattleInterface/CBattleInterfaceClasses.h"
#include "../CCallback.h"
#include "CCastleInterface.h"
#include "CCreatureWindow.h"
#include "UIFramework/CCursorHandler.h"
#include "CGameInfo.h"
#include "CHeroWindow.h"
#include "CMessage.h"
#include "../lib/CConfigHandler.h"
#include "BattleInterface/CCreatureAnimation.h"
#include "CPlayerInterface.h"
#include "Graphics.h"
#include "CAnimation.h"
#include "../lib/CArtHandler.h"
#include "../lib/CBuildingHandler.h"
#include "../lib/CGeneralTextHandler.h"
#include "../lib/CHeroHandler.h"
#include "../lib/CModHandler.h"
#include "../lib/CObjectHandler.h"
#include "../lib/CSpellHandler.h"
#include "../lib/CTownHandler.h"
#include "../lib/CondSh.h"
#include "../lib/Mapping/CMap.h"
#include "mapHandler.h"
#include "../lib/CStopWatch.h"
#include "../lib/NetPacks.h"
#include "CSpellWindow.h"
#include "CHeroWindow.h"
#include "CVideoHandler.h"
#include "../lib/StartInfo.h"
#include "CPreGame.h"
#include "../lib/HeroBonus.h"
#include "../lib/CCreatureHandler.h"
#include "CMusicHandler.h"
#include "../lib/BattleState.h"
#include "../lib/CGameState.h"
#include "../lib/GameConstants.h"
#include "UIFramework/CGuiHandler.h"

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

extern std::queue<SDL_Event> events;
extern boost::mutex eventsM;

std::list<CFocusable*> CFocusable::focusables;
CFocusable * CFocusable::inputWithFocus;

#undef min
#undef max

void CArmyTooltip::init(const InfoAboutArmy &army)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL;

	new CLabel(66, 2, FONT_SMALL, TOPLEFT, Colors::WHITE, army.name);

	std::vector<Point> slotsPos;
	slotsPos.push_back(Point(36,73));
	slotsPos.push_back(Point(72,73));
	slotsPos.push_back(Point(108,73));
	slotsPos.push_back(Point(18,122));
	slotsPos.push_back(Point(54,122));
	slotsPos.push_back(Point(90,122));
	slotsPos.push_back(Point(126,122));

	BOOST_FOREACH(auto & slot, army.army)
	{
		if(slot.first.getNum() >= GameConstants::ARMY_SIZE)
		{
			tlog3 << "Warning: " << army.name << " has stack in slot " << slot.first << std::endl;
			continue;
		}

		new CAnimImage("CPRSMALL", slot.second.type->iconIndex, 0, slotsPos[slot.first.getNum()].x, slotsPos[slot.first.getNum()].y);

		std::string subtitle;
		if(army.army.isDetailed)
			subtitle = boost::lexical_cast<std::string>(slot.second.count);
		else
		{
			//if =0 - we have no information about stack size at all
			if (slot.second.count)
				subtitle = CGI->generaltexth->arraytxt[171 + 3*(slot.second.count)];
		}

		new CLabel(slotsPos[slot.first.getNum()].x + 17, slotsPos[slot.first.getNum()].y + 41, FONT_TINY, CENTER, Colors::WHITE, subtitle);
	}

}

CArmyTooltip::CArmyTooltip(Point pos, const InfoAboutArmy &army):
    CIntObject(0, pos)
{
	init(army);
}

CArmyTooltip::CArmyTooltip(Point pos, const CArmedInstance * army):
    CIntObject(0, pos)
{
	init(InfoAboutArmy(army, true));
}

void CHeroTooltip::init(const InfoAboutHero &hero)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL;
	new CAnimImage("PortraitsLarge", hero.portrait, 0, 3, 2);

	if(hero.details)
	{
		for (size_t i = 0; i < hero.details->primskills.size(); i++)
			new CLabel(75 + 28 * i, 58, FONT_SMALL, CENTER, Colors::WHITE,
			           boost::lexical_cast<std::string>(hero.details->primskills[i]));

		new CLabel(158, 98, FONT_TINY, CENTER, Colors::WHITE,
		           boost::lexical_cast<std::string>(hero.details->mana));

		new CAnimImage("IMRL22", hero.details->morale + 3, 0, 5, 74);
		new CAnimImage("ILCK22", hero.details->luck + 3, 0, 5, 91);
	}
}

CHeroTooltip::CHeroTooltip(Point pos, const InfoAboutHero &hero):
    CArmyTooltip(pos, hero)
{
	init(hero);
}

CHeroTooltip::CHeroTooltip(Point pos, const CGHeroInstance * hero):
    CArmyTooltip(pos, InfoAboutHero(hero, true))
{
	init(InfoAboutHero(hero, true));
}

void CTownTooltip::init(const InfoAboutTown &town)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL;

	//order of icons in def: fort, citadel, castle, no fort
	size_t fortIndex = town.fortLevel ? town.fortLevel - 1 : 3;

	new CAnimImage("ITMCLS", fortIndex, 0, 105, 31);

	assert(town.tType);

	size_t iconIndex = town.tType->clientInfo.icons[town.fortLevel > 0][town.built >= CGI->modh->settings.MAX_BUILDING_PER_TURN];

	new CAnimImage("itpt", iconIndex, 0, 3, 2);

	if(town.details)
	{
		if (town.details->hallLevel)
			new CAnimImage("ITMTLS", town.details->hallLevel, 0, 67, 31);

		if (town.details->goldIncome)
			new CLabel(157, 58, FONT_TINY, CENTER, Colors::WHITE,
		               boost::lexical_cast<std::string>(town.details->goldIncome));

		if(town.details->garrisonedHero) //garrisoned hero icon
			new CPicture("TOWNQKGH", 149, 76);

		if(town.details->customRes)//silo is built
		{
			if (town.tType->primaryRes == 127 )// wood & ore
			{
				new CAnimImage("SMALRES", Res::WOOD, 0, 7, 75);
				new CAnimImage("SMALRES", Res::ORE , 0, 7, 88);
			}
			else
				new CAnimImage("SMALRES", town.tType->primaryRes, 0, 7, 81);
		}
	}
}

CTownTooltip::CTownTooltip(Point pos, const InfoAboutTown &town):
    CArmyTooltip(pos, town)
{
	init(town);
}

CTownTooltip::CTownTooltip(Point pos, const CGTownInstance * town):
    CArmyTooltip(pos, InfoAboutTown(town, true))
{
	init(InfoAboutTown(town, true));
}

void CGarrisonSlot::setHighlight(bool on)
{
	if (on)
		selectionImage->enable(); //show
	else
		selectionImage->disable(); //hide
}

void CGarrisonSlot::hover (bool on)
{
	////Hoverable::hover(on);
	if(on)
	{
		std::string temp;
		if(creature)
		{
			if(owner->getSelection())
			{
				if(owner->getSelection() == this)
				{
					temp = CGI->generaltexth->tcommands[4]; //View %s
					boost::algorithm::replace_first(temp,"%s",creature->nameSing);
				}
				else if (owner->getSelection()->creature == creature)
				{
					temp = CGI->generaltexth->tcommands[2]; //Combine %s armies
					boost::algorithm::replace_first(temp,"%s",creature->nameSing);
				}
				else if (owner->getSelection()->creature)
				{
					temp = CGI->generaltexth->tcommands[7]; //Exchange %s with %s
					boost::algorithm::replace_first(temp,"%s",owner->getSelection()->creature->nameSing);
					boost::algorithm::replace_first(temp,"%s",creature->nameSing);
				}
				else
				{
					tlog2 << "Warning - shouldn't be - highlighted void slot "<<owner->getSelection()<<std::endl;
					tlog2 << "Highlighted set to NULL"<<std::endl;
					owner->selectSlot(nullptr);
				}
			}
			else
			{
				if(upg)
				{
					temp = CGI->generaltexth->tcommands[32]; //Select %s (visiting)
				}
				else if(owner->armedObjs[0] && owner->armedObjs[0]->ID == Obj::TOWN)
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
			if(owner->getSelection())
			{
				const CArmedInstance *highl = owner->getSelection()->getObj();
				if(  highl->needsLastStack()		//we are moving stack from hero's
				  && highl->stacksCount() == 1	//it's only stack
				  && owner->getSelection()->upg != upg	//we're moving it to the other garrison
				  )
				{
					temp = CGI->generaltexth->tcommands[5]; //Cannot move last army to garrison
				}
				else
				{
					temp = CGI->generaltexth->tcommands[6]; //Move %s
					boost::algorithm::replace_first(temp,"%s",owner->getSelection()->creature->nameSing);
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

const CArmedInstance * CGarrisonSlot::getObj() const
{
	return 	(!upg)?(owner->armedObjs[0]):(owner->armedObjs[1]);
}

bool CGarrisonSlot::our() const
{
	return 	upg?(owner->owned[1]):(owner->owned[0]);
}

void CGarrisonSlot::clickRight(tribool down, bool previousState)
{
	if(down && creature)
	{
		GH.pushInt(createCreWindow(myStack, CCreatureWindow::ARMY));
	}
}
void CGarrisonSlot::clickLeft(tribool down, bool previousState)
{
	if(down)
	{
		bool refr = false;
		if(owner->getSelection())
		{
			if(owner->getSelection() == this) //view info
			{
				UpgradeInfo pom;
				LOCPLINT->cb->getUpgradeInfo(getObj(), ID, pom);

				bool canUpgrade = getObj()->tempOwner == LOCPLINT->playerID && pom.oldID>=0; //upgrade is possible
				bool canDismiss = getObj()->tempOwner == LOCPLINT->playerID && (getObj()->stacksCount()>1  || !getObj()->needsLastStack());
				boost::function<void()> upgr = NULL;
				boost::function<void()> dism = NULL;
				if (canUpgrade) upgr = boost::bind(&CCallback::upgradeCreature, LOCPLINT->cb, getObj(), ID, pom.newID[0]);
				if (canDismiss) dism = boost::bind(&CCallback::dismissCreature, LOCPLINT->cb, getObj(), ID);

				owner->selectSlot(nullptr);
				owner->setSplittingMode(false);

				for(size_t i = 0; i<owner->splitButtons.size(); i++)
					owner->splitButtons[i]->block(true);

				redraw();
				refr = true;
				CIntObject *creWindow = createCreWindow(myStack, CCreatureWindow::HERO, upgr, dism, &pom);
				GH.pushInt(creWindow);
			}
			else
			{
				// Only allow certain moves if troops aren't removable or not ours.
				if (  ( owner->getSelection()->our()//our creature is selected
				     || owner->getSelection()->creature == creature )//or we are rebalancing army
				   && ( owner->removableUnits
				     || (upg == 0 &&  ( owner->getSelection()->upg == 1 && !creature ) )
					 || (upg == 1 &&    owner->getSelection()->upg == 1 ) ) )
				{
					//we want to split
					if((owner->getSplittingMode() || LOCPLINT->shiftPressed())
						&& (!creature
							|| (creature == owner->getSelection()->creature)))
					{
						owner->p2 = ID; //store the second stack pos
						owner->pb = upg;//store the second stack owner (up or down army)
						owner->setSplittingMode(false);

						int minLeft=0, minRight=0;

						if(upg != owner->getSelection()->upg) //not splitting within same army
						{
							if(owner->getSelection()->getObj()->stacksCount() == 1 //we're splitting away the last stack
								&& owner->getSelection()->getObj()->needsLastStack() )
							{
								minLeft = 1;
							}
							if(getObj()->stacksCount() == 1 //destination army can't be emptied, unless we're rebalancing two stacks of same creature
								&& owner->getSelection()->creature == creature
								&& getObj()->needsLastStack() )
							{
								minRight = 1;
							}
						}

						int countLeft = owner->getSelection()->myStack ? owner->getSelection()->myStack->count : 0;
						int countRight = myStack ? myStack->count : 0;

						GH.pushInt(new CSplitWindow(owner->getSelection()->creature, boost::bind(&CGarrisonInt::splitStacks, owner, _1, _2),
						                            minLeft, minRight, countLeft, countRight));
						refr = true;
					}
					else if(creature != owner->getSelection()->creature) //swap
					{
						LOCPLINT->cb->swapCreatures(
							(!upg)?(owner->armedObjs[0]):(owner->armedObjs[1]),
							(!owner->getSelection()->upg)?(owner->armedObjs[0]):(owner->armedObjs[1]),
							ID,owner->getSelection()->ID);
					}
					else //merge
					{
						LOCPLINT->cb->mergeStacks(
							(!owner->getSelection()->upg)?(owner->armedObjs[0]):(owner->armedObjs[1]),
							(!upg)?(owner->armedObjs[0]):(owner->armedObjs[1]),
							owner->getSelection()->ID,ID);
					}
				}
				else // Highlight
				{
					if(creature)
						owner->selectSlot(this);
					redraw();
					refr = true;
				}
			}
		}
		else //highlight or drop artifact
		{
			bool artSelected = false;
			if (CWindowWithArtifacts* chw = dynamic_cast<CWindowWithArtifacts*>(GH.topInt())) //dirty solution
			{
				const CArtifactsOfHero::SCommonPart *commonInfo = chw->artSets.front()->commonInfo;
				if (const CArtifactInstance *art = commonInfo->src.art)
				{
					const CGHeroInstance *srcHero = commonInfo->src.AOH->getHero();
					artSelected = true;
					ArtifactLocation src(srcHero, commonInfo->src.slotID);
					ArtifactLocation dst(myStack, ArtifactPosition::CREATURE_SLOT);
					if (art->canBePutAt(dst, true))
					{	//equip clicked stack
						if(dst.getArt())
						{
							//creature can wear only one active artifact
							//if we are placing a new one, the old one will be returned to the hero's backpack
							LOCPLINT->cb->swapArtifacts(dst, ArtifactLocation(srcHero, dst.getArt()->firstBackpackSlot(srcHero)));
						}
						LOCPLINT->cb->swapArtifacts(src, dst);
					}
				}
			}
			if (!artSelected && creature)
			{
				owner->selectSlot(this);
				if(creature)
				{
					for(size_t i = 0; i<owner->splitButtons.size(); i++)
						owner->splitButtons[i]->block(false);
				}
			}
			redraw();
			refr = true;
		}
		if(refr) {hover(false);	hover(true); } //to refresh statusbar
	}
}

void CGarrisonSlot::update()
{
	if (getObj() != nullptr)
	{
		addUsedEvents(LCLICK | RCLICK | HOVER);
		myStack = getObj()->getStackPtr(ID);
		creature = myStack ? myStack->type : NULL;
	}
	else
	{
		removeUsedEvents(LCLICK | RCLICK | HOVER);
		myStack = nullptr;
		creature = nullptr;
	}

	if (creature)
	{
		creatureImage->enable();
		creatureImage->setFrame(creature->iconIndex);

		stackCount->enable();
		stackCount->setTxt(boost::lexical_cast<std::string>(myStack->count));
	}
	else
	{
		creatureImage->disable();
		stackCount->disable();
	}
}

CGarrisonSlot::CGarrisonSlot(CGarrisonInt *Owner, int x, int y, SlotID IID, int Upg, const CStackInstance * Creature):
    ID(IID),
    owner(Owner),
    myStack(Creature),
    creature(Creature ? Creature->type : nullptr),
    upg(Upg)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL;
	if (getObj())
		addUsedEvents(LCLICK | RCLICK | HOVER);
	pos.x += x;
	pos.y += y;

	std::string imgName = owner->smallIcons ? "cprsmall" : "TWCRPORT";

	creatureImage = new CAnimImage(imgName, creature ? creature->iconIndex : 0);
	if (!creature)
		creatureImage->disable();

	selectionImage = new CAnimImage(imgName, 1);
	selectionImage->disable();

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

	stackCount = new CLabel(pos.w, pos.h, owner->smallIcons ? FONT_TINY : FONT_MEDIUM, BOTTOMRIGHT, Colors::WHITE);
	if (!creature)
		stackCount->disable();
	else
		stackCount->setTxt(boost::lexical_cast<std::string>(myStack->count));
}

void CGarrisonInt::addSplitBtn(CAdventureMapButton * button)
{
	addChild(button);
	button->recActions = defActions;
	splitButtons.push_back(button);
	button->block(getSelection() == nullptr);
}

void CGarrisonInt::createSet(std::vector<CGarrisonSlot*> &ret, const CCreatureSet * set, int posX, int posY, int distance, int Upg )
{
	ret.resize(7);

	if (set)
	{
		for(TSlots::const_iterator i=set->Slots().begin(); i!=set->Slots().end(); i++)
		{
			ret[i->first.getNum()] = new CGarrisonSlot(this, posX + (i->first.getNum()*distance), posY, i->first, Upg, i->second);
		}
	}

	for(int i=0; i<ret.size(); i++)
		if(!ret[i])
			ret[i] = new CGarrisonSlot(this, posX + (i*distance), posY, SlotID(i), Upg, NULL);

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

	int width = smallIcons? 32 : 58;

	createSet(slotsUp, armedObjs[0], 0, 0, width+interx, 0);
	createSet(slotsDown, armedObjs[1], garOffset.x, garOffset.y, width+interx, 1);
}

void CGarrisonInt::recreateSlots()
{
	setSplittingMode(false);
	selectSlot(nullptr);

	for(size_t i = 0; i<splitButtons.size(); i++)
		splitButtons[i]->block(true);


	BOOST_FOREACH(CGarrisonSlot * slot, slotsUp)
		slot->update();

	BOOST_FOREACH(CGarrisonSlot * slot, slotsDown)
		slot->update();
}

void CGarrisonInt::splitClick()
{
	if(!getSelection())
		return;
	setSplittingMode(!getSplittingMode());
	redraw();
}
void CGarrisonInt::splitStacks(int, int amountRight)
{
	LOCPLINT->cb->splitStack(armedObjs[getSelection()->upg], armedObjs[pb], getSelection()->ID, p2, amountRight);
}

CGarrisonInt::CGarrisonInt(int x, int y, int inx, const Point &garsOffset,
                           SDL_Surface *pomsur, const Point& SurOffset,
                           const CArmedInstance *s1, const CArmedInstance *s2,
                           bool _removableUnits, bool smallImgs, bool _twoRows ) :
    highlighted(nullptr),
    inSplittingMode(false),
    interx(inx),
    garOffset(garsOffset),
    smallIcons(smallImgs),
    removableUnits(_removableUnits),
    twoRows(_twoRows)
{
	setArmy(s1, false);
	setArmy(s2, true);
	pos.x += x;
	pos.y += y;
	createSlots();
}

const CGarrisonSlot * CGarrisonInt::getSelection()
{
	return highlighted;
}

void CGarrisonInt::selectSlot(CGarrisonSlot *slot)
{
	if (slot != highlighted)
	{
		if (highlighted)
			highlighted->setHighlight(false);

		highlighted = slot;
		BOOST_FOREACH (auto button, splitButtons)
			button->block(highlighted == nullptr);

		if (highlighted)
			highlighted->setHighlight(true);
	}
}

void CGarrisonInt::setSplittingMode(bool on)
{
	assert(on == false || highlighted != nullptr); //can't be in splitting mode without selection

	if (inSplittingMode || on)
	{
		BOOST_FOREACH(CGarrisonSlot * slot, slotsUp)
			slot->setHighlight(slot->creature == nullptr || slot->creature == getSelection()->creature);

		BOOST_FOREACH(CGarrisonSlot * slot, slotsDown)
			slot->setHighlight(slot->creature == nullptr || slot->creature == getSelection()->creature);
		inSplittingMode = on;
	}
}

bool CGarrisonInt::getSplittingMode()
{
	return inSplittingMode;
}

void CGarrisonInt::setArmy(const CArmedInstance *army, bool bottomGarrison)
{
	owned[bottomGarrison] =  army ? (army->tempOwner == LOCPLINT->playerID || army->tempOwner == PlayerColor::UNFLAGGABLE) : false;
	armedObjs[bottomGarrison] = army;
}

CInfoWindow::CInfoWindow(std::string Text, PlayerColor player, const TCompsInfo &comps, const TButtonsInfo &Buttons, bool delComps)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL;

	type |= BLOCK_ADV_HOTKEYS;
	ID = -1;
	for(int i=0;i<Buttons.size();i++)
	{
		CAdventureMapButton *button = new CAdventureMapButton("","",boost::bind(&CInfoWindow::close,this),0,0,Buttons[i].first);
		button->borderColor = Colors::METALLIC_GOLD;
		button->borderEnabled = true;
		button->callback.add(Buttons[i].second); //each button will close the window apart from call-defined actions
		buttons.push_back(button);
	}

	text = new CTextBox(Text, Rect(0, 0, 250, 100), 0, FONT_MEDIUM, CENTER, Colors::WHITE);
	if(!text->slider)
	{
		text->pos.w = text->maxW;
		text->pos.h = text->maxH;
	}

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

void CInfoWindow::showAll(SDL_Surface * to)
{
	CSimpleWindow::show(to);
	CIntObject::showAll(to);
}

void CInfoWindow::showYesNoDialog(const std::string & text, const std::vector<CComponent*> *components, const CFunctionList<void( ) > &onYes, const CFunctionList<void()> &onNo, bool DelComps, PlayerColor player)
{
	assert(!LOCPLINT || LOCPLINT->showingDialog->get());
	std::vector<std::pair<std::string,CFunctionList<void()> > > pom;
	pom.push_back(std::pair<std::string,CFunctionList<void()> >("IOKAY.DEF",0));
	pom.push_back(std::pair<std::string,CFunctionList<void()> >("ICANCEL.DEF",0));
	CInfoWindow * temp = new CInfoWindow(text, player, components ? *components : std::vector<CComponent*>(), pom, DelComps);
	for(int i=0;i<onYes.funcs.size();i++)
		temp->buttons[0]->callback += onYes.funcs[i];
	for(int i=0;i<onNo.funcs.size();i++)
		temp->buttons[1]->callback += onNo.funcs[i];

	GH.pushInt(temp);
}

CInfoWindow * CInfoWindow::create(const std::string &text, PlayerColor playerID /*= 1*/, const std::vector<CComponent*> *components /*= NULL*/, bool DelComps)
{
	std::vector<std::pair<std::string,CFunctionList<void()> > > pom;
	pom.push_back(std::pair<std::string,CFunctionList<void()> >("IOKAY.DEF",0));
	CInfoWindow * ret = new CInfoWindow(text, playerID, components ? *components : std::vector<CComponent*>(), pom, DelComps);
	return ret;
}

std::string CInfoWindow::genText(std::string title, std::string description)
{
	return std::string("{") + title + "}" + "\n\n" + description;
}

void CInfoWindow::setDelComps(bool DelComps)
{
	delComps = DelComps;
	BOOST_FOREACH(CComponent *comp, components)
	{
		if(delComps)
			comp->recActions |= DISPOSE;
		else
			comp->recActions &= ~DISPOSE;
	}
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
	vstd::amax(pos.x, 0);
	vstd::amax(pos.y, 0);
	vstd::amin(pos.x, screen->w - bitmap->w);
	vstd::amin(pos.y, screen->h - bitmap->h);
}

CComponent::CComponent(Etype Type, int Subtype, int Val, ESize imageSize):
	image(nullptr),
    perDay(false)
{
	addUsedEvents(RCLICK);
	init(Type, Subtype, Val, imageSize);
}

CComponent::CComponent(const Component &c):
	image(nullptr),
    perDay(false)
{
	addUsedEvents(RCLICK);

	if(c.id == Component::RESOURCE && c.when==-1)
		perDay = true;

	init((Etype)c.id,c.subtype,c.val, large);
}

void CComponent::init(Etype Type, int Subtype, int Val, ESize imageSize)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL;

	compType = Type;
	subtype = Subtype;
	val = Val;
	size = imageSize;

	assert(compType < typeInvalid);
	assert(size < sizeInvalid);

	setSurface(getFileName()[size], getIndex());

	pos.w = image->pos.w;
	pos.h = image->pos.h;

	EFonts font = FONT_SMALL;
	if (imageSize < small)
		font = FONT_TINY; //other sizes?

	pos.h += 4; //distance between text and image

	std::vector<std::string> textLines = CMessage::breakText(getSubtitle(), std::max<int>(80, pos.w), font);
	BOOST_FOREACH(auto & line, textLines)
	{
		int height = graphics->fonts[font]->getLineHeight();
		CLabel * label = new CLabel(pos.w/2, pos.h + height/2, font, CENTER, Colors::WHITE, line);

		pos.h += height;
		if (label->pos.w > pos.w)
		{
			pos.x -= (label->pos.w - pos.w)/2;
			pos.w = label->pos.w;
		}
	}
}

const std::vector<std::string> CComponent::getFileName()
{
	static const std::string  primSkillsArr [] = {"PSKIL32",        "PSKIL32",        "PSKIL42",        "PSKILL"};
	static const std::string  secSkillsArr [] =  {"SECSK32",        "SECSK32",        "SECSKILL",       "SECSK82"};
	static const std::string  resourceArr [] =   {"SMALRES",        "RESOURCE",       "RESOUR82",       "RESOUR82"};
	static const std::string  creatureArr [] =   {"CPRSMALL",       "CPRSMALL",       "TWCRPORT",       "TWCRPORT"};
	static const std::string  artifactArr[]  =   {"Artifact",       "Artifact",       "Artifact",       "Artifact"};
	static const std::string  spellsArr [] =     {"SpellInt",       "SpellInt",       "SPELLSCR",       "SPELLSCR"};
	static const std::string  moraleArr [] =     {"IMRL22",         "IMRL30",         "IMRL42",         "imrl82"};
	static const std::string  luckArr [] =       {"ILCK22",         "ILCK30",         "ILCK42",         "ilck82"};
	static const std::string  heroArr [] =       {"PortraitsSmall", "PortraitsSmall", "PortraitsLarge", "PortraitsLarge"};
	static const std::string  flagArr [] =       {"CREST58",        "CREST58",        "CREST58",        "CREST58"};

	auto gen = [](const std::string * arr)
	{
		return std::vector<std::string>(arr, arr + 4);
	};

	switch(compType)
	{
	case primskill:  return gen(primSkillsArr);
	case secskill:   return gen(secSkillsArr);
	case resource:   return gen(resourceArr);
	case creature:   return gen(creatureArr);
	case artifact:   return gen(artifactArr);
	case experience: return gen(primSkillsArr);
	case spell:      return gen(spellsArr);
	case morale:     return gen(moraleArr);
	case luck:       return gen(luckArr);
	case building:   return std::vector<std::string>(4, CGI->townh->towns[subtype].clientInfo.buildingsIcons);
	case hero:       return gen(heroArr);
	case flag:       return gen(flagArr);
	}
	assert(0);
	return std::vector<std::string>();
}

size_t CComponent::getIndex()
{
	switch(compType)
	{
	case primskill:  return subtype;
	case secskill:   return subtype*3 + 3 + val - 1;
	case resource:   return subtype;
	case creature:   return CGI->creh->creatures[subtype]->iconIndex;
	case artifact:   return CGI->arth->artifacts[subtype]->iconIndex;
	case experience: return 4;
	case spell:      return subtype;
	case morale:     return val+3;
	case luck:       return val+3;
	case building:   return val;
	case hero:       return CGI->heroh->heroes[subtype]->imageIndex;
	case flag:       return subtype;
	}
	assert(0);
	return 0;
}

std::string CComponent::getDescription()
{
	switch (compType)
	{
	case primskill:  return (subtype < 4)? CGI->generaltexth->arraytxt[2+subtype] //Primary skill
	                                     : CGI->generaltexth->allTexts[149]; //mana
	case secskill:   return CGI->generaltexth->skillInfoTexts[subtype][val-1];
	case resource:   return CGI->generaltexth->allTexts[242];
	case creature:   return "";
	case artifact:   return CGI->arth->artifacts[subtype]->Description();
	case experience: return CGI->generaltexth->allTexts[241];
	case spell:      return CGI->spellh->spells[subtype]->descriptions[val];
	case morale:     return CGI->generaltexth->heroscrn[ 4 - (val>0) + (val<0)];
	case luck:       return CGI->generaltexth->heroscrn[ 7 - (val>0) + (val<0)];
	case building:   return CGI->townh->towns[subtype].buildings[BuildingID(val)]->Description();
	case hero:       return CGI->heroh->heroes[subtype]->name;
	case flag:       return "";
	}
	assert(0);
	return "";
}

std::string CComponent::getSubtitle()
{
	if (!perDay)
		return getSubtitleInternal();

	std::string ret = CGI->generaltexth->allTexts[3];
	boost::replace_first(ret, "%d", getSubtitleInternal());
	return ret;
}

std::string CComponent::getSubtitleInternal()
{
	//FIXME: some of these are horrible (e.g creature)
	switch(compType)
	{
	case primskill:  return boost::str(boost::format("%+d %s") % val % (subtype < 4 ? CGI->generaltexth->primarySkillNames[subtype] : CGI->generaltexth->allTexts[387]));
	case secskill:   return CGI->generaltexth->levels[val-1] + "\n" + CGI->generaltexth->skillName[subtype];
	case resource:   return boost::lexical_cast<std::string>(val);
	case creature:   return (val? boost::lexical_cast<std::string>(val) + " " : "") + CGI->creh->creatures[subtype]->*(val != 1 ? &CCreature::namePl : &CCreature::nameSing);
	case artifact:   return CGI->arth->artifacts[subtype]->Name();
	case experience: return (subtype && val==1) ? CGI->generaltexth->allTexts[442] : boost::lexical_cast<std::string>(val);
	case spell:      return CGI->spellh->spells[subtype]->name;
	case morale:     return "";
	case luck:       return "";
	case building:   return CGI->townh->towns[subtype].buildings[BuildingID(val)]->Name();
	case hero:       return CGI->heroh->heroes[subtype]->name;
	case flag:       return CGI->generaltexth->capColors[subtype];
	}
	assert(0);
	return "";
}

void CComponent::setSurface(std::string defName, int imgPos)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL;
	vstd::clear_pointer(image);
	image = new CAnimImage(defName, imgPos);
}

void CComponent::clickRight(tribool down, bool previousState)
{
	if(!getDescription().empty())
		adventureInt->handleRightClick(getDescription(), down);
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

CSelectableComponent::CSelectableComponent(const Component &c, boost::function<void()> OnSelect):
	CComponent(c),onSelect(OnSelect)
{
	type |= REDRAW_PARENT;
	addUsedEvents(LCLICK | KEYBOARD);
	init();
}

CSelectableComponent::CSelectableComponent(Etype Type, int Sub, int Val, ESize imageSize, boost::function<void()> OnSelect):
	CComponent(Type,Sub,Val, imageSize),onSelect(OnSelect)
{
	type |= REDRAW_PARENT;
	addUsedEvents(LCLICK | KEYBOARD);
	init();
}

void CSelectableComponent::select(bool on)
{
	if(on != selected)
	{
		selected = on;
		redraw();
	}
}

void CSelectableComponent::showAll(SDL_Surface * to)
{
	CComponent::showAll(to);
	if(selected)
	{
		CSDL_Ext::drawBorder(to, Rect::around(image->pos), int3(239,215,123));
	}
}

void CComponentBox::selectionChanged(CSelectableComponent * newSelection)
{
	if (newSelection == selected)
		return;

	if (selected)
		selected->select(false);

	selected = newSelection;
	if (onSelect)
		onSelect(selectedIndex());

	if (selected)
		selected->select(true);
}

int CComponentBox::selectedIndex()
{
	if (selected)
		return std::find(components.begin(), components.end(), selected) - components.begin();
	return -1;
}

Point CComponentBox::getOrTextPos(CComponent *left, CComponent *right)
{
	int leftSubtitle  = ( left->pos.w -  left->image->pos.w) / 2;
	int rightSubtitle = (right->pos.w - right->image->pos.w) / 2;
	int fullDistance = getDistance(left, right) + leftSubtitle + rightSubtitle;

	return Point(fullDistance/2 - leftSubtitle, (left->image->pos.h + right->image->pos.h) / 4);
}

int CComponentBox::getDistance(CComponent *left, CComponent *right)
{
	static const int betweenImagesMin = 20;
	static const int betweenSubtitlesMin = 10;

	int leftSubtitle  = ( left->pos.w -  left->image->pos.w) / 2;
	int rightSubtitle = (right->pos.w - right->image->pos.w) / 2;
	int subtitlesOffset = leftSubtitle + rightSubtitle;

	return std::max(betweenSubtitlesMin, betweenImagesMin - subtitlesOffset);
}

void CComponentBox::placeComponents(bool selectable)
{
	static const int betweenRows = 22;

	OBJ_CONSTRUCTION_CAPTURING_ALL;
	if (components.empty())
		return;

	//prepare components
	BOOST_FOREACH(auto & comp, components)
	{
		addChild(comp);
		comp->moveTo(Point(pos.x, pos.y));
	}

	struct RowData
	{
		size_t comps;
		int width;
		int height;
		RowData (size_t Comps, int Width, int Height):
		comps(Comps), width (Width), height (Height){};
	};
	std::vector<RowData> rows;
	rows.push_back (RowData (0,0,0));

	//split components in rows
	CComponent * prevComp = nullptr;

	BOOST_FOREACH(CComponent * comp, components)
	{
		//make sure that components are smaller than our width
		//assert(pos.w == 0 || pos.w < comp->pos.w);

		const int distance = prevComp ? getDistance(prevComp, comp) : 0;

		//start next row
		if ((pos.w != 0 && rows.back().width + comp->pos.w + distance > pos.w) // row is full
			|| rows.back().comps >= 4) // no more than 4 comps per row
		{
			prevComp = nullptr;
			rows.push_back (RowData (0,0,0));
		}

		if (prevComp)
			rows.back().width += distance;

		rows.back().comps++;
		rows.back().width += comp->pos.w;

		vstd::amax(rows.back().height, comp->pos.h);
		prevComp = comp;
	}

	if (pos.w == 0)
	{
		BOOST_FOREACH(auto & row, rows)
			vstd::amax(pos.w, row.width);
	}

	int height = (rows.size() - 1) * betweenRows;
	BOOST_FOREACH(auto & row, rows)
		height += row.height;

	//assert(pos.h == 0 || pos.h < height);
	if (pos.h == 0)
		pos.h = height;

	auto iter = components.begin();
	int currentY = (pos.h - height) / 2;

	//move components to their positions
	for (size_t row = 0; row < rows.size(); row++)
	{
		// amount of free space we may add on each side of every component
		int freeSpace = (pos.w - rows[row].width) / (rows[row].comps * 2);
		prevComp = nullptr;

		int currentX = 0;
		for (size_t col = 0; col < rows[row].comps; col++)
		{
			currentX += freeSpace;
			if (prevComp)
			{
				if (selectable)
				{
					Point orPos = Point(currentX - freeSpace, currentY) + getOrTextPos(prevComp, *iter);

					new CLabel(orPos.x, orPos.y, FONT_MEDIUM, CENTER, Colors::WHITE, CGI->generaltexth->allTexts[4]);
				}
				currentX += getDistance(prevComp, *iter);
			}

			(*iter)->moveBy(Point(currentX, currentY));
			currentX += (*iter)->pos.w;
			currentX += freeSpace;

			prevComp = *(iter++);
		}
		currentY += rows[row].height + betweenRows;
	}
}

CComponentBox::CComponentBox(CComponent * _components, Rect position):
    components(1, _components),
    selected(nullptr)
{
	type |= REDRAW_PARENT;
	pos = position + pos;
	placeComponents(false);
}

CComponentBox::CComponentBox(std::vector<CComponent *> _components, Rect position):
    components(_components),
    selected(nullptr)
{
	type |= REDRAW_PARENT;
	pos = position + pos;
	placeComponents(false);
}

CComponentBox::CComponentBox(std::vector<CSelectableComponent *> _components, Rect position, boost::function<void(int newID)> _onSelect):
    components(_components.begin(), _components.end()),
    selected(nullptr),
    onSelect(_onSelect)
{
	type |= REDRAW_PARENT;
	pos = position + pos;
	placeComponents(true);

	assert(!components.empty());

	int key = SDLK_1;
	BOOST_FOREACH(auto & comp, _components)
	{
		comp->onSelect = boost::bind(&CComponentBox::selectionChanged, this, comp);
		comp->assignedKeys.insert(key++);
	}
	selectionChanged(_components.front());
}

void CSelWindow::selectionChange(unsigned to)
{
	for (unsigned i=0;i<components.size();i++)
	{
		CSelectableComponent * pom = dynamic_cast<CSelectableComponent*>(components[i]);
		if (!pom)
			continue;
		pom->select(i==to);
	}
	redraw();
}

CSelWindow::CSelWindow(const std::string &Text, PlayerColor player, int charperline, const std::vector<CSelectableComponent*> &comps, const std::vector<std::pair<std::string,CFunctionList<void()> > > &Buttons, int askID)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL;
	ID = askID;
	for(int i=0;i<Buttons.size();i++)
	{
		buttons.push_back(new CAdventureMapButton("","",Buttons[i].second,0,0,Buttons[i].first));
		if(!i  &&  askID >= 0)
			buttons.back()->callback += boost::bind(&CSelWindow::madeChoice,this);
		buttons[i]->callback += boost::bind(&CInfoWindow::close,this); //each button will close the window apart from call-defined actions
	}

	text = new CTextBox(Text, Rect(0, 0, 250, 100), 0, FONT_MEDIUM, CENTER, Colors::WHITE);

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

CCreaturePic::CCreaturePic(int x, int y, const CCreature *cre, bool Big, bool Animated)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL;
	pos.x+=x;
	pos.y+=y;

	TFaction faction = cre->faction;

	assert(vstd::contains(CGI->townh->factions, cre->faction));

	if(Big)
		bg = new CPicture(CGI->townh->factions[faction].creatureBg130);
	else
		bg = new CPicture(CGI->townh->factions[faction].creatureBg120);
	bg->needRefresh = true;
	anim = new CCreatureAnim(0, 0, cre->animDefName, Rect());
	anim->clipRect(cre->isDoubleWide()?170:150, 155, bg->pos.w, bg->pos.h);
	anim->startPreview(cre->hasBonusOfType(Bonus::SIEGE_WEAPON));

	pos.w = bg->pos.w;
	pos.h = bg->pos.h;
}

CRecruitmentWindow::CCreatureCard::CCreatureCard(CRecruitmentWindow *window, const CCreature *crea, int totalAmount):
    CIntObject(LCLICK | RCLICK),
    parent(window),
    selected(false),
    creature(crea),
    amount(totalAmount)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL;
	pic = new CCreaturePic(1,1, creature, true, true);
	// 1 + 1 px for borders
	pos.w = pic->pos.w + 2;
	pos.h = pic->pos.h + 2;
}

void CRecruitmentWindow::CCreatureCard::select(bool on)
{
	selected = on;
	redraw();
}

void CRecruitmentWindow::CCreatureCard::clickLeft(tribool down, bool previousState)
{
	if (down)
		parent->select(this);
}

void CRecruitmentWindow::CCreatureCard::clickRight(tribool down, bool previousState)
{
	if (down)
		GH.pushInt(createCreWindow(creature->idNumber, CCreatureWindow::OTHER, 0));
}

void CRecruitmentWindow::CCreatureCard::showAll(SDL_Surface * to)
{
	CIntObject::showAll(to);
	if (selected)
		drawBorder(to, pos, int3(248, 0, 0));
	else
		drawBorder(to, pos, int3(232, 212, 120));
}

CRecruitmentWindow::CCostBox::CCostBox(Rect position, std::string title)
{
	type |= REDRAW_PARENT;
	pos = position + pos;
	OBJ_CONSTRUCTION_CAPTURING_ALL;
	new CLabel(pos.w/2, 10, FONT_SMALL, CENTER, Colors::WHITE, title);
}

void CRecruitmentWindow::CCostBox::set(TResources res)
{
	//just update values
	BOOST_FOREACH(auto & item, resources)
	{
		item.second.first->setTxt(boost::lexical_cast<std::string>(res[item.first]));
	}
}

void CRecruitmentWindow::CCostBox::createItems(TResources res)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL;

	BOOST_FOREACH(auto & curr, resources)
	{
		delete curr.second.first;
		delete curr.second.second;
	}
	resources.clear();

	TResources::nziterator iter(res);
	while (iter.valid())
	{
		CAnimImage * image = new CAnimImage("RESOURCE", iter->resType);
		CLabel * text = new CLabel(15, 43, FONT_SMALL, CENTER, Colors::WHITE, "0");

		resources.insert(std::make_pair(iter->resType, std::make_pair(text, image)));
		iter++;
	}

	if (!resources.empty())
	{
		int curx = pos.w / 2 - (16 * resources.size()) - (8 * (resources.size() - 1));
		//reverse to display gold as first resource
		BOOST_REVERSE_FOREACH(auto & res, resources)
		{
			res.second.first->moveBy(Point(curx, 22));
			res.second.second->moveBy(Point(curx, 22));
			curx += 48;
		}
	}
	redraw();
}

void CRecruitmentWindow::select(CCreatureCard *card)
{
	if (card == selected)
		return;

	if (selected)
		selected->select(false);

	selected = card;

	if (selected)
		selected->select(true);

	if (card)
	{
		si32 maxAmount = card->creature->maxAmount(LOCPLINT->cb->getResourceAmount());

		vstd::amin(maxAmount, card->amount);

		slider->setAmount(maxAmount);

		if(slider->value != maxAmount)
			slider->moveTo(maxAmount);
		else // if slider already at 0 - emulate call to sliderMoved()
			sliderMoved(maxAmount);

		costPerTroopValue->createItems(card->creature->cost);
		totalCostValue->createItems(card->creature->cost);

		costPerTroopValue->set(card->creature->cost);
		totalCostValue->set(card->creature->cost * maxAmount);

		//Recruit %s
		title->setTxt(boost::str(boost::format(CGI->generaltexth->tcommands[21]) % card->creature->namePl));

		maxButton->block(maxAmount == 0);
		slider->block(maxAmount == 0);
	}
}

void CRecruitmentWindow::buy()
{
	CreatureID crid =  selected->creature->idNumber;
	SlotID dstslot = dst-> getSlotFor(crid);

	if(!dstslot.validSlot() && !vstd::contains(CGI->arth->bigArtifacts,CGI->arth->creatureToMachineID(crid))) //no available slot
	{
		std::string txt;
		if(dst->ID == Obj::HERO)
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

	onRecruit(crid, slider->value);
	if(level >= 0)
		close();
}

void CRecruitmentWindow::showAll(SDL_Surface * to)
{
	CWindowObject::showAll(to);

	// recruit\total values
	drawBorder(to, pos.x + 172, pos.y + 222, 67, 42, int3(239,215,123));
	drawBorder(to, pos.x + 246, pos.y + 222, 67, 42, int3(239,215,123));

	//cost boxes
	drawBorder(to, pos.x + 64,  pos.y + 222, 99, 76, int3(239,215,123));
	drawBorder(to, pos.x + 322, pos.y + 222, 99, 76, int3(239,215,123));

	//buttons borders
	drawBorder(to, pos.x + 133, pos.y + 312, 66, 34, int3(173,142,66));
	drawBorder(to, pos.x + 211, pos.y + 312, 66, 34, int3(173,142,66));
	drawBorder(to, pos.x + 289, pos.y + 312, 66, 34, int3(173,142,66));
}

CRecruitmentWindow::CRecruitmentWindow(const CGDwelling *Dwelling, int Level, const CArmedInstance *Dst, const boost::function<void(CreatureID,int)> &Recruit, int y_offset):
    CWindowObject(PLAYER_COLORED, "TPRCRT"),
	onRecruit(Recruit),
    level(Level),
    dst(Dst),
    selected(nullptr),
    dwelling(Dwelling)
{
	moveBy(Point(0, y_offset));

	OBJ_CONSTRUCTION_CAPTURING_ALL;
	new CGStatusBar(new CPicture(*background, Rect(8, pos.h - 26, pos.w - 16, 19), 8, pos.h - 26));

	slider = new CSlider(176,279,135,0,0,0,0,true);
	slider->moved = boost::bind(&CRecruitmentWindow::sliderMoved,this, _1);

	maxButton = new CAdventureMapButton(CGI->generaltexth->zelp[553],boost::bind(&CSlider::moveToMax,slider),134,313,"IRCBTNS.DEF",SDLK_m);
	buyButton = new CAdventureMapButton(CGI->generaltexth->zelp[554],boost::bind(&CRecruitmentWindow::buy,this),212,313,"IBY6432.DEF",SDLK_RETURN);
	cancelButton = new CAdventureMapButton(CGI->generaltexth->zelp[555],boost::bind(&CRecruitmentWindow::close,this),290,313,"ICN6432.DEF",SDLK_ESCAPE);

	title = new CLabel(243, 32, FONT_BIG, CENTER, Colors::YELLOW);
	availableValue = new CLabel(205, 253, FONT_SMALL, CENTER, Colors::WHITE);
	toRecruitValue = new CLabel(279, 253, FONT_SMALL, CENTER, Colors::WHITE);

	costPerTroopValue =  new CCostBox(Rect(65, 222, 97, 74), CGI->generaltexth->allTexts[346]);
	totalCostValue = new CCostBox(Rect(323, 222, 97, 74), CGI->generaltexth->allTexts[466]);

	new CLabel(205, 233, FONT_SMALL, CENTER, Colors::WHITE, CGI->generaltexth->allTexts[465]); //available t
	new CLabel(279, 233, FONT_SMALL, CENTER, Colors::WHITE, CGI->generaltexth->allTexts[16]); //recruit t

	availableCreaturesChanged();
}

void CRecruitmentWindow::availableCreaturesChanged()
{
	OBJ_CONSTRUCTION_CAPTURING_ALL;

	size_t selectedIndex = 0;

	if (!cards.empty() && selected) // find position of selected item
		selectedIndex = std::find(cards.begin(), cards.end(), selected) - cards.begin();

	//deselect card
	select(nullptr);

	//delete old cards
	BOOST_FOREACH(auto & card, cards)
		delete card;
	cards.clear();

	for(int i=0; i<dwelling->creatures.size(); i++)
	{
		//find appropriate level
		if(level >= 0 && i != level)
			continue;

		int amount = dwelling->creatures[i].first;

		//create new cards
		BOOST_REVERSE_FOREACH(auto & creature, dwelling->creatures[i].second)
			cards.push_back(new CCreatureCard(this, CGI->creh->creatures[creature], amount));
	}

	assert(!cards.empty());

	const int creatureWidth = 102;

	//normal distance between cards - 18px
	int requiredSpace = 18;
	//maximum distance we can use without reaching window borders
	int availableSpace = pos.w - 50 - creatureWidth * cards.size();

	if (cards.size() > 1) // avoid division by zero
		availableSpace /= cards.size() - 1;
	else
		availableSpace = 0;

	assert(availableSpace >= 0);

	const int spaceBetween = std::min(requiredSpace, availableSpace);
	const int totalCreatureWidth = spaceBetween + creatureWidth;

	//now we know total amount of cards and can move them to correct position
	int curx = pos.w / 2 - (creatureWidth*cards.size()/2) - (spaceBetween*(cards.size()-1)/2);
	BOOST_FOREACH(auto & card, cards)
	{
		card->moveBy(Point(curx, 64));
		curx += totalCreatureWidth;
	}

	//restore selection
	select(cards[selectedIndex]);

	if(slider->value == slider->amount)
		slider->moveTo(slider->amount);
	else // if slider already at 0 - emulate call to sliderMoved()
		sliderMoved(slider->amount);
}

void CRecruitmentWindow::sliderMoved(int to)
{
	if (!selected)
		return;

	buyButton->block(!to);
	availableValue->setTxt(boost::lexical_cast<std::string>(selected->amount - to));
	toRecruitValue->setTxt(boost::lexical_cast<std::string>(to));

	totalCostValue->set(selected->creature->cost * to);
}

CSplitWindow::CSplitWindow(const CCreature * creature, boost::function<void(int, int)> callback_,
                           int leftMin_, int rightMin_, int leftAmount_, int rightAmount_):
    CWindowObject(PLAYER_COLORED, "GPUCRDIV"),
    callback(callback_),
    leftAmount(leftAmount_),
    rightAmount(rightAmount_),
    leftMin(leftMin_),
    rightMin(rightMin_),
    slider(nullptr)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL;

	int total = leftAmount + rightAmount;
	int leftMax = total - rightMin;
	int rightMax = total - leftMin;

	ok = new CAdventureMapButton("", "", boost::bind(&CSplitWindow::apply, this), 20, 263, "IOK6432", SDLK_RETURN);
	cancel = new CAdventureMapButton("", "", boost::bind(&CSplitWindow::close, this), 214, 263, "ICN6432", SDLK_ESCAPE);

	int sliderPositions = total - leftMin - rightMin;

	leftInput = new CTextInput(Rect(20, 218, 100, 36), FONT_BIG, boost::bind(&CSplitWindow::setAmountText, this, _1, true));
	rightInput = new CTextInput(Rect(176, 218, 100, 36), FONT_BIG, boost::bind(&CSplitWindow::setAmountText, this, _1, false));

	//add filters to allow only number input
	leftInput->filters.add(boost::bind(&CTextInput::numberFilter, _1, _2, leftMin, leftMax));
	rightInput->filters.add(boost::bind(&CTextInput::numberFilter, _1, _2, rightMin, rightMax));

	leftInput->setTxt(boost::lexical_cast<std::string>(leftAmount), false);
	rightInput->setTxt(boost::lexical_cast<std::string>(rightAmount), false);

	animLeft = new CCreaturePic(20, 54, creature, true, false);
	animRight = new CCreaturePic(177, 54,creature, true, false);

	slider = new CSlider(21, 194, 257, boost::bind(&CSplitWindow::sliderMoved, this, _1), 0, sliderPositions, rightAmount - rightMin, true);

	std::string title = CGI->generaltexth->allTexts[256];
	boost::algorithm::replace_first(title,"%s", creature->namePl);
	new CLabel(150, 34, FONT_BIG, CENTER, Colors::YELLOW, title);
}

void CSplitWindow::setAmountText(std::string text, bool left)
{
	try
	{
		setAmount(boost::lexical_cast<int>(text), left);
		slider->moveTo(rightAmount - rightMin);
	}
	catch(boost::bad_lexical_cast &)
	{
	}
}

void CSplitWindow::setAmount(int value, bool left)
{
	int total = leftAmount + rightAmount;
	leftAmount  = left ? value : total - value;
	rightAmount = left ? total - value : value;

	leftInput->setTxt(boost::lexical_cast<std::string>(leftAmount));
	rightInput->setTxt(boost::lexical_cast<std::string>(rightAmount));
}

void CSplitWindow::apply()
{
	callback(leftAmount, rightAmount);
	close();
}

void CSplitWindow::sliderMoved(int to)
{
	setAmount(rightMin + to, false);
}

CLevelWindow::CLevelWindow(const CGHeroInstance *hero, PrimarySkill::PrimarySkill pskill, std::vector<SecondarySkill> &skills, boost::function<void(ui32)> callback):
    CWindowObject(PLAYER_COLORED, "LVLUPBKG"),
    cb(callback)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL;

	LOCPLINT->showingDialog->setn(true);

	new CAnimImage("PortraitsLarge", hero->portrait, 0, 170, 66);
	new CAdventureMapButton("", "", boost::bind(&CLevelWindow::close, this), 297, 413, "IOKAY", SDLK_RETURN);

	//%s has gained a level.
	new CLabel(192, 33, FONT_MEDIUM, CENTER, Colors::WHITE,
	           boost::str(boost::format(CGI->generaltexth->allTexts[444]) % hero->name));

	//%s is now a level %d %s.
	new CLabel(192, 162, FONT_MEDIUM, CENTER, Colors::WHITE,
	           boost::str(boost::format(CGI->generaltexth->allTexts[445]) % hero->name % hero->level % hero->type->heroClass->name));

	new CAnimImage("PSKIL42", pskill, 0, 174, 190);

	new CLabel(192, 253, FONT_MEDIUM, CENTER, Colors::WHITE,
	           CGI->generaltexth->primarySkillNames[pskill] + " +1");

	if (!skills.empty())
	{
		std::vector<CSelectableComponent *> comps;

		for(size_t i=0;i<skills.size();i++)
		{
			comps.push_back(new CSelectableComponent(
								CComponent::secskill,
								skills[i],
								hero->getSecSkillLevel( SecondarySkill(skills[i]) )+1,
								CComponent::medium));
		}
		box = new CComponentBox(comps, Rect(75, 300, pos.w - 150, 100));
	}
	else
		box = nullptr;
}

CLevelWindow::~CLevelWindow()
{
	//FIXME: call callback if there was nothing to select?
	if (box && box->selectedIndex() != -1)
		cb(box->selectedIndex());

	LOCPLINT->showingDialog->setn(false);
}

void CMinorResDataBar::show(SDL_Surface * to)
{
}

void CMinorResDataBar::showAll(SDL_Surface * to)
{
	blitAt(bg,pos.x,pos.y,to);
	for (Res::ERes i=Res::WOOD; i<=Res::GOLD; vstd::advance(i, 1))
	{
		std::string text = boost::lexical_cast<std::string>(LOCPLINT->cb->getResourceAmount(i));

		graphics->fonts[FONT_SMALL]->renderTextCenter(to, text, Colors::WHITE, Point(pos.x + 50 + 76 * i, pos.y + pos.h/2));
	}
	std::vector<std::string> temp;

	temp.push_back(boost::lexical_cast<std::string>(LOCPLINT->cb->getDate(Date::MONTH)));
	temp.push_back(boost::lexical_cast<std::string>(LOCPLINT->cb->getDate(Date::WEEK)));
	temp.push_back(boost::lexical_cast<std::string>(LOCPLINT->cb->getDate(Date::DAY_OF_WEEK)));

	std::string datetext =  CGI->generaltexth->allTexts[62]+": %s, " + CGI->generaltexth->allTexts[63]
	                        + ": %s, " + CGI->generaltexth->allTexts[64] + ": %s";

	graphics->fonts[FONT_SMALL]->renderTextCenter(to, processStr(datetext,temp), Colors::WHITE, Point(pos.x+545+(pos.w-545)/2,pos.y+pos.h/2));
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

CObjectListWindow::CItem::CItem(CObjectListWindow *_parent, size_t _id, std::string _text):
	parent(_parent),
	index(_id)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL;
	border = new CPicture("TPGATES");
	pos = border->pos;
	addUsedEvents(LCLICK);
	type |= REDRAW_PARENT;

	text = new CLabel(pos.w/2, pos.h/2, FONT_SMALL, CENTER, Colors::WHITE, _text);
	select(index == parent->selected);
}

void CObjectListWindow::CItem::select(bool on)
{
	if (on)
		border->recActions = 255;
	else
		border->recActions = ~(UPDATE | SHOWALL);
	redraw();
}

void CObjectListWindow::CItem::clickLeft(tribool down, bool previousState)
{
	if( previousState && !down)
		parent->changeSelection(index);
}

CObjectListWindow::CObjectListWindow(const std::vector<int> &_items, CPicture * titlePic, std::string _title, std::string _descr,
				boost::function<void(int)> Callback):
    CWindowObject(PLAYER_COLORED, "TPGATE"),
	onSelect(Callback)
{
	items.reserve(_items.size());
	BOOST_FOREACH(int id, _items)
	{
		items.push_back(std::make_pair(id, CGI->mh->map->objects[id]->hoverName));
	}

	init(titlePic, _title, _descr);
}

CObjectListWindow::CObjectListWindow(const std::vector<std::string> &_items, CPicture * titlePic, std::string _title, std::string _descr,
				boost::function<void(int)> Callback):
    CWindowObject(PLAYER_COLORED, "TPGATE"),
	onSelect(Callback)
{
	items.reserve(_items.size());

	for (size_t i=0; i<_items.size(); i++)
		items.push_back(std::make_pair(int(i), _items[i]));

	init(titlePic, _title, _descr);
}

void CObjectListWindow::init(CPicture * titlePic, std::string _title, std::string _descr)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL;

	title = new CLabel(152, 27, FONT_BIG, CENTER, Colors::YELLOW, _title);
	descr = new CLabel(145, 133, FONT_SMALL, CENTER, Colors::WHITE, _descr);

	ok = new CAdventureMapButton("","",boost::bind(&CObjectListWindow::elementSelected, this),15,402,"IOKAY.DEF", SDLK_RETURN);
	ok->block(true);
	exit = new CAdventureMapButton("","",boost::bind(&CGuiHandler::popIntTotally,&GH, this),228,402,"ICANCEL.DEF",SDLK_ESCAPE);

	if (titlePic)
	{
		titleImage = titlePic;
		addChild(titleImage);
		titleImage->recActions = defActions;
		titleImage->pos.x = pos.w/2 + pos.x - titleImage->pos.w/2;
		titleImage->pos.y =75 + pos.y - titleImage->pos.h/2;
	}
	list = new CListBox(boost::bind(&CObjectListWindow::genItem, this, _1), CListBox::DestroyFunc(),
		Point(14, 151), Point(0, 25), 9, items.size(), 0, 1, Rect(262, -32, 256, 256) );
	list->type |= REDRAW_PARENT;
}

CIntObject * CObjectListWindow::genItem(size_t index)
{
	if (index < items.size())
		return new CItem(this, index, items[index].second);
	return NULL;
}

void CObjectListWindow::elementSelected()
{
	boost::function<void(int)> toCall = onSelect;//save
	int where = items[selected].first;      //required variables
	GH.popIntTotally(this);//then destroy window
	toCall(where);//and send selected object
}

void CObjectListWindow::changeSelection(size_t which)
{
	ok->block(false);
	if (selected == which)
		return;

	std::list< CIntObject * > elements = list->getItems();
	BOOST_FOREACH(CIntObject * element, elements)
	{
		CItem *item;
		if ( (item = dynamic_cast<CItem*>(element)) )
		{
			if (item->index == selected)
				item->select(false);
			if (item->index == which)
				item->select(true);
		}
	}
	selected = which;
}

void CObjectListWindow::keyPressed (const SDL_KeyboardEvent & key)
{
	if(key.state != SDL_PRESSED)
		return;

	int sel = selected;

	switch(key.keysym.sym)
	{
	break; case SDLK_UP:
		sel -=1;

	break; case SDLK_DOWN:
		sel +=1;

	break; case SDLK_PAGEUP:
		sel -=9;

	break; case SDLK_PAGEDOWN:
		sel +=9;

	break; case SDLK_HOME:
		sel = 0;

	break; case SDLK_END:
		sel = items.size();

	break; default:
		return;
	}

	vstd::abetween(sel, 0, items.size()-1);
	list->scrollTo(sel);
	changeSelection(sel);
}

CTradeWindow::CTradeableItem::CTradeableItem(Point pos, EType Type, int ID, bool Left, int Serial):
    CIntObject(LCLICK | HOVER | RCLICK, pos),
    type(EType(-1)),// set to invalid, will be corrected in setType
    id(ID),
    serial(Serial),
    left(Left)
{
	downSelection = false;
	hlp = NULL;
	image = nullptr;
	setType(Type);
}

void CTradeWindow::CTradeableItem::setType(EType newType)
{
	if (type != newType)
	{
		OBJ_CONSTRUCTION_CAPTURING_ALL;
		type = newType;
		delete image;

		if (getIndex() < 0)
		{
			image = new CAnimImage(getFilename(), 0);
			image->disable();
		}
		else
			image = new CAnimImage(getFilename(), getIndex());
	}
}

void CTradeWindow::CTradeableItem::setID(int newID)
{
	if (id != newID)
	{
		id = newID;
		if (image)
		{
			int index = getIndex();
			if (index < 0)
				image->disable();
			else
			{
				image->enable();
				image->setFrame(index);
			}
		}
	}
}

std::string CTradeWindow::CTradeableItem::getFilename()
{
	switch(type)
	{
	case RESOURCE:
		return "RESOURCE";
	case PLAYER:
		return "CREST58";
	case ARTIFACT_TYPE:
	case ARTIFACT_PLACEHOLDER:
	case ARTIFACT_INSTANCE:
		return "artifact";
	case CREATURE:
		return "TWCRPORT";
	default:
		return "";
	}
}

int CTradeWindow::CTradeableItem::getIndex()
{
	if (id < 0)
		return -1;

	switch(type)
	{
	case RESOURCE:
	case PLAYER:
		return id;
	case ARTIFACT_TYPE:
	case ARTIFACT_INSTANCE:
	case ARTIFACT_PLACEHOLDER:
		return VLC->arth->artifacts[id]->iconIndex;
	case CREATURE:
		return VLC->creh->creatures[id]->iconIndex;
	default:
		return -1;
	}
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

	if (image)
	{
		image->moveTo(pos.topLeft() + posToBitmap);
		CIntObject::showAll(to);
	}

	printAtMiddleLoc(subtitle, posToSubCenter, FONT_SMALL, Colors::WHITE, to);
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
				CCS->curh->dragAndDropCursor(new CAnimImage("artifact", art->artType->iconIndex));

				aw->arts->artifactsOnAltar.erase(art);
				setID(-1);
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

void CTradeWindow::CTradeableItem::showAllAt(const Point &dstPos, const std::string &customSub, SDL_Surface * to)
{
	Rect oldPos = pos;
	std::string oldSub = subtitle;
	downSelection = true;

	moveTo(dstPos);
	subtitle = customSub;
	showAll(to);

	downSelection = false;
	moveTo(oldPos.topLeft());
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
		setID(art->artType->id);
	else
		setID(-1);
}

CTradeWindow::CTradeWindow(std::string bgName, const IMarket *Market, const CGHeroInstance *Hero, EMarketMode::EMarketMode Mode):
    CWindowObject(PLAYER_COLORED, bgName),
	market(Market),
    hero(Hero),
    arts(NULL),
    hLeft(NULL),
    hRight(NULL),
    readyToTrade(false)
{
	type |= BLOCK_ADV_HOTKEYS;
	mode = Mode;
	initTypes();
}

void CTradeWindow::initTypes()
{
	switch(mode)
	{
	case EMarketMode::RESOURCE_RESOURCE:
		itemsType[1] = RESOURCE;
		itemsType[0] = RESOURCE;
		break;
	case EMarketMode::RESOURCE_PLAYER:
		itemsType[1] = RESOURCE;
		itemsType[0] = PLAYER;
		break;
	case EMarketMode::CREATURE_RESOURCE:
		itemsType[1] = CREATURE;
		itemsType[0] = RESOURCE;
		break;
	case EMarketMode::RESOURCE_ARTIFACT:
		itemsType[1] = RESOURCE;
		itemsType[0] = ARTIFACT_TYPE;
		break;
	case EMarketMode::ARTIFACT_RESOURCE:
		itemsType[1] = ARTIFACT_INSTANCE;
		itemsType[0] = RESOURCE;
		break;
	case EMarketMode::CREATURE_EXP:
		itemsType[1] = CREATURE;
		itemsType[0] = CREATURE_PLACEHOLDER;
		break;
	case EMarketMode::ARTIFACT_EXP:
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
		if(mode == EMarketMode::ARTIFACT_RESOURCE)
		{
			xOffset = -361;
			yOffset = +46;

			CTradeableItem *hlp = new CTradeableItem(Point(137, 469), itemsType[Left], -1, 1, 0);
			hlp->recActions &= ~(UPDATE | SHOWALL);
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

		if(mode == EMarketMode::ARTIFACT_RESOURCE)
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
		vstd::amin(amount, ids->size());

	for(int j=0; j<amount; j++)
	{
		int id = (ids && ids->size()>j) ? (*ids)[j] : j;
		if(id < 0 && mode != EMarketMode::ARTIFACT_EXP)  //when sacrificing artifacts we need to prepare empty slots
			continue;

		CTradeableItem *hlp = new CTradeableItem(pos[j].topLeft(), itemsType[Left], id, Left, j);
		hlp->pos = pos[j] + this->pos.topLeft();
		items[Left].push_back(hlp);
	}

	initSubs(Left);
}

std::vector<int> *CTradeWindow::getItemsIds(bool Left)
{
	std::vector<int> *ids = NULL;

	if(mode == EMarketMode::ARTIFACT_EXP)
		return new std::vector<int>(22, -1);

	if(Left)
	{
		switch(itemsType[1])
		{
		case CREATURE:
			ids = new std::vector<int>;
			for(int i = 0; i < 7; i++)
			{
				if(const CCreature *c = hero->getCreature(SlotID(i)))
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
			for(int i = 0; i < PlayerColor::PLAYER_LIMIT_I; i++)
				if(PlayerColor(i) != LOCPLINT->playerID && LOCPLINT->cb->getPlayerStatus(PlayerColor(i)) == EPlayerStatus::INGAME)
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
	if(mode == EMarketMode::ARTIFACT_EXP && !Left)
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
				t->subtitle = boost::lexical_cast<std::string>(hero->getStackCount(SlotID(t->serial)));
				break;
			case RESOURCE:
				t->subtitle = boost::lexical_cast<std::string>(LOCPLINT->cb->getResourceAmount(static_cast<Res::ERes>(t->serial)));
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
				if(t->id != hLeft->id || mode != EMarketMode::RESOURCE_RESOURCE) //don't allow exchanging same resources
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
	CWindowObject::showAll(to);

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
	items[t->left] -= t;
	delete t;

	if(hRight == t)
	{
		hRight = NULL;
		selectionChanged(false);
	}
}

void CTradeWindow::getEmptySlots(std::set<CTradeableItem *> &toRemove)
{
	BOOST_FOREACH(CTradeableItem *t, items[1])
		if(!hero->getStackCount(SlotID(t->serial)))
			toRemove.insert(t);
}

void CTradeWindow::setMode(EMarketMode::EMarketMode Mode)
{
	const IMarket *m = market;
	const CGHeroInstance *h = hero;
	CTradeWindow *nwindow = NULL;

	GH.popIntTotally(this);

	switch(Mode)
	{
	case EMarketMode::CREATURE_EXP:
	case EMarketMode::ARTIFACT_EXP:
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
	assert(mode == EMarketMode::ARTIFACT_RESOURCE);
	items[1][0]->setArtInstance(slot->ourArt);
	if(slot->ourArt)
		hLeft = items[1][0];
	else
		hLeft = NULL;

	selectionChanged(true);
}

std::string CMarketplaceWindow::getBackgroundForMode(EMarketMode::EMarketMode mode)
{
	switch(mode)
	{
	case EMarketMode::RESOURCE_RESOURCE:
		return "TPMRKRES.bmp";
	case EMarketMode::RESOURCE_PLAYER:
		return "TPMRKPTS.bmp";
	case EMarketMode::CREATURE_RESOURCE:
		return "TPMRKCRS.bmp";
	case EMarketMode::RESOURCE_ARTIFACT:
		return "TPMRKABS.bmp";
	case EMarketMode::ARTIFACT_RESOURCE:
		return "TPMRKASS.bmp";
	}
	assert(0);
	return "";
}

CMarketplaceWindow::CMarketplaceWindow(const IMarket *Market, const CGHeroInstance *Hero, EMarketMode::EMarketMode Mode)
    : CTradeWindow(getBackgroundForMode(Mode), Market, Hero, Mode)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL;

	madeTransaction = false;
	bool sliderNeeded = true;

	new CGStatusBar(new CPicture(*background, Rect(8, pos.h - 26, pos.w - 16, 19), 8, pos.h - 26));

	std::string title;

	if (market->o->ID == Obj::TOWN)
	{
		switch (mode)
		{
		break; case EMarketMode::CREATURE_RESOURCE:
			title = CGI->townh->towns[ETownType::STRONGHOLD].buildings[BuildingID::FREELANCERS_GUILD]->Name();

		break; case EMarketMode::RESOURCE_ARTIFACT:
			title = CGI->townh->towns[market->o->subID].buildings[BuildingID::ARTIFACT_MERCHANT]->Name();
			sliderNeeded = false;

		break; case EMarketMode::ARTIFACT_RESOURCE:
			title = CGI->townh->towns[market->o->subID].buildings[BuildingID::ARTIFACT_MERCHANT]->Name();
			sliderNeeded = false;

		break; default:
			title = CGI->generaltexth->allTexts[158];
		}
	}
	else
	{
		switch (market->o->ID)
		{
		break; case Obj::BLACK_MARKET:   title = CGI->generaltexth->allTexts[349];
		break; case Obj::TRADING_POST:  title = CGI->generaltexth->allTexts[159];
		break; case Obj::TRADING_POST_SNOW: title = CGI->generaltexth->allTexts[159];
		break; default:  title = market->o->getHoverText();
		}
	}

	new CLabel(300, 27, FONT_BIG, CENTER, Colors::YELLOW, title);

	initItems(false);
	initItems(true);

	ok = new CAdventureMapButton(CGI->generaltexth->zelp[600],boost::bind(&CGuiHandler::popIntTotally,&GH,this),516,520,"IOK6432.DEF",SDLK_RETURN);
	ok->assignedKeys.insert(SDLK_ESCAPE);
	deal = new CAdventureMapButton(CGI->generaltexth->zelp[595],boost::bind(&CMarketplaceWindow::makeDeal,this),307,520,"TPMRKB.DEF");
	deal->block(true);

	if(sliderNeeded)
	{
		slider = new CSlider(231,490,137,0,0,0);
		slider->moved = boost::bind(&CMarketplaceWindow::sliderMoved,this,_1);
		max = new CAdventureMapButton(CGI->generaltexth->zelp[596],boost::bind(&CMarketplaceWindow::setMax,this),229,520,"IRCBTNS.DEF");
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
	case EMarketMode::RESOURCE_RESOURCE:
	case EMarketMode::RESOURCE_PLAYER:
	case EMarketMode::RESOURCE_ARTIFACT:
		new CLabel(154, 148, FONT_SMALL, CENTER, Colors::WHITE, CGI->generaltexth->allTexts[270]);
		break;

	case EMarketMode::CREATURE_RESOURCE:
		//%s's Creatures
		new CLabel(152, 102, FONT_SMALL, CENTER, Colors::WHITE,
		           boost::str(boost::format(CGI->generaltexth->allTexts[272]) % hero->name));
		break;
	case EMarketMode::ARTIFACT_RESOURCE:
		//%s's Artifacts
		new CLabel(152, 102, FONT_SMALL, CENTER, Colors::WHITE,
		           boost::str(boost::format(CGI->generaltexth->allTexts[272]) % hero->name));
		break;
	}

	//right side
	switch(Mode)
	{
	case EMarketMode::RESOURCE_RESOURCE:
	case EMarketMode::CREATURE_RESOURCE:
	case EMarketMode::RESOURCE_ARTIFACT:
	case EMarketMode::ARTIFACT_RESOURCE:
		new CLabel(445, 148, FONT_SMALL, CENTER, Colors::WHITE, CGI->generaltexth->allTexts[168]);
		traderTextRect = Rect(316, 48, 260, 75);
		break;
	case EMarketMode::RESOURCE_PLAYER:
		new CLabel(445, 55, FONT_SMALL, CENTER, Colors::WHITE, CGI->generaltexth->allTexts[169]);
		traderTextRect = Rect(28, 48, 260, 75);
		break;
	}

	traderText = new CTextBox("", traderTextRect, 0, FONT_SMALL, CENTER);
	int specialOffset = mode == EMarketMode::ARTIFACT_RESOURCE ? 35 : 0; //in selling artifacts mode we need to move res-res and art-res buttons down

	if(printButtonFor(EMarketMode::RESOURCE_PLAYER))
		new CAdventureMapButton(CGI->generaltexth->zelp[612],boost::bind(&CMarketplaceWindow::setMode,this, EMarketMode::RESOURCE_PLAYER), 18, 520,"TPMRKBU1.DEF");
	if(printButtonFor(EMarketMode::RESOURCE_RESOURCE))
		new CAdventureMapButton(CGI->generaltexth->zelp[605],boost::bind(&CMarketplaceWindow::setMode,this, EMarketMode::RESOURCE_RESOURCE), 516, 450 + specialOffset,"TPMRKBU5.DEF");
	if(printButtonFor(EMarketMode::CREATURE_RESOURCE))
		new CAdventureMapButton(CGI->generaltexth->zelp[599],boost::bind(&CMarketplaceWindow::setMode,this, EMarketMode::CREATURE_RESOURCE), 516, 485,"TPMRKBU4.DEF"); //was y=450, changed to not overlap res-res in some conditions
	if(printButtonFor(EMarketMode::RESOURCE_ARTIFACT))
		new CAdventureMapButton(CGI->generaltexth->zelp[598],boost::bind(&CMarketplaceWindow::setMode,this, EMarketMode::RESOURCE_ARTIFACT), 18, 450 + specialOffset,"TPMRKBU2.DEF");
	if(printButtonFor(EMarketMode::ARTIFACT_RESOURCE))
		new CAdventureMapButton(CGI->generaltexth->zelp[613],boost::bind(&CMarketplaceWindow::setMode,this, EMarketMode::ARTIFACT_RESOURCE), 18, 485,"TPMRKBU3.DEF"); //was y=450, changed to not overlap res-art in some conditions

	updateTraderText();
}

CMarketplaceWindow::~CMarketplaceWindow()
{
	hLeft = hRight = NULL;
	for(int i=0;i<items[1].size();i++)
		delete items[1][i];
	for(int i=0;i<items[0].size();i++)
		delete items[0][i];

	items[1].clear();
	items[0].clear();
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
	switch (mode)
	{
		case EMarketMode::CREATURE_RESOURCE:
			leftIdToSend = hLeft->serial;
			break;
		case EMarketMode::ARTIFACT_RESOURCE:
			leftIdToSend = hLeft->getArtInstance()->id.getNum();
			break;
		default:
			leftIdToSend = hLeft->id;
			break;
	}

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
	if(mode == EMarketMode::RESOURCE_RESOURCE)
		readyToTrade = readyToTrade && (hLeft->id != hRight->id); //for resource trade, two DIFFERENT resources must be selected

 	if(mode == EMarketMode::ARTIFACT_RESOURCE && !hLeft)
		arts->unmarkSlots(false);

	if(readyToTrade)
	{
		int soldItemId = hLeft->id;
		market->getOffer(soldItemId, hRight->id, r1, r2, mode);

		if(slider)
		{
			int newAmount = -1;
			if(itemsType[1] == RESOURCE)
				newAmount = LOCPLINT->cb->getResourceAmount(static_cast<Res::ERes>(soldItemId));
			else if(itemsType[1] ==  CREATURE)
				newAmount = hero->getStackCount(SlotID(hLeft->serial)) - (hero->Slots().size() == 1  &&  hero->needsLastStack());
			else
				assert(0);

			slider->setAmount(newAmount / r1);
			slider->moveTo(0);
			max->block(false);
			deal->block(false);
		}
		else if(itemsType[1] == RESOURCE) //buying -> check if we can afford transaction
		{
			deal->block(LOCPLINT->cb->getResourceAmount(static_cast<Res::ERes>(soldItemId)) < r1);
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

bool CMarketplaceWindow::printButtonFor(EMarketMode::EMarketMode M) const
{
	return market->allowsTrade(M) && M != mode && (hero || ( M != EMarketMode::CREATURE_RESOURCE && M != EMarketMode::RESOURCE_ARTIFACT && M != EMarketMode::ARTIFACT_RESOURCE ));
}

void CMarketplaceWindow::garrisonChanged()
{
	if(mode != EMarketMode::CREATURE_RESOURCE)
		return;

	std::set<CTradeableItem *> toRemove;
	getEmptySlots(toRemove);


	removeItems(toRemove);
	initSubs(true);
}

void CMarketplaceWindow::artifactsChanged(bool Left)
{
	assert(!Left);
	if(mode != EMarketMode::RESOURCE_ARTIFACT)
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
			if(mode == EMarketMode::ARTIFACT_RESOURCE)
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
		if(mode == EMarketMode::RESOURCE_PLAYER)
		{
			//I can give %s to the %s player.
			traderText->setTxt(boost::str(boost::format(CGI->generaltexth->allTexts[165]) % hLeft->getName() % hRight->getName()));
		}
		else if(mode == EMarketMode::RESOURCE_ARTIFACT)
		{
			//I can offer you the %s for %d %s of %s.
			traderText->setTxt(boost::str(boost::format(CGI->generaltexth->allTexts[267]) % hRight->getName() % r1 % CGI->generaltexth->allTexts[160 + (r1==1)] % hLeft->getName()));
		}
		else if(mode == EMarketMode::RESOURCE_RESOURCE)
		{
			//I can offer you %d %s of %s for %d %s of %s.
			traderText->setTxt(boost::str(boost::format(CGI->generaltexth->allTexts[157]) % r2 % CGI->generaltexth->allTexts[160 + (r2==1)] % hRight->getName() % r1 % CGI->generaltexth->allTexts[160 + (r1==1)] % hLeft->getName()));
		}
		else if(mode == EMarketMode::CREATURE_RESOURCE)
		{
			//I can offer you %d %s of %s for %d %s.
			traderText->setTxt(boost::str(boost::format(CGI->generaltexth->allTexts[269]) % r2 % CGI->generaltexth->allTexts[160 + (r2==1)] % hRight->getName() % r1 % hLeft->getName(r1)));
		}
		else if(mode == EMarketMode::ARTIFACT_RESOURCE)
		{
			//I can offer you %d %s of %s for your %s.
			traderText->setTxt(boost::str(boost::format(CGI->generaltexth->allTexts[268]) % r2 % CGI->generaltexth->allTexts[160 + (r2==1)] % hRight->getName() % hLeft->getName(r1)));
		}
		return;
	}

	int gnrtxtnr = -1;
	if(madeTransaction)
	{
		if(mode == EMarketMode::RESOURCE_PLAYER)
			gnrtxtnr = 166; //Are there any other resources you'd like to give away?
		else
			gnrtxtnr = 162; //You have received quite a bargain.  I expect to make no profit on the deal.  Can I interest you in any of my other wares?
	}
	else
	{
		if(mode == EMarketMode::RESOURCE_PLAYER)
			gnrtxtnr = 167; //If you'd like to give any of your resources to another player, click on the item you wish to give and to whom.
		else
			gnrtxtnr = 163; //Please inspect our fine wares.  If you feel like offering a trade, click on the items you wish to trade with and for.
	}
	traderText->setTxt(CGI->generaltexth->allTexts[gnrtxtnr]);
}

CAltarWindow::CAltarWindow(const IMarket *Market, const CGHeroInstance *Hero /*= NULL*/, EMarketMode::EMarketMode Mode)
	:CTradeWindow((Mode == EMarketMode::CREATURE_EXP ? "ALTARMON.bmp" : "ALTRART2.bmp"), Market, Hero, Mode)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL;
	if(Mode == EMarketMode::CREATURE_EXP)
	{
		//%s's Creatures
		new CLabel(155, 30, FONT_SMALL, CENTER, Colors::YELLOW,
		           boost::str(boost::format(CGI->generaltexth->allTexts[272]) % hero->name));

		//Altar of Sacrifice
		new CLabel(450, 30, FONT_SMALL, CENTER, Colors::YELLOW, CGI->generaltexth->allTexts[479]);

		 //To sacrifice creatures, move them from your army on to the Altar and click Sacrifice
		new CTextBox(CGI->generaltexth->allTexts[480], Rect(320, 56, 256, 40), 0, FONT_SMALL, CENTER, Colors::YELLOW);

		slider = new CSlider(231,481,137,0,0,0);
		slider->moved = boost::bind(&CAltarWindow::sliderMoved,this,_1);
		max = new CAdventureMapButton(CGI->generaltexth->zelp[578],boost::bind(&CSlider::moveToMax, slider),147,520,"IRCBTNS.DEF");

		sacrificedUnits.resize(GameConstants::ARMY_SIZE, 0);
		sacrificeAll = new CAdventureMapButton(CGI->generaltexth->zelp[579],boost::bind(&CAltarWindow::SacrificeAll,this),393,520,"ALTARMY.DEF");
		sacrificeBackpack = NULL;

		initItems(true);
		mimicCres();
		artIcon = nullptr;
	}
	else
	{
		//Sacrifice artifacts for experience
		new CLabel(450, 34, FONT_SMALL, CENTER, Colors::YELLOW, CGI->generaltexth->allTexts[477]);
		//%s's Creatures
		new CLabel(302, 423, FONT_SMALL, CENTER, Colors::YELLOW, CGI->generaltexth->allTexts[478]);

		sacrificeAll = new CAdventureMapButton(CGI->generaltexth->zelp[571], boost::bind(&CAltarWindow::SacrificeAll,this),393,520,"ALTFILL.DEF");
		sacrificeAll->block(hero->artifactsInBackpack.empty() && hero->artifactsWorn.empty());
		sacrificeBackpack = new CAdventureMapButton(CGI->generaltexth->zelp[570],boost::bind(&CAltarWindow::SacrificeBackpack,this),147,520,"ALTEMBK.DEF");
		sacrificeBackpack->block(hero->artifactsInBackpack.empty());

		slider = NULL;
		max = NULL;

		initItems(true);
		initItems(false);
		artIcon = new CAnimImage("ARTIFACT", 0, 0, 281, 442);
		artIcon->disable();
	}

	//Experience needed to reach next level
	new CTextBox(CGI->generaltexth->allTexts[475], Rect(15, 415, 125, 50), 0, FONT_SMALL, CENTER, Colors::YELLOW);
	//Total experience on the Altar
	new CTextBox(CGI->generaltexth->allTexts[476], Rect(15, 495, 125, 40), 0, FONT_SMALL, CENTER, Colors::YELLOW);

	new CGStatusBar(new CPicture(*background, Rect(8, pos.h - 26, pos.w - 16, 19), 8, pos.h - 26));

	ok = new CAdventureMapButton(CGI->generaltexth->zelp[568],boost::bind(&CGuiHandler::popIntTotally,&GH,this),516,520,"IOK6432.DEF",SDLK_RETURN);
	ok->assignedKeys.insert(SDLK_ESCAPE);

	deal = new CAdventureMapButton(CGI->generaltexth->zelp[585],boost::bind(&CAltarWindow::makeDeal,this),269,520,"ALTSACR.DEF");

	if(Hero->getAlignment() != ::EAlignment::EVIL && Mode == EMarketMode::CREATURE_EXP)
		new CAdventureMapButton(CGI->generaltexth->zelp[580], boost::bind(&CTradeWindow::setMode,this, EMarketMode::ARTIFACT_EXP), 516, 421, "ALTART.DEF");
	if(Hero->getAlignment() != ::EAlignment::GOOD && Mode == EMarketMode::ARTIFACT_EXP)
		new CAdventureMapButton(CGI->generaltexth->zelp[572], boost::bind(&CTradeWindow::setMode,this, EMarketMode::CREATURE_EXP), 516, 421, "ALTSACC.DEF");

	expPerUnit.resize(GameConstants::ARMY_SIZE, 0);
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
	if(mode == EMarketMode::CREATURE_EXP)
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
			t->setType(CREATURE_PLACEHOLDER);
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
			t->setID(-1);
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
	if(mode == EMarketMode::CREATURE_EXP)
	{
		bool movedAnything = false;
		BOOST_FOREACH(CTradeableItem *t, items[1])
			sacrificedUnits[t->serial] = hero->getStackCount(SlotID(t->serial));

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
		for(auto i = hero->artifactsWorn.cbegin(); i != hero->artifactsWorn.cend(); i++)
		{
			if(i->second.artifact->artType->id != ArtifactID::ART_LOCK) //ignore locks from assembled artifacts
				moveFromSlotToAltar(i->first, NULL, i->second.artifact);
		}

		SacrificeBackpack();
	}
	redraw();
}

void CAltarWindow::selectionChanged(bool side)
{
	if(mode != EMarketMode::CREATURE_EXP)
		return;

	CTradeableItem *&selected = side ? hLeft : hRight;
	CTradeableItem *&theOther = side ? hRight : hLeft;

	theOther = *std::find_if(items[!side].begin(), items[!side].end(), boost::bind(&CTradeableItem::serial, _1) == selected->serial);

	int stackCount = 0;
	for (int i = 0; i < GameConstants::ARMY_SIZE; i++)
		if(hero->getStackCount(SlotID(i)) > sacrificedUnits[i])
			stackCount++;

	slider->setAmount(hero->getStackCount(SlotID(hLeft->serial)) - (stackCount == 1));
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
		CTradeableItem *hlp = new CTradeableItem(positions[t->serial].topLeft(), CREATURE_PLACEHOLDER, t->id, false, t->serial);
		hlp->pos = positions[t->serial] + this->pos.topLeft();
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
	if(mode != EMarketMode::CREATURE_EXP)
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
			market->getOffer(t->id, 0, dump, expPerUnit[t->serial], EMarketMode::CREATURE_EXP);
}

void CAltarWindow::calcTotalExp()
{
	int val = 0;
	if(mode == EMarketMode::CREATURE_EXP)
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
	val = hero->calculateXp(val);
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
	toUpdate->setType(val ? CREATURE : CREATURE_PLACEHOLDER);
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
	if(mode == EMarketMode::ARTIFACT_EXP && arts && arts->commonInfo->src.art)
	{
		artIcon->setFrame(arts->commonInfo->src.art->artType->iconIndex);
		artIcon->showAll(to);

		int dmp, val;
		market->getOffer(arts->commonInfo->src.art->artType->id, 0, dmp, val, EMarketMode::ARTIFACT_EXP);
		printAtMiddleLoc(boost::lexical_cast<std::string>(val), 304, 498, FONT_SMALL, Colors::WHITE, to);
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
	market->getOffer(artID, 0, dmp, val, EMarketMode::ARTIFACT_EXP);

	arts->artifactsOnAltar.insert(art);
	altarSlot->setArtInstance(art);
	altarSlot->subtitle = boost::lexical_cast<std::string>(val);

	deal->block(false);
	return true;
}

void CAltarWindow::moveFromSlotToAltar(ArtifactPosition slotID, CTradeableItem* altarSlot, const CArtifactInstance *art)
{
	auto freeBackpackSlot = ArtifactPosition(hero->artifactsInBackpack.size() + GameConstants::BACKPACK_START);
	if(arts->commonInfo->src.art)
	{
		arts->commonInfo->dst.slotID = freeBackpackSlot;
		arts->commonInfo->dst.AOH = arts;
	}

	if(putOnAltar(altarSlot, art))
	{
		if(slotID < GameConstants::BACKPACK_START)
			LOCPLINT->cb->swapArtifacts(ArtifactLocation(hero, slotID), ArtifactLocation(hero, freeBackpackSlot));
		else
		{
			arts->commonInfo->src.clear();
			arts->commonInfo->dst.clear();
			CCS->curh->dragAndDropCursor(NULL);
			arts->unmarkSlots(false);
		}
	}
}

void CSystemOptionsWindow::setMusicVolume( int newVolume )
{
	Settings volume = settings.write["general"]["music"];
	volume->Float() = newVolume;
}

void CSystemOptionsWindow::setSoundVolume( int newVolume )
{
	Settings volume = settings.write["general"]["sound"];
	volume->Float() = newVolume;
}

void CSystemOptionsWindow::setHeroMoveSpeed( int newSpeed )
{
	Settings speed = settings.write["adventure"]["heroSpeed"];
	speed->Float() = newSpeed;
}

void CSystemOptionsWindow::setMapScrollingSpeed( int newSpeed )
{
	Settings speed = settings.write["adventure"]["scrollSpeed"];
	speed->Float() = newSpeed;
}

CSystemOptionsWindow::CSystemOptionsWindow():
    CWindowObject(PLAYER_COLORED, "SysOpBck"),
    onFullscreenChanged(settings.listen["video"]["fullscreen"])
{
	//TODO: translation and\or config file
	static const std::string fsLabel = "Fullscreen";
	static const std::string fsHelp  = "{Fullscreen}\n\n If selected, VCMI will run in fullscreen mode, othervice VCMI will run in window";
	static const std::string cwLabel = "Classic creature window";
	static const std::string cwHelp  = "{Classic creature window}\n\n Enable original Heroes 3 creature window instead of new window from VCMI";
	static const std::string rsLabel = "Resolution";
	static const std::string rsHelp = "{Select resolution}\n\n Change in-game screen resolution. Will only affect adventure map. Game restart required to apply new resolution.";

	OBJ_CONSTRUCTION_CAPTURING_ALL;
	title = new CLabel(242, 32, FONT_BIG, CENTER, Colors::YELLOW, CGI->generaltexth->allTexts[568]);

	//left window section
	leftGroup =  new CLabelGroup(FONT_MEDIUM, CENTER, Colors::YELLOW);
	leftGroup->add(122,  64, CGI->generaltexth->allTexts[569]);
	leftGroup->add(122, 130, CGI->generaltexth->allTexts[570]);
	leftGroup->add(122, 196, CGI->generaltexth->allTexts[571]);
	leftGroup->add(122, 262, rsLabel); //CGI->generaltexth->allTexts[20]
	leftGroup->add(122, 347, CGI->generaltexth->allTexts[394]);
	leftGroup->add(122, 412, CGI->generaltexth->allTexts[395]);

	//right section
	rightGroup = new CLabelGroup(FONT_MEDIUM, TOPLEFT, Colors::WHITE);
	rightGroup->add(282, 57,  CGI->generaltexth->allTexts[572]);
	rightGroup->add(282, 89,  CGI->generaltexth->allTexts[573]);
	rightGroup->add(282, 121, CGI->generaltexth->allTexts[574]);
	rightGroup->add(282, 153, CGI->generaltexth->allTexts[575]);
	rightGroup->add(282, 185, cwLabel); //CGI->generaltexth->allTexts[576]
	rightGroup->add(282, 217, fsLabel); //CGI->generaltexth->allTexts[577]

	//setting up buttons
	load = new CAdventureMapButton (CGI->generaltexth->zelp[321].first, CGI->generaltexth->zelp[321].second,
									boost::bind(&CSystemOptionsWindow::bloadf, this), 246,  298, "SOLOAD.DEF", SDLK_l);
	load->swappedImages = true;
	load->update();

	save = new CAdventureMapButton (CGI->generaltexth->zelp[322].first, CGI->generaltexth->zelp[322].second,
	                                boost::bind(&CSystemOptionsWindow::bsavef, this), 357, 298, "SOSAVE.DEF", SDLK_s);
	save->swappedImages = true;
	save->update();

	restart = new CAdventureMapButton (CGI->generaltexth->zelp[323].first, CGI->generaltexth->zelp[323].second,
									   boost::bind(&CSystemOptionsWindow::brestartf, this), 246, 357, "SORSTRT", SDLK_r);
	restart->swappedImages = true;
	restart->update();

	mainMenu = new CAdventureMapButton (CGI->generaltexth->zelp[320].first, CGI->generaltexth->zelp[320].second,
	                                    boost::bind(&CSystemOptionsWindow::bmainmenuf, this), 357, 357, "SOMAIN.DEF", SDLK_m);
	mainMenu->swappedImages = true;
	mainMenu->update();

	quitGame = new CAdventureMapButton (CGI->generaltexth->zelp[324].first, CGI->generaltexth->zelp[324].second,
	                                    boost::bind(&CSystemOptionsWindow::bquitf, this), 246, 415, "soquit.def", SDLK_q);
	quitGame->swappedImages = true;
	quitGame->update();
	backToMap = new CAdventureMapButton (CGI->generaltexth->zelp[325].first, CGI->generaltexth->zelp[325].second,
	                                     boost::bind(&CSystemOptionsWindow::breturnf, this), 357, 415, "soretrn.def", SDLK_RETURN);
	backToMap->swappedImages = true;
	backToMap->update();
	backToMap->assignedKeys.insert(SDLK_ESCAPE);

	heroMoveSpeed = new CHighlightableButtonsGroup(0);
	heroMoveSpeed->addButton(boost::assign::map_list_of(0,CGI->generaltexth->zelp[349].second),CGI->generaltexth->zelp[349].second, "sysopb1.def", 28, 77, 1);
	heroMoveSpeed->addButton(boost::assign::map_list_of(0,CGI->generaltexth->zelp[350].second),CGI->generaltexth->zelp[350].second, "sysopb2.def", 76, 77, 2);
	heroMoveSpeed->addButton(boost::assign::map_list_of(0,CGI->generaltexth->zelp[351].second),CGI->generaltexth->zelp[351].second, "sysopb3.def", 124, 77, 4);
	heroMoveSpeed->addButton(boost::assign::map_list_of(0,CGI->generaltexth->zelp[352].second),CGI->generaltexth->zelp[352].second, "sysopb4.def", 172, 77, 8);
	heroMoveSpeed->select(settings["adventure"]["heroSpeed"].Float(), 1);
	heroMoveSpeed->onChange = boost::bind(&CSystemOptionsWindow::setHeroMoveSpeed, this, _1);

	mapScrollSpeed = new CHighlightableButtonsGroup(0);
	mapScrollSpeed->addButton(boost::assign::map_list_of(0,CGI->generaltexth->zelp[357].second),CGI->generaltexth->zelp[357].second, "sysopb9.def", 28, 210, 1);
	mapScrollSpeed->addButton(boost::assign::map_list_of(0,CGI->generaltexth->zelp[358].second),CGI->generaltexth->zelp[358].second, "sysob10.def", 92, 210, 2);
	mapScrollSpeed->addButton(boost::assign::map_list_of(0,CGI->generaltexth->zelp[359].second),CGI->generaltexth->zelp[359].second, "sysob11.def", 156, 210, 4);
	mapScrollSpeed->select(settings["adventure"]["scrollSpeed"].Float(), 1);
	mapScrollSpeed->onChange = boost::bind(&CSystemOptionsWindow::setMapScrollingSpeed, this, _1);

	musicVolume = new CHighlightableButtonsGroup(0, true);
	for(int i=0; i<10; ++i)
		musicVolume->addButton(boost::assign::map_list_of(0,CGI->generaltexth->zelp[326+i].second),CGI->generaltexth->zelp[326+i].second, "syslb.def", 29 + 19*i, 359, i*11);

	musicVolume->select(CCS->musich->getVolume(), 1);
	musicVolume->onChange = boost::bind(&CSystemOptionsWindow::setMusicVolume, this, _1);

	effectsVolume = new CHighlightableButtonsGroup(0, true);
	for(int i=0; i<10; ++i)
		effectsVolume->addButton(boost::assign::map_list_of(0,CGI->generaltexth->zelp[336+i].second),CGI->generaltexth->zelp[336+i].second, "syslb.def", 29 + 19*i, 425, i*11);

	effectsVolume->select(CCS->soundh->getVolume(), 1);
	effectsVolume->onChange = boost::bind(&CSystemOptionsWindow::setSoundVolume, this, _1);

	showReminder = new CHighlightableButton(
		boost::bind(&CSystemOptionsWindow::toggleReminder, this, true), boost::bind(&CSystemOptionsWindow::toggleReminder, this, false),
		std::map<int,std::string>(), CGI->generaltexth->zelp[361].second, false, "sysopchk.def", NULL, 246, 87, false);

	newCreatureWin = new CHighlightableButton(
		boost::bind(&CSystemOptionsWindow::toggleCreatureWin, this, true), boost::bind(&CSystemOptionsWindow::toggleCreatureWin, this, false),
		std::map<int,std::string>(), cwHelp, false, "sysopchk.def", NULL, 246, 183, false);

	fullscreen = new CHighlightableButton(
		boost::bind(&CSystemOptionsWindow::toggleFullscreen, this, true), boost::bind(&CSystemOptionsWindow::toggleFullscreen, this, false),
		std::map<int,std::string>(), fsHelp, false, "sysopchk.def", NULL, 246, 215, false);

	showReminder->select(settings["adventure"]["heroReminder"].Bool());
	newCreatureWin->select(settings["general"]["classicCreatureWindow"].Bool());
	fullscreen->select(settings["video"]["fullscreen"].Bool());

	onFullscreenChanged([&](const JsonNode &newState){ fullscreen->select(newState.Bool());});

	gameResButton = new CAdventureMapButton("", rsHelp, boost::bind(&CSystemOptionsWindow::selectGameRes, this), 28, 275,"SYSOB12", SDLK_g);

	std::string resText;
	resText += boost::lexical_cast<std::string>(settings["video"]["screenRes"]["width"].Float());
	resText += "x";
	resText += boost::lexical_cast<std::string>(settings["video"]["screenRes"]["height"].Float());
	gameResLabel = new CLabel(170, 292, FONT_MEDIUM, CENTER, Colors::YELLOW, resText);

}

void CSystemOptionsWindow::selectGameRes()
{
	//TODO: translation and\or config file
	static const std::string rsLabel = "Select resolution";
	static const std::string rsHelp = "Change in-game screen resolution.";

	std::vector<std::string> items;

	BOOST_FOREACH( config::CConfigHandler::GuiOptionsMap::value_type& value, conf.guiOptions)
	{
		std::string resX = boost::lexical_cast<std::string>(value.first.first);
		std::string resY = boost::lexical_cast<std::string>(value.first.second);
		items.push_back(resX + 'x' + resY);
	}

	GH.pushInt(new CObjectListWindow(items, NULL, rsLabel, rsHelp,
			   boost::bind(&CSystemOptionsWindow::setGameRes, this, _1)));
}

void CSystemOptionsWindow::setGameRes(int index)
{
	auto iter = conf.guiOptions.begin();
	std::advance(iter, index);

	//do not set resolution to illegal one (0x0)
	assert(iter!=conf.guiOptions.end() && iter->first.first > 0 && iter->first.second > 0);

	Settings gameRes = settings.write["video"]["screenRes"];
	gameRes["width"].Float() = iter->first.first;
	gameRes["height"].Float() = iter->first.second;

	std::string resText;
	resText += boost::lexical_cast<std::string>(iter->first.first);
	resText += "x";
	resText += boost::lexical_cast<std::string>(iter->first.second);
	gameResLabel->setTxt(resText);
}

void CSystemOptionsWindow::toggleReminder(bool on)
{
	Settings heroReminder = settings.write["adventure"]["heroReminder"];
	heroReminder->Bool() = on;
}

void CSystemOptionsWindow::toggleCreatureWin(bool on)
{
	Settings classicCreatureWindow = settings.write["general"]["classicCreatureWindow"];
	classicCreatureWindow->Bool() = on;
}

void CSystemOptionsWindow::toggleFullscreen(bool on)
{
	Settings fullscreen = settings.write["video"]["fullscreen"];
	fullscreen->Bool() = on;
}

void CSystemOptionsWindow::bquitf()
{
	LOCPLINT->showYesNoDialog(CGI->generaltexth->allTexts[578], [this]{ closeAndPushEvent(SDL_QUIT); }, 0);
}

void CSystemOptionsWindow::breturnf()
{
	GH.popIntTotally(this);
}

void CSystemOptionsWindow::bmainmenuf()
{
	LOCPLINT->showYesNoDialog(CGI->generaltexth->allTexts[578], [this]{ closeAndPushEvent(SDL_USEREVENT, RETURN_TO_MAIN_MENU); }, 0);
}

void CSystemOptionsWindow::bloadf()
{
	GH.popIntTotally(this);
	LOCPLINT->proposeLoadingGame();
}

void CSystemOptionsWindow::bsavef()
{
	GH.popIntTotally(this);
	GH.pushInt(new CSavingScreen(CPlayerInterface::howManyPeople > 1));
}

void CSystemOptionsWindow::brestartf()
{
	LOCPLINT->showYesNoDialog(CGI->generaltexth->allTexts[67], [this]{ closeAndPushEvent(SDL_USEREVENT, RESTART_GAME); }, 0);
}

void CSystemOptionsWindow::closeAndPushEvent(int eventType, int code /*= 0*/)
{
	GH.popIntTotally(this);
	GH.pushSDLEvent(eventType, code);
}

CTavernWindow::CTavernWindow(const CGObjectInstance *TavernObj):
    CWindowObject(PLAYER_COLORED, "TPTAVERN"),
	tavernObj(TavernObj)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL;
	std::vector<const CGHeroInstance*> h = LOCPLINT->cb->getAvailableHeroes(TavernObj);
	assert(h.size() == 2);

	h1 = new HeroPortrait(selected,0,72,299,h[0]);
	h2 = new HeroPortrait(selected,1,162,299,h[1]);

	selected = 0;
	if (!h[0])
		selected = 1;
	if (!h[0] && !h[1])
		selected = -1;
	oldSelected = -1;

	new CLabel(200, 35, FONT_BIG, CENTER, Colors::YELLOW, CGI->generaltexth->jktexts[37]);
	new CLabel(320, 328, FONT_SMALL, CENTER, Colors::WHITE, "2500");
	new CTextBox(LOCPLINT->cb->getTavernGossip(tavernObj), Rect(32, 190, 330, 68), 0, FONT_SMALL, CENTER, Colors::WHITE);

	new CGStatusBar(new CPicture(*background, Rect(8, pos.h - 26, pos.w - 16, 19), 8, pos.h - 26));
	cancel = new CAdventureMapButton(CGI->generaltexth->tavernInfo[7],"", boost::bind(&CTavernWindow::close, this), 310, 428, "ICANCEL.DEF", SDLK_ESCAPE);
	recruit = new CAdventureMapButton("", "", boost::bind(&CTavernWindow::recruitb, this), 272, 355, "TPTAV01.DEF", SDLK_RETURN);
	thiefGuild = new CAdventureMapButton(CGI->generaltexth->tavernInfo[5],"", boost::bind(&CTavernWindow::thievesguildb, this), 22, 428, "TPTAV02.DEF", SDLK_t);

	if(LOCPLINT->cb->getResourceAmount(Res::GOLD) < 2500) //not enough gold
	{
		recruit->hoverTexts[0] = CGI->generaltexth->tavernInfo[0]; //Cannot afford a Hero
		recruit->block(true);
	}
	else if(LOCPLINT->cb->howManyHeroes(false) >= 8)
	{
		recruit->hoverTexts[0] = CGI->generaltexth->tavernInfo[1]; //Cannot recruit. You already have %d Heroes.
		boost::algorithm::replace_first(recruit->hoverTexts[0],"%d",boost::lexical_cast<std::string>(LOCPLINT->cb->howManyHeroes()));
		recruit->block(true);
	}
	else if(LOCPLINT->castleInt && LOCPLINT->castleInt->town->visitingHero)
	{
		recruit->hoverTexts[0] = CGI->generaltexth->tavernInfo[2]; //Cannot recruit. You already have a Hero in this town.
		recruit->block(true);
	}
	else
	{
		if(selected == -1)
			recruit->block(true);
	}

	CCS->videoh->open("TAVERN.BIK");
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

void CTavernWindow::show(SDL_Surface * to)
{
	CWindowObject::show(to);

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

		printAtMiddleWBLoc(sel->descr, 146, 395, FONT_SMALL, 200, Colors::WHITE, to);
		CSDL_Ext::drawBorder(to,sel->pos.x-2,sel->pos.y-2,sel->pos.w+4,sel->pos.h+4,int3(247,223,123));
	}
}

void CTavernWindow::HeroPortrait::clickLeft(tribool down, bool previousState)
{
	if(previousState && !down && h)
		*_sel = _id;
}

void CTavernWindow::HeroPortrait::clickRight(tribool down, bool previousState)
{
	if(down && h)
	{
		GH.pushInt(new CRClickPopupInt(new CHeroWindow(h), true));
	}
}

CTavernWindow::HeroPortrait::HeroPortrait(int &sel, int id, int x, int y, const CGHeroInstance *H)
: h(H), _sel(&sel), _id(id)
{
	addUsedEvents(LCLICK | RCLICK | HOVER);
	OBJ_CONSTRUCTION_CAPTURING_ALL;
	h = H;
	pos.x += x;
	pos.y += y;
	pos.w = 58;
	pos.h = 64;

	if(H)
	{
		hoverName = CGI->generaltexth->tavernInfo[4];
		boost::algorithm::replace_first(hoverName,"%s",H->name);

		int artifs = h->artifactsWorn.size() + h->artifactsInBackpack.size();
		for(int i=13; i<=17; i++) //war machines and spellbook don't count
			if(vstd::contains(h->artifactsWorn, ArtifactPosition(i)))
				artifs--;
		sprintf_s(descr, sizeof(descr),CGI->generaltexth->allTexts[215].c_str(),
				  h->name.c_str(), h->level, h->type->heroClass->name.c_str(), artifs);
		descr[sizeof(descr)-1] = '\0';

		new CAnimImage("portraitsLarge", h->portrait);
	}
}

void CTavernWindow::HeroPortrait::hover( bool on )
{
	//Hoverable::hover(on);
	if(on)
		GH.statusbar->print(hoverName);
	else
		GH.statusbar->clear();
}

void CInGameConsole::show(SDL_Surface * to)
{
	int number = 0;

	std::vector<std::list< std::pair< std::string, int > >::iterator> toDel;

	boost::unique_lock<boost::mutex> lock(texts_mx);
	for(std::list< std::pair< std::string, int > >::iterator it = texts.begin(); it != texts.end(); ++it, ++number)
	{
		Point leftBottomCorner(0, screen->h);
		if(LOCPLINT->battleInt)
		{
			leftBottomCorner = LOCPLINT->battleInt->pos.bottomLeft();
		}
		graphics->fonts[FONT_MEDIUM]->renderTextLeft(to, it->first, Colors::GREEN,
		    Point(leftBottomCorner.x + 50, leftBottomCorner.y - texts.size() * 20 - 80 + number*20));

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
	boost::unique_lock<boost::mutex> lock(texts_mx);
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
        GH.statusbar->alignment = TOPLEFT;
        GH.statusbar->calcOffset();
		GH.statusbar->print(enteredText);

        //Prevent changes to the text from mouse interaction with the adventure map
        GH.statusbar->lock(true);
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
		//print(txt);
	}
	enteredText = "";
	if(GH.topInt() == adventureInt)
	{
        GH.statusbar->alignment = CENTER;
        GH.statusbar->calcOffset();
        GH.statusbar->lock(false);
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
        GH.statusbar->lock(false);
        GH.statusbar->clear();
		GH.statusbar->print(enteredText);
        GH.statusbar->lock(true);
	}
	else if(LOCPLINT->battleInt)
	{
		LOCPLINT->battleInt->console->ingcAlter = enteredText;
	}
}

CInGameConsole::CInGameConsole() : prevEntDisp(-1), defaultTimeout(10000), maxDisplayedTexts(10)
{
	addUsedEvents(KEYBOARD);
}

CGarrisonWindow::CGarrisonWindow( const CArmedInstance *up, const CGHeroInstance *down, bool removableUnits ):
    CWindowObject(PLAYER_COLORED, "GARRISON")
{
	OBJ_CONSTRUCTION_CAPTURING_ALL;

	garr = new CGarrisonInt(92, 127, 4, Point(0,96), background->bg, Point(93,127), up, down, removableUnits);
	{
		CAdventureMapButton *split = new CAdventureMapButton(CGI->generaltexth->tcommands[3],"",boost::bind(&CGarrisonInt::splitClick,garr),88,314,"IDV6432.DEF");
		removeChild(split);
		garr->addSplitBtn(split);
	}
	quit = new CAdventureMapButton(CGI->generaltexth->tcommands[8],"",boost::bind(&CGarrisonWindow::close,this),399,314,"IOK6432.DEF",SDLK_RETURN);

	std::string titleText;
	if (garr->armedObjs[1]->tempOwner == garr->armedObjs[0]->tempOwner)
		titleText = CGI->generaltexth->allTexts[709];
	else
	{
		titleText = CGI->generaltexth->allTexts[35];
		boost::algorithm::replace_first(titleText, "%s", garr->armedObjs[0]->Slots().begin()->second->type->namePl);
	}
	new CLabel(275, 30, FONT_BIG, CENTER, Colors::YELLOW, titleText);

	new CAnimImage("CREST58", garr->armedObjs[0]->getOwner().getNum(), 0, 28, 124);
	new CAnimImage("PortraitsLarge", dynamic_cast<const CGHeroInstance*>(garr->armedObjs[1])->portrait, 0, 29, 222);
}


IShowActivatable::IShowActivatable()
{
	type = 0;
}

CGarrisonHolder::CGarrisonHolder()
{
}

void CWindowWithGarrison::updateGarrisons()
{
	garr->recreateSlots();
}

CArtPlace::CArtPlace(Point position, const CArtifactInstance * Art):
    locked(false), picked(false), marked(false), ourArt(Art)
{
	pos += position;
	pos.w = pos.h = 44;
	createImage();
}

void CArtPlace::createImage()
{
	OBJ_CONSTRUCTION_CAPTURING_ALL;

	int graphic = 0;
	if (ourArt)
		graphic = ourArt->artType->iconIndex;
	if (locked)
		graphic = ArtifactID::ART_LOCK;

	image = new CAnimImage("artifact", graphic);
	if (!ourArt)
		image->disable();

	selection = new CAnimImage("artifact", ArtifactID::ART_SELECTION);
	selection->disable();
}

void CArtPlace::lockSlot(bool on)
{
	if (locked == on)
		return;

	locked = on;

	if (on)
		image->setFrame(ArtifactID::ART_LOCK);
	else
		image->setFrame(ourArt->artType->iconIndex);
}

void CArtPlace::pickSlot(bool on)
{
	if (picked == on)
		return;

	picked = on;
	if (on)
		image->disable();
	else
		image->enable();
}

void CArtPlace::selectSlot(bool on)
{
	if (marked == on)
		return;

	marked = on;
	if (on)
		selection->enable();
	else
		selection->disable();
}

void CArtPlace::clickLeft(tribool down, bool previousState)
{
	//LRClickableAreaWTextComp::clickLeft(down);
	bool inBackpack = slotID >= GameConstants::BACKPACK_START,
		srcInBackpack = ourOwner->commonInfo->src.slotID >= GameConstants::BACKPACK_START,
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
				selectSlot(true);
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
			CSpellWindow * spellWindow = new CSpellWindow(genRect(595, 620, (screen->w - 620)/2, (screen->h - 595)/2), ourOwner->curHero, LOCPLINT, LOCPLINT->battleInt);
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
					std::vector<CComponent *> catapult(1, new CComponent(CComponent::artifact, 3, 0));
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
					case ArtifactID::CATAPULT:
						//should not happen, catapult cannot be selected
						assert(cur->id != ArtifactID::CATAPULT);
						break;
					case ArtifactID::BALLISTA: case ArtifactID::AMMO_CART: case ArtifactID::FIRST_AID_TENT: //war machines cannot go to backpack
						LOCPLINT->showInfoDialog(boost::str(boost::format(CGI->generaltexth->allTexts[153]) % cur->Name()));
						break;
					default:
						setMeAsDest();
						vstd::amin(ourOwner->commonInfo->dst.slotID, ArtifactPosition(
							ourOwner->curHero->artifactsInBackpack.size() + GameConstants::BACKPACK_START));
						if(srcInBackpack && srcInSameHero)
						{
							if(!ourArt								//cannot move from backpack to AFTER backpack -> combined with vstd::amin above it will guarantee that dest is at most the last artifact
							  || ourOwner->commonInfo->src.slotID < ourOwner->commonInfo->dst.slotID) //rearranging arts in backpack after taking src artifact, the dest id will be shifted
								vstd::advance(ourOwner->commonInfo->dst.slotID, -1);
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
	if(down && ourArt && !locked && text.size() && !picked)  //if there is no description or it's a lock, do nothing ;]
	{
		if (slotID < GameConstants::BACKPACK_START)
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
					return;
				}

				// Otherwise if the artifact can be diasassembled, display dialog.
				if(ourArt->canBeDisassembled())
				{
					LOCPLINT->showArtifactAssemblyDialog(
						ourArt->artType->id,
						0,
						false,
						boost::bind(&CCallback::assembleArtifacts, LOCPLINT->cb, ourOwner->curHero, slotID, false, ArtifactID()),
						0);
					return;
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

	selectSlot(true);
	pickSlot(true);
	if(ourArt->canBeDisassembled() && slotID < GameConstants::BACKPACK_START) //worn combined artifact -> locks have to disappear
	{
		for(int i = 0; i < GameConstants::BACKPACK_START; i++)
		{
			CArtPlace *ap = ourOwner->getArtPlace(i);
			ap->pickSlot(ourArt->isPart(ap->ourArt));
		}
	}

	//int backpackCorrection = -(slotID - Arts::BACKPACK_START < ourOwner->backpackPos);

	CCS->curh->dragAndDropCursor(new CAnimImage("artifact", ourArt->artType->iconIndex));
	ourOwner->commonInfo->src.setTo(this, false);
	ourOwner->markPossibleSlots(ourArt);

	if(slotID >= GameConstants::BACKPACK_START)
		ourOwner->scrollBackpack(0); //will update slots

	ourOwner->updateParentWindow();
	ourOwner->safeRedraw();
}

/**
 * Deselects the artifact slot. FIXME: Not used. Maybe it should?
 */
void CArtPlace::deselect ()
{
	pickSlot(false);
	if(ourArt && ourArt->canBeDisassembled()) //combined art returned to its slot -> restore locks
	{
		for(int i = 0; i < GameConstants::BACKPACK_START; i++)
			ourOwner->getArtPlace(i)->pickSlot(false);
	}

	CCS->curh->dragAndDropCursor(NULL);
	ourOwner->unmarkSlots();
	ourOwner->commonInfo->src.clear();
	if(slotID >= GameConstants::BACKPACK_START)
		ourOwner->scrollBackpack(0); //will update slots


	ourOwner->updateParentWindow();
	ourOwner->safeRedraw();
}

void CArtPlace::showAll(SDL_Surface * to)
{
	if (ourArt && !picked && ourArt == ourOwner->curHero->getArt(slotID, false)) //last condition is needed for disassembling -> artifact may be gone, but we don't know yet TODO: real, nice solution
	{
		CIntObject::showAll(to);
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
	if (slotID >= GameConstants::BACKPACK_START)
		return !CGI->arth->isBigArtifact(art->artType->id);

	return art->canBePutAt(ArtifactLocation(ourOwner->curHero, slotID), true);
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
		image->disable();
		text = std::string();
		hoverText = CGI->generaltexth->allTexts[507];
	}
	else
	{
		image->enable();
		image->setFrame(locked ? ArtifactID::ART_LOCK : art->artType->iconIndex);

		std::string artDesc = ourArt->artType->Description();
		if (vstd::contains (artDesc, '{'))
			text = artDesc;
		else
			text = '{' + ourArt->artType->Name() + "}\n\n" + artDesc; //workaround for new artifacts with single name, turns it to H3-style

		if(art->artType->id == 1) //spell scroll
		{
			// we expect scroll description to be like this: This scroll contains the [spell name] spell which is added into your spell book for as long as you carry the scroll.
			// so we want to replace text in [...] with a spell name
			// however other language versions don't have name placeholder at all, so we have to be careful
			int spellID = art->getGivenSpellID();
			size_t nameStart = text.find_first_of('[');
			size_t nameEnd = text.find_first_of(']', nameStart);
			if(spellID >= 0)
			{
				if(nameStart != std::string::npos  &&  nameEnd != std::string::npos)
					text = text.replace(nameStart, nameEnd - nameStart + 1, CGI->spellh->spells[spellID]->name);

				//add spell component info (used to provide a pic in r-click popup)
				baseType = CComponent::spell;
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

void LRClickableAreaWTextComp::clickLeft(tribool down, bool previousState)
{
	if((!down) && previousState)
	{
		std::vector<CComponent*> comp(1, createComponent());
		LOCPLINT->showInfoDialog(text, comp);
	}
}

LRClickableAreaWTextComp::LRClickableAreaWTextComp(const Rect &Pos, int BaseType)
	: LRClickableAreaWText(Pos), baseType(BaseType), bonusValue(-1)
{
}

CComponent * LRClickableAreaWTextComp::createComponent() const
{
	if(baseType >= 0)
		return new CComponent(CComponent::Etype(baseType), type, bonusValue);
	else
		return NULL;
}

void LRClickableAreaWTextComp::clickRight(tribool down, bool previousState)
{
	if(down)
	{
		if(CComponent *comp = createComponent())
		{
			CRClickPopup::createAndPush(text, CInfoWindow::TCompsInfo(1, comp));
			return;
		}
	}

	LRClickableAreaWText::clickRight(down, previousState); //only if with-component variant not occurred
}

CHeroArea::CHeroArea(int x, int y, const CGHeroInstance * _hero):hero(_hero)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL;

	addUsedEvents(LCLICK | RCLICK | HOVER);
	pos.x += x;	pos.w = 58;
	pos.y += y;	pos.h = 64;

	if (hero)
		new CAnimImage("PortraitsLarge", hero->portrait);
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

void LRClickableAreaOpenTown::clickLeft(tribool down, bool previousState)
{
	if((!down) && previousState && town)
		{
		LOCPLINT->openTownWindow(town);
		if ( type == 2 )
			LOCPLINT->castleInt->builds->buildingClicked(BuildingID::VILLAGE_HALL);
		else if ( type == 3 && town->fortLevel() )
			LOCPLINT->castleInt->builds->buildingClicked(BuildingID::FORT);
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
		setSlotData(artWorn[g], ArtifactPosition(g));
	scrollBackpack(0);
}

void CArtifactsOfHero::dispose()
{
	//vstd::clear_pointer(curHero);
	//unmarkSlots(false);
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
			auto slotID = ArtifactPosition(GameConstants::BACKPACK_START + (s + backpackPos)%artsInBackpack);
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
		eraseSlotData(backpack[s-omitedSoFar], ArtifactPosition(GameConstants::BACKPACK_START + s));

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
			place->selectSlot(art->canBePutAt(ArtifactLocation(aoh->curHero, place->slotID), true));

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
		place->selectSlot(false);
	BOOST_FOREACH(CArtPlace *place, backpack)
		place->selectSlot(false);

	if(withRedraw)
		safeRedraw();
}

/**
 * Assigns an artifacts to an artifact place depending on it's new slot ID.
 */
void CArtifactsOfHero::setSlotData(CArtPlace* artPlace, ArtifactPosition slotID)
{
	if(!artPlace && slotID >= GameConstants::BACKPACK_START) //spurious call from artifactMoved in attempt to update hidden backpack slot
	{
		return;
	}

	artPlace->pickSlot(false);
	artPlace->slotID = slotID;

	if(const ArtSlotInfo *asi = curHero->getSlot(slotID))
	{
		artPlace->setArtifact(asi->artifact);
		artPlace->lockSlot(asi->locked);
	}
	else
		artPlace->setArtifact(NULL);
}

/**
 * Makes given artifact slot appear as empty with a certain slot ID.
 */
void CArtifactsOfHero::eraseSlotData (CArtPlace* artPlace, ArtifactPosition slotID)
{
	artPlace->pickSlot(false);
	artPlace->slotID = slotID;
	artPlace->setArtifact(NULL);
}

CArtifactsOfHero::CArtifactsOfHero(std::vector<CArtPlace *> ArtWorn, std::vector<CArtPlace *> Backpack,
	CAdventureMapButton *leftScroll, CAdventureMapButton *rightScroll, bool createCommonPart):

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
		eraseSlotData(artWorn[g], ArtifactPosition(g));
	}

	// Init slots for the backpack.
	for(size_t s=0; s<backpack.size(); ++s)
	{
		backpack[s]->ourOwner = this;
		eraseSlotData(backpack[s], ArtifactPosition(GameConstants::BACKPACK_START + s));
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

	std::vector<Point> slotPos;
	slotPos += Point(509,30),  Point(567,240), Point(509,80),
	           Point(383,68),  Point(564,183), Point(509,130),
	           Point(431,68),  Point(610,183), Point(515,295),
	           Point(383,143), Point(399,194), Point(415,245),
	           Point(431,296), Point(564,30),  Point(610,30),
	           Point(610,76),  Point(610,122), Point(610,310),
	           Point(381,296);

	// Create slots for worn artifacts.
	for (size_t g = 0; g < GameConstants::BACKPACK_START ; g++)
	{
		artWorn[g] = new CArtPlace(slotPos[g]);
		artWorn[g]->ourOwner = this;
		eraseSlotData(artWorn[g], ArtifactPosition(g));
	}

	// Create slots for the backpack.
	for(size_t s=0; s<5; ++s)
	{
		CArtPlace * add = new CArtPlace(Point(403 + 46 * s, 365));

		add->ourOwner = this;
		eraseSlotData(add, ArtifactPosition(GameConstants::BACKPACK_START + s));

		backpack.push_back(add);
	}

	leftArtRoll = new CAdventureMapButton(std::string(), std::string(), boost::bind(&CArtifactsOfHero::scrollBackpack,this,-1), 379, 364, "hsbtns3.def", SDLK_LEFT);
	rightArtRoll = new CAdventureMapButton(std::string(), std::string(), boost::bind(&CArtifactsOfHero::scrollBackpack,this,+1), 632, 364, "hsbtns5.def", SDLK_RIGHT);
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
	if (active)
	{
		if(parent)
			parent->redraw();
		else
			redraw();
	}
}

void CArtifactsOfHero::realizeCurrentTransaction()
{
	assert(commonInfo->src.AOH);
	assert(commonInfo->dst.AOH);
	LOCPLINT->cb->swapArtifacts(ArtifactLocation(commonInfo->src.AOH->curHero, commonInfo->src.slotID),
								ArtifactLocation(commonInfo->dst.AOH->curHero, commonInfo->dst.slotID));
}

void CArtifactsOfHero::artifactMoved(const ArtifactLocation &src, const ArtifactLocation &dst)
{
	bool isCurHeroSrc = src.isHolder(curHero),
		isCurHeroDst = dst.isHolder(curHero);
	if(isCurHeroSrc && src.slot >= GameConstants::BACKPACK_START)
		updateSlot(src.slot);
	if(isCurHeroDst && dst.slot >= GameConstants::BACKPACK_START)
		updateSlot(dst.slot);
	if(isCurHeroSrc  ||  isCurHeroDst) //we need to update all slots, artifact might be combined and affect more slots
		updateWornSlots(false);

	if (!src.isHolder(curHero) && !isCurHeroDst)
		return;

	if(commonInfo->src == src) //artifact was taken from us
	{
		assert(commonInfo->dst == dst  //expected movement from slot ot slot
			||  dst.slot == dst.getHolderArtSet()->artifactsInBackpack.size() + GameConstants::BACKPACK_START //artifact moved back to backpack (eg. to make place for art we are moving)
			|| dst.getHolderArtSet()->bearerType() != ArtBearer::HERO);
		commonInfo->reset();
		unmarkSlots();
	}
	else if(commonInfo->dst == src) //the dest artifact was moved -> we are picking it
	{
		assert(dst.slot >= GameConstants::BACKPACK_START);
		commonInfo->reset();

		CArtPlace *ap = NULL;
		BOOST_FOREACH(CArtifactsOfHero *aoh, commonInfo->participants)
		{
			if(dst.isHolder(aoh->curHero))
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
			CCS->curh->dragAndDropCursor(new CAnimImage("artifact", dst.getArt()->artType->iconIndex));
			markPossibleSlots(dst.getArt());
		}
	}
	else if(src.slot >= GameConstants::BACKPACK_START &&
	        src.slot <  commonInfo->src.slotID &&
			src.isHolder(commonInfo->src.AOH->curHero)) //artifact taken from before currently picked one
	{
		//int fixedSlot = src.hero->getArtPos(commonInfo->src.art);
		vstd::advance(commonInfo->src.slotID, -1);
		assert(commonInfo->src.valid());
	}
	else
	{
		//when moving one artifact onto another it leads to two art movements: dst->backapck; src->dst
		// however after first movement we pick the art from backpack and the second movement coming when
		// we have a different artifact may look surprising... but it's valid.
		//tlog1 << "Unexpected artifact movement...\n";
	}

	updateParentWindow();
 	int shift = 0;
// 	if(dst.slot >= Arts::BACKPACK_START && dst.slot - Arts::BACKPACK_START < backpackPos)
// 		shift++;
//
 	if(src.slot < GameConstants::BACKPACK_START  &&  dst.slot - GameConstants::BACKPACK_START < backpackPos)
		shift++;
	if(dst.slot < GameConstants::BACKPACK_START  &&  src.slot - GameConstants::BACKPACK_START < backpackPos)
 		shift--;

	if( (isCurHeroSrc && src.slot >= GameConstants::BACKPACK_START)
	 || (isCurHeroDst && dst.slot >= GameConstants::BACKPACK_START) )
		scrollBackpack(shift); //update backpack slots
}

void CArtifactsOfHero::artifactRemoved(const ArtifactLocation &al)
{
	if(al.isHolder(curHero))
	{
		if(al.slot < GameConstants::BACKPACK_START)
			updateWornSlots(0);
		else
			scrollBackpack(0); //update backpack slots
	}
}

CArtPlace * CArtifactsOfHero::getArtPlace(int slot)
{
	if(slot < GameConstants::BACKPACK_START)
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
	if(al.isHolder(curHero))
		updateWornSlots();
}

void CArtifactsOfHero::artifactDisassembled(const ArtifactLocation &al)
{
	if(al.isHolder(curHero))
		updateWornSlots();
}

void CArtifactsOfHero::updateWornSlots(bool redrawParent /*= true*/)
{
	for(int i = 0; i < artWorn.size(); i++)
		updateSlot(ArtifactPosition(i));


	if(redrawParent)
		updateParentWindow();
}

const CGHeroInstance * CArtifactsOfHero::getHero() const
{
	return curHero;
}

void CArtifactsOfHero::updateSlot(ArtifactPosition slotID)
{
	setSlotData(getArtPlace(slotID), slotID);
}

void CExchangeWindow::questlog(int whichHero)
{
	CCS->curh->dragAndDropCursor(NULL);
}

void CExchangeWindow::prepareBackground()
{
	//printing heroes' names and levels
	auto genTitle = [](const CGHeroInstance *h)
	{
		return boost::str(boost::format(CGI->generaltexth->allTexts[138])
	                      % h->name % h->level % h->type->heroClass->name);
	};

	new CLabel(147, 25, FONT_SMALL, CENTER, Colors::WHITE, genTitle(heroInst[0]));
	new CLabel(653, 25, FONT_SMALL, CENTER, Colors::WHITE, genTitle(heroInst[1]));

	//printing primary skills
	for(int g=0; g<4; ++g)
		new CAnimImage("PSKIL32", g, 0, 385, 19 + 36*g);

	//heroes related thing
	for(int b=0; b<ARRAY_COUNT(heroInst); b++)
	{
		CHeroWithMaybePickedArtifact heroWArt = CHeroWithMaybePickedArtifact(this, heroInst[b]);
		//printing primary skills' amounts
		for(int m=0; m<GameConstants::PRIMARY_SKILLS; ++m)
			new CLabel(352 + 93 * b, 35 + 36 * m, FONT_SMALL, CENTER, Colors::WHITE,
		               boost::lexical_cast<std::string>(heroWArt.getPrimSkillLevel(static_cast<PrimarySkill::PrimarySkill>(m))));

		//printing secondary skills
		for(int m=0; m<heroInst[b]->secSkills.size(); ++m)
		{
			int id = heroInst[b]->secSkills[m].first;
			int level = heroInst[b]->secSkills[m].second;
			new CAnimImage("SECSK32", id*3 + level + 2 , 0, 32 + 36 * m + 454 * b, 88);
		}

		//hero's specialty
		new CAnimImage("UN32", heroInst[b]->type->imageIndex, 0, 67 + 490*b, 45);

		//experience
		new CAnimImage("PSKIL32", 4, 0, 103 + 490*b, 45);
		new CLabel(119 + 490*b, 71, FONT_SMALL, CENTER, Colors::WHITE, makeNumberShort(heroInst[b]->exp));

		//mana points
		new CAnimImage("PSKIL32", 5, 0, 139 + 490*b, 45);
		new CLabel(155 + 490*b, 71, FONT_SMALL, CENTER, Colors::WHITE, makeNumberShort(heroInst[b]->mana));
	}

	//printing portraits
	new CAnimImage("PortraitsLarge", heroInst[0]->portrait, 0, 257, 13);
	new CAnimImage("PortraitsLarge", heroInst[1]->portrait, 0, 485, 13);
}

CExchangeWindow::CExchangeWindow(ObjectInstanceID hero1, ObjectInstanceID hero2):
    CWindowObject(PLAYER_COLORED | BORDERED, "TRADE2")
{
	OBJ_CONSTRUCTION_CAPTURING_ALL;
	char bufor[400];
	heroInst[0] = LOCPLINT->cb->getHero(hero1);
	heroInst[1] = LOCPLINT->cb->getHero(hero2);

	prepareBackground();

	artifs[0] = new CArtifactsOfHero(Point(-334, 150));
	artifs[0]->commonInfo = new CArtifactsOfHero::SCommonPart;
	artifs[0]->commonInfo->participants.insert(artifs[0]);
	artifs[0]->setHero(heroInst[0]);
	artifs[1] = new CArtifactsOfHero(Point(96, 150));
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
		primSkillAreas[g]->pos = genRect(32, 140, pos.x + 329, pos.y + 19 + 36 * g);
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

		portrait[b] = new CHeroArea(257 + 228*b, 13, heroInst[b]);

		specialty[b] = new LRClickableAreaWText();
		specialty[b]->pos = genRect(32, 32, pos.x + 69 + 490*b, pos.y + 45);
		specialty[b]->hoverText = CGI->generaltexth->heroscrn[27];
		specialty[b]->text = heroInst[b]->type->specDescr;

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
		morale[b] = new MoraleLuckBox(true, genRect(32, 32, 176 + 490*b, 39), true);
		morale[b]->set(heroInst[b]);
		//setting luck
		luck[b] = new MoraleLuckBox(false, genRect(32, 32, 212 + 490*b, 39), true);
		luck[b]->set(heroInst[b]);
	}

	//buttons
	quit = new CAdventureMapButton(CGI->generaltexth->zelp[600], boost::bind(&CExchangeWindow::close, this), 732, 567, "IOKAY.DEF", SDLK_RETURN);
	questlogButton[0] = new CAdventureMapButton(CGI->generaltexth->heroscrn[0], "", boost::bind(&CExchangeWindow::questlog,this, 0), 10,  44, "hsbtns4.def");
	questlogButton[1] = new CAdventureMapButton(CGI->generaltexth->heroscrn[0], "", boost::bind(&CExchangeWindow::questlog,this, 1), 740, 44, "hsbtns4.def");

	Rect barRect(5, 578, 725, 18);
	ourBar = new CGStatusBar(new CPicture(*background, barRect, 5, 578, false));

	//garrison interface
	garr = new CGarrisonInt(69, 131, 4, Point(418,0), *background, Point(69,131), heroInst[0],heroInst[1], true, true);
	garr->addSplitBtn(new CAdventureMapButton(CGI->generaltexth->tcommands[3], "", boost::bind(&CGarrisonInt::splitClick, garr),  10, 132, "TSBTNS.DEF"));
	garr->addSplitBtn(new CAdventureMapButton(CGI->generaltexth->tcommands[3], "", boost::bind(&CGarrisonInt::splitClick, garr), 740, 132, "TSBTNS.DEF"));
}

CExchangeWindow::~CExchangeWindow() //d-tor
{
	delete artifs[0]->commonInfo;
	artifs[0]->commonInfo = NULL;
	artifs[1]->commonInfo = NULL;
}

CShipyardWindow::CShipyardWindow(const std::vector<si32> &cost, int state, int boatType, const boost::function<void()> &onBuy):
    CWindowObject(PLAYER_COLORED, "TPSHIP")
{
	OBJ_CONSTRUCTION_CAPTURING_ALL;

	bgWater = new CPicture("TPSHIPBK", 100, 69);

	std::string boatFilenames[3] = {"AB01_", "AB02_", "AB03_"};

	Point waterCenter = Point(bgWater->pos.x+bgWater->pos.w/2, bgWater->pos.y+bgWater->pos.h/2);
	bgShip = new CAnimImage(boatFilenames[boatType], 0, 7, 120, 96, CShowableAnim::USE_RLE);
	bgShip->center(waterCenter);

	// Create resource icons and costs.
	std::string goldValue = boost::lexical_cast<std::string>(cost[Res::GOLD]);
	std::string woodValue = boost::lexical_cast<std::string>(cost[Res::WOOD]);

	goldCost = new CLabel(118, 294, FONT_SMALL, CENTER, Colors::WHITE, goldValue);
	woodCost = new CLabel(212, 294, FONT_SMALL, CENTER, Colors::WHITE, woodValue);

	goldPic = new CAnimImage("RESOURCE", Res::GOLD, 0, 100, 244);
	woodPic = new CAnimImage("RESOURCE", Res::WOOD, 0, 196, 244);

	quit = new CAdventureMapButton(CGI->generaltexth->allTexts[599], "", boost::bind(&CShipyardWindow::close, this), 224, 312, "ICANCEL", SDLK_RETURN);
	build = new CAdventureMapButton(CGI->generaltexth->allTexts[598], "", boost::bind(&CShipyardWindow::close, this), 42, 312, "IBUY30", SDLK_RETURN);
	build->callback += onBuy;

	for(Res::ERes i = Res::WOOD; i <= Res::GOLD; vstd::advance(i, 1))
	{
		if(cost[i] > LOCPLINT->cb->getResourceAmount(i))
		{
			build->block(true);
			break;
		}
	}

	statusBar = new CGStatusBar(new CPicture(*background, Rect(8, pos.h - 26, pos.w - 16, 19), 8, pos.h - 26));

	title =     new CLabel(164, 27,  FONT_BIG,    CENTER, Colors::YELLOW, CGI->generaltexth->jktexts[13]);
	costLabel = new CLabel(164, 220, FONT_MEDIUM, CENTER, Colors::WHITE,   CGI->generaltexth->jktexts[14]);
}

CPuzzleWindow::CPuzzleWindow(const int3 &GrailPos, double discoveredRatio):
    CWindowObject(PLAYER_COLORED | BORDERED, "PUZZLE"),
    grailPos(GrailPos),
    currentAlpha(SDL_ALPHA_OPAQUE)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL;
	CCS->soundh->playSound(soundBase::OBELISK);

	quitb = new CAdventureMapButton(CGI->generaltexth->allTexts[599], "", boost::bind(&CPuzzleWindow::close, this), 670, 538, "IOK6432.DEF", SDLK_RETURN);
	quitb->assignedKeys.insert(SDLK_ESCAPE);
	quitb->borderColor = Colors::METALLIC_GOLD;
	quitb->borderEnabled = true;

	new CPicture("PUZZLOGO", 607, 3);
	new CLabel(700, 95, FONT_BIG, CENTER, Colors::YELLOW, CGI->generaltexth->allTexts[463]);
	new CResDataBar("ZRESBAR.bmp", 3, 575, 32, 2, 85, 85);

	int faction = LOCPLINT->cb->getStartInfo()->playerInfos.find(LOCPLINT->playerID)->second.castle;

	auto & puzzleMap = CGI->townh->factions[faction].puzzleMap;

	for(int g=0; g<puzzleMap.size(); ++g)
	{
		const SPuzzleInfo & info = puzzleMap[g];

		auto piece = new CPicture(info.filename, info.x, info.y);

		//piece that will slowly disappear
		if(info.whenUncovered < GameConstants::PUZZLE_MAP_PIECES * discoveredRatio)
		{
			piecesToRemove.push_back(piece);
			piece->needRefresh = true;
			piece->recActions = piece->recActions & ~SHOWALL;
		}
	}
}

void CPuzzleWindow::showAll(SDL_Surface * to)
{
	int3 moveInt = int3(8, 9, 0);
	Rect mapRect = genRect(544, 591, pos.x + 8, pos.y + 7);

	CGI->mh->terrainRect
		(grailPos - moveInt, adventureInt->anim,
		 &LOCPLINT->cb->getVisibilityMap(), true, adventureInt->heroAnim,
		 to, &mapRect, 0, 0, true, moveInt);

	CWindowObject::showAll(to);
}

void CPuzzleWindow::show(SDL_Surface * to)
{
	static int animSpeed = 2;

	if (currentAlpha < animSpeed)
	{
		//animation done
		BOOST_FOREACH(auto & piece, piecesToRemove)
			delete piece;
		piecesToRemove.clear();
	}
	else
	{
		//update disappearing puzzles
		BOOST_FOREACH(auto & piece, piecesToRemove)
			piece->setAlpha(currentAlpha);
		currentAlpha -= animSpeed;
	}
	CWindowObject::show(to);
}

void CTransformerWindow::CItem::move()
{
	if (left)
		moveBy(Point(289, 0));
	else
		moveBy(Point(-289, 0));
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

void CTransformerWindow::CItem::update()
{
	icon->setFrame(parent->army->getCreature(SlotID(id))->idNumber + 2);
}

CTransformerWindow::CItem::CItem(CTransformerWindow * parent, int size, int id):
	id(id), size(size), parent(parent)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL;
	addUsedEvents(LCLICK);
	left = true;
	pos.w = 58;
	pos.h = 64;

	pos.x += 45  + (id%3)*83 + id/6*83;
	pos.y += 109 + (id/3)*98;
	icon = new CAnimImage("TWCRPORT", parent->army->getCreature(SlotID(id))->idNumber + 2);
	new CLabel(28, 76,FONT_SMALL, CENTER, Colors::WHITE, boost::lexical_cast<std::string>(size));//stack size
}

void CTransformerWindow::makeDeal()
{
	for (int i=0; i<items.size(); i++)
		if (!items[i]->left)
			LOCPLINT->cb->trade(town, EMarketMode::CREATURE_UNDEAD, items[i]->id, 0, 0, hero);
}

void CTransformerWindow::addAll()
{
	for (int i=0; i<items.size(); i++)
		if (items[i]->left)
			items[i]->move();
	showAll(screen2);
}

void CTransformerWindow::updateGarrisons()
{
	BOOST_FOREACH(auto & item, items)
	{
		item->update();
	}
}

CTransformerWindow::CTransformerWindow(const CGHeroInstance * _hero, const CGTownInstance * _town):
    CWindowObject(PLAYER_COLORED, "SKTRNBK"),
    hero(_hero),
    town(_town)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL;
	if (hero)
		army = hero;
	else
		army = town;

	for (int i=0; i<GameConstants::ARMY_SIZE; i++ )
		if ( army->getCreature(SlotID(i)) )
			items.push_back(new CItem(this, army->getStackCount(SlotID(i)), i));

	all    = new CAdventureMapButton(CGI->generaltexth->zelp[590],boost::bind(&CTransformerWindow::addAll,this),     146,416,"ALTARMY.DEF",SDLK_a);
	convert= new CAdventureMapButton(CGI->generaltexth->zelp[591],boost::bind(&CTransformerWindow::makeDeal,this),   269,416,"ALTSACR.DEF",SDLK_RETURN);
	cancel = new CAdventureMapButton(CGI->generaltexth->zelp[592],boost::bind(&CTransformerWindow::close, this),392,416,"ICANCEL.DEF",SDLK_ESCAPE);
	bar    = new CGStatusBar(new CPicture(*background, Rect(8, pos.h - 26, pos.w - 16, 19), 8, pos.h - 26));

	new CLabel(153, 29,FONT_SMALL, CENTER, Colors::YELLOW, CGI->generaltexth->allTexts[485]);//holding area
	new CLabel(153+295, 29, FONT_SMALL, CENTER, Colors::YELLOW, CGI->generaltexth->allTexts[486]);//transformer
	new CTextBox(CGI->generaltexth->allTexts[487], Rect(26,  56, 255, 40), 0, FONT_MEDIUM, CENTER, Colors::YELLOW);//move creatures to create skeletons
	new CTextBox(CGI->generaltexth->allTexts[488], Rect(320, 56, 255, 40), 0, FONT_MEDIUM, CENTER, Colors::YELLOW);//creatures here will become skeletons

}

void CUniversityWindow::CItem::clickLeft(tribool down, bool previousState)
{
	if(previousState && (!down))
	{
		if ( state() != 2 )
			return;
		CUnivConfirmWindow *win = new CUnivConfirmWindow(parent, ID, LOCPLINT->cb->getResourceAmount(Res::GOLD) >= 2000);
		GH.pushInt(win);
	}
}

void CUniversityWindow::CItem::clickRight(tribool down, bool previousState)
{
	if(down)
	{
		CRClickPopup::createAndPush(CGI->generaltexth->skillInfoTexts[ID][0],
		        new CComponent(CComponent::secskill, ID, 1));
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
	if (parent->hero->getSecSkillLevel(SecondarySkill(ID)))//hero know this skill
		return 1;
	if (!parent->hero->canLearnSkill())//can't learn more skills
		return 0;
	if (parent->hero->type->heroClass->secSkillProbability[ID]==0)//can't learn this skill (like necromancy for most of non-necros)
		return 0;
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
	printAtMiddleLoc  (CGI->generaltexth->skillName[ID], 22, -13, FONT_SMALL, Colors::WHITE,to);//Name
	printAtMiddleLoc  (CGI->generaltexth->levels[0], 22, 57, FONT_SMALL, Colors::WHITE,to);//Level(always basic)

	CAnimImage::showAll(to);
}

CUniversityWindow::CItem::CItem(CUniversityWindow * _parent, int _ID, int X, int Y):
	CAnimImage ("SECSKILL", _ID*3+3, 0, X, Y),
	ID(_ID),
	parent(_parent)
{
	addUsedEvents(LCLICK | RCLICK | HOVER);
}

CUniversityWindow::CUniversityWindow(const CGHeroInstance * _hero, const IMarket * _market):
    CWindowObject(PLAYER_COLORED, "UNIVERS1"),
    hero(_hero),
    market(_market)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL;

	green  = new CPicture("UNIVGREN.PCX");
	yellow = new CPicture("UNIVGOLD.PCX");//bars
	red    = new CPicture("UNIVRED.PCX");

	green->recActions  =
	yellow->recActions =
	red->recActions    = DISPOSE;

	CIntObject * titlePic = nullptr;

	if (market->o->ID == Obj::TOWN)
		titlePic = new CAnimImage(CGI->townh->towns[ETownType::CONFLUX].clientInfo.buildingsIcons, BuildingID::MAGIC_UNIVERSITY);
	else
		titlePic = new CPicture("UNIVBLDG");

	titlePic->center(Point(232 + pos.x, 76 + pos.y));

	//Clerk speech
	new CTextBox(CGI->generaltexth->allTexts[603], Rect(24, 129, 413, 70), 0, FONT_SMALL, CENTER, Colors::WHITE);

	//University
	new CLabel(231, 26, FONT_MEDIUM, CENTER, Colors::YELLOW, CGI->generaltexth->allTexts[602]);

	std::vector<int> list = market->availableItemsIds(EMarketMode::RESOURCE_SKILL);

	assert(list.size() == 4);

	for (int i=0; i<list.size(); i++)//prepare clickable items
		items.push_back(new CItem(this, list[i], 54+i*104, 234));

	cancel = new CAdventureMapButton(CGI->generaltexth->zelp[632],
		boost::bind(&CUniversityWindow::close, this),200,313,"IOKAY.DEF",SDLK_RETURN);

	bar = new CGStatusBar(new CPicture(*background, Rect(8, pos.h - 26, pos.w - 16, 19), 8, pos.h - 26));
}

CUnivConfirmWindow::CUnivConfirmWindow(CUniversityWindow * PARENT, int SKILL, bool available ):
    CWindowObject(PLAYER_COLORED, "UNIVERS2.PCX"),
    parent(PARENT)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL;

	std::string text = CGI->generaltexth->allTexts[608];
	boost::replace_first(text, "%s", CGI->generaltexth->levels[0]);
	boost::replace_first(text, "%s", CGI->generaltexth->skillName[SKILL]);
	boost::replace_first(text, "%d", "2000");

	new CTextBox(text, Rect(24, 129, 413, 70), 0, FONT_SMALL, CENTER, Colors::WHITE);//Clerk speech

	new CLabel(230, 37,  FONT_SMALL, CENTER, Colors::WHITE, CGI->generaltexth-> skillName[SKILL]);//Skill name
	new CAnimImage("SECSKILL", SKILL*3+3, 0, 211, 51);//skill
	new CLabel(230, 107, FONT_SMALL, CENTER, Colors::WHITE, CGI->generaltexth->levels[1]);//Skill level

	new CAnimImage("RESOURCE", Res::GOLD, 0, 210, 210);//gold
	new CLabel(230, 267, FONT_SMALL, CENTER, Colors::WHITE, "2000");//Cost

	std::string hoverText = CGI->generaltexth->allTexts[609];
	boost::replace_first(hoverText, "%s", CGI->generaltexth->levels[0]+ " " + CGI->generaltexth->skillName[SKILL]);

	text = CGI->generaltexth->zelp[633].second;
	boost::replace_first(text, "%s", CGI->generaltexth->levels[0]);
	boost::replace_first(text, "%s", CGI->generaltexth->skillName[SKILL]);
	boost::replace_first(text, "%d", "2000");

	confirm= new CAdventureMapButton(hoverText, text, boost::bind(&CUnivConfirmWindow::makeDeal, this, SKILL),
	         148,299,"IBY6432.DEF",SDLK_RETURN);
	confirm->block(!available);

	cancel = new CAdventureMapButton(CGI->generaltexth->zelp[631],boost::bind(&CUnivConfirmWindow::close, this),
	         252,299,"ICANCEL.DEF",SDLK_ESCAPE);
	bar = new CGStatusBar(new CPicture(*background, Rect(8, pos.h - 26, pos.w - 16, 19), 8, pos.h - 26));
}

void CUnivConfirmWindow::makeDeal(int skill)
{
	LOCPLINT->cb->trade(parent->market->o, EMarketMode::RESOURCE_SKILL, 6, skill, 1, parent->hero);
	close();
}

CHillFortWindow::CHillFortWindow(const CGHeroInstance *visitor, const CGObjectInstance *object):
    CWindowObject(PLAYER_COLORED, "APHLFTBK"),
	fort(object),
    hero(visitor)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL;

	slotsCount=7;
	resources =  CDefHandler::giveDefEss("SMALRES.DEF");

	new CLabel(325, 32, FONT_BIG, CENTER, Colors::YELLOW, fort->hoverName);//Hill Fort

	heroPic = new CHeroArea(30, 60, hero);

	currState.resize(slotsCount+1);
	costs.resize(slotsCount);
	totalSumm.resize(GameConstants::RESOURCE_QUANTITY);
	std::vector<std::string> files;
	files += "APHLF1R.DEF", "APHLF1Y.DEF", "APHLF1G.DEF";
	for (int i=0; i<slotsCount; i++)
	{
		currState[i] = getState(SlotID(i));
		upgrade[i] = new CAdventureMapButton(getTextForSlot(SlotID(i)),"",boost::bind(&CHillFortWindow::makeDeal, this, SlotID(i)),
		                                    107+i*76, 171, "", SDLK_1+i, &files);
		upgrade[i]->block(currState[i] == -1);
	}
	files.clear();
	files += "APHLF4R.DEF", "APHLF4Y.DEF", "APHLF4G.DEF";
	currState[slotsCount] = getState(SlotID(slotsCount));
	upgradeAll = new CAdventureMapButton(CGI->generaltexth->allTexts[432],"",boost::bind(&CHillFortWindow::makeDeal, this, SlotID(slotsCount)),
	                                    30, 231, "", SDLK_0, &files);
	quit = new CAdventureMapButton("","",boost::bind(&CHillFortWindow::close, this), 294, 275, "IOKAY.DEF", SDLK_RETURN);
	bar = new CGStatusBar(new CPicture(*background, Rect(8, pos.h - 26, pos.w - 16, 19), 8, pos.h - 26));

	garr = new CGarrisonInt(108, 60, 18, Point(),background->bg,Point(108,60),hero,NULL);
	updateGarrisons();
}

void CHillFortWindow::updateGarrisons()
{
	for (int i=0; i<GameConstants::RESOURCE_QUANTITY; i++)
		totalSumm[i]=0;

	for (int i=0; i<slotsCount; i++)
	{
		costs[i].clear();
		int newState = getState(SlotID(i));
		if (newState != -1)
		{
			UpgradeInfo info;
			LOCPLINT->cb->getUpgradeInfo(hero, SlotID(i), info);
			if (info.newID.size())//we have upgrades here - update costs
			{
				costs[i] = info.cost[0] * hero->getStackCount(SlotID(i));
				totalSumm += costs[i];
			}
		}

		currState[i] = newState;
		upgrade[i]->setIndex(newState);
		upgrade[i]->block(currState[i] == -1);
		upgrade[i]->hoverTexts[0] = getTextForSlot(SlotID(i));
	}

	int newState = getState(SlotID(slotsCount));
	currState[slotsCount] = newState;
	upgradeAll->setIndex(newState);
	garr->recreateSlots();
}

void CHillFortWindow::makeDeal(SlotID slot)
{
	assert(slot.getNum()>=0);
	int offset = (slot.getNum() == slotsCount)?2:0;
	switch (currState[slot.getNum()])
	{
		case 0:
			LOCPLINT->showInfoDialog(CGI->generaltexth->allTexts[314 + offset],
			          std::vector<CComponent*>(), soundBase::sound_todo);
			break;
		case 1:
			LOCPLINT->showInfoDialog(CGI->generaltexth->allTexts[313 + offset],
			          std::vector<CComponent*>(), soundBase::sound_todo);
			break;
		case 2:
			for (int i=0; i<slotsCount; i++)
				if ( slot.getNum() ==i || ( slot.getNum() == slotsCount && currState[i] == 2 ) )//this is activated slot or "upgrade all"
				{
					UpgradeInfo info;
					LOCPLINT->cb->getUpgradeInfo(hero, SlotID(i), info);
					LOCPLINT->cb->upgradeCreature(hero, SlotID(i), info.newID[0]);
				}
			break;

	}
}

void CHillFortWindow::showAll (SDL_Surface *to)
{
	CWindowObject::showAll(to);

	for ( int i=0; i<slotsCount; i++)
	{
		if ( currState[i] == 0 || currState[i] == 2 )
		{
			if ( costs[i].size() )//we have several elements
			{
				int curY = 128;//reverse iterator is used to display gold as first element
				for(int j = costs[i].size()-1; j >= 0; j--)
				{
					int val = costs[i][j];
					if(!val) continue;

					blitAtLoc(resources->ourImages[j].bitmap, 104+76*i, curY, to);
					printToLoc(boost::lexical_cast<std::string>(val), 168+76*i, curY+16, FONT_SMALL, Colors::WHITE, to);
					curY += 20;
				}
			}
			else//free upgrade - print gold image and "Free" text
			{
				blitAtLoc(resources->ourImages[6].bitmap, 104+76*i, 128, to);
				printToLoc(CGI->generaltexth->allTexts[344], 168+76*i, 144, FONT_SMALL, Colors::WHITE, to);
			}
		}
	}
	for (int i=0; i<GameConstants::RESOURCE_QUANTITY; i++)
	{
		if (totalSumm[i])//this resource is used - display it
		{
			blitAtLoc(resources->ourImages[i].bitmap, 104+76*i, 237, to);
			printToLoc(boost::lexical_cast<std::string>(totalSumm[i]), 166+76*i, 253, FONT_SMALL, Colors::WHITE, to);
		}
	}
}

std::string CHillFortWindow::getTextForSlot(SlotID slot)
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

int CHillFortWindow::getState(SlotID slot)
{
	TResources myRes = LOCPLINT->cb->getResourceAmount();
	if ( slot.getNum() == slotsCount )//"Upgrade all" slot
	{
		bool allUpgraded = true;//All creatures are upgraded?
		for (int i=0; i<slotsCount; i++)
			allUpgraded &=  currState[i] == 1 || currState[i] == -1;

		if (allUpgraded)
			return 1;

		if(!totalSumm.canBeAfforded(myRes))
			return 0;

		return 2;
	}

	if (hero->slotEmpty(slot))//no creature here
		return -1;

	UpgradeInfo info;
	LOCPLINT->cb->getUpgradeInfo(hero, slot, info);
	if (!info.newID.size())//already upgraded
		return 1;

	if(!(info.cost[0] * hero->getStackCount(slot)).canBeAfforded(myRes))
			return 0;

	return 2;//can upgrade
}

CThievesGuildWindow::CThievesGuildWindow(const CGObjectInstance * _owner):
    CWindowObject(PLAYER_COLORED | BORDERED, "TpRank"),
	owner(_owner)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL;
	type |= BLOCK_ADV_HOTKEYS;

	SThievesGuildInfo tgi; //info to be displayed
	LOCPLINT->cb->getThievesGuildInfo(tgi, owner);

	exitb = new CAdventureMapButton (CGI->generaltexth->allTexts[600], "", boost::bind(&CThievesGuildWindow::close,this), 748, 556, "TPMAGE1", SDLK_RETURN);
	exitb->assignedKeys.insert(SDLK_ESCAPE);
	statusBar = new CGStatusBar(3, 555, "TStatBar.bmp", 742);

	resdatabar = new CMinorResDataBar();
	resdatabar->pos.x += pos.x;
	resdatabar->pos.y += pos.y;

	//data for information table:
	// fields[row][column] = list of id's of players for this box
	static std::vector< std::vector< PlayerColor > > SThievesGuildInfo::* fields[] =
		{ &SThievesGuildInfo::numOfTowns, &SThievesGuildInfo::numOfHeroes,       &SThievesGuildInfo::gold,
		  &SThievesGuildInfo::woodOre,    &SThievesGuildInfo::mercSulfCrystGems, &SThievesGuildInfo::obelisks,
		  &SThievesGuildInfo::artifacts,  &SThievesGuildInfo::army,              &SThievesGuildInfo::income };

	//printing texts & descriptions to background

	for(int g=0; g<12; ++g)
	{
		int posY[] = {400, 460, 510};
		int y;
		if(g < 9)
			y = 52 + 32*g;
		else
			y = posY[g-9];

		std::string text = CGI->generaltexth->jktexts[24+g];
		boost::algorithm::trim_if(text,boost::algorithm::is_any_of("\""));
		new CLabel(135, y, FONT_MEDIUM, CENTER, Colors::YELLOW, text);
	}

	for(int g=1; g<tgi.playerColors.size(); ++g)
		new CAnimImage("PRSTRIPS", g-1, 0, 250 + 66*g, 7);

	for(int g=0; g<tgi.playerColors.size(); ++g)
		new CLabel(283 + 66*g, 24, FONT_BIG, CENTER, Colors::YELLOW, CGI->generaltexth->jktexts[16+g]);

	//printing flags
	for(int g = 0; g < ARRAY_COUNT(fields); ++g) //by lines
	{
		for(int b=0; b<(tgi .* fields[g]).size(); ++b) //by places (1st, 2nd, ...)
		{
			std::vector<PlayerColor> &players = (tgi .* fields[g])[b]; //get players with this place in this line

			//position of box
			int xpos = 259 + 66 * b;
			int ypos = 41 +  32 * g;

			size_t rowLength[2]; //size of each row
			rowLength[0] = std::min<size_t>(players.size(), 4);
			rowLength[1] = players.size() - rowLength[0];

			for (size_t j=0; j< 2; j++)
			{
				// origin of this row | offset for 2nd row| shift right for short rows
				//if we have 2 rows, start either from mid or beginning (depending on count), otherwise center the flags
				int rowStartX = xpos   +   (j ? 6 + (rowLength[j] < 3 ? 12 : 0) : 24 - 6 * rowLength[j]);
				int rowStartY = ypos   +   (j ? 4 : 0);

				for (size_t i=0; i< rowLength[j]; i++)
				{
					new CAnimImage("itgflags", players[i + j*4].getNum(), 0, rowStartX + i*12, rowStartY);
				}
			}
		}
	}

	static const std::string colorToBox[] = {"PRRED.BMP", "PRBLUE.BMP", "PRTAN.BMP", "PRGREEN.BMP", "PRORANGE.BMP", "PRPURPLE.BMP", "PRTEAL.BMP", "PRROSE.bmp"};

	//printing best hero
	int counter = 0;
	BOOST_FOREACH(auto & iter, tgi.colorToBestHero)
	{
		if(iter.second.portrait >= 0)
		{
			new CPicture(colorToBox[iter.first.getNum()], 253 + 66 * counter, 334);
			new CAnimImage("PortraitsSmall", iter.second.portrait, 0, 260 + 66 * counter, 360);
			//TODO: r-click info:
			// - r-click on hero
			// - r-click on primary skill label
			if(iter.second.details)
			{
				new CTextBox(CGI->generaltexth->allTexts[184], Rect(260 + 66*counter, 396, 52, 64),
				             0, FONT_TINY, TOPLEFT, Colors::WHITE);
				for (int i=0; i<iter.second.details->primskills.size(); ++i)
				{
					new CLabel(310 + 66 * counter, 407 + 11*i, FONT_TINY, BOTTOMRIGHT, Colors::WHITE,
					           boost::lexical_cast<std::string>(iter.second.details->primskills[i]));
				}
			}
		}
		counter++;
	}

	//printing best creature
	counter = 0;
	BOOST_FOREACH(auto & it, tgi.bestCreature)
	{
		if(it.second >= 0)
			new CAnimImage("TWCRPORT", it.second+2, 0, 255 + 66 * counter, 479);
		counter++;
	}

	//printing personality
	counter = 0;
	BOOST_FOREACH(auto & it, tgi.personality)
	{
		std::string text;
        if(it.second == EAiTactic::NONE)
        {
			text = CGI->generaltexth->arraytxt[172];
        }
        else if(it.second != EAiTactic::RANDOM)
        {
            text = CGI->generaltexth->arraytxt[168 + it.second];
        }

		new CLabel(283 + 66*counter, 459, FONT_SMALL, CENTER, Colors::WHITE, text);

		counter++;
	}
}

void MoraleLuckBox::set(const IBonusBearer *node)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL;
	const int textId[] = {62, 88}; //eg %s \n\n\n {Current Luck Modifiers:}
	const int noneTxtId = 108; //Russian version uses same text for neutral morale\luck
	const int neutralDescr[] = {60, 86}; //eg {Neutral Morale} \n\n Neutral morale means your armies will neither be blessed with extra attacks or freeze in combat.
	const int componentType[] = {CComponent::luck, CComponent::morale};
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

	std::string imageName;
	if (small)
		imageName = morale ? "IMRL30": "ILCK30";
	else
		imageName = morale ? "IMRL42" : "ILCK42";

	delete image;
	image = new CAnimImage(imageName, bonusValue + 3);
	image->moveBy(Point(pos.w/2 - image->pos.w/2, pos.h/2 - image->pos.h/2));//center icon
}

MoraleLuckBox::MoraleLuckBox(bool Morale, const Rect &r, bool Small):
	image(NULL),
	morale(Morale),
	small(Small)
{
	bonusValue = 0;
	pos = r + pos;
}

CArtifactHolder::CArtifactHolder()
{
}

void CWindowWithArtifacts::artifactRemoved(const ArtifactLocation &artLoc)
{
	BOOST_FOREACH(CArtifactsOfHero *aoh, artSets)
		aoh->artifactRemoved(artLoc);
}

void CWindowWithArtifacts::artifactMoved(const ArtifactLocation &artLoc, const ArtifactLocation &destLoc)
{
	CArtifactsOfHero *destaoh = NULL;
	BOOST_FOREACH(CArtifactsOfHero *aoh, artSets)
	{
		aoh->artifactMoved(artLoc, destLoc);
		aoh->redraw();
		if(destLoc.isHolder(aoh->getHero()))
			destaoh = aoh;
	}

	//Make sure the status bar is updated so it does not display old text
	if(destaoh != NULL && destaoh->getArtPlace(destLoc.slot) != NULL)
	{
		destaoh->getArtPlace(destLoc.slot)->hover(true);
	}
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
	slotID = ArtifactPosition::PRE_FIRST;
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
	bool ret = al.isHolder(AOH->curHero)  &&  al.slot == slotID;

	//assert(al.getArt() == art);
	return ret;
}

bool CArtifactsOfHero::SCommonPart::Artpos::valid()
{
	assert(AOH && art);
	return art == AOH->curHero->getArt(slotID);
}

void CRClickPopup::clickRight(tribool down, bool previousState)
{
	if(down)
		return;
	close();
}

void CRClickPopup::close()
{
	GH.popIntTotally(this);
}

void CRClickPopup::createAndPush(const std::string &txt, const CInfoWindow::TCompsInfo &comps)
{
	PlayerColor player = LOCPLINT ? LOCPLINT->playerID : PlayerColor(1); //if no player, then use blue

	CSimpleWindow * temp = new CInfoWindow(txt, player, comps);
	temp->center(Point(GH.current->motion)); //center on mouse
	temp->fitToScreen(10);
	CRClickPopupInt *rcpi = new CRClickPopupInt(temp,true);
	GH.pushInt(rcpi);
}

void CRClickPopup::createAndPush(const std::string &txt, CComponent * component)
{
	CInfoWindow::TCompsInfo intComps;
	intComps.push_back(component);

	createAndPush(txt, intComps);
}

Point CInfoBoxPopup::toScreen(Point p)
{
	vstd::abetween(p.x, adventureInt->terrain.pos.x + 100, adventureInt->terrain.pos.x + adventureInt->terrain.pos.w - 100);
	vstd::abetween(p.y, adventureInt->terrain.pos.y + 100, adventureInt->terrain.pos.y + adventureInt->terrain.pos.h - 100);

	return p;
}

CInfoBoxPopup::CInfoBoxPopup(Point position, const CGTownInstance * town):
    CWindowObject(RCLICK_POPUP | PLAYER_COLORED, "TOWNQVBK", toScreen(position))
{
	InfoAboutTown iah;
	LOCPLINT->cb->getTownInfo(town, iah);

	OBJ_CONSTRUCTION_CAPTURING_ALL;
	new CTownTooltip(Point(9, 10), iah);
}

CInfoBoxPopup::CInfoBoxPopup(Point position, const CGHeroInstance * hero):
    CWindowObject(RCLICK_POPUP | PLAYER_COLORED, "HEROQVBK", toScreen(position))
{
	InfoAboutHero iah;
	LOCPLINT->cb->getHeroInfo(hero, iah);

	OBJ_CONSTRUCTION_CAPTURING_ALL;
	new CHeroTooltip(Point(9, 10), iah);
}

CInfoBoxPopup::CInfoBoxPopup(Point position, const CGGarrison * garr):
    CWindowObject(RCLICK_POPUP | PLAYER_COLORED, "TOWNQVBK", toScreen(position))
{
	InfoAboutTown iah;
	LOCPLINT->cb->getTownInfo(garr, iah);

	OBJ_CONSTRUCTION_CAPTURING_ALL;
	new CArmyTooltip(Point(9, 10), iah);
}

CIntObject * CRClickPopup::createInfoWin(Point position, const CGObjectInstance * specific) //specific=0 => draws info about selected town/hero
{
	if(!specific)
		specific = adventureInt->selection;

	assert(specific);

	switch(specific->ID)
	{
	case Obj::HERO:
		return new CInfoBoxPopup(position, dynamic_cast<const CGHeroInstance *>(specific));
	case Obj::TOWN:
		return new CInfoBoxPopup(position, dynamic_cast<const CGTownInstance *>(specific));
	case Obj::GARRISON:
	case Obj::GARRISON2:
		return new CInfoBoxPopup(position, dynamic_cast<const CGGarrison *>(specific));
	default:
		return NULL;
	}
}

void CRClickPopup::createAndPush(const CGObjectInstance *obj, const Point &p, EAlignment alignment /*= BOTTOMRIGHT*/)
{
	CIntObject *iWin = createInfoWin(p, obj); //try get custom infowindow for this obj
	if(iWin)
		GH.pushInt(iWin);
	else
		CRClickPopup::createAndPush(obj->getHoverText());
}

CRClickPopup::CRClickPopup()
{
	addUsedEvents(RCLICK);
}

CRClickPopup::~CRClickPopup()
{
}

void CRClickPopupInt::show(SDL_Surface * to)
{
	inner->show(to);
}

CRClickPopupInt::CRClickPopupInt( IShowActivatable *our, bool deleteInt )
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
