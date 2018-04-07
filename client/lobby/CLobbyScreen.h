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

class CLobbyScreen : public CSelectionBase
{
public:
	std::shared_ptr<CButton> buttonChat;

	CLobbyScreen(ESelectionScreen type);
	~CLobbyScreen();
	void toggleTab(std::shared_ptr<CIntObject> tab) override;
	void startCampaign();
	void startScenario(bool allowOnlyAI = false);
	void toggleMode(bool host);
	void toggleChat();

	void updateAfterStateChange();

	const CMapInfo * getMapInfo() override;
	const StartInfo * getStartInfo() override;

	std::shared_ptr<CBonusSelection> bonusSel;
};
