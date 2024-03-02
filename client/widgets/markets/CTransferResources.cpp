/*
 * CTransferResources.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "CTransferResources.h"

#include "../../gui/CGuiHandler.h"
#include "../../widgets/Buttons.h"
#include "../../widgets/TextControls.h"

#include "../../CGameInfo.h"
#include "../../CPlayerInterface.h"

#include "../../../CCallback.h"

#include "../../../lib/CGeneralTextHandler.h"

CTransferResources::CTransferResources(const IMarket * market, const CGHeroInstance * hero)
	: CMarketBase(market, hero, [this](){return CTransferResources::getSelectionParams();})
	, CResourcesSelling([this](const std::shared_ptr<CTradeableItem> & heroSlot){CTransferResources::onSlotClickPressed(heroSlot, hLeft);})
	, CMarketSlider([this](int newVal){CMarketSlider::onOfferSliderMoved(newVal);})
{
	OBJECT_CONSTRUCTION_CAPTURING(255 - DISPOSE);

	labels.emplace_back(std::make_shared<CLabel>(titlePos.x, titlePos.y, FONT_BIG, ETextAlignment::CENTER, Colors::YELLOW, CGI->generaltexth->allTexts[158]));
	labels.emplace_back(std::make_shared<CLabel>(445, 56, FONT_SMALL, ETextAlignment::CENTER, Colors::WHITE, CGI->generaltexth->allTexts[169]));
	deal = std::make_shared<CButton>(dealButtonPosWithSlider, AnimationPath::builtin("TPMRKB.DEF"),
		CGI->generaltexth->zelp[595], [this]() {CTransferResources::makeDeal();});

	// Player's resources
	assert(bidTradePanel);
	bidTradePanel->moveTo(pos.topLeft() + Point(40, 183));

	// Players panel
	offerTradePanel = std::make_shared<PlayersPanel>([this](const std::shared_ptr<CTradeableItem> & heroSlot)
		{
			CTransferResources::onSlotClickPressed(heroSlot, hRight);
		});
	offerTradePanel->moveTo(pos.topLeft() + Point(333, 84));

	CMarketBase::update();
	CTransferResources::deselect();
}

void CTransferResources::deselect()
{
	CMarketBase::deselect();
	CMarketSlider::deselect();
}

void CTransferResources::makeDeal()
{
	if(auto toTrade = offerSlider->getValue(); toTrade != 0)
	{
		LOCPLINT->cb->trade(market, EMarketMode::RESOURCE_PLAYER, GameResID(hLeft->id), PlayerColor(hRight->id), toTrade, hero);
		deselect();
	}
}

CMarketBase::SelectionParams CTransferResources::getSelectionParams() const
{
	if(hLeft && hRight)
		return std::make_tuple(
			SelectionParamOneSide {std::to_string(offerSlider->getValue()), hLeft->id},
			SelectionParamOneSide {CGI->generaltexth->capColors[hRight->id], hRight->id});
	else
		return std::make_tuple(std::nullopt, std::nullopt);
}

void CTransferResources::highlightingChanged()
{
	if(hLeft)
	{
		if(hRight)
		{
			offerSlider->setAmount(LOCPLINT->cb->getResourceAmount(GameResID(hLeft->id)));
			offerSlider->scrollTo(0);
			offerSlider->block(false);
			maxAmount->block(false);
			deal->block(false);
		}
		offerTradePanel->update();
	}
	updateSelected();
}

void CTransferResources::onSlotClickPressed(const std::shared_ptr<CTradeableItem> & newSlot, std::shared_ptr<CTradeableItem> & hCurSlot)
{
	assert(newSlot);
	if(newSlot == hCurSlot)
		return;

	CMarketBase::onSlotClickPressed(newSlot, hCurSlot);
	highlightingChanged();
	redraw();
}
