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
		[this](const std::shared_ptr<CTradeableItem> & resSlot){CArtifactsSelling::onSlotClickPressed(resSlot, offerTradePanel);},
		[this](){CMarketBase::updateSubtitlesForBid(EMarketMode::ARTIFACT_RESOURCE, this->hero->getArt(selectedHeroSlot)->getTypeId().num);})
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
	offerTradePanel->showcaseSlot->moveTo(pos.topLeft() + Point(409, 473));
	offerTradePanel->showcaseSlot->subtitle->moveBy(Point(0, 1));
	
	// Hero's artifacts
	heroArts = std::make_shared<CArtifactsOfHeroMarket>(Point(-361, 46), offerTradePanel->selectionWidth);
	heroArts->setHero(hero);
	heroArts->selectArtCallback = [this](const CArtPlace * artPlace)
	{
		assert(artPlace);
		selectedHeroSlot = artPlace->slot;
		CArtifactsSelling::highlightingChanged();
		CIntObject::redraw();
	};

	CArtifactsSelling::updateShowcases();
	CArtifactsSelling::deselect();
}

void CArtifactsSelling::deselect()
{
	CMarketBase::deselect();
	selectedHeroSlot = ArtifactPosition::PRE_FIRST;
	heroArts->unmarkSlots();
	bidSelectedSlot->clear();
}

void CArtifactsSelling::makeDeal()
{
	const auto art = hero->getArt(selectedHeroSlot);
	assert(art);
	LOCPLINT->cb->trade(market, EMarketMode::ARTIFACT_RESOURCE, art->getId(), GameResID(offerTradePanel->getSelectedItemId()), offerQty, hero);
}

void CArtifactsSelling::updateShowcases()
{
	const auto art = hero->getArt(selectedHeroSlot);
	if(art && offerTradePanel->highlightedSlot)
	{
		bidSelectedSlot->image->enable();
		bidSelectedSlot->setID(art->getTypeId().num);
		bidSelectedSlot->image->setFrame(CGI->artifacts()->getByIndex(art->getTypeId())->getIconIndex());
		bidSelectedSlot->subtitle->setText(std::to_string(bidQty));
	}
	else
	{
		bidSelectedSlot->clear();
	}
	CMarketBase::updateShowcases();
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
	if(hero->getArt(selectedHeroSlot) && offerTradePanel->highlightedSlot)
		return std::make_tuple(
			std::nullopt,
			SelectionParamOneSide {std::to_string(offerQty), offerTradePanel->getSelectedItemId()}
		);
	else
		return std::make_tuple(std::nullopt, std::nullopt);
}

void CArtifactsSelling::highlightingChanged()
{
	const auto art = hero->getArt(selectedHeroSlot);
	if(art && offerTradePanel->highlightedSlot)
	{
		market->getOffer(art->getTypeId(), offerTradePanel->getSelectedItemId(), bidQty, offerQty, EMarketMode::ARTIFACT_RESOURCE);
		deal->block(false);
	}
	CMarketBase::highlightingChanged();
}
