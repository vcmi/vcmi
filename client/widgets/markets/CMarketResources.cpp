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
#include "../../widgets/Slider.h"
#include "../../widgets/TextControls.h"

#include "../../CGameInfo.h"
#include "../../CPlayerInterface.h"

#include "../../../CCallback.h"

#include "../../../lib/CGeneralTextHandler.h"
#include "../../../lib/mapObjects/CGHeroInstance.h"
#include "../../../lib/mapObjects/CGMarket.h"

CMarketResources::CMarketResources(const IMarket * market, const CGHeroInstance * hero)
	: CTradeBase(market, hero)
	, CResourcesPurchasing([this](){CMarketResources::updateSubtitles();})
{
	OBJECT_CONSTRUCTION_CAPTURING(255 - DISPOSE);

	labels.emplace_back(std::make_shared<CLabel>(299, 27, FONT_BIG, ETextAlignment::CENTER, Colors::YELLOW, CGI->generaltexth->allTexts[158]));
	deal = std::make_shared<CButton>(Point(306, 520), AnimationPath::builtin("TPMRKB.DEF"),
		CGI->generaltexth->zelp[595], [this]() {CMarketResources::makeDeal(); });
	maxAmount = std::make_shared<CButton>(Point(228, 520), AnimationPath::builtin("IRCBTNS.DEF"), CGI->generaltexth->zelp[596],
		[this]() {offerSlider->scrollToMax(); });
	offerSlider = std::make_shared<CSlider>(Point(230, 489), 137, [this](int newVal)
		{
			CMarketResources::onOfferSliderMoved(newVal);
		}, 0, 0, 0, Orientation::HORIZONTAL);

	// Player's resources
	assert(leftTradePanel);
	std::for_each(leftTradePanel->slots.cbegin(), leftTradePanel->slots.cend(), [this](auto & slot)
		{
			slot->clickPressedCallback = [this](const std::shared_ptr<CTradeableItem> & heroSlot)
			{
				CMarketResources::onSlotClickPressed(heroSlot, hLeft);
			};
		});
	leftTradePanel->moveTo(pos.topLeft() + Point(39, 181));

	// Market resources panel
	assert(rightTradePanel);
	rightTradePanel->moveTo(pos.topLeft() + Point(327, 181));
	std::for_each(rightTradePanel->slots.cbegin(), rightTradePanel->slots.cend(), [this](auto & slot)
		{
			slot->clickPressedCallback = [this](const std::shared_ptr<CTradeableItem> & resSlot)
			{
				CMarketResources::onSlotClickPressed(resSlot, hRight);
			};
		});

	CResourcesSelling::updateSlots();
	CMarketResources::deselect();
}

void CMarketResources::makeDeal()
{
	if(auto toTrade = offerSlider->getValue(); toTrade != 0)
	{
		LOCPLINT->cb->trade(market, EMarketMode::RESOURCE_RESOURCE, GameResID(hLeft->id), GameResID(hRight->id), bidQty * toTrade, hero);
		deselect();
	}
}

void CMarketResources::deselect()
{
	CResourcesPurchasing::deselect();
	updateSelected();
}

void CMarketResources::updateSelected()
{
	std::optional<size_t> lImageIndex = std::nullopt;
	std::optional<size_t> rImageIndex = std::nullopt;

	if(hLeft && hRight && hLeft->id != hRight->id)
	{
		leftTradePanel->selectedSubtitle->setText(std::to_string(bidQty * offerSlider->getValue()));
		rightTradePanel->selectedSubtitle->setText(std::to_string(offerQty * offerSlider->getValue()));
		lImageIndex = hLeft->id;
		rImageIndex = hRight->id;
	}
	else
	{
		leftTradePanel->selectedSubtitle->setText("");
		rightTradePanel->selectedSubtitle->setText("");
	}
	leftTradePanel->setSelectedFrameIndex(lImageIndex);
	rightTradePanel->setSelectedFrameIndex(rImageIndex);
}

void CMarketResources::onOfferSliderMoved(int newVal)
{
	if(hLeft && hRight)
	{
		offerSlider->scrollTo(newVal);
		updateSelected();
		redraw();
	}
}

void CMarketResources::onSlotClickPressed(const std::shared_ptr<CTradeableItem> & newSlot, std::shared_ptr<CTradeableItem> & hCurSlot)
{
	if(newSlot == hCurSlot)
		return;

	CTradeBase::onSlotClickPressed(newSlot, hCurSlot);
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
		updateSelected();
		rightTradePanel->updateSlots();
	}
	redraw();
}

void CMarketResources::updateSubtitles()
{
	CResourcesPurchasing::updateSubtitles(EMarketMode::RESOURCE_RESOURCE);
	if(hLeft)
		rightTradePanel->slots[hLeft->serial]->subtitle = CGI->generaltexth->allTexts[164]; // n/a
}
