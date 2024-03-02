/*
 * CArtifactsBuying.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "CMarketBase.h"

class CArtifactsBuying : public CResourcesSelling
{
public:
	CArtifactsBuying(const IMarket * market, const CGHeroInstance * hero);
	void makeDeal() override;

private:
	CMarketBase::SelectionParams getSelectionParams() const override;
	void highlightingChanged();
	void onSlotClickPressed(const std::shared_ptr<CTradeableItem> & newSlot, std::shared_ptr<CTradeableItem> & hCurSlot);
};
