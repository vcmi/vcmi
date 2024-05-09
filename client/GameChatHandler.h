/*
 * GameChatHandler.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../lib/constants/EntityIdentifiers.h"

struct GameChatMessage
{
	std::string senderName;
	std::string messageText;
	std::string dateFormatted;
};

/// Class that manages game chat for current game match
/// Used for all matches - singleplayer, local multiplayer, online multiplayer
class GameChatHandler : boost::noncopyable
{
	std::vector<GameChatMessage> chatHistory;
public:
	/// Returns all message history for current match
	const std::vector<GameChatMessage> & getChatHistory() const;

	/// Erases any local state, must be called when client disconnects from match server
	void resetMatchState();

	/// Must be called when local player sends new message into chat from gameplay mode (adventure map)
	void sendMessageGameplay(const std::string & messageText);

	/// Must be called when local player sends new message into chat from pregame mode (match lobby)
	void sendMessageLobby(const std::string & senderName, const std::string & messageText);

	/// Must be called when client receives new chat message from server
	void onNewLobbyMessageReceived(const std::string & senderName, const std::string & messageText);

	/// Must be called when client receives new chat message from server
	void onNewGameMessageReceived(PlayerColor sender, const std::string & messageText);

	/// Must be called when client receives new message from "system" sender
	void onNewSystemMessageReceived(const std::string & messageText);
};
