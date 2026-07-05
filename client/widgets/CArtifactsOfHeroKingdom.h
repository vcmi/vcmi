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

struct ArtifactLocation;

class CArtifactsOfHeroKingdom : public CArtifactsOfHeroBase
{
public:
	CArtifactsOfHeroKingdom() = delete;
	CArtifactsOfHeroKingdom(ArtPlaceMap ArtWorn, std::vector<ArtPlacePtr> Backpack,
		std::shared_ptr<CButton> leftScroll, std::shared_ptr<CButton> rightScroll);

	void deactivate() override;
};
