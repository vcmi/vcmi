/*
 * GlobalLobbyClient.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "GlobalLobbyClient.h"

#include "GlobalLobbyWindow.h"

#include "../gui/CGuiHandler.h"
#include "../gui/WindowHandler.h"
#include "../windows/InfoWindows.h"

#include "../../lib/MetaString.h"
#include "../../lib/CConfigHandler.h"
#include "../../lib/network/NetworkClient.h"

GlobalLobbyClient::~GlobalLobbyClient() = default;

GlobalLobbyClient::GlobalLobbyClient(GlobalLobbyWindow * window)
	: networkClient(std::make_unique<NetworkClient>(*this))
	, window(window)
{}

static std::string getCurrentTimeFormatted(int timeOffsetSeconds = 0)
{
	// FIXME: better/unified way to format date
	auto timeNowChrono = std::chrono::system_clock::now();
	timeNowChrono += std::chrono::seconds(timeOffsetSeconds);

	std::time_t timeNowC = std::chrono::system_clock::to_time_t(timeNowChrono);
	std::tm timeNowTm = *std::localtime(&timeNowC);

	MetaString timeFormatted;
	timeFormatted.appendRawString("%d:%d");
	timeFormatted.replaceNumber(timeNowTm.tm_hour);
	timeFormatted.replaceNumber(timeNowTm.tm_min);

	return timeFormatted.toString();
}

void GlobalLobbyClient::onPacketReceived(const std::shared_ptr<NetworkConnection> &, const std::vector<uint8_t> & message)
{
	// FIXME: find better approach
	const char * payloadBegin = reinterpret_cast<const char*>(message.data());
	JsonNode json(payloadBegin, message.size());

	if (json["type"].String() == "chatHistory")
	{
		for (auto const & entry : json["messages"].Vector())
		{
			std::string senderName = entry["senderName"].String();
			std::string messageText = entry["messageText"].String();
			int ageSeconds = entry["ageSeconds"].Integer();
			std::string timeFormatted = getCurrentTimeFormatted(-ageSeconds);
			window->onGameChatMessage(senderName, messageText, timeFormatted);
		}
	}

	if (json["type"].String() == "chatMessage")
	{
		std::string senderName = json["senderName"].String();
		std::string messageText = json["messageText"].String();
		std::string timeFormatted = getCurrentTimeFormatted();
		window->onGameChatMessage(senderName, messageText, timeFormatted);
	}
}

void GlobalLobbyClient::onConnectionEstablished(const std::shared_ptr<NetworkConnection> &)
{
	JsonNode toSend;
	toSend["type"].String() = "authentication";
	toSend["accountName"].String() = settings["general"]["playerName"].String();

	sendMessage(toSend);
}

void GlobalLobbyClient::onConnectionFailed(const std::string & errorMessage)
{
	GH.windows().popWindows(1);
	CInfoWindow::showInfoDialog("Failed to connect to game lobby!\n" + errorMessage, {});
}

void GlobalLobbyClient::onDisconnected(const std::shared_ptr<NetworkConnection> &)
{
	GH.windows().popWindows(1);
	CInfoWindow::showInfoDialog("Connection to game lobby was lost!", {});
}

void GlobalLobbyClient::onTimer()
{
	// no-op
}

void GlobalLobbyClient::sendMessage(const JsonNode & data)
{
	std::string payloadString = data.toJson(true);

	// FIXME: find better approach
	uint8_t * payloadBegin = reinterpret_cast<uint8_t*>(payloadString.data());
	uint8_t * payloadEnd = payloadBegin + payloadString.size();

	std::vector<uint8_t> payloadBuffer(payloadBegin, payloadEnd);

	networkClient->sendPacket(payloadBuffer);
}

void GlobalLobbyClient::start(const std::string & host, uint16_t port)
{
	networkClient->start(host, port);
}

void GlobalLobbyClient::run()
{
	networkClient->run();
}

void GlobalLobbyClient::poll()
{
	networkClient->poll();
}
