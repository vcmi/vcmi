/*
 * CArtifactsSelling.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "CArtifactsSelling.h"

#include "../../GameEngine.h"
#include "../../GameInstance.h"
#include "../../gui/Shortcut.h"
#include "../../widgets/Buttons.h"
#include "../../widgets/TextControls.h"

#include "../../CPlayerInterface.h"

#include "../../../CCallback.h"

#include "../../../lib/GameLibrary.h"
#include "../../../lib/entities/artifact/CArtifact.h"
#include "../../../lib/mapObjects/CGHeroInstance.h"
#include "../../../lib/mapObjects/IMarket.h"
#include "../../../lib/texts/CGeneralTextHandler.h"

CArtifactsSelling::CArtifactsSelling(const IMarket * market, const CGHeroInstance * hero, const std::string & title)
	: CMarketBase(market, hero)
	, CResourcesBuying(
		[this](const std::shared_ptr<CTradeableItem> & resSlot){CArtifactsSelling::onSlotClickPressed(resSlot, offerTradePanel);},
		[this](){CArtifactsSelling::updateSubtitles();})
{
	OBJECT_CONSTRUCTION;

	labels.emplace_back(std::make_shared<CLabel>(titlePos.x, titlePos.y, FONT_BIG, ETextAlignment::CENTER, Colors::YELLOW, title));
	labels.push_back(std::make_shared<CLabel>(155, 56, FONT_SMALL, ETextAlignment::CENTER, Colors::WHITE, boost::str(boost::format(LIBRARY->generaltexth->allTexts[271]) % hero->getNameTranslated())));
	deal = std::make_shared<CButton>(dealButtonPos, AnimationPath::builtin("TPMRKB.DEF"),
		LIBRARY->generaltexth->zelp[595], [this](){CArtifactsSelling::makeDeal();}, EShortcut::MARKET_DEAL);
	bidSelectedSlot = std::make_shared<CTradeableItem>(Rect(Point(123, 470), Point(69, 66)), EType::ARTIFACT_TYPE, 0, 0);

	// Market resources panel
	assert(offerTradePanel);
	offerTradePanel->moveTo(pos.topLeft() + Point(326, 184));
	offerTradePanel->showcaseSlot->moveTo(pos.topLeft() + Point(409, 473));
	offerTradePanel->showcaseSlot->subtitle->moveBy(Point(0, 1));
	
	// Hero's artifacts
	heroArts = std::make_shared<CArtifactsOfHeroMarket>(Point(-361, 46), offerTradePanel->selectionWidth);
	heroArts->setHero(hero);
	heroArts->onSelectArtCallback = [this](const CArtPlace * artPlace)
	{
		assert(artPlace);
		selectedHeroSlot = artPlace->slot;
		CArtifactsSelling::highlightingChanged();
		CIntObject::redraw();
	};
	heroArts->onClickNotTradableCallback = []()
	{
		// This item can't be traded
		GAME->interface()->showInfoDialog(LIBRARY->generaltexth->allTexts[21]);
	};
	CArtifactsSelling::updateShowcases();
	CArtifactsSelling::deselect();
}

void CArtifactsSelling::deselect()
{
	CMarketBase::deselect();
	CMarketTraderText::deselect();
	selectedHeroSlot = ArtifactPosition::PRE_FIRST;
	heroArts->unmarkSlots();
	bidSelectedSlot->clear();
}

void CArtifactsSelling::makeDeal()
{
	const auto art = hero->getArt(selectedHeroSlot);
	assert(art);
	GAME->interface()->cb->trade(market->getObjInstanceID(), EMarketMode::ARTIFACT_RESOURCE, art->getId(),
		GameResID(offerTradePanel->getHighlightedItemId()), offerQty, hero);
	CMarketTraderText::makeDeal();
}

void CArtifactsSelling::updateShowcases()
{
	const auto art = hero->getArt(selectedHeroSlot);
	if(art && offerTradePanel->isHighlighted())
	{
		bidSelectedSlot->image->enable();
		bidSelectedSlot->setID(art->getTypeId().num);
		bidSelectedSlot->image->setFrame(art->getTypeId().toEntity(LIBRARY)->getIconIndex());
		bidSelectedSlot->subtitle->setText(std::to_string(bidQty));
	}
	else
	{
		bidSelectedSlot->clear();
	}
	CMarketBase::updateShowcases();
}

void CArtifactsSelling::update()
{
	CMarketBase::update();
	if(selectedHeroSlot != ArtifactPosition::PRE_FIRST)
	{
		if(hero->getArt(selectedHeroSlot) == nullptr)
		{
			deselect();
			selectedHeroSlot = ArtifactPosition::PRE_FIRST;
		}
		else
		{
			heroArts->getArtPlace(selectedHeroSlot)->selectSlot(true);
		}
		highlightingChanged();
	}
}

std::shared_ptr<CArtifactsOfHeroMarket> CArtifactsSelling::getAOHset() const
{
	return heroArts;
}

CMarketBase::MarketShowcasesParams CArtifactsSelling::getShowcasesParams() const
{
	if(hero->getArt(selectedHeroSlot) && offerTradePanel->isHighlighted())
		return MarketShowcasesParams
		{
			std::nullopt,
			ShowcaseParams {std::to_string(offerQty), offerTradePanel->getHighlightedItemId()}
		};
	else
		return MarketShowcasesParams {std::nullopt, std::nullopt};
}

void CArtifactsSelling::updateSubtitles()
{
	const auto art = this->hero->getArt(selectedHeroSlot);
	const int bidId = art == nullptr ? -1 : art->getTypeId().num;
	CMarketBase::updateSubtitlesForBid(EMarketMode::ARTIFACT_RESOURCE, bidId);
}

void CArtifactsSelling::highlightingChanged()
{
	const auto art = hero->getArt(selectedHeroSlot);
	if(art && offerTradePanel->isHighlighted())
	{
		market->getOffer(art->getTypeId(), offerTradePanel->getHighlightedItemId(), bidQty, offerQty, EMarketMode::ARTIFACT_RESOURCE);
		deal->block(!GAME->interface()->makingTurn);
	}
	CMarketBase::highlightingChanged();
	CMarketTraderText::highlightingChanged();
}

std::string CArtifactsSelling::getTraderText()
{
	const auto art = hero->getArt(selectedHeroSlot);
	if(art && offerTradePanel->isHighlighted())
	{
		MetaString message = MetaString::createFromTextID("core.genrltxt.268");
		message.replaceNumber(offerQty);
		message.replaceRawString(offerQty == 1 ? LIBRARY->generaltexth->allTexts[161] : LIBRARY->generaltexth->allTexts[160]);
		message.replaceName(GameResID(offerTradePanel->getHighlightedItemId()));
		message.replaceName(art->getTypeId());
		return message.toString();
	}
	else
	{
		return madeTransaction ? LIBRARY->generaltexth->allTexts[162] : LIBRARY->generaltexth->allTexts[163];
	}
}
