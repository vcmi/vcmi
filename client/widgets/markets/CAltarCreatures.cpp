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
	: CTradeBase(market, hero)
{
	OBJECT_CONSTRUCTION_CUSTOM_CAPTURING(255 - DISPOSE);

	deal = std::make_shared<CButton>(dealButtonPos, AnimationPath::builtin("ALTSACR.DEF"),
		CGI->generaltexth->zelp[584], [this]() {CAltarCreatures::makeDeal();});
	labels.emplace_back(std::make_shared<CLabel>(155, 30, FONT_SMALL, ETextAlignment::CENTER, Colors::YELLOW,
		boost::str(boost::format(CGI->generaltexth->allTexts[272]) % hero->getNameTranslated())));
	labels.emplace_back(std::make_shared<CLabel>(450, 30, FONT_SMALL, ETextAlignment::CENTER, Colors::YELLOW, CGI->generaltexth->allTexts[479]));
	texts.emplace_back(std::make_unique<CTextBox>(CGI->generaltexth->allTexts[480], Rect(320, 56, 256, 40), 0, FONT_SMALL, ETextAlignment::CENTER, Colors::YELLOW));
	lSubtitle = std::make_shared<CLabel>(180, 503, FONT_SMALL, ETextAlignment::CENTER, Colors::WHITE);
	rSubtitle = std::make_shared<CLabel>(426, 503, FONT_SMALL, ETextAlignment::CENTER, Colors::WHITE);

	offerSlider = std::make_shared<CSlider>(Point(231, 481), 137, std::bind(&CAltarCreatures::onOfferSliderMoved, this, _1), 0, 0, 0, Orientation::HORIZONTAL);
	maxUnits = std::make_shared<CButton>(Point(147, 520), AnimationPath::builtin("IRCBTNS.DEF"), CGI->generaltexth->zelp[578], std::bind(&CSlider::scrollToMax, offerSlider));

	unitsOnAltar.resize(GameConstants::ARMY_SIZE, 0);
	expPerUnit.resize(GameConstants::ARMY_SIZE, 0);
	sacrificeAllButton = std::make_shared<CButton>(
		Point(393, 520), AnimationPath::builtin("ALTARMY.DEF"), CGI->generaltexth->zelp[579], std::bind(&CExperienceAltar::sacrificeAll, this));

	// Hero creatures panel
	assert(leftTradePanel);
	leftTradePanel->moveBy(Point(45, 110));
	leftTradePanel->updateSlotsCallback = std::bind(&CCreaturesSelling::updateSubtitle, this);
	for(const auto & slot : leftTradePanel->slots)
		slot->clickPressedCallback = [this](const std::shared_ptr<CTradeableItem> & heroSlot) {CAltarCreatures::onSlotClickPressed(heroSlot, hLeft);};

	// Altar creatures panel
	rightTradePanel = std::make_shared<CreaturesPanel>([this](const std::shared_ptr<CTradeableItem> & altarSlot)
		{
			CAltarCreatures::onSlotClickPressed(altarSlot, hRight);
		}, leftTradePanel->slots);
	rightTradePanel->moveBy(Point(334, 110));

	leftTradePanel->deleteSlotsCheck = rightTradePanel->deleteSlotsCheck = std::bind(&CCreaturesSelling::slotDeletingCheck, this, _1);
	readExpValues();
	expForHero->setText(std::to_string(0));
	CAltarCreatures::deselect();
};

void CAltarCreatures::readExpValues()
{
	int dump;
	for(auto heroSlot : leftTradePanel->slots)
	{
		if(heroSlot->id >= 0)
			market->getOffer(heroSlot->id, 0, dump, expPerUnit[heroSlot->serial], EMarketMode::CREATURE_EXP);
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
	maxUnits->block(offerSlider->getAmount() == 0);
}

void CAltarCreatures::updateSubtitlesForSelected()
{
	if(hLeft)
		lSubtitle->setText(std::to_string(offerSlider->getValue()));
	else
		lSubtitle->setText("");
	if(hRight)
		rSubtitle->setText(hRight->subtitle);
	else
		rSubtitle->setText("");
}

void CAltarCreatures::updateSlots()
{
	rightTradePanel->deleteSlots();
	leftTradePanel->deleteSlots();
	assert(leftTradePanel->slots.size() == rightTradePanel->slots.size());
	readExpValues();
	leftTradePanel->updateSlots();
}

void CAltarCreatures::deselect()
{
	CTradeBase::deselect();
	offerSlider->block(true);
	maxUnits->block(true);
	updateSubtitlesForSelected();
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
	offerSlider->scrollTo(0);
	expForHero->setText(std::to_string(0));

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

	for(auto heroSlot : rightTradePanel->slots)
	{
		heroSlot->setType(EType::CREATURE_PLACEHOLDER);
		heroSlot->subtitle.clear();
	}
}

void CAltarCreatures::sacrificeAll()
{
	std::optional<SlotID> lastSlot;
	for(auto heroSlot : leftTradePanel->slots)
	{
		auto stackCount = hero->getStackCount(SlotID(heroSlot->serial));
		if(stackCount > unitsOnAltar[heroSlot->serial])
		{
			if(!lastSlot.has_value())
				lastSlot = SlotID(heroSlot->serial);
			unitsOnAltar[heroSlot->serial] = stackCount;
		}
	}
	assert(lastSlot.has_value());
	unitsOnAltar[lastSlot.value().num]--;

	if(hRight)
		offerSlider->scrollTo(unitsOnAltar[hRight->serial]);
	for(auto altarSlot : rightTradePanel->slots)
		updateAltarSlot(altarSlot);
	updateSubtitlesForSelected();

	deal->block(calcExpAltarForHero() == 0);
}

void CAltarCreatures::updateAltarSlot(std::shared_ptr<CTradeableItem> slot)
{
	auto units = unitsOnAltar[slot->serial];
	slot->setType(units > 0 ? EType::CREATURE : EType::CREATURE_PLACEHOLDER);
	slot->subtitle = units > 0 ?
		boost::str(boost::format(CGI->generaltexth->allTexts[122]) % std::to_string(hero->calculateXp(units * expPerUnit[slot->serial]))) : "";
}

void CAltarCreatures::onOfferSliderMoved(int newVal)
{
	if(hLeft)
		unitsOnAltar[hLeft->serial] = newVal;
	if(hRight)
		updateAltarSlot(hRight);
	deal->block(calcExpAltarForHero() == 0);
	updateControls();
	updateSubtitlesForSelected();
}

void CAltarCreatures::onSlotClickPressed(const std::shared_ptr<CTradeableItem> & newSlot, std::shared_ptr<CTradeableItem> & hCurSide)
{
	if(hCurSide == newSlot)
		return;

	auto * oppositeSlot = &hLeft;
	auto oppositePanel = leftTradePanel;
	CTradeBase::onSlotClickPressed(newSlot, hCurSide);
	if(hCurSide == hLeft)
	{
		oppositeSlot = &hRight;
		oppositePanel = rightTradePanel;
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
	updateSubtitlesForSelected();
	redraw();
}
