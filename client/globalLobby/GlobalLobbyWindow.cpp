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

#include "../gui/CGuiHandler.h"
#include "../widgets/TextControls.h"

#include "../../lib/MetaString.h"
#include "../../lib/CConfigHandler.h"

GlobalLobbyWindow::GlobalLobbyWindow():
	CWindowObject(BORDERED)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL_NO_DISPOSE;
	widget = std::make_shared<GlobalLobbyWidget>(this);
	pos = widget->pos;
	center();
	connection = std::make_shared<GlobalLobbyClient>(this);

	connection->start("127.0.0.1", 30303);
	widget->getAccountNameLabel()->setText(settings["general"]["playerName"].String());

	addUsedEvents(TIME);
}

void GlobalLobbyWindow::tick(uint32_t msPassed)
{
	connection->poll();
}

void GlobalLobbyWindow::doSendChatMessage()
{
	std::string messageText = widget->getMessageInput()->getText();

	JsonNode toSend;
	toSend["type"].String() = "sendChatMessage";
	toSend["messageText"].String() = messageText;

	connection->sendMessage(toSend);

	widget->getMessageInput()->setText("");
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
