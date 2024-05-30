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
#include "../../gui/Shortcut.h"
#include "../../widgets/Buttons.h"
#include "../../widgets/TextControls.h"

#include "../../CGameInfo.h"
#include "../../CPlayerInterface.h"

#include "../../../CCallback.h"

#include "../../../lib/CGeneralTextHandler.h"
#include "../../../lib/MetaString.h"
#include "../../../lib/mapObjects/CGHeroInstance.h"
#include "../../../lib/mapObjects/CGMarket.h"

CFreelancerGuild::CFreelancerGuild(const IMarket * market, const CGHeroInstance * hero)
	: CMarketBase(market, hero)
	, CResourcesBuying(
		[this](const std::shared_ptr<CTradeableItem> & heroSlot){CFreelancerGuild::onSlotClickPressed(heroSlot, offerTradePanel);},
		[this](){CMarketBase::updateSubtitlesForBid(EMarketMode::CREATURE_RESOURCE, bidTradePanel->getSelectedItemId());})
	, CMarketSlider([this](int newVal){CMarketSlider::onOfferSliderMoved(newVal);})
{
	OBJECT_CONSTRUCTION_CAPTURING(255 - DISPOSE);

	labels.emplace_back(std::make_shared<CLabel>(titlePos.x, titlePos.y, FONT_BIG, ETextAlignment::CENTER, Colors::YELLOW,
		VLC->generaltexth->translate("object.core.freelancersGuild.name")));
	labels.emplace_back(std::make_shared<CLabel>(155, 103, FONT_SMALL, ETextAlignment::CENTER, Colors::WHITE,
		boost::str(boost::format(CGI->generaltexth->allTexts[272]) % hero->getNameTranslated())));
	deal = std::make_shared<CButton>(dealButtonPosWithSlider, AnimationPath::builtin("TPMRKB.DEF"),
		CGI->generaltexth->zelp[595], [this]() {CFreelancerGuild::makeDeal();}, EShortcut::MARKET_DEAL);
	offerSlider->moveTo(pos.topLeft() + Point(232, 489));

	// Hero creatures panel
	assert(bidTradePanel);
	bidTradePanel->moveTo(pos.topLeft() + Point(45, 123));
	bidTradePanel->showcaseSlot->subtitle->moveBy(Point(0, -1));
	bidTradePanel->deleteSlotsCheck = std::bind(&CCreaturesSelling::slotDeletingCheck, this, _1);
	std::for_each(bidTradePanel->slots.cbegin(), bidTradePanel->slots.cend(), [this](auto & slot)
		{
			slot->clickPressedCallback = [this](const std::shared_ptr<CTradeableItem> & heroSlot)
			{
				CFreelancerGuild::onSlotClickPressed(heroSlot, bidTradePanel);
			};
		});

	CFreelancerGuild::deselect();
}

void CFreelancerGuild::deselect()
{
	CMarketBase::deselect();
	CMarketSlider::deselect();
	CMarketTraderText::deselect();
}

void CFreelancerGuild::makeDeal()
{
	if(auto toTrade = offerSlider->getValue(); toTrade != 0)
	{
		LOCPLINT->cb->trade(market, EMarketMode::CREATURE_RESOURCE, SlotID(bidTradePanel->highlightedSlot->serial), GameResID(offerTradePanel->getSelectedItemId()), bidQty * toTrade, hero);
		CMarketTraderText::makeDeal();
		deselect();
	}
}

CMarketBase::MarketShowcasesParams CFreelancerGuild::getShowcasesParams() const
{
	if(bidTradePanel->isHighlighted() && offerTradePanel->isHighlighted())
		return MarketShowcasesParams
		{
			ShowcaseParams {std::to_string(bidQty * offerSlider->getValue()), CGI->creatures()->getByIndex(bidTradePanel->getSelectedItemId())->getIconIndex()},
			ShowcaseParams {std::to_string(offerQty * offerSlider->getValue()), offerTradePanel->getSelectedItemId()}
		};
	else
		return MarketShowcasesParams {std::nullopt, std::nullopt};
}

void CFreelancerGuild::highlightingChanged()
{
	if(bidTradePanel->isHighlighted() && offerTradePanel->isHighlighted())
	{
		market->getOffer(bidTradePanel->getSelectedItemId(), offerTradePanel->getSelectedItemId(), bidQty, offerQty, EMarketMode::CREATURE_RESOURCE);
		offerSlider->setAmount((hero->getStackCount(SlotID(bidTradePanel->highlightedSlot->serial)) - (hero->stacksCount() == 1 && hero->needsLastStack() ? 1 : 0)) / bidQty);
		offerSlider->scrollTo(0);
		offerSlider->block(false);
		maxAmount->block(false);
		deal->block(!LOCPLINT->makingTurn);
	}
	CMarketBase::highlightingChanged();
	CMarketTraderText::highlightingChanged();
}

std::string CFreelancerGuild::getTraderText()
{
	if(bidTradePanel->isHighlighted() && offerTradePanel->isHighlighted())
	{
		MetaString message = MetaString::createFromTextID("core.genrltxt.269");
		message.replaceNumber(offerQty);
		message.replaceRawString(offerQty == 1 ? CGI->generaltexth->allTexts[161] : CGI->generaltexth->allTexts[160]);
		message.replaceName(GameResID(offerTradePanel->getSelectedItemId()));
		message.replaceNumber(bidQty);
		if(bidQty == 1)
			message.replaceNameSingular(bidTradePanel->getSelectedItemId());
		else
			message.replaceNamePlural(bidTradePanel->getSelectedItemId());
		return message.toString();
	}
	else
	{
		return madeTransaction ? CGI->generaltexth->allTexts[162] : CGI->generaltexth->allTexts[163];
	}
}
