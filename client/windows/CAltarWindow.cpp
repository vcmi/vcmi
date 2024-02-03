/*
 * CAltarWindow.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "CAltarWindow.h"

#include "../widgets/TextControls.h"

#include "../CGameInfo.h"

#include "../lib/networkPacks/ArtifactLocation.h"
#include "../../lib/CHeroHandler.h"
#include "../../lib/mapObjects/CGHeroInstance.h"

void CAltarWindow::updateExpToLevel()
{
	assert(altar);
	altar->expToLevel->setText(std::to_string(CGI->heroh->reqExp(CGI->heroh->level(altar->hero->exp) + 1) - altar->hero->exp));
}

void CAltarWindow::updateGarrisons()
{
	if(auto altarCreatures = std::static_pointer_cast<CAltarCreatures>(altar))
		altarCreatures->updateSlots();
}

bool CAltarWindow::holdsGarrison(const CArmedInstance * army)
{
	return getHero() == army;
}

void CAltarWindow::artifactMoved(const ArtifactLocation & srcLoc, const ArtifactLocation & destLoc, bool withRedraw)
{
	if(!getState().has_value())
		return;

	if(auto altarArtifacts = std::static_pointer_cast<CAltarArtifacts>(altar))
	{
		if(srcLoc.artHolder == altarArtifacts->getObjId() || destLoc.artHolder == altarArtifacts->getObjId())
			altarArtifacts->updateSlots();

		if(const auto pickedArt = getPickedArtifact())
			altarArtifacts->setSelectedArtifact(pickedArt);
		else
			altarArtifacts->setSelectedArtifact(nullptr);
	}
	CWindowWithArtifacts::artifactMoved(srcLoc, destLoc, withRedraw);
}
