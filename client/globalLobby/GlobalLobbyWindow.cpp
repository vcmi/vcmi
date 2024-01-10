/*
 * GlobalLobbyWindow.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "GlobalLobbyWindow.h"

#include "GlobalLobbyWidget.h"
#include "GlobalLobbyClient.h"
#include "GlobalLobbyServerSetup.h"

#include "../gui/CGuiHandler.h"
#include "../gui/WindowHandler.h"
#include "../widgets/TextControls.h"
#include "../CServerHandler.h"

#include "../../lib/MetaString.h"
#include "../../lib/CConfigHandler.h"

GlobalLobbyWindow::GlobalLobbyWindow():
	CWindowObject(BORDERED)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL_NO_DISPOSE;
	widget = std::make_shared<GlobalLobbyWidget>(this);
	pos = widget->pos;
	center();

	widget->getAccountNameLabel()->setText(settings["lobby"]["displayName"].String());
}

void GlobalLobbyWindow::doSendChatMessage()
{
	std::string messageText = widget->getMessageInput()->getText();

	JsonNode toSend;
	toSend["type"].String() = "sendChatMessage";
	toSend["messageText"].String() = messageText;

	CSH->getGlobalLobby().sendMessage(toSend);

	widget->getMessageInput()->setText("");
}

void GlobalLobbyWindow::doCreateGameRoom()
{
	GH.windows().createAndPushWindow<GlobalLobbyServerSetup>();
	// TODO:
	// start local server and supply our UUID / client credentials to it
	// server logs into lobby ( uuid = client, mode = server ). This creates 'room' in mode 'empty'
	// server starts accepting connections from players (including host)
	// client connects to local server
	// client sends createGameRoom query to lobby with own / server UUID and mode 'direct' (non-proxy)
	// client requests to change room status to private or public
}

void GlobalLobbyWindow::onGameChatMessage(const std::string & sender, const std::string & message, const std::string & when)
{
	MetaString chatMessageFormatted;
	chatMessageFormatted.appendRawString("[%s] {%s}: %s\n");
	chatMessageFormatted.replaceRawString(when);
	chatMessageFormatted.replaceRawString(sender);
	chatMessageFormatted.replaceRawString(message);

	chatHistory += chatMessageFormatted.toString();

	widget->getGameChat()->setText(chatHistory);
}
