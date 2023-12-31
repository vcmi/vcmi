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

CArtifactsOfHeroMarket::CArtifactsOfHeroMarket(const Point & position)
{
	init(
		std::bind(&CArtifactsOfHeroBase::clickPrassedArtPlace, this, _1, _2),
		std::bind(&CArtifactsOfHeroBase::showPopupArtPlace, this, _1, _2),
		position,
		std::bind(&CArtifactsOfHeroMarket::scrollBackpack, this, _1));
};

void CArtifactsOfHeroMarket::scrollBackpack(int offset)
{
	CArtifactsOfHeroBase::scrollBackpackForArtSet(offset, *curHero);

	// We may have highlight on one of backpack artifacts
	if(selectArtCallback)
	{
		for(auto & artPlace : backpack)
		{
			if(artPlace->isSelected())
			{
				selectArtCallback(artPlace.get());
				break;
			}
		}
	}
	redraw();
}