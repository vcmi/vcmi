/*
 * CAltarCreatures.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "CAltarCreatures.h"

#include "../../gui/CGuiHandler.h"
#include "../../gui/Shortcut.h"
#include "../../widgets/Buttons.h"
#include "../../widgets/TextControls.h"

#include "../../CGameInfo.h"
#include "../../CPlayerInterface.h"

#include "../../../CCallback.h"

#include "../../../lib/texts/CGeneralTextHandler.h"
#include "../../../lib/mapObjects/CGHeroInstance.h"
#include "../../../lib/mapObjects/CGMarket.h"

CAltarCreatures::CAltarCreatures(const IMarket * market, const CGHeroInstance * hero)
	: CMarketBase(market, hero)
	, CMarketSlider(std::bind(&CAltarCreatures::onOfferSliderMoved, this, _1))
	, CMarketTraderText(Point(28, 31), FONT_MEDIUM, Colors::YELLOW)
{
	OBJECT_CONSTRUCTION;

	deal = std::make_shared<CButton>(dealButtonPosWithSlider, AnimationPath::builtin("ALTSACR.DEF"),
		CGI->generaltexth->zelp[584], [this]() {CAltarCreatures::makeDeal();}, EShortcut::MARKET_DEAL);
	labels.emplace_back(std::make_shared<CLabel>(155, 30, FONT_SMALL, ETextAlignment::CENTER, Colors::YELLOW,
		boost::str(boost::format(CGI->generaltexth->allTexts[272]) % hero->getNameTranslated())));
	labels.emplace_back(std::make_shared<CLabel>(450, 30, FONT_SMALL, ETextAlignment::CENTER, Colors::YELLOW, CGI->generaltexth->allTexts[479]));
	texts.emplace_back(std::make_unique<CTextBox>(CGI->generaltexth->allTexts[480], Rect(320, 56, 256, 40), 0, FONT_SMALL, ETextAlignment::CENTER, Colors::YELLOW));
	offerSlider->moveTo(pos.topLeft() + Point(231, 481));
	maxAmount->setHelp(CGI->generaltexth->zelp[578]);

	unitsOnAltar.resize(GameConstants::ARMY_SIZE, 0);
	expPerUnit.resize(GameConstants::ARMY_SIZE, 0);
	sacrificeAllButton = std::make_shared<CButton>(
		Point(393, 520), AnimationPath::builtin("ALTARMY.DEF"), CGI->generaltexth->zelp[579], std::bind(&CExperienceAltar::sacrificeAll, this), EShortcut::MARKET_SACRIFICE_ALL);

	// Hero creatures panel
	assert(bidTradePanel);
	bidTradePanel->moveTo(pos.topLeft() + Point(45, 110));
	bidTradePanel->showcaseSlot->moveTo(pos.topLeft() + Point(149, 422));
	bidTradePanel->showcaseSlot->subtitle->moveBy(Point(0, 3));
	for(const auto & slot : bidTradePanel->slots)
		slot->clickPressedCallback = [this](const std::shared_ptr<CTradeableItem> & heroSlot) {CAltarCreatures::onSlotClickPressed(heroSlot, bidTradePanel);};

	// Altar creatures panel
	offerTradePanel = std::make_shared<CreaturesPanel>([this](const std::shared_ptr<CTradeableItem> & altarSlot)
		{
			CAltarCreatures::onSlotClickPressed(altarSlot, offerTradePanel);
		}, bidTradePanel->slots);
	offerTradePanel->moveTo(pos.topLeft() + Point(334, 110));
	offerTradePanel->showcaseSlot->moveTo(pos.topLeft() + Point(395, 422));
	offerTradePanel->showcaseSlot->subtitle->moveBy(Point(0, 3));
	offerTradePanel->updateSlotsCallback = [this]()
	{
		for(const auto & altarSlot : offerTradePanel->slots)
			updateAltarSlot(altarSlot);
	};
	bidTradePanel->deleteSlotsCheck = offerTradePanel->deleteSlotsCheck = std::bind(&CCreaturesSelling::slotDeletingCheck, this, _1);
	
	readExpValues();
	CAltarCreatures::deselect();
};

void CAltarCreatures::readExpValues()
{
	int bidQty = 0;
	for(const auto & heroSlot : bidTradePanel->slots)
	{
		if(heroSlot->id >= 0)
			market->getOffer(heroSlot->id, 0, bidQty, expPerUnit[heroSlot->serial], EMarketMode::CREATURE_EXP);
	}
}

void CAltarCreatures::highlightingChanged()
{
	int sliderAmount = 0;
	if(bidTradePanel->isHighlighted())
	{
		std::optional<SlotID> lastSlot;
		for(auto slot = SlotID(0); slot.num < GameConstants::ARMY_SIZE; slot++)
		{
			if(hero->getStackCount(slot) > unitsOnAltar[slot.num])
			{
				if(lastSlot.has_value())
				{
					lastSlot = std::nullopt;
					break;
				}
				else
				{
					lastSlot = slot;
				}
			}
		}
		sliderAmount = hero->getStackCount(SlotID(bidTradePanel->highlightedSlot->serial));
		if(lastSlot.has_value() && lastSlot.value() == SlotID(bidTradePanel->highlightedSlot->serial))
			sliderAmount--;
	}
	offerSlider->setAmount(sliderAmount);
	offerSlider->block(!offerSlider->getAmount());
	if(bidTradePanel->isHighlighted())
		offerSlider->scrollTo(unitsOnAltar[bidTradePanel->highlightedSlot->serial]);
	maxAmount->block(offerSlider->getAmount() == 0);
	updateShowcases();
	CMarketTraderText::highlightingChanged();
}

void CAltarCreatures::update()
{
	CMarketBase::update();
	CExperienceAltar::update();
	assert(bidTradePanel->slots.size() == offerTradePanel->slots.size());
}

void CAltarCreatures::deselect()
{
	CMarketBase::deselect();
	CExperienceAltar::deselect();
	CMarketSlider::deselect();
	CMarketTraderText::deselect();
}

TExpType CAltarCreatures::calcExpAltarForHero()
{
	TExpType expOnAltar(0);
	auto oneUnitExp = expPerUnit.begin();
	for(const auto units : unitsOnAltar)
		expOnAltar += *oneUnitExp++ * units;
	auto resultExp = hero->calculateXp(expOnAltar);
	expForHero->setText(std::to_string(resultExp));
	return resultExp;
}

void CAltarCreatures::makeDeal()
{
	std::vector<TradeItemSell> ids;
	std::vector<ui32> toSacrifice;

	for(int i = 0; i < unitsOnAltar.size(); i++)
	{
		if(unitsOnAltar[i])
		{
			ids.push_back(SlotID(i));
			toSacrifice.push_back(unitsOnAltar[i]);
		}
	}

	LOCPLINT->cb->trade(market, EMarketMode::CREATURE_EXP, ids, {}, toSacrifice, hero);

	for(int & units : unitsOnAltar)
		units = 0;

	for(auto heroSlot : offerTradePanel->slots)
	{
		heroSlot->setType(EType::CREATURE_PLACEHOLDER);
		heroSlot->subtitle->clear();
	}
	deselect();
}

CMarketBase::MarketShowcasesParams CAltarCreatures::getShowcasesParams() const
{
	std::optional<ShowcaseParams> bidSelected = std::nullopt;
	std::optional<ShowcaseParams> offerSelected = std::nullopt;
	if(bidTradePanel->isHighlighted())
		bidSelected = ShowcaseParams {std::to_string(offerSlider->getValue()), CGI->creatures()->getByIndex(bidTradePanel->getSelectedItemId())->getIconIndex()};
	if(offerTradePanel->isHighlighted() && offerSlider->getValue() > 0)
		offerSelected = ShowcaseParams {offerTradePanel->highlightedSlot->subtitle->getText(), CGI->creatures()->getByIndex(offerTradePanel->getSelectedItemId())->getIconIndex()};
	return MarketShowcasesParams {bidSelected, offerSelected};
}

void CAltarCreatures::sacrificeAll()
{
	std::optional<SlotID> lastSlot;
	for(auto heroSlot : bidTradePanel->slots)
	{
		auto stackCount = hero->getStackCount(SlotID(heroSlot->serial));
		if(stackCount > unitsOnAltar[heroSlot->serial])
		{
			if(!lastSlot.has_value())
				lastSlot = SlotID(heroSlot->serial);
			unitsOnAltar[heroSlot->serial] = stackCount;
		}
	}
	if(hero->needsLastStack())
	{
		assert(lastSlot.has_value());
		unitsOnAltar[lastSlot.value().num]--;
	}

	if(offerTradePanel->isHighlighted())
		offerSlider->scrollTo(unitsOnAltar[offerTradePanel->highlightedSlot->serial]);
	offerTradePanel->update();
	updateShowcases();

	deal->block(calcExpAltarForHero() == 0 || !LOCPLINT->makingTurn);
}

void CAltarCreatures::updateAltarSlot(const std::shared_ptr<CTradeableItem> & slot)
{
	auto units = unitsOnAltar[slot->serial];
	slot->setType(units > 0 ? EType::CREATURE : EType::CREATURE_PLACEHOLDER);
	slot->subtitle->setText(units > 0 ?
		boost::str(boost::format(CGI->generaltexth->allTexts[122]) % std::to_string(hero->calculateXp(units * expPerUnit[slot->serial]))) : "");
}

void CAltarCreatures::onOfferSliderMoved(int newVal)
{
	if(bidTradePanel->isHighlighted())
		unitsOnAltar[bidTradePanel->highlightedSlot->serial] = newVal;
	if(offerTradePanel->isHighlighted())
		updateAltarSlot(offerTradePanel->highlightedSlot);
	deal->block(calcExpAltarForHero() == 0 || !LOCPLINT->makingTurn);
	highlightingChanged();
	redraw();
}

void CAltarCreatures::onSlotClickPressed(const std::shared_ptr<CTradeableItem> & newSlot, std::shared_ptr<TradePanelBase> & curPanel)
{
	assert(newSlot);
	assert(curPanel);
	if(newSlot == curPanel->highlightedSlot)
		return;

	auto oppositePanel = bidTradePanel;
	curPanel->onSlotClickPressed(newSlot);
	if(curPanel->highlightedSlot == bidTradePanel->highlightedSlot)
	{
		oppositePanel = offerTradePanel;
	}
	std::shared_ptr<CTradeableItem> oppositeNewSlot;
	for(const auto & slot : oppositePanel->slots)
		if(slot->serial == newSlot->serial)
		{
			oppositeNewSlot = slot;
			break;
		}
	assert(oppositeNewSlot);
	oppositePanel->onSlotClickPressed(oppositeNewSlot);
	highlightingChanged();
	redraw();
}

std::string CAltarCreatures::getTraderText()
{
	if(bidTradePanel->isHighlighted() && offerTradePanel->isHighlighted())
	{
		MetaString message = MetaString::createFromTextID("core.genrltxt.484");
		message.replaceNamePlural(CreatureID(bidTradePanel->getSelectedItemId()));
		return message.toString();
	}
	else
	{
		return "";
	}
}
