/*
 * CArtifactsOfHeroKingdom.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "CArtifactsOfHeroKingdom.h"

#include "Buttons.h"

#include "../CPlayerInterface.h"
#include "../../lib/mapObjects/CGHeroInstance.h"

#include "../../lib/networkPacks/ArtifactLocation.h"

CArtifactsOfHeroKingdom::CArtifactsOfHeroKingdom(ArtPlaceMap ArtWorn, std::vector<ArtPlacePtr> Backpack,
	std::shared_ptr<CButton> leftScroll, std::shared_ptr<CButton> rightScroll)
{
	artWorn = ArtWorn;
	backpack = Backpack;
	leftBackpackRoll = leftScroll;
	rightBackpackRoll = rightScroll;

	for(auto artPlace : artWorn)
	{
		artPlace.second->slot = artPlace.first;
		artPlace.second->setArtifact(ArtifactID(ArtifactID::NONE));
		artPlace.second->setClickPressedCallback(std::bind(&CArtifactsOfHeroBase::clickPressedArtPlace, this, _1, _2));
		artPlace.second->setShowPopupCallback(std::bind(&CArtifactsOfHeroBase::showPopupArtPlace, this, _1, _2));
	}
	enableGesture();
	for(auto artPlace : backpack)
	{
		artPlace->setArtifact(ArtifactID(ArtifactID::NONE));
		artPlace->setClickPressedCallback(std::bind(&CArtifactsOfHeroBase::clickPressedArtPlace, this, _1, _2));
		artPlace->setShowPopupCallback(std::bind(&CArtifactsOfHeroBase::showPopupArtPlace, this, _1, _2));
	}
	leftBackpackRoll->addCallback(std::bind(&CArtifactsOfHeroBase::scrollBackpack, this, -1));
	rightBackpackRoll->addCallback(std::bind(&CArtifactsOfHeroBase::scrollBackpack, this, +1));

	setRedrawParent(true);
}

void CArtifactsOfHeroKingdom::deactivate()
{
	putBackPickedArtifact();
	CArtifactsOfHeroBase::deactivate();
}
