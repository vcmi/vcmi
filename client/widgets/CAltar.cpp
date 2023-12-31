/*
 * CAltarWindow.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "CAltar.h"

#include "../widgets/CAltar.h"
#include "../gui/CGuiHandler.h"
#include "../gui/CursorHandler.h"
#include "../widgets/Buttons.h"
#include "../widgets/Slider.h"
#include "../widgets/TextControls.h"

#include "../CGameInfo.h"
#include "../CPlayerInterface.h"

#include "../../CCallback.h"

#include "../../lib/networkPacks/ArtifactLocation.h"
#include "../../lib/CGeneralTextHandler.h"
#include "../../lib/mapObjects/CGHeroInstance.h"
#include "../../lib/mapObjects/CGMarket.h"

CAltar::CAltar(const IMarket * market, const CGHeroInstance * hero)
	: CTradeBase(market, hero)
{
	OBJECT_CONSTRUCTION_CAPTURING(255-DISPOSE);

	// Experience needed to reach next level
	texts.emplace_back(std::make_shared<CTextBox>(CGI->generaltexth->allTexts[475], Rect(15, 415, 125, 50), 0, FONT_SMALL, ETextAlignment::CENTER, Colors::YELLOW));
	// Total experience on the Altar
	texts.emplace_back(std::make_shared<CTextBox>(CGI->generaltexth->allTexts[476], Rect(15, 495, 125, 40), 0, FONT_SMALL, ETextAlignment::CENTER, Colors::YELLOW));
	deal = std::make_shared<CButton>(Point(269, 520), AnimationPath::builtin("ALTSACR.DEF"), CGI->generaltexth->zelp[585], std::bind(&CAltar::makeDeal, this));
	expToLevel = std::make_shared<CLabel>(75, 477, FONT_SMALL, ETextAlignment::CENTER);
	expForHero = std::make_shared<CLabel>(75, 545, FONT_SMALL, ETextAlignment::CENTER);
}

void CAltar::deselect()
{
	hLeft = hRight = nullptr;
	deal->block(true);
}

CAltarArtifacts::CAltarArtifacts(const IMarket * market, const CGHeroInstance * hero)
	: CAltar(market, hero)
{
	OBJECT_CONSTRUCTION_CAPTURING(255 - DISPOSE);

	labels.emplace_back(std::make_shared<CLabel>(450, 34, FONT_SMALL, ETextAlignment::CENTER, Colors::YELLOW, CGI->generaltexth->allTexts[477]));
	labels.emplace_back(std::make_shared<CLabel>(302, 423, FONT_SMALL, ETextAlignment::CENTER, Colors::YELLOW, CGI->generaltexth->allTexts[478]));
	selectedCost = std::make_shared<CLabel>(302, 500, FONT_SMALL, ETextAlignment::CENTER, Colors::WHITE);
	selectedArt = std::make_shared<CArtPlace>(Point(280, 442));

	sacrificeAllButton = std::make_shared<CButton>(Point(393, 520), AnimationPath::builtin("ALTFILL.DEF"),
		CGI->generaltexth->zelp[571], std::bind(&CAltar::sacrificeAll, this));
	sacrificeAllButton->block(hero->artifactsInBackpack.empty() && hero->artifactsWorn.empty());

	sacrificeBackpackButton = std::make_shared<CButton>(Point(147, 520), AnimationPath::builtin("ALTEMBK.DEF"),
		CGI->generaltexth->zelp[570], std::bind(&CAltarArtifacts::sacrificeBackpack, this));
	sacrificeBackpackButton->block(hero->artifactsInBackpack.empty());

	arts = std::make_shared<CArtifactsOfHeroAltar>(Point(-365, -11));
	arts->setHero(hero);

	int slotNum = 0;
	for(auto & altarSlotPos : posSlotsAltar)
	{
		auto altarSlot = std::make_shared<CTradeableItem>(altarSlotPos, EType::ARTIFACT_PLACEHOLDER, -1, false, slotNum++);
		altarSlot->clickPressedCallback = std::bind(&CAltarArtifacts::onSlotClickPressed, this, _1);
		altarSlot->subtitle = "";
		items.front().emplace_back(altarSlot);
	}

	calcExpAltarForHero();
	deselect();
};

TExpType CAltarArtifacts::calcExpAltarForHero()
{
	auto artifactsOfHero = std::dynamic_pointer_cast<CArtifactsOfHeroAltar>(arts);
	TExpType expOnAltar(0);
	for(const auto art : artifactsOfHero->artifactsOnAltar)
	{
		int dmp, expOfArt;
		market->getOffer(art->artType->getId(), 0, dmp, expOfArt, EMarketMode::ARTIFACT_EXP);
		expOnAltar += expOfArt;
	}
	auto resultExp = hero->calculateXp(expOnAltar);
	expForHero->setText(std::to_string(resultExp));
	return resultExp;
}

void CAltarArtifacts::makeDeal()
{
	std::vector<TradeItemSell> positions;
	for(const auto art : arts->artifactsOnAltar)
	{
		positions.push_back(hero->getSlotByInstance(art));
	}
	std::sort(positions.begin(), positions.end());
	std::reverse(positions.begin(), positions.end());

	LOCPLINT->cb->trade(market, EMarketMode::ARTIFACT_EXP, positions, std::vector<TradeItemBuy>(), std::vector<ui32>(), hero);
	arts->artifactsOnAltar.clear();

	for(auto item : items[0])
	{
		item->setID(-1);
		item->subtitle = "";
	}
	deal->block(true);
	calcExpAltarForHero();
}

void CAltarArtifacts::sacrificeAll()
{
	std::vector<ConstTransitivePtr<CArtifactInstance>> artsForMove;
	for(const auto & slotInfo : arts->getHero()->artifactsWorn)
	{
		if(!slotInfo.second.locked && slotInfo.second.artifact->artType->isTradable())
			artsForMove.push_back(slotInfo.second.artifact);
	}
	for(auto artInst : artsForMove)
		moveArtToAltar(nullptr, artInst);
	arts->updateWornSlots();
	sacrificeBackpack();
}

void CAltarArtifacts::sacrificeBackpack()
{
	while(!arts->visibleArtSet.artifactsInBackpack.empty())
	{
		if(!putArtOnAltar(nullptr, arts->visibleArtSet.artifactsInBackpack[0].artifact))
			break;
	};
	calcExpAltarForHero();
}

void CAltarArtifacts::setSelectedArtifact(const CArtifactInstance * art)
{
	if(art)
	{
		selectedArt->setArtifact(art);
		int dmp, exp;
		market->getOffer(art->getTypeId(), 0, dmp, exp, EMarketMode::ARTIFACT_EXP);
		selectedCost->setText(std::to_string(hero->calculateXp(exp)));
	}
	else
	{
		selectedArt->setArtifact(nullptr);
		selectedCost->setText("");
	}
}

void CAltarArtifacts::moveArtToAltar(std::shared_ptr<CTradeableItem> altarSlot, const CArtifactInstance * art)
{
	if(putArtOnAltar(altarSlot, art))
	{
		CCS->curh->dragAndDropCursor(nullptr);
		arts->unmarkSlots();
	}
}

std::shared_ptr<CArtifactsOfHeroAltar> CAltarArtifacts::getAOHset() const
{
	return arts;
}

bool CAltarArtifacts::putArtOnAltar(std::shared_ptr<CTradeableItem> altarSlot, const CArtifactInstance * art)
{
	if(!art->artType->isTradable())
	{
		logGlobal->warn("Cannot put special artifact on altar!");
		return false;
	}

	if(!altarSlot || altarSlot->id != -1)
	{
		int slotIndex = -1;
		while(items[0][++slotIndex]->id >= 0 && slotIndex + 1 < items[0].size());
		slotIndex = items[0][slotIndex]->id == -1 ? slotIndex : -1;
		if(slotIndex < 0)
		{
			logGlobal->warn("No free slots on altar!");
			return false;
		}
		altarSlot = items[0][slotIndex];
	}

	int dmp, exp;
	market->getOffer(art->artType->getId(), 0, dmp, exp, EMarketMode::ARTIFACT_EXP);
	exp = static_cast<int>(hero->calculateXp(exp));

	arts->artifactsOnAltar.insert(art);
	altarSlot->setArtInstance(art);
	altarSlot->subtitle = std::to_string(exp);

	deal->block(false);
	return true;
};

void CAltarArtifacts::onSlotClickPressed(std::shared_ptr<CTradeableItem> altarSlot)
{
	const auto pickedArtInst = arts->getPickedArtifact();
	if(pickedArtInst)
	{
		arts->pickedArtMoveToAltar(ArtifactPosition::TRANSITION_POS);
		moveArtToAltar(altarSlot, pickedArtInst);
	}
	else if(const CArtifactInstance * art = altarSlot->getArtInstance())
	{
		const auto hero = arts->getHero();
		const auto slot = hero->getSlotByInstance(art);
		assert(slot != ArtifactPosition::PRE_FIRST);
		LOCPLINT->cb->swapArtifacts(ArtifactLocation(hero->id, slot),
			ArtifactLocation(hero->id, ArtifactPosition::TRANSITION_POS));
		arts->pickedArtFromSlot = slot;
		arts->artifactsOnAltar.erase(art);
		altarSlot->setID(-1);
		altarSlot->subtitle.clear();
		deal->block(!arts->artifactsOnAltar.size());
	}
	calcExpAltarForHero();
}

CAltarCreatures::CAltarCreatures(const IMarket * market, const CGHeroInstance * hero)
	: CAltar(market, hero)
{
	OBJECT_CONSTRUCTION_CUSTOM_CAPTURING(255 - DISPOSE);

	labels.emplace_back(std::make_shared<CLabel>(155, 30, FONT_SMALL, ETextAlignment::CENTER, Colors::YELLOW,
		boost::str(boost::format(CGI->generaltexth->allTexts[272]) % hero->getNameTranslated())));
	labels.emplace_back(std::make_shared<CLabel>(450, 30, FONT_SMALL, ETextAlignment::CENTER, Colors::YELLOW, CGI->generaltexth->allTexts[479]));
	texts.emplace_back(std::make_unique<CTextBox>(CGI->generaltexth->allTexts[480], Rect(320, 56, 256, 40), 0, FONT_SMALL, ETextAlignment::CENTER, Colors::YELLOW));
	lSubtitle = std::make_shared<CLabel>(180, 503, FONT_SMALL, ETextAlignment::CENTER, Colors::WHITE);
	rSubtitle = std::make_shared<CLabel>(426, 503, FONT_SMALL, ETextAlignment::CENTER, Colors::WHITE);

	unitsSlider = std::make_shared<CSlider>(Point(231, 481), 137, std::bind(&CAltarCreatures::onUnitsSliderMoved, this, _1), 0, 0, 0, Orientation::HORIZONTAL);
	maxUnits = std::make_shared<CButton>(Point(147, 520), AnimationPath::builtin("IRCBTNS.DEF"), CGI->generaltexth->zelp[578], std::bind(&CSlider::scrollToMax, unitsSlider));

	unitsOnAltar.resize(GameConstants::ARMY_SIZE, 0);
	expPerUnit.resize(GameConstants::ARMY_SIZE, 0);
	sacrificeAllButton = std::make_shared<CButton>(
		Point(393, 520), AnimationPath::builtin("ALTARMY.DEF"), CGI->generaltexth->zelp[579], std::bind(&CAltar::sacrificeAll, this));

	// Creating slots for hero creatures
	for(int slotIdx = 0; slotIdx < GameConstants::ARMY_SIZE; slotIdx++)
	{
		CreatureID creatureId = CreatureID::NONE;
		if(const auto & creature = hero->getCreature(SlotID(slotIdx)))
			creatureId = creature->getId();
		else
			continue;

		auto heroSlot = std::make_shared<CTradeableItem>(posSlotsHero[slotIdx], EType::CREATURE, creatureId.num, true, slotIdx);
		heroSlot->clickPressedCallback = [this](std::shared_ptr<CTradeableItem> altarSlot) -> void
			{
				onSlotClickPressed(altarSlot, items[0], hLeft, hRight);
			};
		heroSlot->subtitle = std::to_string(hero->getStackCount(SlotID(slotIdx)));
		items[1].emplace_back(heroSlot);
	}

	// Creating slots for creatures on altar
	assert(items[1].size() <= posSlotsAltar.size());
	for(const auto & heroSlot : items[1])
	{
		auto altarSlot = std::make_shared<CTradeableItem>(posSlotsAltar[heroSlot->serial], EType::CREATURE_PLACEHOLDER, heroSlot->id, false, heroSlot->serial);
		altarSlot->pos.w = heroSlot->pos.w; altarSlot->pos.h = heroSlot->pos.h;
		altarSlot->clickPressedCallback = [this](std::shared_ptr<CTradeableItem> altarSlot) -> void
			{
				onSlotClickPressed(altarSlot, items[1], hRight, hLeft);
			};
		items[0].emplace_back(altarSlot);
	}

	readExpValues();
	calcExpAltarForHero();
	deselect();
};

void CAltarCreatures::readExpValues()
{
	int dump;
	for(auto heroSlot : items[1])
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
	unitsSlider->setAmount(sliderAmount);
	unitsSlider->block(!unitsSlider->getAmount());
	if(hLeft)
		unitsSlider->scrollTo(unitsOnAltar[hLeft->serial]);
	maxUnits->block(unitsSlider->getAmount() == 0);
}

void CAltarCreatures::updateSubtitlesForSelected()
{
	if(hLeft)
		lSubtitle->setText(std::to_string(unitsSlider->getValue()));
	else
		lSubtitle->setText("");
	if(hRight)
		rSubtitle->setText(hRight->subtitle);
	else
		rSubtitle->setText("");
}

void CAltarCreatures::updateGarrison()
{
	std::set<std::shared_ptr<CTradeableItem>> empty;
	getEmptySlots(empty);
	removeItems(empty);
	readExpValues();
	for(auto & heroSlot : items[1])
		heroSlot->subtitle = std::to_string(hero->getStackCount(SlotID(heroSlot->serial)));
}

void CAltarCreatures::deselect()
{
	CAltar::deselect();
	unitsSlider->block(true);
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
	unitsSlider->scrollTo(0);
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

	for(auto heroSlot : items[0])
	{
		heroSlot->setType(CREATURE_PLACEHOLDER);
		heroSlot->subtitle = "";
	}
}

void CAltarCreatures::sacrificeAll()
{
	std::optional<SlotID> lastSlot;
	for(auto heroSlot : items[1])
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
		unitsSlider->scrollTo(unitsOnAltar[hRight->serial]);
	for(auto altarSlot : items[0])
		updateAltarSlot(altarSlot);
	updateSubtitlesForSelected();

	deal->block(calcExpAltarForHero() == 0);
}

void CAltarCreatures::updateAltarSlot(std::shared_ptr<CTradeableItem> slot)
{
	auto units = unitsOnAltar[slot->serial];
	slot->setType(units > 0 ? CREATURE : CREATURE_PLACEHOLDER);
	slot->subtitle = units > 0 ?
		boost::str(boost::format(CGI->generaltexth->allTexts[122]) % std::to_string(hero->calculateXp(units * expPerUnit[slot->serial]))) : "";
}

void CAltarCreatures::onUnitsSliderMoved(int newVal)
{
	if(hLeft)
		unitsOnAltar[hLeft->serial] = newVal;
	if(hRight)
		updateAltarSlot(hRight);
	deal->block(calcExpAltarForHero() == 0);
	updateControls();
	updateSubtitlesForSelected();
}

void CAltarCreatures::onSlotClickPressed(std::shared_ptr<CTradeableItem> altarSlot,
	std::vector<std::shared_ptr<CTradeableItem>> & oppositeSlots,
	std::shared_ptr<CTradeableItem> & hCurSide, std::shared_ptr<CTradeableItem> & hOppSide)
{
	std::shared_ptr<CTradeableItem> oppositeSlot;
	for(const auto & slot : oppositeSlots)
		if(slot->serial == altarSlot->serial)
		{
			oppositeSlot = slot;
			break;
		}

	if(hCurSide != altarSlot && oppositeSlot)
	{
		hCurSide = altarSlot;
		hOppSide = oppositeSlot;
		updateControls();
		updateSubtitlesForSelected();
		redraw();
	}
}
