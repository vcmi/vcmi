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
struct GlobalLobbyAccount;
struct GlobalLobbyRoom;

class GlobalLobbyWindow : public CWindowObject
{
	std::string chatHistory;
	std::string currentChannelType;
	std::string currentChannelName;

	std::shared_ptr<GlobalLobbyWidget> widget;
	std::set<std::string> unreadChannels;
	std::set<std::string> unreadInvites;

public:
	GlobalLobbyWindow();

	// Callbacks for UI

	void doSendChatMessage();
	void doCreateGameRoom();
	void doOpenChannel(const std::string & channelType, const std::string & channelName, const std::string & roomDescription);
	void doInviteAccount(const std::string & accountID);
	void doJoinRoom(const std::string & roomID);

	/// Returns true if provided chat channel is the one that is currently open in UI
	bool isChannelOpen(const std::string & channelType, const std::string & channelName) const;
	bool isChannelUnread(const std::string & channelType, const std::string & channelName) const;
	bool isInviteUnread(const std::string & gameRoomID) const;

	// Callbacks for network packs

	void onGameChatMessage(const std::string & sender, const std::string & message, const std::string & when, const std::string & channelType, const std::string & channelName);
	void onActiveAccounts(const std::vector<GlobalLobbyAccount> & accounts);
	void onActiveRooms(const std::vector<GlobalLobbyRoom> & rooms);
	void onMatchesHistory(const std::vector<GlobalLobbyRoom> & history);
	void onInviteReceived(const std::string & invitedRoomID);
	void onJoinedRoom();
	void onLeftRoom();
};
