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

#include "GlobalLobbyLoginWindow.h"
#include "GlobalLobbyWindow.h"

#include "../gui/CGuiHandler.h"
#include "../gui/WindowHandler.h"
#include "../windows/InfoWindows.h"

#include "../../lib/CConfigHandler.h"
#include "../../lib/MetaString.h"
#include "../../lib/TextOperations.h"
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

	return TextOperations::getFormattedTimeLocal(std::chrono::system_clock::to_time_t(timeNowChrono));
}

void GlobalLobbyClient::onPacketReceived(const std::shared_ptr<NetworkConnection> &, const std::vector<uint8_t> & message)
{
	boost::mutex::scoped_lock interfaceLock(GH.interfaceMutex);

	JsonNode json(message.data(), message.size());

	if(json["type"].String() == "accountCreated")
		return receiveAccountCreated(json);

	if(json["type"].String() == "loginFailed")
		return receiveLoginFailed(json);

	if(json["type"].String() == "loginSuccess")
		return receiveLoginSuccess(json);

	if(json["type"].String() == "chatHistory")
		return receiveChatHistory(json);

	if(json["type"].String() == "chatMessage")
		return receiveChatMessage(json);

	if(json["type"].String() == "activeAccounts")
		return receiveActiveAccounts(json);

	throw std::runtime_error("Received unexpected message from lobby server: " + json["type"].String());
}

void GlobalLobbyClient::receiveAccountCreated(const JsonNode & json)
{
	auto loginWindowPtr = loginWindow.lock();

	if(!loginWindowPtr || !GH.windows().topWindow<GlobalLobbyLoginWindow>())
		throw std::runtime_error("lobby connection finished without active login window!");

	{
		Settings configID = settings.write["lobby"]["accountID"];
		configID->String() = json["accountID"].String();

		Settings configName = settings.write["lobby"]["displayName"];
		configName->String() = json["displayName"].String();

		Settings configCookie = settings.write["lobby"]["accountCookie"];
		configCookie->String() = json["accountCookie"].String();
	}

	sendClientLogin();
}

void GlobalLobbyClient::receiveLoginFailed(const JsonNode & json)
{
	auto loginWindowPtr = loginWindow.lock();

	if(!loginWindowPtr || !GH.windows().topWindow<GlobalLobbyLoginWindow>())
		throw std::runtime_error("lobby connection finished without active login window!");

	loginWindowPtr->onConnectionFailed(json["reason"].String());
}

void GlobalLobbyClient::receiveLoginSuccess(const JsonNode & json)
{
	{
		Settings configCookie = settings.write["lobby"]["accountCookie"];
		configCookie->String() = json["accountCookie"].String();

		Settings configName = settings.write["lobby"]["displayName"];
		configName->String() = json["displayName"].String();
	}

	auto loginWindowPtr = loginWindow.lock();

	if(!loginWindowPtr || !GH.windows().topWindow<GlobalLobbyLoginWindow>())
		throw std::runtime_error("lobby connection finished without active login window!");

	loginWindowPtr->onConnectionSuccess();
}

void GlobalLobbyClient::receiveChatHistory(const JsonNode & json)
{
	for(const auto & entry : json["messages"].Vector())
	{
		std::string accountID = entry["accountID"].String();
		std::string displayName = entry["displayName"].String();
		std::string messageText = entry["messageText"].String();
		int ageSeconds = entry["ageSeconds"].Integer();
		std::string timeFormatted = getCurrentTimeFormatted(-ageSeconds);

		auto lobbyWindowPtr = lobbyWindow.lock();
		if(lobbyWindowPtr)
			lobbyWindowPtr->onGameChatMessage(displayName, messageText, timeFormatted);
	}
}

void GlobalLobbyClient::receiveChatMessage(const JsonNode & json)
{
	std::string accountID = json["accountID"].String();
	std::string displayName = json["displayName"].String();
	std::string messageText = json["messageText"].String();
	std::string timeFormatted = getCurrentTimeFormatted();
	auto lobbyWindowPtr = lobbyWindow.lock();
	if(lobbyWindowPtr)
		lobbyWindowPtr->onGameChatMessage(displayName, messageText, timeFormatted);
}

void GlobalLobbyClient::receiveActiveAccounts(const JsonNode & json)
{
	//for (auto const & jsonEntry : json["messages"].Vector())
	//{
	//	std::string accountID = jsonEntry["accountID"].String();
	//	std::string displayName = jsonEntry["displayName"].String();
	//}
}

void GlobalLobbyClient::onConnectionEstablished(const std::shared_ptr<NetworkConnection> &)
{
	JsonNode toSend;

	std::string accountID = settings["lobby"]["accountID"].String();

	if(accountID.empty())
		sendClientRegister();
	else
		sendClientLogin();
}

void GlobalLobbyClient::sendClientRegister()
{
	JsonNode toSend;
	toSend["type"].String() = "clientRegister";
	toSend["displayName"] = settings["lobby"]["displayName"];
	sendMessage(toSend);
}

void GlobalLobbyClient::sendClientLogin()
{
	JsonNode toSend;
	toSend["type"].String() = "clientLogin";
	toSend["accountID"] = settings["lobby"]["accountID"];
	toSend["accountCookie"] = settings["lobby"]["accountCookie"];
	sendMessage(toSend);
}

void GlobalLobbyClient::onConnectionFailed(const std::string & errorMessage)
{
	auto loginWindowPtr = loginWindow.lock();

	if(!loginWindowPtr || !GH.windows().topWindow<GlobalLobbyLoginWindow>())
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
	uint8_t * payloadBegin = reinterpret_cast<uint8_t *>(payloadString.data());
	uint8_t * payloadEnd = payloadBegin + payloadString.size();

	std::vector<uint8_t> payloadBuffer(payloadBegin, payloadEnd);

	networkClient->sendPacket(payloadBuffer);
}

void GlobalLobbyClient::connect()
{
	std::string hostname = settings["lobby"]["hostname"].String();
	int16_t port = settings["lobby"]["port"].Integer();
	networkClient->start(hostname, port);
}

bool GlobalLobbyClient::isConnected()
{
	return networkClient->isConnected();
}

std::shared_ptr<GlobalLobbyLoginWindow> GlobalLobbyClient::createLoginWindow()
{
	auto loginWindowPtr = loginWindow.lock();
	if(loginWindowPtr)
		return loginWindowPtr;

	auto loginWindowNew = std::make_shared<GlobalLobbyLoginWindow>();
	loginWindow = loginWindowNew;

	return loginWindowNew;
}

std::shared_ptr<GlobalLobbyWindow> GlobalLobbyClient::createLobbyWindow()
{
	auto lobbyWindowPtr = lobbyWindow.lock();
	if(lobbyWindowPtr)
		return lobbyWindowPtr;

	lobbyWindowPtr = std::make_shared<GlobalLobbyWindow>();
	lobbyWindow = lobbyWindowPtr;
	lobbyWindowLock = lobbyWindowPtr;
	return lobbyWindowPtr;
}
