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
#include "../../lib/Languages.h"
#include "../../lib/MetaString.h"
#include "../../lib/TextOperations.h"

GlobalLobbyWindow::GlobalLobbyWindow()
	: CWindowObject(BORDERED)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL_NO_DISPOSE;
	widget = std::make_shared<GlobalLobbyWidget>(this);
	pos = widget->pos;
	center();

	widget->getAccountNameLabel()->setText(CSH->getGlobalLobby().getAccountDisplayName());
	doOpenChannel("global", "english", Languages::getLanguageOptions("english").nameNative);

	widget->getChannelListHeader()->setText(MetaString::createFromTextID("vcmi.lobby.header.channels").toString());
}

bool GlobalLobbyWindow::isChannelOpen(const std::string & testChannelType, const std::string & testChannelName) const
{
	return testChannelType == currentChannelType && testChannelName == currentChannelName;
}

void GlobalLobbyWindow::doOpenChannel(const std::string & channelType, const std::string & channelName, const std::string & roomDescription)
{
	currentChannelType = channelType;
	currentChannelName = channelName;
	chatHistory.clear();
	unreadChannels.erase(channelType + "_" + channelName);
	widget->getGameChat()->setText("");

	auto history = CSH->getGlobalLobby().getChannelHistory(channelType, channelName);

	for(const auto & entry : history)
		onGameChatMessage(entry.displayName, entry.messageText, entry.timeFormatted, channelType, channelName);

	MetaString text;
	text.appendTextID("vcmi.lobby.header.chat." + channelType);
	text.replaceRawString(roomDescription);
	widget->getGameChatHeader()->setText(text.toString());

	// Update currently selected item in UI
	widget->getAccountList()->reset();
	widget->getChannelList()->reset();
	widget->getMatchList()->reset();

	redraw();
}

void GlobalLobbyWindow::doSendChatMessage()
{
	std::string messageText = widget->getMessageInput()->getText();

	JsonNode toSend;
	toSend["type"].String() = "sendChatMessage";
	toSend["channelType"].String() = currentChannelType;
	toSend["channelName"].String() = currentChannelName;
	toSend["messageText"].String() = messageText;

	assert(TextOperations::isValidUnicodeString(messageText));

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
	unreadInvites.erase(roomID);

	JsonNode toSend;
	toSend["type"].String() = "joinGameRoom";
	toSend["gameRoomID"].String() = roomID;

	CSH->getGlobalLobby().sendMessage(toSend);
}

void GlobalLobbyWindow::onGameChatMessage(const std::string & sender, const std::string & message, const std::string & when, const std::string & channelType, const std::string & channelName)
{
	if (channelType != currentChannelType || channelName != currentChannelName)
	{
		// mark channel as unread
		unreadChannels.insert(channelType + "_" + channelName);
		widget->getAccountList()->reset();
		widget->getChannelList()->reset();
		widget->getMatchList()->reset();
		return;
	}

	MetaString chatMessageFormatted;
	chatMessageFormatted.appendRawString("[%s] {%s}: %s\n");
	chatMessageFormatted.replaceRawString(when);
	chatMessageFormatted.replaceRawString(sender);
	chatMessageFormatted.replaceRawString(message);

	chatHistory += chatMessageFormatted.toString();

	widget->getGameChat()->setText(chatHistory);
}

bool GlobalLobbyWindow::isChannelUnread(const std::string & channelType, const std::string & channelName) const
{
	return unreadChannels.count(channelType + "_" + channelName) > 0;
}

void GlobalLobbyWindow::onActiveAccounts(const std::vector<GlobalLobbyAccount> & accounts)
{
	if (accounts.size() == widget->getAccountList()->size())
		widget->getAccountList()->reset();
	else
		widget->getAccountList()->resize(accounts.size());

	MetaString text = MetaString::createFromTextID("vcmi.lobby.header.players");
	text.replaceNumber(accounts.size());
	widget->getAccountListHeader()->setText(text.toString());
}

void GlobalLobbyWindow::onActiveRooms(const std::vector<GlobalLobbyRoom> & rooms)
{
	if (rooms.size() == widget->getRoomList()->size())
		widget->getRoomList()->reset();
	else
		widget->getRoomList()->resize(rooms.size());

	MetaString text = MetaString::createFromTextID("vcmi.lobby.header.rooms");
	text.replaceNumber(rooms.size());
	widget->getRoomListHeader()->setText(text.toString());
}

void GlobalLobbyWindow::onMatchesHistory(const std::vector<GlobalLobbyRoom> & history)
{
	if (history.size() == widget->getMatchList()->size())
		widget->getMatchList()->reset();
	else
		widget->getMatchList()->resize(history.size());

	MetaString text = MetaString::createFromTextID("vcmi.lobby.header.history");
	text.replaceNumber(history.size());
	widget->getMatchListHeader()->setText(text.toString());
}

void GlobalLobbyWindow::onInviteReceived(const std::string & invitedRoomID)
{
	unreadInvites.insert(invitedRoomID);
	widget->getRoomList()->reset();
}

bool GlobalLobbyWindow::isInviteUnread(const std::string & gameRoomID) const
{
	return unreadInvites.count(gameRoomID) > 0;
}

void GlobalLobbyWindow::onJoinedRoom()
{
	widget->getAccountList()->reset();
}

void GlobalLobbyWindow::onLeftRoom()
{
	widget->getAccountList()->reset();
}
