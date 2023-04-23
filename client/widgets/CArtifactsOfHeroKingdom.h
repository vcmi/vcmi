/*
 * CArtifactsOfHeroKingdom.h, part of VCMI engine
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

class CArtifactsOfHeroKingdom : public CArtifactsOfHeroBase
{
public:
	CArtifactsOfHeroKingdom(ArtPlaceMap ArtWorn, std::vector<ArtPlacePtr> Backpack,
		std::shared_ptr<CButton> leftScroll, std::shared_ptr<CButton> rightScroll);
	void swapArtifacts(const ArtifactLocation & srcLoc, const ArtifactLocation & dstLoc);
	void pickUpArtifact(CHeroArtPlace & artPlace);
};