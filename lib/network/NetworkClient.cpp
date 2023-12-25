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

DLL_LINKAGE bool checkNetworkPortIsFree(const std::string & host, uint16_t port)
{
	boost::asio::io_service io;
	NetworkAcceptor acceptor(io);

	boost::system::error_code ec;
	acceptor.open(boost::asio::ip::tcp::v4(), ec);
	if (ec)
		return false;

	acceptor.bind(boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port), ec);
	if (ec)
		return false;

	return true;
}

NetworkClient::NetworkClient(INetworkClientListener & listener)
	: io(new NetworkService)
	, socket(new NetworkSocket(*io))
	, listener(listener)
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
		listener.onConnectionFailed(ec.message());
		return;
	}

	connection = std::make_shared<NetworkConnection>(socket, *this);
	connection->start();

	listener.onConnectionEstablished(connection);
}

void NetworkClient::run()
{
	boost::asio::executor_work_guard<decltype(io->get_executor())> work{io->get_executor()};
	io->run();
}

void NetworkClient::poll()
{
	io->poll();
}

void NetworkClient::stop()
{
	io->stop();
}

void NetworkClient::sendPacket(const std::vector<uint8_t> & message)
{
	connection->sendPacket(message);
}

void NetworkClient::onDisconnected(const std::shared_ptr<NetworkConnection> & connection)
{
	listener.onDisconnected(connection);
}

void NetworkClient::onPacketReceived(const std::shared_ptr<NetworkConnection> & connection, const std::vector<uint8_t> & message)
{
	listener.onPacketReceived(connection, message);
}


VCMI_LIB_NAMESPACE_END
