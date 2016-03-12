#include "StdInc.h"
#include "CGarrisonInt.h"

#include "../gui/CGuiHandler.h"

#include "../CGameInfo.h"
#include "../CPlayerInterface.h"
#include "../widgets/Buttons.h"
#include "../widgets/TextControls.h"
#include "../windows/CCreatureWindow.h"
#include "../windows/GUIClasses.h"

#include "../../CCallback.h"

#include "../../lib/CGeneralTextHandler.h"
#include "../../lib/CCreatureHandler.h"
#include "../../lib/mapObjects/CGHeroInstance.h"

#include "../../lib/CGameState.h"

/*
 * CGarrisonInt.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

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
					logGlobal->warnStream() << "Warning - shouldn't be - highlighted void slot "<<owner->getSelection();
					logGlobal->warnStream() << "Highlighted set to nullptr";
					owner->selectSlot(nullptr);
				}
			}
			else
			{
				if(upg == EGarrisonType::UP)
				{
					temp = CGI->generaltexth->tcommands[12]; //Select %s (in garrison)
				}
				else if(owner->armedObjs[0] && (owner->armedObjs[0]->ID == Obj::TOWN || owner->armedObjs[0]->ID == Obj::HERO))
				{
					temp = CGI->generaltexth->tcommands[32]; //Select %s (visiting)
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
		GH.statusbar->setText(temp);
	}
	else
	{
		GH.statusbar->clear();
	}
}

const CArmedInstance * CGarrisonSlot::getObj() const
{
	return 	owner->armedObjs[upg];
}

/// @return Whether the unit in the slot belongs to the current player.
bool CGarrisonSlot::our() const
{
	return owner->owned[upg];
}

/// @return Whether the unit in the slot belongs to an ally but not to the current player.
bool CGarrisonSlot::ally() const
{
	if(!getObj())
		return false;

	return PlayerRelations::ALLIES == LOCPLINT->cb->getPlayerRelations(LOCPLINT->playerID, getObj()->tempOwner);
}

/// The creature slot has been clicked twice, therefore the creature info should be shown
/// @return Whether the view should be refreshed
bool CGarrisonSlot::viewInfo()
{
	UpgradeInfo pom;
	LOCPLINT->cb->getUpgradeInfo(getObj(), ID, pom);

	bool canUpgrade = getObj()->tempOwner == LOCPLINT->playerID && pom.oldID>=0; //upgrade is possible
	bool canDismiss = getObj()->tempOwner == LOCPLINT->playerID && (getObj()->stacksCount()>1  || !getObj()->needsLastStack());
	std::function<void(CreatureID)> upgr = nullptr;
	std::function<void()> dism = nullptr;
	if(canUpgrade) upgr = [=] (CreatureID newID) { LOCPLINT->cb->upgradeCreature(getObj(), ID, newID); };
	if(canDismiss) dism = [=] { LOCPLINT->cb->dismissCreature(getObj(), ID); };

	owner->selectSlot(nullptr);
	owner->setSplittingMode(false);

	for(auto & elem : owner->splitButtons)
		elem->block(true);

	redraw();
	GH.pushInt(new CStackWindow(myStack, dism, pom, upgr));
	return true;
}

/// The selection is empty, therefore the creature should be moved
/// @return Whether the view should be refreshed
bool CGarrisonSlot::highlightOrDropArtifact()
{
	bool artSelected = false;
	if (CWindowWithArtifacts* chw = dynamic_cast<CWindowWithArtifacts*>(GH.topInt())) //dirty solution
	{
		const CArtifactsOfHero::SCommonPart *commonInfo = chw->artSets.front()->commonInfo;
		if (const CArtifactInstance *art = commonInfo->src.art)
		{
			const CGHeroInstance *srcHero = commonInfo->src.AOH->getHero();
			artSelected = true;
			if (myStack) // try dropping the artifact only if the slot isn't empty
			{
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
	}
	if (!artSelected && creature)
	{
		owner->selectSlot(this);
		if(creature)
		{
			for(auto & elem : owner->splitButtons)
				elem->block(!our());
		}
	}
	redraw();
	return true;
}

/// The creature is only being partially moved
/// @return Whether the view should be refreshed
bool CGarrisonSlot::split()
{
	const CGarrisonSlot * selection = owner->getSelection();
	owner->p2 = ID;   // store the second stack pos
	owner->pb = upg;  // store the second stack owner (up or down army)
	owner->setSplittingMode(false);

	int minLeft=0, minRight=0;

	if(upg != selection->upg) // not splitting within same army
	{
		if(selection->getObj()->stacksCount() == 1 // we're splitting away the last stack
			&& selection->getObj()->needsLastStack() )
		{
			minLeft = 1;
		}
		// destination army can't be emptied, unless we're rebalancing two stacks of same creature
		if(getObj()->stacksCount() == 1
			&& selection->creature == creature
			&& getObj()->needsLastStack() )
		{
			minRight = 1;
		}
	}

	int countLeft = selection->myStack ? selection->myStack->count : 0;
	int countRight = myStack ? myStack->count : 0;

	GH.pushInt(new CSplitWindow(selection->creature, std::bind(&CGarrisonInt::splitStacks, owner, _1, _2),
	                            minLeft, minRight, countLeft, countRight));
	return true;
}

/// If certain creates cannot be moved, the selection should change
/// Force reselection in these cases
///     * When attempting to take creatures from ally
///     * When attempting to swap creatures with an ally
///     * When attempting to take unremovable units
/// @return Whether reselection must be done
bool CGarrisonSlot::mustForceReselection() const
{
	const CGarrisonSlot * selection = owner->getSelection();
	bool withAlly = selection->our() ^ our();
	if (!creature || !selection->creature)
		return false;
	// Attempt to take creatures from ally (select theirs first)
	if (!selection->our())
		return true;
	// Attempt to swap creatures with ally (select ours first)
	if (selection->creature != creature && withAlly)
		return true;
	if (!owner->removableUnits)
	{
		if (selection->upg == EGarrisonType::UP)
			return true;
		else
			return creature || upg == EGarrisonType::UP;
	}
	return false;
}

void CGarrisonSlot::clickRight(tribool down, bool previousState)
{
	if(down && creature)
	{
		GH.pushInt(new CStackWindow(myStack, true));
	}
}

void CGarrisonSlot::clickLeft(tribool down, bool previousState)
{
	if(down)
	{
		bool refr = false;
		const CGarrisonSlot * selection = owner->getSelection();
		if(!selection)
			refr = highlightOrDropArtifact();
		else if(selection == this)
			refr = viewInfo();
		// Re-highlight if troops aren't removable or not ours.
		else if (mustForceReselection())
		{
			if(creature)
				owner->selectSlot(this);
			redraw();
			refr = true;
		}
		else
		{
			const CArmedInstance * selectedObj = owner->armedObjs[selection->upg];
			bool lastHeroStackSelected = false;
			if(selectedObj->stacksCount() == 1
				&& owner->getSelection()->upg != upg
				&& dynamic_cast<const CGHeroInstance*>(selectedObj))
			{
				lastHeroStackSelected = true;
			}

			if((owner->getSplittingMode() || LOCPLINT->shiftPressed()) // split window
				&& (!creature || creature == selection->creature))
			{
				refr = split();
			}
			else if(!creature && lastHeroStackSelected) // split all except last creature
				LOCPLINT->cb->splitStack(selectedObj, owner->armedObjs[upg], selection->ID, ID, selection->myStack->count - 1);
			else if(creature != selection->creature) // swap
				LOCPLINT->cb->swapCreatures(owner->armedObjs[upg], selectedObj, ID, selection->ID);
			else if(lastHeroStackSelected) // merge last stack to other hero stack
				refr = split();
			else // merge
				LOCPLINT->cb->mergeStacks(selectedObj, owner->armedObjs[upg], selection->ID, ID);
		}
		if(refr)
		{
			// Refresh Statusbar
			hover(false);
			hover(true);
		}
	}
}

void CGarrisonSlot::update()
{
	if (getObj() != nullptr)
	{
		addUsedEvents(LCLICK | RCLICK | HOVER);
		myStack = getObj()->getStackPtr(ID);
		creature = myStack ? myStack->type : nullptr;
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
		stackCount->setText(boost::lexical_cast<std::string>(myStack->count));
	}
	else
	{
		creatureImage->disable();
		stackCount->disable();
	}
}

CGarrisonSlot::CGarrisonSlot(CGarrisonInt *Owner, int x, int y, SlotID IID, CGarrisonSlot::EGarrisonType Upg, const CStackInstance * Creature):
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
		stackCount->setText(boost::lexical_cast<std::string>(myStack->count));
}

void CGarrisonInt::addSplitBtn(CButton * button)
{
	addChild(button);
	button->recActions = defActions;
	splitButtons.push_back(button);
	button->block(getSelection() == nullptr);
}

void CGarrisonInt::createSet(std::vector<CGarrisonSlot*> &ret, const CCreatureSet * set, int posX, int posY, int distance, CGarrisonSlot::EGarrisonType Upg )
{
	ret.resize(7);

	if (set)
	{
		for(auto & elem : set->Slots())
		{
			ret[elem.first.getNum()] = new CGarrisonSlot(this, posX + (elem.first.getNum()*distance), posY, elem.first, Upg, elem.second);
		}
	}

	for(int i=0; i<ret.size(); i++)
		if(!ret[i])
			ret[i] = new CGarrisonSlot(this, posX + (i*distance), posY, SlotID(i), Upg, nullptr);

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

	createSet(slotsUp, armedObjs[0], 0, 0, width+interx, CGarrisonSlot::EGarrisonType::UP);
	createSet(slotsDown, armedObjs[1], garOffset.x, garOffset.y, width+interx, CGarrisonSlot::EGarrisonType::DOWN);
}

void CGarrisonInt::recreateSlots()
{
	selectSlot(nullptr);
	setSplittingMode(false);

	for(auto & elem : splitButtons)
		elem->block(true);


	for(CGarrisonSlot * slot : slotsUp)
		slot->update();

	for(CGarrisonSlot * slot : slotsDown)
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
		for (auto button : splitButtons)
			button->block(highlighted == nullptr || !slot->our());

		if (highlighted)
			highlighted->setHighlight(true);
	}
}

void CGarrisonInt::setSplittingMode(bool on)
{
	assert(on == false || highlighted != nullptr); //can't be in splitting mode without selection

	if (inSplittingMode || on)
	{
		for(CGarrisonSlot * slot : slotsUp)
			slot->setHighlight( ( on && (slot->our() || slot->ally()) && (slot->creature == nullptr || slot->creature == getSelection()->creature)));

		for(CGarrisonSlot * slot : slotsDown)
			slot->setHighlight( ( on && (slot->our() || slot->ally()) && (slot->creature == nullptr || slot->creature == getSelection()->creature)));
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

CGarrisonWindow::CGarrisonWindow( const CArmedInstance *up, const CGHeroInstance *down, bool removableUnits ):
	CWindowObject(PLAYER_COLORED, "GARRISON")
{
	OBJ_CONSTRUCTION_CAPTURING_ALL;

	garr = new CGarrisonInt(92, 127, 4, Point(0,96), background->bg, Point(93,127), up, down, removableUnits);
	{
		CButton *split = new CButton(Point(88, 314), "IDV6432.DEF", CButton::tooltip(CGI->generaltexth->tcommands[3], ""), [&]{ garr->splitClick(); } );
		removeChild(split);
		garr->addSplitBtn(split);
	}
	quit = new CButton(Point(399, 314), "IOK6432.DEF", CButton::tooltip(CGI->generaltexth->tcommands[8], ""), [&]{ close(); }, SDLK_RETURN);

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

CGarrisonHolder::CGarrisonHolder()
{
}

void CWindowWithGarrison::updateGarrisons()
{
	garr->recreateSlots();
}
