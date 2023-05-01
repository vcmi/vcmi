/*
 * CArtifactsOfHeroAltar.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "CArtifactsOfHeroBase.h"

#include "../../lib/CArtHandler.h"

class CArtifactsOfHeroAltar : public CArtifactsOfHeroBase
{
public:
	std::set<const CArtifactInstance*> artifactsOnAltar;
	ArtifactPosition pickedArtFromSlot;
	CArtifactFittingSet visibleArtSet;

	CArtifactsOfHeroAltar(const Point & position);
	void setHero(const CGHeroInstance * hero) override;
	void updateWornSlots() override;
	void updateBackpackSlots() override;
	void scrollBackpack(int offset) override;
	void pickUpArtifact(CHeroArtPlace & artPlace);
	void swapArtifacts(const ArtifactLocation & srcLoc, const ArtifactLocation & dstLoc);
	void pickedArtMoveToAltar(const ArtifactPosition & slot);
	void deleteFromVisible(const CArtifactInstance * artInst);
};
