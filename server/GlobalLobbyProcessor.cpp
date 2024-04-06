/*
 * GlobalLobbyProcessor.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "GlobalLobbyProcessor.h"

#include "CVCMIServer.h"
#include "../lib/CConfigHandler.h"
#include "../lib/json/JsonUtils.h"

GlobalLobbyProcessor::GlobalLobbyProcessor(CVCMIServer & owner)
	: owner(owner)
{
	logGlobal->info("Connecting to lobby server");
	establishNewConnection();
}

void GlobalLobbyProcessor::establishNewConnection()
{
	std::string hostname = settings["lobby"]["hostname"].String();
	uint16_t port = settings["lobby"]["port"].Integer();
	owner.getNetworkHandler().connectToRemote(*this, hostname, port);
}

void GlobalLobbyProcessor::onDisconnected(const std::shared_ptr<INetworkConnection> & connection, const std::string & errorMessage)
{
	if (connection == controlConnection)
	{
		owner.setState(EServerState::SHUTDOWN);
		return;
	}
	else
	{
		if (owner.getState() == EServerState::LOBBY)
		{
			for (auto const & proxy : proxyConnections)
			{
				if (proxy.second == connection)
				{
					JsonNode message;
					message["type"].String() = "leaveGameRoom";
					message["accountID"].String() = proxy.first;

					sendMessage(controlConnection, message);
					break;
				}
			}
		}

		// player disconnected
		owner.onDisconnected(connection, errorMessage);
	}
}

void GlobalLobbyProcessor::onPacketReceived(const std::shared_ptr<INetworkConnection> & connection, const std::vector<std::byte> & message)
{
	if (connection == controlConnection)
	{
		JsonNode json(message.data(), message.size());

		if(json["type"].String() == "operationFailed")
			return receiveOperationFailed(json);

		if(json["type"].String() == "serverLoginSuccess")
			return receiveServerLoginSuccess(json);

		if(json["type"].String() == "accountJoinsRoom")
			return receiveAccountJoinsRoom(json);

		logGlobal->error("Received unexpected message from lobby server of type '%s' ", json["type"].String());
	}
	else
	{
		// received game message via proxy connection
		owner.onPacketReceived(connection, message);
	}
}

void GlobalLobbyProcessor::receiveOperationFailed(const JsonNode & json)
{
	logGlobal->info("Lobby: Failed to login into a lobby server!");

	owner.setState(EServerState::SHUTDOWN);
}

void GlobalLobbyProcessor::receiveServerLoginSuccess(const JsonNode & json)
{
	// no-op, wait just for any new commands from lobby
	logGlobal->info("Lobby: Succesfully connected to lobby server");
	owner.startAcceptingIncomingConnections();
}

void GlobalLobbyProcessor::receiveAccountJoinsRoom(const JsonNode & json)
{
	std::string accountID = json["accountID"].String();

	logGlobal->info("Lobby: Account %s will join our room!", accountID);
	assert(proxyConnections.count(accountID) == 0);

	proxyConnections[accountID] = nullptr;
	establishNewConnection();
}

void GlobalLobbyProcessor::onConnectionFailed(const std::string & errorMessage)
{
	owner.setState(EServerState::SHUTDOWN);
}

void GlobalLobbyProcessor::onConnectionEstablished(const std::shared_ptr<INetworkConnection> & connection)
{
	if (controlConnection == nullptr)
	{
		controlConnection = connection;
		logGlobal->info("Connection to lobby server established");

		JsonNode toSend;
		toSend["type"].String() = "serverLogin";
		toSend["gameRoomID"].String() = owner.uuid;
		toSend["accountID"].String() = getHostAccountID();
		toSend["accountCookie"].String() = getHostAccountCookie();
		toSend["version"].String() = VCMI_VERSION_STRING;

		sendMessage(connection, toSend);
	}
	else
	{
		// Proxy connection for a player
		std::string guestAccountID;
		for (auto const & proxies : proxyConnections)
			if (proxies.second == nullptr)
				guestAccountID = proxies.first;

		JsonNode toSend;
		toSend["type"].String() = "serverProxyLogin";
		toSend["gameRoomID"].String() = owner.uuid;
		toSend["guestAccountID"].String() = guestAccountID;
		toSend["accountCookie"].String() = getHostAccountCookie();

		sendMessage(connection, toSend);

		proxyConnections[guestAccountID] = connection;
		owner.onNewConnection(connection);
	}
}

void GlobalLobbyProcessor::sendGameStarted()
{
	JsonNode toSend;
	toSend["type"].String() = "gameStarted";
	sendMessage(controlConnection, toSend);
}

void GlobalLobbyProcessor::sendChangeRoomDescription(const std::string & description)
{
	JsonNode toSend;
	toSend["type"].String() = "changeRoomDescription";
	toSend["description"].String() = description;

	sendMessage(controlConnection, toSend);
}

void GlobalLobbyProcessor::sendMessage(const NetworkConnectionPtr & targetConnection, const JsonNode & toSend)
{
	assert(JsonUtils::validate(toSend, "vcmi:lobbyProtocol/" + toSend["type"].String(), toSend["type"].String() + " pack"));
	targetConnection->sendPacket(toSend.toBytes());
}

//FIXME: consider whether these methods should be replaced with some other way to define our account ID / account cookie
static const std::string & getServerHost()
{
	return settings["lobby"]["hostname"].String();
}

const std::string & GlobalLobbyProcessor::getHostAccountID() const
{
	return persistentStorage["lobby"][getServerHost()]["accountID"].String();
}

const std::string & GlobalLobbyProcessor::getHostAccountCookie() const
{
	return persistentStorage["lobby"][getServerHost()]["accountCookie"].String();
}

const std::string & GlobalLobbyProcessor::getHostAccountDisplayName() const
{
	return persistentStorage["lobby"][getServerHost()]["displayName"].String();
}
