/*
 * NetworkHandler.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "NetworkHandler.h"

#include "NetworkServer.h"
#include "NetworkConnection.h"

VCMI_LIB_NAMESPACE_BEGIN

std::unique_ptr<INetworkHandler> INetworkHandler::createHandler()
{
	return std::make_unique<NetworkHandler>();
}

NetworkHandler::NetworkHandler()
	: io(std::make_unique<NetworkContext>())
{}

std::unique_ptr<INetworkServer> NetworkHandler::createServerTCP(INetworkServerListener & listener)
{
	return std::make_unique<NetworkServer>(listener, *io);
}

void NetworkHandler::connectToRemote(INetworkClientListener & listener, const std::string & host, uint16_t port)
{
	auto socket = std::make_shared<NetworkSocket>(*io);
	auto resolver = std::make_shared<boost::asio::ip::tcp::resolver>(*io);

	resolver->async_resolve(host, std::to_string(port),
	[this, &listener, resolver, socket](const boost::system::error_code& error, const boost::asio::ip::tcp::resolver::results_type & endpoints)
	{
		if (error)
		{
			listener.onConnectionFailed(error.message());
			return;
		}

		boost::asio::async_connect(*socket, endpoints, [this, socket, &listener](const boost::system::error_code& error, const boost::asio::ip::tcp::endpoint& endpoint)
		{
			if (error)
			{
				listener.onConnectionFailed(error.message());
				return;
			}
			auto connection = std::make_shared<NetworkConnection>(listener, socket, *io);
			connection->start();

			listener.onConnectionEstablished(connection);
		});
	});
}

void NetworkHandler::run()
{
	boost::asio::executor_work_guard<decltype(io->get_executor())> work{io->get_executor()};
	io->run();
}

void NetworkHandler::createTimer(INetworkTimerListener & listener, std::chrono::milliseconds duration)
{
	auto timer = std::make_shared<NetworkTimer>(*io, duration);
	timer->async_wait([&listener, timer](const boost::system::error_code& error){
		if (!error)
			listener.onTimer();
	});
}

void NetworkHandler::createInternalConnection(INetworkClientListener & listener, INetworkServer & server)
{
	auto localConnection = std::make_shared<InternalConnection>(listener, *io);

	server.receiveInternalConnection(localConnection);

	boost::asio::post(*io, [&listener, localConnection](){
		listener.onConnectionEstablished(localConnection);
	});
}

void NetworkHandler::stop()
{
	io->stop();
}

VCMI_LIB_NAMESPACE_END
