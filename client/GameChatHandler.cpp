/*
 * GameChatHandler.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "GameChatHandler.h"
#include "GameInstance.h"
#include "CServerHandler.h"
#include "CPlayerInterface.h"
#include "PlayerLocalState.h"
#include "globalLobby/GlobalLobbyClient.h"
#include "lobby/CLobbyScreen.h"

#include "adventureMap/CInGameConsole.h"

#include "../lib/callback/CCallback.h"
#include "../lib/networkPacks/PacksForLobby.h"
#include "../lib/mapObjects/army/CArmedInstance.h"
#include "../lib/CConfigHandler.h"
#include "../lib/GameLibrary.h"
#include "../lib/texts/CGeneralTextHandler.h"
#include "../lib/texts/TextOperations.h"

const std::vector<GameChatMessage> & GameChatHandler::getChatHistory() const
{
	return chatHistory;
}

void GameChatHandler::resetMatchState()
{
	chatHistory.clear();
}

void GameChatHandler::sendMessageGameplay(const std::string & messageText)
{
	GAME->interface()->cb->sendMessage(messageText, GAME->interface()->localState->getCurrentArmy());
	GAME->server().getGlobalLobby().sendMatchChatMessage(messageText);
}

void GameChatHandler::sendMessageLobby(const std::string & senderName, const std::string & messageText)
{
	LobbyChatMessage lcm;
	MetaString txt;
	txt.appendRawString(messageText);
	lcm.message = txt;
	lcm.playerName = senderName;
	GAME->server().sendLobbyPack(lcm);
	GAME->server().getGlobalLobby().sendMatchChatMessage(messageText);
}

void GameChatHandler::onNewLobbyMessageReceived(const std::string & senderName, const std::string & messageText)
{
	if (!SEL)
	{
		logGlobal->debug("Received chat message for lobby but lobby not yet exists!");
		return;
	}

	auto * lobby = dynamic_cast<CLobbyScreen*>(SEL);

	// FIXME: when can this happen?
	assert(lobby);
	assert(lobby->card);

	if(lobby && lobby->card)
	{
		MetaString formatted = MetaString::createFromRawString("[%s] %s: %s");
		formatted.replaceRawString(TextOperations::getCurrentFormattedTimeLocal());
		formatted.replaceRawString(senderName);
		formatted.replaceRawString(messageText);

		lobby->card->chat->addNewMessage(formatted.toString());
		if (!lobby->card->showChat)
				lobby->toggleChat();
	}

	chatHistory.push_back({senderName, messageText, TextOperations::getCurrentFormattedTimeLocal()});
}

void GameChatHandler::onNewGameMessageReceived(PlayerColor sender, const std::string & messageText)
{

	std::string timeFormatted = TextOperations::getCurrentFormattedTimeLocal();
	std::string playerName = "<UNKNOWN>";

	if (sender.isValidPlayer())
		playerName = GAME->interface()->cb->getStartInfo()->playerInfos.at(sender).name;

	if (sender.isSpectator())
		playerName = LIBRARY->generaltexth->translate("vcmi.lobby.login.spectator");

	chatHistory.push_back({playerName, messageText, timeFormatted});

	GAME->interface()->cingconsole->addMessage(timeFormatted, playerName, messageText);
}

void GameChatHandler::onNewSystemMessageReceived(const std::string & messageText)
{
	chatHistory.push_back({"System", messageText, TextOperations::getCurrentFormattedTimeLocal()});

	if(GAME->interface() && !settings["session"]["hideSystemMessages"].Bool())
		GAME->interface()->cingconsole->addMessage(TextOperations::getCurrentFormattedTimeLocal(), "System", messageText);
}
