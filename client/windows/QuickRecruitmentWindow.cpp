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
#include "../gui/CGuiHandler.h"
#include "../../CCallback.h"
#include "../CreatureCostBox.h"
#include "../lib/ResourceSet.h"
#include "CreaturePurhaseCard.h"


void QuickRecruitmentWindow::setButtons()
{
	setCancelButton();
	setBuyButton();
	setMaxButton();
}

void QuickRecruitmentWindow::setCancelButton()
{
	cancelButton = std::make_shared<CButton>(Point((pos.w / 2) + 47, 417), "ICN6432.DEF", CButton::tooltip(), [&](){ close(); }, SDLK_RETURN);
	cancelButton->setImageOrder(0, 1, 2, 3);
}

void QuickRecruitmentWindow::setBuyButton()
{
	buyButton = std::make_shared<CButton>(Point((pos.w/2)-33, 417), "IBY6432.DEF", CButton::tooltip(), [&](){ purhaseUnits(); });
	cancelButton->assignedKeys.insert(SDLK_ESCAPE);
	buyButton->setImageOrder(0, 1, 2, 3);
}

void QuickRecruitmentWindow::setMaxButton()
{
	maxButton = std::make_shared<CButton>(Point((pos.w/2)-113, 417), "IRCBTNS.DEF", CButton::tooltip(), [&](){ maxAllCards(cards); });
	maxButton->setImageOrder(0, 1, 2, 3);
}

void QuickRecruitmentWindow::setCreaturePurhaseCards()
{
	Point pos = Point(8,64);
	for (int i=0; i < GameConstants::CREATURES_PER_TOWN; i++)
	{
		if(!town->town->creatures.at(i).empty() && !town->creatures.at(i).second.empty() && town->creatures[i].first)
		{
			CreatureID crid = town->creatures[i].second[0];
			if(town->creatures[i].second.size() > 1)
				crid = town->creatures[i].second[1];

			cards.push_back(std::make_shared<CreaturePurhaseCard>(VLC->creh->creatures[crid], pos, town->creatures[i].first, this));
			pos.x += 108;
		}
	}
	totalCost = std::make_shared<CreatureCostBox>(Rect((this->pos.w/2)-45, pos.y + 260, 97, 74), "");
}

void QuickRecruitmentWindow::initWindow()
{
	pos.x = 238;
	pos.y = 45;
	pos.w = 332;
	pos.h = 461;
	int creaturesAmount = getAvailableCreatures();
	if(creaturesAmount > 3)
	{
		pos.w += 108 * (creaturesAmount - 3);
		pos.x -= 55 * (creaturesAmount - 3);
	}
	backgroundTexture = std::make_shared<CFilledTexture>("DIBOXBCK.pcx", Rect(0, 0, pos.w, pos.h));
}

void QuickRecruitmentWindow::maxAllCards(std::vector<std::shared_ptr<CreaturePurhaseCard> > cards)
{
	auto allAvailableResources = LOCPLINT->cb->getResourceAmount();
	for(auto i : boost::adaptors::reverse(cards))
	{
		si32 maxAmount = i->creature->maxAmount(allAvailableResources);
		vstd::amin(maxAmount, i->maxamount);

		i->slider->setAmount(maxAmount);

		if(i->slider->getValue() != maxAmount)
			i->slider->moveTo(maxAmount);
		else
			i->sliderMoved(maxAmount);

		i->slider->moveToMax();
		allAvailableResources -= (i->creature->cost * maxAmount);
	}
	maxButton->block(allAvailableResources == LOCPLINT->cb->getResourceAmount());
}


void QuickRecruitmentWindow::purhaseUnits()
{
	for(auto selected : cards)
	{
		if(selected->slider->getValue())
		{
			auto onRecruit = [=](CreatureID id, int count){ LOCPLINT->cb->recruitCreatures(town, town->getUpperArmy(), id, count, selected->creature->level-1); };
			CreatureID crid =  selected->creature->idNumber;
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
		allAvailableResources -= (i->creature->cost * i->slider->getValue());
	for(auto i : cards)
	{
		si32 maxAmount = i->creature->maxAmount(allAvailableResources);
		vstd::amin(maxAmount, i->maxamount);
		if(maxAmount < 0)
			continue;
		if(i->slider->getValue() + maxAmount < i->maxamount)
			i->slider->setAmount(i->slider->getValue() + maxAmount);
		else
			i->slider->setAmount(i->maxamount);
		i->slider->moveTo(i->slider->getValue());
	}
	totalCost->createItems(LOCPLINT->cb->getResourceAmount() - allAvailableResources);
	totalCost->set(LOCPLINT->cb->getResourceAmount() - allAvailableResources);
}

QuickRecruitmentWindow::QuickRecruitmentWindow(const CGTownInstance * townd) : CWindowObject(PLAYER_COLORED | BORDERED), town(townd)
{
	OBJECT_CONSTRUCTION_CAPTURING(ACTIVATE + DEACTIVATE + UPDATE + SHOWALL);

	initWindow();
	setButtons();
	setCreaturePurhaseCards();
	maxAllCards(cards);
}
