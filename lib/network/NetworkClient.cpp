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

NetworkClient::NetworkClient()
	: io(new NetworkService)
	, socket(new NetworkSocket(*io))
//	, timer(new NetworkTimer(*io))
{
}

void NetworkClient::start(const std::string & host, uint16_t port)
{
	boost::asio::ip::tcp::resolver resolver(*io);
	auto endpoints = resolver.resolve(host, std::to_string(port));

	boost::asio::async_connect(*socket, endpoints, std::bind(&NetworkClient::onConnected, this, _1));
}

void NetworkClient::onConnected(const boost::system::error_code & ec)
{
	if (ec)
	{
		onConnectionFailed(ec.message());
		return;
	}

	connection = std::make_shared<NetworkConnection>(socket, *this);
	connection->start();
}

void NetworkClient::run()
{
	io->run();
}

void NetworkClient::poll()
{
	io->poll();
}

void NetworkClient::sendPacket(const std::vector<uint8_t> & message)
{
	connection->sendPacket(message);
}

void NetworkClient::onDisconnected(const std::shared_ptr<NetworkConnection> & connection)
{
	onDisconnected();
}

void NetworkClient::onPacketReceived(const std::shared_ptr<NetworkConnection> & connection, const std::vector<uint8_t> & message)
{
	onPacketReceived(message);
}


VCMI_LIB_NAMESPACE_END
