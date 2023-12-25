/*
 * CArtifactsOfHeroMain.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "CArtifactsOfHeroMain.h"

#include "../CPlayerInterface.h"
#include "../../lib/mapObjects/CGHeroInstance.h"

#include "../../CCallback.h"
#include "../../lib/networkPacks/ArtifactLocation.h"

CArtifactsOfHeroMain::CArtifactsOfHeroMain(const Point & position)
{
	init(
		std::bind(&CArtifactsOfHeroBase::clickPrassedArtPlace, this, _1, _2),
		std::bind(&CArtifactsOfHeroBase::showPopupArtPlace, this, _1, _2),
		position,
		std::bind(&CArtifactsOfHeroBase::scrollBackpack, this, _1));
	addGestureCallback(std::bind(&CArtifactsOfHeroBase::gestureArtPlace, this, _1, _2));
}

CArtifactsOfHeroMain::~CArtifactsOfHeroMain()
{
	putBackPickedArtifact();
}

void CArtifactsOfHeroMain::swapArtifacts(const ArtifactLocation & srcLoc, const ArtifactLocation & dstLoc)
{
	LOCPLINT->cb->swapArtifacts(srcLoc, dstLoc);
}

void CArtifactsOfHeroMain::pickUpArtifact(CArtPlace & artPlace)
{
	LOCPLINT->cb->swapArtifacts(ArtifactLocation(curHero->id, artPlace.slot),
		ArtifactLocation(curHero->id, ArtifactPosition::TRANSITION_POS));
}
