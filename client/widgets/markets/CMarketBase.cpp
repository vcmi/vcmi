/*
 * CMarketBase.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "CMarketBase.h"

#include "../MiscWidgets.h"

#include "../Images.h"
#include "../../gui/CGuiHandler.h"
#include "../../widgets/Buttons.h"
#include "../../widgets/TextControls.h"

#include "../../CGameInfo.h"
#include "../../CPlayerInterface.h"

#include "../../../CCallback.h"

#include "../../../lib/CGeneralTextHandler.h"
#include "../../../lib/mapObjects/CGHeroInstance.h"
#include "../../../lib/CHeroHandler.h"
#include "../../../lib/mapObjects/CGMarket.h"

CMarketBase::CMarketBase(const IMarket * market, const CGHeroInstance * hero, const SelectionParamsFunctor & getParamsCallback)
	: market(market)
	, hero(hero)
	, selectionParamsCallback(getParamsCallback)
{
}

void CMarketBase::deselect()
{
	if(hLeft)
		hLeft->selectSlot(false);
	if(hRight)
		hRight->selectSlot(false);
	hLeft = hRight = nullptr;
	deal->block(true);
	bidQty = 0;
	offerQty = 0;
	updateSelected();
}

void CMarketBase::onSlotClickPressed(const std::shared_ptr<CTradeableItem> & newSlot, std::shared_ptr<CTradeableItem> & hCurSlot)
{
	if(newSlot == hCurSlot)
		return;

	if(hCurSlot)
		hCurSlot->selectSlot(false);
	hCurSlot = newSlot;
	newSlot->selectSlot(true);
}

void CMarketBase::update()
{
	if(bidTradePanel)
		bidTradePanel->update();
	if(offerTradePanel)
		offerTradePanel->update();
}

void CMarketBase::updateSubtitles(EMarketMode marketMode)
{
	if(hLeft)
		for(const auto & slot : offerTradePanel->slots)
		{
			int bidQty = 0;
			int offerQty = 0;
			market->getOffer(hLeft->id, slot->id, bidQty, offerQty, marketMode);
			offerTradePanel->updateOffer(*slot, bidQty, offerQty);
		}
	else
		offerTradePanel->clearSubtitles();
};

void CMarketBase::updateSelected()
{
	const auto updateSelectedBody = [](std::shared_ptr<TradePanelBase> & tradePanel, const std::optional<const SelectionParamOneSide> & params)
	{
		std::optional<size_t> lImageIndex = std::nullopt;
		if(params.has_value())
		{
			tradePanel->setSelectedSubtitleText(params.value().text);
			lImageIndex = params.value().imageIndex;
		}
		else
		{
			tradePanel->clearSelectedSubtitleText();
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

CExperienceAltar::CExperienceAltar()
{
	OBJECT_CONSTRUCTION_CAPTURING(255 - DISPOSE);

	// Experience needed to reach next level
	texts.emplace_back(std::make_shared<CTextBox>(CGI->generaltexth->allTexts[475], Rect(15, 415, 125, 50), 0, FONT_SMALL, ETextAlignment::CENTER, Colors::YELLOW));
	// Total experience on the Altar
	texts.emplace_back(std::make_shared<CTextBox>(CGI->generaltexth->allTexts[476], Rect(15, 495, 125, 40), 0, FONT_SMALL, ETextAlignment::CENTER, Colors::YELLOW));
	expToLevel = std::make_shared<CLabel>(76, 477, FONT_SMALL, ETextAlignment::CENTER);
	expForHero = std::make_shared<CLabel>(76, 545, FONT_SMALL, ETextAlignment::CENTER);
}

void CExperienceAltar::deselect()
{
	expForHero->setText(std::to_string(0));
}

void CExperienceAltar::update()
{
	expToLevel->setText(std::to_string(CGI->heroh->reqExp(CGI->heroh->level(hero->exp) + 1) - hero->exp));
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
	bidTradePanel->updateSlotsCallback = std::bind(&CCreaturesSelling::updateSubtitles, this);
}

bool CCreaturesSelling::slotDeletingCheck(const std::shared_ptr<CTradeableItem> & slot)
{
	return hero->getStackCount(SlotID(slot->serial)) == 0 ? true : false;
}

void CCreaturesSelling::updateSubtitles()
{
	for(auto & heroSlot : bidTradePanel->slots)
		heroSlot->subtitle->setText(std::to_string(this->hero->getStackCount(SlotID(heroSlot->serial))));
}

CResourcesBuying::CResourcesBuying(const CTradeableItem::ClickPressedFunctor & clickPressedCallback,
	const TradePanelBase::UpdateSlotsFunctor & updSlotsCallback)
{
	OBJECT_CONSTRUCTION_CAPTURING(255 - DISPOSE);

	offerTradePanel = std::make_shared<ResourcesPanel>(clickPressedCallback, updSlotsCallback);
	offerTradePanel->moveTo(pos.topLeft() + Point(327, 182));
	labels.emplace_back(std::make_shared<CLabel>(445, 148, FONT_SMALL, ETextAlignment::CENTER, Colors::WHITE, CGI->generaltexth->allTexts[168]));
}

CResourcesSelling::CResourcesSelling(const CTradeableItem::ClickPressedFunctor & clickPressedCallback)
{
	OBJECT_CONSTRUCTION_CAPTURING(255 - DISPOSE);

	bidTradePanel = std::make_shared<ResourcesPanel>(clickPressedCallback, std::bind(&CResourcesSelling::updateSubtitles, this));
	labels.emplace_back(std::make_shared<CLabel>(156, 148, FONT_SMALL, ETextAlignment::CENTER, Colors::WHITE, CGI->generaltexth->allTexts[270]));
}

void CResourcesSelling::updateSubtitles()
{
	for(const auto & slot : bidTradePanel->slots)
		slot->subtitle->setText(std::to_string(LOCPLINT->cb->getResourceAmount(static_cast<EGameResID>(slot->serial))));
}

CMarketSlider::CMarketSlider(const CSlider::SliderMovingFunctor & movingCallback)
{
	OBJECT_CONSTRUCTION_CAPTURING(255 - DISPOSE);

	offerSlider = std::make_shared<CSlider>(Point(230, 489), 137, movingCallback, 0, 0, 0, Orientation::HORIZONTAL);
	maxAmount = std::make_shared<CButton>(Point(228, 520), AnimationPath::builtin("IRCBTNS.DEF"), CGI->generaltexth->zelp[596],
		[this]()
		{
			offerSlider->scrollToMax();
		});
}

void CMarketSlider::deselect()
{
	maxAmount->block(true);
	offerSlider->scrollTo(0);
	offerSlider->block(true);
}

void CMarketSlider::onOfferSliderMoved(int newVal)
{
	if(hLeft && hRight)
	{
		offerSlider->scrollTo(newVal);
		updateSelected();
		redraw();
	}
}
