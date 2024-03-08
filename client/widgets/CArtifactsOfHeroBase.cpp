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

#include "../gui/CGuiHandler.h"
#include "../gui/Shortcut.h"

#include "Buttons.h"

#include "../CPlayerInterface.h"
#include "../CGameInfo.h"

#include "../../CCallback.h"

#include "../../lib/ArtifactUtils.h"
#include "../../lib/mapObjects/CGHeroInstance.h"
#include "../../lib/networkPacks/ArtifactLocation.h"

CArtifactsOfHeroBase::CArtifactsOfHeroBase()
	: curHero(nullptr),
	putBackPickedArtCallback(nullptr)
{
}

void CArtifactsOfHeroBase::putBackPickedArtifact()
{
	// Artifact located in artifactsTransitionPos should be returned
	if(getPickedArtifact())
	{
		auto slot = ArtifactUtils::getArtAnyPosition(curHero, curHero->artifactsTransitionPos.begin()->artifact->getTypeId());
		if(slot == ArtifactPosition::PRE_FIRST)
		{
			LOCPLINT->cb->eraseArtifactByClient(ArtifactLocation(curHero->id, ArtifactPosition::TRANSITION_POS));
		}
		else
		{
			LOCPLINT->cb->swapArtifacts(ArtifactLocation(curHero->id, ArtifactPosition::TRANSITION_POS), ArtifactLocation(curHero->id, slot));
		}
	}
	if(putBackPickedArtCallback)
		putBackPickedArtCallback();
}

void CArtifactsOfHeroBase::setPutBackPickedArtifactCallback(PutBackPickedArtCallback callback)
{
	putBackPickedArtCallback = callback;
}

void CArtifactsOfHeroBase::init(
	const CArtPlace::ClickFunctor & onClickPressedCallback,
	const CArtPlace::ClickFunctor & onShowPopupCallback,
	const Point & position,
	const BpackScrollFunctor & scrollCallback)
{
	// CArtifactsOfHeroBase::init may be transform to CArtifactsOfHeroBase::CArtifactsOfHeroBase if OBJECT_CONSTRUCTION_CAPTURING is removed
	OBJECT_CONSTRUCTION_CAPTURING(255 - DISPOSE);
	pos += position;
	for(int g = 0; g < ArtifactPosition::BACKPACK_START; g++)
	{
		artWorn[ArtifactPosition(g)] = std::make_shared<CHeroArtPlace>(slotPos[g]);
	}
	backpack.clear();
	for(int s = 0; s < 5; s++)
	{
		auto artPlace = std::make_shared<CHeroArtPlace>(Point(403 + 46 * s, 365));
		backpack.push_back(artPlace);
	}
	for(auto artPlace : artWorn)
	{
		artPlace.second->slot = artPlace.first;
		artPlace.second->setArtifact(nullptr);
		artPlace.second->setClickPressedCallback(onClickPressedCallback);
		artPlace.second->setShowPopupCallback(onShowPopupCallback);
	}
	for(auto artPlace : backpack)
	{
		artPlace->setArtifact(nullptr);
		artPlace->setClickPressedCallback(onClickPressedCallback);
		artPlace->setShowPopupCallback(onShowPopupCallback);
	}
	leftBackpackRoll = std::make_shared<CButton>(Point(379, 364), AnimationPath::builtin("hsbtns3.def"), CButton::tooltip(),
		[scrollCallback](){scrollCallback(true);}, EShortcut::MOVE_LEFT);
	rightBackpackRoll = std::make_shared<CButton>(Point(632, 364), AnimationPath::builtin("hsbtns5.def"), CButton::tooltip(),
		[scrollCallback](){scrollCallback(false);}, EShortcut::MOVE_RIGHT);
	leftBackpackRoll->block(true);
	rightBackpackRoll->block(true);

	setRedrawParent(true);
}

void CArtifactsOfHeroBase::clickPrassedArtPlace(CArtPlace & artPlace, const Point & cursorPosition)
{
	if(clickPressedCallback)
		clickPressedCallback(*this, artPlace, cursorPosition);
}

void CArtifactsOfHeroBase::showPopupArtPlace(CArtPlace & artPlace, const Point & cursorPosition)
{
	if(showPopupCallback)
		showPopupCallback(*this, artPlace, cursorPosition);
}

void CArtifactsOfHeroBase::gestureArtPlace(CArtPlace & artPlace, const Point & cursorPosition)
{
	if(gestureCallback)
		gestureCallback(*this, artPlace, cursorPosition);
}

void CArtifactsOfHeroBase::setHero(const CGHeroInstance * hero)
{
	curHero = hero;

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
	LOCPLINT->cb->scrollBackpackArtifacts(curHero->id, left);
}

void CArtifactsOfHeroBase::markPossibleSlots(const CArtifactInstance * art, bool assumeDestRemoved)
{
	for(auto artPlace : artWorn)
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
	if(ArtifactUtils::isSlotEquipment(slot))
	{
		if(artWorn.find(slot) == artWorn.end())
		{
			logGlobal->error("CArtifactsOfHero::getArtPlace: invalid slot %d", slot);
			return nullptr;
		}
		return artWorn[slot];
	}
	if(ArtifactUtils::isSlotBackpack(slot))
	{
		for(ArtPlacePtr artPlace : backpack)
			if(artPlace->slot == slot)
				return artPlace;
		return nullptr;
	}
	else
	{
		return nullptr;
	}
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
}

void CArtifactsOfHeroBase::updateSlot(const ArtifactPosition & slot)
{
	setSlotData(getArtPlace(slot), slot);
}

const CArtifactInstance * CArtifactsOfHeroBase::getPickedArtifact()
{
	// Returns only the picked up artifact. Not just highlighted like in the trading window.
	if(!curHero || curHero->artifactsTransitionPos.empty())
		return nullptr;
	else
		return curHero->getArt(ArtifactPosition::TRANSITION_POS);
}

void CArtifactsOfHeroBase::addGestureCallback(CArtPlace::ClickFunctor callback)
{
	for(auto & artPlace : artWorn)
	{
		artPlace.second->setGestureCallback(callback);
		artPlace.second->addUsedEvents(GESTURE);
	}
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
		artPlace->lockSlot(slotInfo->locked);
		artPlace->setArtifact(slotInfo->artifact);
		if(!slotInfo->artifact->isCombined())
		{
			// If the artifact is part of at least one combined artifact, add additional information
			std::map<const CArtifact*, int> arts;
			for(const auto combinedArt : slotInfo->artifact->artType->getPartOf())
			{
				arts.insert(std::pair(combinedArt, 0));
				for(const auto part : combinedArt->getConstituents())
				{
					if(curHero->hasArt(part->getId(), false))
						arts.at(combinedArt)++;
				}
			}
			artPlace->addCombinedArtInfo(arts);
		}
	}
	else
	{
		artPlace->setArtifact(nullptr);
	}
}
