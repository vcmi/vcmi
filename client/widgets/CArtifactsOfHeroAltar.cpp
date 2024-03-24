/*
 * CArtifactsOfHeroAltar.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "CArtifactsOfHeroAltar.h"

#include "Buttons.h"
#include "../CPlayerInterface.h"

#include "../../CCallback.h"

#include "../../lib/ArtifactUtils.h"
#include "../../lib/mapObjects/CGHeroInstance.h"
#include "../../lib/networkPacks/ArtifactLocation.h"

CArtifactsOfHeroAltar::CArtifactsOfHeroAltar(const Point & position)
{
	init(
		std::bind(&CArtifactsOfHeroBase::clickPrassedArtPlace, this, _1, _2),
		std::bind(&CArtifactsOfHeroBase::showPopupArtPlace, this, _1, _2),
		position,
		std::bind(&CArtifactsOfHeroBase::scrollBackpack, this, _1));

	// The backpack is in the altar window above and to the right
	for(auto & slot : backpack)
		slot->moveBy(Point(2, -1));
	leftBackpackRoll->moveBy(Point(2, -1));
	rightBackpackRoll->moveBy(Point(2, -1));
};

CArtifactsOfHeroAltar::~CArtifactsOfHeroAltar()
{
	putBackPickedArtifact();
}
