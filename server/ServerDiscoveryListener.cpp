
/*
 * ServerDiscoveryListener.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"

#include "CVCMIServer.h"
#include "ServerDiscoveryListener.h"

ServerDiscoveryListener::ServerDiscoveryListener(CVCMIServer & server, unsigned short port) : server(server), port(port), running(false) {}

ServerDiscoveryListener::~ServerDiscoveryListener()
{
	stop();
}

void ServerDiscoveryListener::start()
{
	if(running)
		return;
	running = true;
	thread = std::make_unique<std::thread>(
		[this]()
		{
			listen();
		}
	);
}

void ServerDiscoveryListener::stop()
{
	running = false;

	if(socket)
		socket->close();

	ioContext.stop();

	if(thread && thread->joinable())
		thread->join();
}

void ServerDiscoveryListener::listen()
{
	try
	{
		running = true;

		boost::asio::ip::udp::endpoint listenEndpoint(boost::asio::ip::udp::v4(), port);

		socket = std::make_unique<boost::asio::ip::udp::socket>(ioContext);
		socket->open(listenEndpoint.protocol());
		socket->bind(listenEndpoint);

		socket->async_receive_from(
			boost::asio::buffer(recvBuffer),
			remoteEndpoint,
			[this](const boost::system::error_code & ec, std::size_t len)
			{
				if(!running || ec == boost::asio::error::operation_aborted)
					return;

				if(!ec && len > 0)
				{
					std::string msg(recvBuffer, recvBuffer + len);

					if(msg == "VCMI_DISCOVERY" && server.getState() == EServerState::LOBBY)
					{
						std::string response = "VCMI_SERVER:" + std::to_string(server.getPort());

						socket->send_to(boost::asio::buffer(response), remoteEndpoint);
					}
				}

				if(running)
					listen();
			}
		);

		ioContext.run();
	}
	catch(const std::exception & e)
	{
		logNetwork->error("Discovery listener error: %s", e.what());
	}
}
