/*
 * NetworkServer.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "NetworkServer.h"
#include "NetworkConnection.h"

VCMI_LIB_NAMESPACE_BEGIN

NetworkServer::NetworkServer(INetworkServerListener & listener, const std::shared_ptr<NetworkContext> & context)
	: io(context)
	, listener(listener)
{
}

void NetworkServer::start(uint16_t port)
{
	acceptor = std::make_shared<NetworkAcceptor>(*io, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port));
	startAsyncAccept();
}

void NetworkServer::startAsyncAccept()
{
	auto upcomingConnection = std::make_shared<NetworkSocket>(*io);
	acceptor->async_accept(*upcomingConnection, [this, upcomingConnection](const auto & ec) { connectionAccepted(upcomingConnection, ec); });
}

void NetworkServer::connectionAccepted(std::shared_ptr<NetworkSocket> upcomingConnection, const boost::system::error_code & ec)
{
	if(ec)
	{
		throw std::runtime_error("Something wrong during accepting: " + ec.message());
	}

	logNetwork->info("We got a new connection! :)");
	auto connection = std::make_shared<NetworkConnection>(*this, upcomingConnection);
	connections.insert(connection);
	connection->start();
	listener.onNewConnection(connection);
	startAsyncAccept();
}

void NetworkServer::onDisconnected(const std::shared_ptr<INetworkConnection> & connection, const std::string & errorMessage)
{
	logNetwork->info("Connection lost! Reason: %s", errorMessage);
	assert(connections.count(connection));
	connections.erase(connection);
	listener.onDisconnected(connection, errorMessage);
}

void NetworkServer::onPacketReceived(const std::shared_ptr<INetworkConnection> & connection, const std::vector<std::byte> & message)
{
	listener.onPacketReceived(connection, message);
}

VCMI_LIB_NAMESPACE_END
