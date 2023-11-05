/*
 * CAltar.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../widgets/CArtifactsOfHeroAltar.h"
#include "../widgets/CTradeBase.h"

class CSlider;

class CAltar : public CTradeBase, public CIntObject
{
public:
	std::shared_ptr<CLabel> expToLevel;
	std::shared_ptr<CLabel> expForHero;
	std::shared_ptr<CButton> sacrificeAllButton;

	CAltar(const IMarket * market, const CGHeroInstance * hero);
	virtual ~CAltar() = default;
	virtual void sacrificeAll() = 0;
	virtual void deselect();
	virtual TExpType calcExpAltarForHero() = 0;
};

class CAltarArtifacts : public CAltar
{
public:
	CAltarArtifacts(const IMarket * market, const CGHeroInstance * hero);
	TExpType calcExpAltarForHero() override;
	void makeDeal() override;
	void sacrificeAll() override;
	void sacrificeBackpack();
	void setSelectedArtifact(const CArtifactInstance * art);
	void moveArtToAltar(std::shared_ptr<CTradeableItem>, const CArtifactInstance * art);
	std::shared_ptr<CArtifactsOfHeroAltar> getAOHset() const;

private:
	std::shared_ptr<CArtPlace> selectedArt;
	std::shared_ptr<CLabel> selectedCost;
	std::shared_ptr<CButton> sacrificeBackpackButton;
	std::shared_ptr<CArtifactsOfHeroAltar> arts;

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

	bool putArtOnAltar(std::shared_ptr<CTradeableItem> altarSlot, const CArtifactInstance * art);
};

class CAltarCreatures : public CAltar
{
public:
	CAltarCreatures(const IMarket * market, const CGHeroInstance * hero);
	void updateGarrison();
	void deselect() override;
	TExpType calcExpAltarForHero() override;
	void makeDeal() override;
	void sacrificeAll() override;
	void updateAltarSlot(std::shared_ptr<CTradeableItem> slot);

private:
	std::shared_ptr<CButton> maxUnits;
	std::shared_ptr<CSlider> unitsSlider;
	std::vector<int> unitsOnAltar;
	std::vector<int> expPerUnit;
	std::shared_ptr<CLabel> lSubtitle, rSubtitle;

	const std::vector<Point> posSlotsAltar =
	{
		Point(334, 110), Point(417, 110), Point(500, 110),
		Point(334, 208), Point(417, 208), Point(500, 208),
		Point(417, 306)
	};
	const std::vector<Point> posSlotsHero =
	{
		Point(45, 110), Point(128, 110), Point(211, 110),
		Point(45, 208), Point(128, 208), Point(211, 208),
		Point(128, 306)
	};

	void readExpValues();
	void updateControls();
	void updateSubtitlesForSelected();
};
