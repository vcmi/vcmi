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

#include "../../gui/CGuiHandler.h"
#include "../../gui/Shortcut.h"
#include "../../widgets/Buttons.h"
#include "../../widgets/TextControls.h"

#include "../../CGameInfo.h"
#include "../../CPlayerInterface.h"

#include "../../../CCallback.h"

#include "../../../lib/CArtifactInstance.h"
#include "../../../lib/entities/building/CBuilding.h"
#include "../../../lib/entities/faction/CTownHandler.h"
#include "../../../lib/mapObjects/CGHeroInstance.h"
#include "../../../lib/mapObjects/CGMarket.h"
#include "../../../lib/mapObjects/CGTownInstance.h"
#include "../../../lib/texts/CGeneralTextHandler.h"

CArtifactsSelling::CArtifactsSelling(const IMarket * market, const CGHeroInstance * hero)
	: CMarketBase(market, hero)
	, CResourcesBuying(
		[this](const std::shared_ptr<CTradeableItem> & resSlot){CArtifactsSelling::onSlotClickPressed(resSlot, offerTradePanel);},
		[this](){CArtifactsSelling::updateSubtitles();})
{
	OBJECT_CONSTRUCTION;

	std::string title;
	if(const auto townMarket = dynamic_cast<const CGTownInstance*>(market))
		title = (*CGI->townh)[townMarket->getFaction()]->town->buildings[BuildingID::ARTIFACT_MERCHANT]->getNameTranslated();
	else if(const auto mapMarket = dynamic_cast<const CGMarket*>(market))
		title = mapMarket->title;

	labels.emplace_back(std::make_shared<CLabel>(titlePos.x, titlePos.y, FONT_BIG, ETextAlignment::CENTER, Colors::YELLOW, title));
	labels.push_back(std::make_shared<CLabel>(155, 56, FONT_SMALL, ETextAlignment::CENTER, Colors::WHITE, boost::str(boost::format(CGI->generaltexth->allTexts[271]) % hero->getNameTranslated())));
	deal = std::make_shared<CButton>(dealButtonPos, AnimationPath::builtin("TPMRKB.DEF"),
		CGI->generaltexth->zelp[595], [this](){CArtifactsSelling::makeDeal();}, EShortcut::MARKET_DEAL);
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
		LOCPLINT->showInfoDialog(CGI->generaltexth->allTexts[21]);
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
	LOCPLINT->cb->trade(market, EMarketMode::ARTIFACT_RESOURCE, art->getId(), GameResID(offerTradePanel->getSelectedItemId()), offerQty, hero);
	CMarketTraderText::makeDeal();
}

void CArtifactsSelling::updateShowcases()
{
	const auto art = hero->getArt(selectedHeroSlot);
	if(art && offerTradePanel->isHighlighted())
	{
		bidSelectedSlot->image->enable();
		bidSelectedSlot->setID(art->getTypeId().num);
		bidSelectedSlot->image->setFrame(CGI->artifacts()->getByIndex(art->getTypeId())->getIconIndex());
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
			ShowcaseParams {std::to_string(offerQty), offerTradePanel->getSelectedItemId()}
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
		market->getOffer(art->getTypeId(), offerTradePanel->getSelectedItemId(), bidQty, offerQty, EMarketMode::ARTIFACT_RESOURCE);
		deal->block(!LOCPLINT->makingTurn);
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
		message.replaceRawString(offerQty == 1 ? CGI->generaltexth->allTexts[161] : CGI->generaltexth->allTexts[160]);
		message.replaceName(GameResID(offerTradePanel->getSelectedItemId()));
		message.replaceName(art->getTypeId());
		return message.toString();
	}
	else
	{
		return madeTransaction ? CGI->generaltexth->allTexts[162] : CGI->generaltexth->allTexts[163];
	}
}
