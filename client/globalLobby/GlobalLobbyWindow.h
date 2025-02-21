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

#include "GlobalLobbyObserver.h"
#include "../windows/CWindowObject.h"

class GlobalLobbyWidget;
struct GlobalLobbyAccount;
struct GlobalLobbyRoom;

class GlobalLobbyWindow final : public CWindowObject, public GlobalLobbyObserver
{
	std::string chatHistory;
	std::string currentChannelType;
	std::string currentChannelName;

	std::shared_ptr<GlobalLobbyWidget> widget;
	std::set<std::string> unreadChannels;

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
	/// Returns true if provided channel has unread messages (only messages that were received after login)
	bool isChannelUnread(const std::string & channelType, const std::string & channelName) const;

	// Callbacks for network packs

	void onGameChatMessage(const std::string & sender, const std::string & message, const std::string & when, const std::string & channelType, const std::string & channelName);
	void refreshChatText();
	void refreshActiveChannels();
	void onActiveAccounts(const std::vector<GlobalLobbyAccount> & accounts) override;
	void onActiveGameRooms(const std::vector<GlobalLobbyRoom> & rooms) override;
	void onMatchesHistory(const std::vector<GlobalLobbyRoom> & history);
	void onInviteReceived(const std::string & invitedRoomID);
	void onJoinedRoom();
	void onLeftRoom();
};
