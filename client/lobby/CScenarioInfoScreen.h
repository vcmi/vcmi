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

/// Scenario information screen shown during the game (thus not really a "pre-game" but fits here anyway)
class CScenarioInfoScreen : public CIntObject, public ISelectionScreenInfo
{
public:
	CButton * back;
	InfoCard * card;
	OptionsTab * opt;

	const StartInfo * localSi;
	CMapInfo * localMi;

	CScenarioInfoScreen();
	~CScenarioInfoScreen();

	const CMapInfo * getMapInfo() override;
	const StartInfo * getStartInfo() override;
};
