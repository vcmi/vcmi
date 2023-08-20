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
	CArtifactsOfHeroBackpack(const Point & position);
	void swapArtifacts(const ArtifactLocation & srcLoc, const ArtifactLocation & dstLoc);
	void pickUpArtifact(CHeroArtPlace & artPlace);
	void scrollBackpack(int offset) override;
	void updateBackpackSlots() override;
	size_t getActiveSlotLinesNum();

private:
	std::shared_ptr<CListBoxWithCallback> backpackListBox;
	std::vector<std::shared_ptr<CPicture>> backpackSlotsBackgrounds;
	const size_t HERO_BACKPACK_WINDOW_SLOT_COLUMNS = 8;
	const size_t HERO_BACKPACK_WINDOW_SLOT_LINES = 8;
};
