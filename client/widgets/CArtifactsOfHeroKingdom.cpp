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

#include "../../CCallback.h"
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
		artPlace.second->setArtifact(nullptr);
		artPlace.second->leftClickCallback = std::bind(&CArtifactsOfHeroBase::leftClickArtPlace, this, _1);
		artPlace.second->showPopupCallback = std::bind(&CArtifactsOfHeroBase::rightClickArtPlace, this, _1);
	}
	for(auto artPlace : backpack)
	{
		artPlace->setArtifact(nullptr);
		artPlace->leftClickCallback = std::bind(&CArtifactsOfHeroBase::leftClickArtPlace, this, _1);
		artPlace->showPopupCallback = std::bind(&CArtifactsOfHeroBase::rightClickArtPlace, this, _1);
	}
	leftBackpackRoll->addCallback(std::bind(&CArtifactsOfHeroBase::scrollBackpack, this, -1));
	rightBackpackRoll->addCallback(std::bind(&CArtifactsOfHeroBase::scrollBackpack, this, +1));

	setRedrawParent(true);
}

CArtifactsOfHeroKingdom::~CArtifactsOfHeroKingdom()
{
	putBackPickedArtifact();
}

void CArtifactsOfHeroKingdom::swapArtifacts(const ArtifactLocation & srcLoc, const ArtifactLocation & dstLoc)
{
	LOCPLINT->cb->swapArtifacts(srcLoc, dstLoc);
}

void CArtifactsOfHeroKingdom::pickUpArtifact(CHeroArtPlace & artPlace)
{
	LOCPLINT->cb->swapArtifacts(ArtifactLocation(curHero->id, artPlace.slot),
		ArtifactLocation(curHero->id, ArtifactPosition::TRANSITION_POS));
}

