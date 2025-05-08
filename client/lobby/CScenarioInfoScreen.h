/*
 * CScenarioInfoScreen.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "CSelectionBase.h"

/// Scenario information screen shown during the game
class CScenarioInfoScreen : public WindowBase, public ISelectionScreenInfo
{
	std::unique_ptr<StartInfo> localSi;
	std::unique_ptr<CMapInfo> localMi;
public:
	std::shared_ptr<CButton> buttonBack;
	std::shared_ptr<InfoCard> card;
	std::shared_ptr<OptionsTab> opt;

	CScenarioInfoScreen();
	~CScenarioInfoScreen();

	const CMapInfo * getMapInfo() override;
	const StartInfo * getStartInfo() override;
};
