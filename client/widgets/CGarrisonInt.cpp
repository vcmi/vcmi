/*
 * CGarrisonInt.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "CGarrisonInt.h"

#include "Buttons.h"
#include "TextControls.h"
#include "RadialMenu.h"

#include "../gui/CGuiHandler.h"
#include "../gui/WindowHandler.h"
#include "../render/IImage.h"
#include "../windows/CCreatureWindow.h"
#include "../windows/GUIClasses.h"
#include "../CGameInfo.h"
#include "../CPlayerInterface.h"

#include "../../CCallback.h"

#include "../../lib/ArtifactUtils.h"
#include "../../lib/CGeneralTextHandler.h"
#include "../../lib/CCreatureHandler.h"
#include "../../lib/mapObjects/CGHeroInstance.h"
#include "../../lib/TextOperations.h"
#include "../../lib/gameState/CGameState.h"

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
					boost::algorithm::replace_first(temp,"%s",creature->getNameSingularTranslated());
				}
				else if (owner->getSelection()->creature == creature)
				{
					temp = CGI->generaltexth->tcommands[2]; //Combine %s armies
					boost::algorithm::replace_first(temp,"%s",creature->getNameSingularTranslated());
				}
				else if (owner->getSelection()->creature)
				{
					temp = CGI->generaltexth->tcommands[7]; //Exchange %s with %s
					boost::algorithm::replace_first(temp,"%s",owner->getSelection()->creature->getNameSingularTranslated());
					boost::algorithm::replace_first(temp,"%s",creature->getNameSingularTranslated());
				}
				else
				{
					logGlobal->warn("Warning - shouldn't be - highlighted void slot %d", owner->getSelection()->ID.getNum());
					logGlobal->warn("Highlighted set to nullptr");
					owner->selectSlot(nullptr);
				}
			}
			else
			{
				const bool isHeroOnMap = owner->upperArmy() // Hero is not a visitor and not a garrison defender
					&& owner->upperArmy()->ID == Obj::HERO
					&& (!owner->lowerArmy() || owner->lowerArmy()->ID == Obj::HERO) // one hero or we are in the Heroes exchange window
					&& !(static_cast<const CGHeroInstance*>(owner->upperArmy()))->inTownGarrison;

				if(isHeroOnMap)
				{
					temp = CGI->generaltexth->allTexts[481]; //Select %s
				}
				else if(upg == EGarrisonType::UPPER)
				{
					temp = CGI->generaltexth->tcommands[12]; //Select %s (in garrison)
				}
				else // Hero is visiting some object (town, mine, etc)
				{
					temp = CGI->generaltexth->tcommands[32]; //Select %s (visiting)
				}
				boost::algorithm::replace_first(temp,"%s",creature->getNameSingularTranslated());
			}
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
					boost::algorithm::replace_first(temp,"%s",owner->getSelection()->creature->getNameSingularTranslated());
				}
			}
			else
			{
				temp = CGI->generaltexth->tcommands[11]; //Empty
			}
		}
		GH.statusbar()->write(temp);
	}
	else
	{
		GH.statusbar()->clear();
	}
}

const CArmedInstance * CGarrisonSlot::getObj() const
{
	return owner->army(upg);
}

bool CGarrisonSlot::our() const
{
	return owner->isArmyOwned(upg);
}

bool CGarrisonSlot::ally() const
{
	if(!getObj())
		return false;

	return PlayerRelations::ALLIES == LOCPLINT->cb->getPlayerRelations(LOCPLINT->playerID, getObj()->tempOwner);
}

std::function<void()> CGarrisonSlot::getDismiss() const
{
	const bool canDismiss = getObj()->tempOwner == LOCPLINT->playerID
		&& (getObj()->stacksCount() > 1 ||
			!getObj()->needsLastStack());

	return canDismiss ? [=]()
	{
		LOCPLINT->cb->dismissCreature(getObj(), ID);
	} : (std::function<void()>)nullptr;
}

/// The creature slot has been clicked twice, therefore the creature info should be shown
/// @return Whether the view should be refreshed
bool CGarrisonSlot::viewInfo()
{
	UpgradeInfo pom;
	LOCPLINT->cb->fillUpgradeInfo(getObj(), ID, pom);

	bool canUpgrade = getObj()->tempOwner == LOCPLINT->playerID && pom.oldID>=0; //upgrade is possible
	std::function<void(CreatureID)> upgr = nullptr;
	auto dism = getDismiss();
	if(canUpgrade) upgr = [=] (CreatureID newID) { LOCPLINT->cb->upgradeCreature(getObj(), ID, newID); };

	owner->selectSlot(nullptr);
	owner->setSplittingMode(false);

	for(auto & elem : owner->splitButtons)
		elem->block(true);

	redraw();
	GH.windows().createAndPushWindow<CStackWindow>(myStack, dism, pom, upgr);
	return true;
}

/// The selection is empty, therefore the creature should be moved
/// @return Whether the view should be refreshed
bool CGarrisonSlot::highlightOrDropArtifact()
{
	bool artSelected = false;
	if (auto chw = GH.windows().topWindow<CWindowWithArtifacts>()) //dirty solution
	{
		const auto pickedArtInst = chw->getPickedArtifact();

		if(pickedArtInst)
		{
			const auto * srcHero = chw->getHeroPickedArtifact();
			artSelected = true;
			if (myStack) // try dropping the artifact only if the slot isn't empty
			{
				ArtifactLocation src(srcHero, ArtifactPosition::TRANSITION_POS);
				ArtifactLocation dst(myStack, ArtifactPosition::CREATURE_SLOT);
				if(pickedArtInst->canBePutAt(dst, true))
				{	//equip clicked stack
					if(dst.getArt())
					{
						//creature can wear only one active artifact
						//if we are placing a new one, the old one will be returned to the hero's backpack
						LOCPLINT->cb->swapArtifacts(dst, ArtifactLocation(srcHero,
							ArtifactUtils::getArtBackpackPosition(srcHero, dst.getArt()->getTypeId())));
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

	auto splitFunctor = [this, selection](int amountLeft, int amountRight)
	{
		owner->splitStacks(selection, owner->army(upg), ID, amountRight);
	};

	GH.windows().createAndPushWindow<CSplitWindow>(selection->creature,  splitFunctor, minLeft, minRight, countLeft, countRight);
	return true;
}

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
		if (selection->upg == EGarrisonType::UPPER)
			return true;
		else
			return creature || upg == EGarrisonType::UPPER;
	}
	return false;
}

void CGarrisonSlot::showPopupWindow(const Point & cursorPosition)
{
	if(creature)
	{
		GH.windows().createAndPushWindow<CStackWindow>(myStack, true);
	}
}

void CGarrisonSlot::clickPressed(const Point & cursorPosition)
{
		bool refr = false;
		const CGarrisonSlot * selection = owner->getSelection();

		if(!selection)
		{
			refr = highlightOrDropArtifact(); // Affects selection
			handleSplittingShortcuts();
		}
		else if(selection == this)
		{
			if(!handleSplittingShortcuts())
				refr = viewInfo(); // Affects selection
		}
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
			const CArmedInstance * selectedObj = owner->army(selection->upg);
			bool lastHeroStackSelected = false;
			if(selectedObj->stacksCount() == 1
				&& owner->getSelection()->upg != upg
				&& selectedObj->needsLastStack())
			{
				lastHeroStackSelected = true;
			}

			if((owner->getSplittingMode() || GH.isKeyboardShiftDown()) // split window
				&& (!creature || creature == selection->creature))
			{
				refr = split();
			}
			else if(!creature && lastHeroStackSelected) // split all except last creature
				LOCPLINT->cb->splitStack(selectedObj, owner->army(upg), selection->ID, ID, selection->myStack->count - 1);
			else if(creature != selection->creature) // swap
				LOCPLINT->cb->swapCreatures(owner->army(upg), selectedObj, ID, selection->ID);
			else if(lastHeroStackSelected) // merge last stack to other hero stack
				refr = split();
			else // merge
				LOCPLINT->cb->mergeStacks(selectedObj, owner->army(upg), selection->ID, ID);
		}
		if(refr)
		{
			// Refresh Statusbar
			hover(false);
			hover(true);
		}
}

void CGarrisonSlot::gesture(bool on, const Point & initialPosition, const Point & finalPosition)
{
	if (!on)
		return;

	std::vector<RadialMenuConfig> menuElements = {
		{ RadialMenuConfig::ITEM_NW, "stackMerge", "", [this](){owner->bulkMergeStacks(this);} },
		{ RadialMenuConfig::ITEM_NE, "stackInfo", "", [this](){viewInfo();} },
		{ RadialMenuConfig::ITEM_WW, "stackSplitOne", "", [this](){splitIntoParts(this->getGarrison(), 1); } },
		{ RadialMenuConfig::ITEM_EE, "stackSplitEqual", "", [this](){owner->bulkSmartSplitStack(this);} },
		{ RadialMenuConfig::ITEM_SW, "heroMove", "", [this](){owner->moveStackToAnotherArmy(this);} },
	};

	GH.windows().createAndPushWindow<RadialMenu>(pos.center(), menuElements);
}

void CGarrisonSlot::update()
{
	if(getObj() != nullptr)
	{
		addUsedEvents(LCLICK | SHOW_POPUP | GESTURE | HOVER);
		myStack = getObj()->getStackPtr(ID);
		creature = myStack ? myStack->type : nullptr;
	}
	else
	{
		removeUsedEvents(LCLICK | SHOW_POPUP | GESTURE | HOVER);
		myStack = nullptr;
		creature = nullptr;
	}

	if(creature)
	{
		creatureImage->enable();
		creatureImage->setFrame(creature->getIconIndex());

		stackCount->enable();
		stackCount->setText(TextOperations::formatMetric(myStack->count, 4));
	}
	else
	{
		creatureImage->disable();
		stackCount->disable();
	}
}

CGarrisonSlot::CGarrisonSlot(CGarrisonInt * Owner, int x, int y, SlotID IID, EGarrisonType Upg, const CStackInstance * creature_)
	: ID(IID),
	owner(Owner),
	myStack(creature_),
	creature(creature_ ? creature_->type : nullptr),
	upg(Upg)
{
	OBJECT_CONSTRUCTION_CAPTURING(255-DISPOSE);

	pos.x += x;
	pos.y += y;

	std::string imgName = owner->smallIcons ? "cprsmall" : "TWCRPORT";

	creatureImage = std::make_shared<CAnimImage>(imgName, 0);
	creatureImage->disable();

	selectionImage = std::make_shared<CAnimImage>(imgName, 1);
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

	int labelPosW = pos.w;
	int labelPosH = pos.h;

	if(Owner->layout == CGarrisonInt::ESlotsLayout::REVERSED_TWO_ROWS) //labels under icon
	{
		labelPosW = pos.w / 2 + 1;
		labelPosH += 7;
	}
	ETextAlignment labelAlignment = Owner->layout == CGarrisonInt::ESlotsLayout::REVERSED_TWO_ROWS
			? ETextAlignment::CENTER
			: ETextAlignment::BOTTOMRIGHT;

	stackCount = std::make_shared<CLabel>(labelPosW, labelPosH, owner->smallIcons ? FONT_TINY : FONT_MEDIUM, labelAlignment, Colors::WHITE);

	update();
}

void CGarrisonSlot::splitIntoParts(EGarrisonType type, int amount)
{
	auto empty = owner->getEmptySlot(type);

	if(empty == SlotID())
		return;

	owner->splitStacks(this, owner->army(type), empty, amount);
}

bool CGarrisonSlot::handleSplittingShortcuts()
{
	const bool isAlt = GH.isKeyboardAltDown();
	const bool isLShift = GH.isKeyboardShiftDown();
	const bool isLCtrl = GH.isKeyboardCtrlDown();

	if(!isAlt && !isLShift && !isLCtrl)
		return false; // This is only case when return false

	auto selected = owner->getSelection();
	if(!selected)
		return true; // Some Shortcusts are pressed but there are no appropriate actions

	auto units = selected->myStack->count;
	if(units < 1)
		return true;

	if (isLShift && isLCtrl && isAlt)
	{
		owner->bulkMoveArmy(selected);
	}
	else if(isLCtrl && isAlt)
	{
		owner->moveStackToAnotherArmy(selected);
	}
	else if(isLShift && isAlt)
	{
		auto dismiss = getDismiss();
		if(dismiss)
			LOCPLINT->showYesNoDialog(CGI->generaltexth->allTexts[12], dismiss, nullptr);
	}
	else if(isAlt)
	{
		owner->bulkMergeStacks(selected);
	}
	else
	{
		if(units <= 1)
			return true;

		if(isLCtrl && isLShift)
			owner->bulkSplitStack(selected);
		else if(isLShift)
			owner->bulkSmartSplitStack(selected);
		else
			splitIntoParts(selected->upg, 1); // LCtrl
	}
	return true;
}

void CGarrisonInt::addSplitBtn(std::shared_ptr<CButton> button)
{
	addChild(button.get());
	button->recActions &= ~DISPOSE;
	splitButtons.push_back(button);
	button->block(getSelection() == nullptr);
}

void CGarrisonInt::createSlots()
{
	int distance = interx + (smallIcons ? 32 : 58);
	for(auto i : { EGarrisonType::UPPER, EGarrisonType::LOWER })
	{
		Point offset = garOffset * static_cast<int>(i);

		std::vector<std::shared_ptr<CGarrisonSlot>> garrisonSlots;
		garrisonSlots.resize(7);
		if(army(i))
		{
			for(auto & elem : army(i)->Slots())
			{
				garrisonSlots[elem.first.getNum()] = std::make_shared<CGarrisonSlot>(this, offset.x + (elem.first.getNum()*distance), offset.y, elem.first, i, elem.second);
			}
		}
		for(int j = 0; j < 7; j++)
		{
			if(!garrisonSlots[j])
				garrisonSlots[j] = std::make_shared<CGarrisonSlot>(this, offset.x + (j*distance), offset.x, SlotID(j), i, nullptr);

			if(layout == ESlotsLayout::TWO_ROWS && j >= 4)
			{
				garrisonSlots[j]->moveBy(Point(-126, 37));
			}
			else if(layout == ESlotsLayout::REVERSED_TWO_ROWS)
			{
				if(j >= 3)
				{
					garrisonSlots[j]->moveBy(Point(-90, 49));
				}
				else
				{
					garrisonSlots[j]->moveBy(Point(36, 0));
				}
			}
		}
		vstd::concatenate(availableSlots, garrisonSlots);
	}
}

void CGarrisonInt::recreateSlots()
{
	selectSlot(nullptr);
	setSplittingMode(false);

	for(auto & elem : splitButtons)
		elem->block(true);

	for(auto slot : availableSlots)
		slot->update();
}

void CGarrisonInt::splitClick()
{
	if(!getSelection())
		return;
	setSplittingMode(!getSplittingMode());
	redraw();
}

void CGarrisonInt::splitStacks(const CGarrisonSlot * from, const CArmedInstance * armyDest, SlotID slotDest, int amount )
{
	LOCPLINT->cb->splitStack(armedObjs[from->upg], armyDest, from->ID, slotDest, amount);
}

bool CGarrisonInt::checkSelected(const CGarrisonSlot * selected, TQuantity min) const
{
	return selected && selected->myStack && selected->myStack->count > min && selected->creature;
}

void CGarrisonInt::moveStackToAnotherArmy(const CGarrisonSlot * selected)
{
	if(!checkSelected(selected))
		return;

	const auto srcArmyType = selected->upg;
	const auto destArmyType = srcArmyType == EGarrisonType::UPPER
		? EGarrisonType::LOWER
		: EGarrisonType::UPPER;

	auto srcArmy = armedObjs[srcArmyType];
	auto destArmy = armedObjs[destArmyType];

	if(!destArmy)
		return;

	auto destSlot = destArmy->getSlotFor(selected->creature);

	if(destSlot == SlotID())
		return;

	const auto srcSlot = selected->ID;
	const bool isDestSlotEmpty = !destArmy->getStackCount(destSlot);

	if(isDestSlotEmpty && !destArmy->getStackCount(srcSlot))
		destSlot = srcSlot; // Same place is more preferable

	const bool isLastStack = srcArmy->stacksCount() == 1 && srcArmy->needsLastStack();
	auto srcAmount = selected->myStack->count - (isLastStack ? 1 : 0);

	if(!srcAmount)
		return;

	if(!isDestSlotEmpty || isLastStack)
	{
		srcAmount += destArmy->getStackCount(destSlot); // Due to 'split' implementation in the 'CGameHandler::arrangeStacks'
		LOCPLINT->cb->splitStack(srcArmy, destArmy, srcSlot, destSlot, srcAmount);
	}
	else
	{
		LOCPLINT->cb->swapCreatures(srcArmy, destArmy, srcSlot, destSlot);
	}
}

void CGarrisonInt::bulkMoveArmy(const CGarrisonSlot * selected)
{
	if(!checkSelected(selected))
		return;

	const auto srcArmyType = selected->upg;
	const auto destArmyType = (srcArmyType == EGarrisonType::UPPER)
		? EGarrisonType::LOWER
		: EGarrisonType::UPPER;

	auto srcArmy = armedObjs[srcArmyType];
	auto destArmy = armedObjs[destArmyType];

	if(!destArmy)
		return;

	const auto srcSlot = selected->ID;
	LOCPLINT->cb->bulkMoveArmy(srcArmy->id, destArmy->id, srcSlot);
}

void CGarrisonInt::bulkMergeStacks(const CGarrisonSlot * selected)
{
	if(!checkSelected(selected))
		return;

	const auto type = selected->upg;

	if(!armedObjs[type]->hasCreatureSlots(selected->creature, selected->ID))
		return;

	LOCPLINT->cb->bulkMergeStacks(armedObjs[type]->id, selected->ID);
}

void CGarrisonInt::bulkSplitStack(const CGarrisonSlot * selected)
{
	if(!checkSelected(selected, 1)) // check if > 1
		return;

	const auto type = selected->upg;

	if(!hasEmptySlot(type))
		return;

	LOCPLINT->cb->bulkSplitStack(armedObjs[type]->id, selected->ID);
}

void CGarrisonInt::bulkSmartSplitStack(const CGarrisonSlot * selected)
{
	if(!checkSelected(selected, 1))
		return;

	const auto type = selected->upg;

	// Do not disturb the server if the creature is already balanced
	if(!hasEmptySlot(type) && armedObjs[type]->isCreatureBalanced(selected->creature))
		return;

	LOCPLINT->cb->bulkSmartSplitStack(armedObjs[type]->id, selected->ID);
}

CGarrisonInt::CGarrisonInt(const Point & position, int inx, const Point & garsOffset, const CArmedInstance * s1, const CArmedInstance * s2, bool _removableUnits, bool smallImgs, ESlotsLayout _layout)
	: highlighted(nullptr)
	, inSplittingMode(false)
	, interx(inx)
	, garOffset(garsOffset)
	, smallIcons(smallImgs)
	, removableUnits(_removableUnits)
	, layout(_layout)
{
	OBJECT_CONSTRUCTION_CAPTURING(255-DISPOSE);

	setArmy(s1, EGarrisonType::UPPER);
	setArmy(s2, EGarrisonType::LOWER);
	pos += position;
	createSlots();
}

const CGarrisonSlot * CGarrisonInt::getSelection() const
{
	return highlighted;
}

void CGarrisonInt::selectSlot(CGarrisonSlot *slot)
{
	if(slot != highlighted)
	{
		if(highlighted)
			highlighted->setHighlight(false);

		highlighted = slot;
		for(auto button : splitButtons)
			button->block(highlighted == nullptr || !slot->our());

		if(highlighted)
			highlighted->setHighlight(true);
	}
}

void CGarrisonInt::setSplittingMode(bool on)
{
	assert(on == false || highlighted != nullptr); //can't be in splitting mode without selection

	if(inSplittingMode || on)
	{
		for(auto slot : availableSlots)
		{
			if(slot.get() != getSelection())
				slot->setHighlight( ( on && (slot->our() || slot->ally()) && (slot->creature == nullptr || slot->creature == getSelection()->creature)));
		}
		inSplittingMode = on;
	}
}

bool CGarrisonInt::getSplittingMode()
{
	return inSplittingMode;
}

SlotID CGarrisonInt::getEmptySlot(EGarrisonType type) const
{
	assert(army(type));
	return army(type) ? army(type)->getFreeSlot() : SlotID();
}

bool CGarrisonInt::hasEmptySlot(EGarrisonType type) const
{
	return getEmptySlot(type) != SlotID();
}

const CArmedInstance * CGarrisonInt::upperArmy() const
{
	return army(EGarrisonType::UPPER);
}

const CArmedInstance * CGarrisonInt::lowerArmy() const
{
	return army(EGarrisonType::LOWER);
}

const CArmedInstance * CGarrisonInt::army(EGarrisonType which) const
{
	if(armedObjs.count(which))
		return armedObjs.at(which);
	return nullptr;
}

bool CGarrisonInt::isArmyOwned(EGarrisonType which) const
{
	const auto * object = army(which);

	if (!object)
		return false;

	if (object->tempOwner == LOCPLINT->playerID)
		return true;

	if (object->tempOwner == PlayerColor::UNFLAGGABLE)
		return true;

	return false;
}

void CGarrisonInt::setArmy(const CArmedInstance * army, EGarrisonType type)
{
	armedObjs[type] = army;
}
