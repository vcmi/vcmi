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
class CMultiLineLabel;

class CLobbyScreen : public CSelectionBase
{
public:
	std::shared_ptr<CButton> buttonChat;
	std::shared_ptr<CButton> buttonMods;

	CLobbyScreen(ESelectionScreen type);
	~CLobbyScreen();
	void toggleTab(std::shared_ptr<CIntObject> tab) override;
	void startCampaign();
	void startScenario(ui8 startOptions = 0);
	void toggleMode(bool host);
	void toggleChat();

	void updateAfterStateChange();

	const CMapInfo * getMapInfo() override;
	const StartInfo * getStartInfo() override;

	CBonusSelection * bonusSel;
};

class CModStatusBox : public CWindowObject
{
	std::shared_ptr<CLabel> labelTitle;
	std::shared_ptr<CLabelGroup> labelGroupMods;
	std::shared_ptr<CLabelGroup> labelGroupStatusWorking;
	std::shared_ptr<CLabelGroup> labelGroupStatusMissing;
	std::shared_ptr<CLabelGroup> labelGroupStatusIncompatible;
	std::shared_ptr<CMultiLineLabel> labelErrorMessage;
	std::shared_ptr<CButton> buttonOk;
	std::shared_ptr<CButton> buttonCancel;
public:
	CModStatusBox(bool showError = false, ui8 startOptions = 0);
};
