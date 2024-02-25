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
#include "../../widgets/Slider.h"
#include "../../widgets/TextControls.h"

#include "../../CGameInfo.h"
#include "../../CPlayerInterface.h"

#include "../../../CCallback.h"

#include "../../../lib/CGeneralTextHandler.h"
#include "../../../lib/mapObjects/CGHeroInstance.h"
#include "../../../lib/mapObjects/CGMarket.h"

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
	offerTradePanel->slots.erase(std::remove(offerTradePanel->slots.begin(), offerTradePanel->slots.end(), item));

	if(hRight == item)
		hRight.reset();
}

void CTradeBase::getEmptySlots(std::set<std::shared_ptr<CTradeableItem>> & toRemove)
{
	for(auto item : bidTradePanel->slots)
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
	if(maxAmount)
		maxAmount->block(true);
	if(offerSlider)
	{
		offerSlider->scrollTo(0);
		offerSlider->block(true);
	}
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
	OBJECT_CONSTRUCTION_CAPTURING(255 - DISPOSE);

	assert(hero);
	CreaturesPanel::slotsData slots;
	for(auto slotId = SlotID(0); slotId.num < GameConstants::ARMY_SIZE; slotId++)
	{
		if(const auto & creature = hero->getCreature(slotId))
			slots.emplace_back(std::make_tuple(creature->getId(), slotId, hero->getStackCount(slotId)));
	}
	bidTradePanel = std::make_shared<CreaturesPanel>(nullptr, slots);
	bidTradePanel->updateSlotsCallback = std::bind(&CCreaturesSelling::updateSubtitle, this);
}

bool CCreaturesSelling::slotDeletingCheck(const std::shared_ptr<CTradeableItem> & slot)
{
	return hero->getStackCount(SlotID(slot->serial)) == 0 ? true : false;
}

void CCreaturesSelling::updateSubtitle()
{
	for(auto & heroSlot : bidTradePanel->slots)
		heroSlot->subtitle = std::to_string(this->hero->getStackCount(SlotID(heroSlot->serial)));
}

void CCreaturesSelling::updateSlots()
{
	bidTradePanel->updateSlots();
}

CResourcesBuying::CResourcesBuying(TradePanelBase::UpdateSlotsFunctor callback)
{
	OBJECT_CONSTRUCTION_CAPTURING(255 - DISPOSE);

	offerTradePanel = std::make_shared<ResourcesPanel>([](const std::shared_ptr<CTradeableItem>&) {}, callback);
	offerTradePanel->moveTo(pos.topLeft() + Point(327, 181));
	labels.emplace_back(std::make_shared<CLabel>(445, 148, FONT_SMALL, ETextAlignment::CENTER, Colors::WHITE, CGI->generaltexth->allTexts[168]));
}

void CResourcesBuying::updateSubtitles(EMarketMode marketMode)
{
	if(hLeft)
		for(const auto & slot : offerTradePanel->slots)
		{
			int h1, h2; //hlp variables for getting offer
			market->getOffer(hLeft->id, slot->id, h1, h2, marketMode);

			offerTradePanel->updateOffer(*slot, h1, h2);
		}
	else
		offerTradePanel->clearSubtitles();
};

CResourcesSelling::CResourcesSelling()
{
	OBJECT_CONSTRUCTION_CAPTURING(255 - DISPOSE);

	bidTradePanel = std::make_shared<ResourcesPanel>([](const std::shared_ptr<CTradeableItem>&) {}, [this]()
		{
			for(const auto & slot : bidTradePanel->slots)
				slot->subtitle = std::to_string(LOCPLINT->cb->getResourceAmount(static_cast<EGameResID>(slot->serial)));
		});
	labels.emplace_back(std::make_shared<CLabel>(156, 148, FONT_SMALL, ETextAlignment::CENTER, Colors::WHITE, CGI->generaltexth->allTexts[270]));
}

void CResourcesSelling::updateSlots()
{
	bidTradePanel->updateSlots();
}

CMarketMisc::CMarketMisc(SelectionParamsFunctor callback)
	: selectionParamsCallback(callback)
{
}

void CMarketMisc::deselect()
{
	CTradeBase::deselect();
	bidQty = 0;
	offerQty = 0;
	updateSelected();
}

void CMarketMisc::updateSelected()
{
	const auto updateSelectedBody = [](std::shared_ptr<TradePanelBase> & tradePanel, const std::optional<const SelectionParamOneSide> & params)
	{
		std::optional<size_t> lImageIndex = std::nullopt;
		if(params.has_value())
		{
			tradePanel->selectedSubtitle->setText(params.value().text);
			lImageIndex = params.value().imageIndex;
		}
		else
		{
			tradePanel->selectedSubtitle->setText("");
		}
		tradePanel->setSelectedFrameIndex(lImageIndex);
	};

	assert(selectionParamsCallback);
	const auto params = selectionParamsCallback();
	if(bidTradePanel)
		updateSelectedBody(bidTradePanel, std::get<0>(params));
	if(offerTradePanel)
		updateSelectedBody(offerTradePanel, std::get<1>(params));
}
