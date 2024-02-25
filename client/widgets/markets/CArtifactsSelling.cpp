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
#include "../../widgets/Buttons.h"
#include "../../widgets/Slider.h"
#include "../../widgets/TextControls.h"

#include "../../CGameInfo.h"

#include "../../../lib/CArtifactInstance.h"
#include "../../../lib/CGeneralTextHandler.h"
#include "../../../lib/mapObjects/CGMarket.h"

CArtifactsSelling::CArtifactsSelling(const IMarket * market, const CGHeroInstance * hero)
	: CTradeBase(market, hero)
	, CResourcesBuying([this](){CArtifactsSelling::updateSubtitles();})
	, CMarketMisc([this](){return CArtifactsSelling::getSelectionParams();})
{
	OBJECT_CONSTRUCTION_CUSTOM_CAPTURING(255 - DISPOSE);

	deal = std::make_shared<CButton>(Point(270, 520), AnimationPath::builtin("TPMRKB.DEF"),
		CGI->generaltexth->zelp[595], [this](){CArtifactsSelling::makeDeal();});
	selectedImage = std::make_shared<CAnimImage>(AnimationPath::builtin("artifact"), 10, 0, 136, 470);
	bidSelectedSubtitle = std::make_shared<CLabel>(157, 529, FONT_SMALL, ETextAlignment::CENTER, Colors::WHITE);

	// Hero's artifacts
	heroArts = std::make_shared<CArtifactsOfHeroMarket>(Point(-361, 46));
	heroArts->setHero(hero);
	heroArts->selectArtCallback = [this](CArtPlace * artPlace)
	{
		assert(artPlace);
		artForTrade = artPlace->getArt();
		assert(artForTrade.has_value());
		if(hRight)
		{
			this->market->getOffer(artForTrade.value()->getTypeId().num, hRight->id, bidQty, offerQty, EMarketMode::ARTIFACT_RESOURCE);
			deal->block(false);
		}
		CArtifactsSelling::updateSelected();
		offerTradePanel->updateSlots();
		redraw();
	};

	// Market resources panel
	assert(offerTradePanel);
	std::for_each(offerTradePanel->slots.cbegin(), offerTradePanel->slots.cend(), [this](auto & slot)
		{
			slot->clickPressedCallback = [this](const std::shared_ptr<CTradeableItem> & resSlot)
			{
				CArtifactsSelling::onSlotClickPressed(resSlot, hRight);
			};
		});
	offerTradePanel->selectedImage->moveTo(pos.topLeft() + Point(428, 479));
	offerTradePanel->selectedSubtitle->moveTo(pos.topLeft() + Point(445, 526));
	offerTradePanel->moveTo(pos.topLeft() + Point(326, 183));
	

	CArtifactsSelling::updateSelected();
	CArtifactsSelling::deselect();
}

void CArtifactsSelling::makeDeal()
{

}

void CArtifactsSelling::deselect()
{
	CTradeBase::deselect();
	artForTrade = std::nullopt;
}

void CArtifactsSelling::updateSelected()
{
	CMarketMisc::updateSelected();

	if(artForTrade.has_value() && hRight)
	{
		bidSelectedSubtitle->setText(std::to_string(bidQty));
		selectedImage->enable();
		selectedImage->setFrame(CGI->artifacts()->getByIndex(artForTrade.value()->getTypeId())->getIconIndex());
	}
	else
	{
		selectedImage->disable();
		selectedImage->setFrame(0);
		bidSelectedSubtitle->setText("");
	}
}

std::shared_ptr<CArtifactsOfHeroMarket> CArtifactsSelling::getAOHset() const
{
	return heroArts;
}

CMarketMisc::SelectionParams CArtifactsSelling::getSelectionParams()
{
	if(artForTrade.has_value() && hRight)
		return std::make_tuple(
			std::nullopt,
			SelectionParamOneSide {std::to_string(offerQty), GameResID(hRight->id)}
		);
	else
		return std::make_tuple(std::nullopt, std::nullopt);
}

void CArtifactsSelling::updateSubtitles()
{
	if(artForTrade.has_value())
		for(const auto & slot : offerTradePanel->slots)
		{
			int bidQty = 0;
			int offerQty = 0;
			market->getOffer(artForTrade.value()->getTypeId().num, slot->id, bidQty, offerQty, EMarketMode::ARTIFACT_RESOURCE);
			offerTradePanel->updateOffer(*slot, bidQty, offerQty);
		}
	else
		offerTradePanel->clearSubtitles();
}

void CArtifactsSelling::onSlotClickPressed(const std::shared_ptr<CTradeableItem> & newSlot, std::shared_ptr<CTradeableItem> & hCurSlot)
{
	assert(newSlot);
	if(newSlot == hCurSlot)
		return;

	CTradeBase::onSlotClickPressed(newSlot, hCurSlot);
	if(artForTrade.has_value())
	{
		if(hRight)
		{
			market->getOffer(artForTrade.value()->getTypeId().num, hRight->id, bidQty, offerQty, EMarketMode::ARTIFACT_RESOURCE);
			deal->block(false);
		}
		updateSelected();
	}
	redraw();
}
