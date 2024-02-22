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
#include "../../widgets/Slider.h"
#include "../../widgets/TextControls.h"

#include "../../CGameInfo.h"
#include "../../CPlayerInterface.h"

#include "../../../CCallback.h"

#include "../../../lib/CGeneralTextHandler.h"

CTransferResources::CTransferResources(const IMarket * market, const CGHeroInstance * hero)
	: CTradeBase(market, hero)
	, CMarketMisc([this](){return CTransferResources::getSelectionParams();})
{
	OBJECT_CONSTRUCTION_CAPTURING(255 - DISPOSE);

	labels.emplace_back(std::make_shared<CLabel>(299, 27, FONT_BIG, ETextAlignment::CENTER, Colors::YELLOW, CGI->generaltexth->allTexts[158]));
	labels.emplace_back(std::make_shared<CLabel>(445, 56, FONT_SMALL, ETextAlignment::CENTER, Colors::WHITE, CGI->generaltexth->allTexts[169]));
	deal = std::make_shared<CButton>(Point(306, 520), AnimationPath::builtin("TPMRKB.DEF"),
		CGI->generaltexth->zelp[595], [this]() {CTransferResources::makeDeal();});
	maxAmount = std::make_shared<CButton>(Point(228, 520), AnimationPath::builtin("IRCBTNS.DEF"), CGI->generaltexth->zelp[596],
		[this]() {offerSlider->scrollToMax();});
	offerSlider = std::make_shared<CSlider>(Point(230, 489), 137, [this](int newVal)
		{
			CTransferResources::onOfferSliderMoved(newVal);
		}, 0, 0, 0, Orientation::HORIZONTAL);

	// Player's resources
	assert(leftTradePanel);
	std::for_each(leftTradePanel->slots.cbegin(), leftTradePanel->slots.cend(), [this](auto & slot)
		{
			slot->clickPressedCallback = [this](const std::shared_ptr<CTradeableItem> & heroSlot)
			{
				CTransferResources::onSlotClickPressed(heroSlot, hLeft);
			};
		});
	leftTradePanel->moveTo(pos.topLeft() + Point(40, 182));

	// Players panel
	rightTradePanel = std::make_shared<PlayersPanel>([this](const std::shared_ptr<CTradeableItem> & heroSlot)
		{
			CTransferResources::onSlotClickPressed(heroSlot, hRight);
		});
	rightTradePanel->moveTo(pos.topLeft() + Point(333, 83));

	CResourcesSelling::updateSlots();
	CTransferResources::deselect();
}

void CTransferResources::makeDeal()
{
	if(auto toTrade = offerSlider->getValue(); toTrade != 0)
	{
		LOCPLINT->cb->trade(market, EMarketMode::RESOURCE_PLAYER, GameResID(hLeft->id), PlayerColor(hRight->id), toTrade, hero);
		deselect();
	}
}

CMarketMisc::SelectionParams CTransferResources::getSelectionParams()
{
	if(hLeft && hRight)
		return std::make_tuple(std::to_string(offerSlider->getValue()),
			CGI->generaltexth->capColors[hRight->id], hLeft->id, hRight->id);
	else
		return std::nullopt;
}

void CTransferResources::onOfferSliderMoved(int newVal)
{
	if(hLeft && hRight)
	{
		offerSlider->scrollTo(newVal);
		updateSelected();
		redraw();
	}
}

void CTransferResources::onSlotClickPressed(const std::shared_ptr<CTradeableItem> & newSlot, std::shared_ptr<CTradeableItem> & hCurSlot)
{
	if(newSlot == hCurSlot)
		return;

	CTradeBase::onSlotClickPressed(newSlot, hCurSlot);
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
		updateSelected();
		rightTradePanel->updateSlots();
	}
	redraw();
}
