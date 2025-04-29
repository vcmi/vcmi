/*
 * CArtifactsBuying.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "CArtifactsBuying.h"

#include "../../gui/Shortcut.h"
#include "../../widgets/Buttons.h"
#include "../../widgets/TextControls.h"

#include "../../CPlayerInterface.h"
#include "../../GameInstance.h"

#include "../../../CCallback.h"

#include "../../../lib/GameLibrary.h"
#include "../../../lib/entities/artifact/CArtHandler.h"
#include "../../../lib/mapObjects/CGHeroInstance.h"
#include "../../../lib/mapObjects/IMarket.h"
#include "../../../lib/texts/CGeneralTextHandler.h"

CArtifactsBuying::CArtifactsBuying(const IMarket * market, const CGHeroInstance * hero, const std::string & title)
	: CMarketBase(market, hero)
	, CResourcesSelling([this](const std::shared_ptr<CTradeableItem> & heroSlot){CArtifactsBuying::onSlotClickPressed(heroSlot, bidTradePanel);})
{
	OBJECT_CONSTRUCTION;

	labels.emplace_back(std::make_shared<CLabel>(titlePos.x, titlePos.y, FONT_BIG, ETextAlignment::CENTER, Colors::YELLOW, title));
	deal = std::make_shared<CButton>(dealButtonPos, AnimationPath::builtin("TPMRKB.DEF"),
		LIBRARY->generaltexth->zelp[595], [this](){CArtifactsBuying::makeDeal();}, EShortcut::MARKET_DEAL);
	labels.emplace_back(std::make_shared<CLabel>(445, 148, FONT_SMALL, ETextAlignment::CENTER, Colors::WHITE, LIBRARY->generaltexth->allTexts[168]));

	// Player's resources
	assert(bidTradePanel);
	bidTradePanel->moveTo(pos.topLeft() + Point(39, 184));
	bidTradePanel->showcaseSlot->image->moveTo(pos.topLeft() + Point(141, 454));

	// Artifacts panel
	offerTradePanel = std::make_shared<ArtifactsPanel>([this](const std::shared_ptr<CTradeableItem> & newSlot)
		{
			CArtifactsBuying::onSlotClickPressed(newSlot, offerTradePanel);
		}, [this]()
		{
			CMarketBase::updateSubtitlesForBid(EMarketMode::RESOURCE_ARTIFACT, bidTradePanel->getHighlightedItemId());
		}, market->availableItemsIds(EMarketMode::RESOURCE_ARTIFACT));
	offerTradePanel->deleteSlotsCheck = [this](const std::shared_ptr<CTradeableItem> & slot)
	{
		return vstd::contains(this->market->availableItemsIds(EMarketMode::RESOURCE_ARTIFACT), ArtifactID(slot->id)) ? false : true;
	};
	offerTradePanel->moveTo(pos.topLeft() + Point(328, 181));

	CMarketBase::update();
	CArtifactsBuying::deselect();
}

void CArtifactsBuying::deselect()
{
	CMarketBase::deselect();
	CMarketTraderText::deselect();
}

void CArtifactsBuying::makeDeal()
{
	if(ArtifactID(offerTradePanel->getHighlightedItemId()).toArtifact()->canBePutAt(hero))
	{
		GAME->interface()->cb->trade(market->getObjInstanceID(), EMarketMode::RESOURCE_ARTIFACT, GameResID(bidTradePanel->getHighlightedItemId()),
			ArtifactID(offerTradePanel->getHighlightedItemId()), offerQty, hero);
		CMarketTraderText::makeDeal();
		deselect();
	}
	else
	{
		GAME->interface()->showInfoDialog(LIBRARY->generaltexth->translate("core.genrltxt.326"));
	}
}

CMarketBase::MarketShowcasesParams CArtifactsBuying::getShowcasesParams() const
{
	if(bidTradePanel->isHighlighted() && offerTradePanel->isHighlighted())
		return MarketShowcasesParams
		{
			ShowcaseParams {std::to_string(deal->isBlocked() ? 0 : bidQty), bidTradePanel->getHighlightedItemId()},
			ShowcaseParams {std::to_string(deal->isBlocked() ? 0 : offerQty), LIBRARY->artifacts()->getByIndex(offerTradePanel->getHighlightedItemId())->getIconIndex()}
		};
	else
		return MarketShowcasesParams {std::nullopt, std::nullopt};
}

void CArtifactsBuying::highlightingChanged()
{
	if(bidTradePanel->isHighlighted() && offerTradePanel->isHighlighted())
	{
		market->getOffer(bidTradePanel->getHighlightedItemId(), offerTradePanel->getHighlightedItemId(), bidQty, offerQty, EMarketMode::RESOURCE_ARTIFACT);
		deal->block(GAME->interface()->cb->getResourceAmount(GameResID(bidTradePanel->getHighlightedItemId())) < bidQty || !GAME->interface()->makingTurn);
	}
	CMarketBase::highlightingChanged();
	CMarketTraderText::highlightingChanged();
}

std::string CArtifactsBuying::getTraderText()
{
	if(bidTradePanel->isHighlighted() && offerTradePanel->isHighlighted())
	{
		MetaString message = MetaString::createFromTextID("core.genrltxt.267");
		message.replaceName(ArtifactID(offerTradePanel->getHighlightedItemId()));
		message.replaceNumber(bidQty);
		message.replaceTextID(bidQty == 1 ? "core.genrltxt.161" : "core.genrltxt.160");
		message.replaceName(GameResID(bidTradePanel->getHighlightedItemId()));
		return message.toString();
	}
	else
	{
		return madeTransaction ? LIBRARY->generaltexth->allTexts[162] : LIBRARY->generaltexth->allTexts[163];
	}
}
