/*
 * CArtifactsOfHeroMarket.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "CArtifactsOfHeroMarket.h"

#include "../../lib/mapObjects/CGHeroInstance.h"

CArtifactsOfHeroMarket::CArtifactsOfHeroMarket(const Point & position, const int selectionWidth)
{
	init(position, std::bind(&CArtifactsOfHeroBase::scrollBackpack, this, _1));

	for(const auto & [slot, artPlace] : artWorn)
		artPlace->setSelectionWidth(selectionWidth);
	for(auto artPlace : backpack)
		artPlace->setSelectionWidth(selectionWidth);
};

void CArtifactsOfHeroMarket::clickPrassedArtPlace(CArtPlace & artPlace, const Point & cursorPosition)
{
	if(artPlace.isLocked())
		return;

	if(const auto art = getArt(artPlace.slot))
	{
		if(onSelectArtCallback && art->artType->isTradable())
		{
			unmarkSlots();
			artPlace.selectSlot(true);
			onSelectArtCallback(&artPlace);
		}
		else
		{
			if(onClickNotTradableCallback)
				onClickNotTradableCallback();
		}
	}
}
