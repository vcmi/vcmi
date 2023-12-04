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

#include "../widgets/markets/CAltarArtifacts.h"
#include "../widgets/markets/CAltarCreatures.h"
#include "../widgets/CWindowWithArtifacts.h"
#include "CWindowObject.h"

class CAltarWindow : public CWindowObject, public CWindowWithArtifacts, public IGarrisonHolder
{
public:
	CAltarWindow(const IMarket * market, const CGHeroInstance * hero, const std::function<void()> & onWindowClosed, EMarketMode mode);
	void updateExpToLevel();
	void updateGarrisons() override;
	bool holdsGarrison(const CArmedInstance * army) override;
	const CGHeroInstance * getHero() const;
	void close() override;

	void artifactMoved(const ArtifactLocation & srcLoc, const ArtifactLocation & destLoc, bool withRedraw) override;
	void showAll(Canvas & to) override;

private:
	const CGHeroInstance * hero;
	std::shared_ptr<CExperienceAltar> altar;
	std::shared_ptr<CButton> changeModeButton;
	std::shared_ptr<CButton> quitButton;
	std::function<void()> windowClosedCallback;
	std::shared_ptr<CGStatusBar> statusBar;

	void createAltarArtifacts(const IMarket * market, const CGHeroInstance * hero);
	void createAltarCreatures(const IMarket * market, const CGHeroInstance * hero);
};
