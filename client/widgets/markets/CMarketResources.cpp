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

#include "../../gui/CGuiHandler.h"
#include "../../widgets/Buttons.h"
#include "../../widgets/TextControls.h"

#include "../../CGameInfo.h"
#include "../../CPlayerInterface.h"

#include "../../../CCallback.h"

#include "../../../lib/CGeneralTextHandler.h"
#include "../../../lib/mapObjects/CGMarket.h"

CMarketResources::CMarketResources(const IMarket * market, const CGHeroInstance * hero)
	: CMarketBase(market, hero, [this](){return CMarketResources::getSelectionParams();})
	, CResourcesBuying(
		[this](const std::shared_ptr<CTradeableItem> & resSlot){CMarketResources::onSlotClickPressed(resSlot, hRight);},
		[this](){CMarketResources::updateSubtitles();})
	, CResourcesSelling([this](const std::shared_ptr<CTradeableItem> & heroSlot){CMarketResources::onSlotClickPressed(heroSlot, hLeft);})
	, CMarketSlider([this](int newVal){CMarketSlider::onOfferSliderMoved(newVal);})
{
	OBJECT_CONSTRUCTION_CAPTURING(255 - DISPOSE);

	labels.emplace_back(std::make_shared<CLabel>(titlePos.x, titlePos.y, FONT_BIG, ETextAlignment::CENTER, Colors::YELLOW, CGI->generaltexth->allTexts[158]));
	deal = std::make_shared<CButton>(dealButtonPosWithSlider, AnimationPath::builtin("TPMRKB.DEF"),
		CGI->generaltexth->zelp[595], [this]() {CMarketResources::makeDeal(); });

	// Player's resources
	assert(bidTradePanel);
	bidTradePanel->moveTo(pos.topLeft() + Point(39, 182));

	// Market resources panel
	assert(offerTradePanel);
	std::for_each(offerTradePanel->slots.cbegin(), offerTradePanel->slots.cend(), [this](auto & slot)
		{
			slot->clickPressedCallback = [this](const std::shared_ptr<CTradeableItem> & resSlot)
			{
				CMarketResources::onSlotClickPressed(resSlot, hRight);
			};
		});

	CMarketBase::update();
	CMarketResources::deselect();
}

void CMarketResources::deselect()
{
	CMarketBase::deselect();
	CMarketSlider::deselect();
}

void CMarketResources::makeDeal()
{
	if(auto toTrade = offerSlider->getValue(); toTrade != 0)
	{
		LOCPLINT->cb->trade(market, EMarketMode::RESOURCE_RESOURCE, GameResID(hLeft->id), GameResID(hRight->id), bidQty * toTrade, hero);
		deselect();
	}
}

CMarketBase::SelectionParams CMarketResources::getSelectionParams() const
{
	if(hLeft && hRight && hLeft->id != hRight->id)
		return std::make_tuple(
			SelectionParamOneSide {std::to_string(bidQty * offerSlider->getValue()), hLeft->id},
			SelectionParamOneSide {std::to_string(offerQty * offerSlider->getValue()), hRight->id});
	else
		return std::make_tuple(std::nullopt, std::nullopt);
}

void CMarketResources::highlightingChanged()
{
	if(hLeft)
	{
		if(hRight)
		{
			market->getOffer(hLeft->id, hRight->id, bidQty, offerQty, EMarketMode::RESOURCE_RESOURCE);
			offerSlider->setAmount(LOCPLINT->cb->getResourceAmount(GameResID(hLeft->id)) / bidQty);
			offerSlider->scrollTo(0);
			const bool isControlsBlocked = hLeft->id != hRight->id ? false : true;
			offerSlider->block(isControlsBlocked);
			maxAmount->block(isControlsBlocked);
			deal->block(isControlsBlocked);
		}
		offerTradePanel->update();
	}
	updateSelected();
}

void CMarketResources::onSlotClickPressed(const std::shared_ptr<CTradeableItem> & newSlot, std::shared_ptr<CTradeableItem> & hCurSlot)
{
	assert(newSlot);
	if(newSlot == hCurSlot)
		return;

	CMarketBase::onSlotClickPressed(newSlot, hCurSlot);
	highlightingChanged();
	redraw();
}

void CMarketResources::updateSubtitles()
{
	CMarketBase::updateSubtitles(EMarketMode::RESOURCE_RESOURCE);
	if(hLeft)
		offerTradePanel->slots[hLeft->serial]->subtitle->setText(CGI->generaltexth->allTexts[164]); // n/a
}
