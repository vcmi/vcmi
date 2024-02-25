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
#include "../../widgets/Slider.h"
#include "../../widgets/TextControls.h"

#include "../../CGameInfo.h"
#include "../../CPlayerInterface.h"

#include "../../../CCallback.h"

#include "../../../lib/CGeneralTextHandler.h"
#include "../../../lib/mapObjects/CGMarket.h"
#include "../../../lib/mapObjects/CGTownInstance.h"

CArtifactsBuying::CArtifactsBuying(const IMarket * market, const CGHeroInstance * hero)
	: CTradeBase(market, hero)
	, CMarketMisc([this](){return CArtifactsBuying::getSelectionParams();})
{
	OBJECT_CONSTRUCTION_CUSTOM_CAPTURING(255 - DISPOSE);

	assert(dynamic_cast<const CGTownInstance*>(market));
	labels.emplace_back(std::make_shared<CLabel>(299, 27, FONT_BIG, ETextAlignment::CENTER, Colors::YELLOW,
		(*CGI->townh)[dynamic_cast<const CGTownInstance*>(market)->getFaction()]->town->buildings[BuildingID::ARTIFACT_MERCHANT]->getNameTranslated()));
	deal = std::make_shared<CButton>(Point(270, 520), AnimationPath::builtin("TPMRKB.DEF"),
		CGI->generaltexth->zelp[595], [this]() {CArtifactsBuying::makeDeal(); });
	labels.emplace_back(std::make_shared<CLabel>(445, 148, FONT_SMALL, ETextAlignment::CENTER, Colors::WHITE, CGI->generaltexth->allTexts[168]));

	// Player's resources
	assert(bidTradePanel);
	std::for_each(bidTradePanel->slots.cbegin(), bidTradePanel->slots.cend(), [this](auto & slot)
		{
			slot->clickPressedCallback = [this](const std::shared_ptr<CTradeableItem> & heroSlot)
			{
				CArtifactsBuying::onSlotClickPressed(heroSlot, hLeft);
			};
		});
	bidTradePanel->moveTo(pos.topLeft() + Point(39, 182));
	bidTradePanel->selectedImage->moveTo(pos.topLeft() + Point(141, 453));

	// Artifacts panel
	offerTradePanel = std::make_shared<ArtifactsPanel>([this](const std::shared_ptr<CTradeableItem> & newSlot)
		{
			CArtifactsBuying::onSlotClickPressed(newSlot, hRight);
		
		}, [this]()
		{
			// TODO move to parent
			if(hLeft)
				for(const auto & slot : offerTradePanel->slots)
				{
					int h1, h2; //hlp variables for getting offer
					this->market->getOffer(hLeft->id, slot->id, h1, h2, EMarketMode::RESOURCE_ARTIFACT);

					offerTradePanel->updateOffer(*slot, h1, h2);
				}
			else
				offerTradePanel->clearSubtitles();
		}, market->availableItemsIds(EMarketMode::RESOURCE_ARTIFACT));
	offerTradePanel->deleteSlotsCheck = [this](const std::shared_ptr<CTradeableItem> & slot)
	{
		return vstd::contains(this->market->availableItemsIds(EMarketMode::RESOURCE_ARTIFACT), ArtifactID(slot->id)) ? false : true;
	};
	offerTradePanel->moveTo(pos.topLeft() + Point(328, 182));

	CArtifactsBuying::updateSlots();
	CMarketMisc::deselect();
}

void CArtifactsBuying::makeDeal()
{
	LOCPLINT->cb->trade(market, EMarketMode::RESOURCE_ARTIFACT, GameResID(hLeft->id), ArtifactID(hRight->id), offerQty, hero);
	deselect();
}

void CArtifactsBuying::updateSlots()
{
	CResourcesSelling::updateSlots();
	offerTradePanel->updateSlots();
}

CMarketMisc::SelectionParams CArtifactsBuying::getSelectionParams()
{
	if(hLeft && hRight)
		return std::make_tuple(
			SelectionParamOneSide {std::to_string(deal->isBlocked() ? 0 : bidQty), hLeft->id},
			SelectionParamOneSide {std::to_string(deal->isBlocked() ? 0 : offerQty), CGI->artifacts()->getByIndex(hRight->id)->getIconIndex()});
	else
		return std::make_tuple(std::nullopt, std::nullopt);
}

void CArtifactsBuying::onSlotClickPressed(const std::shared_ptr<CTradeableItem> & newSlot, std::shared_ptr<CTradeableItem> & hCurSlot)
{
	if(newSlot == hCurSlot)
		return;

	CTradeBase::onSlotClickPressed(newSlot, hCurSlot);
	if(hLeft)
	{
		if(hRight)
		{
			market->getOffer(hLeft->id, hRight->id, bidQty, offerQty, EMarketMode::RESOURCE_ARTIFACT);
			deal->block(LOCPLINT->cb->getResourceAmount(GameResID(hLeft->id)) >= bidQty ? false : true);
		}
		updateSelected();
		offerTradePanel->updateSlots();
	}
	redraw();
}
