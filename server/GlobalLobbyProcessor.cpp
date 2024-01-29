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

GlobalLobbyProcessor::GlobalLobbyProcessor(CVCMIServer & owner)
	: owner(owner)
{
	logGlobal->info("Connecting to lobby server");
	establishNewConnection();
}

void GlobalLobbyProcessor::establishNewConnection()
{
	std::string hostname = settings["lobby"]["hostname"].String();
	int16_t port = settings["lobby"]["port"].Integer();
	owner.networkHandler->connectToRemote(*this, hostname, port);
}

void GlobalLobbyProcessor::onDisconnected(const std::shared_ptr<INetworkConnection> & connection)
{
	throw std::runtime_error("Lost connection to a lobby server!");
}

void GlobalLobbyProcessor::onPacketReceived(const std::shared_ptr<INetworkConnection> & connection, const std::vector<uint8_t> & message)
{
	if (connection == controlConnection)
	{
		JsonNode json(message.data(), message.size());

		if(json["type"].String() == "loginFailed")
			return receiveLoginFailed(json);

		if(json["type"].String() == "loginSuccess")
			return receiveLoginSuccess(json);

		if(json["type"].String() == "accountJoinsRoom")
			return receiveAccountJoinsRoom(json);

		throw std::runtime_error("Received unexpected message from lobby server: " + json["type"].String());
	}
	else
	{
		// received game message via proxy connection
		owner.onPacketReceived(connection, message);
	}
}

void GlobalLobbyProcessor::receiveLoginFailed(const JsonNode & json)
{
	logGlobal->info("Lobby: Failed to login into a lobby server!");

	throw std::runtime_error("Failed to login into a lobby server!");
}

void GlobalLobbyProcessor::receiveLoginSuccess(const JsonNode & json)
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
	throw std::runtime_error("Failed to connect to a lobby server!");
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
		toSend["accountID"] = settings["lobby"]["accountID"];
		toSend["accountCookie"] = settings["lobby"]["accountCookie"];
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
		toSend["accountCookie"] = settings["lobby"]["accountCookie"];
		sendMessage(connection, toSend);

		proxyConnections[guestAccountID] = connection;
		owner.onNewConnection(connection);
	}
}

void GlobalLobbyProcessor::sendMessage(const NetworkConnectionPtr & target, const JsonNode & data)
{
	std::string payloadString = data.toJson(true);

	// FIXME: find better approach
	uint8_t * payloadBegin = reinterpret_cast<uint8_t *>(payloadString.data());
	uint8_t * payloadEnd = payloadBegin + payloadString.size();

	std::vector<uint8_t> payloadBuffer(payloadBegin, payloadEnd);

	target->sendPacket(payloadBuffer);
}
