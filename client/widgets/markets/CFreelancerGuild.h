/*
 * CFreelancerGuild.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "CMarketBase.h"

class CFreelancerGuild : public CCreaturesSelling , public CResourcesBuying, public CMarketSlider
{
public:
	CFreelancerGuild(const IMarket * market, const CGHeroInstance * hero);
	void deselect() override;
	void makeDeal() override;

private:
	CMarketBase::SelectionParams getSelectionParams() const override;
	void highlightingChanged();
	void onSlotClickPressed(const std::shared_ptr<CTradeableItem> & newSlot, std::shared_ptr<CTradeableItem> & hCurSlot) override;
};
