/*
 * CArtifactsSelling.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../CArtifactsOfHeroMarket.h"
#include "CMarketBase.h"

class CArtifactsSelling : public CResourcesBuying
{
public:
	CArtifactsSelling(const IMarket * market, const CGHeroInstance * hero);
	void deselect() override;
	void makeDeal() override;
	void updateShowcases() override;
	void update() override;
	std::shared_ptr<CArtifactsOfHeroMarket> getAOHset() const;

private:
	std::shared_ptr<CArtifactsOfHeroMarket> heroArts;
	std::shared_ptr<CLabel> bidSelectedSubtitle;
	std::shared_ptr<CTradeableItem> bidSelectedSlot;
	ArtifactPosition selectedHeroSlot;

	CMarketBase::SelectionParams getSelectionParams() const override;
	void highlightingChanged() override;
};
