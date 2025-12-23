/*
 * CMarketResources.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "CMarketResources.h"

#include "../../GameEngine.h"
#include "../../GameInstance.h"
#include "../../gui/Shortcut.h"
#include "../../widgets/Buttons.h"
#include "../../widgets/TextControls.h"

#include "../../CPlayerInterface.h"

#include "../../../lib/callback/CCallback.h"
#include "../../../lib/texts/CGeneralTextHandler.h"
#include "../../../lib/mapObjects/IMarket.h"
#include "../../../lib/GameLibrary.h"

CMarketResources::CMarketResources(const IMarket * market, const CGHeroInstance * hero)
	: CMarketBase(market, hero)
	, CResourcesSelling([this](const std::shared_ptr<CTradeableItem> & heroSlot){CMarketResources::onSlotClickPressed(heroSlot, bidTradePanel);})
	, CResourcesBuying(
		[this](const std::shared_ptr<CTradeableItem> & resSlot){CMarketResources::onSlotClickPressed(resSlot, offerTradePanel);},
		[this](){CMarketResources::updateSubtitles();})
	, CMarketSlider([this](int newVal){CMarketSlider::onOfferSliderMoved(newVal);})
{
	OBJECT_CONSTRUCTION;

	labels.emplace_back(std::make_shared<CLabel>(titlePos.x, titlePos.y, FONT_BIG, ETextAlignment::CENTER, Colors::YELLOW, LIBRARY->generaltexth->allTexts[158]));
	deal = std::make_shared<CButton>(dealButtonPosWithSlider, AnimationPath::builtin("TPMRKB.DEF"),
		LIBRARY->generaltexth->zelp[595], [this]() {CMarketResources::makeDeal(); }, EShortcut::MARKET_DEAL);

	// Player's resources
	assert(bidTradePanel);
	bidTradePanel->moveTo(pos.topLeft() + Point(39, 182));

	// Market resources panel
	assert(offerTradePanel);

	CMarketBase::update();
	CMarketResources::deselect();
}

void CMarketResources::deselect()
{
	CMarketBase::deselect();
	CMarketSlider::deselect();
	CMarketTraderText::deselect();
}

void CMarketResources::makeDeal()
{
	if(auto toTrade = offerSlider->getValue(); toTrade != 0)
	{
		GAME->interface()->cb->trade(market->getObjInstanceID(), EMarketMode::RESOURCE_RESOURCE, GameResID(bidTradePanel->getHighlightedItemId()),
			GameResID(offerTradePanel->highlightedSlot->id), bidQty * toTrade, hero);
		CMarketTraderText::makeDeal();
		deselect();
	}
}

CMarketBase::MarketShowcasesParams CMarketResources::getShowcasesParams() const
{
	if(bidTradePanel->highlightedSlot && offerTradePanel->highlightedSlot && bidTradePanel->getHighlightedItemId() != offerTradePanel->getHighlightedItemId())
		return MarketShowcasesParams
		{
			ShowcaseParams {std::to_string(bidQty * offerSlider->getValue()), bidTradePanel->getHighlightedItemId()},
			ShowcaseParams {std::to_string(offerQty * offerSlider->getValue()), offerTradePanel->getHighlightedItemId()}
		};
	else
		return MarketShowcasesParams {std::nullopt, std::nullopt};
}

void CMarketResources::highlightingChanged()
{
	if(bidTradePanel->isHighlighted() && offerTradePanel->isHighlighted())
	{
		market->getOffer(bidTradePanel->getHighlightedItemId(), offerTradePanel->getHighlightedItemId(), bidQty, offerQty, EMarketMode::RESOURCE_RESOURCE);
		offerSlider->setAmount(GAME->interface()->cb->getResourceAmount(GameResID(bidTradePanel->getHighlightedItemId())) / bidQty);
		offerSlider->scrollTo(0);
		const bool isControlsBlocked = bidTradePanel->getHighlightedItemId() != offerTradePanel->getHighlightedItemId() ? false : true;
		offerSlider->block(isControlsBlocked);
		maxAmount->block(isControlsBlocked);
		deal->block(isControlsBlocked || !GAME->interface()->makingTurn);
	}
	CMarketBase::highlightingChanged();
	CMarketTraderText::highlightingChanged();
}

void CMarketResources::updateSubtitles()
{
	CMarketBase::updateSubtitlesForBid(EMarketMode::RESOURCE_RESOURCE, bidTradePanel->getHighlightedItemId());
	if(bidTradePanel && bidTradePanel->highlightedSlot)
		for(auto & slot : offerTradePanel->slots)
			if(slot->id == bidTradePanel->highlightedSlot->id)
				slot->subtitle->setText(LIBRARY->generaltexth->allTexts[164]); // n/a
}

std::string CMarketResources::getTraderText()
{
	if(bidTradePanel->isHighlighted() && offerTradePanel->isHighlighted() &&
		bidTradePanel->getHighlightedItemId() != offerTradePanel->getHighlightedItemId())
	{
		MetaString message = MetaString::createFromTextID("core.genrltxt.157");
		message.replaceNumber(offerQty);
		message.replaceRawString(offerQty == 1 ? LIBRARY->generaltexth->allTexts[161] : LIBRARY->generaltexth->allTexts[160]);
		message.replaceName(GameResID(offerTradePanel->getHighlightedItemId()));
		message.replaceNumber(bidQty);
		message.replaceRawString(bidQty == 1 ? LIBRARY->generaltexth->allTexts[161] : LIBRARY->generaltexth->allTexts[160]);
		message.replaceName(GameResID(bidTradePanel->getHighlightedItemId()));
		return message.toString();
	}
	else
	{
		return madeTransaction ? LIBRARY->generaltexth->allTexts[162] : LIBRARY->generaltexth->allTexts[163];
	}
}
