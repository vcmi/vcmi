/*
 * CAltarArtifacts.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "CAltarArtifacts.h"

#include "../../gui/CGuiHandler.h"
#include "../../gui/CursorHandler.h"
#include "../../widgets/Buttons.h"
#include "../../widgets/TextControls.h"

#include "../../CGameInfo.h"
#include "../../CPlayerInterface.h"

#include "../../../CCallback.h"

#include "../../../lib/networkPacks/ArtifactLocation.h"
#include "../../../lib/CGeneralTextHandler.h"
#include "../../../lib/mapObjects/CGHeroInstance.h"
#include "../../../lib/mapObjects/CGMarket.h"

CAltarArtifacts::CAltarArtifacts(const IMarket * market, const CGHeroInstance * hero)
	: CTradeBase(market, hero)
{
	OBJECT_CONSTRUCTION_CAPTURING(255 - DISPOSE);

	deal = std::make_shared<CButton>(dealButtonPos, AnimationPath::builtin("ALTSACR.DEF"),
		CGI->generaltexth->zelp[585], [this]() {CAltarArtifacts::makeDeal(); });
	labels.emplace_back(std::make_shared<CLabel>(450, 34, FONT_SMALL, ETextAlignment::CENTER, Colors::YELLOW, CGI->generaltexth->allTexts[477]));
	labels.emplace_back(std::make_shared<CLabel>(302, 423, FONT_SMALL, ETextAlignment::CENTER, Colors::YELLOW, CGI->generaltexth->allTexts[478]));
	selectedCost = std::make_shared<CLabel>(302, 500, FONT_SMALL, ETextAlignment::CENTER, Colors::WHITE);
	selectedArt = std::make_shared<CArtPlace>(Point(280, 442));

	sacrificeAllButton = std::make_shared<CButton>(Point(393, 520), AnimationPath::builtin("ALTFILL.DEF"),
		CGI->generaltexth->zelp[571], std::bind(&CExperienceAltar::sacrificeAll, this));
	sacrificeAllButton->block(hero->artifactsInBackpack.empty() && hero->artifactsWorn.empty());

	sacrificeBackpackButton = std::make_shared<CButton>(Point(147, 520), AnimationPath::builtin("ALTEMBK.DEF"),
		CGI->generaltexth->zelp[570], std::bind(&CAltarArtifacts::sacrificeBackpack, this));
	sacrificeBackpackButton->block(hero->artifactsInBackpack.empty());

	arts = std::make_shared<CArtifactsOfHeroAltar>(Point(-365, -11));
	arts->setHero(hero);

	int slotNum = 0;
	for(auto & altarSlotPos : posSlotsAltar)
	{
		auto altarSlot = std::make_shared<CTradeableItem>(Rect(altarSlotPos, Point(44, 44)), EType::ARTIFACT_PLACEHOLDER, -1, false, slotNum);
		altarSlot->clickPressedCallback = std::bind(&CAltarArtifacts::onSlotClickPressed, this, _1, hRight);
		altarSlot->subtitle.clear();
		items.front().emplace_back(altarSlot);
		slotNum++;
	}

	expForHero->setText(std::to_string(0));
	CTradeBase::deselect();
};

TExpType CAltarArtifacts::calcExpAltarForHero()
{
	auto artifactsOfHero = std::dynamic_pointer_cast<CArtifactsOfHeroAltar>(arts);
	TExpType expOnAltar(0);
	for(const auto art : artifactsOfHero->artifactsOnAltar)
	{
		int dmp = 0;
		int expOfArt = 0;
		market->getOffer(art->getTypeId(), 0, dmp, expOfArt, EMarketMode::ARTIFACT_EXP);
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
		item->subtitle.clear();
	}
	deal->block(true);
	calcExpAltarForHero();
}

void CAltarArtifacts::sacrificeAll()
{
	std::vector<ConstTransitivePtr<CArtifactInstance>> artsForMove;
	for(const auto & [slot, slotInfo] : arts->getHero()->artifactsWorn)
	{
		if(!slotInfo.locked && slotInfo.artifact->artType->isTradable())
			artsForMove.emplace_back(slotInfo.artifact);
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
		int dmp = 0;
		int exp = 0;
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

	int dmp = 0;
	int exp = 0;
	market->getOffer(art->artType->getId(), 0, dmp, exp, EMarketMode::ARTIFACT_EXP);
	exp = static_cast<int>(hero->calculateXp(exp));

	arts->artifactsOnAltar.insert(art);
	altarSlot->setArtInstance(art);
	altarSlot->subtitle = std::to_string(exp);

	deal->block(false);
	return true;
};

void CAltarArtifacts::onSlotClickPressed(const std::shared_ptr<CTradeableItem> & newSlot, std::shared_ptr<CTradeableItem> & hCurSlot)
{
	const auto pickedArtInst = arts->getPickedArtifact();
	if(pickedArtInst)
	{
		arts->pickedArtMoveToAltar(ArtifactPosition::TRANSITION_POS);
		moveArtToAltar(newSlot, pickedArtInst);
	}
	else if(const CArtifactInstance * art = newSlot->getArtInstance())
	{
		const auto hero = arts->getHero();
		const auto slot = hero->getSlotByInstance(art);
		assert(slot != ArtifactPosition::PRE_FIRST);
		LOCPLINT->cb->swapArtifacts(ArtifactLocation(hero->id, slot),
			ArtifactLocation(hero->id, ArtifactPosition::TRANSITION_POS));
		arts->pickedArtFromSlot = slot;
		arts->artifactsOnAltar.erase(art);
		newSlot->setID(-1);
		newSlot->subtitle.clear();
		deal->block(!arts->artifactsOnAltar.size());
	}
	calcExpAltarForHero();
}
