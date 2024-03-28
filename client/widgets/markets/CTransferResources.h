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

#include "CMarketBase.h"

class CTransferResources : public CResourcesSelling, public CMarketSlider, public CMarketTraderText
{
public:
	CTransferResources(const IMarket * market, const CGHeroInstance * hero);
	void deselect() override;
	void makeDeal() override;

private:
	CMarketBase::MarketShowcasesParams getShowcasesParams() const override;
	void highlightingChanged() override;
	std::string getTraderText() override;
};
