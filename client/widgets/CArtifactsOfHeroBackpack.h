/*
 * CArtifactsOfHeroBackpack.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "CArtifactsOfHeroBase.h"

VCMI_LIB_NAMESPACE_BEGIN

struct ArtifactLocation;

VCMI_LIB_NAMESPACE_END

class CListBoxWithCallback;

class CArtifactsOfHeroBackpack : public CArtifactsOfHeroBase
{
public:
	CArtifactsOfHeroBackpack(size_t slotsColumnsMax, size_t slotsRowsMax);
	CArtifactsOfHeroBackpack();
	void onSliderMoved(int newVal);
	void updateBackpackSlots() override;
	size_t getActiveSlotRowsNum();
	size_t getSlotsNum();

protected:
	std::shared_ptr<CListBoxWithCallback> backpackListBox;
	std::vector<std::shared_ptr<CPicture>> backpackSlotsBackgrounds;
	size_t slotsColumnsMax;
	size_t slotsRowsMax;
	const int slotSizeWithMargin = 46;
	const int sliderPosOffsetX = 5;
	int backpackPos; // Position to display artifacts in heroes backpack

	void initAOHbackpack(size_t slots, bool slider);
	size_t calcRows(size_t slots);
};

class CArtifactsOfHeroQuickBackpack : public CArtifactsOfHeroBackpack
{
public:
	CArtifactsOfHeroQuickBackpack(const ArtifactPosition filterBySlot);
	void setHero(const CGHeroInstance * hero);
	ArtifactPosition getFilterSlot();
	void selectSlotAt(const Point & position);
	void swapSelected();

private:
	ArtifactPosition filterBySlot;
};
