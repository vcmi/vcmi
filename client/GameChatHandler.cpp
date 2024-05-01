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
#include "CServerHandler.h"
#include "CPlayerInterface.h"
#include "PlayerLocalState.h"
#include "globalLobby/GlobalLobbyClient.h"
#include "lobby/CLobbyScreen.h"

#include "adventureMap/CInGameConsole.h"

#include "../CCallback.h"

#include "../lib/networkPacks/PacksForLobby.h"
#include "../lib/TextOperations.h"
#include "../lib/mapObjects/CArmedInstance.h"
#include "../lib/CConfigHandler.h"
#include "../lib/MetaString.h"
#include "../lib/VCMI_Lib.h"
#include "../lib/CGeneralTextHandler.h"

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
	LOCPLINT->cb->sendMessage(messageText, LOCPLINT->localState->getCurrentArmy());
	CSH->getGlobalLobby().sendMatchChatMessage(messageText);
}

void GameChatHandler::sendMessageLobby(const std::string & senderName, const std::string & messageText)
{
	LobbyChatMessage lcm;
	lcm.message = messageText;
	lcm.playerName = senderName;
	CSH->sendLobbyPack(lcm);
	CSH->getGlobalLobby().sendMatchChatMessage(messageText);
}

void GameChatHandler::onNewLobbyMessageReceived(const std::string & senderName, const std::string & messageText)
{
	if (!SEL)
	{
		logGlobal->debug("Received chat message for lobby but lobby not yet exists!");
		return;
	}

	auto * lobby = dynamic_cast<CLobbyScreen*>(SEL);

	std::string messageTextReplaced = translationReplace(messageText);

	// FIXME: when can this happen?
	assert(lobby);
	assert(lobby->card);

	if(lobby && lobby->card)
	{
		MetaString formatted = MetaString::createFromRawString("[%s] %s: %s");
		formatted.replaceRawString(TextOperations::getCurrentFormattedTimeLocal());
		formatted.replaceRawString(senderName);
		formatted.replaceRawString(messageTextReplaced);

		lobby->card->chat->addNewMessage(formatted.toString());
		if (!lobby->card->showChat)
				lobby->toggleChat();
	}

	chatHistory.push_back({senderName, messageTextReplaced, TextOperations::getCurrentFormattedTimeLocal()});
}

void GameChatHandler::onNewGameMessageReceived(PlayerColor sender, const std::string & messageText)
{

	std::string timeFormatted = TextOperations::getCurrentFormattedTimeLocal();
	std::string playerName = "<UNKNOWN>";

	std::string messageTextReplaced = translationReplace(messageText);

	if (sender.isValidPlayer())
		playerName = LOCPLINT->cb->getStartInfo()->playerInfos.at(sender).name;

	if (sender.isSpectator())
		playerName = "Spectator"; // FIXME: translate? Provide nickname somewhere?

	chatHistory.push_back({playerName, messageTextReplaced, timeFormatted});

	LOCPLINT->cingconsole->addMessage(timeFormatted, playerName, messageTextReplaced);
}

void GameChatHandler::onNewSystemMessageReceived(const std::string & messageText)
{
	std::string messageTextReplaced = translationReplace(messageText);

	chatHistory.push_back({"System", messageTextReplaced, TextOperations::getCurrentFormattedTimeLocal()});

	if(LOCPLINT && !settings["session"]["hideSystemMessages"].Bool())
		LOCPLINT->cingconsole->addMessage(TextOperations::getCurrentFormattedTimeLocal(), "System", messageTextReplaced);
}

std::string GameChatHandler::translationReplace(std::string txt)
{
	std::regex expr("~~([\\w\\d\\.]+)~~");
	std::string result = "";
	std::string tmp_suffix = "";
	std::smatch match;
	std::string::const_iterator searchStart( txt.cbegin() );
	while(std::regex_search(searchStart, txt.cend(), match, expr) )
	{
		result += match.prefix();
		result += VLC->generaltexth->translate(match.str(1));
		
		searchStart = match.suffix().first;
		tmp_suffix = match.suffix();
	}
	return result.empty() ? txt : result + tmp_suffix;
}
