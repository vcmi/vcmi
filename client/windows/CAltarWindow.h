/*
 * CAltarWindow.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../widgets/CWindowWithArtifacts.h"
#include "../widgets/markets/CAltarArtifacts.h"
#include "../widgets/markets/CAltarCreatures.h"

class CExperienceAltar;

class CAltarWindow : public CWindowWithArtifacts, public IGarrisonHolder
{
public:
	void updateExpToLevel();
	void updateGarrisons() override;
	bool holdsGarrison(const CArmedInstance * army) override;
	virtual const CGHeroInstance * getHero() const = 0;

	void artifactMoved(const ArtifactLocation & srcLoc, const ArtifactLocation & destLoc, bool withRedraw) override;

	std::shared_ptr<CExperienceAltar> altar;
};
