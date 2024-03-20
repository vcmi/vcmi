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

CArtifactsOfHeroBackpack::CArtifactsOfHeroBackpack(size_t slotsColumnsMax, size_t slotsRowsMax)
	: slotsColumnsMax(slotsColumnsMax)
	, slotsRowsMax(slotsRowsMax)
	, backpackPos(0)
{
	setRedrawParent(true);
}

CArtifactsOfHeroBackpack::CArtifactsOfHeroBackpack()
	: CArtifactsOfHeroBackpack(8, 8)
{
	const auto backpackCap = VLC->settings()->getInteger(EGameSettings::HEROES_BACKPACK_CAP);
	auto visibleCapacityMax = slotsRowsMax * slotsColumnsMax;
	if(backpackCap >= 0)
		visibleCapacityMax = visibleCapacityMax > backpackCap ? backpackCap : visibleCapacityMax;

	initAOHbackpack(visibleCapacityMax, backpackCap < 0 || visibleCapacityMax < backpackCap);
}

void CArtifactsOfHeroBackpack::onSliderMoved(int newVal)
{
	if(backpackListBox)
		backpackListBox->resize(getActiveSlotRowsNum());
	backpackPos += newVal;
	auto slot = ArtifactPosition::BACKPACK_START + backpackPos;
	for(auto artPlace : backpack)
	{
		setSlotData(artPlace, slot);
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
	return (curHero->artifactsInBackpack.size() + slotsColumnsMax - 1) / slotsColumnsMax;
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
		const auto pos = Point(slotSizeWithMargin * (artPlaceIdx % slotsColumnsMax),
			slotSizeWithMargin * (artPlaceIdx / slotsColumnsMax));
		backpackSlotsBackgrounds.emplace_back(std::make_shared<CPicture>(ImagePath::builtin("heroWindow/artifactSlotEmpty"), pos));
		artPlace = std::make_shared<CHeroArtPlace>(pos);
		artPlace->setArtifact(nullptr);
		artPlace->setClickPressedCallback(std::bind(&CArtifactsOfHeroBase::clickPrassedArtPlace, this, _1, _2));
		artPlace->setShowPopupCallback(std::bind(&CArtifactsOfHeroBase::showPopupArtPlace, this, _1, _2));
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
			onSliderMoved(static_cast<int>(pos) * slotsColumnsMax - backpackPos);
		};
		backpackListBox = std::make_shared<CListBoxWithCallback>(
			posMoved, onCreate, Point(0, 0), Point(0, 0), slotsRowsMax, 0, 0, 1,
			Rect(slotsColumnsMax * slotSizeWithMargin + sliderPosOffsetX, 0, slotsRowsMax * slotSizeWithMargin - 2, 0));
	}

	pos.w = slots > slotsColumnsMax ? slotsColumnsMax : slots;
	pos.w *= slotSizeWithMargin;
	if(slider)
		pos.w += sliderPosOffsetX + 16; // 16 is slider width. TODO: get it from CListBox directly;
	pos.h = calcRows(slots) * slotSizeWithMargin;
}

size_t CArtifactsOfHeroBackpack::calcRows(size_t slots)
{
	size_t rows = 0;
	if(slotsColumnsMax != 0)
	{
		rows = slots / slotsColumnsMax;
		if(slots % slotsColumnsMax != 0)
			rows += 1;
	}
	return rows;
}

CArtifactsOfHeroQuickBackpack::CArtifactsOfHeroQuickBackpack(const ArtifactPosition filterBySlot)
	: CArtifactsOfHeroBackpack(0, 0)
{
	assert(filterBySlot != ArtifactPosition::FIRST_AVAILABLE);

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
		ArtifactID artInSlotId = ArtifactID::NONE;
		SpellID scrollInSlotSpellId = SpellID::NONE;
		if(auto artInSlot = curHero->getArt(filterBySlot))
		{
			artInSlotId = artInSlot->getTypeId();
			scrollInSlotSpellId = artInSlot->getScrollSpellID();
		}

		std::map<const ArtifactID, const CArtifactInstance*> filteredArts;
		for(auto & slotInfo : curHero->artifactsInBackpack)
			if(slotInfo.artifact->getTypeId() != artInSlotId &&	!slotInfo.artifact->isScroll() &&
				slotInfo.artifact->artType->canBePutAt(curHero, filterBySlot, true))
			{
				filteredArts.insert(std::pair(slotInfo.artifact->getTypeId(), slotInfo.artifact));
			}

		std::map<const SpellID, const CArtifactInstance*> filteredScrolls;
		if(filterBySlot == ArtifactPosition::MISC1 || filterBySlot == ArtifactPosition::MISC2 || filterBySlot == ArtifactPosition::MISC3 ||
			filterBySlot == ArtifactPosition::MISC4 || filterBySlot == ArtifactPosition::MISC5)
		{
			for(auto & slotInfo : curHero->artifactsInBackpack)
			{
				if(slotInfo.artifact->isScroll() && slotInfo.artifact->getScrollSpellID() != scrollInSlotSpellId)
					filteredScrolls.insert(std::pair(slotInfo.artifact->getScrollSpellID(), slotInfo.artifact));
			}
		}

		backpack.clear();
		auto requiredSlots = filteredArts.size() + filteredScrolls.size();
		slotsColumnsMax = ceilf(sqrtf(requiredSlots));
		slotsRowsMax = calcRows(requiredSlots);
		initAOHbackpack(requiredSlots, false);
		auto artPlace = backpack.begin();
		for(auto & art : filteredArts)
			setSlotData(*artPlace++, curHero->getSlotByInstance(art.second));
		for(auto & art : filteredScrolls)
			setSlotData(*artPlace++, curHero->getSlotByInstance(art.second));
	}
}

ArtifactPosition CArtifactsOfHeroQuickBackpack::getFilterSlot()
{
	return filterBySlot;
}

void CArtifactsOfHeroQuickBackpack::selectSlotAt(const Point & position)
{
	for(auto & artPlace : backpack)
		artPlace->selectSlot(artPlace->pos.isInside(position));
}

void CArtifactsOfHeroQuickBackpack::swapSelected()
{
	ArtifactLocation backpackLoc(curHero->id, ArtifactPosition::PRE_FIRST);
	for(auto & artPlace : backpack)
		if(artPlace->isSelected())
		{
			backpackLoc.slot = artPlace->slot;
			break;
		}
	if(backpackLoc.slot != ArtifactPosition::PRE_FIRST && filterBySlot != ArtifactPosition::PRE_FIRST && curHero)
		LOCPLINT->cb->swapArtifacts(backpackLoc, ArtifactLocation(curHero->id, filterBySlot));
}