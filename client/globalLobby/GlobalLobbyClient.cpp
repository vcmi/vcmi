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
#include "../CServerHandler.h"
#include "../mainmenu/CMainMenu.h"
#include "../CGameInfo.h"

#include "../../lib/CConfigHandler.h"
#include "../../lib/MetaString.h"
#include "../../lib/json/JsonUtils.h"
#include "../../lib/TextOperations.h"
#include "../../lib/CGeneralTextHandler.h"

GlobalLobbyClient::GlobalLobbyClient() = default;
GlobalLobbyClient::~GlobalLobbyClient() = default;

static std::string getCurrentTimeFormatted(int timeOffsetSeconds = 0)
{
	// FIXME: better/unified way to format date
	auto timeNowChrono = std::chrono::system_clock::now();
	timeNowChrono += std::chrono::seconds(timeOffsetSeconds);

	return TextOperations::getFormattedTimeLocal(std::chrono::system_clock::to_time_t(timeNowChrono));
}

void GlobalLobbyClient::onPacketReceived(const std::shared_ptr<INetworkConnection> &, const std::vector<std::byte> & message)
{
	boost::mutex::scoped_lock interfaceLock(GH.interfaceMutex);

	JsonNode json(message.data(), message.size());

	if(json["type"].String() == "accountCreated")
		return receiveAccountCreated(json);

	if(json["type"].String() == "operationFailed")
		return receiveOperationFailed(json);

	if(json["type"].String() == "clientLoginSuccess")
		return receiveClientLoginSuccess(json);

	if(json["type"].String() == "chatHistory")
		return receiveChatHistory(json);

	if(json["type"].String() == "chatMessage")
		return receiveChatMessage(json);

	if(json["type"].String() == "activeAccounts")
		return receiveActiveAccounts(json);

	if(json["type"].String() == "activeGameRooms")
		return receiveActiveGameRooms(json);

	if(json["type"].String() == "joinRoomSuccess")
		return receiveJoinRoomSuccess(json);

	if(json["type"].String() == "inviteReceived")
		return receiveInviteReceived(json);

	logGlobal->error("Received unexpected message from lobby server: %s", json["type"].String());
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

void GlobalLobbyClient::receiveOperationFailed(const JsonNode & json)
{
	auto loginWindowPtr = loginWindow.lock();

	if(loginWindowPtr)
		loginWindowPtr->onConnectionFailed(json["reason"].String());

	// TODO: handle errors in lobby menu
}

void GlobalLobbyClient::receiveClientLoginSuccess(const JsonNode & json)
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

	loginWindowPtr->onLoginSuccess();
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
	activeAccounts.clear();

	for (auto const & jsonEntry : json["accounts"].Vector())
	{
		GlobalLobbyAccount account;

		account.accountID = jsonEntry["accountID"].String();
		account.displayName = jsonEntry["displayName"].String();
		account.status = jsonEntry["status"].String();

		activeAccounts.push_back(account);
	}

	auto lobbyWindowPtr = lobbyWindow.lock();
	if(lobbyWindowPtr)
		lobbyWindowPtr->onActiveAccounts(activeAccounts);
}

void GlobalLobbyClient::receiveActiveGameRooms(const JsonNode & json)
{
	activeRooms.clear();

	for (auto const & jsonEntry : json["gameRooms"].Vector())
	{
		GlobalLobbyRoom room;

		room.gameRoomID = jsonEntry["gameRoomID"].String();
		room.hostAccountID = jsonEntry["hostAccountID"].String();
		room.hostAccountDisplayName = jsonEntry["hostAccountDisplayName"].String();
		room.description = jsonEntry["description"].String();
		room.playersCount = jsonEntry["playersCount"].Integer();
		room.playerLimit = jsonEntry["playerLimit"].Integer();

		activeRooms.push_back(room);
	}

	auto lobbyWindowPtr = lobbyWindow.lock();
	if(lobbyWindowPtr)
		lobbyWindowPtr->onActiveRooms(activeRooms);
}

void GlobalLobbyClient::receiveInviteReceived(const JsonNode & json)
{
	assert(0); //TODO
}

void GlobalLobbyClient::receiveJoinRoomSuccess(const JsonNode & json)
{
	Settings configRoom = settings.write["lobby"]["roomID"];
	configRoom->String() = json["gameRoomID"].String();

	if (json["proxyMode"].Bool())
	{
		CSH->resetStateForLobby(EStartMode::NEW_GAME, ESelectionScreen::newGame, EServerMode::LOBBY_GUEST, {});
		CSH->loadMode = ELoadMode::MULTI;

		std::string hostname = settings["lobby"]["hostname"].String();
		int16_t port = settings["lobby"]["port"].Integer();
		CSH->connectToServer(hostname, port);
	}
}

void GlobalLobbyClient::onConnectionEstablished(const std::shared_ptr<INetworkConnection> & connection)
{
	boost::mutex::scoped_lock interfaceLock(GH.interfaceMutex);
	networkConnection = connection;

	auto loginWindowPtr = loginWindow.lock();

	if(!loginWindowPtr || !GH.windows().topWindow<GlobalLobbyLoginWindow>())
		throw std::runtime_error("lobby connection established without active login window!");

	loginWindowPtr->onConnectionSuccess();
}

void GlobalLobbyClient::sendClientRegister(const std::string & accountName)
{
	JsonNode toSend;
	toSend["type"].String() = "clientRegister";
	toSend["displayName"].String() = accountName;
	toSend["language"].String() = CGI->generaltexth->getPreferredLanguage();
	toSend["version"].String() = VCMI_VERSION_STRING;
	sendMessage(toSend);
}

void GlobalLobbyClient::sendClientLogin()
{
	JsonNode toSend;
	toSend["type"].String() = "clientLogin";
	toSend["accountID"] = settings["lobby"]["accountID"];
	toSend["accountCookie"] = settings["lobby"]["accountCookie"];
	toSend["language"].String() = CGI->generaltexth->getPreferredLanguage();
	toSend["version"].String() = VCMI_VERSION_STRING;
	sendMessage(toSend);
}

void GlobalLobbyClient::onConnectionFailed(const std::string & errorMessage)
{
	boost::mutex::scoped_lock interfaceLock(GH.interfaceMutex);

	auto loginWindowPtr = loginWindow.lock();

	if(!loginWindowPtr || !GH.windows().topWindow<GlobalLobbyLoginWindow>())
		throw std::runtime_error("lobby connection failed without active login window!");

	logGlobal->warn("Connection to game lobby failed! Reason: %s", errorMessage);
	loginWindowPtr->onConnectionFailed(errorMessage);
}

void GlobalLobbyClient::onDisconnected(const std::shared_ptr<INetworkConnection> & connection, const std::string & errorMessage)
{
	boost::mutex::scoped_lock interfaceLock(GH.interfaceMutex);

	assert(connection == networkConnection);
	networkConnection.reset();

	while (!GH.windows().findWindows<GlobalLobbyWindow>().empty())
	{
		// if global lobby is open, pop all dialogs on top of it as well as lobby itself
		GH.windows().popWindows(1);
	}

	CInfoWindow::showInfoDialog("Connection to game lobby was lost!", {});
}

void GlobalLobbyClient::sendMessage(const JsonNode & data)
{
	assert(JsonUtils::validate(data, "vcmi:lobbyProtocol/" + data["type"].String(), data["type"].String() + " pack"));
	networkConnection->sendPacket(data.toBytes());
}

void GlobalLobbyClient::sendOpenRoom(const std::string & mode, int playerLimit)
{
	JsonNode toSend;
	toSend["type"].String() = "activateGameRoom";
	toSend["hostAccountID"] = settings["lobby"]["accountID"];
	toSend["roomType"].String() = mode;
	toSend["playerLimit"].Integer() = playerLimit;
	sendMessage(toSend);
}

void GlobalLobbyClient::connect()
{
	std::string hostname = settings["lobby"]["hostname"].String();
	int16_t port = settings["lobby"]["port"].Integer();
	CSH->getNetworkHandler().connectToRemote(*this, hostname, port);
}

bool GlobalLobbyClient::isConnected() const
{
	return networkConnection != nullptr;
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

const std::vector<GlobalLobbyAccount> & GlobalLobbyClient::getActiveAccounts() const
{
	return activeAccounts;
}

const std::vector<GlobalLobbyRoom> & GlobalLobbyClient::getActiveRooms() const
{
	return activeRooms;
}

void GlobalLobbyClient::activateInterface()
{
	if (GH.windows().topWindow<GlobalLobbyWindow>() != nullptr)
	{
		GH.windows().popWindows(1);
		return;
	}

	if (!GH.windows().findWindows<GlobalLobbyWindow>().empty())
		return;

	if (!GH.windows().findWindows<GlobalLobbyLoginWindow>().empty())
		return;

	if (isConnected())
		GH.windows().pushWindow(createLobbyWindow());
	else
		GH.windows().pushWindow(createLoginWindow());
}

void GlobalLobbyClient::sendProxyConnectionLogin(const NetworkConnectionPtr & netConnection)
{
	JsonNode toSend;
	toSend["type"].String() = "clientProxyLogin";
	toSend["accountID"] = settings["lobby"]["accountID"];
	toSend["accountCookie"] = settings["lobby"]["accountCookie"];
	toSend["gameRoomID"] = settings["lobby"]["roomID"];

	assert(JsonUtils::validate(toSend, "vcmi:lobbyProtocol/" + toSend["type"].String(), toSend["type"].String() + " pack"));
	netConnection->sendPacket(toSend.toBytes());
}
