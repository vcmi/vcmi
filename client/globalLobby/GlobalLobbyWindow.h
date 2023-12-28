/*
 * GlobalLobbyWindow.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../windows/CWindowObject.h"

class GlobalLobbyWidget;

class GlobalLobbyWindow : public CWindowObject
{
	std::string chatHistory;

	std::shared_ptr<GlobalLobbyWidget> widget;

public:
	GlobalLobbyWindow();

	void doSendChatMessage();
	void doCreateGameRoom();

	void onGameChatMessage(const std::string & sender, const std::string & message, const std::string & when);

};
