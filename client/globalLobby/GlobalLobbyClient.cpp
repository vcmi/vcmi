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

#include "GlobalLobbyInviteWindow.h"
#include "GlobalLobbyLoginWindow.h"
#include "GlobalLobbyWindow.h"

#include "../CGameInfo.h"
#include "../CMusicHandler.h"
#include "../CServerHandler.h"
#include "../gui/CGuiHandler.h"
#include "../gui/WindowHandler.h"
#include "../mainmenu/CMainMenu.h"
#include "../windows/InfoWindows.h"

#include "../../lib/CConfigHandler.h"
#include "../../lib/MetaString.h"
#include "../../lib/json/JsonUtils.h"
#include "../../lib/TextOperations.h"
#include "../../lib/CGeneralTextHandler.h"

GlobalLobbyClient::GlobalLobbyClient()
{
	activeChannels.emplace_back("english");
	if (CGI->generaltexth->getPreferredLanguage() != "english")
		activeChannels.emplace_back(CGI->generaltexth->getPreferredLanguage());
}

GlobalLobbyClient::~GlobalLobbyClient() = default;

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

	if(json["type"].String() == "matchesHistory")
		return receiveMatchesHistory(json);

	logGlobal->error("Received unexpected message from lobby server: %s", json["type"].String());
}

void GlobalLobbyClient::receiveAccountCreated(const JsonNode & json)
{
	auto loginWindowPtr = loginWindow.lock();

	if(!loginWindowPtr || !GH.windows().topWindow<GlobalLobbyLoginWindow>())
		throw std::runtime_error("lobby connection finished without active login window!");

	{
		setAccountID(json["accountID"].String());
		setAccountDisplayName(json["displayName"].String());
		setAccountCookie(json["accountCookie"].String());
	}

	sendClientLogin();
}

void GlobalLobbyClient::receiveOperationFailed(const JsonNode & json)
{
	auto loginWindowPtr = loginWindow.lock();

	if(loginWindowPtr)
		loginWindowPtr->onConnectionFailed(json["reason"].String());

	logGlobal->warn("Operation failed! Reason: %s", json["reason"].String());
	// TODO: handle errors in lobby menu
}

void GlobalLobbyClient::receiveClientLoginSuccess(const JsonNode & json)
{
	accountLoggedIn = true;
	setAccountDisplayName(json["displayName"].String());
	setAccountCookie(json["accountCookie"].String());

	auto loginWindowPtr = loginWindow.lock();

	if(!loginWindowPtr || !GH.windows().topWindow<GlobalLobbyLoginWindow>())
		throw std::runtime_error("lobby connection finished without active login window!");

	loginWindowPtr->onLoginSuccess();
}

void GlobalLobbyClient::receiveChatHistory(const JsonNode & json)
{
	std::string channelType = json["channelType"].String();
	std::string channelName = json["channelName"].String();
	std::string channelKey = channelType + '_' + channelName;

	// create empty entry, potentially replacing any pre-existing data
	chatHistory[channelKey] = {};

	auto lobbyWindowPtr = lobbyWindow.lock();

	for(const auto & entry : json["messages"].Vector())
	{
		GlobalLobbyChannelMessage message;

		message.accountID = entry["accountID"].String();
		message.displayName = entry["displayName"].String();
		message.messageText = entry["messageText"].String();
		std::chrono::seconds ageSeconds (entry["ageSeconds"].Integer());
		message.timeFormatted = TextOperations::getCurrentFormattedTimeLocal(-ageSeconds);

		chatHistory[channelKey].push_back(message);

		if(lobbyWindowPtr)
			lobbyWindowPtr->onGameChatMessage(message.displayName, message.messageText, message.timeFormatted, channelType, channelName);
	}
}

void GlobalLobbyClient::receiveChatMessage(const JsonNode & json)
{
	GlobalLobbyChannelMessage message;

	message.accountID = json["accountID"].String();
	message.displayName = json["displayName"].String();
	message.messageText = json["messageText"].String();
	message.timeFormatted = TextOperations::getCurrentFormattedTimeLocal();

	std::string channelType = json["channelType"].String();
	std::string channelName = json["channelName"].String();
	std::string channelKey = channelType + '_' + channelName;

	chatHistory[channelKey].push_back(message);

	auto lobbyWindowPtr = lobbyWindow.lock();
	if(lobbyWindowPtr)
		lobbyWindowPtr->onGameChatMessage(message.displayName, message.messageText, message.timeFormatted, channelType, channelName);

	CCS->soundh->playSound(AudioPath::builtin("CHAT"));
}

void GlobalLobbyClient::receiveActiveAccounts(const JsonNode & json)
{
	activeAccounts.clear();

	for(const auto & jsonEntry : json["accounts"].Vector())
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

	for(const auto & jsonEntry : json["gameRooms"].Vector())
	{
		GlobalLobbyRoom room;

		room.gameRoomID = jsonEntry["gameRoomID"].String();
		room.hostAccountID = jsonEntry["hostAccountID"].String();
		room.hostAccountDisplayName = jsonEntry["hostAccountDisplayName"].String();
		room.description = jsonEntry["description"].String();
		room.statusID = jsonEntry["status"].String();
		std::chrono::seconds ageSeconds (jsonEntry["ageSeconds"].Integer());
		room.startDateFormatted = TextOperations::getCurrentFormattedDateTimeLocal(-ageSeconds);

		for(const auto & jsonParticipant : jsonEntry["participants"].Vector())
		{
			GlobalLobbyAccount account;
			account.accountID =  jsonParticipant["accountID"].String();
			account.displayName =  jsonParticipant["displayName"].String();
			room.participants.push_back(account);
		}
		room.playerLimit = jsonEntry["playerLimit"].Integer();

		activeRooms.push_back(room);
	}

	auto lobbyWindowPtr = lobbyWindow.lock();
	if(lobbyWindowPtr)
		lobbyWindowPtr->onActiveRooms(activeRooms);
}

void GlobalLobbyClient::receiveMatchesHistory(const JsonNode & json)
{
	matchesHistory.clear();

	for(const auto & jsonEntry : json["matchesHistory"].Vector())
	{
		GlobalLobbyRoom room;

		room.gameRoomID = jsonEntry["gameRoomID"].String();
		room.hostAccountID = jsonEntry["hostAccountID"].String();
		room.hostAccountDisplayName = jsonEntry["hostAccountDisplayName"].String();
		room.description = jsonEntry["description"].String();
		room.statusID = jsonEntry["status"].String();
		std::chrono::seconds ageSeconds (jsonEntry["ageSeconds"].Integer());
		room.startDateFormatted = TextOperations::getCurrentFormattedDateTimeLocal(-ageSeconds);

		for(const auto & jsonParticipant : jsonEntry["participants"].Vector())
		{
			GlobalLobbyAccount account;
			account.accountID =  jsonParticipant["accountID"].String();
			account.displayName =  jsonParticipant["displayName"].String();
			room.participants.push_back(account);
		}
		room.playerLimit = jsonEntry["playerLimit"].Integer();

		matchesHistory.push_back(room);
	}

	auto lobbyWindowPtr = lobbyWindow.lock();
	if(lobbyWindowPtr)
		lobbyWindowPtr->onMatchesHistory(matchesHistory);
}

void GlobalLobbyClient::receiveInviteReceived(const JsonNode & json)
{
	auto lobbyWindowPtr = lobbyWindow.lock();
	std::string gameRoomID = json["gameRoomID"].String();
	std::string accountID = json["accountID"].String();

	activeInvites.insert(gameRoomID);
	if(lobbyWindowPtr)
	{
		std::string message = MetaString::createFromTextID("vcmi.lobby.invite.notification").toString();
		std::string time = TextOperations::getCurrentFormattedTimeLocal();

		lobbyWindowPtr->onGameChatMessage("System", message, time, "player", accountID);
		lobbyWindowPtr->onInviteReceived(gameRoomID);
	}

	CCS->soundh->playSound(AudioPath::builtin("CHAT"));
}

void GlobalLobbyClient::receiveJoinRoomSuccess(const JsonNode & json)
{
	if (json["proxyMode"].Bool())
	{
		CSH->resetStateForLobby(EStartMode::NEW_GAME, ESelectionScreen::newGame, EServerMode::LOBBY_GUEST, {});
		CSH->loadMode = ELoadMode::MULTI;

		std::string hostname = getServerHost();
		uint16_t port = getServerPort();
		CSH->connectToServer(hostname, port);
	}

	// NOTE: must be set after CSH->resetStateForLobby call
	currentGameRoomUUID = json["gameRoomID"].String();
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
	toSend["accountID"].String() = getAccountID();
	toSend["accountCookie"].String() = getAccountCookie();
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
	accountLoggedIn = false;

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
	toSend["hostAccountID"].String() = getAccountID();
	toSend["roomType"].String() = mode;
	toSend["playerLimit"].Integer() = playerLimit;
	sendMessage(toSend);
}

void GlobalLobbyClient::connect()
{
	std::string hostname = getServerHost();
	uint16_t port = getServerPort();
	CSH->getNetworkHandler().connectToRemote(*this, hostname, port);
}

bool GlobalLobbyClient::isLoggedIn() const
{
	return networkConnection != nullptr && accountLoggedIn;
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

const std::vector<std::string> & GlobalLobbyClient::getActiveChannels() const
{
	return activeChannels;
}

const std::vector<GlobalLobbyRoom> & GlobalLobbyClient::getMatchesHistory() const
{
	return matchesHistory;
}

const std::vector<GlobalLobbyChannelMessage> & GlobalLobbyClient::getChannelHistory(const std::string & channelType, const std::string & channelName) const
{
	static const std::vector<GlobalLobbyChannelMessage> emptyVector;

	std::string keyToTest = channelType + '_' + channelName;

	if (chatHistory.count(keyToTest) == 0)
	{
		if (channelType != "global")
		{
			JsonNode toSend;
			toSend["type"].String() = "requestChatHistory";
			toSend["channelType"].String() = channelType;
			toSend["channelName"].String() = channelName;
			CSH->getGlobalLobby().sendMessage(toSend);
		}
		return emptyVector;
	}
	else
		return chatHistory.at(keyToTest);
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

	if (isLoggedIn())
		GH.windows().pushWindow(createLobbyWindow());
	else
		GH.windows().pushWindow(createLoginWindow());
}

void GlobalLobbyClient::activateRoomInviteInterface()
{
	GH.windows().createAndPushWindow<GlobalLobbyInviteWindow>();
}

void GlobalLobbyClient::setAccountID(const std::string & accountID)
{
	Settings configID = persistentStorage.write["lobby"][getServerHost()]["accountID"];
	configID->String() = accountID;
}

void GlobalLobbyClient::setAccountCookie(const std::string & accountCookie)
{
	Settings configCookie = persistentStorage.write["lobby"][getServerHost()]["accountCookie"];
	configCookie->String() = accountCookie;
}

void GlobalLobbyClient::setAccountDisplayName(const std::string & accountDisplayName)
{
	Settings configName = persistentStorage.write["lobby"][getServerHost()]["displayName"];
	configName->String() = accountDisplayName;
}

const std::string & GlobalLobbyClient::getAccountID() const
{
	return persistentStorage["lobby"][getServerHost()]["accountID"].String();
}

const std::string & GlobalLobbyClient::getAccountCookie() const
{
	return persistentStorage["lobby"][getServerHost()]["accountCookie"].String();
}

const std::string & GlobalLobbyClient::getAccountDisplayName() const
{
	return persistentStorage["lobby"][getServerHost()]["displayName"].String();
}

const std::string & GlobalLobbyClient::getServerHost() const
{
	return settings["lobby"]["hostname"].String();
}

uint16_t GlobalLobbyClient::getServerPort() const
{
	return settings["lobby"]["port"].Integer();
}

void GlobalLobbyClient::sendProxyConnectionLogin(const NetworkConnectionPtr & netConnection)
{
	JsonNode toSend;
	toSend["type"].String() = "clientProxyLogin";
	toSend["accountID"].String() = getAccountID();
	toSend["accountCookie"].String() = getAccountCookie();
	toSend["gameRoomID"].String() = currentGameRoomUUID;

	assert(JsonUtils::validate(toSend, "vcmi:lobbyProtocol/" + toSend["type"].String(), toSend["type"].String() + " pack"));
	netConnection->sendPacket(toSend.toBytes());
}

void GlobalLobbyClient::resetMatchState()
{
	currentGameRoomUUID.clear();
}

void GlobalLobbyClient::sendMatchChatMessage(const std::string & messageText)
{
	if (!isLoggedIn())
		return; // we are not playing with lobby

	if (currentGameRoomUUID.empty())
		return; // we are not playing through lobby

	JsonNode toSend;
	toSend["type"].String() = "sendChatMessage";
	toSend["channelType"].String() = "match";
	toSend["channelName"].String() = currentGameRoomUUID;
	toSend["messageText"].String() = messageText;

	assert(TextOperations::isValidUnicodeString(messageText));

	CSH->getGlobalLobby().sendMessage(toSend);
}

bool GlobalLobbyClient::isInvitedToRoom(const std::string & gameRoomID)
{
	return activeInvites.count(gameRoomID) > 0;
}
