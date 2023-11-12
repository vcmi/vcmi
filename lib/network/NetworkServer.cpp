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

void NetworkServer::start(uint16_t port)
{
	io = std::make_shared<boost::asio::io_service>();
	acceptor = std::make_shared<NetworkAcceptor>(*io, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port));

	startAsyncAccept();
}

void NetworkServer::startAsyncAccept()
{
	std::shared_ptr<NetworkSocket> upcomingConnection = std::make_shared<NetworkSocket>(*io);
	acceptor->async_accept(*upcomingConnection, std::bind(&NetworkServer::connectionAccepted, this, upcomingConnection, _1));
}

void NetworkServer::run()
{
	io->run();
}

void NetworkServer::connectionAccepted(std::shared_ptr<NetworkSocket> upcomingConnection, const boost::system::error_code & ec)
{
	if(ec)
	{
		throw std::runtime_error("Something wrong during accepting: " + ec.message());
	}

	logNetwork->info("We got a new connection! :)");
	auto connection = std::make_shared<NetworkConnection>(upcomingConnection, *this);
	connections.insert(connection);
	connection->start();
	onNewConnection(connection);
	startAsyncAccept();
}

void NetworkServer::sendPacket(const std::shared_ptr<NetworkConnection> & connection, const std::vector<uint8_t> & message)
{
	connection->sendPacket(message);
}

void NetworkServer::onDisconnected(const std::shared_ptr<NetworkConnection> & connection)
{
	assert(connections.count(connection));
	connections.erase(connection);
	onConnectionLost(connection);
}

VCMI_LIB_NAMESPACE_END
