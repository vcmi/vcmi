/*
 * CArtifactsOfHeroMarket.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "CArtifactsOfHeroBase.h"

class CArtifactsOfHeroMarket : public CArtifactsOfHeroBase
{
public:
	std::function<void(const CArtPlace*)> onSelectArtCallback;
	std::function<void()> onClickNotTradableCallback;

	CArtifactsOfHeroMarket(const Point & position, const int selectionWidth);
	void clickPrassedArtPlace(CArtPlace & artPlace, const Point & cursorPosition) override;
};
