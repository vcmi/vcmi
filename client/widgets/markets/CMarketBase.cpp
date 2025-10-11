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
#include "../../GameEngine.h"
#include "../../GameInstance.h"
#include "../../gui/Shortcut.h"
#include "../../widgets/Buttons.h"
#include "../../widgets/TextControls.h"

#include "../../CPlayerInterface.h"

#include "../../../lib/GameLibrary.h"
#include "../../../lib/callback/CCallback.h"
#include "../../../lib/entities/hero/CHeroHandler.h"
#include "../../../lib/mapObjects/CGHeroInstance.h"
#include "../../../lib/mapObjects/CGMarket.h"
#include "../../../lib/texts/CGeneralTextHandler.h"

CMarketBase::CMarketBase(const IMarket * market, const CGHeroInstance * hero)
	: market(market)
	, hero(hero)
{
}

void CMarketBase::deselect()
{
	if(bidTradePanel && bidTradePanel->highlightedSlot)
	{
		bidTradePanel->highlightedSlot->selectSlot(false);
		bidTradePanel->highlightedSlot.reset();
	}
	if(offerTradePanel && offerTradePanel->highlightedSlot)
	{
		offerTradePanel->highlightedSlot->selectSlot(false);
		offerTradePanel->highlightedSlot.reset();
	}
	if(deal)
		deal->block(true);
	bidQty = 0;
	offerQty = 0;
	updateShowcases();
}

void CMarketBase::onSlotClickPressed(const std::shared_ptr<CTradeableItem> & newSlot, std::shared_ptr<TradePanelBase> & curPanel)
{
	if(!newSlot)
	{
		deselect();
		return;
	}

	assert(curPanel);
	if(newSlot == curPanel->highlightedSlot)
		return;

	curPanel->onSlotClickPressed(newSlot);
	highlightingChanged();
	redraw();
}

void CMarketBase::update()
{
	if(bidTradePanel)
		bidTradePanel->update();
	if(offerTradePanel)
		offerTradePanel->update();
}

void CMarketBase::updateSubtitlesForBid(EMarketMode marketMode, int bidId)
{
	if(bidId == -1)
	{
		if(offerTradePanel)
			offerTradePanel->clearSubtitles();
	}
	else
	{
		if(offerTradePanel)
			for(const auto & slot : offerTradePanel->slots)
			{
				int slotBidQty = 0;
				int slotOfferQty = 0;
				market->getOffer(bidId, slot->id, slotBidQty, slotOfferQty, marketMode);
				offerTradePanel->updateOffer(*slot, slotBidQty, slotOfferQty);
			}
	}
};

void CMarketBase::updateShowcases()
{
	const auto updateShowcase = [](const std::shared_ptr<TradePanelBase> & tradePanel, const std::optional<const ShowcaseParams> & params)
	{
		if(params.has_value())
		{
			tradePanel->setShowcaseSubtitle(params.value().text);
			tradePanel->showcaseSlot->image->enable();
			tradePanel->showcaseSlot->image->setFrame(params.value().imageIndex);
		}
		else
		{
			tradePanel->showcaseSlot->clear();
		}
	};

	const auto params = getShowcasesParams();
	if(bidTradePanel)
		updateShowcase(bidTradePanel, params.bidParams);
	if(offerTradePanel)
		updateShowcase(offerTradePanel, params.offerParams);
}

void CMarketBase::highlightingChanged()
{
	offerTradePanel->update();
	updateShowcases();
}

CMarketBase::MarketShowcasesParams CMarketBase::getShowcasesParams() const
{
	return {};
}

CExperienceAltar::CExperienceAltar()
{
	OBJECT_CONSTRUCTION;

	// Experience needed to reach next level
	texts.emplace_back(std::make_shared<CTextBox>(LIBRARY->generaltexth->allTexts[475], Rect(15, 415, 125, 50), 0, FONT_SMALL, ETextAlignment::CENTER, Colors::YELLOW));
	// Total experience on the Altar
	texts.emplace_back(std::make_shared<CTextBox>(LIBRARY->generaltexth->allTexts[476], Rect(15, 495, 125, 40), 0, FONT_SMALL, ETextAlignment::CENTER, Colors::YELLOW));
	expToLevel = std::make_shared<CLabel>(76, 477, FONT_SMALL, ETextAlignment::CENTER);
	expForHero = std::make_shared<CLabel>(76, 545, FONT_SMALL, ETextAlignment::CENTER);
}

void CExperienceAltar::deselect()
{
	expForHero->setText(std::to_string(0));
}

void CExperienceAltar::update()
{
	expToLevel->setText(std::to_string(LIBRARY->heroh->reqExp(LIBRARY->heroh->level(hero->exp) + 1) - hero->exp));
}

CCreaturesSelling::CCreaturesSelling()
{
	OBJECT_CONSTRUCTION;

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

bool CCreaturesSelling::slotDeletingCheck(const std::shared_ptr<CTradeableItem> & slot) const
{
	return hero->getStackCount(SlotID(slot->serial)) == 0 ? true : false;
}

void CCreaturesSelling::updateSubtitles() const
{
	if(bidTradePanel)
		for(const auto & heroSlot : bidTradePanel->slots)
			heroSlot->subtitle->setText(std::to_string(this->hero->getStackCount(SlotID(heroSlot->serial))));
}

CResourcesBuying::CResourcesBuying(const CTradeableItem::ClickPressedFunctor & clickPressedCallback,
	const TradePanelBase::UpdateSlotsFunctor & updSlotsCallback)
{
	OBJECT_CONSTRUCTION;

	offerTradePanel = std::make_shared<ResourcesPanel>(clickPressedCallback, updSlotsCallback);
	offerTradePanel->moveTo(pos.topLeft() + Point(327, 182));
	labels.emplace_back(std::make_shared<CLabel>(445, 148, FONT_SMALL, ETextAlignment::CENTER, Colors::WHITE, LIBRARY->generaltexth->allTexts[168]));
}

CResourcesSelling::CResourcesSelling(const CTradeableItem::ClickPressedFunctor & clickPressedCallback)
{
	OBJECT_CONSTRUCTION;

	bidTradePanel = std::make_shared<ResourcesPanel>(clickPressedCallback, std::bind(&CResourcesSelling::updateSubtitles, this));
	labels.emplace_back(std::make_shared<CLabel>(156, 148, FONT_SMALL, ETextAlignment::CENTER, Colors::WHITE, LIBRARY->generaltexth->allTexts[270]));
}

void CResourcesSelling::updateSubtitles() const
{
	if(bidTradePanel)
		for(const auto & slot : bidTradePanel->slots)
			slot->subtitle->setText(std::to_string(GAME->interface()->cb->getResourceAmount(static_cast<EGameResID>(slot->serial))));
}

CMarketSlider::CMarketSlider(const CSlider::SliderMovingFunctor & movingCallback)
{
	OBJECT_CONSTRUCTION;

	offerSlider = std::make_shared<CSlider>(Point(230, 489), 137, movingCallback, 0, 0, 0, Orientation::HORIZONTAL);
	offerSlider->setScrollBounds(Rect(-215, -50, 575, 120));
	maxAmount = std::make_shared<CButton>(Point(228, 520), AnimationPath::builtin("IRCBTNS.DEF"), LIBRARY->generaltexth->zelp[596],
		[this]()
		{
			offerSlider->scrollToMax();
		}, EShortcut::MARKET_MAX_AMOUNT);
}

void CMarketSlider::deselect()
{
	maxAmount->block(true);
	offerSlider->scrollTo(0);
	offerSlider->block(true);
}

void CMarketSlider::onOfferSliderMoved(int newVal)
{
	if(bidTradePanel->highlightedSlot && offerTradePanel->highlightedSlot)
	{
		offerSlider->scrollTo(newVal);
		updateShowcases();
		redraw();
	}
}

CMarketTraderText::CMarketTraderText(const Point & pos, const EFonts & font, const ColorRGBA & color)
	: madeTransaction(false)
{
	OBJECT_CONSTRUCTION;

	traderText = std::make_shared<CTextBox>("", Rect(pos, traderTextDimensions), 0, font, ETextAlignment::CENTER, color);
}

void CMarketTraderText::deselect()
{
	highlightingChanged();
}

void CMarketTraderText::makeDeal()
{
	madeTransaction = true;
}

void CMarketTraderText::highlightingChanged()
{
	traderText->setText(getTraderText());
}
