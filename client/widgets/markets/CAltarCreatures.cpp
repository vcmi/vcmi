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
#include "../../widgets/Buttons.h"
#include "../../widgets/Slider.h"
#include "../../widgets/TextControls.h"

#include "../../CGameInfo.h"
#include "../../CPlayerInterface.h"

#include "../../../CCallback.h"

#include "../../../lib/CGeneralTextHandler.h"
#include "../../../lib/mapObjects/CGHeroInstance.h"
#include "../../../lib/mapObjects/CGMarket.h"

CAltarCreatures::CAltarCreatures(const IMarket * market, const CGHeroInstance * hero)
	: CTradeBase(market, hero, [this](){return CAltarCreatures::getSelectionParams();})
{
	OBJECT_CONSTRUCTION_CUSTOM_CAPTURING(255 - DISPOSE);

	deal = std::make_shared<CButton>(dealButtonPos, AnimationPath::builtin("ALTSACR.DEF"),
		CGI->generaltexth->zelp[584], [this]() {CAltarCreatures::makeDeal();});
	labels.emplace_back(std::make_shared<CLabel>(155, 30, FONT_SMALL, ETextAlignment::CENTER, Colors::YELLOW,
		boost::str(boost::format(CGI->generaltexth->allTexts[272]) % hero->getNameTranslated())));
	labels.emplace_back(std::make_shared<CLabel>(450, 30, FONT_SMALL, ETextAlignment::CENTER, Colors::YELLOW, CGI->generaltexth->allTexts[479]));
	texts.emplace_back(std::make_unique<CTextBox>(CGI->generaltexth->allTexts[480], Rect(320, 56, 256, 40), 0, FONT_SMALL, ETextAlignment::CENTER, Colors::YELLOW));
	offerSlider = std::make_shared<CSlider>(Point(231, 481), 137, std::bind(&CAltarCreatures::onOfferSliderMoved, this, _1), 0, 0, 0, Orientation::HORIZONTAL);
	maxAmount = std::make_shared<CButton>(Point(147, 520), AnimationPath::builtin("IRCBTNS.DEF"), CGI->generaltexth->zelp[578], std::bind(&CSlider::scrollToMax, offerSlider));

	unitsOnAltar.resize(GameConstants::ARMY_SIZE, 0);
	expPerUnit.resize(GameConstants::ARMY_SIZE, 0);
	sacrificeAllButton = std::make_shared<CButton>(
		Point(393, 520), AnimationPath::builtin("ALTARMY.DEF"), CGI->generaltexth->zelp[579], std::bind(&CExperienceAltar::sacrificeAll, this));

	// Hero creatures panel
	assert(bidTradePanel);
	bidTradePanel->moveTo(pos.topLeft() + Point(45, 110));
	bidTradePanel->selectedSlot->moveTo(pos.topLeft() + Point(149, 422));
	bidTradePanel->selectedSlot->subtitle->moveBy(Point(0, 3));
	for(const auto & slot : bidTradePanel->slots)
		slot->clickPressedCallback = [this](const std::shared_ptr<CTradeableItem> & heroSlot) {CAltarCreatures::onSlotClickPressed(heroSlot, hLeft);};

	// Altar creatures panel
	offerTradePanel = std::make_shared<CreaturesPanel>([this](const std::shared_ptr<CTradeableItem> & altarSlot)
		{
			CAltarCreatures::onSlotClickPressed(altarSlot, hRight);
		}, bidTradePanel->slots);
	offerTradePanel->moveTo(pos.topLeft() + Point(334, 110));
	offerTradePanel->selectedSlot->moveTo(pos.topLeft() + Point(395, 422));
	offerTradePanel->selectedSlot->subtitle->moveBy(Point(0, 3));
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

void CAltarCreatures::updateControls()
{
	int sliderAmount = 0;
	if(hLeft)
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
		sliderAmount = hero->getStackCount(SlotID(hLeft->serial));
		if(lastSlot.has_value() && lastSlot.value() == SlotID(hLeft->serial))
			sliderAmount--;
	}
	offerSlider->setAmount(sliderAmount);
	offerSlider->block(!offerSlider->getAmount());
	if(hLeft)
		offerSlider->scrollTo(unitsOnAltar[hLeft->serial]);
	maxAmount->block(offerSlider->getAmount() == 0);
}

void CAltarCreatures::updateSlots()
{
	CTradeBase::updateSlots();
	assert(bidTradePanel->slots.size() == offerTradePanel->slots.size());
}

void CAltarCreatures::deselect()
{
	CTradeBase::deselect();
	updateSelected();
	expForHero->setText(std::to_string(0));
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
	deselect();

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
}

CTradeBase::SelectionParams CAltarCreatures::getSelectionParams() const
{
	std::optional<SelectionParamOneSide> bidSelected = std::nullopt;
	std::optional<SelectionParamOneSide> offerSelected = std::nullopt;
	if(hLeft)
		bidSelected = SelectionParamOneSide {std::to_string(offerSlider->getValue()), CGI->creatures()->getByIndex(hLeft->id)->getIconIndex()};
	if(hRight && offerSlider->getValue() > 0)
		offerSelected = SelectionParamOneSide {hRight->subtitle->getText(), CGI->creatures()->getByIndex(hRight->id)->getIconIndex()};
	return std::make_tuple(bidSelected, offerSelected);
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

	if(hRight)
		offerSlider->scrollTo(unitsOnAltar[hRight->serial]);
	offerTradePanel->updateSlots();
	updateSelected();

	deal->block(calcExpAltarForHero() == 0);
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
	if(hLeft)
		unitsOnAltar[hLeft->serial] = newVal;
	if(hRight)
		updateAltarSlot(hRight);
	deal->block(calcExpAltarForHero() == 0);
	updateControls();
	updateSelected();
	redraw();
}

void CAltarCreatures::onSlotClickPressed(const std::shared_ptr<CTradeableItem> & newSlot, std::shared_ptr<CTradeableItem> & hCurSlot)
{
	if(hCurSlot == newSlot)
		return;

	auto * oppositeSlot = &hLeft;
	auto oppositePanel = bidTradePanel;
	CTradeBase::onSlotClickPressed(newSlot, hCurSlot);
	if(hCurSlot == hLeft)
	{
		oppositeSlot = &hRight;
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
	CTradeBase::onSlotClickPressed(oppositeNewSlot, *oppositeSlot);
	updateControls();
	updateSelected();
	redraw();
}
