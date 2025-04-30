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

#include "../../lib/entities/artifact/CArtifact.h"
#include "../../lib/mapObjects/CGHeroInstance.h"

CArtifactsOfHeroMarket::CArtifactsOfHeroMarket(const Point & position, const int selectionWidth)
{
	init(position, std::bind(&CArtifactsOfHeroBase::scrollBackpack, this, _1));
	setClickPressedArtPlacesCallback(std::bind(&CArtifactsOfHeroBase::clickPressedArtPlace, this, _1, _2));
	for(const auto & [slot, artPlace] : artWorn)
		artPlace->setSelectionWidth(selectionWidth);
	for(auto artPlace : backpack)
		artPlace->setSelectionWidth(selectionWidth);
};

void CArtifactsOfHeroMarket::clickPressedArtPlace(CComponentHolder & artPlace, const Point & cursorPosition)
{
	if(auto ownedPlace = getArtPlace(cursorPosition))
	{
		if(ownedPlace->isLocked())
			return;

		if(const auto art = getArt(ownedPlace->slot))
		{
			if(onSelectArtCallback && art->getType()->isTradable())
			{
				unmarkSlots();
				artPlace.selectSlot(true);
				onSelectArtCallback(ownedPlace.get());
			}
			else
			{
				if(onClickNotTradableCallback)
					onClickNotTradableCallback();
			}
		}
	}
}
