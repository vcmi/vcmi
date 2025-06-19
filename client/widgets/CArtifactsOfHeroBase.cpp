/*
 * CArtifactsOfHeroBase.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "CArtifactsOfHeroBase.h"

#include "../GameEngine.h"
#include "../GameInstance.h"
#include "../gui/Shortcut.h"

#include "Buttons.h"

#include "../CPlayerInterface.h"

#include "../../lib/callback/CCallback.h"
#include "../../lib/entities/artifact/ArtifactUtils.h"
#include "../../lib/entities/artifact/CArtifact.h"
#include "../../lib/entities/artifact/CArtifactFittingSet.h"
#include "../../lib/mapObjects/CGHeroInstance.h"
#include "../../lib/networkPacks/ArtifactLocation.h"

CArtifactsOfHeroBase::CArtifactsOfHeroBase()
	: curHero(nullptr)
{
}

void CArtifactsOfHeroBase::putBackPickedArtifact()
{
	// Artifact located in artifactsTransitionPos should be returned
	if(const auto art = getPickedArtifact())
	{
		auto slot = ArtifactUtils::getArtAnyPosition(curHero, art->getTypeId());
		if(slot == ArtifactPosition::PRE_FIRST)
		{
			GAME->interface()->cb->eraseArtifactByClient(ArtifactLocation(curHero->id, ArtifactPosition::TRANSITION_POS));
		}
		else
		{
			GAME->interface()->cb->swapArtifacts(ArtifactLocation(curHero->id, ArtifactPosition::TRANSITION_POS), ArtifactLocation(curHero->id, slot));
		}
	}
}

void CArtifactsOfHeroBase::init(
	const Point & position,
	const BpackScrollFunctor & scrollCallback)
{
	// CArtifactsOfHeroBase::init may be transform to CArtifactsOfHeroBase::CArtifactsOfHeroBase if OBJECT_CONSTRUCTION is removed
	OBJECT_CONSTRUCTION;
	pos += position;
	for(int g = 0; g < ArtifactPosition::BACKPACK_START; g++)
	{
		artWorn[ArtifactPosition(g)] = std::make_shared<CArtPlace>(slotPos[g]);
	}
	backpack.clear();
	for(int s = 0; s < 5; s++)
	{
		auto artPlace = std::make_shared<CArtPlace>(Point(403 + 46 * s, 365));
		backpack.push_back(artPlace);
	}
	for(auto & artPlace : artWorn)
	{
		artPlace.second->slot = artPlace.first;
		artPlace.second->setArtifact(ArtifactID(ArtifactID::NONE));
	}
	for(const auto & artPlace : backpack)
	{
		artPlace->setArtifact(ArtifactID(ArtifactID::NONE));
	}
	leftBackpackRoll = std::make_shared<CButton>(Point(379, 364), AnimationPath::builtin("hsbtns3.def"), CButton::tooltip(),
		[scrollCallback](){scrollCallback(true);}, EShortcut::MOVE_LEFT);
	rightBackpackRoll = std::make_shared<CButton>(Point(632, 364), AnimationPath::builtin("hsbtns5.def"), CButton::tooltip(),
		[scrollCallback](){scrollCallback(false);}, EShortcut::MOVE_RIGHT);
	leftBackpackRoll->block(true);
	rightBackpackRoll->block(true);

	backpackScroller = std::make_shared<BackpackScroller>(this, Rect(380, 30, 278, 382));
	backpackScroller->setScrollingEnabled(false);

	setRedrawParent(true);
}

void CArtifactsOfHeroBase::setClickPressedArtPlacesCallback(const CArtPlace::ClickFunctor & callback) const
{
	for(const auto & [slot, artPlace] : artWorn)
		artPlace->setClickPressedCallback(callback);
	for(const auto & artPlace : backpack)
		artPlace->setClickPressedCallback(callback);
}

void CArtifactsOfHeroBase::setShowPopupArtPlacesCallback(const CArtPlace::ClickFunctor & callback) const
{
	for(const auto & [slot, artPlace] : artWorn)
		artPlace->setShowPopupCallback(callback);
	for(const auto & artPlace : backpack)
		artPlace->setShowPopupCallback(callback);
}

void CArtifactsOfHeroBase::clickPressedArtPlace(CComponentHolder & artPlace, const Point & cursorPosition)
{
	if(auto ownedPlace = getArtPlace(cursorPosition))
	{
		if(ownedPlace->isLocked())
			return;

		if(clickPressedCallback)
			clickPressedCallback(*ownedPlace, cursorPosition);
	}
}

void CArtifactsOfHeroBase::showPopupArtPlace(CComponentHolder & artPlace, const Point & cursorPosition)
{
	if(auto ownedPlace = getArtPlace(cursorPosition))
	{
		if(ownedPlace->isLocked())
			return;

		if(showPopupCallback)
			showPopupCallback(*ownedPlace, cursorPosition);
	}
}

void CArtifactsOfHeroBase::gestureArtPlace(CComponentHolder & artPlace, const Point & cursorPosition)
{
	if(auto ownedPlace = getArtPlace(cursorPosition))
	{
		if(ownedPlace->isLocked())
			return;

		if(gestureCallback)
			gestureCallback(*ownedPlace, cursorPosition);
	}
}

void CArtifactsOfHeroBase::setHero(const CGHeroInstance * hero)
{
	curHero = hero;
	if (!hero)
		return;

	for(auto slot : artWorn)
	{
		setSlotData(slot.second, slot.first);
	}
	updateBackpackSlots();
}

const CGHeroInstance * CArtifactsOfHeroBase::getHero() const
{
	return curHero;
}

void CArtifactsOfHeroBase::scrollBackpack(bool left)
{
	GAME->interface()->cb->scrollBackpackArtifacts(curHero->id, left);
}

void CArtifactsOfHeroBase::markPossibleSlots(const CArtifact * art, bool assumeDestRemoved)
{
	for(const auto & artPlace : artWorn)
		artPlace.second->selectSlot(art->canBePutAt(curHero, artPlace.second->slot, assumeDestRemoved));
}

void CArtifactsOfHeroBase::unmarkSlots()
{
	for(auto & artPlace : artWorn)
		artPlace.second->selectSlot(false);

	for(auto & artPlace : backpack)
		artPlace->selectSlot(false);
}

CArtifactsOfHeroBase::ArtPlacePtr CArtifactsOfHeroBase::getArtPlace(const ArtifactPosition & slot)
{
	if(ArtifactUtils::isSlotEquipment(slot) && artWorn.find(slot) != artWorn.end())
		return artWorn[slot];
	if(ArtifactUtils::isSlotBackpack(slot) && slot - ArtifactPosition::BACKPACK_START < backpack.size())
		return(backpack[slot - ArtifactPosition::BACKPACK_START]);
	logGlobal->error("CArtifactsOfHero::getArtPlace: invalid slot %d", slot);
	return nullptr;
}

CArtifactsOfHeroBase::ArtPlacePtr CArtifactsOfHeroBase::getArtPlace(const Point & cursorPosition)
{
	for(const auto & [slot, artPlace] : artWorn)
	{
		if(artPlace->pos.isInside(cursorPosition))
			return artPlace;
	}
	for(const auto & artPlace : backpack)
	{
		if(artPlace->pos.isInside(cursorPosition))
			return artPlace;
	}
	return nullptr;
}

void CArtifactsOfHeroBase::updateWornSlots()
{
	for(auto place : artWorn)
		updateSlot(place.first);
}

void CArtifactsOfHeroBase::updateBackpackSlots()
{
	ArtifactPosition slot = ArtifactPosition::BACKPACK_START;
	for(const auto & artPlace : backpack)
	{
		setSlotData(artPlace, slot);
		slot = slot + 1;
	}
	auto scrollingPossible = static_cast<int>(curHero->artifactsInBackpack.size()) > backpack.size();
	// Blocking scrolling if there is not enough artifacts to scroll
	if(leftBackpackRoll)
		leftBackpackRoll->block(!scrollingPossible);
	if(rightBackpackRoll)
		rightBackpackRoll->block(!scrollingPossible);
	if (backpackScroller)
		backpackScroller->setScrollingEnabled(scrollingPossible);
}

void CArtifactsOfHeroBase::updateSlot(const ArtifactPosition & slot)
{
	setSlotData(getArtPlace(slot), slot);
}

const CArtifactInstance * CArtifactsOfHeroBase::getPickedArtifact()
{
	// Returns only the picked up artifact. Not just highlighted like in the trading window.
	if(curHero)
		return curHero->getArt(ArtifactPosition::TRANSITION_POS);
	else
		return nullptr;
}

void CArtifactsOfHeroBase::enableGesture()
{
	for(auto & artPlace : artWorn)
	{
		artPlace.second->setGestureCallback(std::bind(&CArtifactsOfHeroBase::gestureArtPlace, this, _1, _2));
		artPlace.second->addUsedEvents(GESTURE);
	}
}

const CArtifactInstance * CArtifactsOfHeroBase::getArt(const ArtifactPosition & slot) const
{
	return curHero ? curHero->getArt(slot) : nullptr;
}

void CArtifactsOfHeroBase::enableKeyboardShortcuts()
{
	addUsedEvents(AEventsReceiver::KEYBOARD);
}

void CArtifactsOfHeroBase::setSlotData(ArtPlacePtr artPlace, const ArtifactPosition & slot)
{
	// Spurious call from artifactMoved in attempt to update hidden backpack slot
	if(!artPlace && ArtifactUtils::isSlotBackpack(slot))
	{
		return;
	}

	artPlace->slot = slot;
	if(auto slotInfo = curHero->getSlot(slot))
	{
		const auto curArt = slotInfo->getArt();

		artPlace->lockSlot(slotInfo->locked);
		artPlace->setArtifact(curArt->getTypeId(), curArt->getScrollSpellID());
		if(slotInfo->locked)
			return;

		// If the artifact has charges, add charges information
		if(curArt->getType()->isCharged())
			artPlace->addChargedArtInfo(curArt->getCharges());

		if(curArt->isCombined())
			return;

		// If the artifact is part of at least one combined artifact, add additional information
		std::map<const ArtifactID, std::vector<ArtifactID>> arts;
		for(const auto combinedArt : slotInfo->getArt()->getType()->getPartOf())
		{
			assert(combinedArt->isCombined());
			arts.try_emplace(combinedArt->getId());
			CArtifactFittingSet fittingSet(*curHero);
			for(const auto part : combinedArt->getConstituents())
			{
				const auto partSlot = fittingSet.getArtPos(part->getId(), false, false);
				if(partSlot != ArtifactPosition::PRE_FIRST)
				{
					arts.at(combinedArt->getId()).emplace_back(part->getId());
					fittingSet.lockSlot(partSlot);
				}
			}
		}
		artPlace->addCombinedArtInfo(arts);
	}
	else
	{
		artPlace->setArtifact(ArtifactID(ArtifactID::NONE));
	}
}

BackpackScroller::BackpackScroller(CArtifactsOfHeroBase * owner, const Rect & dimensions)
	: Scrollable(0, Point(), Orientation::HORIZONTAL)
	, owner(owner)
{
	pos = dimensions + pos.topLeft();
	setPanningStep(46);
}

void BackpackScroller::scrollBy(int distance)
{
	if (distance != 0)
		owner->scrollBackpack(distance < 0);
}
