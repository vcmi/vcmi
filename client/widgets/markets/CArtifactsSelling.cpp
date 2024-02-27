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
	, CResourcesBuying(
		[this](const std::shared_ptr<CTradeableItem> & resSlot){CArtifactsSelling::onSlotClickPressed(resSlot, hRight);},
		[this](){CTradeBase::updateSubtitles(EMarketMode::ARTIFACT_RESOURCE);})
	, CMarketMisc([this](){return CArtifactsSelling::getSelectionParams();})
{
	OBJECT_CONSTRUCTION_CUSTOM_CAPTURING(255 - DISPOSE);

	deal = std::make_shared<CButton>(Point(270, 520), AnimationPath::builtin("TPMRKB.DEF"),
		CGI->generaltexth->zelp[595], [this](){CArtifactsSelling::makeDeal();});
	bidSelectedSlot = std::make_shared<CTradeableItem>(Rect(Point(123, 470), Point(69, 66)), EType::ARTIFACT_TYPE, 0, 0);

	// Hero's artifacts
	heroArts = std::make_shared<CArtifactsOfHeroMarket>(Point(-361, 46));
	heroArts->setHero(hero);
	heroArts->selectArtCallback = [this](CArtPlace * artPlace)
	{
		assert(artPlace);
		const auto artForTrade = artPlace->getArt();
		assert(artForTrade);
		bidSelectedSlot->setID(artForTrade->getTypeId().num);
		hLeft = bidSelectedSlot;
		if(hRight)
		{	// dublicate
			this->market->getOffer(hLeft->id, hRight->id, bidQty, offerQty, EMarketMode::ARTIFACT_RESOURCE);
			deal->block(false);
		}
		CArtifactsSelling::updateSelected();
		offerTradePanel->updateSlots();
		redraw();
	};

	// Market resources panel
	assert(offerTradePanel);
	offerTradePanel->moveTo(pos.topLeft() + Point(326, 184));
	offerTradePanel->selectedSlot->moveTo(pos.topLeft() + Point(409, 473));
	
	CArtifactsSelling::updateSelected();
	CMarketMisc::deselect();
}

void CArtifactsSelling::makeDeal()
{

}

void CArtifactsSelling::updateSelected()
{
	CMarketMisc::updateSelected();

	if(hLeft && hRight)
	{
		bidSelectedSlot->image->enable();
		bidSelectedSlot->image->setFrame(CGI->artifacts()->getByIndex(hLeft->id)->getIconIndex());
		bidSelectedSlot->subtitle->setText(std::to_string(bidQty));
	}
	else
	{
		bidSelectedSlot->image->disable();
		bidSelectedSlot->image->setFrame(0);
		bidSelectedSlot->subtitle->clear();
	}
}

std::shared_ptr<CArtifactsOfHeroMarket> CArtifactsSelling::getAOHset() const
{
	return heroArts;
}

CMarketMisc::SelectionParams CArtifactsSelling::getSelectionParams()
{
	if(hLeft && hRight)
		return std::make_tuple(
			std::nullopt,
			SelectionParamOneSide {std::to_string(offerQty), GameResID(hRight->id)}
		);
	else
		return std::make_tuple(std::nullopt, std::nullopt);
}

void CArtifactsSelling::onSlotClickPressed(const std::shared_ptr<CTradeableItem> & newSlot, std::shared_ptr<CTradeableItem> & hCurSlot)
{
	assert(newSlot);
	if(newSlot == hCurSlot)
		return;

	CTradeBase::onSlotClickPressed(newSlot, hCurSlot);
	if(hLeft)
	{
		if(hRight)
		{
			market->getOffer(hLeft->id, hRight->id, bidQty, offerQty, EMarketMode::ARTIFACT_RESOURCE);
			deal->block(false);
		}
		updateSelected();
	}
	redraw();
}
