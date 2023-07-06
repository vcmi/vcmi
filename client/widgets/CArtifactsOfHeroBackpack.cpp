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
#include "GameSettings.h"
#include "IHandlerBase.h"

#include "../CPlayerInterface.h"

#include "../../CCallback.h"

CArtifactsOfHeroBackpack::CArtifactsOfHeroBackpack(const Point & position, DestroyHandler destroyThisCallback)
{
	OBJECT_CONSTRUCTION_CAPTURING(255 - DISPOSE);
	pos += position;

	const auto backpackCap = VLC->settings()->getInteger(EGameSettings::HEROES_BACKPACK_CAP);
	auto visibleCapasityMax = HERO_BACKPACK_WINDOW_SLOT_LINES * HERO_BACKPACK_WINDOW_SLOT_COLUMNS;
	if(backpackCap >= 0)
		visibleCapasityMax = visibleCapasityMax > backpackCap ? backpackCap : visibleCapasityMax;

	backpack.resize(visibleCapasityMax);
	size_t artPlaceIdx = 0;
	for(auto & artPlace : backpack)
	{
		artPlace = std::make_shared<CHeroArtPlace>(
			Point(46 * (artPlaceIdx % HERO_BACKPACK_WINDOW_SLOT_COLUMNS), 46 * (artPlaceIdx / HERO_BACKPACK_WINDOW_SLOT_COLUMNS)));
		artPlace->setArtifact(nullptr);
		artPlace->leftClickCallback = std::bind(&CArtifactsOfHeroBase::leftClickArtPlace, this, _1);
		artPlace->rightClickCallback = std::bind(&CArtifactsOfHeroBase::rightClickArtPlace, this, _1);
		artPlaceIdx++;
	}
	if(backpackCap < 0 || visibleCapasityMax < backpackCap)
	{
		auto scrollHandler = std::bind(&CArtifactsOfHeroBase::scrollBackpack, this, _1);
		leftBackpackRoll = std::make_shared<CButton>(Point(-20, 0), "hsbtns3.def", CButton::tooltip(), [scrollHandler]() { scrollHandler(-1); }, EShortcut::MOVE_LEFT);
		rightBackpackRoll = std::make_shared<CButton>(Point(368, 318), "hsbtns5.def", CButton::tooltip(), [scrollHandler]() { scrollHandler(+1); }, EShortcut::MOVE_RIGHT);
		leftBackpackRoll->block(true);
		rightBackpackRoll->block(true);
	}
	this->destroyThisCallback = destroyThisCallback;
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

void CArtifactsOfHeroBackpack::destroyThis()
{
	if(destroyThisCallback)
		destroyThisCallback();
}
