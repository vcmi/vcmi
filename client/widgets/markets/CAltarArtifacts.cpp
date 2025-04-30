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

#include "../../GameEngine.h"
#include "../../GameInstance.h"
#include "../../gui/Shortcut.h"
#include "../../widgets/Buttons.h"
#include "../../widgets/TextControls.h"

#include "../../CPlayerInterface.h"

#include "../../../CCallback.h"

#include "../../../lib/GameLibrary.h"
#include "../../../lib/entities/artifact/CArtifact.h"
#include "../../../lib/networkPacks/ArtifactLocation.h"
#include "../../../lib/texts/CGeneralTextHandler.h"
#include "../../../lib/mapObjects/CGHeroInstance.h"
#include "../../../lib/mapObjects/IMarket.h"

CAltarArtifacts::CAltarArtifacts(const IMarket * market, const CGHeroInstance * hero)
	: CMarketBase(market, hero)
{
	OBJECT_CONSTRUCTION;

	assert(market->getArtifactsStorage());
	altarArtifactsStorage = market->getArtifactsStorage();

	deal = std::make_shared<CButton>(Point(269, 520), AnimationPath::builtin("ALTSACR.DEF"),
		LIBRARY->generaltexth->zelp[585], [this]() {CAltarArtifacts::makeDeal(); }, EShortcut::MARKET_DEAL);
	labels.emplace_back(std::make_shared<CLabel>(450, 32, FONT_SMALL, ETextAlignment::CENTER, Colors::YELLOW, LIBRARY->generaltexth->allTexts[477]));
	labels.emplace_back(std::make_shared<CLabel>(302, 424, FONT_SMALL, ETextAlignment::CENTER, Colors::YELLOW, LIBRARY->generaltexth->allTexts[478]));

	sacrificeAllButton = std::make_shared<CButton>(Point(393, 520), AnimationPath::builtin("ALTFILL.DEF"),
		LIBRARY->generaltexth->zelp[571], std::bind(&CExperienceAltar::sacrificeAll, this), EShortcut::MARKET_SACRIFICE_ALL);
	sacrificeAllButton->block(hero->artifactsInBackpack.empty() && hero->artifactsWorn.empty());

	sacrificeBackpackButton = std::make_shared<CButton>(Point(147, 520), AnimationPath::builtin("ALTEMBK.DEF"),
		LIBRARY->generaltexth->zelp[570], std::bind(&CAltarArtifacts::sacrificeBackpack, this), EShortcut::MARKET_SACRIFICE_BACKPACK);
	sacrificeBackpackButton->block(hero->artifactsInBackpack.empty());

	// Hero's artifacts
	heroArts = std::make_shared<CArtifactsOfHeroAltar>(Point(-365, -11));
	heroArts->setHero(hero);
	heroArts->altarId = market->getObjInstanceID();

	// Altar
	offerTradePanel = std::make_shared<ArtifactsAltarPanel>([this](const std::shared_ptr<CTradeableItem> & altarSlot)
		{
			CAltarArtifacts::onSlotClickPressed(altarSlot, offerTradePanel);
		});
	offerTradePanel->updateSlotsCallback = std::bind(&CAltarArtifacts::updateAltarSlots, this);
	offerTradePanel->moveTo(pos.topLeft() + Point(315, 53));

	CMarketBase::updateShowcases();
	CAltarArtifacts::deselect();
};

TExpType CAltarArtifacts::calcExpAltarForHero()
{
	TExpType expOnAltar(0);
	for(const auto & tradeSlot : tradeSlotsMap)
		expOnAltar += calcExpCost(tradeSlot.second->getTypeId());
	expForHero->setText(std::to_string(expOnAltar));
	return expOnAltar;
}

void CAltarArtifacts::deselect()
{
	CMarketBase::deselect();
	CExperienceAltar::deselect();
	tradeSlotsMap.clear();
	// The event for removing artifacts from the altar will not be triggered. Therefore, we clean the altar immediately.
	for(const auto & slot : offerTradePanel->slots)
		slot->clear();
	offerTradePanel->showcaseSlot->clear();
}

void CAltarArtifacts::update()
{
	CMarketBase::update();
	CExperienceAltar::update();
	if(const auto art = hero->getArt(ArtifactPosition::TRANSITION_POS))
		offerQty = calcExpCost(art->getTypeId());
	else
		offerQty = 0;
	updateShowcases();
	redraw();
}

void CAltarArtifacts::makeDeal()
{
	std::vector<TradeItemSell> positions;
	for(const auto & [altarSlot, artInst] : tradeSlotsMap)
	{
		positions.push_back(artInst->getId());
	}
	GAME->interface()->cb->trade(market->getObjInstanceID(), EMarketMode::ARTIFACT_EXP, positions, std::vector<TradeItemBuy>(), std::vector<ui32>(), hero);
	deselect();
}

void CAltarArtifacts::sacrificeAll()
{
	GAME->interface()->cb->bulkMoveArtifacts(heroArts->getHero()->id, heroArts->altarId, false, true, true);
}

void CAltarArtifacts::sacrificeBackpack()
{
	GAME->interface()->cb->bulkMoveArtifacts(heroArts->getHero()->id, heroArts->altarId, false, false, true);
}

std::shared_ptr<CArtifactsOfHeroAltar> CAltarArtifacts::getAOHset() const
{
	return heroArts;
}

void CAltarArtifacts::updateAltarSlots()
{
	assert(altarArtifactsStorage->artifactsInBackpack.size() <= GameConstants::ALTAR_ARTIFACTS_SLOTS);
	assert(tradeSlotsMap.size() <= GameConstants::ALTAR_ARTIFACTS_SLOTS);
	
	auto tradeSlotsMapNewArts = tradeSlotsMap;
	for(const auto & altarSlot : offerTradePanel->slots)
		if(altarSlot->id != -1)
		{
			if(tradeSlotsMap.find(altarSlot) == tradeSlotsMap.end())
			{
				altarSlot->setID(-1);
				altarSlot->subtitle->clear();
			}
			else
			{
				tradeSlotsMapNewArts.erase(altarSlot);
			}
		}

	for(auto & tradeSlot : tradeSlotsMapNewArts)
	{
		assert(tradeSlot.first->id == -1);
		assert(altarArtifactsStorage->getArtPos(tradeSlot.second) != ArtifactPosition::PRE_FIRST);
		tradeSlot.first->setID(tradeSlot.second->getTypeId().num);
		tradeSlot.first->subtitle->setText(std::to_string(calcExpCost(tradeSlot.second->getTypeId())));
	}

	auto newArtsFromBulkMove = altarArtifactsStorage->artifactsInBackpack;
	for(const auto & [altarSlot, art] : tradeSlotsMap)
	{
		newArtsFromBulkMove.erase(std::remove_if(newArtsFromBulkMove.begin(), newArtsFromBulkMove.end(), [artForRemove = art](auto & slotInfo)
			{
				return slotInfo.artifactID == artForRemove->getId();
			}));
	}
	for(const auto & slotInfo : newArtsFromBulkMove)
	{
		for(const auto & altarSlot : offerTradePanel->slots)
			if(altarSlot->id == -1)
			{
				altarSlot->setID(slotInfo.getArt()->getTypeId().num);
				altarSlot->subtitle->setText(std::to_string(calcExpCost(slotInfo.getArt()->getTypeId())));
				tradeSlotsMap.try_emplace(altarSlot, slotInfo.getArt());
				break;
			}
	}

	calcExpAltarForHero();
	deal->block(tradeSlotsMap.empty() || !GAME->interface()->makingTurn);
}

void CAltarArtifacts::putBackArtifacts()
{
	// TODO: If the backpack capacity limit is enabled, artifacts may remain on the altar.
	// Perhaps should be erased in CGameHandler::objectVisitEnded if id of visited object will be available
	if(!altarArtifactsStorage->artifactsInBackpack.empty())
		GAME->interface()->cb->bulkMoveArtifacts(heroArts->altarId, heroArts->getHero()->id, false, true, true);
}

CMarketBase::MarketShowcasesParams CAltarArtifacts::getShowcasesParams() const
{
	if(const auto art = hero->getArt(ArtifactPosition::TRANSITION_POS))
		return MarketShowcasesParams
		{
			std::nullopt,
			ShowcaseParams {std::to_string(offerQty), art->getType()->getIconIndex()}
		};
	return MarketShowcasesParams {std::nullopt, std::nullopt};
}

void CAltarArtifacts::onSlotClickPressed(const std::shared_ptr<CTradeableItem> & altarSlot, std::shared_ptr<TradePanelBase> & curPanel)
{
	assert(altarSlot);

	if(const auto pickedArtInst = heroArts->getPickedArtifact())
	{
		if(pickedArtInst->canBePutAt(altarArtifactsStorage))
		{
			if(pickedArtInst->getType()->isTradable())
			{
				if(altarSlot->id == -1)
					tradeSlotsMap.try_emplace(altarSlot, pickedArtInst);
				deal->block(!GAME->interface()->makingTurn);

				GAME->interface()->cb->swapArtifacts(ArtifactLocation(heroArts->getHero()->id, ArtifactPosition::TRANSITION_POS),
					ArtifactLocation(heroArts->altarId, ArtifactPosition::ALTAR));
			}
			else
			{
				logGlobal->warn("Cannot put special artifact on altar!");
				return;
			}
		}
	}
	else if(altarSlot->id != -1)
	{
		assert(tradeSlotsMap.at(altarSlot));
		const auto slot = altarArtifactsStorage->getArtPos(tradeSlotsMap.at(altarSlot));
		assert(slot != ArtifactPosition::PRE_FIRST);
		GAME->interface()->cb->swapArtifacts(ArtifactLocation(heroArts->altarId, slot),
			ArtifactLocation(hero->id, ENGINE->isKeyboardCtrlDown() ? ArtifactPosition::FIRST_AVAILABLE : ArtifactPosition::TRANSITION_POS));
		tradeSlotsMap.erase(altarSlot);
	}
}

TExpType CAltarArtifacts::calcExpCost(ArtifactID id) const
{
	int bidQty = 0;
	int expOfArt = 0;
	market->getOffer(id, 0, bidQty, expOfArt, EMarketMode::ARTIFACT_EXP);
	return hero->calculateXp(expOfArt);
}
