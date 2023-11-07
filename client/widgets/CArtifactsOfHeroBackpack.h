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
	CArtifactsOfHeroBackpack();
	CArtifactsOfHeroBackpack(const Point & position);
	void swapArtifacts(const ArtifactLocation & srcLoc, const ArtifactLocation & dstLoc);
	void pickUpArtifact(CHeroArtPlace & artPlace);
	void scrollBackpack(int offset) override;
	void updateBackpackSlots() override;
	size_t getActiveSlotRowsNum();
	size_t getSlotsNum();

protected:
	std::shared_ptr<CListBoxWithCallback> backpackListBox;
	std::vector<std::shared_ptr<CPicture>> backpackSlotsBackgrounds;
	const size_t slots_columns_max = 8;
	const size_t slots_rows_max = 8;
	const int slotSizeWithMargin = 46;
	const int sliderPosOffsetX = 5;

	void initAOHbackpack(size_t slots, bool slider);
};

class CArtifactsOfHeroQuickBackpack : public CArtifactsOfHeroBackpack
{
public:
	CArtifactsOfHeroQuickBackpack(const Point & position, const ArtifactPosition filterBySlot);
	void setHero(const CGHeroInstance * hero);
	ArtifactPosition getFilterSlot();

private:
	ArtifactPosition filterBySlot;
};
