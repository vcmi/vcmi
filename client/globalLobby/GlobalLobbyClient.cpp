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
#include "GlobalLobbyObserver.h"
#include "GlobalLobbyWindow.h"

#include "../CServerHandler.h"
#include "../GameEngine.h"
#include "../GameInstance.h"
#include "../gui/WindowHandler.h"
#include "../mainmenu/CMainMenu.h"
#include "../media/ISoundPlayer.h"
#include "../windows/InfoWindows.h"

#include "../../lib/CConfigHandler.h"
#include "../../lib/json/JsonUtils.h"
#include "../../lib/texts/CGeneralTextHandler.h"
#include "../../lib/texts/MetaString.h"
#include "../../lib/texts/TextOperations.h"
#include "../../lib/GameLibrary.h"

GlobalLobbyClient::GlobalLobbyClient()
{
	auto customChannels = settings["lobby"]["languageRooms"].convertTo<std::vector<std::string>>();

	if (customChannels.empty())
	{
		activeChannels.emplace_back("english");
		if (LIBRARY->generaltexth->getPreferredLanguage() != "english")
			activeChannels.emplace_back(LIBRARY->generaltexth->getPreferredLanguage());
	}
	else
	{
		activeChannels = customChannels;
	}
}

void GlobalLobbyClient::addChannel(const std::string & channel)
{
	activeChannels.emplace_back(channel);

	auto lobbyWindowPtr = lobbyWindow.lock();
	if(lobbyWindowPtr)
		lobbyWindowPtr->refreshActiveChannels();

	JsonNode toSend;
	toSend["type"].String() = "requestChatHistory";
	toSend["channelType"].String() = "global";
	toSend["channelName"].String() = channel;
	GAME->server().getGlobalLobby().sendMessage(toSend);

	Settings languageRooms = settings.write["lobby"]["languageRooms"];

	languageRooms->Vector().clear();
	for (const auto & lang : activeChannels)
		languageRooms->Vector().push_back(JsonNode(lang));
}

void GlobalLobbyClient::closeChannel(const std::string & channel)
{
	vstd::erase(activeChannels, channel);

	auto lobbyWindowPtr = lobbyWindow.lock();
	if(lobbyWindowPtr)
		lobbyWindowPtr->refreshActiveChannels();

	Settings languageRooms = settings.write["lobby"]["languageRooms"];

	languageRooms->Vector().clear();
	for (const auto & lang : activeChannels)
		languageRooms->Vector().push_back(JsonNode(lang));
}

GlobalLobbyClient::~GlobalLobbyClient() = default;

void GlobalLobbyClient::onPacketReceived(const std::shared_ptr<INetworkConnection> &, const std::vector<std::byte> & message)
{
	std::scoped_lock interfaceLock(ENGINE->interfaceMutex);

	JsonNode json(message.data(), message.size(), "<lobby network packet>");

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

	if(!loginWindowPtr || !ENGINE->windows().topWindow<GlobalLobbyLoginWindow>())
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

	if(!loginWindowPtr || !ENGINE->windows().topWindow<GlobalLobbyLoginWindow>())
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

		if(lobbyWindowPtr && lobbyWindowPtr->isChannelOpen(channelType, channelName))
			lobbyWindowPtr->onGameChatMessage(message.displayName, message.messageText, message.timeFormatted, channelType, channelName);
	}

	if(lobbyWindowPtr && lobbyWindowPtr->isChannelOpen(channelType, channelName))
		lobbyWindowPtr->refreshChatText();
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
	{
		lobbyWindowPtr->onGameChatMessage(message.displayName, message.messageText, message.timeFormatted, channelType, channelName);
		lobbyWindowPtr->refreshChatText();

		if(channelType == "player" || (lobbyWindowPtr->isChannelOpen(channelType, channelName) && lobbyWindowPtr->isActive()))
			ENGINE->sound().playSound(AudioPath::builtin("CHAT"));
	}
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

	for (auto const & window : ENGINE->windows().findWindows<GlobalLobbyObserver>())
		window->onActiveAccounts(activeAccounts);
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
		room.gameVersion = jsonEntry["version"].String();
		room.modList = ModVerificationInfo::jsonDeserializeList(jsonEntry["mods"]);
		std::chrono::seconds ageSeconds (jsonEntry["ageSeconds"].Integer());
		room.startDateFormatted = TextOperations::getCurrentFormattedDateTimeLocal(-ageSeconds);

		for(const auto & jsonParticipant : jsonEntry["participants"].Vector())
		{
			GlobalLobbyAccount account;
			account.accountID =  jsonParticipant["accountID"].String();
			account.displayName =  jsonParticipant["displayName"].String();
			room.participants.push_back(account);
		}

		for(const auto & jsonParticipant : jsonEntry["invited"].Vector())
		{
			GlobalLobbyAccount account;
			account.accountID =  jsonParticipant["accountID"].String();
			account.displayName =  jsonParticipant["displayName"].String();
			room.invited.push_back(account);
		}

		room.playerLimit = jsonEntry["playerLimit"].Integer();

		activeRooms.push_back(room);
	}

	auto lobbyWindowPtr = lobbyWindow.lock();
	if(lobbyWindowPtr)
		lobbyWindowPtr->onActiveGameRooms(activeRooms);

	for (auto const & window : ENGINE->windows().findWindows<GlobalLobbyObserver>())
		window->onActiveGameRooms(activeRooms);
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
		lobbyWindowPtr->refreshChatText();
	}

	ENGINE->sound().playSound(AudioPath::builtin("CHAT"));
}

void GlobalLobbyClient::receiveJoinRoomSuccess(const JsonNode & json)
{
	if (json["proxyMode"].Bool())
	{
		GAME->server().resetStateForLobby(EStartMode::NEW_GAME, ESelectionScreen::newGame, EServerMode::LOBBY_GUEST, { GAME->server().getGlobalLobby().getAccountDisplayName() });
		GAME->server().loadMode = ELoadMode::MULTI;

		std::string hostname = getServerHost();
		uint16_t port = getServerPort();
		GAME->server().connectToServer(hostname, port);
	}

	// NOTE: must be set after GAME->server().resetStateForLobby call
	currentGameRoomUUID = json["gameRoomID"].String();
}

void GlobalLobbyClient::onConnectionEstablished(const std::shared_ptr<INetworkConnection> & connection)
{
	std::scoped_lock interfaceLock(ENGINE->interfaceMutex);
	networkConnection = connection;

	auto loginWindowPtr = loginWindow.lock();

	if(!loginWindowPtr || !ENGINE->windows().topWindow<GlobalLobbyLoginWindow>())
		throw std::runtime_error("lobby connection established without active login window!");

	loginWindowPtr->onConnectionSuccess();
}

void GlobalLobbyClient::sendClientRegister(const std::string & accountName)
{
	JsonNode toSend;
	toSend["type"].String() = "clientRegister";
	toSend["displayName"].String() = accountName;
	toSend["language"].String() = LIBRARY->generaltexth->getPreferredLanguage();
	toSend["version"].String() = VCMI_VERSION_STRING;
	sendMessage(toSend);
}

void GlobalLobbyClient::sendClientLogin()
{
	JsonNode toSend;
	toSend["type"].String() = "clientLogin";
	toSend["accountID"].String() = getAccountID();
	toSend["accountCookie"].String() = getAccountCookie();
	toSend["language"].String() = LIBRARY->generaltexth->getPreferredLanguage();
	toSend["version"].String() = VCMI_VERSION_STRING;

	for (const auto & language : activeChannels)
		toSend["languageRooms"].Vector().push_back(JsonNode(language));

	sendMessage(toSend);
}

void GlobalLobbyClient::onConnectionFailed(const std::string & errorMessage)
{
	std::scoped_lock interfaceLock(ENGINE->interfaceMutex);

	auto loginWindowPtr = loginWindow.lock();

	if(!loginWindowPtr || !ENGINE->windows().topWindow<GlobalLobbyLoginWindow>())
		throw std::runtime_error("lobby connection failed without active login window!");

	logGlobal->warn("Connection to game lobby failed! Reason: %s", errorMessage);
	loginWindowPtr->onConnectionFailed(errorMessage);
}

void GlobalLobbyClient::onDisconnected(const std::shared_ptr<INetworkConnection> & connection, const std::string & errorMessage)
{
	std::scoped_lock interfaceLock(ENGINE->interfaceMutex);

	assert(connection == networkConnection);
	networkConnection.reset();
	accountLoggedIn = false;

	while (!ENGINE->windows().findWindows<GlobalLobbyWindow>().empty())
	{
		// if global lobby is open, pop all dialogs on top of it as well as lobby itself
		ENGINE->windows().popWindows(1);
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
	GAME->server().getNetworkHandler().connectToRemote(*this, hostname, port);
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
	if(lobbyWindowPtr && ENGINE->screenDimensions().x >= lobbyWindowPtr->pos.w) // if wide window doesn't fit anymore after ingame screen resolution change
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

const GlobalLobbyRoom & GlobalLobbyClient::getActiveRoomByName(const std::string & roomUUID) const
{
	for (auto const & room : activeRooms)
	{
		if (room.gameRoomID == roomUUID)
			return room;
	}

	throw std::out_of_range("Failed to find room with UUID of " + roomUUID);
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
			GAME->server().getGlobalLobby().sendMessage(toSend);
		}
		return emptyVector;
	}
	else
		return chatHistory.at(keyToTest);
}

void GlobalLobbyClient::activateInterface()
{
	if (ENGINE->windows().topWindow<GlobalLobbyWindow>() != nullptr)
	{
		ENGINE->windows().popWindows(1);
		return;
	}

	if (!ENGINE->windows().findWindows<GlobalLobbyWindow>().empty())
		return;

	if (!ENGINE->windows().findWindows<GlobalLobbyLoginWindow>().empty())
		return;

	if (isLoggedIn())
		ENGINE->windows().pushWindow(createLobbyWindow());
	else
		ENGINE->windows().pushWindow(createLoginWindow());

	ENGINE->windows().topWindow<CIntObject>()->center();
}

void GlobalLobbyClient::activateRoomInviteInterface()
{
	ENGINE->windows().createAndPushWindow<GlobalLobbyInviteWindow>();
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

const std::string & GlobalLobbyClient::getCurrentGameRoomID() const
{
	return currentGameRoomUUID;
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

	GAME->server().getGlobalLobby().sendMessage(toSend);
}

bool GlobalLobbyClient::isInvitedToRoom(const std::string & gameRoomID)
{
	if (activeInvites.count(gameRoomID) > 0)
		return true;

	const auto & gameRoom = GAME->server().getGlobalLobby().getActiveRoomByName(gameRoomID);
	for (auto const & invited : gameRoom.invited)
	{
		if (invited.accountID == getAccountID())
			return true;
	}

	return false;
}
