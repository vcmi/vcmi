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
class GraphicalPrimitiveCanvas;

class CLobbyScreen final : public CSelectionBase
{
public:
	CLobbyScreen(ESelectionScreen type, bool hideScreen = false);
	~CLobbyScreen();
	void toggleTab(std::shared_ptr<CIntObject> tab) final;
	void start(bool campaign);
	void startCampaign();
	void startScenario(bool allowOnlyAI = false);
	void toggleMode(bool host);
	void toggleChat();

	void updateAfterStateChange();
	void onRemoteClientLobbyStateChanged();

	const CMapInfo * getMapInfo() final;
	const StartInfo * getStartInfo() final;

	std::shared_ptr<CBonusSelection> bonusSel;

private:
	std::shared_ptr<CButton> buttonChat;
	std::shared_ptr<GraphicalPrimitiveCanvas> blackScreen;

	bool waitingForPlayersMessageShown = false;
	bool compatibilityFilterInitialized = false;
	size_t lastRequiredHumanPlayers = 0;
	std::string lastCompatibilityNotice;

	bool isMultiplayerNetworkLobby() const;
	bool isMultiplayerHost() const;
	bool canStartLobbyGame() const;
	bool isLanOrOnlineMultiplayerHost() const;
	void updateCompatibilityNotice(size_t requiredHumanPlayers);
	void updateHostLobbyChatState();
	void updateStartButtonState();
};
