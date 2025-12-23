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

#include "../../GameEngine.h"
#include "../../GameInstance.h"
#include "../../gui/Shortcut.h"
#include "../../widgets/Buttons.h"
#include "../../widgets/TextControls.h"

#include "../../CPlayerInterface.h"

#include "../../../lib/GameLibrary.h"
#include "../../../lib/callback/CCallback.h"
#include "../../../lib/mapObjects/CGHeroInstance.h"
#include "../../../lib/mapObjects/IMarket.h"
#include "../../../lib/texts/CGeneralTextHandler.h"

CFreelancerGuild::CFreelancerGuild(const IMarket * market, const CGHeroInstance * hero)
	: CMarketBase(market, hero)
	, CResourcesBuying(
		[this](const std::shared_ptr<CTradeableItem> & heroSlot){CFreelancerGuild::onSlotClickPressed(heroSlot, offerTradePanel);},
		[this](){CMarketBase::updateSubtitlesForBid(EMarketMode::CREATURE_RESOURCE, bidTradePanel->getHighlightedItemId());})
	, CMarketSlider([this](int newVal){CMarketSlider::onOfferSliderMoved(newVal);})
{
	OBJECT_CONSTRUCTION;

	labels.emplace_back(std::make_shared<CLabel>(titlePos.x, titlePos.y, FONT_BIG, ETextAlignment::CENTER, Colors::YELLOW,
		LIBRARY->generaltexth->translate("object.core.freelancersGuild.name")));
	labels.emplace_back(std::make_shared<CLabel>(155, 103, FONT_SMALL, ETextAlignment::CENTER, Colors::WHITE,
		boost::str(boost::format(LIBRARY->generaltexth->allTexts[272]) % hero->getNameTranslated())));
	deal = std::make_shared<CButton>(dealButtonPosWithSlider, AnimationPath::builtin("TPMRKB.DEF"),
		LIBRARY->generaltexth->zelp[595], [this]() {CFreelancerGuild::makeDeal();}, EShortcut::MARKET_DEAL);
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
		GAME->interface()->cb->trade(market->getObjInstanceID(), EMarketMode::CREATURE_RESOURCE, SlotID(bidTradePanel->highlightedSlot->serial), GameResID(offerTradePanel->getHighlightedItemId()), bidQty * toTrade, hero);
		CMarketTraderText::makeDeal();
		deselect();
	}
}

CMarketBase::MarketShowcasesParams CFreelancerGuild::getShowcasesParams() const
{
	if(bidTradePanel->isHighlighted() && offerTradePanel->isHighlighted())
		return MarketShowcasesParams
		{
			ShowcaseParams {std::to_string(bidQty * offerSlider->getValue()), LIBRARY->creatures()->getByIndex(bidTradePanel->getHighlightedItemId())->getIconIndex()},
			ShowcaseParams {std::to_string(offerQty * offerSlider->getValue()), offerTradePanel->getHighlightedItemId()}
		};
	else
		return MarketShowcasesParams {std::nullopt, std::nullopt};
}

void CFreelancerGuild::highlightingChanged()
{
	if(bidTradePanel->isHighlighted() && offerTradePanel->isHighlighted())
	{
		market->getOffer(bidTradePanel->getHighlightedItemId(), offerTradePanel->getHighlightedItemId(), bidQty, offerQty, EMarketMode::CREATURE_RESOURCE);
		offerSlider->setAmount((hero->getStackCount(SlotID(bidTradePanel->highlightedSlot->serial)) - (hero->stacksCount() == 1 && hero->needsLastStack() ? 1 : 0)) / bidQty);
		offerSlider->scrollTo(0);
		offerSlider->block(false);
		maxAmount->block(false);
		deal->block(!GAME->interface()->makingTurn);
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
		message.replaceRawString(offerQty == 1 ? LIBRARY->generaltexth->allTexts[161] : LIBRARY->generaltexth->allTexts[160]);
		message.replaceName(GameResID(offerTradePanel->getHighlightedItemId()));
		message.replaceNumber(bidQty);
		if(bidQty == 1)
			message.replaceNameSingular(bidTradePanel->getHighlightedItemId());
		else
			message.replaceNamePlural(bidTradePanel->getHighlightedItemId());
		return message.toString();
	}
	else
	{
		return madeTransaction ? LIBRARY->generaltexth->allTexts[162] : LIBRARY->generaltexth->allTexts[163];
	}
}
