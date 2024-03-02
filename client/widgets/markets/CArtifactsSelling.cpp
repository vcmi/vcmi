/*
 * CArtifactsSelling.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "CArtifactsSelling.h"

#include "../../gui/CGuiHandler.h"
#include "../../widgets/Buttons.h"
#include "../../widgets/TextControls.h"

#include "../../CGameInfo.h"
#include "../../CPlayerInterface.h"

#include "../../../CCallback.h"

#include "../../../lib/CArtifactInstance.h"
#include "../../../lib/CGeneralTextHandler.h"
#include "../../../lib/mapObjects/CGHeroInstance.h"
#include "../../../lib/mapObjects/CGMarket.h"
#include "../../../lib/mapObjects/CGTownInstance.h"

CArtifactsSelling::CArtifactsSelling(const IMarket * market, const CGHeroInstance * hero)
	: CMarketBase(market, hero, [this](){return CArtifactsSelling::getSelectionParams();})
	, CResourcesBuying(
		[this](const std::shared_ptr<CTradeableItem> & resSlot){CArtifactsSelling::onSlotClickPressed(resSlot, hRight);},
		[this](){CMarketBase::updateSubtitles(EMarketMode::ARTIFACT_RESOURCE);})
{
	OBJECT_CONSTRUCTION_CUSTOM_CAPTURING(255 - DISPOSE);

	labels.emplace_back(std::make_shared<CLabel>(titlePos.x, titlePos.y, FONT_BIG, ETextAlignment::CENTER, Colors::YELLOW,
		(*CGI->townh)[dynamic_cast<const CGTownInstance*>(market)->getFaction()]->town->buildings[BuildingID::ARTIFACT_MERCHANT]->getNameTranslated()));
	labels.push_back(std::make_shared<CLabel>(155, 56, FONT_SMALL, ETextAlignment::CENTER, Colors::WHITE, boost::str(boost::format(CGI->generaltexth->allTexts[271]) % hero->getNameTranslated())));
	deal = std::make_shared<CButton>(dealButtonPos, AnimationPath::builtin("TPMRKB.DEF"),
		CGI->generaltexth->zelp[595], [this](){CArtifactsSelling::makeDeal();});
	bidSelectedSlot = std::make_shared<CTradeableItem>(Rect(Point(123, 470), Point(69, 66)), EType::ARTIFACT_TYPE, 0, 0);

	// Market resources panel
	assert(offerTradePanel);
	offerTradePanel->moveTo(pos.topLeft() + Point(326, 184));
	offerTradePanel->selectedSlot->moveTo(pos.topLeft() + Point(409, 473));
	offerTradePanel->selectedSlot->subtitle->moveBy(Point(0, 1));
	
	// Hero's artifacts
	heroArts = std::make_shared<CArtifactsOfHeroMarket>(Point(-361, 46), offerTradePanel->selectionWidth);
	heroArts->setHero(hero);
	heroArts->selectArtCallback = [this](CArtPlace * artPlace)
	{
		assert(artPlace);
		const auto artForTrade = artPlace->getArt();
		assert(artForTrade);
		bidSelectedSlot->setID(artForTrade->getTypeId().num);
		hLeft = bidSelectedSlot;
		selectedHeroSlot = artPlace->slot;
		CArtifactsSelling::highlightingChanged();
		offerTradePanel->update();
		redraw();
	};

	CArtifactsSelling::updateSelected();
	CArtifactsSelling::deselect();
}

void CArtifactsSelling::deselect()
{
	CMarketBase::deselect();
	selectedHeroSlot = ArtifactPosition::PRE_FIRST;
	heroArts->unmarkSlots();
}

void CArtifactsSelling::makeDeal()
{
	const auto art = hero->getArt(selectedHeroSlot);
	assert(art);
	LOCPLINT->cb->trade(market, EMarketMode::ARTIFACT_RESOURCE, art->getId(), GameResID(hRight->id), offerQty, hero);
}

void CArtifactsSelling::updateSelected()
{
	CMarketBase::updateSelected();

	if(hLeft && hRight)
	{
		bidSelectedSlot->image->enable();
		bidSelectedSlot->image->setFrame(CGI->artifacts()->getByIndex(hLeft->id)->getIconIndex());
		bidSelectedSlot->subtitle->setText(std::to_string(bidQty));
	}
	else
	{
		bidSelectedSlot->image->disable();
		bidSelectedSlot->image->setFrame(0);
		bidSelectedSlot->subtitle->clear();
	}
}

void CArtifactsSelling::update()
{
	CMarketBase::update();
	if(selectedHeroSlot != ArtifactPosition::PRE_FIRST)
	{
		if(selectedHeroSlot.num >= ArtifactPosition::BACKPACK_START + hero->artifactsInBackpack.size())
		{
			selectedHeroSlot = ArtifactPosition::BACKPACK_START;
			deselect();
		}
		if(hero->getArt(selectedHeroSlot) == nullptr)
			deselect();
	}
}

std::shared_ptr<CArtifactsOfHeroMarket> CArtifactsSelling::getAOHset() const
{
	return heroArts;
}

CMarketBase::SelectionParams CArtifactsSelling::getSelectionParams() const
{
	if(hLeft && hRight)
		return std::make_tuple(
			std::nullopt,
			SelectionParamOneSide {std::to_string(offerQty), hRight->id}
		);
	else
		return std::make_tuple(std::nullopt, std::nullopt);
}

void CArtifactsSelling::highlightingChanged()
{
	if(hLeft && hRight)
	{
		market->getOffer(hLeft->id, hRight->id, bidQty, offerQty, EMarketMode::ARTIFACT_RESOURCE);
		deal->block(false);
	}
	updateSelected();
}

void CArtifactsSelling::onSlotClickPressed(const std::shared_ptr<CTradeableItem> & newSlot, std::shared_ptr<CTradeableItem> & hCurSlot)
{
	assert(newSlot);
	if(newSlot == hCurSlot)
		return;

	CMarketBase::onSlotClickPressed(newSlot, hCurSlot);
	highlightingChanged();
	redraw();
}
