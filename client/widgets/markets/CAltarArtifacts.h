/*
 * CAltarArtifacts.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../CArtifactsOfHeroAltar.h"
#include "CTradeBase.h"

class CAltarArtifacts : public CExperienceAltar
{
public:
	CAltarArtifacts(const IMarket * market, const CGHeroInstance * hero);
	TExpType calcExpAltarForHero() override;
	void makeDeal() override;
	void sacrificeAll() override;
	void sacrificeBackpack();
	void setSelectedArtifact(const CArtifactInstance * art);
	std::shared_ptr<CArtifactsOfHeroAltar> getAOHset() const;
	ObjectInstanceID getObjId() const;
	void updateSlots();
	void putBackArtifacts();

private:
	ObjectInstanceID altarId;
	const CArtifactSet * altarArtifacts;
	std::shared_ptr<CArtPlace> selectedArt;
	std::shared_ptr<CLabel> selectedCost;
	std::shared_ptr<CButton> sacrificeBackpackButton;
	std::shared_ptr<CArtifactsOfHeroAltar> heroArts;
	std::map<const CArtifactInstance*, std::shared_ptr<CTradeableItem>> tradeSlotsMap;

	const std::vector<Point> posSlotsAltar =
	{
		Point(317, 53), Point(371, 53), Point(425, 53),
		Point(479, 53), Point(533, 53), Point(317, 123),
		Point(371, 123), Point(425, 123), Point(479, 123),
		Point(533, 123), Point(317, 193), Point(371, 193),
		Point(425, 193), Point(479, 193), Point(533, 193),
		Point(317, 263), Point(371, 263), Point(425, 263),
		Point(479, 263), Point(533, 263), Point(398, 333),
		Point(452, 333)
	};

	void onSlotClickPressed(const std::shared_ptr<CTradeableItem> & altarSlot, std::shared_ptr<CTradeableItem> & hCurSlot) override;
	TExpType calcExpCost(const CArtifactInstance * art);
};
