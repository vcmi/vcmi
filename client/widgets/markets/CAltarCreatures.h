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
	void updateSlots() override;
	void deselect() override;
	TExpType calcExpAltarForHero() override;
	void makeDeal() override;
	void sacrificeAll() override;

private:
	std::vector<int> unitsOnAltar;
	std::vector<int> expPerUnit;

	CTradeBase::SelectionParams getSelectionParams() const override;
	void updateAltarSlot(const std::shared_ptr<CTradeableItem> & slot);
	void readExpValues();
	void updateControls();
	void onOfferSliderMoved(int newVal);
	void onSlotClickPressed(const std::shared_ptr<CTradeableItem> & newSlot, std::shared_ptr<CTradeableItem> & hCurSlot) override;
};
