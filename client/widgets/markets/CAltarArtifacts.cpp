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

	assert(market);
	auto altarObj = dynamic_cast<const CGArtifactsAltar*>(market);
	altarId = altarObj->id;
	altarArtifacts = altarObj;

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

	heroArts = std::make_shared<CArtifactsOfHeroAltar>(Point(-365, -11));
	heroArts->setHero(hero);

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
	TExpType expOnAltar(0);
	for(const auto & tradeSlot : tradeSlotsMap)
		expOnAltar += calcExpCost(tradeSlot.first);
	expForHero->setText(std::to_string(expOnAltar));
	return expOnAltar;
}

void CAltarArtifacts::makeDeal()
{
	std::vector<TradeItemSell> positions;
	for(const auto & [artInst, altarSlot] : tradeSlotsMap)
	{
		positions.push_back(artInst->getId());
	}
	LOCPLINT->cb->trade(market, EMarketMode::ARTIFACT_EXP, positions, std::vector<TradeItemBuy>(), std::vector<ui32>(), hero);

	tradeSlotsMap.clear();
	// The event for removing artifacts from the altar will not be triggered. Therefore, we clean the altar immediately.
	for(auto item : items[0])
	{
		item->setID(-1);
		item->subtitle.clear();
	}
	calcExpAltarForHero();
	deal->block(tradeSlotsMap.empty());
}

void CAltarArtifacts::sacrificeAll()
{
	LOCPLINT->cb->bulkMoveArtifacts(heroArts->getHero()->id, altarId, false, true, true);
}

void CAltarArtifacts::sacrificeBackpack()
{
	LOCPLINT->cb->bulkMoveArtifacts(heroArts->getHero()->id, altarId, false, false, true);
}

void CAltarArtifacts::setSelectedArtifact(const CArtifactInstance * art)
{
	selectedArt->setArtifact(art);
	selectedCost->setText(art == nullptr ? "" : std::to_string(calcExpCost(art)));
}

std::shared_ptr<CArtifactsOfHeroAltar> CAltarArtifacts::getAOHset() const
{
	return heroArts;
}

ObjectInstanceID CAltarArtifacts::getObjId() const
{
	return altarId;
}

void CAltarArtifacts::updateSlots()
{
	assert(altarArtifacts->artifactsInBackpack.size() <= GameConstants::ALTAR_ARTIFACTS_SLOTS);
	assert(tradeSlotsMap.size() <= GameConstants::ALTAR_ARTIFACTS_SLOTS);
	
	auto slotsToAdd = tradeSlotsMap;
	for(auto & altarSlot : items[0])
		if(altarSlot->id != -1)
		{
			if(tradeSlotsMap.find(altarSlot->getArtInstance()) == tradeSlotsMap.end())
			{
				altarSlot->setID(-1);
				altarSlot->subtitle.clear();
			}
			else
			{
				slotsToAdd.erase(altarSlot->getArtInstance());
			}
		}

	for(auto & tradeSlot : slotsToAdd)
	{
		assert(tradeSlot.second->id == -1);
		assert(altarArtifacts->getSlotByInstance(tradeSlot.first) != ArtifactPosition::PRE_FIRST);
		tradeSlot.second->setArtInstance(tradeSlot.first);
		tradeSlot.second->subtitle = std::to_string(calcExpCost(tradeSlot.first));
	}
	for(auto & slotInfo : altarArtifacts->artifactsInBackpack)
	{
		if(tradeSlotsMap.find(slotInfo.artifact) == tradeSlotsMap.end())
		{
			for(auto & altarSlot : items[0])
				if(altarSlot->id == -1)
				{
					altarSlot->setArtInstance(slotInfo.artifact);
					altarSlot->subtitle = std::to_string(calcExpCost(slotInfo.artifact));
					tradeSlotsMap.try_emplace(slotInfo.artifact, altarSlot);
					break;
				}
		}
	}
	calcExpAltarForHero();
	deal->block(tradeSlotsMap.empty());
}

void CAltarArtifacts::putBackArtifacts()
{
	// TODO: If the backpack capacity limit is enabled, artifacts may remain on the altar.
	// Perhaps should be erased in CGameHandler::objectVisitEnded if id of visited object will be available
	if(!altarArtifacts->artifactsInBackpack.empty())
		LOCPLINT->cb->bulkMoveArtifacts(altarId, heroArts->getHero()->id, false, true, true);
}

void CAltarArtifacts::onSlotClickPressed(const std::shared_ptr<CTradeableItem> & altarSlot, std::shared_ptr<CTradeableItem> & hCurSlot)
{
	assert(altarSlot);

	if(const auto pickedArtInst = heroArts->getPickedArtifact())
	{
		if(pickedArtInst->canBePutAt(altarArtifacts))
		{
			if(pickedArtInst->artType->isTradable())
			{
				if(altarSlot->id == -1)
					tradeSlotsMap.try_emplace(pickedArtInst, altarSlot);
				deal->block(false);

				LOCPLINT->cb->swapArtifacts(ArtifactLocation(heroArts->getHero()->id, ArtifactPosition::TRANSITION_POS),
					ArtifactLocation(altarId, ArtifactPosition::ALTAR));
			}
			else
			{
				logGlobal->warn("Cannot put special artifact on altar!");
				return;
			}
		}
	}
	else if(const CArtifactInstance * art = altarSlot->getArtInstance())
	{
		const auto slot = altarArtifacts->getSlotByInstance(art);
		assert(slot != ArtifactPosition::PRE_FIRST);
		LOCPLINT->cb->swapArtifacts(ArtifactLocation(altarId, slot), ArtifactLocation(hero->id, ArtifactPosition::TRANSITION_POS));
		tradeSlotsMap.erase(art);
	}
}

TExpType CAltarArtifacts::calcExpCost(const CArtifactInstance * art)
{
	int dmp = 0;
	int expOfArt = 0;
	market->getOffer(art->getTypeId(), 0, dmp, expOfArt, EMarketMode::ARTIFACT_EXP);
	return hero->calculateXp(expOfArt);
}
