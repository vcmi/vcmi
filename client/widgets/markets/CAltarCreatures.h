/*
 * CAltarCreatures.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "CTradeBase.h"

class CAltarCreatures : public CExperienceAltar, public CCreaturesSelling
{
public:
	CAltarCreatures(const IMarket * market, const CGHeroInstance * hero);
	void updateSlots();
	void deselect() override;
	TExpType calcExpAltarForHero() override;
	void makeDeal() override;
	void sacrificeAll() override;
	void updateAltarSlot(std::shared_ptr<CTradeableItem> slot);

private:
	std::shared_ptr<CButton> maxUnits;
	std::vector<int> unitsOnAltar;
	std::vector<int> expPerUnit;
	std::shared_ptr<CLabel> lSubtitle;
	std::shared_ptr<CLabel> rSubtitle;

	void readExpValues();
	void updateControls();
	void updateSubtitlesForSelected();
	void onOfferSliderMoved(int newVal);
	void onSlotClickPressed(const std::shared_ptr<CTradeableItem> & newSlot, std::shared_ptr<CTradeableItem> & hCurSide) override;
};
