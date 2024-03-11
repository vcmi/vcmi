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

#include "GlobalLobbyClient.h"
#include "GlobalLobbyServerSetup.h"
#include "GlobalLobbyWidget.h"

#include "../CServerHandler.h"
#include "../gui/CGuiHandler.h"
#include "../gui/WindowHandler.h"
#include "../widgets/TextControls.h"
#include "../widgets/ObjectLists.h"

#include "../../lib/CConfigHandler.h"
#include "../../lib/MetaString.h"

GlobalLobbyWindow::GlobalLobbyWindow()
	: CWindowObject(BORDERED)
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
}

void GlobalLobbyWindow::doInviteAccount(const std::string & accountID)
{
	JsonNode toSend;
	toSend["type"].String() = "sendInvite";
	toSend["accountID"].String() = accountID;

	CSH->getGlobalLobby().sendMessage(toSend);
}

void GlobalLobbyWindow::doJoinRoom(const std::string & roomID)
{
	JsonNode toSend;
	toSend["type"].String() = "joinGameRoom";
	toSend["gameRoomID"].String() = roomID;

	CSH->getGlobalLobby().sendMessage(toSend);
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

void GlobalLobbyWindow::onActiveAccounts(const std::vector<GlobalLobbyAccount> & accounts)
{
	if (accounts.size() == widget->getAccountList()->size())
		widget->getAccountList()->reset();
	else
		widget->getAccountList()->resize(accounts.size());
}

void GlobalLobbyWindow::onActiveRooms(const std::vector<GlobalLobbyRoom> & rooms)
{
	if (rooms.size() == widget->getAccountList()->size())
		widget->getRoomList()->reset();
	else
		widget->getRoomList()->resize(rooms.size());
}

void GlobalLobbyWindow::onJoinedRoom()
{
	widget->getAccountList()->reset();
}

void GlobalLobbyWindow::onLeftRoom()
{
	widget->getAccountList()->reset();
}
