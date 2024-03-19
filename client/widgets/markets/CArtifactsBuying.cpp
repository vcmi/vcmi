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
#include "../../widgets/Buttons.h"
#include "../../widgets/TextControls.h"

#include "../../CGameInfo.h"
#include "../../CPlayerInterface.h"

#include "../../../CCallback.h"

#include "../../../lib/CGeneralTextHandler.h"
#include "../../../lib/mapObjects/CGMarket.h"
#include "../../../lib/mapObjects/CGTownInstance.h"

CArtifactsBuying::CArtifactsBuying(const IMarket * market, const CGHeroInstance * hero)
	: CMarketBase(market, hero, [this](){return CArtifactsBuying::getSelectionParams();})
	, CResourcesSelling([this](const std::shared_ptr<CTradeableItem> & heroSlot){CArtifactsBuying::onSlotClickPressed(heroSlot, bidTradePanel);})
{
	OBJECT_CONSTRUCTION_CUSTOM_CAPTURING(255 - DISPOSE);

	std::string title;
	if(auto townMarket = dynamic_cast<const CGTownInstance*>(market))
		title = (*CGI->townh)[townMarket->getFaction()]->town->buildings[BuildingID::ARTIFACT_MERCHANT]->getNameTranslated();
	else
		title = "Black market";	// find string allTexts!!
	labels.emplace_back(std::make_shared<CLabel>(titlePos.x, titlePos.y, FONT_BIG, ETextAlignment::CENTER, Colors::YELLOW, title));
	deal = std::make_shared<CButton>(dealButtonPos, AnimationPath::builtin("TPMRKB.DEF"),
		CGI->generaltexth->zelp[595], [this](){CArtifactsBuying::makeDeal();});
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
	CMarketBase::deselect();
}

void CArtifactsBuying::makeDeal()
{
	LOCPLINT->cb->trade(market, EMarketMode::RESOURCE_ARTIFACT, GameResID(bidTradePanel->highlightedSlot->id),
		ArtifactID(offerTradePanel->highlightedSlot->id), offerQty, hero);
	deselect();
}

CMarketBase::SelectionParams CArtifactsBuying::getSelectionParams() const
{
	if(bidTradePanel->highlightedSlot && offerTradePanel->highlightedSlot)
		return std::make_tuple(
			SelectionParamOneSide {std::to_string(deal->isBlocked() ? 0 : bidQty), bidTradePanel->highlightedSlot->id},
			SelectionParamOneSide {std::to_string(deal->isBlocked() ? 0 : offerQty), CGI->artifacts()->getByIndex(offerTradePanel->highlightedSlot->id)->getIconIndex()});
	else
		return std::make_tuple(std::nullopt, std::nullopt);
}

void CArtifactsBuying::highlightingChanged()
{
	if(bidTradePanel->highlightedSlot && offerTradePanel->highlightedSlot)
	{
		market->getOffer(bidTradePanel->highlightedSlot->id, offerTradePanel->highlightedSlot->id, bidQty, offerQty, EMarketMode::RESOURCE_ARTIFACT);
		deal->block(LOCPLINT->cb->getResourceAmount(GameResID(bidTradePanel->highlightedSlot->id)) >= bidQty ? false : true);
	}
	CMarketBase::highlightingChanged();
}
