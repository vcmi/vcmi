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
#include "../gui/Shortcut.h"

#include "Buttons.h"
#include "Images.h"
#include "GameSettings.h"
#include "IHandlerBase.h"
#include "ObjectLists.h"

#include "../CPlayerInterface.h"
#include "../../lib/mapObjects/CGHeroInstance.h"

#include "../../CCallback.h"

CArtifactsOfHeroBackpack::CArtifactsOfHeroBackpack(const Point & position)
{
	OBJECT_CONSTRUCTION_CAPTURING(255 - DISPOSE);
	pos += position;
	setRedrawParent(true);

	const auto backpackCap = VLC->settings()->getInteger(EGameSettings::HEROES_BACKPACK_CAP);
	auto visibleCapacityMax = HERO_BACKPACK_WINDOW_SLOT_ROWS * HERO_BACKPACK_WINDOW_SLOT_COLUMNS;
	if(backpackCap >= 0)
		visibleCapacityMax = visibleCapacityMax > backpackCap ? backpackCap : visibleCapacityMax;

	backpack.resize(visibleCapacityMax);
	size_t artPlaceIdx = 0;

	const int slotSizeWithMargin = 46;

	for(int i = 0; i < visibleCapacityMax; i++)
	{
		auto artifactSlotBackground = std::make_shared<CPicture>("heroWindow/artifactSlotEmpty",
			Point(slotSizeWithMargin * (i % HERO_BACKPACK_WINDOW_SLOT_COLUMNS), slotSizeWithMargin * (i / HERO_BACKPACK_WINDOW_SLOT_COLUMNS)));

		backpackSlotsBackgrounds.emplace_back(artifactSlotBackground);
	}

	for(auto & artPlace : backpack)
	{
		artPlace = std::make_shared<CHeroArtPlace>(
			Point(slotSizeWithMargin * (artPlaceIdx % HERO_BACKPACK_WINDOW_SLOT_COLUMNS), slotSizeWithMargin * (artPlaceIdx / HERO_BACKPACK_WINDOW_SLOT_COLUMNS)));
		artPlace->setArtifact(nullptr);
		artPlace->leftClickCallback = std::bind(&CArtifactsOfHeroBase::leftClickArtPlace, this, _1);
		artPlace->rightClickCallback = std::bind(&CArtifactsOfHeroBase::rightClickArtPlace, this, _1);
		artPlaceIdx++;
	}

	if(backpackCap < 0 || visibleCapacityMax < backpackCap)
	{
		auto onCreate = [](size_t index) -> std::shared_ptr<CIntObject>
		{
			return std::make_shared<CIntObject>();
		};
		CListBoxWithCallback::MovedPosCallback posMoved = [this](size_t pos) -> void
		{
			scrollBackpack(static_cast<int>(pos) * HERO_BACKPACK_WINDOW_SLOT_COLUMNS - backpackPos);
		};
		backpackListBox = std::make_shared<CListBoxWithCallback>(
				posMoved, onCreate, Point(0, 0), Point(0, 0), HERO_BACKPACK_WINDOW_SLOT_ROWS, 0, 0, 1,
				Rect(HERO_BACKPACK_WINDOW_SLOT_COLUMNS * slotSizeWithMargin + 10, 0, HERO_BACKPACK_WINDOW_SLOT_ROWS * slotSizeWithMargin - 5, 0));
	}
}

void CArtifactsOfHeroBackpack::swapArtifacts(const ArtifactLocation & srcLoc, const ArtifactLocation & dstLoc)
{
	LOCPLINT->cb->swapArtifacts(srcLoc, dstLoc);
}

void CArtifactsOfHeroBackpack::pickUpArtifact(CHeroArtPlace & artPlace)
{
	LOCPLINT->cb->swapArtifacts(ArtifactLocation(curHero, artPlace.slot),
		ArtifactLocation(curHero, ArtifactPosition::TRANSITION_POS));
}

void CArtifactsOfHeroBackpack::scrollBackpack(int offset)
{
	if(backpackListBox)
		backpackListBox->resize(getActiveSlotLinesNum());
	backpackPos += offset;
	auto slot = ArtifactPosition(GameConstants::BACKPACK_START + backpackPos);
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
		backpackListBox->resize(getActiveSlotLinesNum());
	CArtifactsOfHeroBase::updateBackpackSlots();
}

size_t CArtifactsOfHeroBackpack::getActiveSlotLinesNum()
{
	return (curHero->artifactsInBackpack.size() + HERO_BACKPACK_WINDOW_SLOT_COLUMNS - 1) / HERO_BACKPACK_WINDOW_SLOT_COLUMNS;
}
