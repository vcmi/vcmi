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
#include "GlobalLobbyLoginWindow.h"

#include "../gui/CGuiHandler.h"
#include "../gui/WindowHandler.h"
#include "../windows/InfoWindows.h"

#include "../../lib/MetaString.h"
#include "../../lib/CConfigHandler.h"
#include "../../lib/network/NetworkClient.h"

GlobalLobbyClient::~GlobalLobbyClient()
{
	networkClient->stop();
	networkThread->join();
}

GlobalLobbyClient::GlobalLobbyClient()
	: networkClient(std::make_unique<NetworkClient>(*this))
{
	networkThread = std::make_unique<boost::thread>([this](){
		networkClient->run();
	});
}

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
	boost::mutex::scoped_lock interfaceLock(GH.interfaceMutex);

	JsonNode json(message.data(), message.size());

	if (json["type"].String() == "authentication")
	{
		auto loginWindowPtr = loginWindow.lock();

		if (!loginWindowPtr || !GH.windows().topWindow<GlobalLobbyLoginWindow>())
			throw std::runtime_error("lobby connection finished without active login window!");

		loginWindowPtr->onConnectionSuccess();
	}

	if (json["type"].String() == "chatHistory")
	{
		for (auto const & entry : json["messages"].Vector())
		{
			std::string senderName = entry["senderName"].String();
			std::string messageText = entry["messageText"].String();
			int ageSeconds = entry["ageSeconds"].Integer();
			std::string timeFormatted = getCurrentTimeFormatted(-ageSeconds);

			auto lobbyWindowPtr = lobbyWindow.lock();
			if(lobbyWindowPtr)
				lobbyWindowPtr->onGameChatMessage(senderName, messageText, timeFormatted);
		}
	}

	if (json["type"].String() == "chatMessage")
	{
		std::string senderName = json["senderName"].String();
		std::string messageText = json["messageText"].String();
		std::string timeFormatted = getCurrentTimeFormatted();
		auto lobbyWindowPtr = lobbyWindow.lock();
		if(lobbyWindowPtr)
			lobbyWindowPtr->onGameChatMessage(senderName, messageText, timeFormatted);
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
	auto loginWindowPtr = loginWindow.lock();

	if (!loginWindowPtr || !GH.windows().topWindow<GlobalLobbyLoginWindow>())
		throw std::runtime_error("lobby connection failed without active login window!");

	logGlobal->warn("Connection to game lobby failed! Reason: %s", errorMessage);
	loginWindowPtr->onConnectionFailed(errorMessage);
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

void GlobalLobbyClient::connect()
{
	networkClient->start("127.0.0.1", 30303);
}

bool GlobalLobbyClient::isConnected()
{
	return networkClient->isConnected();
}

std::shared_ptr<GlobalLobbyLoginWindow> GlobalLobbyClient::createLoginWindow()
{
	auto loginWindowPtr = loginWindow.lock();
	if (loginWindowPtr)
		return loginWindowPtr;

	auto loginWindowNew = std::make_shared<GlobalLobbyLoginWindow>();
	loginWindow = loginWindowNew;

	return loginWindowNew;
}

std::shared_ptr<GlobalLobbyWindow> GlobalLobbyClient::createLobbyWindow()
{
	auto lobbyWindowPtr = lobbyWindow.lock();
	if (lobbyWindowPtr)
		return lobbyWindowPtr;

	lobbyWindowPtr = std::make_shared<GlobalLobbyWindow>();
	lobbyWindow = lobbyWindowPtr;
	lobbyWindowLock = lobbyWindowPtr;
	return lobbyWindowPtr;
}

