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

#include "../../gui/CGuiHandler.h"
#include "../../gui/Shortcut.h"
#include "../../widgets/Buttons.h"
#include "../../widgets/TextControls.h"

#include "../../CGameInfo.h"
#include "../../CPlayerInterface.h"

#include "../../../CCallback.h"

#include "../../../lib/entities/building/CBuilding.h"
#include "../../../lib/entities/faction/CTownHandler.h"
#include "../../../lib/mapObjects/CGHeroInstance.h"
#include "../../../lib/mapObjects/CGMarket.h"
#include "../../../lib/mapObjects/CGTownInstance.h"
#include "../../../lib/texts/CGeneralTextHandler.h"

CArtifactsBuying::CArtifactsBuying(const IMarket * market, const CGHeroInstance * hero)
	: CMarketBase(market, hero)
	, CResourcesSelling([this](const std::shared_ptr<CTradeableItem> & heroSlot){CArtifactsBuying::onSlotClickPressed(heroSlot, bidTradePanel);})
{
	OBJECT_CONSTRUCTION;

	std::string title;
	if(auto townMarket = dynamic_cast<const CGTownInstance*>(market))
		title = (*CGI->townh)[townMarket->getFaction()]->town->buildings[BuildingID::ARTIFACT_MERCHANT]->getNameTranslated();
	else
		title = CGI->generaltexth->allTexts[349];
	labels.emplace_back(std::make_shared<CLabel>(titlePos.x, titlePos.y, FONT_BIG, ETextAlignment::CENTER, Colors::YELLOW, title));
	deal = std::make_shared<CButton>(dealButtonPos, AnimationPath::builtin("TPMRKB.DEF"),
		CGI->generaltexth->zelp[595], [this](){CArtifactsBuying::makeDeal();}, EShortcut::MARKET_DEAL);
	labels.emplace_back(std::make_shared<CLabel>(445, 148, FONT_SMALL, ETextAlignment::CENTER, Colors::WHITE, CGI->generaltexth->allTexts[168]));

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
			CMarketBase::updateSubtitlesForBid(EMarketMode::RESOURCE_ARTIFACT, bidTradePanel->getSelectedItemId());
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
	if(ArtifactID(offerTradePanel->getSelectedItemId()).toArtifact()->canBePutAt(hero))
	{
		LOCPLINT->cb->trade(market, EMarketMode::RESOURCE_ARTIFACT, GameResID(bidTradePanel->getSelectedItemId()),
			ArtifactID(offerTradePanel->getSelectedItemId()), offerQty, hero);
		CMarketTraderText::makeDeal();
		deselect();
	}
	else
	{
		LOCPLINT->showInfoDialog(CGI->generaltexth->translate("core.genrltxt.326"));
	}
}

CMarketBase::MarketShowcasesParams CArtifactsBuying::getShowcasesParams() const
{
	if(bidTradePanel->isHighlighted() && offerTradePanel->isHighlighted())
		return MarketShowcasesParams
		{
			ShowcaseParams {std::to_string(deal->isBlocked() ? 0 : bidQty), bidTradePanel->getSelectedItemId()},
			ShowcaseParams {std::to_string(deal->isBlocked() ? 0 : offerQty), CGI->artifacts()->getByIndex(offerTradePanel->getSelectedItemId())->getIconIndex()}
		};
	else
		return MarketShowcasesParams {std::nullopt, std::nullopt};
}

void CArtifactsBuying::highlightingChanged()
{
	if(bidTradePanel->isHighlighted() && offerTradePanel->isHighlighted())
	{
		market->getOffer(bidTradePanel->getSelectedItemId(), offerTradePanel->getSelectedItemId(), bidQty, offerQty, EMarketMode::RESOURCE_ARTIFACT);
		deal->block(LOCPLINT->cb->getResourceAmount(GameResID(bidTradePanel->getSelectedItemId())) < bidQty || !LOCPLINT->makingTurn);
	}
	CMarketBase::highlightingChanged();
	CMarketTraderText::highlightingChanged();
}

std::string CArtifactsBuying::getTraderText()
{
	if(bidTradePanel->isHighlighted() && offerTradePanel->isHighlighted())
	{
		MetaString message = MetaString::createFromTextID("core.genrltxt.267");
		message.replaceName(ArtifactID(offerTradePanel->getSelectedItemId()));
		message.replaceNumber(bidQty);
		message.replaceTextID(bidQty == 1 ? "core.genrltxt.161" : "core.genrltxt.160");
		message.replaceName(GameResID(bidTradePanel->getSelectedItemId()));
		return message.toString();
	}
	else
	{
		return madeTransaction ? CGI->generaltexth->allTexts[162] : CGI->generaltexth->allTexts[163];
	}
}
