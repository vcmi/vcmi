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

#include "../widgets/markets/CMarketBase.h"
#include "CWindowWithArtifacts.h"

class CMarketWindow final : public CStatusbarWindow, public CWindowWithArtifacts, public IGarrisonHolder, public IMarketHolder
{
public:
	CMarketWindow(const IMarket * market, const CGHeroInstance * hero, const std::function<void()> & onWindowClosed, EMarketMode mode);
	void updateResources() override;
	void updateArtifacts() override;
	void updateGarrisons() override;
	void updateExperience() override;

	void update();
	void close() override;
	bool holdsGarrison(const CArmedInstance * army) override;

private:
	void createChangeModeButtons(EMarketMode currentMode, const IMarket * market, const CGHeroInstance * hero);
	void initWidgetInternals(const EMarketMode mode, const std::pair<std::string, std::string> & quitButtonHelpContainer);
	std::string getMarketTitle(const ObjectInstanceID marketId, const EMarketMode mode) const;

	void createArtifactsBuying(const IMarket * market, const CGHeroInstance * hero);
	void createArtifactsSelling(const IMarket * market, const CGHeroInstance * hero);
	void createMarketResources(const IMarket * market, const CGHeroInstance * hero);
	void createFreelancersGuild(const IMarket * market, const CGHeroInstance * hero);
	void createTransferResources(const IMarket * market, const CGHeroInstance * hero);
	void createAltarArtifacts(const IMarket * market, const CGHeroInstance * hero);
	void createAltarCreatures(const IMarket * market, const CGHeroInstance * hero);

	const int buttonHeightWithMargin = 32 + 3;
	std::vector<std::shared_ptr<CButton>> changeModeButtons;
	std::shared_ptr<CButton> quitButton;
	std::function<void()> windowClosedCallback;
	const Point quitButtonPos = Point(516, 520);
	std::shared_ptr<CMarketBase> marketWidget;

	// This is workaround for bug in H3 files where this slot for ragdoll on this screen is missing
	std::shared_ptr<CPicture> artSlotBack;
};
