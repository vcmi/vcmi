/*
 * CTransferResources.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "CTradeBase.h"

class CTransferResources : public CResourcesSelling
{
public:
	CTransferResources(const IMarket * market, const CGHeroInstance * hero);
	void makeDeal() override;
	void deselect() override;

private:
	void updateSelected();
	void onOfferSliderMoved(int newVal);
	void onSlotClickPressed(const std::shared_ptr<CTradeableItem> & newSlot, std::shared_ptr<CTradeableItem> & hCurSlot);
};
