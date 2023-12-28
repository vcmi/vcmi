/*
 * GlobalLobbyWidget.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "GlobalLobbyWidget.h"
#include "GlobalLobbyWindow.h"

#include "../gui/CGuiHandler.h"
#include "../gui/WindowHandler.h"
#include "../widgets/TextControls.h"

GlobalLobbyWidget::GlobalLobbyWidget(GlobalLobbyWindow * window)
	: window(window)
{
	addCallback("closeWindow", [](int) { GH.windows().popWindows(1); });
	addCallback("sendMessage", [this](int) { this->window->doSendChatMessage(); });
	addCallback("createGameRoom", [this](int) { this->window->doCreateGameRoom(); });

	const JsonNode config(JsonPath::builtin("config/widgets/lobbyWindow.json"));
	build(config);
}

std::shared_ptr<CLabel> GlobalLobbyWidget::getAccountNameLabel()
{
	return widget<CLabel>("accountNameLabel");
}

std::shared_ptr<CTextInput> GlobalLobbyWidget::getMessageInput()
{
	return widget<CTextInput>("messageInput");
}

std::shared_ptr<CTextBox> GlobalLobbyWidget::getGameChat()
{
	return widget<CTextBox>("gameChat");
}
