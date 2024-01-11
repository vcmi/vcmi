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

#include "NetworkClient.h"
#include "NetworkServer.h"

VCMI_LIB_NAMESPACE_BEGIN

std::unique_ptr<INetworkHandler> INetworkHandler::createHandler()
{
	return std::make_unique<NetworkHandler>();
}

NetworkHandler::NetworkHandler()
	: io(std::make_shared<NetworkContext>())
{}

std::unique_ptr<INetworkServer> NetworkHandler::createServerTCP(INetworkServerListener & listener)
{
	return std::make_unique<NetworkServer>(listener, io);
}

std::unique_ptr<INetworkClient> NetworkHandler::createClientTCP(INetworkClientListener & listener)
{
	return std::make_unique<NetworkClient>(listener, io);
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

void NetworkHandler::stop()
{
	io->stop();
}

VCMI_LIB_NAMESPACE_END
