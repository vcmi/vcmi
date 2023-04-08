/*
 * QuickRecruitmentWindow.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "QuickRecruitmentWindow.h"
#include "../../lib/mapObjects/CGTownInstance.h"
#include "../CPlayerInterface.h"
#include "../widgets/Buttons.h"
#include "../widgets/CreatureCostBox.h"
#include "../gui/CGuiHandler.h"
#include "../../CCallback.h"
#include "../../lib/ResourceSet.h"
#include "../../lib/CCreatureHandler.h"
#include "CreaturePurchaseCard.h"


void QuickRecruitmentWindow::setButtons()
{
	setCancelButton();
	setBuyButton();
	setMaxButton();
}

void QuickRecruitmentWindow::setCancelButton()
{
	cancelButton = std::make_shared<CButton>(Point((pos.w / 2) + 48, 418), "ICN6432.DEF", CButton::tooltip(), [&](){ close(); }, SDLK_ESCAPE);
	cancelButton->setImageOrder(0, 1, 2, 3);
}

void QuickRecruitmentWindow::setBuyButton()
{
	buyButton = std::make_shared<CButton>(Point((pos.w / 2) - 32, 418), "IBY6432.DEF", CButton::tooltip(), [&](){ purchaseUnits(); }, SDLK_RETURN);
	cancelButton->assignedKeys.insert(SDLK_ESCAPE);
	buyButton->setImageOrder(0, 1, 2, 3);
}

void QuickRecruitmentWindow::setMaxButton()
{
	maxButton = std::make_shared<CButton>(Point((pos.w/2)-112, 418), "IRCBTNS.DEF", CButton::tooltip(), [&](){ maxAllCards(cards); }, SDLK_m);
	maxButton->setImageOrder(0, 1, 2, 3);
}

void QuickRecruitmentWindow::setCreaturePurchaseCards()
{
	int availableAmount = getAvailableCreatures();
	Point position = Point((pos.w - 100*availableAmount - 8*(availableAmount-1))/2,64);
	for (int i = 0; i < GameConstants::CREATURES_PER_TOWN; i++)
	{
		if(!town->town->creatures.at(i).empty() && !town->creatures.at(i).second.empty() && town->creatures[i].first)
		{
			cards.push_back(std::make_shared<CreaturePurchaseCard>(town->creatures[i].second, position, town->creatures[i].first, this));
			position.x += 108;
		}
	}
	totalCost = std::make_shared<CreatureCostBox>(Rect((this->pos.w/2)-45, position.y+260, 97, 74), "");
}

void QuickRecruitmentWindow::initWindow(Rect startupPosition)
{
	pos.x = startupPosition.x + 238;
	pos.y = startupPosition.y + 45;
	pos.w = 332;
	pos.h = 461;
	int creaturesAmount = getAvailableCreatures();
	if(creaturesAmount > 3)
	{
		pos.w += 108 * (creaturesAmount - 3);
		pos.x -= 55 * (creaturesAmount - 3);
	}
	backgroundTexture = std::make_shared<CFilledTexture>("DIBOXBCK.pcx", Rect(0, 0, pos.w, pos.h));
	costBackground = std::make_shared<CPicture>("QuickRecruitmentWindow/costBackground.png", pos.w/2-113, 335);
}

void QuickRecruitmentWindow::maxAllCards(std::vector<std::shared_ptr<CreaturePurchaseCard> > cards)
{
	auto allAvailableResources = LOCPLINT->cb->getResourceAmount();
	for(auto i : boost::adaptors::reverse(cards))
	{
		si32 maxAmount = i->creatureOnTheCard->maxAmount(allAvailableResources);
		vstd::amin(maxAmount, i->maxAmount);

		i->slider->setAmount(maxAmount);

		if(i->slider->getValue() != maxAmount)
			i->slider->moveTo(maxAmount);
		else
			i->sliderMoved(maxAmount);

		i->slider->moveToMax();
		allAvailableResources -= (i->creatureOnTheCard->getFullRecruitCost() * maxAmount);
	}
	maxButton->block(allAvailableResources == LOCPLINT->cb->getResourceAmount());
}


void QuickRecruitmentWindow::purchaseUnits()
{
	for(auto selected : boost::adaptors::reverse(cards))
	{
		if(selected->slider->getValue())
		{
			auto onRecruit = [=](CreatureID id, int count){ LOCPLINT->cb->recruitCreatures(town, town->getUpperArmy(), id, count, selected->creatureOnTheCard->getLevel()-1); };
			CreatureID crid =  selected->creatureOnTheCard->getId();
			SlotID dstslot = town -> getSlotFor(crid);
			if(!dstslot.validSlot())
				continue;
			onRecruit(crid, selected->slider->getValue());
		}
	}
	close();
}

int QuickRecruitmentWindow::getAvailableCreatures()
{
	int creaturesAmount = 0;
	for (int i=0; i< GameConstants::CREATURES_PER_TOWN; i++)
		if(!town->town->creatures.at(i).empty() && !town->creatures.at(i).second.empty() && town->creatures[i].first)
			creaturesAmount++;
	return creaturesAmount;
}

void QuickRecruitmentWindow::updateAllSliders()
{
	auto allAvailableResources = LOCPLINT->cb->getResourceAmount();
	for(auto i : boost::adaptors::reverse(cards))
		allAvailableResources -= (i->creatureOnTheCard->getFullRecruitCost() * i->slider->getValue());
	for(auto i : cards)
	{
		si32 maxAmount = i->creatureOnTheCard->maxAmount(allAvailableResources);
		vstd::amin(maxAmount, i->maxAmount);
		if(maxAmount < 0)
			continue;
		if(i->slider->getValue() + maxAmount < i->maxAmount)
			i->slider->setAmount(i->slider->getValue() + maxAmount);
		else
			i->slider->setAmount(i->maxAmount);
		i->slider->moveTo(i->slider->getValue());
	}
	totalCost->createItems(LOCPLINT->cb->getResourceAmount() - allAvailableResources);
	totalCost->set(LOCPLINT->cb->getResourceAmount() - allAvailableResources);
}

QuickRecruitmentWindow::QuickRecruitmentWindow(const CGTownInstance * townd, Rect startupPosition)
	: CWindowObject(PLAYER_COLORED | BORDERED),
	town(townd)
{
	OBJECT_CONSTRUCTION_CAPTURING(ACTIVATE + DEACTIVATE + UPDATE + SHOWALL);

	initWindow(startupPosition);
	setButtons();
	setCreaturePurchaseCards();
	maxAllCards(cards);
}
