/*
 * CArtifactsOfHeroBackpack.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "CArtifactsOfHeroBackpack.h"

#include "../gui/CGuiHandler.h"

#include "Images.h"
#include "GameSettings.h"
#include "ObjectLists.h"

#include "../CPlayerInterface.h"
#include "../../lib/ArtifactUtils.h"
#include "../../lib/mapObjects/CGHeroInstance.h"
#include "../../lib/networkPacks/ArtifactLocation.h"

#include "../../CCallback.h"

CArtifactsOfHeroBackpack::CArtifactsOfHeroBackpack()
{
	setRedrawParent(true);
}

CArtifactsOfHeroBackpack::CArtifactsOfHeroBackpack(const Point & position)
	: CArtifactsOfHeroBackpack()
{
	pos += position;

	const auto backpackCap = VLC->settings()->getInteger(EGameSettings::HEROES_BACKPACK_CAP);
	auto visibleCapacityMax = slots_rows_max * slots_columns_max;
	if(backpackCap >= 0)
		visibleCapacityMax = visibleCapacityMax > backpackCap ? backpackCap : visibleCapacityMax;

	initAOHbackpack(visibleCapacityMax, backpackCap < 0 || visibleCapacityMax < backpackCap);
}

void CArtifactsOfHeroBackpack::swapArtifacts(const ArtifactLocation & srcLoc, const ArtifactLocation & dstLoc)
{
	LOCPLINT->cb->swapArtifacts(srcLoc, dstLoc);
}

void CArtifactsOfHeroBackpack::pickUpArtifact(CArtPlace & artPlace)
{
	LOCPLINT->cb->swapArtifacts(ArtifactLocation(curHero->id, artPlace.slot),
		ArtifactLocation(curHero->id, ArtifactPosition::TRANSITION_POS));
}

void CArtifactsOfHeroBackpack::scrollBackpack(int offset)
{
	if(backpackListBox)
		backpackListBox->resize(getActiveSlotRowsNum());
	backpackPos += offset;
	auto slot = ArtifactPosition::BACKPACK_START + backpackPos;
	for(auto artPlace : backpack)
	{
		setSlotData(artPlace, slot, *curHero);
		slot = slot + 1;
	}
	redraw();
}

void CArtifactsOfHeroBackpack::updateBackpackSlots()
{
	if(backpackListBox)
		backpackListBox->resize(getActiveSlotRowsNum());
	CArtifactsOfHeroBase::updateBackpackSlots();
}

size_t CArtifactsOfHeroBackpack::getActiveSlotRowsNum()
{
	return (curHero->artifactsInBackpack.size() + slots_columns_max - 1) / slots_columns_max;
}

size_t CArtifactsOfHeroBackpack::getSlotsNum()
{
	return backpack.size();
}

void CArtifactsOfHeroBackpack::initAOHbackpack(size_t slots, bool slider)
{
	OBJECT_CONSTRUCTION_CAPTURING(255 - DISPOSE);

	backpack.resize(slots);
	size_t artPlaceIdx = 0;
	for(auto & artPlace : backpack)
	{
		const auto pos = Point(slotSizeWithMargin * (artPlaceIdx % slots_columns_max),
			slotSizeWithMargin * (artPlaceIdx / slots_columns_max));
		backpackSlotsBackgrounds.emplace_back(std::make_shared<CPicture>(ImagePath::builtin("heroWindow/artifactSlotEmpty"), pos));
		artPlace = std::make_shared<CHeroArtPlace>(pos);
		artPlace->setArtifact(nullptr);
		artPlace->setClickPressedCallback(std::bind(&CArtifactsOfHeroBase::leftClickArtPlace, this, _1, _2));
		artPlace->setShowPopupCallback(std::bind(&CArtifactsOfHeroBase::rightClickArtPlace, this, _1, _2));
		artPlaceIdx++;
	}

	if(slider)
	{
		auto onCreate = [](size_t index) -> std::shared_ptr<CIntObject>
		{
			return std::make_shared<CIntObject>();
		};
		CListBoxWithCallback::MovedPosCallback posMoved = [this](size_t pos) -> void
		{
			scrollBackpack(static_cast<int>(pos) * slots_columns_max - backpackPos);
		};
		backpackListBox = std::make_shared<CListBoxWithCallback>(
			posMoved, onCreate, Point(0, 0), Point(0, 0), slots_rows_max, 0, 0, 1,
			Rect(slots_columns_max * slotSizeWithMargin + sliderPosOffsetX, 0, slots_rows_max * slotSizeWithMargin - 2, 0));
	}

	pos.w = slots > slots_columns_max ? slots_columns_max : slots;
	pos.w *= slotSizeWithMargin;
	if(slider)
		pos.w += sliderPosOffsetX + 16; // 16 is slider width. TODO: get it from CListBox directly;

	pos.h = slots / slots_columns_max;
	if(slots % slots_columns_max != 0)
		pos.h += 1;
	pos.h *= slotSizeWithMargin;
}

CArtifactsOfHeroQuickBackpack::CArtifactsOfHeroQuickBackpack(const Point & position, const ArtifactPosition filterBySlot)
{
	pos += position;

	if(!ArtifactUtils::isSlotEquipment(filterBySlot))
		return;

	this->filterBySlot = filterBySlot;
}

void CArtifactsOfHeroQuickBackpack::setHero(const CGHeroInstance * hero)
{
	if(curHero == hero)
		return;
	
	curHero = hero;
	if(curHero)
	{
		std::map<const CArtifact*, const CArtifactInstance*> filteredArts;
		for(auto & slotInfo : curHero->artifactsInBackpack)
		{
			if(slotInfo.artifact->artType->canBePutAt(curHero, filterBySlot, true))
				filteredArts.insert(std::pair(slotInfo.artifact->artType, slotInfo.artifact));
		}
		backpack.clear();
		initAOHbackpack(filteredArts.size(), false);
		auto artPlace = backpack.begin();
		for(auto & art : filteredArts)
			setSlotData(*artPlace++, curHero->getSlotByInstance(art.second), *curHero);
	}
}

ArtifactPosition CArtifactsOfHeroQuickBackpack::getFilterSlot()
{
	return filterBySlot;
}
