/*
 * CMarketWindow.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "CTradeWindow.h"
#include "CAltarWindow.h"

class CFreelancerGuild;
class CMarketResources;

class CMarketWindow : public CStatusbarWindow, public CAltarWindow // TODO remove CAltarWindow
{
public:
	CMarketWindow(const IMarket * market, const CGHeroInstance * hero, const std::function<void()> & onWindowClosed, EMarketMode mode);
	void resourceChanged();
	void artifactsChanged();
	void updateGarrisons() override;
	void close() override;
	const CGHeroInstance * getHero() const;

private:
	void createChangeModeButtons(EMarketMode currentMode, const IMarket * market, const CGHeroInstance * hero);
	void createInternals(EMarketMode mode, const IMarket * market, const CGHeroInstance * hero);

	void createArtifactsBuying(const IMarket * market, const CGHeroInstance * hero);
	void createArtifactsSelling(const IMarket * market, const CGHeroInstance * hero);
	void createMarketResources(const IMarket * market, const CGHeroInstance * hero);
	void createFreelancersGuild(const IMarket * market, const CGHeroInstance * hero);
	void createTransferResources(const IMarket * market, const CGHeroInstance * hero);
	void createAltarArtifacts(const IMarket * market, const CGHeroInstance * hero);
	void createAltarCreatures(const IMarket * market, const CGHeroInstance * hero);

	const int buttonHeightWithMargin = 32 + 3;
	const CGHeroInstance * hero;
	std::vector<std::shared_ptr<CButton>> changeModeButtons;
	std::shared_ptr<CButton> quitButton;
	std::function<void()> windowClosedCallback;
	const Point quitButtonPos = Point(516, 520);

	std::shared_ptr<CMarketplaceWindow> market;
	std::shared_ptr<CFreelancerGuild> guild;
	std::shared_ptr<CMarketResources> resRes;
};
