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
	: visibleArtSet(ArtBearer::ArtBearer::HERO)
{
	init(
		std::bind(&CArtifactsOfHeroBase::clickPrassedArtPlace, this, _1, _2),
		std::bind(&CArtifactsOfHeroBase::showPopupArtPlace, this, _1, _2),
		position,
		std::bind(&CArtifactsOfHeroAltar::scrollBackpack, this, _1));
	pickedArtFromSlot = ArtifactPosition::PRE_FIRST;

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

void CArtifactsOfHeroAltar::setHero(const CGHeroInstance * hero)
{
	if(hero)
	{
		visibleArtSet.artifactsWorn = hero->artifactsWorn;
		visibleArtSet.artifactsInBackpack = hero->artifactsInBackpack;
		CArtifactsOfHeroBase::setHero(hero);
	}
}

void CArtifactsOfHeroAltar::updateWornSlots()
{
	for(auto place : artWorn)
		setSlotData(getArtPlace(place.first), place.first, visibleArtSet);
}

void CArtifactsOfHeroAltar::updateBackpackSlots()
{
	for(auto artPlace : backpack)
		setSlotData(getArtPlace(artPlace->slot), artPlace->slot, visibleArtSet);
}

void CArtifactsOfHeroAltar::scrollBackpack(int offset)
{
	CArtifactsOfHeroBase::scrollBackpackForArtSet(offset, visibleArtSet);
	redraw();
}

void CArtifactsOfHeroAltar::pickUpArtifact(CArtPlace & artPlace)
{
	if(const auto art = artPlace.getArt())
	{
		pickedArtFromSlot = artPlace.slot;
		artPlace.setArtifact(nullptr);
		deleteFromVisible(art);
		if(ArtifactUtils::isSlotBackpack(pickedArtFromSlot))
			pickedArtFromSlot = curHero->getSlotByInstance(art);
		assert(pickedArtFromSlot != ArtifactPosition::PRE_FIRST);
		LOCPLINT->cb->swapArtifacts(ArtifactLocation(curHero->id, pickedArtFromSlot), ArtifactLocation(curHero->id, ArtifactPosition::TRANSITION_POS));
	}
}

void CArtifactsOfHeroAltar::swapArtifacts(const ArtifactLocation & srcLoc, const ArtifactLocation & dstLoc)
{
	LOCPLINT->cb->swapArtifacts(srcLoc, dstLoc);
	const auto pickedArtInst = curHero->getArt(ArtifactPosition::TRANSITION_POS);
	assert(pickedArtInst);
	visibleArtSet.putArtifact(dstLoc.slot, const_cast<CArtifactInstance*>(pickedArtInst));
}

void CArtifactsOfHeroAltar::pickedArtMoveToAltar(const ArtifactPosition & slot)
{
	if(ArtifactUtils::isSlotBackpack(slot) || ArtifactUtils::isSlotEquipment(slot) || slot == ArtifactPosition::TRANSITION_POS)
	{
		assert(curHero->getSlot(slot)->getArt());
		LOCPLINT->cb->swapArtifacts(ArtifactLocation(curHero->id, slot), ArtifactLocation(curHero->id, pickedArtFromSlot));
		pickedArtFromSlot = ArtifactPosition::PRE_FIRST;
	}
}

void CArtifactsOfHeroAltar::deleteFromVisible(const CArtifactInstance * artInst)
{
	const auto slot = visibleArtSet.getSlotByInstance(artInst);
	visibleArtSet.removeArtifact(slot);
	if(ArtifactUtils::isSlotBackpack(slot))
	{
		scrollBackpackForArtSet(0, visibleArtSet);
	}
	else
	{
		for(const auto & part : artInst->getPartsInfo())
		{
			if(part.slot != ArtifactPosition::PRE_FIRST)
				getArtPlace(part.slot)->setArtifact(nullptr);
		}
	}
}
