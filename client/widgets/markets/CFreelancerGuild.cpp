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
	, CResourcesMarket(EMarketMode::CREATURE_RESOURCE)
{
	OBJECT_CONSTRUCTION_CAPTURING(255 - DISPOSE);

	labels.emplace_back(std::make_shared<CLabel>(254, -96, FONT_BIG, ETextAlignment::CENTER, Colors::YELLOW,
		(*CGI->townh)[ETownType::STRONGHOLD]->town->buildings[BuildingID::FREELANCERS_GUILD]->getNameTranslated()));
	labels.emplace_back(std::make_shared<CLabel>(110, -20, FONT_SMALL, ETextAlignment::CENTER, Colors::WHITE,
		boost::str(boost::format(CGI->generaltexth->allTexts[272]) % hero->getNameTranslated())));
	deal = std::make_shared<CButton>(Point(262, 397), AnimationPath::builtin("ALTSACR.DEF"),
		CGI->generaltexth->zelp[595], [this]() {CFreelancerGuild::makeDeal();});
	deal->block(true);
	maxUnits = std::make_shared<CButton>(Point(183, 397), AnimationPath::builtin("IRCBTNS.DEF"), CGI->generaltexth->zelp[596],
		[this]() {offerSlider->scrollToMax();});
	maxUnits->block(true);
	lSubtitle = std::make_shared<CLabel>(113, 403, FONT_SMALL, ETextAlignment::CENTER, Colors::WHITE);
	rSubtitle = std::make_shared<CLabel>(400, 382, FONT_SMALL, ETextAlignment::CENTER, Colors::WHITE);

	// Hero creatures panel
	assert(leftTradePanel);
	leftTradePanel->deleteSlotsCheck = [this, hero](const std::shared_ptr<CTradeableItem> & slot)
	{
		return hero->getStackCount(SlotID(slot->serial)) == 0 ? true : false;
	};
	std::for_each(leftTradePanel->slots.cbegin(), leftTradePanel->slots.cend(), [this](auto & slot)
		{
			slot->clickPressedCallback = [this](const std::shared_ptr<CTradeableItem> & heroSlot)
			{
				CFreelancerGuild::onSlotClickPressed(heroSlot, hLeft);
			};
		});
	leftTradePanel->selectedImage->moveTo(pos.topLeft() + Point(83, 327));

	// Guild resources panel
	assert(rightTradePanel);
	rightTradePanel->moveBy(Point(282, 58));
	std::for_each(rightTradePanel->slots.cbegin(), rightTradePanel->slots.cend(), [this](auto & slot)
		{
			slot->clickPressedCallback = [this](const std::shared_ptr<CTradeableItem> & heroSlot)
			{
				CFreelancerGuild::onSlotClickPressed(heroSlot, hRight);
			};
		});
	rightTradePanel->selectedImage->moveTo(pos.topLeft() + Point(383, 335));

	offerSlider = std::make_shared<CSlider>(Point(187, 366), 137, [this](int newVal)
		{
			CFreelancerGuild::onOfferSliderMoved(newVal);
		}, 0, 0, 0, Orientation::HORIZONTAL);

	CFreelancerGuild::deselect();
}

void CFreelancerGuild::updateSelected()
{
	std::optional<size_t> lImageIndex = std::nullopt;
	std::optional<size_t> rImageIndex = std::nullopt;
	
	if(hLeft && hRight)
	{
		lSubtitle->setText(std::to_string(qtyPerPrice * offerSlider->getValue()));
		rSubtitle->setText(std::to_string(price * offerSlider->getValue()));
		lImageIndex = CGI->creatures()->getByIndex(hLeft->id)->getIconIndex();
		rImageIndex = hRight->id;
	}
	else
	{
		lSubtitle->setText("");
		rSubtitle->setText("");
	}
	leftTradePanel->setSelectedFrameIndex(lImageIndex);
	rightTradePanel->setSelectedFrameIndex(rImageIndex);
}

void CFreelancerGuild::makeDeal()
{
	LOCPLINT->cb->trade(market, EMarketMode::CREATURE_RESOURCE, SlotID(hLeft->serial), GameResID(hRight->id), qtyPerPrice * offerSlider->getValue(), hero);
	deselect();
}

void CFreelancerGuild::deselect()
{
	CTradeBase::deselect();
	maxUnits->block(true);
	updateSelected();
	qtyPerPrice = 0;
	price = 0;
	offerSlider->scrollTo(0);
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
	CTradeBase::onSlotClickPressed(newSlot, hCurSlot);
	
	if(hLeft)
	{
		if(hRight)
		{
			market->getOffer(hLeft->id, hRight->id, qtyPerPrice, price, EMarketMode::CREATURE_RESOURCE);
			offerSlider->setAmount((hero->getStackCount(SlotID(hLeft->serial)) - (hero->stacksCount() == 1 && hero->needsLastStack() ? 1 : 0)) / qtyPerPrice);
			offerSlider->scrollTo(0);
			maxUnits->block(false);
			deal->block(false);
		}
		updateSelected();
		rightTradePanel->updateSlots();
	}
	redraw();
}
