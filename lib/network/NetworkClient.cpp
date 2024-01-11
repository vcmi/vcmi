/*
 * NetworkClient.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "NetworkClient.h"
#include "NetworkConnection.h"

VCMI_LIB_NAMESPACE_BEGIN

NetworkClient::NetworkClient(INetworkClientListener & listener, const std::shared_ptr<NetworkContext> & context)
	: io(context)
	, socket(std::make_shared<NetworkSocket>(*context))
	, listener(listener)
{
}

void NetworkClient::start(const std::string & host, uint16_t port)
{
	if (isConnected())
		throw std::runtime_error("Attempting to connect while already connected!");

	boost::asio::ip::tcp::resolver resolver(*io);
	auto endpoints = resolver.resolve(host, std::to_string(port));

	boost::asio::async_connect(*socket, endpoints, std::bind(&NetworkClient::onConnected, this, _1));
}

void NetworkClient::onConnected(const boost::system::error_code & ec)
{
	if (ec)
	{
		listener.onConnectionFailed(ec.message());
		return;
	}

	connection = std::make_shared<NetworkConnection>(*this, socket);
	connection->start();

	listener.onConnectionEstablished(connection);
}

bool NetworkClient::isConnected() const
{
	return connection != nullptr;
}

void NetworkClient::sendPacket(const std::vector<uint8_t> & message)
{
	connection->sendPacket(message);
}

void NetworkClient::onDisconnected(const std::shared_ptr<INetworkConnection> & connection)
{
	this->connection.reset();
	listener.onDisconnected(connection);
}

void NetworkClient::onPacketReceived(const std::shared_ptr<INetworkConnection> & connection, const std::vector<uint8_t> & message)
{
	listener.onPacketReceived(connection, message);
}


VCMI_LIB_NAMESPACE_END
