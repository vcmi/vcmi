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
#include "../GameEngine.h"
#include "../GameInstance.h"
#include "../gui/WindowHandler.h"
#include "../widgets/CTextInput.h"
#include "../widgets/Slider.h"
#include "../widgets/ObjectLists.h"
#include "../widgets/TextControls.h"

#include "../../lib/texts/Languages.h"
#include "../../lib/texts/MetaString.h"
#include "../../lib/texts/TextOperations.h"

GlobalLobbyWindow::GlobalLobbyWindow()
	: CWindowObject(BORDERED)
{
	OBJECT_CONSTRUCTION;
	widget = std::make_shared<GlobalLobbyWidget>(this);
	pos = widget->pos;
	center();

	widget->getAccountNameLabel()->setText(GAME->server().getGlobalLobby().getAccountDisplayName());
	doOpenChannel("global", "english", Languages::getLanguageOptions("english").nameNative);

	widget->getChannelListHeader()->setText(MetaString::createFromTextID("vcmi.lobby.header.channels").toString());
	widget->getChannelList()->resize(GAME->server().getGlobalLobby().getActiveChannels().size()+1);
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

	auto history = GAME->server().getGlobalLobby().getChannelHistory(channelType, channelName);

	for(const auto & entry : history)
		onGameChatMessage(entry.displayName, entry.messageText, entry.timeFormatted, channelType, channelName);

	refreshChatText();

	MetaString text;
	text.appendTextID("vcmi.lobby.header.chat." + channelType);
	text.replaceRawString(roomDescription);
	widget->getGameChatHeader()->setText(text.toString());

	// Update currently selected item in UI
	// WARNING: this invalidates function parameters since some of them are members of objects that will be destroyed by reset
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

	GAME->server().getGlobalLobby().sendMessage(toSend);

	widget->getMessageInput()->setText("");
}

void GlobalLobbyWindow::doCreateGameRoom()
{
	ENGINE->windows().createAndPushWindow<GlobalLobbyServerSetup>();
}

void GlobalLobbyWindow::doInviteAccount(const std::string & accountID)
{
	JsonNode toSend;
	toSend["type"].String() = "sendInvite";
	toSend["accountID"].String() = accountID;

	GAME->server().getGlobalLobby().sendMessage(toSend);
}

void GlobalLobbyWindow::doJoinRoom(const std::string & roomID)
{
	JsonNode toSend;
	toSend["type"].String() = "joinGameRoom";
	toSend["gameRoomID"].String() = roomID;

	GAME->server().getGlobalLobby().sendMessage(toSend);
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
}
void GlobalLobbyWindow::refreshChatText()
{
	widget->getGameChat()->setText(chatHistory);
	if (widget->getGameChat()->slider)
		widget->getGameChat()->slider->scrollToMax();
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

void GlobalLobbyWindow::onActiveGameRooms(const std::vector<GlobalLobbyRoom> & rooms)
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

void GlobalLobbyWindow::refreshActiveChannels()
{
	const auto & activeChannels = GAME->server().getGlobalLobby().getActiveChannels();

	if (activeChannels.size()+1 == widget->getChannelList()->size())
		widget->getChannelList()->reset();
	else
		widget->getChannelList()->resize(activeChannels.size()+1);

	if (currentChannelType == "global" && !vstd::contains(activeChannels, currentChannelName) && !activeChannels.empty())
	{
		doOpenChannel("global", activeChannels.front(), Languages::getLanguageOptions(activeChannels.front()).nameNative);
	}
}

void GlobalLobbyWindow::onInviteReceived(const std::string & invitedRoomID)
{
	widget->getRoomList()->reset();
}

void GlobalLobbyWindow::onJoinedRoom()
{
	widget->getAccountList()->reset();
}

void GlobalLobbyWindow::onLeftRoom()
{
	widget->getAccountList()->reset();
}
