/*
 * CTradeBase.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "CTradeBase.h"

#include "../MiscWidgets.h"

#include "../../gui/CGuiHandler.h"
#include "../../widgets/Buttons.h"
#include "../../widgets/TextControls.h"

#include "../../CGameInfo.h"

#include "../../../lib/CGeneralTextHandler.h"
#include "../../../lib/mapObjects/CGHeroInstance.h"

CTradeBase::CTradeBase(const IMarket * market, const CGHeroInstance * hero)
	: market(market)
	, hero(hero)
{
}

void CTradeBase::removeItems(const std::set<std::shared_ptr<CTradeableItem>> & toRemove)
{
	for(auto item : toRemove)
		removeItem(item);
}

void CTradeBase::removeItem(std::shared_ptr<CTradeableItem> item)
{
	rightTradePanel->slots.erase(std::remove(rightTradePanel->slots.begin(), rightTradePanel->slots.end(), item));

	if(hRight == item)
		hRight.reset();
}

void CTradeBase::getEmptySlots(std::set<std::shared_ptr<CTradeableItem>> & toRemove)
{
	for(auto item : leftTradePanel->slots)
		if(!hero->getStackCount(SlotID(item->serial)))
			toRemove.insert(item);
}

void CTradeBase::deselect()
{
	if(hLeft)
		hLeft->selectSlot(false);
	if(hRight)
		hRight->selectSlot(false);
	hLeft = hRight = nullptr;
	deal->block(true);
}

void CTradeBase::onSlotClickPressed(const std::shared_ptr<CTradeableItem> & newSlot, std::shared_ptr<CTradeableItem> & hCurSlot)
{
	if(newSlot == hCurSlot)
		return;

	if(hCurSlot)
		hCurSlot->selectSlot(false);
	hCurSlot = newSlot;
	newSlot->selectSlot(true);
}

CExperienceAltar::CExperienceAltar()
{
	OBJECT_CONSTRUCTION_CAPTURING(255 - DISPOSE);

	// Experience needed to reach next level
	texts.emplace_back(std::make_shared<CTextBox>(CGI->generaltexth->allTexts[475], Rect(15, 415, 125, 50), 0, FONT_SMALL, ETextAlignment::CENTER, Colors::YELLOW));
	// Total experience on the Altar
	texts.emplace_back(std::make_shared<CTextBox>(CGI->generaltexth->allTexts[476], Rect(15, 495, 125, 40), 0, FONT_SMALL, ETextAlignment::CENTER, Colors::YELLOW));
	expToLevel = std::make_shared<CLabel>(75, 477, FONT_SMALL, ETextAlignment::CENTER);
	expForHero = std::make_shared<CLabel>(75, 545, FONT_SMALL, ETextAlignment::CENTER);
}

CCreaturesSelling::CCreaturesSelling()
{
	assert(hero);
	CreaturesPanel::slotsData slots;
	for(auto slotId = SlotID(0); slotId.num < GameConstants::ARMY_SIZE; slotId++)
	{
		if(const auto & creature = hero->getCreature(slotId))
			slots.emplace_back(std::make_tuple(creature->getId(), slotId, hero->getStackCount(slotId)));
	}
	leftTradePanel = std::make_shared<CreaturesPanel>(nullptr, slots);
}

bool CCreaturesSelling::slotDeletingCheck(const std::shared_ptr<CTradeableItem> & slot)
{
	return hero->getStackCount(SlotID(slot->serial)) == 0 ? true : false;
}

void CCreaturesSelling::updateSubtitle()
{
	for(auto & heroSlot : leftTradePanel->slots)
		heroSlot->subtitle = std::to_string(this->hero->getStackCount(SlotID(heroSlot->serial)));
}
