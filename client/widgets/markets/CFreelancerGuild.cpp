/*
 * CFreelancerGuild.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "CFreelancerGuild.h"

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
#include "../../../lib/mapObjects/CGTownInstance.h"

CFreelancerGuild::CFreelancerGuild(const IMarket * market, const CGHeroInstance * hero)
	: CTradeBase(market, hero)
	, CResourcesBuying([this](){CResourcesBuying::updateSubtitles(EMarketMode::CREATURE_RESOURCE);})
	, CMarketMisc([this](){return CFreelancerGuild::getSelectionParams();})
{
	OBJECT_CONSTRUCTION_CAPTURING(255 - DISPOSE);

	labels.emplace_back(std::make_shared<CLabel>(299, 27, FONT_BIG, ETextAlignment::CENTER, Colors::YELLOW,
		(*CGI->townh)[ETownType::STRONGHOLD]->town->buildings[BuildingID::FREELANCERS_GUILD]->getNameTranslated()));
	labels.emplace_back(std::make_shared<CLabel>(155, 103, FONT_SMALL, ETextAlignment::CENTER, Colors::WHITE,
		boost::str(boost::format(CGI->generaltexth->allTexts[272]) % hero->getNameTranslated())));
	deal = std::make_shared<CButton>(Point(306, 520), AnimationPath::builtin("TPMRKB.DEF"),
		CGI->generaltexth->zelp[595], [this]() {CFreelancerGuild::makeDeal();});
	maxAmount = std::make_shared<CButton>(Point(228, 520), AnimationPath::builtin("IRCBTNS.DEF"), CGI->generaltexth->zelp[596],
		[this]() {offerSlider->scrollToMax();});
	offerSlider = std::make_shared<CSlider>(Point(232, 489), 137, [this](int newVal)
		{
			CFreelancerGuild::onOfferSliderMoved(newVal);
		}, 0, 0, 0, Orientation::HORIZONTAL);

	// Hero creatures panel
	assert(leftTradePanel);
	leftTradePanel->moveTo(pos.topLeft() + Point(45, 123));
	leftTradePanel->deleteSlotsCheck = std::bind(&CCreaturesSelling::slotDeletingCheck, this, _1);
	std::for_each(leftTradePanel->slots.cbegin(), leftTradePanel->slots.cend(), [this](auto & slot)
		{
			slot->clickPressedCallback = [this](const std::shared_ptr<CTradeableItem> & heroSlot)
			{
				CFreelancerGuild::onSlotClickPressed(heroSlot, hLeft);
			};
		});

	// Guild resources panel
	assert(rightTradePanel);
	std::for_each(rightTradePanel->slots.cbegin(), rightTradePanel->slots.cend(), [this](auto & slot)
		{
			slot->clickPressedCallback = [this](const std::shared_ptr<CTradeableItem> & heroSlot)
			{
				CFreelancerGuild::onSlotClickPressed(heroSlot, hRight);
			};
		});

	CMarketMisc::deselect();
}

void CFreelancerGuild::makeDeal()
{
	if(auto toTrade = offerSlider->getValue(); toTrade != 0)
	{
		LOCPLINT->cb->trade(market, EMarketMode::CREATURE_RESOURCE, SlotID(hLeft->serial), GameResID(hRight->id), bidQty * toTrade, hero);
		deselect();
	}
}

CMarketMisc::SelectionParams CFreelancerGuild::getSelectionParams()
{
	if(hLeft && hRight)
		return std::make_tuple(std::to_string(bidQty * offerSlider->getValue()), std::to_string(offerQty * offerSlider->getValue()),
			CGI->creatures()->getByIndex(hLeft->id)->getIconIndex(), hRight->id);
	else
		return std::nullopt;
}

void CFreelancerGuild::onOfferSliderMoved(int newVal)
{
	if(hLeft && hRight)
	{
		offerSlider->scrollTo(newVal);
		updateSelected();
		redraw();
	}
}

void CFreelancerGuild::onSlotClickPressed(const std::shared_ptr<CTradeableItem> & newSlot, std::shared_ptr<CTradeableItem> & hCurSlot)
{
	if(newSlot == hCurSlot)
		return;

	CTradeBase::onSlotClickPressed(newSlot, hCurSlot);
	if(hLeft)
	{
		if(hRight)
		{
			market->getOffer(hLeft->id, hRight->id, bidQty, offerQty, EMarketMode::CREATURE_RESOURCE);
			offerSlider->setAmount((hero->getStackCount(SlotID(hLeft->serial)) - (hero->stacksCount() == 1 && hero->needsLastStack() ? 1 : 0)) / bidQty);
			offerSlider->scrollTo(0);
			offerSlider->block(false);
			maxAmount->block(false);
			deal->block(false);
		}
		updateSelected();
		rightTradePanel->updateSlots();
	}
	redraw();
}
