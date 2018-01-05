/*
 * CLobbyScreen.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "CSelectionBase.h"

class CBonusSelection;
/// The actual map selection screen which consists of the options and selection tab
class CLobbyScreen : public CSelectionBase
{
public:
	CLobbyScreen(ESelectionScreen type);
	~CLobbyScreen();
	void toggleTab(CIntObject * tab) override;
	void startCampaign();
	void startScenario();
	void toggleMode(bool host);

	void activate() override;
	void updateAfterStateChange();

	const CMapInfo * getMapInfo() override;
	const StartInfo * getStartInfo() override;

	CBonusSelection * bonusSel;
};
